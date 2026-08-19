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

    def test_un_contratto_incoerente_e_una_violazione(self):
        """Il quarto invariante deve essere **collegato** a `controlla()`, non solo esistere.
        La verifica di mutazione l'ha trovato scollegato: sostituendo la chiamata con una lista
        vuota, prima di questo test non cadeva niente — un gate che tace e' peggio di uno assente,
        perche' il suo OK viene creduto."""
        finto = {"output": "docs/inesistente.json", "generatore": "scripts/non-esiste.py",
                 "comando": "x", "check": None, "sorgenti": [], "consumatori": []}
        with unittest.mock.patch.object(inv, "CONTRATTI", [finto]):
            self.assertGreater(inv.controlla(inventario_finto()), 0)

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

    def test_il_contratto_dei_generati_dice_il_vero(self):
        """Sui file veri: ogni generatore esiste, ogni output esiste, ogni `--check` dichiarato e'
        implementato, e la tabella di `docs/generated/README.md` e' allineata alla dichiarazione.
        E' il test che si accorge di un contratto diventato finzione."""
        guasti = inv.controlla_contratto()
        self.assertEqual(guasti, [], chr(10).join(guasti))

    def test_generati_e_contratti_non_divergono(self):
        """Due elenchi della stessa cosa divergono alla prima aggiunta: `GENERATI` serve
        all'invariante sulle orfane, `CONTRATTI` alla provenienza, e devono parlare degli stessi
        artefatti."""
        output = {c["output"].rstrip("*") for c in inv.CONTRATTI}
        for area in inv.GENERATI:
            self.assertTrue(any(area.startswith(o) or o.startswith(area) for o in output),
                            f"l'area {area} non compare fra gli output del contratto")

    def test_la_tabella_non_si_scrive_a_mano(self):
        """Il blocco fra i marcatori e' generato: se qualcuno lo edita nel documento, il confronto
        con `tabella_contratto()` lo dice. Senza, il contratto diventerebbe due testi che divergono."""
        path = os.path.join(REPO, inv.CONTRATTO_DOC)
        text = open(path, encoding="utf-8").read()
        self.assertIn(inv.CONTRATTO_BEGIN, text)
        corpo = text.split(inv.CONTRATTO_BEGIN, 1)[1].split(inv.CONTRATTO_END, 1)[0].strip()
        self.assertEqual(corpo, inv.tabella_contratto().strip())

    def test_i_conteggi_dei_piani_sono_letti_dai_file(self):
        """La somma delle categorie torna col totale contato a parte: e' il controllo che il README
        di `roadmap/plans/` chiede da se', e che a mano e' fallito tre volte in un giorno."""
        tot, per_banner, arch = inv.conteggio_piani()
        self.assertEqual(sum(per_banner.values()), tot,
                         "la somma dei banner non torna col totale")
        self.assertGreater(tot, 0)
        self.assertGreater(arch, 0)

    def test_il_blocco_dei_piani_e_allineato(self):
        """Il blocco fra i marcatori e' generato: se qualcuno lo riscrive nel documento, il gate
        lo dice. Il numero che ci stava prima era sbagliato in due modi contemporaneamente."""
        path = os.path.join(REPO, inv.PIANI_DIR, "README.md")
        text = open(path, encoding="utf-8").read()
        self.assertIn(inv.PIANI_BEGIN, text)
        corpo = text.split(inv.PIANI_BEGIN, 1)[1].split(inv.PIANI_END, 1)[0].strip()
        self.assertEqual(corpo, inv.tabella_piani().strip())

    def test_in_archivio_niente_di_vivo_e_nessuno_snapshot_nuovo(self):
        """La regola raffinata: uno `SNAPSHOT` resta in `plans/` finche' e' l'ultima misura del suo
        oggetto. I due README si contraddicevano, e una contraddizione fra due testi in prosa non
        la vede nessuno script — qui diventa verificabile.

        Due asserzioni distinte: **nessun CURRENT** in archivio, che sarebbe un errore vero; e
        nessuno `SNAPSHOT` fuori dall'insieme congelato del 2026-08-14, cosi' l'eredita' resta
        ammessa e la regola vecchia non puo' rientrare per abitudine."""
        import subprocess as _s
        arch = [p for p in _s.run(["git", "-C", REPO, "ls-files", inv.PIANI_ARCHIVIO],
                                  capture_output=True, text=True, encoding="utf-8").stdout.split()
                if p.endswith(".md") and not p.endswith("README.md")]
        vivi = [p for p in arch if inv._banner(os.path.join(REPO, p)) == "CURRENT"]
        self.assertEqual(vivi, [], "in archivio ci sono documenti CURRENT: " + ", ".join(vivi))
        nuovi = [p for p in arch
                 if inv._banner(os.path.join(REPO, p)) == "SNAPSHOT"
                 and os.path.basename(p) not in inv.SNAPSHOT_ARCHIVIATI_2026_08_14]
        self.assertEqual(nuovi, [],
                         "uno SNAPSHOT nuovo e' finito in archivio, cioe' la regola vecchia "
                         "riapplicata per abitudine: " + ", ".join(nuovi))

    def test_ogni_dichiarazione_porta_una_data(self):
        """Una promessa senza data non scade, e un allowlist che non scade diventa un permesso."""
        for chiave, ragione in list(inv.DUPLICATI_NOTI.items()) + list(inv.ORFANE_NOTE.items()):
            self.assertRegex(ragione, r"20\d\d-\d\d-\d\d",
                             f"la voce {chiave[:12]} non dice quando e' stata presa")
            self.assertRegex(ragione, r"#\d+",
                             f"la voce {chiave[:12]} non nomina la issue che la chiude")


if __name__ == "__main__":
    unittest.main(verbosity=2)
