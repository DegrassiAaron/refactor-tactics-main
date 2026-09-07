"""Resource enforcement: `WriterCount(WorktreePath) <= 1`, difeso dal database.

L'invariante di questo file non e' una convenzione che il codice si impegna a
rispettare: e' un indice unico parziale su `leases(resource_type, resource_key)
WHERE state='ACTIVE'`. La differenza si vede nel test concorrente - due thread che
partono insieme non hanno una finestra fra il controllo e la scrittura, perche' non c'e'
nessun controllo: si tenta la INSERT e una delle due fallisce.

Prima di questa milestone l'audit misurava `WriterCount = 2` senza alcun rifiuto.
"""

import threading
import unittest

from rt3.errors import (
    LeaseNotFound,
    NotLeaseOwner,
    UnknownSessionField,
    UnrealAlreadyOwned,
    WriterAlreadyOwned,
)
from rt3.model import canonical_path_key
from tests.harness import LocalDaemon, Rt3TestCase, client, start_session

WT = r"D:/Repositories/refactor-tactict-dev"
WT_ALTRO = r"D:/Repositories/refactor-tactics-main"


class WriterLeaseTest(Rt3TestCase):
    def setUp(self):
        super().setUp()
        self.store = self.open_store()

    def _dev(self, sid, mode="READ_ONLY", worktree=WT, **kw):
        return self.store.start_session(
            sid, "DEV", "DEV", "DEV", worktree_path=worktree, write_mode=mode, **kw
        )

    def _writers_su(self, worktree):
        chiave = canonical_path_key(worktree)
        return [
            s
            for s in self.store.list_sessions()
            if s["write_mode"] == "WRITER"
            and canonical_path_key(s["worktree_path"]) == chiave
        ]

    # -- §25 base ---------------------------------------------------------

    def test_read_only_puo_diventare_writer_se_l_albero_e_libero(self):
        self._dev("DEV-1", "READ_ONLY")
        aggiornata = self.store.update_session("DEV-1", write_mode="WRITER")
        self.assertEqual(aggiornata["write_mode"], "WRITER")
        self.assertIsNotNone(
            self.store.get_lease("GIT_WRITER", canonical_path_key(WT))
        )

    # -- §26 collisione ---------------------------------------------------

    def test_il_secondo_writer_sullo_stesso_albero_e_rifiutato(self):
        """🔴 L'invariante della milestone."""
        self._dev("DEV-1", "WRITER")
        with self.assertRaises(WriterAlreadyOwned) as ctx:
            self._dev("DEV-2", "WRITER")
        exc = ctx.exception
        self.assertEqual(exc.code, "RT3_WRITER_ALREADY_OWNED")
        self.assertEqual(exc.owner_session_id, "DEV-1")
        self.assertEqual(exc.requester_session_id, "DEV-2")
        self.assertEqual(exc.resource_key, canonical_path_key(WT))
        self.assertEqual(len(self._writers_su(WT)), 1)

    def test_la_sessione_rifiutata_non_resta_registrata(self):
        """Il rollback tocca anche la sessione: non deve restare una riga che dichiara
        un potere che non ha - e' quella riga che il planner leggerebbe."""
        self._dev("DEV-1", "WRITER")
        with self.assertRaises(WriterAlreadyOwned):
            self._dev("DEV-2", "WRITER")
        self.assertIsNone(self.store.get_session("DEV-2"))

    def test_l_upgrade_rifiutato_non_cambia_il_write_mode(self):
        self._dev("DEV-1", "WRITER")
        self._dev("DEV-2", "READ_ONLY")
        with self.assertRaises(WriterAlreadyOwned):
            self.store.update_session("DEV-2", write_mode="WRITER")
        self.assertEqual(self.store.get_session("DEV-2")["write_mode"], "READ_ONLY")

    def test_alberi_diversi_non_collidono(self):
        """Controllo positivo: se il rifiuto scattasse sempre, i test sopra sarebbero
        verdi per la ragione sbagliata."""
        self._dev("DEV-1", "WRITER", worktree=WT)
        altra = self._dev("MAIN-1", "WRITER", worktree=WT_ALTRO)
        self.assertEqual(altra["write_mode"], "WRITER")

    def test_la_chiave_e_il_path_canonico_non_la_stringa(self):
        """⚠️ Lo stesso albero scritto in due modi deve collidere lo stesso: e' il caso
        reale su Windows, dove maiuscole e separatori variano fra terminali."""
        self._dev("DEV-1", "WRITER", worktree=r"D:/Repositories/refactor-tactict-dev")
        with self.assertRaises(WriterAlreadyOwned):
            self._dev("DEV-2", "WRITER", worktree="D:\\Repositories\\REFACTOR-TACTICT-DEV")

    def test_una_sessione_writer_senza_worktree_e_rifiutata(self):
        """Senza path non c'e' risorsa, e due sessioni non collidono mai."""
        with self.assertRaises(Exception) as ctx:
            self.store.start_session(
                "DEV-X", "DEV", "DEV", "DEV", worktree_path=None, write_mode="WRITER"
            )
        self.assertIn("worktree", str(ctx.exception).lower())

    # -- §27 concorrenza --------------------------------------------------

    def test_due_acquisizioni_CONCORRENTI_ne_lasciano_passare_una(self):
        """🔴 Il test che distingue un vincolo dal database da un `if` applicativo.

        Dieci coppie di thread che partono insieme sulla stessa risorsa. Un
        `SELECT`+`if libero`+`INSERT` avrebbe una finestra, e sotto questa pressione si
        aprirebbe: qui la INSERT viene tentata e basta.
        """
        for giro in range(10):
            chiave = "{}/concorrenza-{}".format(WT, giro)
            self.store.start_session(
                "A-%d" % giro, "DEV", "DEV", "DEV", worktree_path=chiave
            )
            self.store.start_session(
                "B-%d" % giro, "DEV", "DEV", "DEV", worktree_path=chiave
            )
            esiti, barriera = [], threading.Barrier(2)

            def tenta(sid, _chiave=chiave):
                barriera.wait()
                try:
                    self.store.acquire_lease(
                        "GIT_WRITER",
                        canonical_path_key(_chiave),
                        sid,
                        error_class=WriterAlreadyOwned,
                    )
                    esiti.append(("OK", sid))
                except WriterAlreadyOwned:
                    esiti.append(("RIFIUTATO", sid))

            t1 = threading.Thread(target=tenta, args=("A-%d" % giro,))
            t2 = threading.Thread(target=tenta, args=("B-%d" % giro,))
            t1.start(); t2.start(); t1.join(10); t2.join(10)

            vincitori = [e for e in esiti if e[0] == "OK"]
            with self.subTest(giro=giro):
                self.assertEqual(
                    len(vincitori), 1, "esattamente uno deve vincere: {}".format(esiti)
                )
                self.assertEqual(len(esiti), 2, esiti)

    # -- §28/§29 rilascio -------------------------------------------------

    def test_dopo_il_rilascio_un_altro_puo_prendere(self):
        self._dev("DEV-1", "WRITER")
        self._dev("DEV-2", "READ_ONLY")
        self.store.release_lease(
            "GIT_WRITER", canonical_path_key(WT), session_id="DEV-1"
        )
        self.assertEqual(
            self.store.update_session("DEV-2", write_mode="WRITER")["write_mode"],
            "WRITER",
        )

    def test_fermare_la_sessione_rilascia_il_lease(self):
        self._dev("DEV-1", "WRITER")
        fermata = self.store.stop_session("DEV-1")
        self.assertEqual(
            [r["resourceType"] for r in fermata["releasedLeases"]], ["GIT_WRITER"]
        )
        self.assertIsNone(self.store.get_lease("GIT_WRITER", canonical_path_key(WT)))
        self.assertEqual(self._dev("DEV-2", "WRITER")["write_mode"], "WRITER")

    def test_il_downgrade_a_read_only_rilascia(self):
        self._dev("DEV-1", "WRITER")
        self.store.update_session("DEV-1", write_mode="READ_ONLY")
        self.assertIsNone(self.store.get_lease("GIT_WRITER", canonical_path_key(WT)))

    def test_non_si_rilascia_il_lease_di_un_altro_senza_force(self):
        self._dev("DEV-1", "WRITER")
        self._dev("DEV-2", "READ_ONLY")
        with self.assertRaises(NotLeaseOwner):
            self.store.release_lease(
                "GIT_WRITER", canonical_path_key(WT), session_id="DEV-2"
            )
        with self.assertRaises(LeaseNotFound):
            self.store.release_lease("GIT_WRITER", "chiave/inesistente")

    def test_force_strappa_e_lascia_traccia(self):
        self._dev("DEV-1", "WRITER")
        self._dev("DEV-2", "READ_ONLY")
        self.store.release_lease(
            "GIT_WRITER", canonical_path_key(WT), session_id="DEV-2", force=True
        )
        storico = self.store.list_leases(include_released=True)
        strappato = [l for l in storico if l["state"] == "RELEASED"][0]
        self.assertEqual(strappato["released_by"], "DEV-2")

    # -- §30 coesistenza --------------------------------------------------

    def test_molti_read_only_convivono_con_un_writer(self):
        self._dev("DEV-1", "WRITER")
        for sid, role in (
            ("EDITOR-DEV", "EDITOR"),
            ("VALIDATOR-DEV", "VALIDATION"),
            ("DEV-2", "DEV"),
        ):
            sess = self.store.start_session(
                sid, role, "DEV", "DEV", worktree_path=WT, write_mode="READ_ONLY"
            )
            self.assertEqual(sess["write_mode"], "READ_ONLY")
        self.assertEqual(len(self._writers_su(WT)), 1)
        self.assertEqual(len(self.store.list_sessions()), 4)

    # -- stale ------------------------------------------------------------

    def test_un_lease_di_sessione_morta_e_STALE_ma_non_viene_rubato(self):
        """Il control plane non distingue una sessione morta da una che tace."""
        self._dev("DEV-1", "WRITER")
        conn = self.store.connect()
        conn.execute("UPDATE sessions SET status='STOPPED' WHERE session_id='DEV-1'")
        lease = self.store.list_leases()[0]
        self.assertTrue(lease["stale"])
        self.assertEqual(lease["state"], "ACTIVE", "resta ATTIVO: non si libera da solo")
        with self.assertRaises(WriterAlreadyOwned):
            self._dev("DEV-2", "WRITER")


