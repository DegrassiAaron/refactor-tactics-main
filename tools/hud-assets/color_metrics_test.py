#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Test di `color_metrics.py`: la lettura dei colori del world overlay da `RTHUD.cpp` (#3626).

    python -m unittest discover -s tools/hud-assets -p "*_test.py"

Solo libreria standard, come il modulo che provano.

Perche' esistono. Il referto legge due righe di `UI/RTHUD.cpp` per FORMA, e il 2026-09-04 `#2184` ne ha
riscritta una: da allora `color_metrics.py` usciva con un errore esplicito, e nessuno lo lanciava. Un
errore forte in uno script che nessuno esegue e' un errore silenzioso.

Che cosa provano, e in quale verso:

  * la forma ATTUALE si legge, coi colori giusti, in tutte le notazioni di colore che il C++ ammette;
  * la forma VECCHIA e una forma INVENTATA danno un errore esplicito, non un insieme vuoto;
  * un COMMENTO non e' codice: una riga vecchia commentata non viene misurata al posto di quella viva;
  * un ancoraggio doppio, o una seconda assegnazione al campo dell'identita', sono errori;
  * il referto distingue «criterio violato» da «lettura fallita» col codice d'uscita, anche in `--json`;
  * il sorgente VERO si legge: e' il test che sarebbe diventato rosso il 2026-09-04.

Che cosa NON provano: che i colori siano distinguibili. Quello lo misura il referto stesso
(`check_overlay`), e l'ultimo test qui sotto verifica solo che arrivi in fondo.
"""

import contextlib
import io
import json
import os
import pathlib
import subprocess
import sys
import tempfile
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

IDENTITA_ATTESA = {"proprie": "#7CF3FF", "nemico": "#FFF37C"}


class LaFormaAttualeSiLegge(unittest.TestCase):
    def test_identita_e_fase(self):
        identity, phase = cm.parse_overlay_colors(FORMA_ATTUALE)
        # `FLinearColor` e' LINEARE: il ciano di squadra e' `#7CF3FF`, non `#33E6FF`.
        self.assertEqual(identity, IDENTITA_ATTESA)
        self.assertEqual(phase, {"Dash": "#009E73"})

    def test_i_due_rami_in_notazione_srgb(self):
        # La notazione che `RTHUD.cpp` usa per `DashColor`. I `::` non sono il `:` del ternario.
        testo = (
            "\tOut.Color = bOwn\n"
            "\t\t? FLinearColor::FromSRGBColor(FColor(124, 243, 255))\n"
            "\t\t: FLinearColor::FromSRGBColor(FColor(255, 243, 124));\n"
        ) + DASH
        identity, _ = cm.parse_overlay_colors(testo)
        self.assertEqual(identity, IDENTITA_ATTESA)

    def test_il_costruttore_da_fcolor_e_srgb_non_lineare(self):
        # `FLinearColor(FColor(...))` converte come `FromSRGBColor`. Letta come tre float lineari
        # darebbe bianco, senza errore.
        testo = (
            "\tOut.Color = bOwn ? FLinearColor(FColor(124, 243, 255)) : FLinearColor(FColor(255, 243, 124));\n"
            "\tconst FLinearColor DashColor(FColor(0, 158, 115));\n"
        )
        identity, phase = cm.parse_overlay_colors(testo)
        self.assertEqual(identity, IDENTITA_ATTESA)
        self.assertEqual(phase, {"Dash": "#009E73"})


class UnCommentoNonECodice(unittest.TestCase):
    def test_la_riga_viva_vince_su_quella_commentata(self):
        # La dichiarazione vecchia e' rimasta in un commento SOPRA quella viva, che porta il magenta di
        # prima di D-234. Si deve misurare il magenta, non il verde del commento.
        testo = FORMA_ATTUALE.replace(
            DASH,
            "\t// const FLinearColor DashColor = FLinearColor::FromSRGBColor(FColor(0, 158, 115));\n"
            "\tconst FLinearColor DashColor = FLinearColor(1.f, 0.2f, 0.9f, 1.f);\n")
        _, phase = cm.parse_overlay_colors(testo)
        self.assertNotEqual(phase["Dash"], "#009E73")

    def test_citare_la_riga_in_un_commento_non_la_sdoppia(self):
        testo = (
            "\t// l'ancoraggio e' `Out.Color = bOwn ? FLinearColor(1.f, 1.f, 1.f) : FLinearColor(0.f, 0.f, 0.f);`\n"
            "\t/* e anche `const FLinearColor DashColor = FLinearColor(1.f, 1.f, 1.f, 1.f);` */\n"
        ) + FORMA_ATTUALE
        identity, phase = cm.parse_overlay_colors(testo)
        self.assertEqual(identity, IDENTITA_ATTESA)
        self.assertEqual(phase, {"Dash": "#009E73"})

    def test_una_forma_cambiata_non_si_nasconde_dietro_il_commento(self):
        # Il codice vivo ha cambiato forma, e quella vecchia e' rimasta commentata: e' un errore, non
        # una lettura riuscita dei colori del commento.
        testo = (
            "\t// Out.Color = bOwn ? FLinearColor(0.2f, 0.9f, 1.f, 1.f) : FLinearColor(1.f, 0.9f, 0.2f, 1.f);\n"
            "\tOut.Color = TintFor(bOwn, bIsSelected);\n"
        ) + DASH
        with self.assertRaises(RuntimeError) as caught:
            cm.parse_overlay_colors(testo)
        self.assertIn("non trovato", str(caught.exception))


class UnaFormaSconosciutaFallisceForte(unittest.TestCase):
    def assert_errore(self, testo: str, frammento: str):
        with self.assertRaises(RuntimeError) as caught:
            cm.parse_overlay_colors(testo)
        self.assertIn(frammento, str(caught.exception))

    def test_forma_vecchia(self):
        # Il messaggio nomina la forma ATTESA: e' cio' che dice a chi legge dove guardare.
        self.assert_errore(FORMA_VECCHIA, "Out.Color = bOwn")

    def test_forma_inventata(self):
        self.assert_errore(FORMA_INVENTATA, "Out.Color = bOwn")

    def test_un_ramo_che_non_e_un_colore(self):
        monco = "\tOut.Color = bOwn ? FLinearColor(0.2f, 0.9f, 1.f, 1.f) : Altro;\n" + DASH
        self.assert_errore(monco, "colore non riconosciuto")

    def test_non_un_ternario(self):
        self.assert_errore("\tOut.Color = bOwn;\n" + DASH, "ternario")

    def test_due_assi_di_identita_sono_ambigui(self):
        self.assert_errore(FORMA_ATTUALE + FORMA_ATTUALE, "compare 2 volte")

    def test_una_seconda_assegnazione_al_campo(self):
        # Una terza tinta scritta da un'altra istruzione non sta nel ternario, e non verrebbe misurata.
        terza = "\tif (View.bDashing) { Out.Color = FLinearColor(1.f, 0.4f, 0.9f, 1.f); }\n"
        self.assert_errore(FORMA_ATTUALE + terza, "assegnato 2 volte")

    def test_un_confronto_non_e_una_assegnazione(self):
        confronto = "\tif (Out.Color == Altro.Color) { return; }\n"
        identity, _ = cm.parse_overlay_colors(FORMA_ATTUALE + confronto)
        self.assertEqual(identity, IDENTITA_ATTESA)

    def test_senza_la_linea_di_scatto(self):
        self.assert_errore(FORMA_ATTUALE.replace(DASH, ""), "DashColor")

    def test_due_linee_di_scatto_sono_ambigue(self):
        self.assert_errore(FORMA_ATTUALE + DASH, "DashColor` compare 2 volte")


class IlRefertoDistingueIDueRossi(unittest.TestCase):
    """`main()` su un albero finto: il codice d'uscita dice SE ha fallito la lettura o un criterio."""

    def esegui(self, rthud: str, argv: list[str]) -> tuple[int, str, str]:
        with tempfile.TemporaryDirectory() as tmp:
            file = pathlib.Path(tmp) / "Source" / "RefactorTactics" / "UI" / "RTHUD.cpp"
            file.parent.mkdir(parents=True)
            file.write_text(rthud, encoding="utf-8")
            out, err = io.StringIO(), io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                code = cm.main(argv, repo_root=pathlib.Path(tmp))
            return code, out.getvalue(), err.getvalue()

    def test_albero_sano(self):
        code, out, _ = self.esegui(FORMA_ATTUALE, [])
        self.assertEqual(code, cm.EXIT_OK, out[-400:])
        self.assertIn("World overlay", out)

    def test_lettura_fallita(self):
        code, out, err = self.esegui(FORMA_VECCHIA, [])
        self.assertEqual(code, cm.EXIT_READ_FAILED)
        self.assertIn("LETTURA FALLITA", err)
        self.assertNotIn("World overlay", out)

    def test_criterio_violato(self):
        # Il colore di fase coincide con l'identita' delle proprie unita': distanza zero.
        testo = FORMA_ATTUALE.replace(DASH, "\tconst FLinearColor DashColor = FLinearColor(0.2f, 0.9f, 1.f, 1.f);\n")
        code, out, _ = self.esegui(testo, [])
        self.assertEqual(code, cm.EXIT_CRITERIA_FAILED)
        self.assertIn("criteri non rispettati", out)

    def test_json_porta_l_overlay(self):
        code, out, _ = self.esegui(FORMA_ATTUALE, ["--json"])
        self.assertEqual(code, cm.EXIT_OK)
        overlay = json.loads(out)["overlay"]
        self.assertEqual(overlay["identita"], IDENTITA_ATTESA)
        self.assertEqual(overlay["fase"], {"Dash": "#009E73"})

    def test_json_non_nasconde_una_lettura_fallita(self):
        # Fino a `#3626` `--json` usciva 0 senza aprire `RTHUD.cpp`.
        code, _, err = self.esegui(FORMA_VECCHIA, ["--json"])
        self.assertEqual(code, cm.EXIT_READ_FAILED)
        self.assertIn("LETTURA FALLITA", err)


