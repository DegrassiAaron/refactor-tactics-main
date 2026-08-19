#!/usr/bin/env python3
"""Inventario di `docs/`: documenti, immagini, riferimenti — e i tre invarianti che li tengono onesti.

Nasce dall'audit di #1165, che ha misurato una cosa sola ma decisiva: **`docs/` e' fatta di
immagini**. Il 2026-08-17 erano 464 file su 888 e il 93,8% dei byte; il numero di oggi lo stampa
questo script, e non si trascrive qui. Ogni gate esistente legge Markdown; nessuno sapeva dire
quante immagini ci fossero, chi le usasse, quali fossero la stessa immagine due volte.

    python scripts/docs_inventory.py                     # report leggibile
    python scripts/docs_inventory.py --check             # exit 1 se un invariante cade
    python scripts/docs_inventory.py --json              # scrive build/docs-audit/*.json
    python scripts/docs_inventory.py --wiki-root <clone> # conta anche il clone come consumer
    python scripts/docs_inventory.py --emit-contract     # riscrive la tabella del contratto

## I quattro invarianti di `--check`

  1. **Nessuna immagine referenziata e mancante.** Il gemello per le immagini di
     `check-docs-links.py`, che pero' verifica i target di *ogni* link: qui la domanda e' ristretta
     e il messaggio dice quale documento la incorpora.
  2. **Nessun gruppo di duplicati esatti non dichiarato.** Due path, stesso SHA-256. Un duplicato
     puo' essere legittimo — vedi `DUPLICATI_NOTI` — ma deve essere **scritto**, con la ragione
     accanto. Non e' un'esenzione: e' una promessa che il gate verifica al contrario, e una voce
     che non corrisponde piu' a niente lo fa fallire, esattamente come `DEBITO_NOTO` dell'altro gate.
  4. **Il contratto degli artefatti generati dice il vero.** Ogni output dichiarato in `CONTRATTI`
     ha un generatore che esiste, un file che esiste, e un `--check` che il generatore **implementa**
     davvero. La tabella di `docs/generated/README.md` si scrive da qui, e il gate fallisce se
     qualcuno la modifica li': un contratto scritto due volte diverge alla prima aggiunta.
  3. **Un'immagine orfana sta solo in area di ricerca, o e' un output con un owner.** Misurato il
     2026-08-17: le 393 immagini senza un solo riferimento erano **tutte** sotto `docs/src/`, e
     fuori di li' gli orfani erano **zero**. E' la proprieta' che la riorganizzazione deve
     conservare — un'immagine che nessuno usa e che non e' materiale grezzo e' un file che nessuno
     sa perche' e' li.
     ⚠️ Un **generato** non ha riferimenti entranti e non deve averne: il suo owner e' il generatore
     che lo scrive, dichiarato in `GENERATI`. Chiedere un link a `docs/generated/icons/` sarebbe la
     domanda sbagliata — sono 296 file prodotti da una geometria, non da una pagina.
     ⚠️ **Questo terzo invariante si valuta solo con `--wiki-root`**, e senza lo script lo dice
     invece di fingere: il clone e' un repository separato (D-076) che incorpora otto radar SVG e
     `roadmap-map.svg` via `raw.githubusercontent.com`. Senza il clone quei nove risultano orfani —
     lo ha fatto davvero la prima esecuzione — e un gate che sbaglia nove volte su dieci viene
     disattivato. Stessa forma della meta' `--online` di `check-capability-owners.py`.

## Cosa NON fa

- **Non decide.** Near-duplicate e ridondanza informativa producono *candidati*, mai cancellazioni:
  la somiglianza non e' una prova. Una famiglia generata dallo stesso template batte ogni metrica
  percettiva — le 38 card di `docs/characters/images/paragon/` hanno prodotto **558 falsi positivi
  su 562** perche' l'hash misura la cornice, non il soggetto. Si escludono per **dichiarazione**
  (`FAMIGLIE_TEMPLATE`), non per soglia.
- **Non richiede dipendenze.** Con Pillow calcola dimensioni e hash percettivi; senza, dice quali
  sezioni ha saltato e i tre invarianti restano verificabili — sono tutti di sola contabilita'.
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import urllib.parse
from collections import defaultdict

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

IMG_EXT = (".png", ".jpg", ".jpeg", ".webp", ".gif", ".svg", ".bmp", ".tif", ".tiff", ".ico")
TEXT_EXT = (".md", ".py", ".ts", ".js", ".mjs", ".html", ".json", ".yaml", ".yml",
            ".cpp", ".h", ".cs", ".ini", ".csv", ".txt", ".sh", ".ps1")

# Aree in cui un'immagine orfana e' ammessa: materiale grezzo non ancora consumato, e storico.
# ➖ `docs/src/` esce il 2026-08-19: la fase 3 di #1165 l'ha svuotata e `git ls-files docs/src`
# restituisce **zero**. La cartella sopravvive nel checkout dell'autore per un export `.zip` che
# `.gitignore` copre, e questo e' il motivo per cui il contratto la verifica con git e non con
# `os.path.isdir`: su questa macchina la cartella c'e', su un clone pulito no.
AREE_GREZZE = ("docs/research/", "docs/archive/")

# Aree **generate**: il loro owner non e' un documento che le incorpora, e' il generatore che le
# scrive. Chiedere un riferimento entrante a un output sarebbe la domanda sbagliata — i 296 file di
# `docs/generated/icons/` non li linka nessuno e non devono: li produce `build-icon-assets.py` dalla
# geometria dichiarata, e cancellarli o rinominarli si fa toccando quello.
#
# ⚠️ Il generatore dev'essere **committato**, ed e' la parte che conta: un output senza generatore
# nel repository e' un binario scritto a mano con un'etichetta che mente. `TestContratto` verifica
# che ognuno di questi file esista davvero, cosi' una rinomina del generatore non lascia in piedi
# un'esenzione che non protegge piu' niente. Trovato misurando: le 38 card Paragon si dichiarano
# «generated wiki data cards» in un manifest, e il loro generatore **non e' nel repository** —
# per questo `docs/characters/images/` NON e' qui dentro.
GENERATI = {
    "docs/generated/icons/": "scripts/build-icon-assets.py",
    "docs/characters/radar/": "tools/radar/generate.ts",
    "docs/roadmap/charts/": "scripts/feature_registry.py",
}

# --- Il contratto degli artefatti generati ---------------------------------------------------------
#
# `source -> generator -> output -> consumer`, dichiarato **una volta**. La tabella di
# `docs/generated/README.md` si scrive da qui (`--emit-contract`) e si verifica da qui (`--check`):
# un contratto scritto due volte diverge alla prima aggiunta, ed e' il difetto che questo repository
# ha gia' pagato con l'elenco dei gate in `AGENTS.md` — sei gate, tre conosciuti.
#
# ⚠️ Perche' non vive in `parallel-batch.yaml`, che ha gia' un `generated_only` con `derives_from`:
# quel file **si riscrive a ogni batch** e lo dichiara in testa («vive quanto il batch, non e' un
# registro storico»). Un contratto che scade quando finisce un lotto di sessioni non e' un contratto.
# Li' resta la sua funzione di governance — *questi path non si assegnano* — e qui la provenienza.
#
# `check` e' il comando che dice se l'output e' allineato alla sorgente. `None` significa **che non
# esiste**, ed e' un'informazione, non una lacuna da nascondere: e' l'unico modo per accorgersi che un
# artefatto versionato non ha modo di dichiararsi stantio.
CONTRATTI = [
    {
        "output": "docs/roadmap/feature-registry.json",
        "generatore": "scripts/feature_registry.py",
        "comando": "python scripts/feature_registry.py generate",
        "check": "python scripts/feature_registry.py generate --check",
        "sorgenti": ["docs/roadmap/feature-registry.yaml"],
        "consumatori": ["docs/control-center/", "docs/roadmap/project-graph.json"],
    },
    {
        "output": "docs/roadmap/project-graph.json",
        "generatore": "scripts/feature_registry.py",
        "comando": "python scripts/feature_registry.py generate",
        "check": "python scripts/feature_registry.py generate --check",
        "sorgenti": ["docs/roadmap/feature-registry.yaml", "docs/roadmap/editor-sessions.yaml",
                     "docs/roadmap/execution-graph.yaml", "docs/roadmap/roadmap-v0.1.md",
                     "docs/roadmap/roadmap-checkpoint.md", "Scenarios/"],
        "consumatori": ["docs/control-center/", "Wiki: Stato del progetto"],
    },
    {
        "output": "docs/roadmap/charts/roadmap-map.svg",
        "generatore": "scripts/feature_registry.py",
        "comando": "python scripts/feature_registry.py generate",
        "check": "python scripts/feature_registry.py generate --check",
        "sorgenti": ["docs/roadmap/feature-registry.yaml"],
        "consumatori": ["docs/roadmap/roadmap-v0.1.md"],
    },
    {
        "output": "docs/roadmap/*.shortlist.md",
        "generatore": "scripts/feature_registry.py",
        "comando": "python scripts/feature_registry.py shortlist",
        "check": None,
        "sorgenti": ["docs/roadmap/feature-registry.yaml", "docs/roadmap/editor-sessions.yaml",
                     "docs/roadmap/roadmap-v0.1.md", "Scenarios/"],
        "consumatori": ["lettura umana in code review — sono le viste che restano nel repository"],
    },
    {
        "output": "docs/characters/radar/*.svg",
        "generatore": "tools/radar/generate.ts",
        "comando": "node tools/radar/generate.ts",
        "check": "node tools/radar/generate.ts --check",
        "sorgenti": ["docs/balance/"],
        "consumatori": ["docs/characters/v0.1/*.md", "Wiki: pagine personaggio (URL assoluto)"],
    },
    {
        "output": "docs/generated/icons/",
        "generatore": "scripts/build-icon-assets.py",
        "comando": "python scripts/build-icon-assets.py",
        "check": "python scripts/build-icon-assets.py --check",
        "sorgenti": ["scripts/build-icon-assets.py (la geometria e' dichiarata nel generatore)",
                     "Source/RefactorTactics/Ability/RTCatalogLibrary.cpp"],
        "consumatori": ["nessuno oggi: sono master iconografici in attesa di E20"],
    },
]

# Blocchi generati DENTRO un documento scritto a mano: il file e' authored, la porzione fra i due
# marcatori no. E' la forma piu' facile da rompere — rigenerare cancella la prosa che qualcuno ha
# scritto dentro il blocco, e un `--check` che confronta il file intero non lo distingue.
BLOCCHI_GENERATI = {
    "RT_SUITE_COUNT": ("python scripts/feature_registry.py suite",
                       ["docs/README.md", "docs/roadmap/roadmap-v0.1.md"]),
    "RT_FEATURE_BY_EPIC": ("python scripts/feature_registry.py generate",
                           ["docs/roadmap/roadmap-v0.1.md"]),
    "RT_FEATURE_STATUS": ("python scripts/feature_registry.py deploy",
                          ["docs/characters/index.md", "docs/characters/v0.1/*.md"]),
}

# Duplicati esatti dichiarati: SHA-256 -> ragione. NON e' un'esenzione, e' una **promessa datata**:
# il gate la verifica al contrario, e una voce che non corrisponde piu' a un gruppo vivo lo fa
# fallire. Stessa disciplina di `DEBITO_NOTO` in `check-docs-links.py`, e stesso motivo per cui
# esiste: un gate senza via d'uscita viene disattivato *per intero* al primo blocco legittimo.
#
# ✅ **Oggi e' vuoto, e va tenuto vuoto.** Le quattro voci con cui e' nato — il debito misurato
# all'apertura di #1165 — sono state consolidate dalla fase 3: due copie del kit graytoolkit e le
# due sorgenti grezze di `technical/img/`. In ogni gruppo e' sopravvissuta la copia nell'area che la
# possiede, e le voci sono sparite con i gruppi. E' il comportamento che il gate pretende: una
# promessa mantenuta si cancella, non si lascia a decorare l'elenco.
DUPLICATI_NOTI: dict[str, str] = {}

# Immagini orfane ammesse fuori dall'area grezza: path -> ragione. Stessa natura di sopra.
ORFANE_NOTE: dict[str, str] = {
    "docs/technical/img/Mock-HUD.png":
        "2026-08-19 · e' il mockup «Planning — Selected + Warning» che §46 di `progettazione-hud.md` "
        "dichiara necessario, e la prima stesura di questa voce diceva che bastava aggiungere il link. "
        "APERTA l'immagine: mostra **FLUX** come eroe selezionato, piu' «ORBITRON» e «EXO 2» — nomi che "
        "precedono perfino il roster legacy di D-130. Incorporarla in un documento CURRENT porterebbe "
        "nomi ritirati nei pixel di una pagina viva, che e' esattamente il difetto di #1166. Resta "
        "orfana finche' non e' rigenerata: il perimetro e' #1166, non una riga mancante",
}

# 🔴 **Un file che DICHIARA un path non lo sta usando**, e questi due lo dichiarano di mestiere.
# Senza questa esclusione `ORFANE_NOTE` si invalida da sola: la voce nomina l'immagine, lo scanner
# dei path dentro gli script la conta come riferimento, l'immagine smette di risultare orfana, e
# la voce diventa «stantia» — un ciclo in cui il gate fallisce **per colpa della propria
# dichiarazione**. Trovato eseguendo `--check` subito dopo aver scritto l'allowlist.
NON_SONO_USI = ("scripts/docs_inventory.py", "scripts/test_docs_inventory.py")

# Famiglie di immagini generate dallo stesso template: stesso sfondo, stessa cornice, cambia il
# testo. Nessuna metrica su pixel le separa, e nessuna soglia le salva — l'informazione che le
# distingue e' testo dentro l'immagine. Si escludono dai candidati near-duplicate per costruzione.
FAMIGLIE_TEMPLATE = (
    "docs/characters/images/paragon/",   # 38 card generate dal manifest v0.7, stesso layout 1200x630
)

# --- estrazione dei riferimenti: stesse regole di check-docs-links.py -----------------------------
# Se questo file contasse i link in modo diverso dal gate, il repository avrebbe due verita'.
LINK_RE = re.compile(r'(!?)\[(`?)([^\]]*?)(`?)\]\(\s*<?([^)>\s]+)>?(?:\s+"[^"]*")?\s*\)')
FENCE_RE = re.compile(r"^([ \t]*)(`{3,}|~{3,}).*?^\1?\2[ \t]*$", re.M | re.S)
IMG_TAG_RE = re.compile(r"""<img\b[^>]*?\bsrc\s*=\s*["']([^"']+)["']""", re.I)
SKIP_SCHEMES = ("http://", "https://", "mailto:", "ftp://", "tel:", "data:")

# Un URL assoluto che ripunta dentro questo repository **e' un riferimento**, e saltarlo e' il
# difetto che la prima stesura di questo script ha commesso: la Wiki incorpora i radar di
# `docs/characters/radar/` via `raw.githubusercontent.com`, e senza questa regex risultavano orfani.
ABS_REPO_URL_RE = re.compile(
    r"https?://(?:raw\.githubusercontent\.com/DegrassiAaron/refactor-tactics-main/[^/\s]+/"
    r"|github\.com/DegrassiAaron/refactor-tactics-main/(?:blob|raw)/[^/\s]+/)"
    r"([A-Za-z0-9_./\-%]+)", re.I)


def _fence_spans(text):
    return [(m.start(), m.end()) for m in FENCE_RE.finditer(text)]


def _is_quoted(text, start, end):
    """Il link e' avvolto per intero in un code span: e' citato, non usato."""
    before = text[start - 1] if start else ""
    after = text[end] if end < len(text) else ""
    return before == "`" and after == "`"


def tracked(root):
    """I file versionati secondo git. Non la working copy: quella e' solo tua."""
    out = subprocess.run(["git", "-C", root, "ls-files", "-z"], capture_output=True,
                         check=True, text=True, encoding="utf-8").stdout
    return sorted(p.replace("\\", "/") for p in out.split("\0") if p)


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def read_text(path):
    for enc in ("utf-8", "utf-8-sig", "cp1252"):
        try:
            with open(path, encoding=enc) as fh:
                return fh.read()
        except (UnicodeDecodeError, OSError):
            continue
    return None


