#!/usr/bin/env python3
"""Test dell'inventario di `docs/`.

Uso:
    python scripts/test_docs_inventory.py          # dalla radice del repo
    python -m unittest discover -s scripts -p "test_*.py"

Due gruppi, e la differenza conta — stessa divisione di `docs/control-center/graph.test.mjs`:

  * i test su **fixture** dicono che la regola e' giusta, e falliscono se la si rompe. Girano su
    cartelle temporanee e su dizionari costruiti a mano: nessuno legge il repository vero;
  * i test di **CONTRATTO** dicono che le dichiarazioni scritte nello script corrispondono ancora
    al repository di oggi, e falliscono se qualcuno rinomina la cartella che una costante nomina.
    Sono quelli che si accorgono di un allowlist diventato finzione.
"""
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import unittest.mock

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import docs_inventory as inv  # noqa: E402

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def inventario_finto(**over):
    """Un inventario minimo: `controlla()` legge solo queste chiavi."""
    base = {"immagini_mancanti": [], "duplicati_esatti": [], "orfane": [],
            "orfane_fuori_area_grezza": [], "wiki_visto": True}
    base.update(over)
    return base


class TestEstrazioneRiferimenti(unittest.TestCase):
    """Le regole di lettura devono coincidere con `check-docs-links.py`: se questo file contasse
    i link in modo diverso dal gate, il repository avrebbe due verita' su cosa e' un riferimento."""

    def setUp(self):
        self.dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.dir, ignore_errors=True)

    def scrivi(self, rel, testo):
        p = os.path.join(self.dir, rel)
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "w", encoding="utf-8") as fh:
            fh.write(testo)
        return rel

    def raccogli(self, rel):
        return inv.raccogli_riferimenti(self.dir, [rel], "main", self.dir)

    def test_immagine_incorporata_e_un_riferimento(self):
        rel = self.scrivi("docs/a.md", "![alt](img/x.png)\n")
        got = self.raccogli(rel)
        self.assertEqual([(r["kind"], r["target"]) for r in got],
                         [("md-image", "docs/img/x.png")])

    def test_dentro_un_blocco_recintato_non_conta(self):
        """In un ```md un `![img](...)` e' un modello da incollare altrove, e i suoi path sono
        relativi alla DESTINAZIONE. E' l'errore in cui `check-docs-links.py` e' cascato da solo."""
        rel = self.scrivi("docs/a.md", "```md\n![alt](img/x.png)\n```\n")
        self.assertEqual(self.raccogli(rel), [])

    def test_link_citato_fra_backtick_non_conta(self):
        rel = self.scrivi("docs/a.md", "esempio: `![alt](img/x.png)` da non seguire\n")
        self.assertEqual(self.raccogli(rel), [])

    def test_url_assoluto_verso_questo_repo_e_un_riferimento(self):
        """Il difetto per cui questo script esiste in questa forma: la Wiki incorpora i radar via
        `raw.githubusercontent.com`, e un resolver che salta gli schemi http li dichiara orfani.
        La prima esecuzione ne ha persi **nove** cosi'."""
        rel = self.scrivi("docs/a.md",
                          "![r](https://raw.githubusercontent.com/DegrassiAaron/"
                          "refactor-tactics-main/main/docs/characters/radar/gadget-profile.svg)\n")
        got = self.raccogli(rel)
        self.assertEqual([r["target"] for r in got], ["docs/characters/radar/gadget-profile.svg"])

    def test_url_esterno_non_e_un_riferimento(self):
        rel = self.scrivi("docs/a.md", "![x](https://example.com/y.png)\n")
        self.assertEqual(self.raccogli(rel), [])

    def test_target_fuori_dalla_radice_scartato(self):
        rel = self.scrivi("docs/a.md", "![x](../../../fuori.png)\n")
        self.assertEqual(self.raccogli(rel), [])

    def test_tag_html_conta(self):
        rel = self.scrivi("docs/a.md", '<img src="img/y.png" width="200">\n')
        got = self.raccogli(rel)
        self.assertEqual([r["kind"] for r in got], ["html-img"])

    def test_lo_script_che_dichiara_un_path_non_lo_usa(self):
        """`ORFANE_NOTE` nomina i path che dichiara: se lo scanner degli script li contasse come
        riferimenti, l'immagine smetterebbe di risultare orfana e la voce diventerebbe «stantia».
        Il gate fallirebbe **per colpa della propria dichiarazione** — ciclo misurato eseguendo
        `--check` subito dopo aver scritto l'allowlist."""
        rel = self.scrivi("scripts/docs_inventory.py",
                          'ORFANE_NOTE = {"docs/technical/img/Mock-HUD.png": "ragione"}\n')
        self.assertEqual(inv.raccogli_riferimenti(self.dir, [rel], "main", self.dir), [])

    def test_path_in_uno_script_e_repo_relative(self):
        """`"docs/x.png"` dentro un `.py` e' relativo alla radice, non alla cartella dello script:
        risolverlo dal file produce `scripts/docs/x.png`, che non esiste, e un falso rotto."""
        rel = self.scrivi("scripts/tool.py", 'PATH = "docs/roadmap/charts/roadmap-map.svg"\n')
        got = inv.raccogli_riferimenti(self.dir, [rel], "main", self.dir)
        self.assertEqual([r["target"] for r in got], ["docs/roadmap/charts/roadmap-map.svg"])


