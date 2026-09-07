"""Lo stato RT3 di una sessione: uno SNAPSHOT autoritativo, e come si mostra.

🔴 Il banner non si costruisce nella conversazione. Chi legge lo status deve poter
credere a ogni riga, e una riga ricordata a memoria da un agente non e' verificabile: se
il branch e' cambiato dieci minuti fa, la memoria dice ancora quello vecchio. Qui ogni
campo viene da una fonte dichiarata.

    Git            worktreePath, repoRoot, branch, head
    Control plane  sessione, ruolo, lane, task, candidate, inbox, lease
    Roadmap        epic, issue, revisione
    Planner        stato derivato, blocco, azione richiesta

Modulo PURO, come `routing.py` e `planner.py`: riceve i dati gia' raccolti e ritorna
strutture. Non apre il database, non esegue Git, non parla in rete. E' la ragione per cui
un renderer si prova senza avviare niente.

## Le due domande separate

    build_snapshot()   che cosa e' vero adesso
    render()           come lo si dice a questo lettore

Tenerle separate serve: `--json` e il testo devono descrivere lo STESSO stato, e un
renderer che calcolasse qualcosa per conto proprio farebbe divergere le due viste.

## StateRevision

⚠️ `generatedAt` cambia a ogni chiamata e NON e' un cambiamento di stato. Due snapshot
identici tranne il timestamp devono risultare semanticamente uguali, altrimenti lo status
automatico diventa spam - e uno status che si ripete uguale smette di essere letto.
`StateRevision` e' l'hash dei soli campi semantici.
"""

import collections
import hashlib
import json

from .model import (
    BLOCKING_REASONS,
    MESSAGE_CLASSES,
    REQUIRED_ACTIONS,
    STATUS_LEVELS,
)

#: Campi che NON entrano nella revisione: cambiano da soli e non dicono nulla di nuovo.
_VOLATILE = ("generatedAt", "stateRevision", "lastSeenAt")


def state_revision(snapshot):
    """Hash dei campi SEMANTICI. Due stati uguali danno la stessa revisione.

    Ordinato e stabile: `sort_keys` perche' due dizionari con lo stesso contenuto e
    ordine diverso descrivono lo stesso stato, e produrre due revisioni diverse
    segnalerebbe un cambiamento che non c'e'.
    """
    semantico = {k: v for k, v in snapshot.items() if k not in _VOLATILE}
    testo = json.dumps(semantico, sort_keys=True, ensure_ascii=False, default=str)
    return "rev_" + hashlib.sha256(testo.encode("utf-8")).hexdigest()[:12]


def build_snapshot(
    session=None,
    git=None,
    leases=None,
    inbox_pending=0,
    roadmap=None,
    planner=None,
    versions=None,
    generated_at=None,
    blocking=None,
    required_action=None,
):
    """Compone lo snapshot dalle fonti. Nessun campo inventato: cio' che manca resta None.

    ⚠️ `None` e' un valore legittimo e diverso da una stringa vuota: «non c'e' una
    sessione» non e' «la sessione si chiama nulla». Il renderer li distingue.
    """
    session = session or {}
    git = git or {}
    leases = leases or {}
    versions = versions or {}
    planner = planner or {}

    writer = leases.get("writer") or {}
    unreal = leases.get("unreal") or {}

    snap = collections.OrderedDict()
    snap["generatedAt"] = generated_at
    snap["sessionId"] = session.get("session_id")
    snap["role"] = session.get("role")
    snap["lane"] = session.get("lane")
    snap["workspaceGroup"] = session.get("workspace_group")
    snap["sessionState"] = session.get("status")

    snap["epicId"] = planner.get("epicId")
    snap["issueId"] = planner.get("issueId") or session.get("task_id")
    snap["candidateId"] = session.get("candidate_id")
    snap["taskState"] = planner.get("taskState")

    # ⚠️ Git e' la fonte di questi quattro, non il control plane: la sessione dichiara
    # cio' che ha visto quando e' partita, e nel frattempo il branch puo' essere cambiato.
    snap["worktreePath"] = git.get("worktreePath") or session.get("worktree_path")
    snap["repoRoot"] = git.get("repoRoot") or session.get("repo_root")
    snap["branch"] = git.get("branch") if git else session.get("branch")
    snap["head"] = git.get("head") if git else session.get("head")

    snap["writeMode"] = session.get("write_mode")
    snap["writerLeaseState"] = "OWNED" if writer else "NONE"
    snap["writerLeaseOwner"] = writer.get("owner")
    snap["unrealLeaseState"] = "OWNED" if unreal else "NONE"
    snap["unrealLeaseOwner"] = unreal.get("owner")

    snap["inboxPending"] = int(inbox_pending or 0)

    snap["blockingReason"] = blocking
    snap["requiredAction"] = required_action or "NONE"

    snap["roadmapId"] = (roadmap or {}).get("roadmap_id")
    snap["roadmapRevision"] = (roadmap or {}).get("content_hash")
    snap["protocolVersion"] = versions.get("protocolVersion")
    snap["schemaVersion"] = versions.get("schemaVersion")
    snap["roadmapSchemaVersion"] = versions.get("roadmapSchemaVersion")

    if snap["blockingReason"] is not None and snap["blockingReason"] not in BLOCKING_REASONS:
        raise ValueError(
            "blockingReason non ammesso: {!r}. Ammessi: {}.".format(
                snap["blockingReason"], ", ".join(BLOCKING_REASONS)
            )
        )
    if snap["requiredAction"] not in REQUIRED_ACTIONS:
        raise ValueError(
            "requiredAction non ammesso: {!r}. Ammessi: {}.".format(
                snap["requiredAction"], ", ".join(REQUIRED_ACTIONS)
            )
        )

    snap["stateRevision"] = state_revision(snap)
    return snap


