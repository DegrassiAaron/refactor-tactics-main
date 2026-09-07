"""Lo StatusSnapshot: cosa contiene, quando cambia davvero, e come si mostra.

Il test che regge questo file e' `NoSpamTest`. Uno status automatico che si ripete
identico a ogni comando smette di essere letto, e con lui smette di essere letto quello
importante: la differenza fra «stato cambiato» e «tempo passato» e' l'unica cosa che
rende sopportabile stampare lo status dopo ogni azione.
"""

import time
import unittest

from rt3.status import (
    SIGNIFICANT,
    build_snapshot,
    diff,
    has_significant_diff,
    message_class,
    render,
    render_diff,
    state_revision,
)

SESSIONE_DEV = {
    "session_id": "DEV-GRID-1", "role": "DEV", "lane": "DEV",
    "workspace_group": "DEV", "status": "ACTIVE", "task_id": "GRID-2303",
    "write_mode": "WRITER", "worktree_path": r"D:/alberi/dev", "candidate_id": None,
}
GIT = {"worktreePath": r"D:/alberi/dev", "repoRoot": r"D:/alberi/dev",
       "branch": "feat/grid", "head": "a" * 40}
VERSIONI = {"protocolVersion": 3, "schemaVersion": 4, "roadmapSchemaVersion": 1}


def snap(**kw):
    base = dict(
        session=SESSIONE_DEV, git=GIT, leases={"writer": {"owner": "DEV-GRID-1"}},
        inbox_pending=0, versions=VERSIONI, generated_at="2026-09-07T10:00:00Z",
        roadmap={"roadmap_id": "wave-01", "content_hash": "sha256:abc"},
        planner={"epicId": "EPIC-GRID", "taskState": "IN_PROGRESS"},
    )
    base.update(kw)
    return build_snapshot(**base)


class SnapshotTest(unittest.TestCase):
    """§24: i campi minimi ci sono, e vengono dalle fonti dichiarate."""

    def test_contiene_tutti_i_campi_richiesti(self):
        s = snap()
        for campo in (
            "stateRevision", "generatedAt", "sessionId", "role", "lane",
            "workspaceGroup", "epicId", "issueId", "candidateId", "sessionState",
            "taskState", "worktreePath", "branch", "head", "writeMode",
            "writerLeaseState", "writerLeaseOwner", "unrealLeaseState",
            "unrealLeaseOwner", "inboxPending", "blockingReason", "requiredAction",
            "roadmapRevision", "protocolVersion", "schemaVersion",
        ):
            self.assertIn(campo, s, campo)

    def test_git_ha_la_precedenza_sui_metadati_della_sessione(self):
        """La sessione dichiara cio' che ha visto all'avvio; nel frattempo il branch
        puo' essere cambiato. Lo status deve dire quello VERO."""
        sess = dict(SESSIONE_DEV, branch="vecchio", head="b" * 40)
        s = snap(session=sess, git=dict(GIT, branch="nuovo", head="c" * 40))
        self.assertEqual(s["branch"], "nuovo")
        self.assertEqual(s["head"], "c" * 40)

    def test_senza_sessione_i_campi_restano_None_non_vuoti(self):
        """`None` e' diverso da stringa vuota: «non c'e' sessione» non e' «si chiama
        nulla»."""
        s = build_snapshot(git=GIT, versions=VERSIONI)
        self.assertIsNone(s["sessionId"])
        self.assertIsNone(s["role"])
        self.assertEqual(s["inboxPending"], 0)

    def test_lease_riflessi(self):
        s = snap(leases={"writer": {"owner": "ALTRO"}, "unreal": {"owner": "EDITOR-MAIN"}})
        self.assertEqual(s["writerLeaseState"], "OWNED")
        self.assertEqual(s["writerLeaseOwner"], "ALTRO")
        self.assertEqual(s["unrealLeaseState"], "OWNED")
        self.assertEqual(s["unrealLeaseOwner"], "EDITOR-MAIN")
        vuoto = snap(leases={})
        self.assertEqual(vuoto["writerLeaseState"], "NONE")
        self.assertIsNone(vuoto["writerLeaseOwner"])

    def test_valori_non_ammessi_sono_rifiutati(self):
        with self.assertRaises(ValueError):
            snap(blocking="QUALCOSA")
        with self.assertRaises(ValueError):
            snap(required_action="FAI_QUALCOSA")