class TestInvarianti(unittest.TestCase):
    """`controlla()` su inventari costruiti a mano: la regola, senza il repository vero.

    Gli allowlist reali sono **svuotati** per la durata di ogni test: verificano il repository di
    oggi, non la regola, e lasciarli pieni farebbe fallire ogni caso per la ragione sbagliata — le
    loro voci risulterebbero stantie contro un inventario finto. Che siano coerenti col repository
    lo dice `TestContratto` e `--check`, non questi."""

    def setUp(self):
        for elenco in (inv.DUPLICATI_NOTI, inv.ORFANE_NOTE):
            toppa = unittest.mock.patch.dict(elenco, {}, clear=True)
            toppa.start()
            self.addCleanup(toppa.stop)

    def test_niente_da_dire_su_un_inventario_pulito(self):
        self.assertEqual(inv.controlla(inventario_finto()), 0)

    def test_immagine_incorporata_ma_assente_e_una_violazione(self):
        guasti = inv.controlla(inventario_finto(immagini_mancanti=[
            {"src": "main:docs/a.md", "riga": 12, "target": "docs/img/manca.png"}]))
        self.assertEqual(guasti, 1)

    def test_duplicato_non_dichiarato_fallisce(self):
        gruppo = {"sha256": "a" * 64, "paths": ["docs/x.png", "docs/y.png"],
                  "bytes": 100, "riferimenti": {"docs/x.png": 0, "docs/y.png": 1}}
        self.assertEqual(inv.controlla(inventario_finto(duplicati_esatti=[gruppo])), 1)

    def test_duplicato_dichiarato_passa(self):
        gruppo = {"sha256": "b" * 64, "paths": ["docs/x.png", "docs/y.png"],
                  "bytes": 100, "riferimenti": {"docs/x.png": 0, "docs/y.png": 1}}
        inv.DUPLICATI_NOTI["b" * 64] = "2026-08-18 · ragione datata, #1165"
        self.assertEqual(inv.controlla(inventario_finto(duplicati_esatti=[gruppo])), 0)

    def test_voce_stantia_di_duplicati_noti_fallisce(self):
        """Un allowlist che non sa invalidarsi diventa il posto dove i difetti vanno a morire:
        se il gruppo e' stato consolidato, la voce deve sparire o il gate lo pretende."""
        inv.DUPLICATI_NOTI["c" * 64] = "gruppo che non esiste piu'"
        self.assertEqual(inv.controlla(inventario_finto(duplicati_esatti=[])), 1)

    def test_orfana_fuori_area_grezza_e_una_violazione(self):
        guasti = inv.controlla(inventario_finto(
            orfane=["docs/technical/img/z.png"],
            orfane_fuori_area_grezza=["docs/technical/img/z.png"]))
        self.assertEqual(guasti, 1)

    def test_orfana_in_area_generata_non_e_una_violazione(self):
        """Un output non ha riferimenti entranti **per costruzione**: il suo owner e' il generatore.
        Senza questa via i 296 file di `docs/generated/icons/` sarebbero 296 violazioni il giorno
        in cui la fase 2 di #1165 li sposta li'."""
        guasti = inv.controlla(inventario_finto(
            orfane=["docs/generated/icons/action-move-24.png"],
            orfane_fuori_area_grezza=[]))
        self.assertEqual(guasti, 0)

    def test_orfana_dichiarata_non_e_una_violazione(self):
        inv.ORFANE_NOTE["docs/technical/img/z.png"] = "2026-08-18 · ragione, #1165"
        guasti = inv.controlla(inventario_finto(orfane=["docs/technical/img/z.png"],
                                                orfane_fuori_area_grezza=[]))
        self.assertEqual(guasti, 0)

    def test_voce_stantia_di_orfane_note_fallisce(self):
        inv.ORFANE_NOTE["docs/technical/img/z.png"] = "2026-08-18 · ragione, #1165"
        self.assertEqual(inv.controlla(inventario_finto(orfane=[])), 1)

    def test_senza_wiki_il_terzo_invariante_non_gira_e_lo_dice(self):
        """Senza il clone, nove immagini incorporate via URL assoluto risultano orfane. Il gate
        non le segnala e **non finge** di aver controllato: e' la meta' non eseguita."""
        guasti = inv.controlla(inventario_finto(
            wiki_visto=False,
            orfane=["docs/characters/radar/gadget-profile.svg"],
            orfane_fuori_area_grezza=["docs/characters/radar/gadget-profile.svg"]))
        self.assertEqual(guasti, 0)

    def test_senza_wiki_gli_altri_due_invarianti_girano_lo_stesso(self):
        """La meta' non eseguita e' una sola: un duplicato taciuto resta un errore anche senza clone."""
        gruppo = {"sha256": "d" * 64, "paths": ["docs/x.png", "docs/y.png"],
                  "bytes": 1, "riferimenti": {"docs/x.png": 0, "docs/y.png": 0}}
        self.assertEqual(inv.controlla(inventario_finto(wiki_visto=False,
                                                        duplicati_esatti=[gruppo])), 1)


