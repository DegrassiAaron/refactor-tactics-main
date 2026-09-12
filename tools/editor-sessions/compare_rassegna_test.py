"""Il gate del verbale: ogni bloccante ha la sua prova scritta."""
from __future__ import annotations

import unittest

import compare_rassegna
from registry import Wire

VERBALE = """# Rassegna dei requires

| check | allestimento | decisione | prova |
|---|---|---|---|
| `PIE-A` | `SET-A` | `asset:WBP_RT_PauseMenu` | non tracciato |
| `PIE-B` | `SET-A` | — | eseguibile oggi |
| `PIE-C` | `SET-A` | `asset:X`, `mount:X#7` | non tracciato, e il feed e' di #7 |
"""

STATO = {
    "PIE-A": {"stato": "⏳", "release": False},
    "PIE-B": {"stato": "🟡", "release": False},
    "PIE-C": {"stato": "❌", "release": True},
    "PIE-D": {"stato": "⏳", "release": False},
    "PIE-E": {"stato": "✅", "release": False},
}


class LeggiVerbaleTest(unittest.TestCase):
    def test_raccoglie_la_decisione_per_check(self):
        letto = compare_rassegna.leggi_verbale(VERBALE)
        self.assertEqual(letto["PIE-A"], "`asset:WBP_RT_PauseMenu`")
        self.assertEqual(letto["PIE-B"], "—")
        self.assertIn("mount:X#7", letto["PIE-C"])

    def test_ignora_la_riga_di_separazione_e_l_intestazione(self):
        self.assertEqual(set(compare_rassegna.leggi_verbale(VERBALE)), {"PIE-A", "PIE-B", "PIE-C"})


class BacinoTest(unittest.TestCase):
    def test_sono_i_cablati_non_verdi(self):
        wires = [
            Wire(check="PIE-A", setup="SET-A"),
            Wire(check="PIE-E", setup="SET-A"),  # verde: fuori
        ]
        self.assertEqual(compare_rassegna.bacino(wires, STATO), ["PIE-A"])


class ConfrontaTest(unittest.TestCase):
    def test_un_requires_senza_riga_nel_verbale_e_un_difetto(self):
        wires = [Wire(check="PIE-D", setup="SET-A", requires=("asset:Y",))]
        r = compare_rassegna.confronta(VERBALE, wires, STATO)
        self.assertEqual(r["senza_prova"], ["PIE-D"])

    def test_un_requires_che_il_verbale_non_nomina_nella_decisione_e_un_difetto(self):
        """Il verbale dice «—» e lo yaml blocca: le due fonti si contraddicono."""
        wires = [Wire(check="PIE-B", setup="SET-A", requires=("asset:Y",))]
        r = compare_rassegna.confronta(VERBALE, wires, STATO)
        self.assertEqual(r["senza_prova"], ["PIE-B"])

    def test_un_requires_riportato_nel_verbale_passa(self):
        wires = [Wire(check="PIE-A", setup="SET-A", requires=("asset:WBP_RT_PauseMenu",))]
        r = compare_rassegna.confronta(VERBALE, wires, STATO)
        self.assertEqual(r["senza_prova"], [])

    def test_un_check_comparso_dopo_la_rassegna_si_stampa_e_non_ferma(self):
        wires = [
            Wire(check="PIE-A", setup="SET-A", requires=("asset:WBP_RT_PauseMenu",)),
            Wire(check="PIE-D", setup="SET-A"),
        ]
        r = compare_rassegna.confronta(VERBALE, wires, STATO)
        self.assertEqual(r["non_esaminati"], ["PIE-D"])
        self.assertEqual(r["senza_prova"], [])

    def test_l_ordine_e_totale(self):
        wires = [
            Wire(check="PIE-D", setup="SET-A"),
            Wire(check="PIE-A", setup="SET-A", requires=("asset:WBP_RT_PauseMenu",)),
        ]
        r = compare_rassegna.confronta(VERBALE, wires, STATO)
        self.assertEqual(r["non_esaminati"], sorted(r["non_esaminati"]))


if __name__ == "__main__":
    unittest.main()
