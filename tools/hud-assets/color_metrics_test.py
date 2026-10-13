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

  * la forma ATTUALE si legge, coi colori giusti, in tutte le notazioni di colore che il referto ammette;
  * la forma VECCHIA e una forma INVENTATA danno un errore esplicito, non un insieme vuoto;
  * un COMMENTO non e' codice, e nemmeno il contenuto di una stringa;
  * cio' che non e' un LETTERALE di colore — un prodotto, un ternario annidato — e' un errore, non una
    lettura del primo colore che capita;
  * una seconda scrittura del campo dell'identita' nella STESSA funzione e' un errore; in un'altra no;
  * i tre criteri: provenienza, fase contro identita', identita' contro identita';
  * il referto distingue «criterio violato» da «lettura fallita» col codice d'uscita, anche in `--json`,
    e una lettura fallita non si porta via la meta' che non dipende da `RTHUD.cpp`;
  * il sorgente VERO si legge: e' il test che sarebbe diventato rosso il 2026-09-04.

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

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import color_metrics as cm  # noqa: E402

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


class IlRefertoDistingueIDueRossi(unittest.TestCase):
    """`main()` su un albero finto: il codice d'uscita dice SE ha fallito la lettura o un criterio."""

    def esegui(self, rthud_cpp, argv: list[str], documenti: bool = True) -> tuple[int, str, str]:
        with tempfile.TemporaryDirectory() as tmp:
            if rthud_cpp is not None:
                file = pathlib.Path(tmp) / "Source" / "RefactorTactics" / "UI" / "RTHUD.cpp"
                file.parent.mkdir(parents=True)
                file.write_text(rthud_cpp, encoding="utf-8")
            if documenti:
                # I due documenti owner della palette, copiati dall'albero vero: il referto li legge
                # (`#3627`), e qui non sono cio' che si sta provando.
                for relativo in (cm.HUD_STYLE_DOC, cm.SEMANTIC_DOC):
                    copia = pathlib.Path(tmp) / relativo
                    copia.parent.mkdir(parents=True, exist_ok=True)
                    copia.write_bytes((cm.REPO_ROOT / relativo).read_bytes())
            out, err = io.StringIO(), io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                code = cm.main(argv, repo_root=pathlib.Path(tmp))
            return code, out.getvalue(), err.getvalue()

    VECCHIA = rthud("\tconst FLinearColor Color = " + IDENTITA)
    VIOLATA = rthud(dash="\tconst FLinearColor DashColor = FLinearColor(0.2f, 0.9f, 1.f, 1.f);\n")

    def test_albero_sano(self):
        code, out, _ = self.esegui(rthud(), [])
        self.assertEqual(code, cm.EXIT_OK, out[-400:])
        self.assertIn("World overlay", out)
        self.assertIn("identita' fra loro", out)

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
        self.assertTrue(json.loads(out)["criteri_non_rispettati"])

    def test_i_documenti_owner_che_mancano_sono_una_lettura_fallita(self):
        # La terza meta' del referto (`#3627`): senza i due documenti il confronto delle tabelle non si puo'
        # fare, e non e' un criterio violato. Le altre due meta' si stampano lo stesso.
        code, out, err = self.esegui(rthud(), [], documenti=False)
        self.assertEqual(code, cm.EXIT_READ_FAILED)
        self.assertIn("LETTURA FALLITA", err)
        self.assertIn("NON LETTE", out)
        self.assertIn("identita' fra loro", out)

    def test_json_porta_l_esito_delle_tabelle(self):
        code, out, _ = self.esegui(rthud(), ["--json"])
        self.assertEqual(code, cm.EXIT_OK)
        self.assertEqual(json.loads(out)["tabelle"], {"divergenze": []})


class LeTabelleSonoCopieDeiDocumenti(unittest.TestCase):
    """`check_palette_owners` e i suoi due mattoni (#3627): le tabelle del generatore contro i loro owner."""

    UI_DOC = ("## 32. Palette\n\n| Token | HEX | Uso |\n|---|---|---|\n"
              "| `RT_UI_Cyan` | `#00E0FF` | focus |\n| `RT_UI_Text_Disabled` | `#6B7684` | testo spento |\n"
              "| `RT_UI_Phase_Dash` | `#009E73` | fase Dash |\n")
    SEM_DOC = ("| Famiglia | Token | HEX | Uso |\n|---|---|---|---|\n"
               "| Movement | `RT_Sem_Movement` | `#009E73` | Move |\n"
               "| Disabled | `RT_Sem_Disabled` | `#6B7280` | esaurito |\n")

    def albero(self, tmp: str, ui: str, sem: str) -> pathlib.Path:
        root = pathlib.Path(tmp)
        for relativo, testo in ((cm.HUD_STYLE_DOC, ui), (cm.SEMANTIC_DOC, sem)):
            file = root / relativo
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_text(testo, encoding="utf-8")
        return root

    def errori(self, fasi: dict, chrome: dict, semantic: dict, ui: str | None = None, sem: str | None = None):
        with tempfile.TemporaryDirectory() as tmp:
            root = self.albero(tmp, self.UI_DOC if ui is None else ui, self.SEM_DOC if sem is None else sem)
            return cm.check_palette_owners(fasi, chrome, semantic, root)

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

    def test_una_tabella_che_ha_cambiato_forma_non_si_legge(self):
        with self.assertRaises(RuntimeError) as caught:
            self.errori({}, {"Cyan": "#00E0FF"}, {}, ui="## 32. Palette\n\nLa tabella e' stata tolta.\n")
        self.assertIn("nessun token letto", str(caught.exception))

    def test_un_token_dichiarato_due_volte_con_due_valori(self):
        doppio = self.UI_DOC + "| `RT_UI_Cyan` | `#00E0FE` | focus, di nuovo |\n"
        with self.assertRaises(RuntimeError) as caught:
            self.errori({}, {"Cyan": "#00E0FF"}, {}, ui=doppio)
        self.assertIn("due volte", str(caught.exception))

    def test_le_tabelle_vere_coincidono_coi_documenti_veri(self):
        # Il test che lega le copie agli owner sull'albero vero: cade quando qualcuno ritocca una tabella
        # del generatore o un documento, e non l'altro.
        sys.path.insert(0, HERE)
        import generate_hud_assets as g  # noqa: PLC0415
        self.assertEqual(cm.check_palette_owners(g.PHASE_INK, g.CHROME, g.SEMANTIC), [])


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
