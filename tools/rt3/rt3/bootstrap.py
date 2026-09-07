"""Attivare una Epic: quali sessioni servono, quali ci sono, cosa manca.

Modulo PURO. Riceve roadmap, grafo, stato e snapshot runtime; ritorna un piano. Non apre
il database, non avvia processi, non crea worktree, non lancia terminali.

⛔ Questo modulo **non provisiona nulla**. Dice quali terminali servono e stampa il
comando da eseguire; ad aprirli e' una persona. La differenza non e' pigrizia: aprire un
terminale significa decidere dove, con quale identita' e su quale albero, e sono decisioni
che il planner non ha gli elementi per prendere.

## REQUIRED non e' ACTIVE

🔴 La distinzione che tiene onesto `epic check`. Il planner puo' dire che serve un
secondo DEV; finche' nessuno apre quel terminale la sessione NON esiste. Marcarla ACTIVE
perche' e' stata suggerita trasformerebbe la verifica in una fotografia dei desideri.

    REQUIRED   il piano dice che serve
    ACTIVE     il control plane la vede registrata e viva
    MISSING    serve e non c'e'
    BLOCKED    serve, ma qualcosa ne impedisce l'apertura
"""

import collections
import os

from .model import SESSION_REQUIREMENT_STATES

#: Convenzione deterministica dei SessionId. Deterministica perche' due persone che
#: leggono lo stesso piano devono aprire terminali con lo stesso nome: un id inventato
#: al momento rende `epic check` cieco alla sessione che pure e' stata aperta.
#:
#:     EDITOR-<LANE>        uno per lane
#:     <PREFISSO>-<N>       i DEV, numerati
#:     VALIDATOR-<SUFFISSO> uno per lane
_DEV_PREFIX = {"MAIN": "DEV-MAIN", "DEV": "DEV", "DESIGNER": "DESIGN-DEV"}


def session_ids(lane, epic_id=None):
    """I nomi canonici per una lane. `epic_id` entra solo se distingue davvero."""
    suffisso = (epic_id or lane).replace("EPIC-", "")
    return {
        "editor": "EDITOR-{}".format(lane),
        "validator": "VALIDATOR-{}".format(suffisso),
        "dev": lambda n: "{}-{}-{}".format(_DEV_PREFIX.get(lane, "DEV"), suffisso, n),
    }


#: `resource_mode` e' la modalita' decisa dal PLANNER, copiata senza reinterpretazione.
#: Esiste perche' la consistenza fra le due viste sia verificabile invece che promessa:
#: se un giorno bootstrap ricominciasse a correggere il planner, un test se ne accorge.
Requirement = collections.namedtuple(
    "Requirement",
    "session_id role lane workspace_group write_mode issue reason state detail "
    "resource_mode",
)