# ---------------------------------------------------------------------------
# Diff
# ---------------------------------------------------------------------------

#: I campi il cui cambiamento merita di essere annunciato. Gli altri cambiano nel
#: silenzio dello snapshot: annunciare tutto e' lo stesso che non annunciare niente.
SIGNIFICANT = (
    "sessionState",
    "taskState",
    "issueId",
    "candidateId",
    "writeMode",
    "writerLeaseState",
    "writerLeaseOwner",
    "unrealLeaseState",
    "unrealLeaseOwner",
    "inboxPending",
    "blockingReason",
    "requiredAction",
    "branch",
    "head",
    "roadmapRevision",
)

#: Etichette leggibili per il diff.
_ETICHETTE = {
    "sessionState": "Session",
    "taskState": "Task",
    "issueId": "Issue",
    "candidateId": "Candidate",
    "writeMode": "WriteMode",
    "writerLeaseState": "WriterLease",
    "writerLeaseOwner": "WriterLease owner",
    "unrealLeaseState": "UnrealLease",
    "unrealLeaseOwner": "UnrealLease owner",
    "inboxPending": "Inbox",
    "blockingReason": "Blocked by",
    "requiredAction": "Action",
    "branch": "Branch",
    "head": "HEAD",
    "roadmapRevision": "Roadmap",
}


def diff(prima, dopo):
    """Cosa e' cambiato fra due snapshot, limitatamente a cio' che conta.

    🔴 Ritorna una lista VUOTA quando cambia solo il timestamp. E' la regola che tiene
    lo status automatico leggibile: un messaggio che si ripete identico a ogni comando
    smette di essere letto, e con lui smette di essere letto quello importante.
    """
    if not prima:
        return []
    cambi = []
    for campo in SIGNIFICANT:
        a, b = prima.get(campo), dopo.get(campo)
        if a != b:
            cambi.append({"field": campo, "label": _ETICHETTE.get(campo, campo),
                          "from": a, "to": b})
    return cambi


def has_significant_diff(prima, dopo):
    return bool(diff(prima, dopo))


# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------


def _dash(v):
    return "-" if v is None or v == "" else str(v)


def message_class(snapshot):
    """La classe del messaggio deriva dallo STATO, non da chi lo stampa."""
    if snapshot.get("blockingReason"):
        return "BLOCKED"
    if snapshot.get("requiredAction") not in (None, "NONE"):
        return "ACTION_REQUIRED"
    return "STATUS"


def render(snapshot, level="normal", role=None):
    """Testo per un lettore. `role` decide cosa mettere in cima, non cosa e' vero."""
    if level not in STATUS_LEVELS:
        raise ValueError(
            "livello non ammesso: {!r}. Ammessi: {}.".format(level, ", ".join(STATUS_LEVELS))
        )
    role = role or snapshot.get("role")
    if level == "compact":
        return _compact(snapshot)
    righe = _normal(snapshot, role)
    if level == "verbose":
        righe += _verbose_extra(snapshot)
    return "\n".join(righe)


