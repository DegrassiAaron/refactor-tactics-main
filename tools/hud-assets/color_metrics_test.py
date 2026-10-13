#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Test di `color_metrics.py`: la lettura dei colori del world overlay da `RTHUD.cpp` (#3626).

    python -m pytest -q tools/hud-assets

Perche' esistono. Il referto legge due righe di `UI/RTHUD.cpp` per FORMA, e il 2026-09-04 `#2184` ne ha
riscritta una: da allora `color_metrics.py` usciva con un errore esplicito, e nessuno lo lanciava. Un
errore forte in uno script che nessuno esegue e' un errore silenzioso.

Che cosa provano, e in quale verso:

  * la forma ATTUALE si legge, coi colori giusti;
  * la forma VECCHIA e una forma INVENTATA danno un errore esplicito, non un insieme vuoto;
  * il sorgente VERO si legge: e' il test che sarebbe diventato rosso il 2026-09-04.

Che cosa NON provano: che i colori siano distinguibili. Quello lo misura il referto stesso
(`check_overlay`), e l'ultimo test qui sotto verifica solo che arrivi in fondo.
"""

import os
import subprocess
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import color_metrics as cm  # noqa: E402

DASH = "\tconst FLinearColor DashColor = FLinearColor::FromSRGBColor(FColor(0, 158, 115));\n"

# La forma che `RTHUD.cpp` porta oggi, coi commenti di riga dove li porta il sorgente.
FORMA_ATTUALE = (
    "\tOut.Color = bOwn\n"
    "\t\t? FLinearColor(0.2f, 0.9f, 1.f, 1.f)   // ciano: le tue unita'\n"
    "\t\t: FLinearColor(1.f, 0.9f, 0.2f, 1.f);  // giallo: nemico rivelato\n"
) + DASH

# La forma di prima di `#2184`. Stessi colori: cio' che deve far fallire e' la FORMA, non i valori.
FORMA_VECCHIA = (
    "\tconst FLinearColor Color = bOwn\n"
    "\t\t? FLinearColor(0.2f, 0.9f, 1.f, 1.f)\n"
    "\t\t: FLinearColor(1.f, 0.9f, 0.2f, 1.f);\n"
) + DASH

# Una forma che il sorgente non ha mai avuto: il colore dell'identita' c'e', sotto un altro nome.
FORMA_INVENTATA = (
    "\tIntent.Tint = bMine\n"
    "\t\t? FLinearColor(0.2f, 0.9f, 1.f, 1.f)\n"
    "\t\t: FLinearColor(1.f, 0.9f, 0.2f, 1.f);\n"
) + DASH


class LaFormaAttualeSiLegge(unittest.TestCase):
    def test_identita_e_fase(self):
        identity, phase = cm.parse_overlay_colors(FORMA_ATTUALE)
        # `FLinearColor` e' LINEARE: il ciano di squadra e' `#7CF3FF`, non `#33E6FF`.
        self.assertEqual(identity, {"proprie": "#7CF3FF", "nemico": "#FFF37C"})
        self.assertEqual(phase, {"Dash": "#009E73"})


class UnaFormaSconosciutaFallisceForte(unittest.TestCase):
    def test_forma_vecchia(self):
        with self.assertRaises(RuntimeError) as caught:
            cm.parse_overlay_colors(FORMA_VECCHIA)
        # Il messaggio nomina la forma ATTESA: e' cio' che dice a chi legge dove guardare.
        self.assertIn("Out.Color = bOwn", str(caught.exception))

    def test_forma_inventata(self):
        with self.assertRaises(RuntimeError) as caught:
            cm.parse_overlay_colors(FORMA_INVENTATA)
        self.assertIn("Out.Color = bOwn", str(caught.exception))

    def test_un_colore_solo(self):
        monco = "\tOut.Color = bOwn ? FLinearColor(0.2f, 0.9f, 1.f, 1.f) : Altro;\n" + DASH
        with self.assertRaises(RuntimeError) as caught:
            cm.parse_overlay_colors(monco)
        self.assertIn("1 colori invece di 2", str(caught.exception))

    def test_due_assi_di_identita_sono_ambigui(self):
        with self.assertRaises(RuntimeError) as caught:
            cm.parse_overlay_colors(FORMA_ATTUALE + FORMA_ATTUALE)
        self.assertIn("compare 2 volte", str(caught.exception))

    def test_senza_la_linea_di_scatto(self):
        with self.assertRaises(RuntimeError) as caught:
            cm.parse_overlay_colors(FORMA_ATTUALE.replace(DASH, ""))
        self.assertIn("DashColor", str(caught.exception))


class IlSorgenteVeroSiLegge(unittest.TestCase):
    """Il test che lega il referto a `RTHUD.cpp`: cade quando la riga cambia forma."""

    def test_overlay_colors(self):
        identity, phase = cm.overlay_colors()
        self.assertEqual(sorted(identity), ["nemico", "proprie"])
        self.assertEqual(sorted(phase), ["Dash"])
        for value in list(identity.values()) + list(phase.values()):
            self.assertRegex(value, r"^#[0-9A-F]{6}$")

    def test_il_referto_arriva_in_fondo(self):
        # ⚠️ Esce 1 anche per un CRITERIO violato, non solo per una lettura fallita: se questo test cade,
        # il messaggio in coda all'output dice quale delle due cose e' successa.
        # `PYTHONIOENCODING`: il referto stampa trattini lunghi, e su una console non UTF-8 il figlio
        # cadrebbe sulla stampa invece che sul criterio — un rosso che non parla del referto.
        done = subprocess.run(
            [sys.executable, os.path.join(HERE, "color_metrics.py")],
            capture_output=True, text=True, encoding="utf-8", errors="replace", check=False,
            env=dict(os.environ, PYTHONIOENCODING="utf-8"))
        self.assertEqual(done.returncode, 0, done.stdout[-600:] + done.stderr[-600:])
        self.assertIn("World overlay", done.stdout)


if __name__ == "__main__":
    unittest.main()