def resolve(target, src_rel, root):
    """Un target di link -> path repo-relative del repository PRINCIPALE, oppure None."""
    t = (target or "").strip()
    if not t or t.startswith("#"):
        return None
    m = ABS_REPO_URL_RE.match(t)
    if m:
        return urllib.parse.unquote(m.group(1)).split("#")[0].split("?")[0]
    if t.lower().startswith(SKIP_SCHEMES):
        return None
    t = t.split("#", 1)[0].split("?", 1)[0]
    if not t:
        return None
    t = urllib.parse.unquote(t).replace("\\", "/")
    if t.startswith("/"):
        base, t = root, t.lstrip("/")
    else:
        base = os.path.join(root, os.path.dirname(src_rel))
    rel = os.path.relpath(os.path.normpath(os.path.join(base, t)), root).replace("\\", "/")
    return None if rel.startswith("..") else rel


def raccogli_riferimenti(root, files, repo_label, principale):
    """[(src, kind, target, riga)] verso path del repository principale.

    Per il clone della Wiki vale la semantica di GitHub Wiki: un path relativo si risolve dalla
    **radice** del wiki anche da una pagina in sottocartella. Quei target restano interni al clone
    e non ci interessano; ci interessano gli URL assoluti, che sono il modo in cui la Wiki
    incorpora un'immagine di `docs/`.
    """
    out = []
    for rel in files:
        if not rel.lower().endswith(TEXT_EXT):
            continue
        abs_p = os.path.join(root, rel)
        if not os.path.exists(abs_p) or os.path.getsize(abs_p) > 8_000_000:
            continue
        text = read_text(abs_p)
        if text is None:
            continue
        trovati = []
        # Le regioni gia' consumate da un link Markdown o da un `<img>`: l'ultimo passo cerca gli URL
        # assoluti **nudi**, e senza questo insieme conterebbe due volte un `![alt](https://raw...)`
        # — una come immagine e una come URL. Trovato dal test, non in produzione: il doppio conteggio
        # non falsifica un invariante (un target con due riferimenti resta referenziato) ma gonfia
        # ogni numero del report, ed e' il genere di errore che si scopre confrontando due misure.
        consumate = []
        if rel.lower().endswith(".md"):
            fences = _fence_spans(text)
            for m in LINK_RE.finditer(text):
                if any(a <= m.start() < b for a, b in fences) or _is_quoted(text, m.start(), m.end()):
                    consumate.append((m.start(), m.end()))
                    continue
                trovati.append(("md-image" if m.group(1) == "!" else "md-link", m.group(5), m.start()))
                consumate.append((m.start(), m.end()))
        for m in IMG_TAG_RE.finditer(text):
            trovati.append(("html-img", m.group(1), m.start()))
            consumate.append((m.start(), m.end()))
        if not rel.lower().endswith(".md") and rel not in NON_SONO_USI:
            # I path dentro script e config sono repo-relative, non relativi al file che li contiene.
            for m in re.finditer(r"""['"`]([^'"`\s]*docs/[^'"`\s]+)['"`]""", text):
                trovati.append(("code-str", m.group(1), m.start()))
                consumate.append((m.start(), m.end()))
        for m in ABS_REPO_URL_RE.finditer(text):
            if any(a <= m.start() < b for a, b in consumate):
                continue
            trovati.append(("abs-url", m.group(0), m.start()))
        for kind, raw, pos in trovati:
            if kind == "code-str":
                target = resolve(raw, ".", principale)
            elif kind == "abs-url" or ABS_REPO_URL_RE.match(raw.strip() or " "):
                target = resolve(raw, rel, principale)
            elif repo_label != "main":
                continue  # riferimento interno al clone: non riguarda `docs/`
            else:
                target = resolve(raw, rel, root)
            if not target or not target.startswith("docs/"):
                continue
            out.append({"src": f"{repo_label}:{rel}", "kind": kind, "target": target,
                        "riga": text[:pos].count("\n") + 1})
    return out