class UnrealLeaseTest(Rt3TestCase):
    def setUp(self):
        super().setUp()
        self.store = self.open_store()

    def test_due_sessioni_non_possono_possedere_l_editor(self):
        self.store.start_session(
            "EDITOR-MAIN", "EDITOR", "MAIN", "MAIN",
            worktree_path=WT_ALTRO, repo_root=WT_ALTRO, unreal_lease="OWNED",
        )
        with self.assertRaises(UnrealAlreadyOwned) as ctx:
            self.store.start_session(
                "DESIGN-DEV", "DEV", "DESIGNER", "DESIGNER",
                worktree_path=WT_ALTRO, repo_root=WT_ALTRO, unreal_lease="OWNED",
            )
        self.assertEqual(ctx.exception.code, "RT3_UNREAL_ALREADY_OWNED")
        self.assertEqual(ctx.exception.owner_session_id, "EDITOR-MAIN")

    def test_git_writer_e_unreal_sono_risorse_DISTINTE(self):
        """§32: possono coesistere su sessioni diverse, e sulla stessa."""
        self.store.start_session(
            "DEV-1", "DEV", "DEV", "DEV", worktree_path=WT, write_mode="WRITER"
        )
        self.store.start_session(
            "EDITOR-MAIN", "EDITOR", "MAIN", "MAIN",
            worktree_path=WT_ALTRO, repo_root=WT_ALTRO, unreal_lease="OWNED",
        )
        tipi = {l["resource_type"]: l["owner_session_id"] for l in self.store.list_leases()}
        self.assertEqual(tipi, {"GIT_WRITER": "DEV-1", "UNREAL_EDITOR": "EDITOR-MAIN"})

    def test_una_sessione_puo_tenere_entrambe(self):
        sess = self.store.start_session(
            "EDITOR-MAIN", "EDITOR", "MAIN", "MAIN",
            worktree_path=WT_ALTRO, repo_root=WT_ALTRO,
            write_mode="WRITER", unreal_lease="OWNED",
        )
        self.assertEqual(sess["write_mode"], "WRITER")
        self.assertEqual(len(self.store.list_leases()), 2)


