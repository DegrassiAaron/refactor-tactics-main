"""La cache raccoglie i `#nnn` anche fuori dal Decision Log.

Serve a chi dichiara un prerequisito con un issue owner: quel numero DEVE
entrare nella cache, altrimenti l'oracolo che lo legge si rifiuta di
rispondere (`oracles.OracoloError`) e l'ordine del giorno non si calcola.
"""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import fetch_github_cache


class ReferencedExtraTest(unittest.TestCase):
    def scrivi(self, testo: str) -> Path:
        d = tempfile.mkdtemp()
        p = Path(d) / "mattoni.yaml"
        p.write_text(testo, encoding="utf-8")
        return p

    def test_raccoglie_i_numeri_dai_requires(self):
        p = self.scrivi(
            "wiring:\n"
            "  - check: PIE-VIS-SIGHTWALL\n"
            "    requires: [ asset:WBP_RT_EventLogRight, mount:WBP_RT_EventLogRight#2697 ]\n"
            "  - check: PIE-VIS-DEFLECT\n"
            "    requires: [ cue:Deflect#2454 ]\n"
        )
        self.assertEqual(fetch_github_cache.referenced_extra(p), [2454, 2697])

    def test_ordina_e_deduplica(self):
        p = self.scrivi("a #2697\nb #2454\nc #2697\n")
        self.assertEqual(fetch_github_cache.referenced_extra(p), [2454, 2697])

    def test_un_file_senza_riferimenti_non_e_un_errore(self):
        p = self.scrivi("wiring:\n  - check: PIE-X\n    setup: SET-A\n")
        self.assertEqual(fetch_github_cache.referenced_extra(p), [])

    def test_il_campo_issue_senza_cancelletto_non_viene_raccolto(self):
        """`issue: 613` e' un riferimento d'attribuzione, non un oracolo.

        Solo un `#nnn` dichiara che qualcuno deve chiudere qualcosa perche' un
        check torni guardabile. Raccogliere anche `issue:` gonfierebbe la cache
        con numeri che nessun oracolo interroga.
        """
        p = self.scrivi("wiring:\n  - check: PIE-X\n    issue: 613\n")
        self.assertEqual(fetch_github_cache.referenced_extra(p), [])


if __name__ == "__main__":
    unittest.main()