# --- immagini: tutto cio' che serve Pillow e' facoltativo ------------------------------------------
try:
    from PIL import Image, ImageStat
    HAVE_PIL = True
except ImportError:
    HAVE_PIL = False


def misura_immagine(abs_path, ext):
    """(larghezza, altezza, dhash) — `None` dove manca Pillow o il formato non si apre."""
    if ext == ".svg" or not HAVE_PIL:
        return (None, None, None, None)
    try:
        with Image.open(abs_path) as im:
            w, h = im.width, im.height
            rgba = im.convert("RGBA")
            fondo = Image.new("RGBA", rgba.size, (255, 255, 255, 255))
            fondo.alpha_composite(rgba)
            g = fondo.convert("L")
            d = g.resize((9, 8), Image.LANCZOS)
            px = list(d.getdata())
            bits = 0
            for riga in range(8):
                for col in range(8):
                    bits = (bits << 1) | (1 if px[riga * 9 + col] > px[riga * 9 + col + 1] else 0)
            std = ImageStat.Stat(g.resize((32, 32), Image.LANCZOS)).stddev[0]
            return (w, h, bits, round(std, 2))
    except Exception:
        return (None, None, None, None)


CONTRATTO_DOC = "docs/generated/README.md"
CONTRATTO_BEGIN = "<!-- RT_CONTRATTO_GENERATI:BEGIN -->"
CONTRATTO_END = "<!-- RT_CONTRATTO_GENERATI:END -->"