def terminal_plan(roadmap, graph, states=None, modes=None, runtime=None, epic_id=None,
                  sessions=None):
    """Le sessioni necessarie per lavorare una Epic ADESSO.

    «Adesso» e' la parola importante: il piano dipende da quante issue sono READY in
    questo momento e da chi tiene le risorse. Una Epic con una sola issue pronta vuole
    tre terminali; la stessa Epic dopo due validazioni puo' volerne quattro.
    """
    from .planner import plan as build_plan
    from .planner import readiness

    epiche = {e.id: e for e in roadmap.epics}
    if epic_id and epic_id not in epiche:
        from .errors import ItemNotFound

        raise ItemNotFound(
            "la roadmap {} non contiene l'epic {!r}. Presenti: {}.".format(
                roadmap.id, epic_id, ", ".join(sorted(epiche))
            )
        )
    scelte = [epiche[epic_id]] if epic_id else list(roadmap.epics)

    piano = build_plan(roadmap, graph, states, modes, runtime=runtime)
    stati = readiness(roadmap, graph, states)
    assegnate = {a["key"]: a for a in piano["assignments"]}
    attive = {s["sessionId"]: s for s in (runtime or {}).get("sessions", [])
              if s.get("status") == "ACTIVE"}
    if sessions:
        attive.update({s["session_id"]: s for s in sessions if s.get("status") == "ACTIVE"})

    requisiti, note = [], []
    # Chi tiene il writer PERMANENTE, secondo il piano. Non «tutti i writer del
    # gruppo»: una lane puo' avere scrittori in alberi temporanei, e nominarli come
    # occupanti del permanente fa dire a una sessione che il writer e' occupato da se'.
    proprietari = {
        gruppo: v["holder"]
        for gruppo, v in (piano.get("capacity") or {}).get("writers", {}).items()
        if v.get("holder")
    }
    for epic in scelte:
        lane = epic.home_work
        nomi = session_ids(lane, epic.id)
        mie = [k for k in epic.issue_keys]
        ready = [k for k in mie if stati[k].state == "READY"]
        in_corso = [k for k in mie if stati[k].state == "IN_PROGRESS"]

        if not ready and not in_corso:
            note.append(
                "{}: nessuna issue READY o IN_PROGRESS - non servono terminali adesso.".format(epic.id)
            )
            continue

        # EDITOR e VALIDATION: uno per lane, sempre, finche' c'e' lavoro.
        requisiti.append(_req(nomi["editor"], "EDITOR", lane, "READ_ONLY", None,
                              "coordina la lane e riceve TASK_READY", attive))
        requisiti.append(_req(nomi["validator"], "VALIDATION", lane, "READ_ONLY", None,
                              "riceve VALIDATION_REQUESTED della lane", attive))

        # I DEV: uno per ogni issue che il piano assegna a questa lane.
        #
        # 🔴 La modalita' NON si ricalcola qui. Il planner ha gia' deciso se quella
        # issue va nel writer permanente o in un worktree temporaneo, e con `owner`
        # dice pure a quale sessione. Reinterpretarlo produrrebbe due verita' su una
        # sola decisione: quella del planner e quella che l'utente legge.
        numero = 0
        for key in [k for k in ready + in_corso if k in assegnate]:
            a = assegnate[key]
            numero += 1
            # L'id lo porta il piano quando la risorsa ha gia' un padrone; altrimenti
            # e' una sessione da aprire, e il nome lo genera la convenzione.
            sid = a.get("ownerSessionId") or nomi["dev"](numero)
            r = _req(sid, "DEV", lane, "WRITER", key.split("/")[-1],
                     "implementa {}".format(key), attive, resource_mode=a["mode"])
            if a["mode"] == "TEMPORARY_WORKTREE_SUGGESTED":
                # ⛔ Il worktree NON viene creato: allocation automatica e' fuori scope.
                r = r._replace(
                    detail="il writer permanente di {} e' occupato{}: questa sessione "
                    "richiede un worktree TEMPORANEO, da creare a mano.".format(
                        lane, " da " + proprietari[lane] if lane in proprietari else ""
                    ),
                    reason="TEMPORARY_WORKTREE_REQUIRED",
                )
            requisiti.append(r)

    return {
        "roadmapId": roadmap.id,
        "epics": [e.id for e in scelte],
        "requirements": [r._asdict() for r in requisiti],
        "notes": note,
        "writerOwners": proprietari,
        "requiredNow": len(requisiti),
        "active": sum(1 for r in requisiti if r.state == "ACTIVE"),
        "missing": sum(1 for r in requisiti if r.state == "MISSING"),
        "ready": all(r.state == "ACTIVE" for r in requisiti) and bool(requisiti),
    }


def _req(session_id, role, lane, write_mode, issue, reason, attive, detail=None,
         resource_mode=None):
    """Costruisce un requisito, decidendo lo stato dai FATTI.

    🔴 `ACTIVE` solo se il control plane la vede registrata e viva. Un requisito non
    diventa attivo perche' e' stato scritto in un piano.
    """
    stato = "ACTIVE" if session_id in attive else "MISSING"
    return Requirement(
        session_id=session_id,
        role=role,
        lane=lane,
        workspace_group=lane,
        write_mode=write_mode,
        issue=issue,
        reason=reason,
        state=stato,
        detail=detail,
        resource_mode=resource_mode,
    )


def command_for(req):
    """Il comando CLI REALE da eseguire per aprire quel terminale.

    ⚠️ Deve corrispondere alla sintassi che `rt3 session start` accetta davvero.
    Stampare un comando inventato e' peggio che non stamparlo: chi lo copia scopre
    l'errore solo dopo, e non sa se sbagliava il comando o il piano.
    """
    parti = [
        "rt3 session start",
        "--id {}".format(req["session_id"]),
        "--role {}".format(req["role"]),
        "--lane {}".format(req["lane"]),
        "--workspace-group {}".format(req["workspace_group"]),
        "--write-mode {}".format(req["write_mode"]),
    ]
    if req.get("issue"):
        parti.append("--task {}".format(req["issue"]))
    return " ".join(parti)


