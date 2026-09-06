"""Regole di routing v1 - test puri, senza database e senza daemon.

Ogni caso della specifica §8 ha un test, e i due che contano di piu' sono NEGATIVI:
provare che un messaggio arriva e' facile, provare che non arriva altrove e' il motivo
per cui la lane esiste.
"""

import unittest

from rt3.errors import InvalidEvent
from rt3.routing import route


def recipients(*args, **kwargs):
    return [(r.session_id, r.role, r.lane) for r in route(*args, **kwargs).recipients]


class TaskReadyTest(unittest.TestCase):
    def test_dev_task_ready_va_all_editor_della_stessa_lane(self):
        self.assertEqual(
            recipients("TASK_READY", "DEV", "DEV"), [(None, "EDITOR", "DEV")]
        )

    def test_dev_designer_task_ready_resta_sulla_lane_designer(self):
        """Il caso negativo della specifica: non deve finire alla lane DEV.

        E' la meta' della regola che un test positivo non copre - un routing che
        ignorasse la lane e mandasse tutto a (EDITOR, DEV) passerebbe il test di sopra.
        """
        got = recipients("TASK_READY", "DEV", "DESIGNER")
        self.assertEqual(got, [(None, "EDITOR", "DESIGNER")])
        self.assertNotIn((None, "EDITOR", "DEV"), got)

    def test_task_ready_da_editor_non_ha_regola(self):
        result = route("TASK_READY", "EDITOR", "DEV")
        self.assertEqual(result.recipients, [])
        self.assertIsNone(result.rule)


class ValidationTest(unittest.TestCase):
    def test_editor_dev_chiede_validazione_alla_lane_dev(self):
        self.assertEqual(
            recipients("VALIDATION_REQUESTED", "EDITOR", "DEV"),
            [(None, "VALIDATION", "DEV")],
        )

    def test_editor_designer_chiede_validazione_alla_lane_designer(self):
        got = recipients("VALIDATION_REQUESTED", "EDITOR", "DESIGNER")
        self.assertEqual(got, [(None, "VALIDATION", "DESIGNER")])
        self.assertNotIn((None, "VALIDATION", "DEV"), got)

    def test_verdetto_torna_all_editor_della_stessa_lane(self):
        for verdict in ("VALIDATION_PASSED", "VALIDATION_FAILED"):
            with self.subTest(verdict=verdict):
                self.assertEqual(
                    recipients(verdict, "VALIDATION", "DEV"), [(None, "EDITOR", "DEV")]
                )
                self.assertEqual(
                    recipients(verdict, "VALIDATION", "DESIGNER"),
                    [(None, "EDITOR", "DESIGNER")],
                )


class IntegrationTest(unittest.TestCase):
    def test_editor_designer_integra_verso_editor_main(self):
        self.assertEqual(
            recipients("INTEGRATION_REQUESTED", "EDITOR", "DESIGNER"),
            [(None, "EDITOR", "MAIN")],
        )

    def test_editor_dev_integra_verso_editor_main(self):
        self.assertEqual(
            recipients("INTEGRATION_REQUESTED", "EDITOR", "DEV"),
            [(None, "EDITOR", "MAIN")],
        )

    def test_editor_gia_su_main_non_si_autoconsegna(self):
        """Caso non enumerato dalla specifica, e deliberatamente senza regola.

        Instradarlo a (EDITOR, MAIN) avrebbe creato una consegna che il mittente stesso
        puo' raccogliere: un promemoria travestito da handoff.
        """
        result = route("INTEGRATION_REQUESTED", "EDITOR", "MAIN")
        self.assertEqual(result.recipients, [])
        self.assertIn("MAIN e' il mittente", result.reason)


class DirectedTest(unittest.TestCase):
    def test_question_esige_un_destinatario(self):
        with self.assertRaises(InvalidEvent) as ctx:
            route("QUESTION", "DEV", "DEV")
        self.assertIn("destinatario esplicito", str(ctx.exception))

    def test_question_a_una_sessione_nominata(self):
        self.assertEqual(
            recipients("QUESTION", "DEV", "DEV", recipient_session_id="EDITOR-DEV"),
            [("EDITOR-DEV", None, None)],
        )

    def test_answer_a_un_ruolo_su_lane_esplicita(self):
        self.assertEqual(
            recipients(
                "ANSWER", "EDITOR", "MAIN", recipient_role="DEV", recipient_lane="DESIGNER"
            ),
            [(None, "DEV", "DESIGNER")],
        )

    def test_ruolo_senza_lane_eredita_quella_del_mittente(self):
        self.assertEqual(
            recipients("QUESTION", "VALIDATION", "DESIGNER", recipient_role="EDITOR"),
            [(None, "EDITOR", "DESIGNER")],
        )

    def test_destinatario_ambiguo_e_rifiutato(self):
        with self.assertRaises(InvalidEvent):
            route(
                "QUESTION",
                "DEV",
                "DEV",
                recipient_session_id="EDITOR-DEV",
                recipient_role="EDITOR",
            )


class OverrideTest(unittest.TestCase):
    def test_destinatario_esplicito_batte_la_regola_automatica(self):
        result = route(
            "TASK_READY", "DEV", "DEV", recipient_session_id="VALIDATOR-DEV"
        )
        self.assertEqual(result.rule, "EXPLICIT_SESSION")
        self.assertEqual(
            [r.session_id for r in result.recipients], ["VALIDATOR-DEV"]
        )


class ValidationOfInputTest(unittest.TestCase):
    def test_tipo_evento_sconosciuto(self):
        with self.assertRaises(InvalidEvent):
            route("TASK_ALMOST_READY", "DEV", "DEV")

    def test_ruolo_mittente_sconosciuto(self):
        with self.assertRaises(InvalidEvent):
            route("TASK_READY", "DEV-LEAD", "DEV")

    def test_lane_mittente_sconosciuta(self):
        with self.assertRaises(InvalidEvent):
            route("TASK_READY", "DEV", "TECHNICAL_DESIGNER")


if __name__ == "__main__":
    unittest.main()