def tabella_contratto():
    """La tabella Markdown del contratto, derivata da `CONTRATTI`. Nessun testo scritto a mano."""
    righe = ["| Output | Generatore | Comando | `--check` | Sorgenti |",
             "|---|---|---|---|---|"]
    for c in CONTRATTI:
        check = f"`{c['check']}`" if c["check"] else "**nessuno**"
        sorgenti = " · ".join(f"`{x}`" for x in c["sorgenti"])
        righe.append(f"| `{c['output']}` | `{c['generatore']}` | `{c['comando']}` | {check} | {sorgenti} |")
    righe += ["", "| Blocco | Comando | Dentro |", "|---|---|---|"]
    for nome, (cmd, dentro) in sorted(BLOCCHI_GENERATI.items()):
        righe.append(f"| `{nome}` | `{cmd}` | {' · '.join(f'`{d}`' for d in dentro)} |")
    return "\n".join(righe)


def scrivi_contratto():
    """Riscrive il blocco fra i marcatori. La prosa attorno resta di chi l'ha scritta."""
    path = os.path.join(REPO, CONTRATTO_DOC)
    text = open(path, encoding="utf-8").read()
    i, j = text.index(CONTRATTO_BEGIN), text.index(CONTRATTO_END)
    # Le righe vuote attorno alla tabella non sono estetica: in GFM un paragrafo comincia alla prima
    # riga non vuota, e il marcatore `<!-- ... -->` E' una riga non vuota. Senza lo stacco il
    # delimitatore `|---|` finisce in terza posizione invece che in seconda, e `check-docs-tables.py`
    # segnala — giustamente — una tabella che GitHub non renderebbe. Trovato dal gate, non dedotto.
    # Le righe vuote attorno alla tabella non sono estetica: in GFM un paragrafo comincia alla
    # prima riga non vuota, e il marcatore `<!-- ... -->` E' una riga non vuota. Senza lo stacco
    # il delimitatore `|---|` finisce in terza posizione invece che in seconda, e
    # `check-docs-tables.py` segnala — giustamente — una tabella che GitHub non renderebbe.
    # Trovato dal gate, non dedotto.
    stacco = "\n\n"
    nuovo = text[:i + len(CONTRATTO_BEGIN)] + stacco + tabella_contratto() + stacco + text[j:]
    if nuovo != text:
        open(path, "w", encoding="utf-8").write(nuovo)
        return True
    return False


