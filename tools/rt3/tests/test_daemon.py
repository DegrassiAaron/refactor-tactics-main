"""Ciclo completo: client HTTP -> rt3d -> SQLite.

Qui si provano le proprieta' che i test sullo store non possono provare, perche' non
riguardano le regole ma il TRASPORTO e la VITA del coordinator: che il daemon pubblichi
un endpoint raggiungibile, che il client rifiuti un protocollo diverso, e soprattutto
che spegnere e riaccendere rt3d non perda niente.
"""

import unittest

from rt3.errors import DaemonUnavailable, ProtocolMismatch, SessionExists
from tests.harness import LocalDaemon, Rt3TestCase, client, start_session


class DaemonLifecycleTest(Rt3TestCase):
    def test_client_senza_daemon_dice_cosa_fare(self):
        with self.assertRaises(DaemonUnavailable) as ctx:
            client()
        self.assertIn("rt3 daemon start", str(ctx.exception))

    def test_health_dichiara_le_versioni(self):
        """L'health porta tutte e tre le versioni, e quella del DB e' quella VERA.

        ⚠️ Il confronto e' con le costanti del pacchetto e non con dei letterali: un
        letterale qui invecchia al primo bump e costringe a modificare il test per
        farlo passare - cioe' toglie al test la capacita' di dire qualcosa.

        `dbSchemaVersion` e' l'asserto che porta piu' informazione: non e' una costante
        ma il numero LETTO dal database, quindi provarlo uguale a SCHEMA_VERSION prova
        che `migrate()` sia arrivata fino in fondo. Se una migrazione mancasse, questo
        e' il test che lo direbbe.
        """
        from rt3 import PROTOCOL_VERSION, ROADMAP_SCHEMA_VERSION, SCHEMA_VERSION

        with LocalDaemon():
            health = client().health()
            self.assertTrue(health["ok"])
            self.assertEqual(health["protocolVersion"], PROTOCOL_VERSION)
            self.assertEqual(health["schemaVersion"], SCHEMA_VERSION)
            self.assertEqual(health["roadmapSchemaVersion"], ROADMAP_SCHEMA_VERSION)
            self.assertEqual(health["dbSchemaVersion"], SCHEMA_VERSION)

    def test_client_di_protocollo_diverso_e_respinto_subito(self):
        """Fail-fast, non degradazione.

        Simula il caso reale: tre workspace, uno aggiornato e due no. Il rifiuto deve
        arrivare al primo comando, non tre eventi dopo su un campo mancante.
        """
        import rt3.client as client_module

        with LocalDaemon():
            original = client_module.PROTOCOL_VERSION
            try:
                client_module.PROTOCOL_VERSION = 99
                with self.assertRaises(ProtocolMismatch) as ctx:
                    client_module.Client(
                        self.daemon_host(), self.daemon_port(), timeout=5
                    ).call("stats")
            finally:
                client_module.PROTOCOL_VERSION = original
        self.assertIn("v99", str(ctx.exception))

    def test_daemon_file_dichiara_protocollo_incompatibile(self):
        """Anche senza chiamata, il file di endpoint basta a fermare un client vecchio."""
        import json

        from rt3.paths import daemon_file

        with LocalDaemon():
            path = daemon_file()
            with open(path, "r", encoding="utf-8") as fh:
                info = json.load(fh)
            info["protocolVersion"] = 42
            with open(path, "w", encoding="utf-8") as fh:
                json.dump(info, fh)
            with self.assertRaises(ProtocolMismatch) as ctx:
                client()
        self.assertIn("v42", str(ctx.exception))

    def daemon_host(self):
        from rt3.daemon import read_daemon_file

        return read_daemon_file()["host"]

    def daemon_port(self):
        from rt3.daemon import read_daemon_file

        return read_daemon_file()["port"]


class RoutingOverHttpTest(Rt3TestCase):
    def test_task_ready_arriva_solo_alla_lane_giusta(self):
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "DEV", taskId="2272")
            start_session(cli, "EDITOR-DEV", "EDITOR", "DEV")
            start_session(cli, "EDITOR-DESIGNER", "EDITOR", "DESIGNER")

            cli.call("event.publish", sessionId="DEV-1", type="TASK_READY")

            dev = cli.call("inbox.list", sessionId="EDITOR-DEV")
            designer = cli.call("inbox.list", sessionId="EDITOR-DESIGNER")

            self.assertEqual(len(dev), 1)
            self.assertEqual(dev[0]["type"], "TASK_READY")
            self.assertEqual(designer, [])

    def test_integration_requested_da_designer_arriva_a_editor_main(self):
        with LocalDaemon():
            cli = client()
            start_session(cli, "EDITOR-DESIGNER", "EDITOR", "DESIGNER")
            start_session(cli, "EDITOR-MAIN", "EDITOR", "MAIN")
            start_session(cli, "EDITOR-DEV", "EDITOR", "DEV")

            cli.call(
                "event.publish",
                sessionId="EDITOR-DESIGNER",
                type="INTEGRATION_REQUESTED",
                note="kit designer pronto",
            )

            self.assertEqual(len(cli.call("inbox.list", sessionId="EDITOR-MAIN")), 1)
            self.assertEqual(cli.call("inbox.list", sessionId="EDITOR-DEV"), [])

    def test_sessione_duplicata_via_http_riporta_il_codice(self):
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "DEV")
            with self.assertRaises(SessionExists):
                start_session(cli, "DEV-1", "DEV", "DEV")


