#!/usr/bin/env python3
"""Test del modello di release del Feature Registry.

Uso:
    python scripts/test_feature_registry_releases.py       # dalla radice del repo
    python -m unittest discover -s scripts -p "test_*.py"

Pinna le tre proprieta' che il 2026-08-13 (D-136) hanno smesso di essere vere da sole quando
`RELEASE_ORDER` e' passato da `v0.4` a `v1.0`. Nessuna delle tre rompeva un gate: tutte e tre
producevano una vista **degradata in silenzio**, che e' il difetto che questo repository ha gia'
pagato piu' volte.

1. **Il parser dell'owner si fermava a `v0.\\d`.** La riga `v1.0` della tabella «Le release» non
   sarebbe stata letta, e le sue epic sarebbero risultate «senza release» — che e' il caso
   legittimo di `E37`, quindi non sospetto.
2. **`known_roadmap_refs()` leggeva solo `roadmap-v0.1.md`.** Scrivere `epic: E38` su una feature
   v0.2 era un errore del validator, quindi 30 feature restavano a `epic: null` **per
   impossibilita'**; otto compensavano con un paragrafo in `out_of_release_scope`.
3. **La tabella §2.2 della v0.1 mostrava feature di qualunque release.** `RT-FEAT-REPLAY-ARCHIVE`
   (`v0.2`, epic `E12`) compariva fra le feature della v0.1 senza che niente lo segnalasse.

I test 1-2 girano sul repository reale perche' la proprieta' e' *«l'owner e il codice concordano»*,
e su una fixture sintetica non direbbe nulla. Il test 3 usa dati costruiti a mano: serve un
disallineamento che il repository non deve avere.
"""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import feature_registry as fr  # noqa: E402


class ReleaseOrderShape(unittest.TestCase):
    """Le invarianti strutturali della tupla, e il gate che le controlla."""

    def test_future_is_last(self):
        self.assertEqual(fr.RELEASE_ORDER[-1], "future")

    def test_no_duplicates(self):
        self.assertEqual(len(set(fr.RELEASE_ORDER)), len(fr.RELEASE_ORDER))

    def test_planned_releases_are_sorted(self):
        planned = [r for r in fr.RELEASE_ORDER if r != "future"]
        self.assertEqual(planned, sorted(planned))

    def test_reaches_v10(self):
        # Non e' decorazione: se qualcuno riduce la tupla, le feature gia' assegnate a v0.5-v1.0
        # diventerebbero «release non valida» e il rimedio ovvio sarebbe spostarle, non ripristinarla.
        self.assertIn("v1.0", fr.RELEASE_ORDER)

    def test_gate_accepts_the_real_repository(self):
        self.assertEqual(fr.check_release_order(), [])

    def test_gate_rejects_a_release_the_owner_does_not_declare(self):
        original = fr.RELEASE_ORDER
        try:
            fr.RELEASE_ORDER = original[:-1] + ("v9.9", "future")
            problems = fr.check_release_order()
            self.assertTrue(any("v9.9" in p for p in problems),
                            f"il gate non ha visto la release non dichiarata: {problems}")
        finally:
            fr.RELEASE_ORDER = original
        self.assertEqual(fr.check_release_order(), [], "ripristino non pulito")


class OwnerTableIsParsed(unittest.TestCase):
    """La tabella «Le release» di `roadmap-post-v0.1.md` e' letta per intero."""

    def setUp(self):
        self.epics = fr.post_v01_epics()
        self.by_release = {}
        for entry in self.epics:
            self.by_release.setdefault(entry["release"], []).append(entry["epic"])

    def test_every_planned_release_after_v01_has_at_least_one_epic(self):
        for release in fr.RELEASE_ORDER:
            if release in ("v0.1", "future"):
                continue
            self.assertIn(release, self.by_release,
                          f"nessuna epic assegnata a {release}: la riga della tabella owner "
                          f"non e' stata letta, oppure la release non ha contenuto")

    def test_v10_row_is_read(self):
        # Il caso che il regex `v0\.\d` avrebbe scartato senza dirlo.
        self.assertIn("v1.0", self.by_release)

    def test_post_v01_epics_are_assignable(self):
        known, _checkpoints, _milestones = fr.known_roadmap_refs()
        for entry in self.epics:
            self.assertIn(entry["epic"], known,
                          f"{entry['epic']} e' dichiarata dall'owner ma il validator la "
                          f"rifiuterebbe in `roadmap.epic`")


class ReleaseViewIsFilteredByRelease(unittest.TestCase):
    """La tabella §2.2 e' la vista della v0.1, e una feature di un'altra release non ci entra."""

    @staticmethod
    def _feature(feature_id, release, epic):
        return {
            "feature_id": feature_id, "title": "t", "area": "A", "release": release,
            "status": "INTEGRATED", "gates_done": 1, "gates_applicable": 2,
            "roadmap": {"epic": epic}, "completed_by": [],
        }

    def test_v01_feature_on_a_v01_epic_is_listed(self):
        block = fr.render_features_by_epic(
            {"features": [self._feature("RT-FEAT-X-A", "v0.1", "E12")]})
        self.assertIn("RT-FEAT-X-A", block)
        self.assertNotIn("appartiene a un'altra release", block)

    def test_feature_whose_epic_is_another_release_goes_to_the_mismatch_table(self):
        # La mutazione e' esattamente `RT-FEAT-REPLAY-ARCHIVE`: release v0.2, epic E12 (v0.1).
        block = fr.render_features_by_epic(
            {"features": [self._feature("RT-FEAT-X-B", "v0.2", "E12")]})
        self.assertIn("appartiene a un'altra release", block,
                      "la contraddizione non e' stata dichiarata")
        head, _, tail = block.partition("appartiene a un'altra release")
        self.assertNotIn("RT-FEAT-X-B", head,
                         "la feature v0.2 compare comunque nella tabella della v0.1")
        self.assertIn("RT-FEAT-X-B", tail)

    def test_feature_in_a_planned_release_without_epic_is_reported(self):
        block = fr.render_features_by_epic(
            {"features": [self._feature("RT-FEAT-X-C", "v0.3", None)]})
        self.assertIn("RT-FEAT-X-C", block)
        self.assertIn("senza epic", block)

    def test_future_without_epic_is_not_reported(self):
        # In `future` un'epic non e' mancante: e' priva di senso, non c'e' release che la contenga.
        block = fr.render_features_by_epic(
            {"features": [self._feature("RT-FEAT-X-D", "future", None)]})
        self.assertNotIn("RT-FEAT-X-D", block)


if __name__ == "__main__":
    unittest.main(verbosity=2)
