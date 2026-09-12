# tools/editor-sessions/compare_legacy_test.py
"""Il confronto dice cosa il calcolo perde rispetto alle sedute scritte a mano."""
from __future__ import annotations

import unittest

import compare_legacy
from agenda import Gruppo, Voce

STATO = {
    "PIE-A": {"stato": "⏳", "release": False},
    "PIE-B": {"stato": "⏳", "release": False},
    "PIE-C": {"stato": "⏳", "release": False},
    "PIE-VERDE": {"stato": "✅", "release": False},
}

GRUPPI = [Gruppo(setup="SET-X", liberi=[Voce("PIE-A", (), False)], bloccati=[Voce("PIE-B", ("x",), False)])]

SESSIONI = [
    {"id": "U1", "verifies": ["PIE-A", "PIE-C", "PIE-VERDE"]},
    {"id": "U2", "verifies": ["PIE-B"]},
]


class ConfrontoTest(unittest.TestCase):
    def setUp(self):
        self.r = compare_legacy.confronta(GRUPPI, ["PIE-SCOP"], SESSIONI, STATO)

    def test_un_check_convocato_solo_dal_vecchio_e_perso(self):
        self.assertEqual(self.r["persi"], ["PIE-C"])

    def test_un_bloccato_stampato_non_conta_come_perso(self):
        self.assertNotIn("PIE-B", self.r["persi"])

    def test_un_verde_non_conta_come_perso(self):
        self.assertNotIn("PIE-VERDE", self.r["persi"])

    def test_la_coda_scoperta_non_e_un_guadagno(self):
        self.assertEqual(self.r["guadagnati"], [])

    def test_riporta_quante_aperture_contro_quante_sedute(self):
        self.assertEqual(self.r["aperture"], 1)
        self.assertEqual(self.r["sedute"], 2)


if __name__ == "__main__":
    unittest.main()
