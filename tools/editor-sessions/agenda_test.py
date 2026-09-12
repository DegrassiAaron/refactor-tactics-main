# tools/editor-sessions/agenda_test.py
"""Il calcolo raggruppa per allestimento, scarta i verdi, e non fa sparire nessuno."""
from __future__ import annotations

import unittest

import agenda
import oracles
from registry import Setup, Wire

SETUPS = {
    "SET-HEX-MATCH": Setup(id="SET-HEX-MATCH", extends=None, fields={"map": "L_HexArena"}),
    "SET-HEX-TURN": Setup(id="SET-HEX-TURN", extends="SET-HEX-MATCH", fields={}),
    "SET-FRONTEND": Setup(id="SET-FRONTEND", extends=None, fields={}),
}

STATO = {
    "PIE-A": {"stato": "⏳", "release": False},
    "PIE-B": {"stato": "⏳", "release": False},
    "PIE-VERDE": {"stato": "✅", "release": True},
    "PIE-REL": {"stato": "🟡", "release": True},
    "PIE-BLOCCATA": {"stato": "⏳", "release": False},
    "PIE-SCOPERTA": {"stato": "⏳", "release": False},
}


def valuta(req: str) -> oracles.Esito:
    if req == "asset:MANCANTE":
        return oracles.Esito(False, "MANCANTE non esiste in Content/")
    return oracles.Esito(True, f"{req} ok")


WIRES = [
    Wire(check="PIE-A", setup="SET-HEX-MATCH"),
    Wire(check="PIE-B", setup="SET-HEX-TURN"),
    Wire(check="PIE-VERDE", setup="SET-HEX-MATCH"),
    Wire(check="PIE-REL", setup="SET-FRONTEND"),
    Wire(check="PIE-BLOCCATA", setup="SET-FRONTEND", requires=("asset:MANCANTE",)),
]


class CalcoloTest(unittest.TestCase):
    def setUp(self):
        self.gruppi, self.scoperta = agenda.calcola(SETUPS, WIRES, STATO, valuta)
        self.per_id = {g.setup: g for g in self.gruppi}

    def test_i_verdi_escono_dal_calcolo(self):
        tutti = [v.check for g in self.gruppi for v in g.liberi + g.bloccati]
        self.assertNotIn("PIE-VERDE", tutti)

    def test_extends_cade_nella_stessa_apertura(self):
        self.assertEqual(sorted(self.per_id), ["SET-FRONTEND", "SET-HEX-MATCH"])
        self.assertEqual(
            sorted(v.check for v in self.per_id["SET-HEX-MATCH"].liberi), ["PIE-A", "PIE-B"]
        )

    def test_un_bloccato_resta_stampato_col_motivo(self):
        g = self.per_id["SET-FRONTEND"]
        self.assertEqual([v.check for v in g.bloccati], ["PIE-BLOCCATA"])
        self.assertIn("MANCANTE", g.bloccati[0].bloccanti[0])
        self.assertEqual([v.check for v in g.liberi], ["PIE-REL"])

    def test_release_va_per_prima(self):
        self.assertEqual(self.gruppi[0].setup, "SET-FRONTEND")

    def test_la_coda_scoperta_elenca_chi_nessuno_cabla(self):
        self.assertEqual(self.scoperta, ["PIE-SCOPERTA"])

    def test_la_coda_scoperta_ignora_i_verdi(self):
        stato = dict(STATO, **{"PIE-SCOPERTA": {"stato": "✅", "release": False}})
        _, scoperta = agenda.calcola(SETUPS, WIRES, stato, valuta)
        self.assertEqual(scoperta, [])

    def test_un_wire_che_cabla_un_check_sconosciuto_viene_rifiutato(self):
        wires = WIRES + [Wire(check="PIE-FANTASMA", setup="SET-FRONTEND")]
        with self.assertRaises(agenda.AgendaError) as e:
            agenda.calcola(SETUPS, wires, STATO, valuta)
        self.assertIn("PIE-FANTASMA", str(e.exception))

    def test_l_ordine_e_deterministico(self):
        a, _ = agenda.calcola(SETUPS, WIRES, STATO, valuta)
        b, _ = agenda.calcola(SETUPS, list(reversed(WIRES)), STATO, valuta)
        self.assertEqual([g.setup for g in a], [g.setup for g in b])
        self.assertEqual(
            [[v.check for v in g.liberi] for g in a],
            [[v.check for v in g.liberi] for g in b],
        )


if __name__ == "__main__":
    unittest.main()
