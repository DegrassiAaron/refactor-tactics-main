"""Validazione della roadmap: la forma, i riferimenti, e il confine PLAN/RUNTIME.

Il test piu' importante di questo file e' `test_stato_nel_file_e_rifiutato`. Tutto il
resto verifica che una roadmap malformata venga respinta con un messaggio utile; quello
verifica che una roadmap BEN formata ma che porta stato venga respinta lo stesso, ed e'
la regola che impedisce al control plane di diventare una seconda autorita' accanto a
Git.
"""

import os
import tempfile
import unittest

from rt3 import ROADMAP_SCHEMA_VERSION
from rt3.roadmap import (
    Roadmap,
    RoadmapError,
    content_hash,
    load,
    load_document,
    normalize,
)
from rt3.yamlmini import parse
from tests.harness import smoke_roadmap_path

MINIMAL = """
roadmapSchemaVersion: 1
id: test
epics:
  - id: E1
    homeWork: DEV
    issues:
      - id: A
        estimate: 1
      - id: B
        estimate: 1
        requires:
          - item: A
            gate: VALIDATED
"""


def _normalize(source):
    return normalize(parse(source))


def _codes(problems):
    return [p.code for p in problems]


class SmokeRoadmapTest(unittest.TestCase):
    """La roadmap versionata deve caricare. E' il file che lo smoke usa davvero."""

    def setUp(self):
        self.roadmap, self.problems = load(smoke_roadmap_path())

    def test_carica_senza_errori(self):
        self.assertEqual([p for p in self.problems if p.level == "ERROR"], [])
        self.assertEqual(self.roadmap.id, "rt3-smoke-multi-epic")
        self.assertEqual(self.roadmap.schema_version, ROADMAP_SCHEMA_VERSION)

    def test_tre_epic_e_dieci_issue(self):
        self.assertEqual([e.id for e in self.roadmap.epics], ["EPIC-A", "EPIC-B", "EPIC-C"])
        self.assertEqual(len(self.roadmap.items), 10)

    def test_chiavi_canoniche(self):
        self.assertIn("EPIC-B/B2", self.roadmap.items)
        self.assertNotIn("B2", self.roadmap.items)

    def test_dipendenza_cross_epic_risolta(self):
        c2 = self.roadmap.get("EPIC-C/C2")
        self.assertEqual(c2.requires, [{"item": "EPIC-B/B2", "gate": "VALIDATED"}])
        self.assertNotEqual(
            c2.epic_id,
            self.roadmap.get("EPIC-B/B2").epic_id,
            "la dipendenza deve attraversare due Epic, altrimenti non prova nulla",
        )

    def test_forma_breve_risolta_alla_chiave_canonica(self):
        """`requires: - item: A1` sta nel file; qui deve essere `EPIC-A/A1`."""
        self.assertEqual(
            self.roadmap.get("EPIC-A/A2").requires,
            [{"item": "EPIC-A/A1", "gate": "VALIDATED"}],
        )

    def test_execution_work_eredita_da_home_work(self):
        for key, item in self.roadmap.items.items():
            self.assertIsNotNone(item.execution_work, key)
        self.assertEqual(self.roadmap.get("EPIC-B/B3").execution_work, "DEV")
        self.assertEqual(self.roadmap.get("EPIC-C/C3").execution_work, "DESIGNER")

    def test_risorsa_unreal_dichiarata_da_due_issue(self):
        """Con una sola issue Unreal il test di capacita' non distinguerebbe
        «rispetta il limite» da «non lo ha mai raggiunto»."""
        unreal = [k for k, i in self.roadmap.items.items() if i.needs_unreal]
        self.assertEqual(sorted(unreal), ["EPIC-A/A3", "EPIC-C/C3"])

    def test_capacita_dichiarate(self):
        self.assertEqual(self.roadmap.writer_capacity("DEV"), 1)
        self.assertEqual(self.roadmap.temporary_worktree_capacity, 1)
        self.assertEqual(self.roadmap.unreal_lease_capacity, 1)

    def test_round_trip_dal_dizionario(self):
        """Il documento salvato nel database deve ricostruire la stessa roadmap."""
        rebuilt = Roadmap.from_dict(self.roadmap.as_dict())
        self.assertEqual(rebuilt.as_dict(), self.roadmap.as_dict())


