"""Session Registry ed Event Store: registrazione, persistenza, mailbox, ack."""

import os
import unittest

from rt3.errors import (
    DeliveryConflict,
    NotAuthorized,
    SessionExists,
    SessionNotFound,
)
from tests.harness import Rt3TestCase


class SessionRegistryTest(Rt3TestCase):
    def test_registrazione_conserva_i_campi_dichiarati(self):
        store = self.open_store()
        session = store.start_session(
            "DEV-1",
            "DEV",
            "DEV",
            "DEV",
            worktree_path="D:/Repositories/refactor-tactict-dev",
            repo_root="D:/Repositories/refactor-tactict-dev",
            branch="feat/rt3",
            head="a" * 40,
            task_id="2272",
            write_mode="WRITER",
            write_set=["Source/RefactorTactics/Turn"],
        )
        self.assertEqual(session["role"], "DEV")
        self.assertEqual(session["lane"], "DEV")
        self.assertEqual(session["status"], "ACTIVE")
        self.assertEqual(session["task_id"], "2272")
        self.assertEqual(session["write_mode"], "WRITER")
        self.assertEqual(session["write_set"], ["Source/RefactorTactics/Turn"])

    def test_branch_nullo_e_uno_stato_legittimo(self):
        """Detached HEAD: `branch` resta None e la registrazione riesce."""
        store = self.open_store()
        session = store.start_session(
            "BUILD-1", "VALIDATION", "MAIN", "MAIN", branch=None, head="b" * 40
        )
        self.assertIsNone(session["branch"])
        self.assertEqual(session["head"], "b" * 40)

    def test_session_id_duplicato_e_rifiutato_finche_e_attiva(self):
        store = self.open_store()
        store.start_session("DEV-1", "DEV", "DEV", "DEV")
        with self.assertRaises(SessionExists):
            store.start_session("DEV-1", "EDITOR", "MAIN", "MAIN")
        # e il rifiuto non ha alterato la sessione esistente
        self.assertEqual(store.get_session("DEV-1")["role"], "DEV")

    def test_replace_forza_il_riuso_di_una_sessione_attiva(self):
        store = self.open_store()
        store.start_session("DEV-1", "DEV", "DEV", "DEV")
        session = store.start_session(
            "DEV-1", "EDITOR", "MAIN", "MAIN", replace=True
        )
        self.assertEqual(session["role"], "EDITOR")

    def test_id_di_una_sessione_fermata_si_riusa_senza_replace(self):
        store = self.open_store()
        store.start_session("DEV-1", "DEV", "DEV", "DEV")
        store.stop_session("DEV-1")
        session = store.start_session("DEV-1", "DEV", "DEV", "DEV")
        self.assertEqual(session["status"], "ACTIVE")
        self.assertIsNone(session["stopped_at"])

    def test_sessione_sconosciuta(self):
        store = self.open_store()
        with self.assertRaises(SessionNotFound):
            store.get_session("MAI-ESISTITA", required=True)

    def test_persistenza_attraverso_la_riapertura_dello_store(self):
        """Il database sopravvive alla chiusura del processo che lo ha scritto."""
        store = self.open_store()
        store.start_session("DEV-1", "DEV", "DEV", "DEV", task_id="2272")
        store.close()

        riaperto = self.open_store()
        session = riaperto.get_session("DEV-1", required=True)
        self.assertEqual(session["task_id"], "2272")
        self.assertEqual(session["status"], "ACTIVE")

    def test_il_file_del_database_e_sotto_rt3_home(self):
        store = self.open_store()
        self.assertTrue(os.path.isfile(store.path))
        self.assertTrue(os.path.abspath(store.path).startswith(os.path.abspath(self.home)))