class QuestionAnswerTest(Rt3TestCase):
    def test_messaggio_diretto_fra_due_sessioni(self):
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "DEV", taskId="2272")
            start_session(cli, "EDITOR-DEV", "EDITOR", "DEV")
            start_session(cli, "VALIDATOR-DEV", "VALIDATION", "DEV")

            question = cli.call(
                "event.publish",
                sessionId="EDITOR-DEV",
                type="QUESTION",
                toSession="DEV-1",
                note="quale sha devo aprire?",
            )
            self.assertEqual(question["routingRule"], "EXPLICIT_SESSION")

            # arriva solo al destinatario nominato
            self.assertEqual(len(cli.call("inbox.list", sessionId="DEV-1")), 1)
            self.assertEqual(cli.call("inbox.list", sessionId="VALIDATOR-DEV"), [])

            cli.call("inbox.ack", sessionId="DEV-1", ref=question["eventId"])

            answer = cli.call(
                "event.publish",
                sessionId="DEV-1",
                type="ANSWER",
                toSession="EDITOR-DEV",
                payload={"head": "e" * 40, "branch": "feat/rt3"},
            )
            inbox = cli.call("inbox.list", sessionId="EDITOR-DEV")
            self.assertEqual([r["event_id"] for r in inbox], [answer["eventId"]])

            shown = cli.call(
                "inbox.show", sessionId="EDITOR-DEV", ref=answer["eventId"]
            )
            self.assertEqual(shown["event"]["payload"]["branch"], "feat/rt3")


class OfflineMailboxTest(Rt3TestCase):
    def test_scenario_completo_con_restart_del_coordinator(self):
        """Lo scenario obbligatorio della specifica §17, passo per passo.

        1. il destinatario non e' attivo
        2. il mittente pubblica
        3. rt3d viene riavviato
        4. il destinatario avvia la sessione
        5. l'evento risulta pending
        6. il destinatario fa ack
        7. l'evento non risulta piu' pending
        """
        # 1-2: EDITOR-DEV non esiste ancora, DEV-1 pubblica comunque.
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "DEV", taskId="2272")
            self.assertEqual(
                [s["session_id"] for s in cli.call("sessions.list")], ["DEV-1"]
            )
            published = cli.call(
                "event.publish",
                sessionId="DEV-1",
                type="TASK_READY",
                note="nessun EDITOR ancora aperto",
            )
            self.assertEqual(len(published["deliveries"]), 1)

        # 3: il coordinator e' spento. Nessun processo tiene piu' lo stato in memoria.
        from rt3.daemon import probe

        self.assertIsNone(probe(timeout=0.3))

        with LocalDaemon():
            cli = client()
            # 4: il destinatario arriva adesso, dopo la pubblicazione e dopo il restart.
            start_session(cli, "EDITOR-DEV", "EDITOR", "DEV")

            # 5: l'evento e' li' ad aspettarlo.
            pending = cli.call("inbox.list", sessionId="EDITOR-DEV")
            self.assertEqual(len(pending), 1)
            self.assertEqual(pending[0]["event_id"], published["eventId"])
            self.assertEqual(pending[0]["state"], "PENDING")
            self.assertEqual(pending[0]["sender_session_id"], "DEV-1")
            self.assertEqual(pending[0]["task_id"], "2272")

            # 6-7
            cli.call("inbox.ack", sessionId="EDITOR-DEV", ref=published["eventId"])
            self.assertEqual(cli.call("inbox.list", sessionId="EDITOR-DEV"), [])
            self.assertEqual(
                cli.call("inbox.count", sessionId="EDITOR-DEV")["pending"], 0
            )

            # e resta consultabile: acknowledged, non cancellato
            storico = cli.call("inbox.list", sessionId="EDITOR-DEV", state="ALL")
            self.assertEqual(len(storico), 1)
            self.assertEqual(storico[0]["state"], "ACKED")
            self.assertEqual(storico[0]["acked_by"], "EDITOR-DEV")

    def test_restart_conserva_sessioni_task_ed_eventi(self):
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "DEV", taskId="2272")
            start_session(cli, "EDITOR-DEV", "EDITOR", "DEV")
            cli.call("event.publish", sessionId="DEV-1", type="TASK_STARTED")
            cli.call("event.publish", sessionId="DEV-1", type="TASK_READY")
            prima = cli.call("stats")

        with LocalDaemon():
            cli = client()
            dopo = cli.call("stats")
            self.assertEqual(prima, dopo)
            self.assertEqual(
                sorted(s["session_id"] for s in cli.call("sessions.list")),
                ["DEV-1", "EDITOR-DEV"],
            )
            task = cli.call("task.get", taskId="2272")
            self.assertEqual(
                [e["type"] for e in task["events"]], ["TASK_STARTED", "TASK_READY"]
            )


if __name__ == "__main__":
    unittest.main()