def controlla_contratto():
    """Il contratto dice il vero: generatori e output esistono, e la tabella e' allineata."""
    guasti = []
    for c in CONTRATTI:
        if not os.path.exists(os.path.join(REPO, c["generatore"])):
            guasti.append(f"il generatore {c['generatore']} non esiste (output {c['output']})")
        out = c["output"]
        if "*" in out:
            import glob as _g
            if not _g.glob(os.path.join(REPO, out)):
                guasti.append(f"nessun file corrisponde all'output {out}")
        elif not os.path.exists(os.path.join(REPO, out)):
            guasti.append(f"l'output {out} non esiste")
        if c["check"] and "--check" not in open(os.path.join(REPO, c["generatore"]),
                                                encoding="utf-8", errors="ignore").read():
            guasti.append(f"{c['generatore']} dichiara un --check che non implementa")

    # ogni area di GENERATI deve comparire fra gli output: sono la stessa cosa detta due volte,
    # e questa e' la riga che impedisce che divergano.
    output = {c["output"].rstrip("*") for c in CONTRATTI}
    for area in GENERATI:
        if not any(area.startswith(o) or o.startswith(area) for o in output):
            guasti.append(f"GENERATI dichiara l'area {area}, che il contratto non nomina")

    for nome, (_, dentro) in BLOCCHI_GENERATI.items():
        for d in dentro:
            if "*" in d:
                continue
            p = os.path.join(REPO, d)
            if not os.path.exists(p):
                guasti.append(f"il blocco {nome} dichiara {d}, che non esiste")
            elif f"{nome}:BEGIN" not in open(p, encoding="utf-8", errors="ignore").read():
                # ⚠️ Si cerca il PREFISSO, non la forma chiusa `<!-- NOME:BEGIN -->`: due dei tre
                # marcatori sono **parametrizzati** — `RT_FEATURE_STATUS:BEGIN RT-FEAT-CHAR-ROSTER`
                # apre un blocco per feature, e ce ne sono piu' d'uno nello stesso file. La prima
                # stesura cercava la forma chiusa e ha dichiarato mancante un marcatore presente
                # quattro volte: un criterio meccanico va validato sul contenuto, non solo scritto.
                guasti.append(f"{d} non contiene il marcatore {nome}")

    path = os.path.join(REPO, CONTRATTO_DOC)
    if not os.path.exists(path):
        guasti.append(f"{CONTRATTO_DOC} non esiste: il contratto non ha un owner leggibile")
    else:
        text = open(path, encoding="utf-8").read()
        if CONTRATTO_BEGIN not in text or CONTRATTO_END not in text:
            guasti.append(f"{CONTRATTO_DOC} ha perso i marcatori del blocco generato")
        else:
            corpo = text.split(CONTRATTO_BEGIN, 1)[1].split(CONTRATTO_END, 1)[0].strip()
            if corpo != tabella_contratto().strip():
                guasti.append(f"{CONTRATTO_DOC} non e' allineato: "
                              "`python scripts/docs_inventory.py --emit-contract`")
    return guasti


