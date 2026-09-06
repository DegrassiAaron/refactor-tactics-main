"""Smoke test RT3: tre terminali, un handoff completo, zero copia-incolla.

Non e' un test unitario travestito. Avvia un `rt3d` VERO come processo staccato e invoca
la CLI VERA tre volte, ogni volta con un `RT3_SESSION_ID` diverso - che e' esattamente
cio' che accade con tre finestre di terminale aperte. Il punto che deve dimostrare non
e' che il codice gira, ma che nessun dato viene trasportato a mano fra i tre:

    DEV-1        pubblica TASK_READY
       |
       v
    EDITOR-DEV   lo trova gia' in mailbox, fa ack, chiede VALIDATION_REQUESTED
       |
       v
    VALIDATOR-DEV lo trova, fa ack, risponde VALIDATION_PASSED
       |
       v
    EDITOR-DEV   riceve il verdetto

⚠️ Gira su un `RT3_HOME` usa e getta. Toccare il control plane reale della macchina
renderebbe lo smoke non ripetibile, e - peggio - lascerebbe tre sessioni finte
nell'elenco che le sessioni vere consultano.

Uso:
    python tools/rt3/smoke.py            # smoke isolato (default)
    python tools/rt3/smoke.py --keep     # non cancella RT3_HOME, per ispezionarlo
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


class SmokeFailure(Exception):
    pass


class Terminal:
    """Un terminale RT3 simulato: stesso eseguibile, ambiente proprio."""

    def __init__(self, session_id, home):
        self.session_id = session_id
        self.env = dict(os.environ)
        self.env["RT3_HOME"] = home
        self.env["PYTHONPATH"] = HERE
        self.env["PYTHONIOENCODING"] = "utf-8"
        if session_id:
            self.env["RT3_SESSION_ID"] = session_id
        else:
            self.env.pop("RT3_SESSION_ID", None)

    def run(self, *args, expect_success=True, echo=True):
        cmd = [sys.executable, "-m", "rt3"] + list(args)
        proc = subprocess.run(
            cmd,
            cwd=HERE,
            env=self.env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=90,
        )
        text = proc.stdout.decode("utf-8", "replace").rstrip()
        if echo:
            label = self.session_id or "-"
            print("[{}] rt3 {}".format(label, " ".join(args)))
            for line in text.splitlines():
                print("        " + line)
            print()
        if expect_success and proc.returncode != 0:
            raise SmokeFailure(
                "`rt3 {}` da {} e' uscito {}:\n{}".format(
                    " ".join(args), self.session_id, proc.returncode, text
                )
            )
        return text

    def json(self, *args):
        text = self.run("--json", *args, echo=False)
        return json.loads(text)


def check(condition, message):
    if not condition:
        raise SmokeFailure(message)
    print("    ok  {}".format(message))


def main(argv=None):
    parser = argparse.ArgumentParser(description="smoke test del control plane RT3")
    parser.add_argument(
        "--keep", action="store_true", help="non cancella la RT3_HOME temporanea"
    )
    args = parser.parse_args(argv)

    home = tempfile.mkdtemp(prefix="rt3-smoke-")
    admin = Terminal(None, home)
    print("=" * 78)
    print("RT3 SMOKE TEST")
    print("RT3_HOME temporanea: {}".format(home))
    print("=" * 78)
    print()

    failure = None
    try:
        # ---------------------------------------------------------------
        print("--- 0. coordinator ---------------------------------------------------")
        admin.run("daemon", "start")
        health = admin.json("daemon", "status")
        check(health["running"], "rt3d risponde")

        # ---------------------------------------------------------------
        print("--- 1. tre sessioni indipendenti -------------------------------------")
        dev = Terminal("DEV-1", home)
        editor = Terminal("EDITOR-DEV", home)
        validator = Terminal("VALIDATOR-DEV", home)

        dev.run(
            "session", "start", "--id", "DEV-1", "--role", "DEV",
            "--lane", "DEV", "--workspace-group", "DEV", "--task", "2272",
        )
        editor.run(
            "session", "start", "--id", "EDITOR-DEV", "--role", "EDITOR",
            "--lane", "DEV", "--workspace-group", "MAIN",
        )
        validator.run(
            "session", "start", "--id", "VALIDATOR-DEV", "--role", "VALIDATION",
            "--lane", "DEV", "--workspace-group", "DEV",
        )
        # Una sessione di un'altra lane: serve a provare che NON riceve niente.
        other = Terminal("EDITOR-DESIGNER", home)
        other.run(
            "session", "start", "--id", "EDITOR-DESIGNER", "--role", "EDITOR",
            "--lane", "DESIGNER", "--workspace-group", "DESIGNER",
        )

        sessions = admin.json("sessions", "list")
        check(len(sessions) == 4, "quattro sessioni registrate e visibili a tutte")

        admin.run("sessions", "list")

        # ---------------------------------------------------------------
        print("--- 2. DEV-1 pubblica TASK_READY -------------------------------------")
        dev.run("event", "publish", "--type", "TASK_STARTED", "--note", "inizio lavoro")
        published = dev.json(
            "event", "publish", "--type", "TASK_READY", "--with-git",
            "--note", "risolutore pronto, branch e sha nel payload",
        )
        check(
            published["routingRule"] == "DEV_TASK_READY_TO_EDITOR_SAME_LANE",
            "il routing ha scelto la regola DEV -> EDITOR stessa lane",
        )

        # ---------------------------------------------------------------
        print("--- 3. EDITOR-DEV lo trova gia' in mailbox ---------------------------")
        inbox = editor.json("inbox", "list")
        check(len(inbox) == 1, "EDITOR-DEV ha 1 evento pending, senza che nessuno glielo abbia detto")
        check(
            inbox[0]["event_id"] == published["eventId"],
            "e' proprio l'evento pubblicato da DEV-1",
        )

        elsewhere = other.json("inbox", "list")
        check(elsewhere == [], "EDITOR-DESIGNER non ha ricevuto nulla (lane diversa)")

        editor.run("inbox", "show", published["eventId"])
        editor.run("inbox", "ack", published["eventId"], "--note", "preso in carico")
        check(
            editor.json("inbox", "list") == [],
            "dopo l'ack l'evento non risulta piu' pending",
        )

        # ---------------------------------------------------------------
        print("--- 4. EDITOR-DEV chiede validazione ---------------------------------")
        validation_req = editor.json(
            "event", "publish", "--type", "VALIDATION_REQUESTED",
            "--note", "asset integrati, serve il gate",
        )
        check(
            validation_req["routingRule"]
            == "EDITOR_VALIDATION_REQUESTED_TO_VALIDATION_SAME_LANE",
            "il routing ha scelto EDITOR -> VALIDATION stessa lane",
        )

        # ---------------------------------------------------------------
        print("--- 5. VALIDATOR-DEV lo riceve e risponde ----------------------------")
        vinbox = validator.json("inbox", "list")
        check(len(vinbox) == 1, "VALIDATOR-DEV ha 1 evento pending")
        check(
            vinbox[0]["event_id"] == validation_req["eventId"],
            "e' la richiesta di validazione di EDITOR-DEV",
        )
        validator.run("inbox", "ack", validation_req["eventId"])

        verdict = validator.json(
            "event", "publish", "--type", "VALIDATION_PASSED",
            "--payload", json.dumps(
                {"automation": "PASS", "determinism": "PASS", "pie": "NOT RUN"}
            ),
            "--note", "gate eseguito sul commit dichiarato",
        )
        check(
            verdict["routingRule"] == "VALIDATION_VERDICT_TO_EDITOR_SAME_LANE",
            "il verdetto torna all'EDITOR della stessa lane",
        )

        # ---------------------------------------------------------------
        print("--- 6. il verdetto chiude il giro su EDITOR-DEV ----------------------")
        final = editor.json("inbox", "list")
        check(len(final) == 1, "EDITOR-DEV ha ricevuto il verdetto")
        check(final[0]["type"] == "VALIDATION_PASSED", "ed e' VALIDATION_PASSED")
        editor.run("inbox", "show", verdict["eventId"])
        editor.run("inbox", "ack", verdict["eventId"])

        # ---------------------------------------------------------------
        print("--- 7. il task racconta il giro intero -------------------------------")
        dev.run("task", "show", "2272")
        task = dev.json("task", "show", "2272")
        types = [e["type"] for e in task["events"]]
        check(
            types
            == [
                "TASK_STARTED",
                "TASK_READY",
                "VALIDATION_REQUESTED",
                "VALIDATION_PASSED",
            ],
            "l'event log del task 2272 porta i quattro eventi in ordine",
        )

        # ---------------------------------------------------------------
        print("--- 8. restart del coordinator ---------------------------------------")
        log_before = admin.json("events", "list", "--limit", "100")
        admin.run("daemon", "restart")
        log_after = admin.json("events", "list", "--limit", "100")
        check(
            [e["event_id"] for e in log_before] == [e["event_id"] for e in log_after],
            "dopo il restart di rt3d l'event log e' identico: niente e' andato perso",
        )
        check(
            editor.json("inbox", "list", "--all") != [],
            "e la mailbox storica di EDITOR-DEV e' ancora consultabile",
        )

        print()
        print("=" * 78)
        print("SMOKE TEST: PASS")
        print("=" * 78)

    except SmokeFailure as exc:
        failure = exc
        print()
        print("=" * 78)
        print("SMOKE TEST: FAIL")
        print(str(exc))
        print("=" * 78)
    finally:
        try:
            admin.run("daemon", "stop", expect_success=False, echo=False)
        except Exception:
            pass
        time.sleep(0.5)
        if args.keep:
            print("RT3_HOME conservata: {}".format(home))
        else:
            shutil.rmtree(home, ignore_errors=True)

    return 1 if failure else 0


if __name__ == "__main__":
    sys.exit(main())