class NoSpamTest(unittest.TestCase):
    """§25: il tempo che passa non e' un cambiamento di stato."""

    def test_snapshot_identici_non_hanno_diff(self):
        self.assertEqual(diff(snap(), snap()), [])
        self.assertFalse(has_significant_diff(snap(), snap()))

    def test_solo_il_timestamp_diverso_non_e_un_diff(self):
        """🔴 La regola che rende sopportabile lo status automatico."""
        a = snap(generated_at="2026-09-07T10:00:00Z")
        b = snap(generated_at="2026-09-07T23:59:59Z")
        self.assertNotEqual(a["generatedAt"], b["generatedAt"])
        self.assertEqual(a["stateRevision"], b["stateRevision"], "stessa revisione")
        self.assertEqual(diff(a, b), [])
        self.assertEqual(render_diff(diff(a, b)), "")

    def test_inbox_da_0_a_1_e_un_diff_significativo(self):
        a, b = snap(inbox_pending=0), snap(inbox_pending=1)
        cambi = diff(a, b)
        self.assertEqual(len(cambi), 1)
        self.assertEqual(cambi[0]["field"], "inboxPending")
        self.assertEqual((cambi[0]["from"], cambi[0]["to"]), (0, 1))
        self.assertNotEqual(a["stateRevision"], b["stateRevision"])

    def test_ogni_campo_significativo_produce_un_diff(self):
        """Controllo positivo: se il diff guardasse solo l'inbox, il test sopra sarebbe
        verde per la ragione sbagliata."""
        base = snap()
        for campo in SIGNIFICANT:
            with self.subTest(campo=campo):
                mutato = dict(base)
                mutato[campo] = "VALORE-DIVERSO-{}".format(campo)
                self.assertTrue(
                    has_significant_diff(base, mutato), "{} non produce diff".format(campo)
                )

    def test_il_primo_snapshot_non_produce_diff(self):
        self.assertEqual(diff(None, snap()), [])

    def test_render_diff_mostra_da_e_a_piu_il_compact(self):
        testo = render_diff(diff(snap(inbox_pending=0), snap(inbox_pending=1)),
                            snap(inbox_pending=1))
        self.assertIn("[RT3 NOTICE]", testo)
        self.assertIn("Inbox:", testo)
        self.assertIn("0 -> 1", testo)
        self.assertIn("[RT3]", testo, "il compact segue il notice")

    def test_la_revisione_non_dipende_dall_ordine_delle_chiavi(self):
        a = snap()
        b = dict(reversed(list(a.items())))
        b["stateRevision"] = state_revision(b)
        self.assertEqual(a["stateRevision"], b["stateRevision"])


class RenderTest(unittest.TestCase):
    """§6, §11: tre livelli, e un ordine che dipende dal ruolo."""

    def test_compact_e_una_riga_sola(self):
        riga = render(snap(), "compact")
        self.assertEqual(len(riga.splitlines()), 1)
        self.assertTrue(riga.startswith("[RT3]"))
        for pezzo in ("DEV-GRID-1", "GRID-2303", "IN_PROGRESS", "WRITER", "Inbox 0"):
            self.assertIn(pezzo, riga)

    def test_compact_distingue_writer_CON_e_SENZA_lease(self):
        """Dichiararsi WRITER senza tenere il lease non e' la stessa cosa di tenerlo."""
        con = render(snap(), "compact")
        senza = render(snap(leases={}), "compact")
        self.assertIn("WRITER+", con)
        self.assertIn("WRITER!", senza)

    def test_normal_ha_le_sezioni_attese(self):
        testo = render(snap(), "normal")
        for pezzo in ("[RT3 SESSION]", "Session: DEV-GRID-1", "Role: DEV", "Lane: DEV",
                      "Epic: EPIC-GRID", "Task: GRID-2303", "Mode: WRITER",
                      "WriterLease: OWNED", "UnrealLease: NONE", "Inbox: 0"):
            self.assertIn(pezzo, testo)

    def test_verbose_aggiunge_i_campi_diagnostici(self):
        testo = render(snap(), "verbose")
        for pezzo in ("Worktree:", "Branch:", "HEAD:", "RoadmapRevision:",
                      "StateRevision:", "GeneratedAt:", "Protocol/Schema/Roadmap:"):
            self.assertIn(pezzo, testo)
        self.assertNotIn("StateRevision:", render(snap(), "normal"))

    def test_validation_mostra_prima_il_candidate(self):
        sess = dict(SESSIONE_DEV, role="VALIDATION", candidate_id="cand_123")
        testo = render(snap(session=sess), "normal", "VALIDATION")
        self.assertIn("Candidate: cand_123", testo)
        self.assertLess(testo.index("Candidate:"), testo.index("Mode:"))

    def test_editor_mostra_l_epic(self):
        sess = dict(SESSIONE_DEV, role="EDITOR")
        self.assertIn("Epic: EPIC-GRID", render(snap(session=sess), "normal", "EDITOR"))

    def test_livello_non_ammesso_e_rifiutato(self):
        with self.assertRaises(ValueError):
            render(snap(), "dettagliatissimo")