class EventStoreTest(Rt3TestCase):
    def setUp(self):
        super().setUp()
        self.store = self.open_store()
        self.store.start_session("DEV-1", "DEV", "DEV", "DEV", task_id="2272")
        self.store.start_session("EDITOR-DEV", "EDITOR", "DEV", "DEV")

    def test_publish_registra_evento_e_consegna(self):
        result = self.store.publish("DEV-1", "TASK_READY", note="pronto")
        self.assertTrue(result["eventId"].startswith("ev_"))
        self.assertEqual(result["routingRule"], "DEV_TASK_READY_TO_EDITOR_SAME_LANE")
        self.assertEqual(len(result["deliveries"]), 1)

        event = self.store.get_event(result["eventId"])
        self.assertEqual(event["type"], "TASK_READY")
        self.assertEqual(event["sender_session_id"], "DEV-1")
        self.assertEqual(event["task_id"], "2272")
        self.assertEqual(event["note"], "pronto")

    def test_payload_arriva_intatto(self):
        result = self.store.publish(
            "DEV-1",
            "TASK_READY",
            payload={"branch": "feat/rt3", "head": "c" * 40, "tests": ["Automation"]},
        )
        event = self.store.get_event(result["eventId"])
        self.assertEqual(event["payload"]["branch"], "feat/rt3")
        self.assertEqual(event["payload"]["tests"], ["Automation"])

    def test_evento_senza_destinatario_e_registrato_ma_non_consegnato(self):
        """Il caso che non deve perdere l'evento ne' fingere di averlo consegnato."""
        result = self.store.publish("EDITOR-DEV", "REVIEW_APPROVED")
        self.assertEqual(result["deliveries"], [])
        self.assertIsNone(result["routingRule"])
        self.assertIsNotNone(self.store.get_event(result["eventId"]))

    def test_evento_verso_una_sessione_inesistente_e_rifiutato(self):
        """Un id scritto male resterebbe pending per sempre: meglio il rifiuto subito."""
        with self.assertRaises(SessionNotFound):
            self.store.publish(
                "DEV-1", "QUESTION", recipient_session_id="EDITOR-DEB"
            )

    def test_retrieval_e_ack(self):
        published = self.store.publish("DEV-1", "TASK_READY")
        pending = self.store.inbox("EDITOR-DEV")
        self.assertEqual(len(pending), 1)
        self.assertEqual(pending[0]["event_id"], published["eventId"])
        self.assertEqual(pending[0]["state"], "PENDING")

        acked = self.store.ack("EDITOR-DEV", published["eventId"])
        self.assertEqual(acked["state"], "ACKED")
        self.assertEqual(acked["acked_by"], "EDITOR-DEV")

        self.assertEqual(self.store.inbox("EDITOR-DEV"), [])
        self.assertEqual(len(self.store.inbox("EDITOR-DEV", state="ALL")), 1)
        self.assertEqual(self.store.pending_count("EDITOR-DEV"), 0)

    def test_secondo_ack_perde_e_dice_chi_ha_vinto(self):
        published = self.store.publish("DEV-1", "TASK_READY")
        self.store.ack("EDITOR-DEV", published["eventId"])
        self.store.start_session("EDITOR-DEV-2", "EDITOR", "DEV", "DEV")
        with self.assertRaises(DeliveryConflict) as ctx:
            self.store.ack("EDITOR-DEV-2", published["eventId"])
        self.assertIn("EDITOR-DEV", str(ctx.exception))

    def test_una_sessione_incompatibile_non_vede_e_non_puo_prendere(self):
        """La separazione per lane e' cio' che impedisce il consumo accidentale."""
        self.store.start_session("EDITOR-DESIGNER", "EDITOR", "DESIGNER", "DESIGNER")
        published = self.store.publish("DEV-1", "TASK_READY")

        self.assertEqual(self.store.inbox("EDITOR-DESIGNER"), [])
        with self.assertRaises(NotAuthorized):
            self.store.ack("EDITOR-DESIGNER", published["eventId"])
        # e la consegna e' rimasta disponibile per chi di dovere
        self.assertEqual(self.store.pending_count("EDITOR-DEV"), 1)

    def test_chi_fa_ack_adotta_il_task_dell_evento(self):
        """Senza questo, la risposta dell'EDITOR non sarebbe legata al task.

        E' il difetto che lo smoke test ha scoperto: il task 2272 mostrava i due eventi
        di DEV-1 e non i due della risposta, perche' EDITOR-DEV non aveva task e nessuno
        glielo aveva passato - cioe' avrebbe dovuto ricopiarlo a mano.
        """
        self.assertIsNone(self.store.get_session("EDITOR-DEV")["task_id"])
        published = self.store.publish("DEV-1", "TASK_READY")
        self.store.ack("EDITOR-DEV", published["eventId"])
        self.assertEqual(self.store.get_session("EDITOR-DEV")["task_id"], "2272")

        # e da li' in poi i suoi eventi finiscono sul task, senza --task
        risposta = self.store.publish("EDITOR-DEV", "VALIDATION_REQUESTED")
        self.assertEqual(
            self.store.get_event(risposta["eventId"])["task_id"], "2272"
        )

    def test_l_adozione_non_sovrascrive_un_task_gia_dichiarato(self):
        """Una sessione che ha scelto il proprio lavoro non viene spostata."""
        self.store.start_session(
            "EDITOR-ALTRO", "EDITOR", "DEV", "DEV", task_id="9999"
        )
        published = self.store.publish("DEV-1", "TASK_READY")
        self.store.ack("EDITOR-ALTRO", published["eventId"])
        self.assertEqual(self.store.get_session("EDITOR-ALTRO")["task_id"], "9999")

    def test_ack_per_delivery_id_oltre_che_per_event_id(self):
        published = self.store.publish("DEV-1", "TASK_READY")
        acked = self.store.ack("EDITOR-DEV", published["deliveries"][0])
        self.assertEqual(acked["event_id"], published["eventId"])

    def test_ordinamento_per_seq_e_non_per_timestamp(self):
        """Due eventi nello stesso secondo devono restare ordinati."""
        first = self.store.publish("DEV-1", "TASK_READY")
        second = self.store.publish("DEV-1", "TASK_READY")
        rows = self.store.inbox("EDITOR-DEV")
        self.assertEqual(
            [r["event_id"] for r in rows], [first["eventId"], second["eventId"]]
        )


