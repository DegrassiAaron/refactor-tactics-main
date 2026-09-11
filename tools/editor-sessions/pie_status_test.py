# tools/editor-sessions/pie_status_test.py
"""Lo stato delle voci PIE si legge dalla tabella del registro, non si deduce."""
from __future__ import annotations

import unittest

import pie_status

REGISTRO = """
Testo di prosa che non e' una riga di tabella e va ignorato.

| **PIE-V01-ROSTER** `RELEASE-V01` | Roster dei 4 eroi | nessun asset | ✅ **Giocata il 2026-09-06** |
| **PIE-HEXPLAY-6** `RELEASE-V01` | Muro attraversabile | scenario | ❌ da rifare in pianificazione |
| **PIE-TD-CLEAN** | Il pannello non sporca | nessuna | ⏳ mai eseguita |
| **PIE-VIS-DEFLECT** | La parata si vede | cue assenti | ⛔ non eseguibile prima delle cue |
| **PIE-AS4a** | Suffisso MINUSCOLO | il registro avverte che un regex `[A-Z0-9]` la tronca | 🟡 mezza |
| **PIE-HEXPLAY-6c** `RELEASE-V01` | Altro suffisso minuscolo | — | ⏳ mai eseguita |

> `PIE-V01-ROSTER` citata in prosa: non e' una riga di tabella e non cambia lo stato.
"""


class ParseTest(unittest.TestCase):
    def test_legge_una_voce_per_riga_di_tabella(self):
        st = pie_status.parse(REGISTRO)
        self.assertEqual(
            sorted(st),
            [
                "PIE-AS4a",
                "PIE-HEXPLAY-6",
                "PIE-HEXPLAY-6c",
                "PIE-TD-CLEAN",
                "PIE-V01-ROSTER",
                "PIE-VIS-DEFLECT",
            ],
        )

    def test_un_suffisso_minuscolo_non_fa_saltare_la_riga(self):
        # Il registro avverte da se' che un regex `[A-Z0-9]` tronca o accorpa nove righe
        # reali. Questo test e' li' perche' il difetto e' gia' costato una volta.
        st = pie_status.parse(REGISTRO)
        self.assertEqual(st["PIE-AS4a"]["stato"], "🟡")
        self.assertEqual(st["PIE-HEXPLAY-6c"]["stato"], "⏳")
        self.assertTrue(st["PIE-HEXPLAY-6c"]["release"])
        self.assertNotIn("PIE-AS4", st)

    def test_lo_stato_e_la_prima_emoji_dell_ultima_cella(self):
        st = pie_status.parse(REGISTRO)
        self.assertEqual(st["PIE-V01-ROSTER"]["stato"], "✅")
        self.assertEqual(st["PIE-HEXPLAY-6"]["stato"], "❌")
        self.assertEqual(st["PIE-TD-CLEAN"]["stato"], "⏳")
        self.assertEqual(st["PIE-VIS-DEFLECT"]["stato"], "⛔")

    def test_il_subset_di_release_si_legge_dalla_prima_cella(self):
        st = pie_status.parse(REGISTRO)
        self.assertTrue(st["PIE-V01-ROSTER"]["release"])
        self.assertTrue(st["PIE-HEXPLAY-6"]["release"])
        self.assertFalse(st["PIE-TD-CLEAN"]["release"])

    def test_la_prosa_non_produce_voci(self):
        st = pie_status.parse("Una riga che nomina `PIE-FANTASMA` senza essere una tabella.")
        self.assertEqual(st, {})

    def test_verde_e_il_marcatore_che_il_calcolo_scarta(self):
        self.assertEqual(pie_status.VERDE, "✅")


if __name__ == "__main__":
    unittest.main()