class StrictUpdateTest(Rt3TestCase):
    """§35: un campo sconosciuto e' un errore, non un no-op."""

    def setUp(self):
        super().setUp()
        self.store = self.open_store()
        self.store.start_session("DEV-1", "DEV", "DEV", "DEV", worktree_path=WT)

    def test_campo_sconosciuto_rifiutato(self):
        with self.assertRaises(UnknownSessionField) as ctx:
            self.store.update_session("DEV-1", unrealLease="OWNED")
        self.assertEqual(ctx.exception.code, "RT3_UNKNOWN_SESSION_FIELD")
        self.assertIn("unrealLease", str(ctx.exception))
        self.assertIn("snake_case", str(ctx.exception))

    def test_i_campi_validi_continuano_a_funzionare(self):
        """Controllo positivo: senza, «rifiuta tutto» supererebbe il test sopra."""
        row = self.store.update_session("DEV-1", task_id="2402", branch="feat/x")
        self.assertEqual(row["task_id"], "2402")
        self.assertEqual(row["branch"], "feat/x")


class RuntimeSnapshotTest(Rt3TestCase):
    def setUp(self):
        super().setUp()
        self.store = self.open_store()

    def test_la_snapshot_dice_chi_tiene_cosa(self):
        self.store.start_session(
            "DEV-1", "DEV", "DEV", "DEV", worktree_path=WT, write_mode="WRITER"
        )
        snap = self.store.runtime_snapshot()
        voce = snap["writers"][canonical_path_key(WT)]
        self.assertEqual(voce["ownerSessionId"], "DEV-1")
        self.assertEqual(voce["workspaceGroup"], "DEV")
        self.assertFalse(voce["stale"])
        self.assertEqual(snap["unreal"], {})
        self.assertEqual(len(snap["sessions"]), 1)

    def test_la_snapshot_e_vuota_quando_nessuno_tiene_niente(self):
        self.store.start_session("DEV-1", "DEV", "DEV", "DEV", worktree_path=WT)
        snap = self.store.runtime_snapshot()
        self.assertEqual(snap["writers"], {})


