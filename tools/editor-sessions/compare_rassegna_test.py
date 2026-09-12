# tools/editor-sessions/compare_rassegna_test.py
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

# Nomi reali del dominio (docs/roadmap/sedute-mattoni.yaml:25): un widget e la sua
# variante `Right` sono voci distinte, e la seconda e' PREFISSATA dalla prima. E' il
# caso che un confronto per sottostringa non distingue.
VERBALE_EVENTLOG = """# Rassegna dei requires

| check | allestimento | decisione | prova |
|---|---|---|---|
| `PIE-X` | `SET-A` | `asset:WBP_RT_EventLogRight`, `mount:WBP_RT_EventLogRight#2697` | non tracciato, e il feed e' di #2697 |
"""

# La decisione ripete il token, la prova e' vuota: il buco che il gate deve chiudere.
VERBALE_PROVA_VUOTA = """# Rassegna dei requires

| check | allestimento | decisione | prova |
|---|---|---|---|
| `PIE-Z` | `SET-A` | `asset:Y` | |
"""

# La decisione scrive il numero della issue FUORI dal token, in prosa naturale: il
# token estratto da `PREREQ` non porta `#2697`, quindi non combacia col `requires`
# scritto nello yaml. E' la severita' che il gate non deve allentare.
VERBALE_PROSA_NATURALE = """# Rassegna dei requires

| check | allestimento | decisione | prova |
|---|---|---|---|
| `PIE-W` | `SET-A` | `mount:WBP_RT_EventLogRight` (#2697) | montato, vedi #2697 |
"""


class LeggiVerbaleTest(unittest.TestCase):
    def test_raccoglie_la_decisione_per_check(self):
        letto = compare_rassegna.leggi_verbale(VERBALE)
        self.assertEqual(letto["PIE-A"], "`asset:WBP_RT_PauseMenu`")
        self.assertEqual(letto["PIE-B"], "—")
        self.assertIn("mount:X#7", letto["PIE-C"])

    def test_ignora_la_riga_di_separazione_e_l_intestazione(self):
        self.assertEqual(set(compare_rassegna.leggi_verbale(VERBALE)), {"PIE-A", "PIE-B", "PIE-C"})


class LeggiProveTest(unittest.TestCase):
    def test_raccoglie_la_prova_per_check(self):
        letto = compare_rassegna.leggi_prove(VERBALE)
        self.assertEqual(letto["PIE-A"], "non tracciato")
        self.assertEqual(letto["PIE-B"], "eseguibile oggi")

    def test_una_colonna_prova_vuota_legge_stringa_vuota(self):
        letto = compare_rassegna.leggi_prove(VERBALE_PROVA_VUOTA)
        self.assertEqual(letto["PIE-Z"], "")


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

    def test_un_requires_prefisso_di_un_altro_nome_non_e_giustificato(self):
        """`asset:WBP_RT_EventLog` e' prefisso di `asset:WBP_RT_EventLogRight`: un
        containment su stringa lo troverebbe comunque, ma non e' la prova che il
        verbale ha scritto per QUEL nome."""
        wires = [Wire(check="PIE-X", setup="SET-A", requires=("asset:WBP_RT_EventLog",))]
        r = compare_rassegna.confronta(VERBALE_EVENTLOG, wires, STATO)
        self.assertEqual(r["senza_prova"], ["PIE-X"])

    def test_un_requires_che_combacia_esattamente_e_giustificato(self):
        wires = [Wire(check="PIE-X", setup="SET-A", requires=("asset:WBP_RT_EventLogRight",))]
        r = compare_rassegna.confronta(VERBALE_EVENTLOG, wires, STATO)
        self.assertEqual(r["senza_prova"], [])

    def test_stessa_forma_ma_issue_diversa_non_e_giustificato(self):
        wires = [Wire(check="PIE-X", setup="SET-A", requires=("mount:WBP_RT_EventLogRight#2698",))]
        r = compare_rassegna.confronta(VERBALE_EVENTLOG, wires, STATO)
        self.assertEqual(r["senza_prova"], ["PIE-X"])

    def test_decisione_giusta_ma_prova_vuota_e_un_difetto(self):
        """Il difetto che il gate esisteva senza chiudere: la colonna 3 ripete il
        token, la colonna 4 (prova) e' vuota, e il vecchio gate non la leggeva."""
        wires = [Wire(check="PIE-Z", setup="SET-A", requires=("asset:Y",))]
        r = compare_rassegna.confronta(VERBALE_PROVA_VUOTA, wires, STATO)
        self.assertEqual(r["senza_prova"], ["PIE-Z"])

    def test_decisione_in_prosa_naturale_con_issue_fuori_dal_token_non_e_giustificato(self):
        """Non e' un allentamento accettabile: un lettore umano leggerebbe questa riga
        come corretta, ma il token estratto da PREREQ non porta `#2697`, e deve
        restare cosi' — fallire qui e' la direzione sicura."""
        wires = [Wire(check="PIE-W", setup="SET-A", requires=("mount:WBP_RT_EventLogRight#2697",))]
        r = compare_rassegna.confronta(VERBALE_PROSA_NATURALE, wires, STATO)
        self.assertEqual(r["senza_prova"], ["PIE-W"])


if __name__ == "__main__":
    unittest.main()