def inventario(wiki_root=None):
    principale = REPO
    files = tracked(principale)
    docs = [p for p in files if p.startswith("docs/")]
    tracked_set = set(files)

    documenti, immagini = {}, {}
    for rel in docs:
        ext = os.path.splitext(rel)[1].lower()
        abs_p = os.path.join(principale, rel)
        if not os.path.exists(abs_p):
            continue
        rec = {"path": rel, "ext": ext, "bytes": os.path.getsize(abs_p), "sha256": sha256(abs_p)}
        if ext in IMG_EXT:
            w, h, dh, std = misura_immagine(abs_p, ext)
            rec.update({"width": w, "height": h, "dhash": dh, "stddev": std})
            immagini[rel] = rec
        documenti[rel] = rec

    refs = raccogli_riferimenti(principale, files, "main", principale)
    if wiki_root:
        refs += raccogli_riferimenti(wiki_root, tracked(wiki_root), "wiki", principale)

    entranti = defaultdict(list)
    for r in refs:
        entranti[r["target"]].append(r)

    mancanti = [r for r in refs
                if r["kind"] in ("md-image", "html-img")
                and r["target"] not in tracked_set
                and os.path.splitext(r["target"])[1].lower() in IMG_EXT]

    per_sha = defaultdict(list)
    for rel, rec in documenti.items():
        per_sha[rec["sha256"]].append(rel)
    duplicati = [{"sha256": s, "paths": sorted(p), "bytes": documenti[p[0]]["bytes"],
                  "riferimenti": {q: len(entranti.get(q, [])) for q in sorted(p)}}
                 for s, p in per_sha.items() if len(p) > 1]
    duplicati.sort(key=lambda g: (-len(g["paths"]), g["paths"][0]))

    orfane = sorted(r for r in immagini if not entranti.get(r))
    orfane_fuori = [r for r in orfane
                    if not r.startswith(AREE_GREZZE)
                    and not r.startswith(tuple(GENERATI))
                    and r not in ORFANE_NOTE]

    candidati = []
    saltate_template = 0
    if HAVE_PIL:
        con_hash = [(r, x) for r, x in immagini.items()
                    if x.get("dhash") is not None and (x.get("stddev") or 0) >= 12]
        for i in range(len(con_hash)):
            ra, a = con_hash[i]
            for j in range(i + 1, len(con_hash)):
                rb, b = con_hash[j]
                if a["sha256"] == b["sha256"]:
                    continue
                if bin(a["dhash"] ^ b["dhash"]).count("1") > 6:
                    continue
                if ra.startswith(FAMIGLIE_TEMPLATE) and rb.startswith(FAMIGLIE_TEMPLATE):
                    saltate_template += 1
                    continue
                candidati.append({"a": ra, "b": rb,
                                  "distanza": bin(a["dhash"] ^ b["dhash"]).count("1"),
                                  "dim_a": [a["width"], a["height"]],
                                  "dim_b": [b["width"], b["height"]],
                                  "revisione_richiesta": True})
        candidati.sort(key=lambda x: (x["distanza"], x["a"]))

    return {
        "documenti": documenti, "immagini": immagini, "riferimenti_entranti": dict(entranti),
        "immagini_mancanti": mancanti, "duplicati_esatti": duplicati,
        "orfane": orfane, "orfane_fuori_area_grezza": orfane_fuori,
        "near_duplicate_candidati": candidati,
        "conteggi": {
            "file_docs": len(documenti),
            "markdown": sum(1 for r in documenti if r.endswith(".md")),
            "immagini": len(immagini),
            "byte_docs": sum(r["bytes"] for r in documenti.values()),
            "byte_immagini": sum(r["bytes"] for r in immagini.values()),
            "riferimenti": len(refs),
            "immagini_referenziate": sum(1 for r in immagini if entranti.get(r)),
            "orfane": len(orfane),
            "orfane_generate": sum(1 for r in orfane if r.startswith(tuple(GENERATI))),
            "gruppi_duplicati_esatti": len(duplicati),
            "near_duplicate_candidati": len(candidati),
            "coppie_template_escluse": saltate_template,
            "pillow": HAVE_PIL,
            "wiki": bool(wiki_root),
        },
        "wiki_visto": bool(wiki_root),
    }


