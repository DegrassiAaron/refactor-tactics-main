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
| **PIE-CP1.4** | Un id col PUNTO | nessuna | ✅ eseguita |
| **PIE-NOTA-PRIMA** `RELEASE-V01` | La cella si APRE con una nota | nessuna | ⛔ **Non vale per il packaged.** ✅ eseguita il 2026-09-30 |

> `PIE-V01-ROSTER` citata in prosa: non e' una riga di tabella e non cambia lo stato.
"""


class ParseTest(unittest.TestCase):
    def test_legge_una_voce_per_riga_di_tabella(self):
        st = pie_status.parse(REGISTRO)
        self.assertEqual(
            sorted(st),
            [
                "PIE-AS4a",
                "PIE-CP1.4",
                "PIE-HEXPLAY-6",
                "PIE-HEXPLAY-6c",
                "PIE-NOTA-PRIMA",
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
        # 🔴 Questa riga asseriva `⛔`, cioe' PINNAVA il difetto di #3362. Il comando
        # canonico riconosce QUATTRO marcatori — ✅ 🟡 ❌ ⏳ — e `⛔` non e' fra
        # essi: e' una NOTA, non uno stato. Una cella che porta solo una nota, per il canonico, e'
        # senza marcatore.
        self.assertEqual(st["PIE-VIS-DEFLECT"]["stato"], pie_status.SCONOSCIUTO)

    def test_il_subset_di_release_si_legge_dalla_prima_cella(self):
        st = pie_status.parse(REGISTRO)
        self.assertTrue(st["PIE-V01-ROSTER"]["release"])
        self.assertTrue(st["PIE-HEXPLAY-6"]["release"])
        self.assertFalse(st["PIE-TD-CLEAN"]["release"])

    def test_la_prosa_non_produce_voci(self):
        st = pie_status.parse("Una riga che nomina `PIE-FANTASMA` senza essere una tabella.")
        self.assertEqual(st, {})

    def test_un_id_col_punto_viene_letto(self):
        # `PIE-CP1.4` esiste nel registro e il regex la saltava: non ammetteva il punto, quindi il
        # `**` di chiusura non arrivava dove lo aspettava e la riga spariva senza rumore.
        st = pie_status.parse(REGISTRO)
        self.assertIn("PIE-CP1.4", st)
        self.assertEqual(st["PIE-CP1.4"]["stato"], "✅")

    def test_una_nota_in_testa_non_diventa_lo_stato(self):
        # 🔴 E' la forma PEGGIORE del difetto: non una voce persa — quella si nota contando —
        # ma uno stato FALSO su una voce che esiste. La cella si apre con `⛔` e il suo stato vero
        # viene dopo.
        st = pie_status.parse(REGISTRO)
        self.assertEqual(st["PIE-NOTA-PRIMA"]["stato"], "✅")
        self.assertTrue(st["PIE-NOTA-PRIMA"]["release"])

    def test_il_modulo_concorda_col_canonico_sul_registro_vero(self):
        # L'oracolo di #3362: il modulo e l'`awk` canonico devono dare la stessa ripartizione sul
        # registro REALE, non su una fixture. Qui il canonico e' riscritto in Python con la stessa
        # regola — `$(NF-1)` e il primo dei quattro marcatori — e il confronto e' voce per voce.
        import re
        CANON = re.compile('✅|🟡|❌|⏳')
        # ⚠️ `pie_status.REGISTRO` e' RELATIVO e risolve solo dalla radice del repository.
        # Qui si risolve dal file del test, cosi' l'oracolo vale anche se qualcuno lo lancia da
        # dentro `tools/editor-sessions/` — che e' la prima cosa che si prova.
        import pathlib
        radice = pathlib.Path(__file__).resolve().parents[2]
        testo = (radice / pie_status.REGISTRO).read_text(encoding='utf-8')
        atteso = {}
        for riga in testo.splitlines():
            if not riga.startswith('| **PIE-'):
                continue
            campi = riga.split('|')
            m = CANON.search(campi[-2] if len(campi) > 2 else '')
            atteso[riga.split('**')[1]] = m.group(0) if m else pie_status.SCONOSCIUTO
        letto = {k: d['stato'] for k, d in pie_status.load().items()}
        self.assertEqual(sorted(letto), sorted(atteso), 'le voci lette non sono le stesse')
        self.assertEqual(letto, atteso, 'uno stato diverge dal canonico')

    def test_verde_e_il_marcatore_che_il_calcolo_scarta(self):
        self.assertEqual(pie_status.VERDE, "✅")


if __name__ == "__main__":
    unittest.main()
