#!/usr/bin/env python3
"""Test della derivazione issue -> `domain_group` e del gate che la fa valere.

Uso:
    python scripts/test_feature_registry_lanes.py        # dalla radice del repo
    python -m unittest discover -s scripts -p "test_*.py"

Pinna la proprieta' che il 2026-08-14 non era vera e non lo diceva: **il `domain_group` scritto a
mano su un nodo del grafo non era confrontato con la sorgente**. Due nodi su diciassette erano
divergenti, per la stessa ragione — il gruppo era stato letto dal *checkpoint* invece che
dall'`area` della feature:

- `issue:319` in `actions_reactions` per via del checkpoint `E14.8`, mentre
  `RT-FEAT-CORE-DECISION-TIME-BANK` ha `area: Core`;
- `issue:625` in `tooling_data_qa`, mentre le due sole feature che la dichiarano sono
  `area: Environment`.

Nessuna delle due rompeva un gate: producevano una vista filtrata male, in silenzio. E' lo stesso
difetto degli altri file di questa cartella — uno stato scritto due volte, di cui la seconda copia
diverge — visto dal lato della classificazione invece che da quello dello stato.

I test di derivazione girano su fixture: serve una contraddizione che il repository **non deve
avere**. L'ultimo gira sul repository reale, perche' quella proprieta' e' *«oggi nessun nodo
contraddice il registry»* e su una fixture non direbbe niente.
"""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import feature_registry as fr  # noqa: E402


def _doc(nodes):
    """Un grafo minimo: due gruppi su due aree, nessuna relazione, nessun checkpoint.

    Senza checkpoint di proposito — un nodo che ne dichiara uno finirebbe nell'altro controllo
    (`checkpoint che nessun owner dichiara`) e l'errore atteso non sarebbe piu' distinguibile.
    """
    return {
        "schema_version": fr.EXEC_SCHEMA_VERSION,
        "execution_lanes": [{"id": "code", "title": "CODE"}],
        "domain_groups": [
            {"id": "core_simulation", "title": "Core & Simulation", "order": 10,
             "feature_areas": ["Core"]},
            {"id": "ui_presentation", "title": "UI & Presentation", "order": 20,
             "feature_areas": ["UI"]},
        ],
        "nodes": nodes,
        "relations": [],
    }


def _registry(*features):
    return {"features": [
        {"feature_id": fid, "title": "t", "area": area, "release": "v0.1",
         "status": "INTEGRATED", "issues": list(issues), "roadmap": {"epic": None}}
        for fid, area, issues in features
    ]}


class DerivationComposesTheTwoTaxonomies(unittest.TestCase):
    """`issue_domain_groups` non classifica: compone `feature.issues` con `feature_areas`."""

    def test_one_feature_gives_one_group(self):
        derived = fr.issue_domain_groups(_registry(("RT-FEAT-A", "Core", [10])), _doc([]))
        self.assertEqual(derived, {10: {"core_simulation": ["RT-FEAT-A"]}})

    def test_two_features_same_area_stay_one_group(self):
        derived = fr.issue_domain_groups(
            _registry(("RT-FEAT-A", "Core", [10]), ("RT-FEAT-B", "Core", [10])), _doc([]))
        self.assertEqual(list(derived[10]), ["core_simulation"],
                         "due feature della stessa area non rendono ambigua la issue")
        self.assertEqual(sorted(derived[10]["core_simulation"]), ["RT-FEAT-A", "RT-FEAT-B"])

    def test_two_areas_make_the_issue_ambiguous(self):
        # Il caso reale e' `#77`: dichiarata da una feature Core e da tre UI.
        derived = fr.issue_domain_groups(
            _registry(("RT-FEAT-A", "Core", [77]), ("RT-FEAT-B", "UI", [77])), _doc([]))
        self.assertEqual(sorted(derived[77]), ["core_simulation", "ui_presentation"])

    def test_area_outside_every_group_yields_nothing(self):
        # Non e' un buco silenzioso: `validate_execution_graph` lo dichiara gia' come errore
        # («area X non appartiene a nessun domain_group»). Qui si pinna che la derivazione non
        # inventi un gruppo per compensare.
        derived = fr.issue_domain_groups(_registry(("RT-FEAT-A", "Vfx", [10])), _doc([]))
        self.assertEqual(derived, {})