def stampa_report(inv):
    c = inv["conteggi"]
    print(f"docs/: {c['file_docs']} file · {c['markdown']} markdown · {c['immagini']} immagini")
    print(f"       {c['byte_docs'] / 1e6:.1f} MB, di cui {c['byte_immagini'] / 1e6:.1f} MB "
          f"({100 * c['byte_immagini'] / max(c['byte_docs'], 1):.1f}%) immagini")
    print(f"riferimenti raccolti: {c['riferimenti']} · immagini referenziate: "
          f"{c['immagini_referenziate']}/{c['immagini']}")
    print(f"orfane: {c['orfane']} (ammesse in {', '.join(AREE_GREZZE)}) · "
          f"generate con owner: {c['orfane_generate']} · "
          f"fuori area: {len(inv['orfane_fuori_area_grezza'])}")
    print(f"gruppi di duplicati esatti: {c['gruppi_duplicati_esatti']} "
          f"(dichiarati in DUPLICATI_NOTI: {len(DUPLICATI_NOTI)})")
    print(f"clone Wiki come consumer: {'si' if c['wiki'] else 'NO — orfane non valutabili'}")
    if c["pillow"]:
        print(f"candidati near-duplicate: {c['near_duplicate_candidati']} "
              f"· coppie di famiglia-template escluse: {c['coppie_template_escluse']}")
    else:
        print("candidati near-duplicate: SALTATI — Pillow non installato "
              "(dimensioni e hash percettivi non calcolati; gli invarianti restano verificabili)")


