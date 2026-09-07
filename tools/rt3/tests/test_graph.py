"""Grafo delle dipendenze: cicli, ordine topologico, cammino critico.

Sul cammino critico c'e' un modo caratteristico di sbagliare che questi test cercano di
chiudere: verificare la DURATA e non il CAMMINO. La durata e' un numero solo, e un
calcolo sbagliato ne produce comunque uno plausibile. I test qui asseriscono la catena
per esteso, e su una roadmap in cui il cammino attraversa due Epic - se restasse dentro
un Epic, non proverebbe che il grafo sia unico e non tre grafi affiancati.
"""

import unittest

from rt3.graph import (
    build,
    critical_path,
    cycle_problems,
    find_cycles,
    load_checked,
    schedule,
    topological,
)
from rt3.roadmap import RoadmapError, normalize
from rt3.yamlmini import parse
from tests.harness import load_smoke_roadmap, smoke_roadmap_path


def _graph(source):
    roadmap, problems = normalize(parse(source))
    assert roadmap is not None, [p.code for p in problems]
    return roadmap, build(roadmap)


LINEAR = """
roadmapSchemaVersion: 1
id: lineare
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
        estimate: 2
      - id: B
        estimate: 3
        requires: [A]
      - id: C
        estimate: 1
        requires: [B]
"""

DIAMOND = """
roadmapSchemaVersion: 1
id: rombo
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
        estimate: 1
      - id: LUNGO
        estimate: 10
        requires: [A]
      - id: CORTO
        estimate: 1
        requires: [A]
      - id: FINE
        estimate: 1
        requires: [LUNGO, CORTO]
"""


class BuildTest(unittest.TestCase):
    def test_predecessori_e_successori(self):
        _roadmap, g = _graph(LINEAR)
        self.assertEqual(g.predecessors["E/B"], ["E/A"])
        self.assertEqual(g.successors["E/A"], ["E/B"])
        self.assertEqual(g.successors["E/C"], [])

    def test_gate_conservato_sull_arco(self):
        _roadmap, g = _graph(LINEAR)
        self.assertEqual(g.gates[("E/A", "E/B")], "VALIDATED")

    def test_ordine_e_quello_di_dichiarazione(self):
        _roadmap, g = _graph(DIAMOND)
        self.assertEqual(g.order, ["E/A", "E/LUNGO", "E/CORTO", "E/FINE"])


class CycleTest(unittest.TestCase):
    def test_ciclo_diretto(self):
        roadmap, problems = normalize(
            parse(
                """
roadmapSchemaVersion: 1
id: ciclo
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
        requires: [B]
      - id: B
        requires: [A]
"""
            )
        )
        self.assertIsNotNone(roadmap, [p.code for p in problems])
        cycles = find_cycles(build(roadmap))
        self.assertEqual(len(cycles), 1)
        self.assertEqual(set(cycles[0]), {"E/A", "E/B"})

    def test_ciclo_indiretto_riporta_la_catena_intera(self):
        roadmap, _ = normalize(
            parse(
                """
roadmapSchemaVersion: 1
id: ciclo
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
        requires: [C]
      - id: B
        requires: [A]
      - id: C
        requires: [B]
"""
            )
        )
        problems = cycle_problems(build(roadmap))
        self.assertEqual(len(problems), 1)
        message = problems[0].message
        for key in ("E/A", "E/B", "E/C"):
            self.assertIn(key, message)
        self.assertIn("READY", message)

    def test_grafo_aciclico_non_produce_falsi_positivi(self):
        _roadmap, g = _graph(DIAMOND)
        self.assertEqual(find_cycles(g), [])

    def test_load_checked_rifiuta_il_ciclo(self):
        """Il ciclo deve fermare il CARICAMENTO, non solo comparire in un report."""
        import os
        import tempfile

        source = """
roadmapSchemaVersion: 1
id: ciclo
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
        requires: [B]
      - id: B
        requires: [A]
"""
        with tempfile.NamedTemporaryFile(
            "w", suffix=".yaml", delete=False, encoding="utf-8"
        ) as handle:
            handle.write(source)
            path = handle.name
        try:
            with self.assertRaises(RoadmapError) as ctx:
                load_checked(path)
            self.assertIn("ciclo", str(ctx.exception).lower())
        finally:
            os.unlink(path)


