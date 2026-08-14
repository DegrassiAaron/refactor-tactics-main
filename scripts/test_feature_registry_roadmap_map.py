#!/usr/bin/env python3
"""Test della Roadmap Map: la collocazione epic → (corsia, release) e la figura che ne esce.

Uso:
    python scripts/test_feature_registry_roadmap_map.py     # dalla radice del repo
    python -m unittest discover -s scripts -p "test_*.py"

La figura ha due assi e un vincolo — ogni epic finisce in una cella — e il vincolo e' esattamente
il posto dove una vista mente volentieri. Le proprieta' pinnate qui sono quelle che, senza test,
si romperebbero producendo una griglia **piu' pulita** invece che un errore:

- un'epic che nessuna feature dichiara non riceve una corsia inventata: esce, con il motivo. Una
  griglia che la collocasse comunque sarebbe indistinguibile da una giusta;
- un'epic le cui feature stanno in piu' corsie viene disegnata dove sta la maggioranza **e lo
  dichiara**. Il difetto silenzioso qui non e' sbagliare corsia: e' far credere che ne abbia una;
- la corsia scelta a parita' di feature dipende dall'`order`, non dall'ordine di iterazione di un
  dict. Senza, due esecuzioni producono due SVG diversi dalla stessa sorgente, e `generate --check`
  diventa rosso a giorni alterni — il difetto che `generato-che-dipende-dal-flag` descrive, spostato
  dall'argomento all'iterazione;
- le release si ordinano per numero, non per stringa. `v0.10` dopo `v0.2` e' l'unico caso in cui
  l'ordinamento sbagliato **non** si vede subito: oggi il repository arriva a `v0.9`, quindi il
  difetto entrerebbe muto e uscirebbe solo alla release successiva.

L'ultimo test gira sul repository reale, perche' la proprieta' e' *«oggi nessuna epic svanisce»* e
su una fixture non direbbe niente.
"""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import feature_registry as fr  # noqa: E402


def _doc():
    """Due corsie, due aree, ordini distinti — il minimo per distinguere maggioranza da ordine."""
    return {
        "domain_groups": [
            {"id": "alpha", "title": "Alpha", "order": 10, "feature_areas": ["Core"]},
            {"id": "beta", "title": "Beta", "order": 20, "feature_areas": ["Map"]},
        ],
        "execution_lanes": [],
        "nodes": [],
        "relations": [],
    }


def _feature(fid, area, epic):
    return {"feature_id": fid, "area": area, "roadmap": {"epic": epic}, "issues": []}


def _entry(epic, release="v0.1", **extra):
    base = {"epic": epic, "title": f"Titolo {epic}", "priority": "P1", "release": release,
            "issue": None, "glyph": "", "cp_done": 0, "cp_total": 0}
    base.update(extra)
    return base


class UnEpicSenzaFeatureNonRiceveUnaCorsia(unittest.TestCase):
    def test_esce_dalla_griglia_col_motivo(self):
        registry = {"features": []}
        model = fr.roadmap_map_model(registry, _doc(), {"E1": _entry("E1")})
        self.assertEqual([], model["epics"])
        self.assertEqual(1, len(model["unplaced"]))
        self.assertIn("nessuna feature", model["unplaced"][0]["why"])

    def test_senza_release_non_si_indovina(self):
        registry = {"features": [_feature("F1", "Core", "E1")]}
        model = fr.roadmap_map_model(registry, _doc(), {"E1": _entry("E1", release=None)})
        self.assertEqual([], model["epics"])
        self.assertIn("release", model["unplaced"][0]["why"])


class LaMaggioranzaDecideELoDichiara(unittest.TestCase):
    def test_va_nella_corsia_con_piu_feature(self):
        registry = {"features": [_feature("F1", "Core", "E1"), _feature("F2", "Core", "E1"),
                                 _feature("F3", "Map", "E1")]}
        model = fr.roadmap_map_model(registry, _doc(), {"E1": _entry("E1")})
        placed = model["epics"][0]
        self.assertEqual("alpha", placed["lane"])
        # La corsia di minoranza non sparisce: e' il dato che distingue «sta qui» da «sta solo qui».
        self.assertEqual(["beta"], placed["also_in"])

    def test_a_parita_decide_l_order_non_l_iterazione(self):
        registry = {"features": [_feature("F1", "Core", "E1"), _feature("F2", "Map", "E1")]}
        model = fr.roadmap_map_model(registry, _doc(), {"E1": _entry("E1")})
        # `alpha` ha order 10, `beta` 20: a parita' vince il primo, sempre, in ogni esecuzione.
        self.assertEqual("alpha", model["epics"][0]["lane"])


class LOrdineDelleReleaseENumerico(unittest.TestCase):
    def test_v0_10_viene_dopo_v0_2(self):
        registry = {"features": [_feature("F1", "Core", "E1"), _feature("F2", "Core", "E2"),
                                 _feature("F3", "Core", "E3")]}
        catalog = {"E1": _entry("E1", release="v0.2"),
                   "E2": _entry("E2", release="v0.10"),
                   "E3": _entry("E3", release="v0.9")}
        model = fr.roadmap_map_model(registry, _doc(), catalog)
        self.assertEqual(["v0.2", "v0.9", "v0.10"], model["releases"])


class LaFiguraNonPerdePezzi(unittest.TestCase):
    def test_un_riquadro_per_ogni_epic_collocata(self):
        registry = {"features": [_feature("F1", "Core", "E1"), _feature("F2", "Map", "E2")]}
        catalog = {"E1": _entry("E1"), "E2": _entry("E2", release="v0.2")}
        model = fr.roadmap_map_model(registry, _doc(), catalog)
        svg = fr.render_roadmap_map_svg(model)
        self.assertEqual(len(model["epics"]), svg.count('rx="6"'))
        for epic in model["epics"]:
            self.assertIn(f">{epic['epic']}<", svg.replace("</text>", "<"))

    def test_la_pagina_dichiara_le_non_collocate(self):
        registry = {"features": []}
        model = fr.roadmap_map_model(registry, _doc(), {"E9": _entry("E9")})
        page = fr.render_roadmap_map_page(model)
        # Il numero e l'identita' vanno scritti: un vuoto non deve poter passare per un verde.
        self.assertIn("`E9`", page)
        self.assertIn("non colloca", page)


class SulRepositoryReale(unittest.TestCase):
    """Qui non serve una fixture: la domanda e' se **oggi** la vista perde qualcosa."""

    def setUp(self):
        self.registry = fr.load_registry()
        self.model = fr.roadmap_map_model(self.registry)

    def test_nessuna_epic_del_catalogo_svanisce(self):
        catalog = fr.epic_catalog()
        visti = {e["epic"] for e in self.model["epics"]} | {e["epic"] for e in self.model["unplaced"]}
        self.assertEqual(set(catalog), visti,
                         "un'epic del catalogo non e' ne' collocata ne' dichiarata: sparisce")

    def test_ogni_epic_collocata_sta_in_una_corsia_dichiarata(self):
        lanes = {lane["id"] for lane in self.model["lanes"]}
        for epic in self.model["epics"]:
            self.assertIn(epic["lane"], lanes)

    def test_la_figura_si_rigenera_identica(self):
        # Stessa sorgente, due letture: se divergono, `generate --check` e' rosso a giorni alterni.
        primo = fr.render_roadmap_map_svg(fr.roadmap_map_model(self.registry))
        secondo = fr.render_roadmap_map_svg(fr.roadmap_map_model(fr.load_registry()))
        self.assertEqual(primo, secondo)


if __name__ == "__main__":
    unittest.main(verbosity=2)