def _compact(s):
    """Una riga sola. Deve stare in coda a un comando senza rubare la scena."""
    pezzi = [
        _dash(s.get("sessionId")),
        _dash(s.get("issueId")),
        _dash(s.get("taskState") or s.get("sessionState")),
    ]
    if s.get("writeMode") == "WRITER":
        pezzi.append("WRITER" + ("+" if s.get("writerLeaseState") == "OWNED" else "!"))
    else:
        pezzi.append("READ_ONLY")
    if s.get("unrealLeaseState") == "OWNED":
        pezzi.append("UNREAL+")
    pezzi.append("Inbox {}".format(s.get("inboxPending", 0)))
    riga = "[RT3] " + " | ".join(pezzi)
    if s.get("blockingReason"):
        riga += "  >> BLOCKED: {}".format(s["blockingReason"])
    elif s.get("requiredAction") not in (None, "NONE"):
        riga += "  >> {}".format(s["requiredAction"])
    return riga


def _normal(s, role):
    cls = message_class(s)
    righe = ["[RT3 SESSION]" if cls == "STATUS" else "[RT3 {}]".format(cls), ""]
    righe.append("Session: {}".format(_dash(s.get("sessionId"))))
    righe.append("Role: {}".format(_dash(s.get("role"))))
    righe.append("Lane: {}".format(_dash(s.get("lane"))))
    righe.append("")

    # ⛔ L'ordine cambia col ruolo, i FATTI no. Un EDITOR guarda prima l'Epic, un
    # VALIDATION prima il candidate: mettere in cima cio' che quel ruolo deve decidere
    # e' la differenza fra uno status che si legge e uno che si scorre.
    if role == "VALIDATION":
        righe.append("Candidate: {}".format(_dash(s.get("candidateId"))))
        righe.append("Issue: {}".format(_dash(s.get("issueId"))))
        righe.append("HEAD: {}".format(_dash(s.get("head"))))
        righe.append("Roadmap: {}".format(_dash(s.get("roadmapRevision"))))
    elif role == "EDITOR":
        righe.append("Epic: {}".format(_dash(s.get("epicId"))))
        righe.append("Issue: {}".format(_dash(s.get("issueId"))))
    else:
        righe.append("Epic: {}".format(_dash(s.get("epicId"))))
        righe.append("Task: {}".format(_dash(s.get("issueId"))))
    righe.append("")
    righe.append("Mode: {}".format(_dash(s.get("writeMode"))))
    righe.append("WriterLease: {}{}".format(
        _dash(s.get("writerLeaseState")),
        " ({})".format(s["writerLeaseOwner"]) if s.get("writerLeaseOwner") else "",
    ))
    righe.append("UnrealLease: {}{}".format(
        _dash(s.get("unrealLeaseState")),
        " ({})".format(s["unrealLeaseOwner"]) if s.get("unrealLeaseOwner") else "",
    ))
    righe.append("")
    righe.append("State: {}".format(_dash(s.get("taskState") or s.get("sessionState"))))
    righe.append("Inbox: {}".format(s.get("inboxPending", 0)))
    if s.get("blockingReason"):
        righe += ["", "Blocked by: {}".format(s["blockingReason"])]
    if s.get("requiredAction") not in (None, "NONE"):
        righe += ["Required action: {}".format(s["requiredAction"])]
    return righe


def _verbose_extra(s):
    return [
        "",
        "-- verbose --",
        "Worktree: {}".format(_dash(s.get("worktreePath"))),
        "RepoRoot: {}".format(_dash(s.get("repoRoot"))),
        "Branch: {}".format(_dash(s.get("branch"))),
        "HEAD: {}".format(_dash(s.get("head"))),
        "WorkspaceGroup: {}".format(_dash(s.get("workspaceGroup"))),
        "SessionState: {}".format(_dash(s.get("sessionState"))),
        "RoadmapId: {}".format(_dash(s.get("roadmapId"))),
        "RoadmapRevision: {}".format(_dash(s.get("roadmapRevision"))),
        "StateRevision: {}".format(_dash(s.get("stateRevision"))),
        "GeneratedAt: {}".format(_dash(s.get("generatedAt"))),
        "Protocol/Schema/Roadmap: v{} / v{} / v{}".format(
            _dash(s.get("protocolVersion")),
            _dash(s.get("schemaVersion")),
            _dash(s.get("roadmapSchemaVersion")),
        ),
    ]


def render_diff(cambi, snapshot=None):
    """Il NOTICE di cambiamento, seguito dal compact. Vuoto se non e' cambiato niente."""
    if not cambi:
        return ""
    righe = ["[RT3 NOTICE]", ""]
    for c in cambi:
        righe.append("{}:".format(c["label"]))
        righe.append("{} -> {}".format(_dash(c["from"]), _dash(c["to"])))
        righe.append("")
    if snapshot:
        righe.append(_compact(snapshot))
    return "\n".join(righe).rstrip()