class TopologicalTest(unittest.TestCase):
    def test_ogni_nodo_dopo_i_suoi_predecessori(self):
        _roadmap, g = _graph(DIAMOND)
        order = topological(g)
        self.assertEqual(len(order), len(g.order))
        position = {k: i for i, k in enumerate(order)}
        for node in g.order:
            for pred in g.predecessors[node]:
                self.assertLess(position[pred], position[node], (pred, node))

    def test_tie_break_e_l_ordine_di_dichiarazione(self):
        _roadmap, g = _graph(DIAMOND)
        order = topological(g)
        self.assertLess(order.index("E/LUNGO"), order.index("E/CORTO"))

    def test_deterministico_su_ripetizioni(self):
        _roadmap, g = _graph(DIAMOND)
        self.assertEqual([topological(g) for _ in range(5)].count(topological(g)), 5)


class CriticalPathTest(unittest.TestCase):
    def test_lineare(self):
        _roadmap, g = _graph(LINEAR)
        path, duration = critical_path(g)
        self.assertEqual(path, ["E/A", "E/B", "E/C"])
        self.assertEqual(duration, 6)

    def test_rombo_sceglie_il_ramo_lungo(self):
        """Il cammino critico e' il piu' pesante, non il piu' lungo per numero di nodi:
        `CORTO` ha lo stesso numero di salti e non e' critico."""
        _roadmap, g = _graph(DIAMOND)
        path, duration = critical_path(g)
        self.assertEqual(path, ["E/A", "E/LUNGO", "E/FINE"])
        self.assertEqual(duration, 12)
        rows, _ = schedule(g)
        self.assertTrue(rows["E/LUNGO"].critical)
        self.assertFalse(rows["E/CORTO"].critical)
        self.assertEqual(rows["E/CORTO"].slack, 9)

    def test_slack_zero_solo_sul_cammino_critico(self):
        _roadmap, g = _graph(DIAMOND)
        rows, _ = schedule(g)
        for key, row in rows.items():
            self.assertEqual(row.critical, row.slack == 0, key)

    def test_ciclo_non_produce_un_numero_sbagliato(self):
        """Un cammino critico calcolato su un ciclo darebbe un numero plausibile."""
        roadmap, _ = normalize(
            parse(
                """
roadmapSchemaVersion: 1
id: ciclo
epics:
  - id: E
    homeWork: DEV
    issues:
      - id: A
        requires: [B]
      - id: B
        requires: [A]
"""
            )
        )
        rows, duration = schedule(build(roadmap))
        self.assertEqual(rows, {})
        self.assertEqual(duration, 0)


class SmokeRoadmapGraphTest(unittest.TestCase):
    def setUp(self):
        self.roadmap, self.graph = load_smoke_roadmap()

    def test_cammino_critico_attraversa_due_epic(self):
        path, duration = critical_path(self.graph)
        self.assertEqual(
            path, ["EPIC-B/B1", "EPIC-B/B2", "EPIC-C/C2", "EPIC-C/C3"]
        )
        self.assertEqual(duration, 14)
        epics = {self.roadmap.get(k).epic_id for k in path}
        self.assertEqual(
            epics,
            {"EPIC-B", "EPIC-C"},
            "un cammino critico dentro un solo Epic non proverebbe che il grafo sia unico",
        )

    def test_le_due_parallele_hanno_slack(self):
        rows, _ = schedule(self.graph)
        self.assertGreater(rows["EPIC-B/B3"].slack, 0)
        self.assertGreater(rows["EPIC-B/B4"].slack, 0)
        self.assertEqual(
            rows["EPIC-B/B3"].earliest_start,
            rows["EPIC-B/B4"].earliest_start,
            "B3 e B4 devono poter partire nello stesso momento: e' cio' che le rende "
            "parallelizzabili",
        )

    def test_b2_sblocca_esattamente_tre_issue(self):
        self.assertEqual(
            sorted(self.graph.successors["EPIC-B/B2"]),
            ["EPIC-B/B3", "EPIC-B/B4", "EPIC-C/C2"],
        )

    def test_nessun_ciclo(self):
        self.assertEqual(find_cycles(self.graph), [])

    def test_load_checked_e_l_ingresso_unico(self):
        roadmap, graph, problems = load_checked(smoke_roadmap_path())
        self.assertEqual([p for p in problems if p.level == "ERROR"], [])
        self.assertEqual(len(graph.order), len(roadmap.items))


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