class IlSorgenteVeroSiLegge(unittest.TestCase):
    """I due test che legano il referto all'albero vero: cadono quando una riga di `RTHUD.cpp` cambia forma."""

    def test_overlay_colors(self):
        identity, phase = cm.overlay_colors()
        self.assertEqual(sorted(identity), ["nemico", "proprie"])
        self.assertEqual(sorted(phase), ["Dash"])
        for value in list(identity.values()) + list(phase.values()):
            self.assertRegex(value, r"^#[0-9A-F]{6}$")

    def test_il_referto_arriva_in_fondo(self):
        # `PYTHONIOENCODING`: il referto stampa trattini lunghi, e su una console non UTF-8 il figlio
        # cadrebbe sulla stampa invece che sul criterio — un rosso che non parla del referto.
        done = subprocess.run(
            [sys.executable, os.path.join(HERE, "color_metrics.py")],
            capture_output=True, text=True, encoding="utf-8", errors="replace", check=False,
            env=dict(os.environ, PYTHONIOENCODING="utf-8"))
        # Il codice dice quale dei due rossi e': 1 un criterio violato, 2 una lettura fallita.
        self.assertEqual(done.returncode, cm.EXIT_OK, done.stdout[-600:] + done.stderr[-600:])
        self.assertIn("World overlay", done.stdout)


if __name__ == "__main__":
    unittest.main()
