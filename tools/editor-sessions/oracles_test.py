# tools/editor-sessions/oracles_test.py
"""Un requires e' soddisfatto o no, e l'oracolo dice sempre chi lo ha stabilito."""
from __future__ import annotations

import unittest

import oracles

INVENTARIO = {"BP_Unit_Aevik", "L_HexArena", "WBP_RT_TacticalHUD"}
CHIUSE = {2723}
NOTE = {2697, 2723, 2454}


class AssetTest(unittest.TestCase):
    def test_asset_presente_e_soddisfatto(self):
        e = oracles.valuta("asset:BP_Unit_Aevik", INVENTARIO, CHIUSE, NOTE)
        self.assertTrue(e.soddisfatto)
        self.assertIn("BP_Unit_Aevik", e.perche)

    def test_asset_assente_blocca_e_dice_dove_manca(self):
        e = oracles.valuta("asset:WBP_RT_PauseMenu", INVENTARIO, CHIUSE, NOTE)
        self.assertFalse(e.soddisfatto)
        self.assertIn("WBP_RT_PauseMenu", e.perche)
        self.assertIn("Content/", e.perche)


class IssueTest(unittest.TestCase):
    def test_issue_aperta_blocca_e_nomina_la_issue(self):
        e = oracles.valuta("mount:WBP_RT_EventLogRight#2697", INVENTARIO, CHIUSE, NOTE)
        self.assertFalse(e.soddisfatto)
        self.assertIn("#2697", e.perche)

    def test_issue_chiusa_sblocca(self):
        e = oracles.valuta("feature:ReactionWindow#2723", INVENTARIO, CHIUSE, NOTE)
        self.assertTrue(e.soddisfatto)
        self.assertIn("#2723", e.perche)

    def test_issue_fuori_cache_viene_rifiutata_invece_di_bloccare(self):
        """Un bloccante per colpa dell'attrezzatura e' il gemello del verde falso.

        Se la cache non conosce il numero, l'oracolo non sa rispondere: fingere
        «bloccato» nasconde il check con una ragione che non e' la sua, e
        nessuno rilancia il fetch perche' l'agenda sembra sana.
        """
        with self.assertRaises(oracles.OracoloError) as e:
            oracles.valuta("cue:Deflect#9999", INVENTARIO, CHIUSE, NOTE)
        self.assertIn("#9999", str(e.exception))
        self.assertIn("fetch_github_cache.py", str(e.exception))
        self.assertIn("--also", str(e.exception))


class FormaTest(unittest.TestCase):
    def test_tipo_sconosciuto_viene_rifiutato(self):
        with self.assertRaises(ValueError) as e:
            oracles.valuta("colore:rosso", INVENTARIO, CHIUSE, NOTE)
        self.assertIn("colore", str(e.exception))

    def test_tipo_con_issue_senza_issue_viene_rifiutato(self):
        with self.assertRaises(ValueError) as e:
            oracles.valuta("mount:WBP_RT_EventLogRight", INVENTARIO, CHIUSE, NOTE)
        self.assertIn("#", str(e.exception))


class CacheTest(unittest.TestCase):
    def test_issue_chiuse_legge_la_forma_della_cache_del_decision_log(self):
        cache = {
            "repo": "DegrassiAaron/refactor-tactics-main",
            "items": {
                "2723": {"state": "closed", "title": "view model"},
                "2697": {"state": "open", "title": "il feed non e' montato"},
            },
        }
        self.assertEqual(oracles.issue_chiuse(cache), {2723})

    def test_una_issue_chiusa_soddisfa(self):
        cache = {"items": {"938": {"state": "closed", "type": "issue"}}}
        self.assertEqual(oracles.issue_chiuse(cache), {938})

    def test_una_pr_chiusa_e_mergiata_soddisfa(self):
        cache = {"items": {"2834": {"state": "closed", "type": "pr", "merged": True}}}
        self.assertEqual(oracles.issue_chiuse(cache), {2834})

    def test_una_pr_chiusa_e_non_mergiata_non_soddisfa(self):
        """Lavoro abbandonato: chiusa senza merge non e' fatta, non basta lo `state`."""
        cache = {"items": {"351": {"state": "closed", "type": "pr", "merged": False}}}
        self.assertEqual(oracles.issue_chiuse(cache), set())

    def test_una_pr_aperta_non_soddisfa_anche_se_merged_manca(self):
        cache = {"items": {"999": {"state": "open", "type": "pr"}}}
        self.assertEqual(oracles.issue_chiuse(cache), set())


if __name__ == "__main__":
    unittest.main()