class RuntimeInPlanTest(unittest.TestCase):
    def test_stato_nel_file_e_rifiutato(self):
        source = MINIMAL.replace("        estimate: 1\n", "        estimate: 1\n        state: VALIDATED\n", 1)
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_RUNTIME_KEY_IN_PLAN", _codes(problems))

    def test_assegnazione_nel_file_e_rifiutata(self):
        source = MINIMAL.replace(
            "        estimate: 1\n", "        estimate: 1\n        assignee: DEV-1\n", 1
        )
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_RUNTIME_KEY_IN_PLAN", _codes(problems))

    def test_messaggio_spiega_dove_vive_lo_stato(self):
        source = MINIMAL.replace(
            "        estimate: 1\n", "        estimate: 1\n        progress: DONE\n", 1
        )
        _roadmap, problems = _normalize(source)
        message = [p.message for p in problems if p.code == "ROADMAP_RUNTIME_KEY_IN_PLAN"][0]
        self.assertIn("control plane", message)


class ValidationTest(unittest.TestCase):
    def test_minimale_e_valida(self):
        roadmap, problems = _normalize(MINIMAL)
        self.assertIsNotNone(roadmap)
        self.assertEqual([p for p in problems if p.level == "ERROR"], [])

    def test_schema_mancante(self):
        source = MINIMAL.replace("roadmapSchemaVersion: 1\n", "")
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_SCHEMA_MISSING", _codes(problems))

    def test_schema_troppo_nuovo_dice_di_aggiornare_il_workspace(self):
        source = MINIMAL.replace("roadmapSchemaVersion: 1", "roadmapSchemaVersion: 99")
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        problem = [p for p in problems if p.code == "ROADMAP_SCHEMA_TOO_NEW"][0]
        self.assertIn("aggiornare il workspace", problem.message.lower())

    def test_riferimento_inesistente(self):
        source = MINIMAL.replace("item: A\n", "item: ZZ\n")
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_UNKNOWN_REF", _codes(problems))

    def test_riferimento_ambiguo_non_viene_scelto(self):
        """Due issue omonime in due Epic: il parser NON ne sceglie una."""
        source = """
roadmapSchemaVersion: 1
id: test
epics:
  - id: E1
    homeWork: DEV
    issues:
      - id: A
      - id: X
  - id: E2
    homeWork: DEV
    issues:
      - id: A
      - id: Y
        requires: [A]
"""
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        problem = [p for p in problems if p.code == "ROADMAP_AMBIGUOUS_REF"][0]
        self.assertIn("E1/A", problem.message)
        self.assertIn("E2/A", problem.message)

    def test_riferimento_ambiguo_risolto_dalla_forma_lunga(self):
        source = """
roadmapSchemaVersion: 1
id: test
epics:
  - id: E1
    homeWork: DEV
    issues:
      - id: A
  - id: E2
    homeWork: DEV
    issues:
      - id: A
      - id: Y
        requires: [E1/A]
"""
        roadmap, problems = _normalize(source)
        self.assertIsNotNone(roadmap, _codes(problems))
        self.assertEqual(
            roadmap.get("E2/Y").requires, [{"item": "E1/A", "gate": "VALIDATED"}]
        )

    def test_auto_dipendenza(self):
        source = MINIMAL.replace("      - id: B\n", "      - id: B2\n").replace(
            "item: A\n", "item: B2\n"
        )
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_SELF_DEPENDENCY", _codes(problems))

    def test_gate_sconosciuto(self):
        source = MINIMAL.replace("gate: VALIDATED", "gate: QUASI")
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_BAD_GATE", _codes(problems))

    def test_risorsa_sconosciuta_e_un_errore_non_un_silenzio(self):
        source = MINIMAL.replace(
            "      - id: A\n", "      - id: A\n        resources: [GPU_FARM]\n"
        )
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        problem = [p for p in problems if p.code == "ROADMAP_UNKNOWN_RESOURCE"][0]
        self.assertIn("ignorata dallo scheduler", problem.message)

    def test_workspace_group_sconosciuto(self):
        source = MINIMAL.replace("homeWork: DEV", "homeWork: QA")
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_BAD_HOMEWORK", _codes(problems))

    def test_chiave_sconosciuta(self):
        source = MINIMAL.replace("      - id: A\n", "      - id: A\n        priorita: alta\n")
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_UNKNOWN_KEY", _codes(problems))

    def test_issue_duplicata_nello_stesso_epic(self):
        source = MINIMAL.replace("      - id: B\n", "      - id: A\n")
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_DUPLICATE_ISSUE", _codes(problems))

    def test_epic_senza_issue(self):
        source = """
roadmapSchemaVersion: 1
id: test
epics:
  - id: E1
    homeWork: DEV
"""
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_EPIC_NO_ISSUES", _codes(problems))

    def test_estimate_negativa(self):
        source = MINIMAL.replace("        estimate: 1\n", "        estimate: -3\n", 1)
        roadmap, problems = _normalize(source)
        self.assertIsNone(roadmap)
        self.assertIn("ROADMAP_NEGATIVE", _codes(problems))

    def test_tutti_gli_errori_insieme_non_solo_il_primo(self):
        """Un errore per volta costringe a un ciclo correggi-riesegui lungo quanto
        il numero di errori."""
        source = MINIMAL.replace("homeWork: DEV", "homeWork: QA").replace(
            "item: A\n", "item: ZZ\n"
        )
        _roadmap, problems = _normalize(source)
        codes = _codes(problems)
        self.assertIn("ROADMAP_BAD_HOMEWORK", codes)
        self.assertIn("ROADMAP_UNKNOWN_REF", codes)

    def test_requiredgate_a_livello_di_issue_avverte_e_non_blocca(self):
        source = MINIMAL.replace(
            "      - id: A\n", "      - id: A\n        requiredGate: VALIDATED\n"
        )
        roadmap, problems = _normalize(source)
        self.assertIsNotNone(roadmap)
        self.assertIn("ROADMAP_REQUIREDGATE_DEPRECATED", _codes(problems))


