"""Session identity: quale sessione rappresenta il terminale corrente."""

import os
import unittest

from rt3.binding import ENV_SESSION, bind, read_binding, resolve, unbind
from rt3.errors import SessionUnbound
from tests.harness import Rt3TestCase


class BindingTest(Rt3TestCase):
    def test_senza_binding_l_errore_spiega_come_uscirne(self):
        with self.assertRaises(SessionUnbound) as ctx:
            resolve()
        self.assertIn("rt3 session start", str(ctx.exception))

    def test_non_obbligatorio_ritorna_none(self):
        self.assertEqual(resolve(required=False), (None, None))

    def test_argomento_esplicito_ha_la_precedenza_massima(self):
        os.environ[ENV_SESSION] = "DA-ENV"
        bind("DA-FILE")
        try:
            self.assertEqual(resolve("DA-ARGOMENTO"), ("DA-ARGOMENTO", "--session"))
        finally:
            os.environ.pop(ENV_SESSION, None)

    def test_variabile_ambiente_batte_il_file(self):
        bind("DA-FILE")
        os.environ[ENV_SESSION] = "DA-ENV"
        try:
            session_id, origin = resolve()
            self.assertEqual(session_id, "DA-ENV")
            self.assertEqual(origin, ENV_SESSION)
        finally:
            os.environ.pop(ENV_SESSION, None)

    def test_il_file_di_binding_e_l_ultima_risorsa(self):
        bind("DEV-1")
        session_id, origin = resolve()
        self.assertEqual(session_id, "DEV-1")
        self.assertIn("binding console", origin)

    def test_due_console_diverse_hanno_binding_diversi(self):
        """E' la proprieta' che la cwd non da': stessa directory, sessioni diverse."""
        bind("DEV-1", pid=1111)
        bind("VALIDATOR-1", pid=2222)
        self.assertEqual(read_binding(1111)["sessionId"], "DEV-1")
        self.assertEqual(read_binding(2222)["sessionId"], "VALIDATOR-1")

    def test_unbind_toglie_il_legame(self):
        bind("DEV-1")
        self.assertTrue(unbind())
        with self.assertRaises(SessionUnbound):
            resolve()

    def test_unbind_su_terminale_non_legato_non_esplode(self):
        self.assertFalse(unbind(pid=987654))

    def test_il_binding_vive_sotto_rt3_home(self):
        path = bind("DEV-1")
        self.assertTrue(os.path.abspath(path).startswith(os.path.abspath(self.home)))


if __name__ == "__main__":
    unittest.main()
