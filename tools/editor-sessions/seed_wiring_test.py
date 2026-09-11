# tools/editor-sessions/seed_wiring_test.py
"""La semina deduce l'allestimento dalla prosa, e dichiara cio' che non sa dedurre."""
from __future__ import annotations

import unittest

import seed_wiring


class DeduzioneTest(unittest.TestCase):
    def test_riconosce_il_frontend(self):
        self.assertEqual(seed_wiring.deduci_setup("si parte da L_Frontend e si preme PLAY"), "SET-FRONTEND")

    def test_riconosce_lo_scenario(self):
        self.assertEqual(seed_wiring.deduci_setup("lancia rt.Test.Scenario Visual.Map.X"), "SET-SCEN")

    def test_riconosce_l_autobattle(self):
        self.assertEqual(
            seed_wiring.deduci_setup("partita con rt.Match.Autobattle=1 su L_HexArena"), "SET-HEX-BOT"
        )

    def test_il_piu_specifico_vince_sull_arena(self):
        self.assertEqual(
            seed_wiring.deduci_setup("apri L_GrayKitPlayground, poi torna su L_HexArena"), "SET-GRAYKIT"
        )

    def test_senza_indizi_non_inventa(self):
        self.assertIsNone(seed_wiring.deduci_setup("una seduta descritta senza nominare mappe"))


class SeminaTest(unittest.TestCase):
    def test_una_riga_per_check_col_setup_dedotto(self):
        sessioni = [
            {"id": "U49", "verifies": ["PIE-V01-SCREENHUD"], "issues": [613],
             "steps": "aprire L_Frontend e premere PLAY"},
        ]
        righe, orfani = seed_wiring.semina(sessioni)
        self.assertEqual(orfani, {})
        self.assertEqual(
            righe,
            [{"check": "PIE-V01-SCREENHUD", "setup": "SET-FRONTEND", "issue": 613}],
        )

    def test_un_check_cablato_due_volte_compare_una_volta_sola(self):
        sessioni = [
            {"id": "U4", "verifies": ["PIE-HEXPLAY-6"], "steps": "rt.Test.Scenario X"},
            {"id": "U46", "verifies": ["PIE-HEXPLAY-6"], "steps": "rt.Test.Scenario X"},
        ]
        righe, _ = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-HEXPLAY-6"])

    def test_cio_che_non_sa_dedurre_resta_raggruppato_per_seduta(self):
        sessioni = [
            {"id": "U17", "verifies": ["PIE-MISTERO", "PIE-ALTRO"], "steps": "nessun indizio"},
            {"id": "U53", "verifies": ["PIE-TERZO"], "steps": "nemmeno qui"},
        ]
        righe, orfani = seed_wiring.semina(sessioni)
        self.assertEqual(righe, [])
        self.assertEqual(orfani, {"U17": ["PIE-ALTRO", "PIE-MISTERO"], "U53": ["PIE-TERZO"]})

    def test_una_seduta_muta_non_produce_orfani_se_un_altra_cabla_lo_stesso_check(self):
        sessioni = [
            {"id": "U1", "verifies": ["PIE-X"], "steps": "L_HexArena"},
            {"id": "U17", "verifies": ["PIE-X"], "steps": "nessun indizio"},
        ]
        righe, orfani = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-X"])
        self.assertEqual(orfani, {})

    def test_l_uscita_e_ordinata_per_check(self):
        sessioni = [{"id": "U1", "verifies": ["PIE-Z", "PIE-A"], "steps": "L_HexArena"}]
        righe, _ = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-A", "PIE-Z"])


if __name__ == "__main__":
    unittest.main()
