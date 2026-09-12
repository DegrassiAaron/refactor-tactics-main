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

    def test_riconosce_l_arena_generata(self):
        self.assertEqual(
            seed_wiring.deduci_setup("il terreno lo fornisce `MapSource = GeneratedTestArena`"),
            "SET-GEN-ARENA",
        )

    def test_l_arena_generata_vince_sull_arena_versionata(self):
        self.assertEqual(
            seed_wiring.deduci_setup("GeneratedTestArena, e non L_HexArena"), "SET-GEN-ARENA"
        )

    def test_uno_scenario_esplicito_vince_sull_arena_generata(self):
        self.assertEqual(
            seed_wiring.deduci_setup("rt.Test.Scenario Visual.X su GeneratedTestArena"), "SET-SCEN"
        )

    def test_senza_indizi_non_inventa(self):
        self.assertIsNone(seed_wiring.deduci_setup("una seduta descritta senza nominare mappe"))


class SeminaTest(unittest.TestCase):
    def test_una_riga_per_check_col_setup_dedotto(self):
        sessioni = [
            {"id": "U49", "verifies": ["PIE-V01-SCREENHUD"], "issues": [613],
             "steps": "aprire L_Frontend e premere PLAY"},
        ]
        righe, orfani, _ = seed_wiring.semina(sessioni)
        self.assertEqual(orfani, {})
        self.assertEqual(
            righe,
            [{"check": "PIE-V01-SCREENHUD", "setup": "SET-FRONTEND", "issue": 613}],
        )

    def test_due_sedute_che_concordano_producono_una_riga_sola(self):
        sessioni = [
            {"id": "U901", "verifies": ["PIE-X"], "steps": "rt.Test.Scenario A"},
            {"id": "U902", "verifies": ["PIE-X"], "steps": "rt.Test.Scenario B"},
        ]
        righe, _, conflitti = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-X"])
        self.assertEqual(conflitti, {})

    def test_due_sedute_in_disaccordo_sono_un_conflitto_non_una_scelta(self):
        sessioni = [
            {"id": "U901", "verifies": ["PIE-X"], "steps": "GeneratedTestArena"},
            {"id": "U902", "verifies": ["PIE-X"], "steps": "rt.Test.Scenario B"},
        ]
        righe, _, conflitti = seed_wiring.semina(sessioni)
        self.assertEqual(righe, [])
        self.assertEqual(
            conflitti, {"PIE-X": {"U901": "SET-GEN-ARENA", "U902": "SET-SCEN"}}
        )

    def test_un_conflitto_risolto_a_mano_produce_la_riga_e_sparisce(self):
        sessioni = [
            {"id": "U901", "verifies": ["PIE-HEXPLAY-6"], "steps": "GeneratedTestArena"},
            {"id": "U902", "verifies": ["PIE-HEXPLAY-6"], "steps": "rt.Test.Scenario B"},
        ]
        righe, _, conflitti = seed_wiring.semina(sessioni)
        self.assertEqual(conflitti, {})
        self.assertEqual(righe[0]["setup"], "SET-SCEN")

    def test_cio_che_non_sa_dedurre_resta_raggruppato_per_seduta(self):
        # ID inventati di proposito: un ID reale finirebbe in ALLESTIMENTO_DICHIARATO e il
        # test misurerebbe i dati invece del comportamento.
        sessioni = [
            {"id": "U997", "verifies": ["PIE-MISTERO", "PIE-ALTRO"], "steps": "nessun indizio"},
            {"id": "U998", "verifies": ["PIE-TERZO"], "steps": "nemmeno qui"},
        ]
        righe, orfani, _ = seed_wiring.semina(sessioni)
        self.assertEqual(righe, [])
        self.assertEqual(orfani, {"U997": ["PIE-ALTRO", "PIE-MISTERO"], "U998": ["PIE-TERZO"]})

    def test_una_seduta_muta_non_produce_orfani_se_un_altra_cabla_lo_stesso_check(self):
        sessioni = [
            {"id": "U1", "verifies": ["PIE-X"], "steps": "L_HexArena"},
            {"id": "U17", "verifies": ["PIE-X"], "steps": "nessun indizio"},
        ]
        righe, orfani, _ = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-X"])
        self.assertEqual(orfani, {})

    def test_un_allestimento_dichiarato_copre_una_seduta_muta(self):
        sessioni = [{"id": "U14", "verifies": ["PIE-X"], "steps": "nessun indizio"}]
        righe, orfani, _ = seed_wiring.semina(sessioni)
        self.assertEqual(orfani, {})
        self.assertEqual(righe, [{"check": "PIE-X", "setup": "SET-HEX-MATCH"}])

    def test_il_dichiarato_vince_sull_euristica(self):
        sessioni = [{"id": "U4", "verifies": ["PIE-X"], "steps": "una partita su L_HexArena"}]
        righe, _, _ = seed_wiring.semina(sessioni)
        self.assertEqual(righe[0]["setup"], "SET-GEN-ARENA")

    def test_una_seduta_non_dichiarata_resta_orfana(self):
        sessioni = [{"id": "U999", "verifies": ["PIE-X"], "steps": "nessun indizio"}]
        _, orfani, _ = seed_wiring.semina(sessioni)
        self.assertEqual(orfani, {"U999": ["PIE-X"]})

    def test_l_uscita_e_ordinata_per_check(self):
        sessioni = [{"id": "U1", "verifies": ["PIE-Z", "PIE-A"], "steps": "L_HexArena"}]
        righe, _, _ = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-A", "PIE-Z"])


if __name__ == "__main__":
    unittest.main()
