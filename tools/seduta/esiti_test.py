#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Test di `esiti.py`.

    python -m pytest -q tools/seduta

I test che contano sono quelli di MUTAZIONE: non basta che lo script dica `PASS` su un log pulito,
deve dire `FAIL` quando il difetto c'e'. Un estrattore che non sa fallire e' un gate cieco, ed e'
esattamente la classe di difetto per cui questo script esiste.
"""

import io
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import esiti  # noqa: E402

PASS, FAIL, NOTRUN, ROTTO = esiti.PASS, esiti.FAIL, esiti.NOTRUN, esiti.ROTTO

# Il separatore di riga come costante: una sequenza di escape in un heredoc si perde.
NL = chr(10)

LOG_PULITO = "\n".join([
    "[2026.10.03-12.00.00:000][  0]LogInit: Display: Engine is initialized.",
    "[2026.10.03-12.00.01:000][  1]LogRT: [RT] Board 2v2 esagonale avviata su 64 celle con 4 eroi",
    "[2026.10.03-12.05.00:000][900]LogRT: [RT] Fine partita al round 12: Team1Wins",
    "[2026.10.03-12.05.00:001][900]LogRT: [RT] Partita finita: Vince il team 1 (rosso)",
])


def scrivi(tmp, nome, testo):
    p = os.path.join(tmp, nome)
    io.open(p, "w", encoding="utf-8", newline="\n").write(testo)
    return p


class TestCancello(unittest.TestCase):
    """Il cancello e' la ragione d'essere dello script: senza, sei zeri sono un'assenza."""

    def setUp(self):
        import tempfile
        self.tmp = tempfile.mkdtemp()

    def test_log_pulito_apre_il_cancello(self):
        righe, valido = esiti.esiti_log(scrivi(self.tmp, "ok.log", LOG_PULITO))
        self.assertTrue(valido)
        d = {r[0]: r[1] for r in righe}
        self.assertEqual(d["CANCELLO: la partita e' avvenuta"], PASS)
        self.assertEqual(d["nessun crash"], PASS)

    def test_MUTAZIONE_senza_partita_il_cancello_chiude_benche_gli_zeri_siano_perfetti(self):
        """Il caso che il gate tutto-negativo non distingue: niente e' mai successo."""
        vuoto = "[2026.10.03-12.00.00:000][  0]LogInit: Display: Engine is initialized."
        righe, valido = esiti.esiti_log(scrivi(self.tmp, "mai.log", vuoto))
        d = {r[0]: r[1] for r in righe}
        # tutti gli esiti negativi sono PASS...
        self.assertEqual(d["nessun crash"], PASS)
        self.assertEqual(d["nessun travel failure"], PASS)
        # ...e tuttavia la misura NON e' valida, che e' il punto
        self.assertEqual(d["CANCELLO: la partita e' avvenuta"], FAIL)
        self.assertFalse(valido)

    def test_MUTAZIONE_file_che_non_e_un_log_non_da_PASS_ma_PATTERN_ROTTO(self):
        """Uno zero su un file che non sa rispondere non e' evidenza."""
        righe, valido = esiti.esiti_log(scrivi(self.tmp, "altro.txt", "ciao\nmondo\n"))
        self.assertEqual(righe[0][1], ROTTO)
        self.assertFalse(valido)

    def test_file_assente_e_NOT_RUN_non_PASS(self):
        righe, valido = esiti.esiti_log(os.path.join(self.tmp, "non-esiste.log"))
        self.assertEqual(righe[0][1], NOTRUN)
        self.assertFalse(valido)


class TestRetiSuiCrash(unittest.TestCase):
    """Tre reti, e lo script deve dire QUALE ha colpito."""

    def setUp(self):
        import tempfile
        self.tmp = tempfile.mkdtemp()

    def test_MUTAZIONE_la_rete_storica_prende_un_assert(self):
        righe, _ = esiti.esiti_log(scrivi(self.tmp, "a.log",
                                          LOG_PULITO + "\nLogWindows: Error: Assertion failed: Foo\n"))
        d = {r[0]: (r[1], r[2]) for r in righe}
        self.assertEqual(d["nessun crash"][0], FAIL)
        self.assertIn("Assertion failed=1", d["nessun crash"][1])

    def test_MUTAZIONE_il_GPU_crash_SFUGGE_alla_rete_storica_e_lo_prendono_le_altre_due(self):
        """E' il caso misurato dalla cella G2: un solo pattern non copre la classe."""
        testo = LOG_PULITO + "\n".join([
            "",
            "LogRHI: Error: GPU Crash dump Triggered",
            "LogD3D12RHI: Error: DXGI_ERROR_DEVICE_REMOVED",
            "LogWindows: FPlatformMisc::RequestExitWithStatus(1, 3, D3D12Util.TerminateOnGPUCrash)",
        ])
        d = {r[0]: (r[1], r[2]) for r in righe_di(testo, self.tmp)}
        self.assertEqual(d["nessun crash"][0], PASS, "la rete storica NON vede il GPU crash")
        self.assertEqual(d["nessun GPU crash"][0], FAIL)
        self.assertEqual(d["nessun crash (rete larga)"][0], FAIL)


def righe_di(testo, tmp):
    return esiti.esiti_log(scrivi(tmp, "g.log", testo))[0]