class LeaseOverHttpTest(Rt3TestCase):
    """Le stesse regole passando dal daemon: e' cosi' che le usa un terminale."""

    def test_collisione_via_daemon(self):
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "DEV", "DEV",
                          worktreePath=WT, writeMode="WRITER")
            with self.assertRaises(WriterAlreadyOwned) as ctx:
                start_session(cli, "DEV-2", "DEV", "DEV", "DEV",
                              worktreePath=WT, writeMode="WRITER")
            self.assertEqual(ctx.exception.owner_session_id, "DEV-1")

    def test_claim_e_release_via_daemon(self):
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "DEV", "DEV", worktreePath=WT)
            chiave = canonical_path_key(WT)
            cli.call("lease.acquire", resourceType="GIT_WRITER",
                     resourceKey=chiave, sessionId="DEV-1")
            self.assertEqual(len(cli.call("leases.list")), 1)
            cli.call("lease.release", resourceType="GIT_WRITER",
                     resourceKey=chiave, sessionId="DEV-1")
            self.assertEqual(cli.call("leases.list"), [])

    def test_il_lease_sopravvive_al_RESTART_del_daemon(self):
        """§36: un daemon che riparte non libera le risorse.

        ⚠️ E' una POLICY, non un dettaglio: liberarle al restart significherebbe che
        riavviare rt3d - cosa che si fa per mille ragioni - autorizza un secondo
        scrittore su un albero dove qualcuno sta lavorando.
        """
        with LocalDaemon():
            cli = client()
            start_session(cli, "DEV-1", "DEV", "DEV", "DEV",
                          worktreePath=WT, writeMode="WRITER")
            prima = cli.call("leases.list")
        with LocalDaemon():
            cli = client()
            self.assertEqual(cli.call("leases.list"), prima)
            start_session(cli, "DEV-2", "DEV", "DEV", "DEV", worktreePath=WT)
            with self.assertRaises(WriterAlreadyOwned):
                cli.call("session.update", sessionId="DEV-2", write_mode="WRITER")


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
