# tools/editor-sessions/registry_test.py
"""Il registro accetta i mattoni corretti e rifiuta i difetti per nome."""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import registry


def scrivi(testo: str) -> Path:
    f = tempfile.NamedTemporaryFile("w", suffix=".yaml", delete=False, encoding="utf-8")
    f.write(testo)
    f.close()
    return Path(f.name)


BUONO = """
setups:
  - id: SET-HEX-MATCH
    map: L_HexArena
  - id: SET-HEX-TURN
    extends: SET-HEX-MATCH
    given: "un turno pianificato e risolto"
wiring:
  - check: PIE-HEXPLAY-4
    setup: SET-HEX-TURN
    issue: 33
  - check: PIE-V01-LOG
    setup: SET-HEX-MATCH
    requires: [ "mount:WBP_RT_EventLogRight#2697" ]
"""


class LetturaTest(unittest.TestCase):
    def test_legge_setup_e_wiring(self):
        setups, wires = registry.load(scrivi(BUONO))
        self.assertEqual(sorted(setups), ["SET-HEX-MATCH", "SET-HEX-TURN"])
        self.assertEqual(setups["SET-HEX-TURN"].extends, "SET-HEX-MATCH")
        self.assertEqual(setups["SET-HEX-MATCH"].fields["map"], "L_HexArena")
        self.assertEqual([w.check for w in wires], ["PIE-HEXPLAY-4", "PIE-V01-LOG"])
        self.assertEqual(wires[1].requires, ("mount:WBP_RT_EventLogRight#2697",))
        self.assertEqual(wires[0].issue, 33)
        self.assertIsNone(wires[1].issue)

    def test_extends_risale_alla_radice(self):
        setups, _ = registry.load(scrivi(BUONO))
        self.assertEqual(registry.root_of("SET-HEX-TURN", setups), "SET-HEX-MATCH")
        self.assertEqual(registry.root_of("SET-HEX-MATCH", setups), "SET-HEX-MATCH")


class RifiutoTest(unittest.TestCase):
    def test_setup_inesistente_nomina_il_difetto(self):
        testo = "setups:\n  - id: SET-A\nwiring:\n  - check: PIE-X\n    setup: SET-FANTASMA\n"
        with self.assertRaises(registry.RegistryError) as e:
            registry.load(scrivi(testo))
        self.assertIn("SET-FANTASMA", str(e.exception))
        self.assertIn("PIE-X", str(e.exception))

    def test_check_rivendicato_due_volte(self):
        testo = (
            "setups:\n  - id: SET-A\n  - id: SET-B\n"
            "wiring:\n  - check: PIE-X\n    setup: SET-A\n  - check: PIE-X\n    setup: SET-B\n"
        )
        with self.assertRaises(registry.RegistryError) as e:
            registry.load(scrivi(testo))
        self.assertIn("PIE-X", str(e.exception))

    def test_setup_duplicato(self):
        with self.assertRaises(registry.RegistryError):
            registry.load(scrivi("setups:\n  - id: SET-A\n  - id: SET-A\n"))

    def test_extends_verso_il_nulla(self):
        with self.assertRaises(registry.RegistryError) as e:
            registry.load(scrivi("setups:\n  - id: SET-A\n    extends: SET-NULLA\n"))
        self.assertIn("SET-NULLA", str(e.exception))

    def test_ciclo_in_extends(self):
        testo = "setups:\n  - id: SET-A\n    extends: SET-B\n  - id: SET-B\n    extends: SET-A\n"
        setups, _ = registry.load(scrivi(testo))
        with self.assertRaises(registry.RegistryError):
            registry.root_of("SET-A", setups)


if __name__ == "__main__":
    unittest.main()