class TestContratto(unittest.TestCase):
    """Sui file veri: le costanti dichiarate corrispondono ancora al repository di oggi."""

    def test_le_famiglie_template_esistono(self):
        """Se la cartella viene rinominata, l'esclusione smette di escludere e nessuno se ne accorge
        finche' i candidati non tornano a 562."""
        for prefisso in inv.FAMIGLIE_TEMPLATE:
            self.assertTrue(os.path.isdir(os.path.join(REPO, prefisso)),
                            f"FAMIGLIE_TEMPLATE nomina {prefisso}, che non esiste piu'")

    def test_ogni_area_generata_ha_il_suo_generatore_nel_repository(self):
        """La parte che conta di `GENERATI`: un output senza generatore committato e' un binario
        scritto a mano con un'etichetta che mente, e l'esenzione smetterebbe di proteggere qualcosa.
        E' anche il motivo per cui `docs/characters/images/` non e' nell'elenco — quelle card si
        dichiarano generate in un manifest, e il generatore nel repository non c'e'."""
        for area, generatore in inv.GENERATI.items():
            self.assertTrue(os.path.isdir(os.path.join(REPO, area)),
                            f"GENERATI nomina l'area {area}, che non esiste")
            self.assertTrue(os.path.isfile(os.path.join(REPO, generatore)),
                            f"l'area {area} dichiara {generatore}, che non esiste")

    def test_ogni_area_grezza_ha_file_versionati(self):
        """Verificata con **git**, non con `os.path.isdir`, e la differenza e' il difetto che questo
        test evita: dopo la fase 3 di #1165 `docs/src/` esiste ancora sul disco dell'autore — ci vive
        un export `.zip` che `.gitignore` copre — ma ha **zero** file versionati. Un controllo sul
        filesystem sarebbe passato qui e caduto su un clone pulito."""
        for prefisso in inv.AREE_GREZZE:
            out = subprocess.run(["git", "-C", REPO, "ls-files", prefisso],
                                 capture_output=True, text=True, encoding="utf-8").stdout
            self.assertTrue(out.strip(),
                            f"AREE_GREZZE nomina {prefisso}, che non ha file versionati")

    def test_docs_src_non_e_piu_un_area(self):
        """La cartella e' stata svuotata: lasciarla nell'elenco significherebbe tenere aperta
        un'eccezione per un posto dove non puo' piu' finire niente."""
        self.assertNotIn("docs/src/", inv.AREE_GREZZE)
        out = subprocess.run(["git", "-C", REPO, "ls-files", "docs/src"],
                             capture_output=True, text=True, encoding="utf-8").stdout
        self.assertEqual(out.strip(), "", "docs/src ha di nuovo file versionati")

    def test_ogni_dichiarazione_porta_una_data(self):
        """Una promessa senza data non scade, e un allowlist che non scade diventa un permesso."""
        for chiave, ragione in list(inv.DUPLICATI_NOTI.items()) + list(inv.ORFANE_NOTE.items()):
            self.assertRegex(ragione, r"20\d\d-\d\d-\d\d",
                             f"la voce {chiave[:12]} non dice quando e' stata presa")
            self.assertRegex(ragione, r"#\d+",
                             f"la voce {chiave[:12]} non nomina la issue che la chiude")


if __name__ == "__main__":
    unittest.main(verbosity=2)
