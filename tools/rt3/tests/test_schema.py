"""Schema, migrazioni e versioni.

Il test sullo splitter DDL non e' cosmetico: pinna il difetto che ha rotto la prima
migrazione - un punto e virgola dentro la PROSA di un commento SQL - e che si presentava
come `near "un": syntax error`, cioe' un messaggio che nomina una parola italiana e non
dice nulla dello statement vero.
"""

import unittest

from rt3 import SCHEMA_VERSION
from rt3.errors import SchemaMismatch
from rt3.store import _split_statements
from tests.harness import Rt3TestCase


class SplitStatementsTest(unittest.TestCase):
    def test_divide_sul_terminatore(self):
        self.assertEqual(
            _split_statements("CREATE TABLE a(x); CREATE TABLE b(y);"),
            ["CREATE TABLE a(x)", " CREATE TABLE b(y)"],
        )

    def test_il_punto_e_virgola_di_un_commento_non_divide(self):
        script = """
        -- prima frase; seconda frase
        CREATE TABLE a(x);
        """
        statements = _split_statements(script)
        self.assertEqual(len(statements), 1)
        self.assertIn("CREATE TABLE a(x)", statements[0])

    def test_scarta_i_frammenti_vuoti(self):
        self.assertEqual(_split_statements(";;\n  \n;"), [])


class SchemaTest(Rt3TestCase):
    def test_la_migrazione_porta_il_database_alla_versione_corrente(self):
        store = self.open_store()
        self.assertEqual(store.schema_version(), SCHEMA_VERSION)

    def test_migrare_due_volte_e_innocuo(self):
        store = self.open_store()
        self.assertEqual(store.migrate(), SCHEMA_VERSION)
        self.assertEqual(store.migrate(), SCHEMA_VERSION)

    def test_tutte_le_tabelle_previste_esistono(self):
        store = self.open_store()
        rows = store.connect().execute(
            "SELECT name FROM sqlite_master WHERE type='table'"
        ).fetchall()
        names = {r["name"] for r in rows}
        for expected in (
            "sessions",
            "tasks",
            "events",
            "deliveries",
            "candidates",
            "leases",
            "meta",
        ):
            self.assertIn(expected, names)

    def test_database_piu_nuovo_del_codice_e_rifiutato(self):
        """Il caso dei tre workspace disallineati, visto dal lato del database.

        Un checkout indietro non deve poter APRIRE un database scritto da uno avanti:
        leggerebbe colonne che non conosce e ne ignorerebbe altre in silenzio.
        """
        store = self.open_store()
        store.connect().execute(
            "UPDATE meta SET value=? WHERE key='schema_version'",
            (str(SCHEMA_VERSION + 7),),
        )
        store.close()

        with self.assertRaises(SchemaMismatch) as ctx:
            self.open_store()
        self.assertIn("piu' VECCHIO", str(ctx.exception))

    def test_la_consegna_non_puo_avere_due_destinatari(self):
        """Il CHECK del database, non una convenzione del codice applicativo."""
        import sqlite3

        store = self.open_store()
        store.start_session("DEV-1", "DEV", "DEV", "DEV")
        published = store.publish("DEV-1", "TASK_READY")
        with self.assertRaises(sqlite3.IntegrityError):
            store.connect().execute(
                "INSERT INTO deliveries(delivery_id, event_id, recipient_session_id, "
                "recipient_role, recipient_lane, state, created_at) "
                "VALUES('dl_x', ?, 'DEV-1', 'EDITOR', 'DEV', 'PENDING', '2026-01-01T00:00:00Z')",
                (published["eventId"],),
            )


if __name__ == "__main__":
    unittest.main()