def controlla(inv):
    """I tre invarianti. Ritorna il numero di violazioni e le stampa."""
    guasti = 0

    if inv["immagini_mancanti"]:
        guasti += len(inv["immagini_mancanti"])
        print(f"\nERRORE — {len(inv['immagini_mancanti'])} immagini incorporate ma assenti:")
        for r in inv["immagini_mancanti"]:
            print(f"  {r['src']}:{r['riga']} -> {r['target']}")

    non_dichiarati = [g for g in inv["duplicati_esatti"] if g["sha256"] not in DUPLICATI_NOTI]
    if non_dichiarati:
        guasti += len(non_dichiarati)
        print(f"\nERRORE — {len(non_dichiarati)} gruppi di file byte-identici non dichiarati:")
        for g in non_dichiarati:
            print(f"  {g['bytes'] / 1024:.0f} KB · {g['sha256'][:12]}")
            for p in g["paths"]:
                print(f"      {p}  (riferimenti: {g['riferimenti'][p]})")
        print("  Consolidane uno e migra i rimandi, oppure dichiara il gruppo in DUPLICATI_NOTI")
        print("  con la ragione accanto: un duplicato legittimo esiste, uno taciuto no.")

    vivi = {g["sha256"] for g in inv["duplicati_esatti"]}
    stantii = [s for s in DUPLICATI_NOTI if s not in vivi]
    if stantii:
        guasti += len(stantii)
        print(f"\nERRORE — {len(stantii)} voci di DUPLICATI_NOTI non corrispondono piu' a niente:")
        for s in stantii:
            print(f"  {s[:12]} — {DUPLICATI_NOTI[s]}")
        print("  Vanno tolte: un allowlist che non sa invalidarsi nasconde il difetto successivo.")

    if not inv["wiki_visto"]:
        print("\nNOTA  terzo invariante NON eseguito: passa --wiki-root <clone> perche' le orfane")
        print("      siano valutate. Il clone e' un repository separato (D-076) e incorpora otto")
        print("      radar SVG piu' roadmap-map.svg via URL assoluto: senza, risultano orfani nove")
        print("      file che orfani non sono.")
        return guasti

    fuori = inv["orfane_fuori_area_grezza"]
    if fuori:
        guasti += len(fuori)
        print(f"\nERRORE — {len(fuori)} immagini senza un solo riferimento, fuori dall'area grezza:")
        for p in fuori:
            print(f"  {p}")
        print(f"  Un'immagine orfana e' ammessa solo in {', '.join(AREE_GREZZE)}, o in un'area")
        print(f"  generata con owner dichiarato ({', '.join(GENERATI)}): altrove significa che nessuno")
        print("  sa perche' e' li. Collegala al documento che la possiede, spostala nell'area giusta,")
        print("  oppure dichiarala in ORFANE_NOTE con la ragione accanto.")

    contratto = controlla_contratto()
    if contratto:
        guasti += len(contratto)
        print(f"\nERRORE — {len(contratto)} incoerenze nel contratto degli artefatti generati:")
        for g in contratto:
            print(f"  {g}")
        print("  Un output versionato senza generatore che lo scriva e' un file a mano con")
        print("  un'etichetta che mente; una tabella disallineata e' un contratto che nessuno legge.")

    orfane_vive = set(inv["orfane"])
    note_stantie = [p for p in ORFANE_NOTE if p not in orfane_vive]
    if note_stantie:
        guasti += len(note_stantie)
        print(f"\nERRORE — {len(note_stantie)} voci di ORFANE_NOTE non descrivono piu' un'orfana:")
        for p in note_stantie:
            print(f"  {p} — {ORFANE_NOTE[p]}")
        print("  L'immagine e' stata collegata o rimossa: togli la voce.")

    return guasti


def main():
    ap = argparse.ArgumentParser(description="Inventario e invarianti di docs/")
    ap.add_argument("--check", action="store_true", help="esce 1 se un invariante cade")
    ap.add_argument("--json", nargs="?", const=os.path.join(REPO, "build", "docs-audit"),
                    metavar="DIR", help="scrive gli artefatti JSON (default build/docs-audit)")
    ap.add_argument("--emit-contract", action="store_true",
                    help=f"riscrive la tabella del contratto in {CONTRATTO_DOC}")
    ap.add_argument("--wiki-root", metavar="PATH",
                    help="il clone della Wiki: e' un repository separato (D-076) e git non lo elenca")
    args = ap.parse_args()

    if args.wiki_root and not os.path.isdir(os.path.join(args.wiki_root, ".git")):
        if not os.path.exists(os.path.join(args.wiki_root, ".git")):
            print(f"--wiki-root non e' un clone git: {args.wiki_root}", file=sys.stderr)
            return 2

    if args.emit_contract:
        cambiato = scrivi_contratto()
        print(f"{CONTRATTO_DOC}: {'riscritto' if cambiato else 'gia allineato'}")
        return 0

    inv = inventario(args.wiki_root)
    stampa_report(inv)

    if args.json:
        os.makedirs(args.json, exist_ok=True)
        for nome, dato in (("conteggi", inv["conteggi"]),
                           ("immagini", inv["immagini"]),
                           ("duplicati-esatti", inv["duplicati_esatti"]),
                           ("orfane", inv["orfane"]),
                           ("immagini-mancanti", inv["immagini_mancanti"]),
                           ("near-duplicate", inv["near_duplicate_candidati"]),
                           ("riferimenti-entranti", inv["riferimenti_entranti"])):
            with open(os.path.join(args.json, f"{nome}.json"), "w", encoding="utf-8") as fh:
                json.dump(dato, fh, indent=1, ensure_ascii=False)
        print(f"\nartefatti in {os.path.relpath(args.json, REPO)}/ "
              f"(cartella ignorata da .gitignore: non si committa)")

    if args.check:
        guasti = controlla(inv)
        if guasti:
            print(f"\n{guasti} violazioni.")
            return 1
        # Il messaggio dice **quali** invarianti hanno girato: un «OK» che ne copre due su tre
        # e ne nomina tre e' la forma di gate che smette di essere creduta.
        if inv["wiki_visto"]:
            print("\nOK — nessuna immagine mancante, nessun duplicato taciuto, nessuna orfana "
                  "fuori dall'area grezza, contratto dei generati coerente.")
        else:
            print("\nOK sui tre invarianti eseguiti — nessuna immagine mancante, nessun "
                  "duplicato taciuto, contratto dei generati coerente. Quello sulle orfane "
                  "non e' stato valutato: vedi la NOTA sopra.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