class GraphNodeCannotContradictTheRegistry(unittest.TestCase):
    """Il gate: un nodo che dichiara un gruppo che la sorgente non deriva e' un errore."""

    def _validate(self, doc, registry):
        original = fr.load_execution_graph
        fr.load_execution_graph = lambda: doc
        try:
            return fr.validate_execution_graph(
                ctx={"sessions": [], "checkpoints": set()}, registry=registry)
        finally:
            fr.load_execution_graph = original

    def test_agreement_is_silent(self):
        errors, _ = self._validate(
            _doc([{"id": "issue:10", "domain_group": "core_simulation"}]),
            _registry(("RT-FEAT-A", "Core", [10])))
        self.assertEqual([e for e in errors if "contraddice" in e], [])

    def test_contradiction_is_an_error(self):
        # La mutazione e' esattamente `issue:319`: gruppo scelto sul checkpoint, area diversa.
        errors, _ = self._validate(
            _doc([{"id": "issue:10", "domain_group": "ui_presentation"}]),
            _registry(("RT-FEAT-A", "Core", [10])))
        contradictions = [e for e in errors if "contraddice" in e]
        self.assertEqual(len(contradictions), 1, "la contraddizione non e' stata dichiarata")
        self.assertIn("core_simulation", contradictions[0],
                      "l'errore non dice quale gruppo la sorgente deriva")
        self.assertIn("RT-FEAT-A", contradictions[0],
                      "l'errore non nomina la feature da cui il gruppo si deriva")

    def test_disambiguating_an_ambiguous_issue_is_allowed(self):
        # Una issue che attraversa due domini ne ammette due: il nodo che ne sceglie uno sta
        # disambiguando, e non contraddice niente.
        #
        # ⚠️ La prima stesura giustificava questa regola con «senza, il gate accenderebbe un
        # errore su `#77`, `#84` e `#324`». **Era falso, ed e' stato misurato**: nessuno dei 17
        # nodi del grafo cita una delle 11 issue ambigue del registry — l'intersezione e' vuota.
        # Il ramo permissivo non e' esercitato da nessun dato reale, e senza di esso il gate oggi
        # non segnalerebbe niente. La regola resta perche' e' la semantica giusta il giorno in cui
        # un nodo citera' una di quelle issue, non perche' ne stia salvando tre adesso: e' una
        # scelta di design, e questo test e' l'unico posto in cui e' esercitata.
        registry = _registry(("RT-FEAT-A", "Core", [77]), ("RT-FEAT-B", "UI", [77]))
        for chosen in ("core_simulation", "ui_presentation"):
            errors, _ = self._validate(
                _doc([{"id": "issue:77", "domain_group": chosen}]), registry)
            self.assertEqual([e for e in errors if "contraddice" in e], [],
                             f"scegliere {chosen} fra i due gruppi derivati non e' una "
                             "contraddizione")

    def test_a_node_without_a_group_is_not_silent(self):
        # Il buco che il gate aveva: senza `domain_group:` il nodo spariva da ogni filtro e
        # `validate` restava verde. Il generatore ora deriva il gruppo quando puo'; quando non
        # puo', questo warning e' l'unica cosa che lo dice.
        errors, warnings = self._validate(
            _doc([{"id": "issue:999"}]), _registry(("RT-FEAT-A", "Core", [10])))
        self.assertEqual(errors, [])
        self.assertTrue([w for w in warnings if "issue:999" in w and "nessun domain_group" in w],
                        "un nodo senza gruppo e senza derivazione passa in silenzio")

    def test_an_ambiguous_node_without_a_group_says_why(self):
        errors, warnings = self._validate(
            _doc([{"id": "issue:77"}]),
            _registry(("RT-FEAT-A", "Core", [77]), ("RT-FEAT-B", "UI", [77])))
        self.assertEqual(errors, [])
        self.assertTrue([w for w in warnings if "issue:77" in w and "piu' domini" in w],
                        "l'ambiguita' non viene distinta dall'assenza di feature")

    def test_issue_no_feature_declares_is_a_warning_not_an_error(self):
        # Il nodo esiste, la issue no: il gruppo non e' verificabile. Non e' un errore — quattro
        # nodi reali sono in questo stato — ma non deve passare in silenzio.
        errors, warnings = self._validate(
            _doc([{"id": "issue:999", "domain_group": "core_simulation"}]),
            _registry(("RT-FEAT-A", "Core", [10])))
        self.assertEqual([e for e in errors if "contraddice" in e], [])
        self.assertTrue([w for w in warnings if "issue:999" in w and "verificabile" in w],
                        "un gruppo non verificabile non viene segnalato")


