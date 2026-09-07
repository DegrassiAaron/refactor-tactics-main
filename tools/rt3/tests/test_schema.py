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


class MigrationToV4Test(Rt3TestCase):
    """§37: un database gia' in uso arriva a v4 senza perdere niente.

    Costruisce un database FERMO a v3 - le migrazioni applicate a mano fino a li' - ci
    scrive dati veri, e poi lo apre con `open_store`, che lo migra. Un test che partisse
    da un database vuoto proverebbe la creazione, non la migrazione: e la domanda di
    questa milestone e' se un runtime.db esistente sopravvive.
    """

    def _database_a_v3(self):
        import sqlite3

        from rt3.paths import db_path, ensure_store_root
        from rt3.store import _MIGRATIONS, Store

        ensure_store_root()
        store = Store(db_path())
        self._stores.append(store)
        conn = store.connect()
        for versione in (1, 2, 3):
            conn.execute("BEGIN IMMEDIATE")
            _MIGRATIONS[versione](conn)
            conn.execute(
                "INSERT INTO meta(key, value) VALUES('schema_version', ?) "
                "ON CONFLICT(key) DO UPDATE SET value=excluded.value",
                (str(versione),),
            )
            conn.execute("COMMIT")
        return store, conn

    def test_da_v3_a_v4_conservando_i_dati(self):
        store, conn = self._database_a_v3()
        self.assertEqual(store.schema_version(), 3)
        conn.execute(
            "INSERT INTO sessions(session_id, role, workspace_group, lane, "
            "started_at, last_seen_at) VALUES('DEV-1','DEV','DEV','DEV','t','t')"
        )
        conn.execute(
            "INSERT INTO roadmaps(roadmap_id, content_hash, roadmap_schema_version, "
            "document, loaded_at) VALUES('r','h',1,'{}','t')"
        )
        store.close()

        from rt3.paths import db_path
        from rt3.store import open_store

        migrato = open_store(db_path())
        self._stores.append(migrato)
        self.assertEqual(migrato.schema_version(), SCHEMA_VERSION)
        self.assertIsNotNone(migrato.get_session("DEV-1"))
        self.assertEqual(len(migrato.list_roadmaps()), 1)

    def test_la_tabella_leases_v1_vuota_viene_sostituita(self):
        store, conn = self._database_a_v3()
        store.close()
        from rt3.paths import db_path
        from rt3.store import open_store

        migrato = open_store(db_path())
        self._stores.append(migrato)
        colonne = {
            r["name"]
            for r in migrato.connect().execute("PRAGMA table_info(leases)")
        }
        self.assertIn("resource_type", colonne)
        self.assertIn("owner_session_id", colonne)
        self.assertNotIn("holder", colonne, "la forma v1 non deve sopravvivere")
        tabelle = {
            r[0]
            for r in migrato.connect().execute(
                "SELECT name FROM sqlite_master WHERE type='table'"
            )
        }
        self.assertNotIn("leases_legacy_v1", tabelle, "vuota: si toglie di mezzo")

    def test_una_tabella_leases_v1_con_righe_viene_CONSERVATA(self):
        """Un database che ho promesso di migrare non perde dati nemmeno quando sono
        certo che non ce ne siano."""
        store, conn = self._database_a_v3()
        conn.execute(
            "INSERT INTO leases(lease_id, resource, holder, state) "
            "VALUES('l1','UNREAL','X','ACTIVE')"
        )
        store.close()
        from rt3.paths import db_path
        from rt3.store import open_store

        migrato = open_store(db_path())
        self._stores.append(migrato)
        righe = migrato.connect().execute(
            "SELECT holder FROM leases_legacy_v1"
        ).fetchall()
        self.assertEqual([r["holder"] for r in righe], ["X"])
        self.assertEqual(migrato.list_leases(), [], "la nuova nasce vuota")

    def test_l_indice_unico_esiste_ed_e_PARZIALE(self):
        """⚠️ Senza `WHERE state='ACTIVE'` un lease rilasciato impedirebbe per sempre
        di riacquisire la stessa risorsa."""
        store = self.open_store()
        riga = store.connect().execute(
            "SELECT sql FROM sqlite_master WHERE name='idx_lease_exclusive'"
        ).fetchone()
        self.assertIsNotNone(riga, "l'indice unico e' la garanzia dell'invariante")
        self.assertIn("UNIQUE", riga["sql"].upper())
        self.assertIn("WHERE", riga["sql"].upper())