class TestPavimentoEditor(unittest.TestCase):
    """Su un log di Editor la rete larga parte da 2: l'Engine esercita il proprio sistema di errori."""

    def setUp(self):
        import tempfile
        self.tmp = tempfile.mkdtemp()

    def test_due_Error_su_un_log_di_EDITOR_non_sono_un_FAIL(self):
        # Misurato il 2026-10-04 su due log di Editor indipendenti: 2 e 2, a run pulita.
        testo = LOG_PULITO + NL.join([
            "",
            "LogTemp: Error with param: UE::UnifiedErrorTest::WithInt: [Error with int -7]",
            "LogAutomationTest: Error: Condition failed",
            "LogAutomationTest: Error: Condition failed",
        ])
        d = {r[0]: (r[1], r[2]) for r in righe_di(testo, self.tmp)}
        self.assertEqual(d["tipo di log"][0], PASS)
        self.assertIn("EDITOR", d["tipo di log"][1])
        self.assertEqual(d["nessun crash (rete larga)"][0], PASS)

    def test_MUTAZIONE_il_TERZO_Error_su_un_log_di_Editor_FA_FAIL(self):
        """Lo sconto e' di DUE: il terzo e' tuo, e il gate deve vederlo."""
        testo = LOG_PULITO + NL.join([
            "",
            "LogTemp: UE::UnifiedErrorTest::Empty",
            "LogAutomationTest: Error: Condition failed",
            "LogAutomationTest: Error: Condition failed",
            "LogRT: Error: questo e' mio e deve essere visto",
        ])
        d = {r[0]: (r[1], r[2]) for r in righe_di(testo, self.tmp)}
        self.assertEqual(d["nessun crash (rete larga)"][0], FAIL)

    def test_MUTAZIONE_su_un_log_di_PACCHETTO_non_si_sconta_niente(self):
        """Senza `UnifiedErrorTest` il log non e' di un Editor: un solo Error: e' un FAIL."""
        testo = LOG_PULITO + NL + "LogRT: Error: uno solo, e sul pacchetto vale" + NL
        righe = righe_di(testo, self.tmp)
        d = {r[0]: (r[1], r[2]) for r in righe}
        self.assertNotIn("tipo di log", d)
        self.assertEqual(d["nessun crash (rete larga)"][0], FAIL)


class TestCsv(unittest.TestCase):
    """Le colonne si cercano per NOME: un indice cablato misura un'altra serie."""

    def setUp(self):
        import tempfile
        self.tmp = tempfile.mkdtemp()
        self.head = esiti_header()

    def test_tre_congiunti_soddisfatti(self):
        righe = esiti.esiti_csv(scrivi(self.tmp, "p.csv", self.head + "\n" + riga_csv(1, 1, 1)))
        d = {r[0]: r[1] for r in righe}
        self.assertEqual(d["input funzionante (>=1 su un turno)"], PASS)

    def test_MUTAZIONE_UndoCount_a_zero_fa_FAIL(self):
        """E' il congiunto legato al tasto DESTRO: senza, il criterio manca il suo difetto."""
        righe = esiti.esiti_csv(scrivi(self.tmp, "p.csv", self.head + "\n" + riga_csv(3, 5, 0)))
        d = {r[0]: r[1] for r in righe}
        self.assertEqual(d["input funzionante (>=1 su un turno)"], FAIL)

    def test_MUTAZIONE_colonna_rinominata_da_PATTERN_ROTTO_non_PASS(self):
        rotta = self.head.replace("UndoCount", "UndoCountX")
        righe = esiti.esiti_csv(scrivi(self.tmp, "p.csv", rotta + "\n" + riga_csv(1, 1, 1)))
        self.assertEqual(righe[0][1], ROTTO)

    def test_csv_non_fornito_e_NOT_RUN(self):
        self.assertEqual(esiti.esiti_csv(None)[0][1], NOTRUN)


def esiti_header():
    """L'intestazione reale di URTPacingLibrary::CsvHeader(), venti colonne."""
    return ("Turn,AliveT0,AliveT1,ActionsAvailable,MsToFirstInput,SelectionCount,OrderCount,"
            "UndoCount,MsToLockIn,MsSinceLastInput,LockInSource,MsPlayback,PlaybackSkipped,"
            "ReactionWindows,ReactionOpportunities,CandidateEvents,CandidatesTotal,"
            "CollectionCpuUsTotal,BoundaryEvents,BoundaryCpuUsTotal")


def riga_csv(sel, order, undo):
    v = ["1", "2", "2", "4", "500", str(sel), str(order), str(undo)] + ["0"] * 12
    return ",".join(v)


class TestColonnePerNomeNonPerIndice(unittest.TestCase):
    def test_le_tre_colonne_sono_la_sesta_settima_e_ottava(self):
        """Il criterio di G13 nomina le colonne 6, 7 e 8: se l'ordine cambia, il gate misura altro."""
        cols = esiti_header().split(",")
        self.assertEqual(cols[5], "SelectionCount")
        self.assertEqual(cols[6], "OrderCount")
        self.assertEqual(cols[7], "UndoCount")
        self.assertEqual(len(cols), 20)


class TestCartelleDiCrash(unittest.TestCase):
    def setUp(self):
        import tempfile
        self.tmp = tempfile.mkdtemp()

    def test_cartella_assente_e_PASS(self):
        r = esiti.esiti_crashes(os.path.join(self.tmp, "niente"))
        self.assertEqual(r[0][1], PASS)

    def test_MUTAZIONE_una_cartella_dentro_fa_FAIL(self):
        d = os.path.join(self.tmp, "Crashes")
        os.makedirs(os.path.join(d, "UECC-Windows-ABC_0000"))
        r = esiti.esiti_crashes(d)
        self.assertEqual(r[0][1], FAIL)

    def test_non_fornita_e_NOT_RUN(self):
        self.assertEqual(esiti.esiti_crashes(None)[0][1], NOTRUN)


if __name__ == "__main__":
    unittest.main()