def managed_command_for(req, cwd=None):
    """Il comando che apre una finestra GESTITA per quel requisito.

    Ritorna `None` quando non c'e' un `cwd` valido, e allora si mostra solo il comando
    manuale.

    ⛔ §41: non si inventa un path. Un requisito che chiede un worktree TEMPORANEO non
    ha ancora una directory - quel worktree nessuno l'ha creato - e stampare un comando
    con un path che non esiste manderebbe chi lo copia contro un errore, dopo.
    """
    if req.get("reason") == "TEMPORARY_WORKTREE_REQUIRED":
        return None
    if not cwd or not os.path.isdir(cwd):
        return None
    parti = [
        "rt3 terminal launch",
        "--id {}".format(req["session_id"]),
        "--role {}".format(req["role"]),
        "--lane {}".format(req["lane"]),
        "--workspace-group {}".format(req["workspace_group"]),
        "--write-mode {}".format(req["write_mode"]),
    ]
    if req.get("issue"):
        parti.append("--task {}".format(req["issue"]))
    parti.append("--worktree {}".format(cwd))
    return " ".join(parti)


def check(piano):
    """Quante delle sessioni richieste esistono davvero."""
    reqs = piano["requirements"]
    attive = [r for r in reqs if r["state"] == "ACTIVE"]
    return {
        "roadmapId": piano["roadmapId"],
        "epics": piano["epics"],
        "total": len(reqs),
        "active": len(attive),
        "missing": [r["session_id"] for r in reqs if r["state"] != "ACTIVE"],
        "ready": bool(reqs) and len(attive) == len(reqs),
        "requirements": reqs,
    }


def render_plan(piano, cwd=None):
    righe = []
    titolo = ", ".join(piano["epics"]) if piano["epics"] else piano["roadmapId"]
    righe.append("{} - TERMINAL SETUP".format(titolo))
    righe.append("")
    righe.append("Required now: {}".format(piano["requiredNow"]))
    righe.append("")
    for i, r in enumerate(piano["requirements"], 1):
        righe.append("{}. {}   [{}]".format(i, r["session_id"], r["state"]))
        righe.append("   Role: {}".format(r["role"]))
        righe.append("   Lane: {}".format(r["lane"]))
        righe.append("   Mode: {}".format(r["write_mode"]))
        if r.get("issue"):
            righe.append("   Task: {}".format(r["issue"]))
        if r.get("detail"):
            righe.append("   ! {}".format(r["detail"]))
            righe.append("   ! Manual worktree setup required.")
        managed = managed_command_for(r, cwd)
        if managed:
            righe.append("   Manual:")
            righe.append("     $ {}".format(command_for(r)))
            righe.append("   Managed (RT3 apre e chiude la finestra):")
            righe.append("     $ {}".format(managed))
        else:
            righe.append("   $ {}".format(command_for(r)))
        righe.append("")
    for n in piano.get("notes", []):
        righe.append("({})".format(n))
    return "\n".join(righe).rstrip()


def render_check(esito):
    righe = ["{} SESSION CHECK".format(", ".join(esito["epics"]) or esito["roadmapId"]), ""]
    for r in esito["requirements"]:
        righe.append("{} {}".format("[x]" if r["state"] == "ACTIVE" else "[ ]", r["session_id"]))
    righe.append("")
    righe.append("Readiness:")
    righe.append(
        "EPIC EXECUTION READY" if esito["ready"]
        else "NOT READY  ({}/{})".format(esito["active"], esito["total"])
    )
    return "\n".join(righe)


def additional_dev_required(piano):
    """I requisiti che nascono dal parallelismo, cioe' i DEV oltre il primo.

    E' cio' che produce il messaggio ACTION_REQUIRED: un secondo DEV serve solo quando
    due issue sono pronte insieme e il writer permanente e' gia' occupato.
    """
    return [
        r for r in piano["requirements"]
        if r["role"] == "DEV" and r.get("reason") == "TEMPORARY_WORKTREE_REQUIRED"
    ]


def writer_owner(piano, req):
    """Chi tiene il writer della lane di quel requisito, secondo QUESTO piano.

    Esiste per non far ricostruire il dato a chi stampa: `epic activate` chiamava
    `render_additional_dev` senza proprietario e stampava «occupied by another
    session», che non dice a chi chiedere.
    """
    return (piano.get("writerOwners") or {}).get(req["lane"])


def render_additional_dev(req, occupato_da=None):
    return "\n".join([
        "[RT3 ACTION_REQUIRED]",
        "",
        "Additional DEV session required",
        "",
        "Session: {}".format(req["session_id"]),
        "Issue: {}".format(req["issue"]),
        "Role: {}".format(req["role"]),
        "Capability: GIT_WRITER",
        "",
        "Permanent writer:",
        "occupied by {}".format(occupato_da or "another session"),
        "",
        "Isolation:",
        "TEMPORARY_WORKTREE_REQUIRED",
        "",
        "Manual worktree setup required.",
        "$ {}".format(command_for(req)),
    ])