class TaskRegistryTest(Rt3TestCase):
    def test_il_task_nasce_dalla_sessione_che_lo_dichiara(self):
        store = self.open_store()
        store.start_session("DEV-1", "DEV", "DEV", "DEV", task_id="2272")
        task = store.get_task("2272", required=True)
        self.assertEqual(task["task_id"], "2272")
        self.assertEqual(task["status"], "ACTIVE")
        self.assertEqual([s["session_id"] for s in task["sessions"]], ["DEV-1"])

    def test_gli_eventi_del_task_sono_recuperabili(self):
        store = self.open_store()
        store.start_session("DEV-1", "DEV", "DEV", "DEV", task_id="2272")
        store.start_session("EDITOR-DEV", "EDITOR", "DEV", "DEV")
        store.publish("DEV-1", "TASK_STARTED")
        store.publish("DEV-1", "TASK_READY")
        task = store.get_task("2272")
        self.assertEqual([e["type"] for e in task["events"]], ["TASK_STARTED", "TASK_READY"])


class CandidateTest(Rt3TestCase):
    def test_candidate_eredita_branch_e_head_della_sessione(self):
        store = self.open_store()
        store.start_session(
            "DEV-1", "DEV", "DEV", "DEV", task_id="2272", branch="feat/rt3", head="d" * 40
        )
        cand = store.create_candidate("DEV-1", note="prima passata")
        self.assertEqual(cand["branch"], "feat/rt3")
        self.assertEqual(cand["head"], "d" * 40)
        self.assertEqual(cand["task_id"], "2272")


if __name__ == "__main__":
    unittest.main()