class MessageClassTest(unittest.TestCase):
    """§7: la classe deriva dallo stato, non da chi stampa."""

    def test_stato_normale_e_STATUS(self):
        self.assertEqual(message_class(snap()), "STATUS")

    def test_azione_richiesta(self):
        self.assertEqual(message_class(snap(required_action="IMPLEMENT_ISSUE")),
                         "ACTION_REQUIRED")

    def test_blocco_vince_sull_azione(self):
        s = snap(blocking="RESOURCE_WRITER", required_action="RESOLVE_WRITER_CONFLICT")
        self.assertEqual(message_class(s), "BLOCKED")

    def test_il_banner_riflette_la_classe(self):
        self.assertIn("[RT3 BLOCKED]", render(snap(blocking="DEPENDENCY"), "normal"))
        self.assertIn("[RT3 ACTION_REQUIRED]",
                      render(snap(required_action="WAIT_REVIEW"), "normal"))


class FailFastTest(unittest.TestCase):
    """§8, §26: quando qualcosa non quadra NON deve comparire READY."""

    CASI = ("WORKTREE_MISMATCH", "RESOURCE_WRITER", "RESOURCE_UNREAL", "PROTOCOL_MISMATCH")

    def test_uno_stato_bloccato_non_dice_mai_READY(self):
        for motivo in self.CASI:
            with self.subTest(motivo=motivo):
                testo = render(snap(blocking=motivo), "normal")
                self.assertNotIn("READY", testo)
                self.assertIn("[RT3 BLOCKED]", testo)
                self.assertIn(motivo, testo)

    def test_anche_il_compact_segnala_il_blocco(self):
        for motivo in self.CASI:
            with self.subTest(motivo=motivo):
                self.assertIn("BLOCKED", render(snap(blocking=motivo), "compact"))


class PerformanceTest(unittest.TestCase):
    """§31: il compact deve costare poco - viene stampato dopo ogni azione."""

    def test_mille_render_compact_sotto_un_secondo(self):
        s = snap()
        inizio = time.perf_counter()
        for _ in range(1000):
            render(s, "compact")
        durata = time.perf_counter() - inizio
        self.assertLess(durata, 1.0, "1000 render compact in {:.3f}s".format(durata))

    def test_costruire_lo_snapshot_e_economico(self):
        inizio = time.perf_counter()
        for _ in range(1000):
            snap()
        durata = time.perf_counter() - inizio
        self.assertLess(durata, 2.0, "1000 snapshot in {:.3f}s".format(durata))


class AsciiOutputTest(unittest.TestCase):
    """Lo status si stampa dopo OGNI azione: non puo' dipendere dalla codepage.

    Il gemello di questo test, in test_bootstrap.py, ha colto un em dash che faceva
    terminare `rt3 epic activate` con exit 1 su cp850. Qui si presidia la stessa
    superficie per i render dello status, che sono quelli stampati piu' spesso.
    """

    CODEPAGE = ("ascii", "cp437", "cp850", "cp1252")

    def _ascii(self, etichetta, testo):
        for enc in self.CODEPAGE:
            try:
                testo.encode(enc)
            except UnicodeEncodeError as e:
                self.fail("{} non e' stampabile in {}: {!r}".format(
                    etichetta, enc, testo[max(0, e.start - 20):e.end + 20]))

    def test_i_tre_livelli_sono_stampabili(self):
        for livello in ("compact", "normal", "verbose"):
            self._ascii("render/" + livello, render(snap(), livello))

    def test_lo_sono_anche_i_banner_di_blocco_e_azione(self):
        for motivo in ("DEPENDENCY", "RESOURCE_WRITER", "RESOURCE_UNREAL",
                       "WORKTREE_MISMATCH", "PROTOCOL_MISMATCH"):
            self._ascii("blocked/" + motivo, render(snap(blocking=motivo), "normal"))
        for azione in ("IMPLEMENT_ISSUE", "WAIT_REVIEW", "REVIEW_CANDIDATE",
                       "VALIDATE_CANDIDATE", "OPEN_ADDITIONAL_DEV"):
            self._ascii("action/" + azione,
                        render(snap(required_action=azione), "normal"))

    def test_lo_e_anche_il_diff(self):
        cambi = diff(snap(inbox_pending=0), snap(inbox_pending=1))
        self._ascii("render_diff", render_diff(cambi, snap(inbox_pending=1)))


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
