#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Test di `color_metrics.py`: la lettura dei colori del world overlay da `RTHUD.cpp` (#3626), e il confronto
delle tabelle di colore del generatore coi documenti che ne sono owner (#3627).

    python -m unittest discover -s tools/hud-assets -p "*_test.py"

Solo libreria standard, come il modulo che provano.

Perche' esistono. Il referto legge due righe di `UI/RTHUD.cpp` per FORMA, e il 2026-09-04 `#2184` ne ha
riscritta una: da allora `color_metrics.py` usciva con un errore esplicito, e nessuno lo lanciava. Un
errore forte in uno script che nessuno esegue e' un errore silenzioso.

Che cosa provano, e in quale verso:

  * la forma ATTUALE si legge, coi colori giusti, in tutte le notazioni di colore che il referto ammette;
  * la forma VECCHIA e una forma INVENTATA danno un errore esplicito, non un insieme vuoto;
  * un COMMENTO non e' codice, e nemmeno il contenuto di una stringa;
  * cio' che non e' un LETTERALE di colore — un prodotto, un ternario annidato — e' un errore, non una
    lettura del primo colore che capita;
  * una seconda scrittura del campo dell'identita' nella STESSA funzione e' un errore; in un'altra no;
  * i tre criteri: provenienza, fase contro identita', identita' contro identita';
  * il referto distingue «criterio violato» da «lettura fallita» col codice d'uscita, anche in `--json`,
    e una lettura fallita non si porta via la meta' che non dipende da `RTHUD.cpp`;
  * il sorgente VERO si legge: e' il test che sarebbe diventato rosso il 2026-09-04;
  * le tabelle del generatore coincidono coi token della SEZIONE owner di ciascun documento, e una riga
    omonima fuori dalla sezione non conta;
  * una riga di token senza un esadecimale leggibile e' una lettura fallita, non un token che manca;
  * un documento che non si legge non toglie il giudizio sull'altro;
  * i controlli hanno un CHIAMANTE: una tabella ritoccata in memoria fa cadere il referto e i gate del
    generatore, `T9` compreso, per ciascuno dei due grigi spenti.

Che cosa NON provano: che i colori di oggi siano belli, o giusti. Provano che il referto li legga e li
giudichi.
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
from unittest import mock

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import color_metrics as cm  # noqa: E402
import generate_hud_assets as g  # noqa: E402

IDENTITA = "bOwn\n\t\t? FLinearColor(0.2f, 0.9f, 1.f, 1.f)   // ciano: le tue unita'\n" \
           "\t\t: FLinearColor(1.f, 0.9f, 0.2f, 1.f);  // giallo: nemico rivelato\n"
ASSEGNA = "\tOut.Color = " + IDENTITA
DASH = "\tconst FLinearColor DashColor = FLinearColor::FromSRGBColor(FColor(0, 158, 115));\n"
IDENTITA_ATTESA = {"proprie": "#7CF3FF", "nemico": "#FFF37C"}


def rthud(identita: str = ASSEGNA, dash: str = DASH, altrove: str = "") -> str:
    """Un `RTHUD.cpp` ridotto all'osso: la funzione dello scatto, quella dell'identita', e il resto."""
    return (
        "void ARTHUD::DrawHUD()\n{\n" + dash + "}\n\n"
        "FRTIntentPresentation ARTHUD::ComposeIntentPresentation(const FRTIntentView& View,\n"
        "\tbool bOwn, bool bIsSelected)\n{\n\tFRTIntentPresentation Out;\n" + identita + "\treturn Out;\n}\n\n"
        + altrove)


class LaFormaAttualeSiLegge(unittest.TestCase):
    def test_identita_e_fase(self):
        identity, phase = cm.parse_overlay_colors(rthud())
        # `FLinearColor` e' LINEARE: il ciano di squadra e' `#7CF3FF`, non `#33E6FF`.
        self.assertEqual(identity, IDENTITA_ATTESA)
        self.assertEqual(phase, {"Dash": "#009E73"})

    def test_i_due_rami_in_notazione_srgb(self):
        # La notazione che `RTHUD.cpp` usa per `DashColor`. I `::` non sono il `:` del ternario.
        identita = ("\tOut.Color = bOwn\n"
                    "\t\t? FLinearColor::FromSRGBColor(FColor(124, 243, 255))\n"
                    "\t\t: FLinearColor::FromSRGBColor(FColor(255, 243, 124));\n")
        identity, _ = cm.parse_overlay_colors(rthud(identita))
        self.assertEqual(identity, IDENTITA_ATTESA)

    def test_il_costruttore_da_fcolor_e_srgb_non_lineare(self):
        # `FLinearColor(FColor(...))` converte come `FromSRGBColor`. Letta come tre float lineari
        # darebbe bianco, senza errore.
        identita = "\tOut.Color = bOwn ? FLinearColor(FColor(124, 243, 255)) : FColor(255, 243, 124);\n"
        identity, phase = cm.parse_overlay_colors(
            rthud(identita, "\tconst FLinearColor DashColor(FColor(0, 158, 115));\n"))
        self.assertEqual(identity, IDENTITA_ATTESA)
        self.assertEqual(phase, {"Dash": "#009E73"})

    def test_la_linea_di_scatto_col_costruttore_lineare(self):
        _, phase = cm.parse_overlay_colors(rthud(dash="\tconst FLinearColor DashColor(0.f, 0.f, 0.f, 1.f);\n"))
        self.assertEqual(phase, {"Dash": "#000000"})


class UnCommentoNonECodice(unittest.TestCase):
    def test_la_riga_viva_vince_su_quella_commentata(self):
        # La dichiarazione vecchia e' rimasta in un commento SOPRA quella viva, che porta il magenta di
        # prima di D-234. Si deve misurare il magenta, non il verde del commento.
        dash = ("\t// const FLinearColor DashColor = FLinearColor::FromSRGBColor(FColor(0, 158, 115));\n"
                "\tconst FLinearColor DashColor = FLinearColor(1.f, 0.2f, 0.9f, 1.f);\n")
        _, phase = cm.parse_overlay_colors(rthud(dash=dash))
        self.assertNotEqual(phase["Dash"], "#009E73")

    def test_citare_la_riga_in_un_commento_non_la_sdoppia(self):
        identita = ("\t// l'ancoraggio e' `Out.Color = bOwn ? FLinearColor(1.f, 1.f, 1.f) : FLinearColor(0.f, 0.f, 0.f);`\n"
                    "\t/* e anche `Out.Color = FLinearColor(1.f, 1.f, 1.f, 1.f);` */\n") + ASSEGNA
        identity, _ = cm.parse_overlay_colors(rthud(identita))
        self.assertEqual(identity, IDENTITA_ATTESA)

    def test_una_forma_cambiata_non_si_nasconde_dietro_il_commento(self):
        identita = ("\t// Out.Color = bOwn ? FLinearColor(0.2f, 0.9f, 1.f, 1.f) : FLinearColor(1.f, 0.9f, 0.2f, 1.f);\n"
                    "\tOut.Color = TintFor(bOwn, bIsSelected);\n")
        with self.assertRaises(RuntimeError) as caught:
            cm.parse_overlay_colors(rthud(identita))
        self.assertIn("non trovato", str(caught.exception))

    def test_un_blocco_aperto_dentro_un_commento_di_riga_non_inghiotte_il_codice(self):
        # `// … Content/RT/UI/*` non apre un commento a blocco. Con due sostituzioni in fila lo aprirebbe,
        # e il codice vivo fino al prossimo `*/` sparirebbe: qui e' la seconda assegnazione a sparire.
        identita = ("\t// vedi Content/RT/UI/* per gli asset\n" + ASSEGNA
                    + "\tOut.Color = FLinearColor(1.f, 0.f, 1.f, 1.f);\n\tFoo(/*bFilled*/ true);\n")
        with self.assertRaises(RuntimeError) as caught:
            cm.parse_overlay_colors(rthud(identita))
        self.assertIn("scritto 2 volte", str(caught.exception))

    def test_una_stringa_con_due_barre_non_taglia_la_riga(self):
        dash = '\tLog(TEXT("http://x")); const FLinearColor DashColor = FLinearColor::FromSRGBColor(FColor(0, 158, 115));\n'
        _, phase = cm.parse_overlay_colors(rthud(dash=dash))
        self.assertEqual(phase, {"Dash": "#009E73"})

    def test_un_ancoraggio_dentro_una_stringa_non_e_codice(self):
        altrove = 'void ARTHUD::Nota()\n{\n\tLog(TEXT("const FLinearColor DashColor = FLinearColor(1.f, 1.f, 1.f, 1.f);"));\n}\n'
        _, phase = cm.parse_overlay_colors(rthud(altrove=altrove))
        self.assertEqual(phase, {"Dash": "#009E73"})


class UnaFormaSconosciutaFallisceForte(unittest.TestCase):
    def assert_errore(self, testo: str, frammento: str):
        with self.assertRaises(RuntimeError) as caught:
            cm.parse_overlay_colors(testo)
        self.assertIn(frammento, str(caught.exception))

    def test_forma_vecchia(self):
        # Il messaggio nomina la forma ATTESA: e' cio' che dice a chi legge dove guardare.
        self.assert_errore(rthud("\tconst FLinearColor Color = " + IDENTITA), "Out.Color = bOwn")

    def test_forma_inventata(self):
        self.assert_errore(rthud("\tIntent.Tint = bMine ? FLinearColor(0.2f, 0.9f, 1.f, 1.f) : FLinearColor(1.f, 0.9f, 0.2f, 1.f);\n"),
                           "Out.Color = bOwn")

    def test_la_funzione_dell_identita_non_c_e_piu(self):
        testo = rthud().replace("ARTHUD::ComposeIntentPresentation", "ARTHUD::ComposeIntentLook")
        self.assert_errore(testo, "ComposeIntentPresentation` compare 0 volte")

    def test_un_ramo_che_non_e_un_colore(self):
        self.assert_errore(rthud("\tOut.Color = bOwn ? FLinearColor(0.2f, 0.9f, 1.f, 1.f) : Altro;\n"),
                           "colore non riconosciuto")

    def test_non_un_ternario(self):
        self.assert_errore(rthud("\tOut.Color = bOwn;\n"), "ternario")

    def test_una_terza_tinta_in_un_ternario_annidato(self):
        # Fra parentesi o no: in entrambi i casi un ramo non e' un letterale, e la terza tinta non si
        # misurerebbe. Prima di questo test la lettura passava in silenzio.
        a, b, c = "FLinearColor(0.2f, 0.9f, 1.f, 1.f)", "FLinearColor(1.f, 0.9f, 0.2f, 1.f)", "FLinearColor(1.f, 0.f, 1.f, 1.f)"
        for corpo in (f"bOwn ? {a} : (bRevealed ? {b} : {c})", f"bOwn ? {a} : bRevealed ? {b} : {c}",
                      f"bOwn ? (bIsSelected ? {c} : {a}) : {b}"):
            with self.subTest(corpo=corpo):
                self.assert_errore(rthud(f"\tOut.Color = {corpo};\n"), "colore non riconosciuto")

    def test_un_colore_calcolato_non_e_un_letterale(self):
        a = "FLinearColor(0.2f, 0.9f, 1.f, 1.f)"
        b = "FLinearColor(1.f, 0.9f, 0.2f, 1.f)"
        for ramo in (f"{a} * 0.25f", f"{a}.Desaturate(0.8f)", f"FMath::Lerp({a}, {b}, 0.5f)",
                     "PhaseTint(0.f, 0.62f, 0.45f)", "FColor(0, 158, 115).ReinterpretAsLinear()"):
            with self.subTest(ramo=ramo):
                self.assert_errore(rthud(f"\tOut.Color = bOwn ? {ramo} : {b};\n"), "colore non riconosciuto")

    def test_due_assi_di_identita_sono_ambigui(self):
        self.assert_errore(rthud(ASSEGNA + ASSEGNA), "compare 2 volte")

    def test_una_seconda_scrittura_del_campo(self):
        # Una terza tinta scritta da un'altra istruzione non sta nel ternario, e non verrebbe misurata.
        for seconda in ("\tif (View.bDashing) { Out.Color = FLinearColor(1.f, 0.4f, 0.9f, 1.f); }\n",
                        "\tOut.Color *= 0.5f;\n", "\tOut.Color.R = 1.f;\n"):
            with self.subTest(seconda=seconda.strip()):
                self.assert_errore(rthud(ASSEGNA + seconda), "scritto 2 volte")

    def test_un_confronto_non_e_una_scrittura(self):
        identity, _ = cm.parse_overlay_colors(rthud(ASSEGNA + "\tif (Out.Color == Altro.Color) { return Out; }\n"))
        self.assertEqual(identity, IDENTITA_ATTESA)

    def test_un_campo_omonimo_in_un_altra_funzione_non_conta(self):
        # In `RTHUD.cpp` piu' funzioni hanno un locale che si chiama `Out`.
        altrove = "FRTChip ARTHUD::ComposeChip()\n{\n\tFRTChip Out;\n\tOut.Color = FLinearColor::White;\n\treturn Out;\n}\n"
        identity, _ = cm.parse_overlay_colors(rthud(altrove=altrove))
        self.assertEqual(identity, IDENTITA_ATTESA)

    def test_senza_la_linea_di_scatto(self):
        self.assert_errore(rthud(dash=""), "DashColor` non trovato")

    def test_due_linee_di_scatto_sono_ambigue(self):
        self.assert_errore(rthud(dash=DASH + DASH), "DashColor` compare 2 volte")

    def test_una_linea_di_scatto_che_sceglie_fra_due_colori(self):
        # Lo scenario di D-234: un colore che entra fuori da ogni palette, in un ramo. Prima si misurava
        # il primo `FColor` trovato, cioe' il ramo buono.
        dash = ("\tconst FLinearColor DashColor = bOwn ? FLinearColor(1.f, 0.2f, 0.9f, 1.f)"
                " : FLinearColor::FromSRGBColor(FColor(0, 158, 115));\n")
        self.assert_errore(rthud(dash=dash), "colore non riconosciuto")


class ITreCriteri(unittest.TestCase):
    """`check_overlay`, coi colori passati a mano: nessuna lettura del C++."""

    FASI = {"Prep": "#56B4E9", "Dash": "#009E73", "Blast": "#D55E00", "Move": "#0072B2"}

    def errori(self, identity: dict, phase: dict) -> list[str]:
        known = cm.inks_by_owner(self.FASI, {"Violet": "#7C5CFF", "Red": "#FF4D4D"}, {"Disabled": "#6B7280"})
        return cm.check_overlay(identity, phase, known, self.FASI)

    def test_i_colori_di_oggi_passano(self):
        self.assertEqual(self.errori(IDENTITA_ATTESA, {"Dash": "#009E73"}), [])

    def test_la_fase_porta_il_proprio_inchiostro_non_uno_qualunque(self):
        # Un altro colore DICHIARATO — un'altra fase, uno stato, il grigio — non e' il colore di `Dash`.
        for altro in ("#D55E00", "#0072B2", "#56B4E9", "#7C5CFF", "#FF4D4D", "#6B7280"):
            with self.subTest(altro=altro):
                errori = self.errori(IDENTITA_ATTESA, {"Dash": altro})
                self.assertTrue(any("non e' l'inchiostro" in e for e in errori), errori)

    def test_fase_e_identita_non_si_confondono(self):
        errori = self.errori({"proprie": "#00A070", "nemico": "#FFF37C"}, {"Dash": "#009E73"})
        self.assertTrue(any("due ASSI" in e for e in errori), errori)

    def test_le_due_identita_non_si_confondono(self):
        errori = self.errori({"proprie": "#7CF3FF", "nemico": "#7CF3FE"}, {"Dash": "#009E73"})
        self.assertTrue(any("le identita'" in e for e in errori), errori)


def documento_ui(righe: dict, prima: str = "", dopo: str = "") -> str:
    """Un `progettazione-hud.md` ridotto alla sezione owner: `righe` e' nome del token (senza prefisso) -> HEX."""
    tabella = "".join(f"| `RT_UI_{nome}` | `{valore}` | uso |\n" for nome, valore in righe.items())
    return (prima + "# 32. Style Guide — palette\n\n## Accenti\n\n| Token | Hex | Uso |\n|---|---|---|\n"
            + tabella + "\n# 33. Style Guide — tipografia\n\n" + dopo)


def documento_sem(righe: dict, dopo: str = "") -> str:
    """Un `02-color-system.md` ridotto alla sezione owner: il token e' la SECONDA cella, come nel vero."""
    tabella = "".join(f"| {nome} | `RT_Sem_{nome}` | `{valore}` | uso |\n" for nome, valore in righe.items())
    return ("# Visual Language — Color System\n\n## 2. Palette semantica\n\n"
            "| Famiglia | Token | HEX | Uso |\n|---|---|---|---|\n" + tabella + "\n## 3. Temi\n\n" + dopo)


def documenti_fedeli() -> tuple[str, str]:
    """I due documenti owner SINTETIZZATI dalle tabelle del generatore: coincidono per costruzione.

    Non si copiano quelli veri: l'albero di `IlRefertoDistingueIDueRossi` e' finto, e deve restarlo. Con i
    documenti veri, una divergenza vera avrebbe reso rossi test che parlano d'altro, e verdi per la ragione
    sbagliata quelli che aspettano un criterio violato. Il legame con l'albero vero lo tengono
    `test_le_tabelle_vere_coincidono_coi_documenti_veri` e `IlSorgenteVeroSiLegge`, e solo loro.
    """
    ui = {**g.CHROME, **{"Phase_" + fase: valore for fase, valore in g.PHASE_INK.items()}}
    return documento_ui(ui), documento_sem(g.SEMANTIC)


class IlRefertoDistingueIDueRossi(unittest.TestCase):
    """`main()` su un albero finto: il codice d'uscita dice SE ha fallito la lettura o un criterio."""

    def esegui(self, rthud_cpp, argv: list[str], ui="fedele", sem="fedele") -> tuple[int, str, str]:
        """`ui` / `sem`: `"fedele"` = sintetizzato dalle tabelle, `None` = il file manca, altrimenti il testo."""
        fedele_ui, fedele_sem = documenti_fedeli()
        with tempfile.TemporaryDirectory() as tmp:
            if rthud_cpp is not None:
                file = pathlib.Path(tmp) / "Source" / "RefactorTactics" / "UI" / "RTHUD.cpp"
                file.parent.mkdir(parents=True)
                file.write_text(rthud_cpp, encoding="utf-8")
            for relativo, testo, fedele in ((cm.HUD_STYLE_DOC, ui, fedele_ui), (cm.SEMANTIC_DOC, sem, fedele_sem)):
                if testo is None:
                    continue
                file = pathlib.Path(tmp) / relativo
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_text(fedele if testo == "fedele" else testo, encoding="utf-8")
            out, err = io.StringIO(), io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                code = cm.main(argv, repo_root=pathlib.Path(tmp))
            return code, out.getvalue(), err.getvalue()

    VECCHIA = rthud("\tconst FLinearColor Color = " + IDENTITA)
    VIOLATA = rthud(dash="\tconst FLinearColor DashColor = FLinearColor(0.2f, 0.9f, 1.f, 1.f);\n")

    def ui_con_un_ciano_diverso(self) -> str:
        """§32 dove `RT_UI_Cyan` non vale piu' cio' che vale `CHROME["Cyan"]`."""
        ui = {**g.CHROME, **{"Phase_" + fase: valore for fase, valore in g.PHASE_INK.items()}}
        ui["Cyan"] = "#00E0FE"
        self.assertNotEqual(g.CHROME["Cyan"].upper(), ui["Cyan"], "premessa: il valore ritoccato e' diverso")
        return documento_ui(ui)

    def test_albero_sano(self):
        code, out, _ = self.esegui(rthud(), [])
        self.assertEqual(code, cm.EXIT_OK, out[-400:])
        self.assertIn("World overlay", out)
        self.assertIn("identita' fra loro", out)
        self.assertIn("coincidono con i token", out)

    def test_lettura_fallita(self):
        code, out, err = self.esegui(self.VECCHIA, [])
        self.assertEqual(code, cm.EXIT_READ_FAILED)
        self.assertIn("LETTURA FALLITA", err)
        # La meta' che non dipende da `RTHUD.cpp` si stampa lo stesso.
        self.assertIn("Palette delle macro-fasi", out)
        self.assertIn("NON LETTO", out)

    def test_il_file_che_manca_e_una_lettura_fallita_non_un_criterio(self):
        code, _, err = self.esegui(None, [])
        self.assertEqual(code, cm.EXIT_READ_FAILED)
        self.assertIn("FileNotFoundError", err)

    def test_criterio_violato(self):
        code, out, _ = self.esegui(self.VIOLATA, [])
        self.assertEqual(code, cm.EXIT_CRITERIA_FAILED)
        self.assertIn("criteri non rispettati", out)
        # Il rosso viene dall'OVERLAY, non da una tabella: i documenti di questo albero sono fedeli.
        self.assertIn("overlay:", out)
        self.assertNotIn("DIVERGONO", out)

    def test_json_sano(self):
        code, out, _ = self.esegui(rthud(), ["--json"])
        self.assertEqual(code, cm.EXIT_OK)
        report = json.loads(out)
        self.assertEqual(report["overlay"]["identita"], IDENTITA_ATTESA)
        self.assertEqual(report["overlay"]["fase"], {"Dash": "#009E73"})
        self.assertEqual(report["criteri_non_rispettati"], [])
        self.assertEqual(report["exit"], cm.EXIT_OK)

    def test_json_con_una_lettura_fallita_resta_json(self):
        # Fino a `#3626` `--json` usciva 0 senza aprire `RTHUD.cpp`. E una lettura fallita non deve
        # lasciare lo stdout vuoto: chi consuma il JSON avrebbe un errore di parsing invece del motivo.
        code, out, _ = self.esegui(self.VECCHIA, ["--json"])
        self.assertEqual(code, cm.EXIT_READ_FAILED)
        report = json.loads(out)
        self.assertIn("errore_di_lettura", report["overlay"])
        self.assertIn("fasi", report)

    def test_json_con_un_criterio_violato_non_resta_verde(self):
        code, out, _ = self.esegui(self.VIOLATA, ["--json"])
        self.assertEqual(code, cm.EXIT_CRITERIA_FAILED)
        report = json.loads(out)
        self.assertTrue(any(e.startswith("overlay:") for e in report["criteri_non_rispettati"]), report)
        self.assertEqual(report["tabelle"]["divergenze"], [])

    def test_i_documenti_owner_che_mancano_sono_una_lettura_fallita(self):
        # La terza parte del referto (`#3627`): senza i due documenti il confronto delle tabelle non si puo'
        # fare, e non e' un criterio violato. Le altre due parti si stampano lo stesso.
        code, out, err = self.esegui(rthud(), [], ui=None, sem=None)
        self.assertEqual(code, cm.EXIT_READ_FAILED)
        self.assertEqual(err.count("LETTURA FALLITA"), 2, err)
        self.assertIn("NON LETTE", out)
        self.assertIn("identita' fra loro", out)

    def test_json_porta_l_esito_delle_tabelle(self):
        code, out, _ = self.esegui(rthud(), ["--json"])
        self.assertEqual(code, cm.EXIT_OK)
        self.assertEqual(json.loads(out)["tabelle"], {"divergenze": [], "letture_fallite": []})

    # ── Il controllo delle tabelle ha un CHIAMANTE, e il suo esito arriva al codice d'uscita ──
    # `check_palette_owners` da sola e' provata piu' sotto. Qui si prova il filo che la lega al referto: una
    # funzione corretta che `main()` non chiama, o di cui scarta l'esito, e' il difetto di `#3626`.

    def test_una_tabella_che_diverge_e_un_criterio_violato(self):
        code, out, _ = self.esegui(rthud(), [], ui=self.ui_con_un_ciano_diverso())
        self.assertEqual(code, cm.EXIT_CRITERIA_FAILED, out[-600:])
        self.assertIn("DIVERGONO", out)
        self.assertIn("RT_UI_Cyan", out)
        self.assertNotIn("overlay:", out)  # il rosso viene da qui, non da un'altra parte del referto

    def test_json_con_una_tabella_che_diverge(self):
        code, out, _ = self.esegui(rthud(), ["--json"], ui=self.ui_con_un_ciano_diverso())
        self.assertEqual(code, cm.EXIT_CRITERIA_FAILED)
        report = json.loads(out)
        self.assertEqual(len(report["tabelle"]["divergenze"]), 1, report["tabelle"])
        self.assertIn("RT_UI_Cyan", report["tabelle"]["divergenze"][0])
        self.assertEqual(report["criteri_non_rispettati"], report["tabelle"]["divergenze"])
        self.assertEqual(report["exit"], cm.EXIT_CRITERIA_FAILED)

    def test_un_documento_che_manca_non_cancella_il_giudizio_sull_altro(self):
        # Il documento semantico manca, e §32 diverge: la divergenza si stampa lo stesso. Vince il `2`,
        # perche' il giudizio e' parziale; ma cio' che si e' letto e' giudicato.
        code, out, err = self.esegui(rthud(), [], ui=self.ui_con_un_ciano_diverso(), sem=None)
        self.assertEqual(code, cm.EXIT_READ_FAILED)
        self.assertEqual(err.count("LETTURA FALLITA"), 1, err)
        self.assertIn("02-color-system.md", err)
        self.assertIn("RT_UI_Cyan", out)
        self.assertIn("DIVERGONO", out)

        code, out, _ = self.esegui(rthud(), ["--json"], ui=self.ui_con_un_ciano_diverso(), sem=None)
        tabelle = json.loads(out)["tabelle"]
        self.assertEqual(code, cm.EXIT_READ_FAILED)
        self.assertEqual(len(tabelle["divergenze"]), 1, tabelle)
        self.assertEqual(len(tabelle["letture_fallite"]), 1, tabelle)


class LeTabelleSonoCopieDeiDocumenti(unittest.TestCase):
    """`check_palette_owners` e i suoi mattoni (#3627): le tabelle del generatore contro i loro owner."""

    UI_DOC = documento_ui({"Cyan": "#00E0FF", "Text_Disabled": "#6B7684", "Phase_Dash": "#009E73"})
    SEM_DOC = documento_sem({"Movement": "#009E73", "Disabled": "#6B7280"})

    def albero(self, tmp: str, ui, sem) -> pathlib.Path:
        root = pathlib.Path(tmp)
        for relativo, testo in ((cm.HUD_STYLE_DOC, ui), (cm.SEMANTIC_DOC, sem)):
            if testo is None:
                continue
            file = root / relativo
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_text(testo, encoding="utf-8")
        return root

    def esito(self, fasi: dict, chrome: dict, semantic: dict, ui="buono", sem="buono") -> tuple[list, list]:
        """`(divergenze, letture_fallite)`. `"buono"` = il documento di classe, `None` = il file manca."""
        with tempfile.TemporaryDirectory() as tmp:
            root = self.albero(tmp, self.UI_DOC if ui == "buono" else ui, self.SEM_DOC if sem == "buono" else sem)
            return cm.check_palette_owners(fasi, chrome, semantic, root)

    def errori(self, fasi: dict, chrome: dict, semantic: dict, ui="buono", sem="buono") -> list:
        """Le sole divergenze, dopo aver asserito che i due documenti si sono letti."""
        divergenze, non_lette = self.esito(fasi, chrome, semantic, ui, sem)
        self.assertEqual(non_lette, [])
        return divergenze

    def lettura_fallita(self, ui="buono", sem="buono") -> str:
        """L'unica lettura fallita di un albero in cui UN documento non si legge."""
        _, non_lette = self.esito({}, {"Cyan": "#00E0FF"}, {"Movement": "#009E73"}, ui, sem)
        self.assertEqual(len(non_lette), 1, non_lette)
        return non_lette[0]

    def test_copie_fedeli(self):
        self.assertEqual(self.errori({"Dash": "#009E73"}, {"Cyan": "#00E0FF", "Text_Disabled": "#6B7684"},
                                     {"Movement": "#009E73", "Disabled": "#6B7280"}), [])

    def test_le_maiuscole_dell_esadecimale_non_contano(self):
        self.assertEqual(self.errori({}, {"Cyan": "#00e0ff"}, {}), [])

    def test_i_due_grigi_restano_due_token(self):
        # Il caso da cui la issue e' nata: stessa parola, due vocabolari. Ciascuno coincide col PROPRIO
        # documento, e nessuno dei due e' una copia divergente dell'altro.
        self.assertEqual(self.errori({}, {"Text_Disabled": "#6B7684"}, {"Disabled": "#6B7280"}), [])
        # Allineare il grigio semantico a quello del testo, invece, e' una divergenza dal suo owner.
        errori = self.errori({}, {}, {"Disabled": "#6B7684"})
        self.assertTrue(any("RT_Sem_Disabled" in e and "divergono" in e for e in errori), errori)

    def test_un_valore_cambiato_solo_nella_copia(self):
        errori = self.errori({}, {"Cyan": "#00E0FE"}, {})
        self.assertEqual(len(errori), 1)
        self.assertIn("RT_UI_Cyan", errori[0])
        self.assertIn("divergono", errori[0])

    def test_un_colore_che_il_documento_non_dichiara(self):
        errori = self.errori({}, {"Magenta": "#FF33E6"}, {})
        self.assertEqual(len(errori), 1)
        self.assertIn("non ha il token `RT_UI_Magenta`", errori[0])

    def test_le_fasi_si_cercano_col_loro_prefisso(self):
        self.assertEqual(self.errori({"Dash": "#009E73"}, {}, {}), [])
        errori = self.errori({"Dash": "#009E74"}, {}, {})
        self.assertTrue(any("RT_UI_Phase_Dash" in e for e in errori), errori)

    def test_un_token_del_documento_senza_copia_non_e_un_errore(self):
        # Il verso e' uno solo: §32 dichiara molti token che le icone non usano.
        self.assertEqual(self.errori({}, {}, {}), [])

    # ── Si legge la SEZIONE owner, non il documento ──

    def test_una_riga_omonima_fuori_dalla_sezione_non_dichiara_il_token(self):
        # Il falso verde: `RT_UI_Cyan` esce da §32 e resta in una tabella di un'altra sezione.
        altrove = "# 34. Asset\n\n| Token | Hex | Uso |\n|---|---|---|\n| `RT_UI_Cyan` | `#00E0FF` | altrove |\n"
        for dove in ("prima", "dopo"):
            with self.subTest(dove=dove):
                ui = documento_ui({"Text_Disabled": "#6B7684"}, **{dove: altrove})
                errori = self.errori({}, {"Cyan": "#00E0FF"}, {}, ui=ui)
                self.assertEqual(len(errori), 1, errori)
                self.assertIn("non ha il token `RT_UI_Cyan`", errori[0])

    def test_una_riga_omonima_fuori_dalla_sezione_non_sdoppia_il_token(self):
        # Il falso rosso: un'altra sezione riusa il nome con un'altra tinta, e §32 e' in ordine.
        altrove = "# 34. Asset\n\n| Token | Hex | Uso |\n|---|---|---|\n| `RT_UI_Cyan` | `#112233` | altrove |\n"
        self.assertEqual(self.errori({}, {"Cyan": "#00E0FF"}, {}, ui=documento_ui({"Cyan": "#00E0FF"}, dopo=altrove)), [])
        nota = "### 3.1 Storia\n\n| Famiglia | Token | HEX |\n|---|---|---|\n| Disabled | `RT_Sem_Disabled` | `#000000` |\n"
        self.assertEqual(self.errori({}, {}, {"Disabled": "#6B7280"}, sem=documento_sem({"Disabled": "#6B7280"}, dopo=nota)), [])

    def test_una_citazione_in_prosa_non_e_una_riga_di_tabella(self):
        # Dentro la sezione, ma non in una tabella: una nota che cita il token con un altro valore.
        ui = self.UI_DOC.replace("\n# 33.", "\n> Prima del 2026 `RT_UI_Cyan` valeva `#00FFFF`.\n\n# 33.")
        self.assertEqual(self.errori({}, {"Cyan": "#00E0FF"}, {}, ui=ui), [])

    def test_senza_il_titolo_della_sezione_non_si_legge(self):
        senza = self.UI_DOC.replace("# 32. Style Guide — palette", "# Palette")
        self.assertIn("compare 0 volte", self.lettura_fallita(ui=senza))
        doppio = self.UI_DOC + "\n# 32. Style Guide — palette, di nuovo\n"
        self.assertIn("compare 2 volte", self.lettura_fallita(ui=doppio))

    # ── Una riga che non si legge e' una LETTURA FALLITA, non un token che manca ──

    def test_una_tabella_che_ha_cambiato_forma_non_si_legge(self):
        vuota = "# 32. Style Guide — palette\n\nLa tabella e' stata tolta.\n\n# 33. Tipografia\n"
        self.assertIn("nessun token letto", self.lettura_fallita(ui=vuota))

    def test_una_colonna_in_piu_fra_token_e_valore(self):
        # Il token c'e'. Con una sola espressione per nome e valore la riga spariva, e il referto diceva
        # «un colore si aggiunge prima al documento owner» di un colore che nel documento stava.
        ui = ("# 32. Style Guide — palette\n\n| Token | Sede | Hex |\n|---|---|---|\n"
              "| `RT_UI_Cyan` | runtime | `#00E0FF` |\n\n# 33. Tipografia\n")
        errore = self.lettura_fallita(ui=ui)
        self.assertIn("RT_UI_Cyan", errore)
        self.assertIn("non porta un esadecimale", errore)

    def test_un_valore_che_non_e_un_esadecimale_a_sei_cifre(self):
        for valore in ("#00E0FF", "`#00E0FFCC`", "`ciano`", ""):
            with self.subTest(valore=valore):
                ui = ("# 32. Style Guide — palette\n\n| Token | Hex | Uso |\n|---|---|---|\n"
                      f"| `RT_UI_Cyan` | {valore} | focus |\n\n# 33. Tipografia\n")
                self.assertIn("non porta un esadecimale", self.lettura_fallita(ui=ui))

    def test_un_token_dichiarato_due_volte(self):
        # Anche con lo STESSO valore: il gemello in C++ chiede che §32 elenchi un token una volta sola.
        for secondo in ("#00E0FE", "#00E0FF"):
            with self.subTest(secondo=secondo):
                doppio = self.UI_DOC.replace("\n# 33.", f"| `RT_UI_Cyan` | `{secondo}` | focus, di nuovo |\n\n# 33.")
                self.assertIn("due volte", self.lettura_fallita(ui=doppio))

    def test_una_barra_con_l_escape_non_separa_le_celle(self):
        ui = ("# 32. Style Guide — palette\n\n| Token | Hex | Uso |\n|---|---|---|\n"
              "| `RT_UI_Cyan` | `#00E0FF` | focus \\| active |\n\n# 33. Tipografia\n")
        self.assertEqual(self.errori({}, {"Cyan": "#00E0FF"}, {}, ui=ui), [])

    # ── I due documenti si leggono e si giudicano SEPARATAMENTE ──

    def test_un_documento_che_non_si_legge_non_toglie_il_giudizio_sull_altro(self):
        for manca, tabelle, atteso in (
                ("sem", ({}, {"Cyan": "#00E0FE"}, {"Movement": "#009E73"}), "RT_UI_Cyan"),
                ("ui", ({}, {"Cyan": "#00E0FF"}, {"Movement": "#009E74"}), "RT_Sem_Movement")):
            with self.subTest(manca=manca):
                divergenze, non_lette = self.esito(*tabelle, **{manca: None})
                self.assertEqual(len(non_lette), 1, non_lette)
                self.assertIn("FileNotFoundError", non_lette[0])
                self.assertEqual(len(divergenze), 1, divergenze)
                self.assertIn(atteso, divergenze[0])

    def test_un_documento_che_non_e_utf8_e_una_lettura_fallita(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.albero(tmp, self.UI_DOC, None)
            file = root / cm.SEMANTIC_DOC
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(b"## 2. Palette semantica\n\n| \xff\xfe |\n")
            divergenze, non_lette = cm.check_palette_owners({}, {"Cyan": "#00E0FF"}, {}, root)
        self.assertEqual(divergenze, [])
        self.assertEqual(len(non_lette), 1, non_lette)
        self.assertIn("UnicodeDecodeError", non_lette[0])

    def test_le_tabelle_vere_coincidono_coi_documenti_veri(self):
        # Il test che lega le copie agli owner sull'albero vero: cade quando qualcuno ritocca una tabella
        # del generatore o un documento, e non l'altro.
        self.assertEqual(cm.check_palette_owners(g.PHASE_INK, g.CHROME, g.SEMANTIC), ([], []))


class GliStatiSiDefinisconoUnaVoltaSola(unittest.TestCase):
    """`hud_state_sets`: gli stati contro cui `T9` e il referto misurano le fasi. Una definizione per due."""

    def test_i_due_grigi_spenti_ci_sono_entrambi(self):
        stati, superfici = cm.hud_state_sets(g.CHROME, g.SEMANTIC)
        self.assertEqual(stati["Text_Disabled"], g.CHROME["Text_Disabled"])
        self.assertEqual(stati["Sem_Disabled"], g.SEMANTIC["Disabled"])
        self.assertNotEqual(stati["Text_Disabled"].upper(), stati["Sem_Disabled"].upper(),
                            "premessa: sono due valori, altrimenti il test non distingue quale si misura")
        self.assertEqual(sorted(superfici), ["BG_Panel", "BG_Raised"])

    def test_il_referto_usa_quella_definizione(self):
        fasi, stati, superfici = cm._palette_sources()  # noqa: SLF001 - e' il filo che si sta provando
        self.assertEqual((stati, superfici), cm.hud_state_sets(g.CHROME, g.SEMANTIC))
        self.assertEqual(fasi, g.PHASE_INK)


class IlGeneratoreChiamaIControlli(unittest.TestCase):
    """I gate di colore del generatore, con una tabella ritoccata IN MEMORIA: il controllo ha un chiamante.

    Sono le prove che questi gate sanno fallire ([D-188]). Senza, togliere la chiamata dal generatore non
    faceva cadere nessun test: il controllo esisteva e nessuno lo eseguiva, che e' il difetto di `#3626`.
    """

    def errori(self, tabella: dict, ritocco: dict, porta=None) -> list[str]:
        porta = g.check_colour_gates if porta is None else porta
        with mock.patch.dict(tabella, ritocco):
            return porta()

    def test_l_albero_vero_passa(self):
        self.assertEqual(g.check_colour_gates(), [])

    def test_una_tabella_ritoccata_e_un_pal_rosso(self):
        for tabella, ritocco, token in ((g.CHROME, {"Cyan": "#00E0FE"}, "RT_UI_Cyan"),
                                        (g.PHASE_INK, {"Dash": "#009E74"}, "RT_UI_Phase_Dash"),
                                        (g.SEMANTIC, {"Hazard": "#E69F01"}, "RT_Sem_Hazard")):
            with self.subTest(token=token):
                errori = self.errori(tabella, ritocco)
                self.assertTrue(any(e.startswith("PAL: ") and token in e for e in errori), errori)

    def test_allineare_il_grigio_semantico_al_testo_e_un_pal_rosso(self):
        # La «correzione» che la issue sembrava chiedere, e che la decisione d'autore `P4` ha escluso.
        errori = self.errori(g.SEMANTIC, {"Disabled": g.CHROME["Text_Disabled"]})
        self.assertTrue(any(e.startswith("PAL: ") and "RT_Sem_Disabled" in e for e in errori), errori)

    def test_un_colore_non_dichiarato_e_un_pal_rosso(self):
        errori = self.errori(g.CHROME, {"Magenta": "#FF33E6"})
        self.assertTrue(any(e.startswith("PAL: ") and "non ha il token `RT_UI_Magenta`" in e for e in errori), errori)

    def test_t9_misura_le_fasi_contro_tutti_e_due_i_grigi(self):
        # Un grigio spento portato addosso a una fase: `T9` deve cadere per QUEL grigio, qualunque dei due.
        for tabella, chiave, stato in ((g.CHROME, "Text_Disabled", "Text_Disabled"),
                                       (g.SEMANTIC, "Disabled", "Sem_Disabled")):
            with self.subTest(stato=stato):
                errori = self.errori(tabella, {chiave: g.PHASE_INK["Move"]})
                self.assertTrue(any(e.startswith("T9: ") and f"STATO `{stato}`" in e for e in errori), errori)

    def test_un_documento_owner_che_non_si_legge_e_un_pal_rosso(self):
        # Un albero finto con `RTHUD.cpp` e §32 in ordine, e senza il documento semantico: il generatore
        # non deve restare verde perche' meta' del confronto non si e' potuta fare.
        fedele_ui, _ = documenti_fedeli()
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            for relativo, testo in (("Source/RefactorTactics/UI/RTHUD.cpp", rthud()), (cm.HUD_STYLE_DOC, fedele_ui)):
                file = root / relativo
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_text(testo, encoding="utf-8")
            with mock.patch.object(g, "REPO_ROOT", root):
                errori = g.check_colour_gates()
        self.assertEqual(len(errori), 1, errori)
        self.assertTrue(errori[0].startswith("PAL: un documento owner della palette non si legge"), errori)
        self.assertIn("02-color-system.md", errori[0])

    def test_il_gate_dell_alfabeto_li_chiama(self):
        # `check_alphabet_gates` e' la porta che il generatore attraversa davvero. Senza voci non rasterizza
        # niente, e i gate di colore non dipendono dalle voci: il `PAL` rosso deve arrivare da li'.
        errori = self.errori(g.CHROME, {"Cyan": "#00E0FE"}, porta=lambda: g.check_alphabet_gates([]))
        self.assertTrue(any(e.startswith("PAL: ") and "RT_UI_Cyan" in e for e in errori), errori)
        puliti = [e for e in g.check_alphabet_gates([]) if not e.startswith("T1/T3: non misurabili")]
        self.assertEqual(puliti, [])


class IlSorgenteVeroSiLegge(unittest.TestCase):
    """I test che legano il referto all'albero vero: cadono quando una riga di `RTHUD.cpp` cambia forma."""

    def test_overlay_colors(self):
        identity, phase = cm.overlay_colors()
        self.assertEqual(sorted(identity), ["nemico", "proprie"])
        self.assertEqual(sorted(phase), ["Dash"])
        for value in list(identity.values()) + list(phase.values()):
            self.assertRegex(value, r"^#[0-9A-F]{6}$")

    def test_il_referto_arriva_in_fondo(self):
        # Nessuna variabile d'ambiente d'aiuto: il referto porta da se' lo stdout a UTF-8, e su una
        # console `cp850` deve arrivare in fondo lo stesso. Il codice dice quale dei due rossi e':
        # 1 un criterio violato, 2 una lettura fallita.
        ambiente = {k: v for k, v in os.environ.items() if k != "PYTHONIOENCODING"}
        done = subprocess.run(
            [sys.executable, os.path.join(HERE, "color_metrics.py")],
            capture_output=True, check=False, env=ambiente)
        stdout = done.stdout.decode("utf-8", errors="replace")
        stderr = done.stderr.decode("utf-8", errors="replace")
        self.assertEqual(done.returncode, cm.EXIT_OK, stdout[-600:] + stderr[-600:])
        self.assertIn("World overlay", stdout)


if __name__ == "__main__":
    unittest.main()