class ContentHashTest(unittest.TestCase):
    def test_hash_non_dipende_dalle_fini_riga(self):
        """🔴 `core.autocrlf=true` da' allo STESSO file byte diversi su Windows.

        Un hash che ne dipendesse direbbe «roadmap diversa» a due workspace che hanno
        lo stesso commit - cioe' fallirebbe esattamente il confronto per cui esiste.
        """
        lf = "roadmapSchemaVersion: 1\nid: test\n"
        crlf = lf.replace("\n", "\r\n")
        self.assertNotEqual(lf, crlf)
        self.assertEqual(content_hash(lf), content_hash(crlf))

    def test_hash_cambia_col_contenuto(self):
        self.assertNotEqual(content_hash("id: a\n"), content_hash("id: b\n"))


class LoadErrorTest(unittest.TestCase):
    def test_file_inesistente(self):
        with self.assertRaises(RoadmapError) as ctx:
            load(os.path.join(tempfile.gettempdir(), "rt3-non-esiste-mai.yaml"))
        self.assertIn("non leggibile", str(ctx.exception))

    def test_yaml_malformato_riporta_la_riga(self):
        with tempfile.NamedTemporaryFile(
            "w", suffix=".yaml", delete=False, encoding="utf-8"
        ) as handle:
            handle.write("roadmapSchemaVersion: 1\nid: test\nx: &ancora\n")
            path = handle.name
        try:
            with self.assertRaises(RoadmapError) as ctx:
                load_document(path)
            self.assertIn("riga 3", str(ctx.exception))
        finally:
            os.unlink(path)


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
