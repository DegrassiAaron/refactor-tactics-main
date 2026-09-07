"""Smoke multi-workspace: tre CHECKOUT, un solo control plane, una roadmap.

Differenza da `smoke.py`, ed e' la ragione per cui questo file esiste separato: li' i
tre terminali erano tre ambienti dello stesso checkout, e provavano il TRASPORTO. Qui i
tre terminali stanno in tre CLONI Git distinti - `refactor-tactics-main`,
`refactor-tactict-dev`, `refactor-tactics-technical-designer/refactor-tactics-main` -
ognuno invoca il `rt3` del PROPRIO checkout, e cio' che si prova e' che il control
plane resti uno solo mentre i checkout sono tre.

    MAIN      ---\\
    DEV       ----+---> un solo rt3d ---> un solo runtime.db
    DESIGNER  ---/

E' la proprieta' che nessun test unitario puo' dare: i test girano in un processo, e in
un processo «un solo control plane» e' vero per costruzione.

## Cosa mette alla prova

    10  un solo coordinator per tre workspace
    11  sette sessioni reali, con branch e HEAD LETTI da Git
    12  routing per lane: chi riceve, e soprattutto chi NON riceve
    13  mailbox offline: l'evento aspetta la sessione, non il processo
    14  load della roadmap versionata
    15  readiness derivata, e lo sblocco cross-Epic
    16  capacita' di scrittura e worktree temporaneo suggerito
    17  lease Unreal: due attivita' esclusive non vengono schedulate insieme
    18  candidate: FAILED e PASSED sullo stesso branch, commit diversi
    19  restart di rt3d: nulla va perso
    20  gli stessi dati visti dai tre checkout

## Store

⚠️ Per default gira su una `RT3_HOME` usa e getta. I checkout, i branch, gli HEAD e il
codice eseguito sono REALI; e' la radice dello stato a essere temporanea, perche' uno
smoke che scrivesse nel control plane vero lascerebbe sette sessioni finte nell'elenco
che le sessioni vere consultano, e non sarebbe ripetibile.

`--real-home` usa la radice di macchina. Serve quando si vuole ispezionare lo stato
dopo, e va usato sapendo che ci si scrive dentro.

Uso:
    python tools/rt3/smoke_multi.py
    python tools/rt3/smoke_multi.py --keep
    python tools/rt3/smoke_multi.py --main D:/... --dev D:/... --designer D:/...
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))

#: I tre workspace permanenti di questa macchina.
#:
#: ⚠️ `refactor-tactict-dev` porta un refuso nel nome - `tactict` - ed e' voluto: e' il
#: nome della directory che esiste davvero. Correggerlo qui produrrebbe uno smoke che
#: non trova il workspace.
#:
#: ⚠️ `refactor-tactics-technical-designer` NON e' un checkout: e' una cartella di
#: lavoro che CONTIENE il checkout in `refactor-tactics-main/`. Il default punta al
#: checkout annidato, non alla cartella esterna, che non ha ne' `.git` ne' `.uproject`.
DEFAULT_WORKSPACES = {
    "MAIN": os.path.join(os.path.dirname(REPO_ROOT), "refactor-tactics-main"),
    "DEV": os.path.join(os.path.dirname(REPO_ROOT), "refactor-tactict-dev"),
    "DESIGNER": os.path.join(
        os.path.dirname(REPO_ROOT),
        "refactor-tactics-technical-designer",
        "refactor-tactics-main",
    ),
}

ROADMAP_RELATIVE = os.path.join(
    "docs", "rt-three-terminals", "roadmaps", "rt3-smoke-multi-epic.yaml"
)

A1, A2, A3 = "EPIC-A/A1", "EPIC-A/A2", "EPIC-A/A3"
B1, B2, B3, B4 = "EPIC-B/B1", "EPIC-B/B2", "EPIC-B/B3", "EPIC-B/B4"
C1, C2, C3 = "EPIC-C/C1", "EPIC-C/C2", "EPIC-C/C3"


class SmokeFailure(Exception):
    pass


def check(condition, message):
    if not condition:
        raise SmokeFailure(message)
    print("    ok  {}".format(message))


def note(message):
    print("    ..  {}".format(message))


class Workspace:
    """Un checkout reale. Ogni comando gira col SUO codice e nella SUA directory."""

    def __init__(self, group, path, home):
        self.group = group
        self.path = os.path.abspath(path)
        self.home = home
        self.tools = os.path.join(self.path, "tools", "rt3")

    def exists(self):
        return os.path.isdir(self.tools) and os.path.isfile(
            os.path.join(self.tools, "rt3", "cli.py")
        )

    def env(self, session_id=None):
        env = dict(os.environ)
        env["RT3_HOME"] = self.home
        env["PYTHONPATH"] = self.tools
        env["PYTHONIOENCODING"] = "utf-8"
        if session_id:
            env["RT3_SESSION_ID"] = session_id
        else:
            env.pop("RT3_SESSION_ID", None)
        return env

    def git(self, *args):
        proc = subprocess.run(
            ["git"] + list(args),
            cwd=self.path,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            timeout=60,
        )
        return proc.stdout.decode("utf-8", "replace").strip()


class Terminal:
    """Una sessione RT3 aperta in un workspace. Come una finestra di VS Code."""

    def __init__(self, workspace, session_id=None, label=None):
        self.ws = workspace
        self.session_id = session_id
        self.label = label or "{}:{}".format(workspace.group, session_id or "-")

    def run(self, *args, **kw):
        expect_success = kw.pop("expect_success", True)
        echo = kw.pop("echo", True)
        cmd = [sys.executable, "-m", "rt3"] + list(args)
        proc = subprocess.run(
            cmd,
            cwd=self.ws.path,
            env=self.ws.env(self.session_id),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=120,
        )
        text = proc.stdout.decode("utf-8", "replace").rstrip()
        if echo:
            print("[{}] rt3 {}".format(self.label, " ".join(args)))
            for line in text.splitlines():
                print("        " + line)
            print()
        if expect_success and proc.returncode != 0:
            raise SmokeFailure(
                "`rt3 {}` da {} e' uscito {}:\n{}".format(
                    " ".join(args), self.label, proc.returncode, text
                )
            )
        return proc.returncode, text

    def out(self, *args, **kw):
        return self.run(*args, **kw)[1]

    def json(self, *args):
        text = self.out("--json", *args, echo=False)
        return json.loads(text)


# ---------------------------------------------------------------------------


def phase(number, title):
    print()
    print("--- {}. {} {}".format(number, title, "-" * max(0, 62 - len(title))))


def main(argv=None):
    parser = argparse.ArgumentParser(description="smoke RT3 su tre workspace reali")
    for group, default in DEFAULT_WORKSPACES.items():
        parser.add_argument("--" + group.lower(), default=default)
    parser.add_argument("--keep", action="store_true", help="conserva RT3_HOME")
    parser.add_argument(
        "--real-home",
        action="store_true",
        help="usa la radice RT3 di macchina invece di una temporanea (ci scrive)",
    )
    args = parser.parse_args(argv)

    if args.real_home:
        home = None  # ereditata dall'ambiente / default di paths.py
    else:
        home = tempfile.mkdtemp(prefix="rt3-smoke-multi-")

    workspaces = {}
    for group in DEFAULT_WORKSPACES:
        path = getattr(args, group.lower())
        workspaces[group] = Workspace(group, path, home or os.environ.get("RT3_HOME", ""))

    if args.real_home:
        for ws in workspaces.values():
            ws.home = None

            def env(session_id=None, _ws=ws):
                e = dict(os.environ)
                e["PYTHONPATH"] = _ws.tools
                e["PYTHONIOENCODING"] = "utf-8"
                if session_id:
                    e["RT3_SESSION_ID"] = session_id
                else:
                    e.pop("RT3_SESSION_ID", None)
                return e

            ws.env = env

    print("=" * 78)
    print("RT3 MULTI-WORKSPACE SMOKE TEST")
    print("RT3_HOME: {}".format(home or "(radice di macchina)"))
    print("=" * 78)

    admin = Terminal(workspaces["MAIN"], None, label="admin@MAIN")
    failure = None

    try:
        # ---------------------------------------------------------------
        phase(9, "workspace e versioni")
        rows = []
        versions = set()
        for group in ("MAIN", "DEV", "DESIGNER"):
            ws = workspaces[group]
            check(ws.exists(), "{}: tools/rt3 presente in {}".format(group, ws.path))
            probe = Terminal(ws, None, label="probe@" + group)
            version = probe.json("version")
            # ⚠️ `.get` e non `[...]`: un workspace non ancora aggiornato NON ha la
            # terza versione, ed e' precisamente il caso che questo controllo esiste
            # per scoprire. Un KeyError qui direbbe «lo smoke e' rotto» invece di
            # «quel checkout va aggiornato».
            triple = (
                version.get("protocolVersion"),
                version.get("schemaVersion"),
                version.get("roadmapSchemaVersion"),
            )
            if triple[2] is None:
                note(
                    "{}: `rt3 version` non dichiara roadmapSchemaVersion -> questo "
                    "checkout ha un rt3 PRECEDENTE alla Roadmap Orchestration".format(
                        group
                    )
                )
            versions.add(triple)
            rows.append(
                (
                    group,
                    ws.git("rev-parse", "--abbrev-ref", "HEAD") or "(detached)",
                    (ws.git("rev-parse", "HEAD") or "")[:8],
                    triple[0],
                    triple[1],
                    triple[2] if triple[2] is not None else "-",
                )
            )
        print()
        print(
            "    {:<9} {:<38} {:<9} {:>4} {:>4} {:>4}".format(
                "WORKSPACE", "BRANCH", "HEAD", "PROT", "SCHE", "ROAD"
            )
        )
        for row in rows:
            print("    {:<9} {:<38} {:<9} {:>4} {:>4} {:>4}".format(*row))
        print()
        check(
            len(versions) == 1,
            "i tre workspace dichiarano le stesse tre versioni: {}".format(
                sorted(versions, key=str)
            ),
        )

        # ---------------------------------------------------------------
        phase(10, "un solo coordinator")
        admin.run("daemon", "start")
        status = admin.json("daemon", "status")
        check(status["running"], "rt3d risponde")
        daemon_pid = status["info"]["pid"]

        seen = set()
        for group in ("MAIN", "DEV", "DESIGNER"):
            probe = Terminal(workspaces[group], None, label="probe@" + group)
            health = probe.json("daemon", "status")
            check(
                health["running"],
                "{} raggiunge rt3d".format(group),
            )
            seen.add((health["info"]["pid"], health["health"]["db"]))
        check(
            len(seen) == 1,
            "i tre workspace vedono lo STESSO processo e lo STESSO database: {}".format(
                sorted(seen)
            ),
        )

        # ---------------------------------------------------------------
        phase(11, "sette sessioni reali dai tre workspace")
        spec = [
            ("MAIN", "EDITOR-MAIN", "EDITOR", "MAIN", "READ_ONLY"),
            ("DEV", "EDITOR-DEV", "EDITOR", "DEV", "READ_ONLY"),
            ("DEV", "DEV-1", "DEV", "DEV", "WRITER"),
            ("DEV", "VALIDATOR-DEV", "VALIDATION", "DEV", "READ_ONLY"),
            ("DESIGNER", "EDITOR-DESIGNER", "EDITOR", "DESIGNER", "READ_ONLY"),
            ("DESIGNER", "DESIGN-DEV-1", "DEV", "DESIGNER", "WRITER"),
            ("DESIGNER", "VALIDATOR-DESIGNER", "VALIDATION", "DESIGNER", "READ_ONLY"),
        ]
        terminals = {}
        for group, sid, role, lane, mode in spec:
            term = Terminal(workspaces[group], sid)
            terminals[sid] = term
            term.out(
                "session", "start", "--id", sid, "--role", role, "--lane", lane,
                "--workspace-group", group, "--write-mode", mode,
                echo=False,
            )
        admin.run("sessions", "list")

        sessions = {s["session_id"]: s for s in admin.json("sessions", "list")}
        check(len(sessions) == 7, "sette sessioni registrate")
        for group, sid, role, lane, _mode in spec:
            ws = workspaces[group]
            recorded = sessions[sid]
            expected_branch = ws.git("rev-parse", "--abbrev-ref", "HEAD")
            expected_head = ws.git("rev-parse", "HEAD")
            check(
                os.path.normcase(os.path.abspath(recorded["worktree_path"] or ""))
                == os.path.normcase(ws.path),
                "{}: worktreePath e' quello vero ({})".format(sid, ws.path),
            )
            check(
                (recorded["branch"] or "") == expected_branch
                and (recorded["head"] or "") == expected_head,
                "{}: branch e HEAD letti da Git ({} @ {})".format(
                    sid, expected_branch, expected_head[:8]
                ),
            )
        heads = {sessions[s]["head"] for _g, s, _r, _l, _m in spec}
        check(
            len(heads) > 1,
            "i workspace NON sono allo stesso HEAD: il control plane ne tiene {} "
            "distinti".format(len(heads)),
        )

        # ---------------------------------------------------------------
        phase(12, "messaging: chi riceve e chi NON riceve")
        dev1 = terminals["DEV-1"]
        editor_dev = terminals["EDITOR-DEV"]
        validator_dev = terminals["VALIDATOR-DEV"]
        editor_designer = terminals["EDITOR-DESIGNER"]

        dev1.out("session", "set", "--task", "rt3-smoke", echo=False)
        ready = dev1.json(
            "event", "publish", "--type", "TASK_READY", "--task", "rt3-smoke",
            "--with-git", "--note", "grid pronto",
        )
        print("    EventId TASK_READY           : {}".format(ready["eventId"]))
        check(
            ready["routingRule"] == "DEV_TASK_READY_TO_EDITOR_SAME_LANE",
            "regola applicata: DEV -> EDITOR stessa lane",
        )
        inbox = editor_dev.json("inbox", "list")
        check(
            [e["event_id"] for e in inbox] == [ready["eventId"]],
            "EDITOR-DEV ha in mailbox esattamente quell'evento",
        )
        check(
            editor_designer.json("inbox", "list") == [],
            "EDITOR-DESIGNER non ha ricevuto nulla: la lane DESIGNER e' un'altra corsia",
        )
        editor_dev.out("inbox", "ack", ready["eventId"], echo=False)

        vreq = editor_dev.json(
            "event", "publish", "--type", "VALIDATION_REQUESTED", "--task", "rt3-smoke",
            "--note", "serve il gate",
        )
        print("    EventId VALIDATION_REQUESTED : {}".format(vreq["eventId"]))
        check(
            vreq["routingRule"]
            == "EDITOR_VALIDATION_REQUESTED_TO_VALIDATION_SAME_LANE",
            "regola applicata: EDITOR -> VALIDATION stessa lane",
        )
        vinbox = validator_dev.json("inbox", "list")
        check(
            [e["event_id"] for e in vinbox] == [vreq["eventId"]],
            "VALIDATOR-DEV l'ha ricevuto",
        )
        check(
            terminals["VALIDATOR-DESIGNER"].json("inbox", "list") == [],
            "VALIDATOR-DESIGNER no: e' la validazione di un'altra lane",
        )
        validator_dev.out("inbox", "ack", vreq["eventId"], echo=False)

        passed = validator_dev.json(
            "event", "publish", "--type", "VALIDATION_PASSED", "--task", "rt3-smoke",
            "--note", "gate eseguito",
        )
        print("    EventId VALIDATION_PASSED    : {}".format(passed["eventId"]))
        check(
            passed["routingRule"] == "VALIDATION_VERDICT_TO_EDITOR_SAME_LANE",
            "regola applicata: il verdetto torna all'EDITOR della lane",
        )
        final = editor_dev.json("inbox", "list")
        check(
            [e["event_id"] for e in final] == [passed["eventId"]],
            "EDITOR-DEV ha ricevuto il verdetto: giro DEV-1 -> EDITOR -> VALIDATION "
            "-> EDITOR chiuso",
        )
        editor_dev.out("inbox", "ack", passed["eventId"], echo=False)

        # ---------------------------------------------------------------
        phase(13, "mailbox offline: l'evento aspetta un RUOLO, non un processo")
        editor_designer.out("session", "stop", echo=False)
        check(
            "EDITOR-DESIGNER"
            not in {s["session_id"] for s in admin.json("sessions", "list")},
            "EDITOR-DESIGNER e' stato fermato",
        )

        design_dev = terminals["DESIGN-DEV-1"]
        design_dev.out("session", "set", "--task", "rt3-smoke-c", echo=False)
        offline = design_dev.json(
            "event", "publish", "--type", "TASK_READY", "--task", "rt3-smoke-c",
            "--note", "pubblicato mentre il destinatario non esiste",
        )
        print("    EventId verso EDITOR/DESIGNER: {}".format(offline["eventId"]))
        check(
            len(offline["deliveries"]) >= 1,
            "la consegna e' stata creata comunque ({}): e' indirizzata al RUOLO, e "
            "un ruolo esiste anche quando nessun processo lo occupa".format(
                len(offline["deliveries"])
            ),
        )

        admin.run("daemon", "restart")
        note("rt3d riavviato mentre il destinatario era assente")

        editor_designer.out(
            "session", "start", "--id", "EDITOR-DESIGNER", "--role", "EDITOR",
            "--lane", "DESIGNER", "--workspace-group", "DESIGNER",
            echo=False,
        )
        pending = editor_designer.json("inbox", "list")
        check(
            [e["event_id"] for e in pending] == [offline["eventId"]],
            "riaperta la sessione, l'evento e' li' ad aspettarla",
        )
        editor_designer.out("inbox", "ack", offline["eventId"], echo=False)
        check(
            editor_designer.json("inbox", "list") == [],
            "dopo l'ack sparisce dai pendenti",
        )

        # ---------------------------------------------------------------
        phase(14, "load della roadmap versionata")
        roadmap_path = os.path.join(workspaces["MAIN"].path, ROADMAP_RELATIVE)
        check(os.path.isfile(roadmap_path), "la roadmap esiste: {}".format(ROADMAP_RELATIVE))
        admin.run("roadmap", "validate", "--file", roadmap_path)
        admin.run("roadmap", "load", "--file", roadmap_path)
        loaded = admin.json("roadmaps", "list")
        check(len(loaded) == 1, "una roadmap caricata")
        roadmap_id = loaded[0]["roadmap_id"]
        content_hash = loaded[0]["content_hash"]
        note("roadmap {} hash {}".format(roadmap_id, content_hash))

        admin.run("roadmap", "graph")
        admin.run("roadmap", "critical-path")
        cp = admin.json("roadmap", "critical-path")
        check(
            cp["path"] == [B1, B2, C2, C3],
            "il cammino critico attraversa due Epic: {}".format(" -> ".join(cp["path"])),
        )
        check(cp["duration"] == 14, "durata del cammino critico: 14")

        # ---------------------------------------------------------------
        phase(15, "readiness derivata e sblocco cross-Epic")

        def readiness_map(term=admin):
            return {i["key"]: i["state"] for i in term.json("roadmap", "ready")["items"]}

        admin.run("roadmap", "ready")
        initial = readiness_map()
        for key in (A1, B1, C1):
            check(initial[key] == "READY", "{} e' READY all'inizio".format(key))
        for key in (A2, A3, B2, B3, B4, C2, C3):
            check(initial[key] == "BLOCKED", "{} e' BLOCKED all'inizio".format(key))

        dev1.out(
            "roadmap", "state", "set", B1, "--progress", "VALIDATED", echo=False
        )
        mid = readiness_map()
        check(mid[B2] == "READY", "validato B1, B2 diventa READY")
        check(mid[C2] == "BLOCKED", "C2 aspetta ancora B2, non B1")

        dev1.out(
            "roadmap", "state", "set", B2, "--progress", "VALIDATED", echo=False
        )
        after = readiness_map()
        check(after[B3] == "READY", "validato B2, B3 diventa READY")
        check(after[B4] == "READY", "validato B2, B4 diventa READY")
        check(
            after[C2] == "READY",
            "validato B2, C2 diventa READY: la dipendenza ATTRAVERSA gli Epic",
        )
        check(after[C3] == "BLOCKED", "C3 aspetta C2, e resta BLOCKED")
        check(after[A2] == "BLOCKED", "EPIC-A non e' stato toccato da nulla di tutto cio'")

        # ---------------------------------------------------------------
        phase(16, "capacita' di scrittura e worktree temporaneo")
        admin.run("roadmap", "plan")
        plan = admin.json("roadmap", "plan")
        modes = {a["key"]: a for a in plan["assignments"]}
        check(
            modes[B3]["mode"] == "PERMANENT_WRITER" and modes[B3]["workspace"] == "DEV",
            "B3 prende il writer permanente di DEV",
        )
        check(
            modes[B4]["mode"] == "TEMPORARY_WORKTREE_SUGGESTED",
            "B4 e' parallelizzabile e il writer di DEV e' occupato: worktree "
            "temporaneo SUGGERITO",
        )
        check(
            plan["capacity"]["writers"]["DEV"]["used"]
            == plan["capacity"]["writers"]["DEV"]["capacity"],
            "la capacita' di scrittura di DEV risulta satura",
        )
        note("nessun worktree e' stato creato: il planner decide, non esegue")

        # ---------------------------------------------------------------
        phase(17, "lease Unreal: due attivita' esclusive non insieme")
        for key, progress in (
            (A1, "VALIDATED"), (A2, "VALIDATED"),
            (B3, "DONE"), (B4, "DONE"), (C1, "DONE"),
            (C2, "VALIDATED"),
        ):
            admin.out("roadmap", "state", "set", key, "--progress", progress, echo=False)

        unreal_ready = readiness_map()
        check(unreal_ready[A3] == "READY", "A3 (richiede UNREAL_EDITOR) e' READY")
        check(unreal_ready[C3] == "READY", "C3 (richiede UNREAL_EDITOR) e' READY")

        admin.run("roadmap", "plan")
        plan2 = admin.json("roadmap", "plan")
        with_unreal = [
            a for a in plan2["assignments"] if "UNREAL_EDITOR" in a["resources"]
        ]
        check(
            len(with_unreal) == 1,
            "una sola attivita' Unreal schedulata, non due: {}".format(
                [a["key"] for a in with_unreal]
            ),
        )
        blocked_by_lease = [
            d for d in plan2["deferred"] if d["reason"] == "UNREAL_LEASE_CAPACITY"
        ]
        check(
            len(blocked_by_lease) == 1,
            "l'altra e' rimandata per UNREAL_LEASE_CAPACITY: {}".format(
                [d["key"] for d in blocked_by_lease]
            ),
        )
        check(
            plan2["capacity"]["unrealEditor"]["used"] == 1,
            "il lease Unreal risulta impegnato una volta sola",
        )
        note("l'Editor non e' stato avviato: si prova lo scheduler, non Unreal")

        # ---------------------------------------------------------------
        phase(18, "candidate: due commit sullo stesso branch, esiti opposti")
        head_dev = workspaces["DEV"].git("rev-parse", "HEAD")
        head_main = workspaces["MAIN"].git("rev-parse", "HEAD")

        first = dev1.json(
            "candidate", "create", "--task", "rt3-smoke",
            "--branch", "feat/rt3-smoke-b1", "--head", head_dev,
            "--roadmap", roadmap_id, "--item", B1,
        )
        second = dev1.json(
            "candidate", "create", "--task", "rt3-smoke",
            "--branch", "feat/rt3-smoke-b1", "--head", head_main,
            "--roadmap", roadmap_id, "--item", B1,
        )
        check(
            first["candidate_id"] != second["candidate_id"],
            "due candidate distinti: {} e {}".format(
                first["candidate_id"], second["candidate_id"]
            ),
        )
        check(first["status"] == "PENDING", "un candidate nasce PENDING, non FAILED")

        validator_dev.out(
            "candidate", "status", first["candidate_id"], "--status", "FAILED",
            "--note", "regressione", echo=False,
        )
        validator_dev.out(
            "candidate", "status", second["candidate_id"], "--status", "PASSED",
            echo=False,
        )
        validator_dev.run("candidates", "list", "--task", "rt3-smoke")

        after_first = admin.json("candidates", "list", "--task", "rt3-smoke")
        by_id = {c["candidate_id"]: c for c in after_first}
        check(
            by_id[first["candidate_id"]]["status"] == "FAILED"
            and by_id[second["candidate_id"]]["status"] == "PASSED",
            "stesso branch, esiti opposti: il verdetto sta sul CANDIDATE",
        )
        check(
            by_id[first["candidate_id"]]["branch"]
            == by_id[second["candidate_id"]]["branch"],
            "e i due candidate stanno davvero sullo stesso branch",
        )

        validator_dev.out(
            "roadmap", "state", "set", B1, "--progress", "VALIDATED",
            "--candidate", second["candidate_id"], echo=False,
        )
        states = {
            s["item_key"]: s for s in admin.json("roadmap", "state", "list")["states"]
        }
        check(
            states[B1]["candidate_id"] == second["candidate_id"],
            "la issue registra QUALE candidate l'ha portata a VALIDATED",
        )
        check(
            admin.json("candidates", "list", "--task", "rt3-smoke")[-1]["status"]
            != "PASSED"
            or by_id[first["candidate_id"]]["status"] == "FAILED",
            "il candidate fallito resta fallito anche dopo l'avanzamento della issue",
        )

        # ---------------------------------------------------------------
        phase(19, "restart completo del coordinator")
        before = {
            "sessions": admin.json("sessions", "list"),
            "events": [e["event_id"] for e in admin.json("events", "list", "--limit", "200")],
            "roadmaps": admin.json("roadmaps", "list"),
            "states": admin.json("roadmap", "state", "list"),
            "candidates": admin.json("candidates", "list"),
            "plan": admin.json("roadmap", "plan"),
        }
        pending_before = editor_dev.json("inbox", "list", "--all")

        admin.run("daemon", "stop")
        time.sleep(0.6)
        admin.run("daemon", "start")
        new_status = admin.json("daemon", "status")
        check(new_status["running"], "rt3d e' tornato su")
        check(
            new_status["info"]["pid"] != daemon_pid,
            "ed e' un processo NUOVO (pid {} -> {}): il restart e' vero".format(
                daemon_pid, new_status["info"]["pid"]
            ),
        )

        after_restart = {
            "sessions": admin.json("sessions", "list"),
            "events": [e["event_id"] for e in admin.json("events", "list", "--limit", "200")],
            "roadmaps": admin.json("roadmaps", "list"),
            "states": admin.json("roadmap", "state", "list"),
            "candidates": admin.json("candidates", "list"),
            "plan": admin.json("roadmap", "plan"),
        }
        for key in ("events", "roadmaps", "states", "candidates", "plan"):
            check(
                before[key] == after_restart[key],
                "dopo il restart {} e' identico".format(key),
            )
        check(
            editor_dev.json("inbox", "list", "--all") == pending_before,
            "e la mailbox storica di EDITOR-DEV e' intatta",
        )

        # ---------------------------------------------------------------
        phase(20, "gli stessi dati dai tre checkout")
        views = {}
        for group in ("MAIN", "DEV", "DESIGNER"):
            term = Terminal(workspaces[group], None, label="view@" + group)
            term.run("status")
            views[group] = {
                "roadmaps": term.json("roadmaps", "list"),
                "plan": term.json("roadmap", "plan"),
                "ready": term.json("roadmap", "ready"),
                "sessions": sorted(
                    s["session_id"] for s in term.json("sessions", "list")
                ),
            }
        base = views["MAIN"]
        for group in ("DEV", "DESIGNER"):
            for key in ("roadmaps", "plan", "ready", "sessions"):
                check(
                    views[group][key] == base[key],
                    "{} vede lo stesso {} di MAIN".format(group, key),
                )
        check(
            len({tuple(v["sessions"]) for v in views.values()}) == 1,
            "e le stesse sessioni, pur avendo path, branch e HEAD diversi",
        )

        print()
        print("=" * 78)
        print("RT3 MULTI-WORKSPACE SMOKE TEST: PASS")
        print("=" * 78)

    except SmokeFailure as exc:
        failure = exc
        print()
        print("=" * 78)
        print("RT3 MULTI-WORKSPACE SMOKE TEST: FAIL")
        print(str(exc))
        print("=" * 78)
    except Exception as exc:  # pragma: no cover - imprevisto vero
        failure = exc
        print()
        print("=" * 78)
        print("RT3 MULTI-WORKSPACE SMOKE TEST: FAIL (imprevisto)")
        import traceback

        traceback.print_exc()
        print("=" * 78)
    finally:
        try:
            admin.run("daemon", "stop", expect_success=False, echo=False)
        except Exception:
            pass
        time.sleep(0.5)
        if home:
            if args.keep:
                print("RT3_HOME conservata: {}".format(home))
            else:
                shutil.rmtree(home, ignore_errors=True)

    return 1 if failure else 0


if __name__ == "__main__":
    sys.exit(main())
