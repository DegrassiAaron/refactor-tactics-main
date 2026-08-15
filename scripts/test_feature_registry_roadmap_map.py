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


class GliArchiSiDerivanoENonSiInventano(unittest.TestCase):
    """Gli archi sono il pezzo per cui questa e' una figura: senza, l'imbuto si conta invece di
    vederlo. E sono anche il pezzo che mente piu' facilmente, perche' una linea in piu' e' bella."""

    def _registry(self):
        # F1 (E1) e' dipendenza di F2 (E2) e F3 (E3): E1 ne sblocca due.
        return {"features": [
            {"feature_id": "F1", "area": "Core", "roadmap": {"epic": "E1"}, "dependencies": []},
            {"feature_id": "F2", "area": "Map", "roadmap": {"epic": "E2"}, "dependencies": ["F1"]},
            {"feature_id": "F3", "area": "Map", "roadmap": {"epic": "E3"}, "dependencies": ["F1"]},
        ]}

    def test_l_arco_nasce_dalle_dipendenze_fra_feature(self):
        catalog = {e: _entry(e) for e in ("E1", "E2", "E3")}
        model = fr.roadmap_map_model(self._registry(), _doc(), catalog)
        self.assertEqual([("E1", "E2"), ("E1", "E3")], model["edges"])
        self.assertEqual(2, model["fanout"]["E1"])

    def test_un_arco_verso_un_epic_non_disegnata_non_si_disegna(self):
        # E3 esce dalla griglia: l'arco E1->E3 non ha una destinazione sulla figura, e una linea
        # che finisce nel vuoto e' peggio di una linea assente — `unplaced` dice gia' chi manca.
        catalog = {"E1": _entry("E1"), "E2": _entry("E2"), "E3": _entry("E3", release=None)}
        model = fr.roadmap_map_model(self._registry(), _doc(), catalog)
        self.assertEqual([("E1", "E2")], model["edges"])
        self.assertEqual(1, model["fanout"]["E1"])

    def test_un_arco_che_PARTE_da_un_epic_non_disegnata_non_si_disegna(self):
        """🔴 Il gemello del test sopra, e senza di lui quello sopra non prova quel che dice.

        La verifica di mutazione l'ha misurato: togliendo il filtro sul **source** i test restavano
        tutti verdi, perche' il caso costruito sopra viene gia' scartato dal filtro sul **target**
        — E3 non e' disegnata *come destinazione*. Il filtro sul source non era coperto da nessuno.

        Qui la direzione e' invertita: E1 esce dalla griglia, E2 resta. L'arco E1->E2 ha una
        destinazione valida e una **partenza che non esiste sulla figura**, ed e' l'unico caso in
        cui quel filtro parla.
        """
        catalog = {"E1": _entry("E1", release=None), "E2": _entry("E2"), "E3": _entry("E3")}
        model = fr.roadmap_map_model(self._registry(), _doc(), catalog)
        self.assertEqual([], model["edges"])
        self.assertEqual({}, model["fanout"])

    def test_conta_solo_come_attraversanti_quelli_che_cambiano_cella(self):
        # E1 sta in `alpha`, E2 ed E3 in `beta`: entrambi gli archi attraversano una corsia.
        catalog = {e: _entry(e) for e in ("E1", "E2", "E3")}
        model = fr.roadmap_map_model(self._registry(), _doc(), catalog)
        self.assertEqual(2, model["totals"]["crossing"])
        self.assertEqual(2, model["totals"]["edges"])

    def test_la_figura_disegna_una_linea_per_arco(self):
        catalog = {e: _entry(e) for e in ("E1", "E2", "E3")}
        model = fr.roadmap_map_model(self._registry(), _doc(), catalog)
        svg = fr.render_roadmap_map_svg(model)
        self.assertEqual(len(model["edges"]), svg.count("<path d="))


class LIssueDellEpicSiLeggeOffline(unittest.TestCase):
    """La prima stesura dichiarava questo dato «non derivabile senza GitHub». Era falso, e il modo
    in cui era falso conta: la ricerca era stata fatta in `roadmap-v0.1.md` — che davvero non lo
    porta — e la conclusione estesa a tutto il repository senza guardare `v0.1-issue-plan.md`."""

    def test_le_due_letture_concordi_danno_il_numero(self):
        mappa, conflicts = fr.epic_issue_map()
        self.assertEqual([], conflicts, "lista e heading di v0.1-issue-plan.md divergono")
        self.assertEqual(26, mappa.get("E12"))

    def test_quando_le_due_letture_divergono_il_numero_non_si_sceglie(self):
        """🔴 Il gate sui conflitti, su fixture: sul repository reale non poteva essere provato.

        Con la riconciliazione dentro `epic_issue_map()` questo controllo era invisibile ai test —
        disattivandolo la lista dei conflitti restava vuota **perche' nessuno la riempiva**, e
        `assertEqual([], conflicts)` passava lo stesso. Serve una contraddizione costruita.
        """
        out, conflicts = fr.reconcile_epic_issues({"E1": 15, "E2": 99}, {"E1": 15, "E2": 16})
        self.assertEqual([("E2", 99, 16)], conflicts)
        self.assertNotIn("E2", out, "un'epic contraddetta non deve ricevere un numero")
        self.assertEqual(15, out["E1"])

    def test_una_lettura_sola_basta_quando_l_altra_tace(self):
        out, conflicts = fr.reconcile_epic_issues({"E1": 15}, {"E2": 16})
        self.assertEqual({"E1": 15, "E2": 16}, out)
        self.assertEqual([], conflicts)

    def test_l_epic_fuori_dal_piano_si_legge_dalla_sua_sezione(self):
        # E21 non e' in `v0.1-issue-plan.md`: la sua sezione in `roadmap-v0.1.md` dichiara
        # «Tracciata su GitHub … epic #286 … Era l'unica epic della v0.1 senza issue».
        mappa, _ = fr.epic_issue_map()
        self.assertEqual(286, mappa.get("E21"))

    def test_oggi_nessuna_epic_del_catalogo_resta_senza_issue(self):
        senza = sorted(e for e, v in fr.epic_catalog().items() if not v.get("issue"))
        self.assertEqual([], senza)

    def test_la_figura_stampa_il_numero(self):
        registry = {"features": [_feature("F1", "Core", "E1")]}
        model = fr.roadmap_map_model(registry, _doc(), {"E1": _entry("E1", issue=26)})
        self.assertIn("#26", fr.render_roadmap_map_svg(model))


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