class TheGeneratedViewDerivesInsteadOfCopying(unittest.TestCase):
    """Un nodo senza `domain_group` non sparisce dai filtri: il generatore lo deriva.

    E' la regola del modulo — «la copia e' generata» — applicata al `domain_group`. Prima il
    valore era scritto a mano su ogni nodo, e per 13 dei 17 nodi reali il registry ne derivava
    esattamente uno: la copia non portava informazione, poteva solo essere sbagliata.
    """

    def _node(self, node, features):
        original_doc, original_reg = fr.load_execution_graph, fr.load_registry
        fr.load_execution_graph = lambda: _doc([node])
        fr.load_registry = lambda: _registry(*features)
        try:
            graph = fr.build_execution(fr.editor_context(), _registry(*features))
        finally:
            fr.load_execution_graph, fr.load_registry = original_doc, original_reg
        return graph["nodes"][0]

    def test_absent_group_is_derived(self):
        record = self._node({"id": "issue:10"}, [("RT-FEAT-A", "Core", [10])])
        self.assertEqual(record["domain_group"], "core_simulation")
        self.assertEqual(record["domain_group_source"], "derived")

    def test_declared_group_wins_and_says_so(self):
        record = self._node({"id": "issue:10", "domain_group": "ui_presentation"},
                            [("RT-FEAT-A", "UI", [10])])
        self.assertEqual(record["domain_group"], "ui_presentation")
        self.assertEqual(record["domain_group_source"], "declared")

    def test_ambiguous_is_not_guessed(self):
        record = self._node({"id": "issue:77"},
                            [("RT-FEAT-A", "Core", [77]), ("RT-FEAT-B", "UI", [77])])
        self.assertIsNone(record["domain_group"],
                          "un'issue che attraversa due domini e' stata assegnata a indovinare")
        self.assertIsNone(record["domain_group_source"])


class AMalformedIdDoesNotKillTheGate(unittest.TestCase):
    """Un `issue:abc` deve produrre l'errore che il validator ha gia', non un traceback.

    Il controllo sul `domain_group` gira in un loop **diverso** da quello che rifiuta gli id non
    conformi, quindi la prima stesura ci arrivava con `int('abc')` e moriva — sopprimendo anche
    il messaggio leggibile che il validator aveva gia' preparato.
    """

    def _doc_with_bad_id(self):
        doc = _doc([{"id": "issue:abc", "domain_group": "core_simulation"}])
        return doc

    def test_validate_reports_instead_of_crashing(self):
        original = fr.load_execution_graph
        fr.load_execution_graph = lambda: self._doc_with_bad_id()
        try:
            errors, _ = fr.validate_execution_graph(
                ctx={"sessions": [], "checkpoints": set()},
                registry=_registry(("RT-FEAT-A", "Core", [10])))
        finally:
            fr.load_execution_graph = original
        self.assertTrue([e for e in errors if "id non conforme" in e],
                        "l'id malformato non e' stato dichiarato")

    def test_render_lanes_survives(self):
        view = fr.render_lanes(_registry(("RT-FEAT-A", "Core", [10])),
                               doc=self._doc_with_bad_id())
        self.assertIn("core_simulation", view)


class CoverageIsMeasuredNotAssumed(unittest.TestCase):
    """`render_lanes` riporta cosa NON copre: e' l'unica parte che invecchia se taciuta."""

    def test_orphan_open_issues_are_named(self):
        view = fr.render_lanes(
            _registry(("RT-FEAT-A", "Core", [10])), doc=_doc([]), open_numbers={10, 4242})
        self.assertIn("#4242", view, "l'issue senza gruppo non compare fra le scoperte")
        self.assertIn("| 2 | 1 | 1 |", view.replace("  ", " "))

    def test_closed_issues_do_not_inflate_the_count(self):
        # Il perimetro e' «le aperte»: una issue del registry che su GitHub e' chiusa non deve
        # comparire nel conteggio del gruppo, altrimenti la vista misura il passato.
        view = fr.render_lanes(
            _registry(("RT-FEAT-A", "Core", [10]), ("RT-FEAT-B", "Core", [11])),
            doc=_doc([]), open_numbers={10})
        self.assertIn("| `core_simulation` | Core & Simulation | 1 |", view)


class TheRealRepositoryAgrees(unittest.TestCase):
    """Sul repository vero: nessun nodo contraddice il registry. E' la regressione."""

    def test_no_node_contradicts_the_registry(self):
        errors, _ = fr.validate_execution_graph()
        contradictions = [e for e in errors if "contraddice il registry" in e]
        self.assertEqual(contradictions, [], "\n".join(contradictions))


if __name__ == "__main__":
    unittest.main(verbosity=2)
