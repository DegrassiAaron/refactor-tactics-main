#!/usr/bin/env python3
"""Gate documentale sul naming del roster (D-120).

D-120 fissa i nomi canonici/player-facing del roster v0.1 — Gadget, Phase, Riktor,
Wraith — e declassa Flux/Riva/Bastion/Vektor a *legacy implementation identifiers*.
Quei token restano legittimi dove nominano qualcosa che esiste davvero in codice,
asset, scenario o replay: `Hero.Flux`, `Vektor.InterceptShot`, `Spec.State.Riva.Flow`.
Non sono piu' legittimi come **nome del personaggio in prosa**.

Questo script cerca esattamente quella differenza: un nome legacy che compare
*fuori* da un contesto tecnico. Dentro backtick, dentro un blocco di codice, dentro
un percorso o un URL non e' un'occorrenza player-facing e non viene segnalata.

Perche' non e' un search/replace
--------------------------------
Un replace globale romperebbe simboli C++, Stable ID, link e riferimenti storici
insieme: D-120 lo vieta esplicitamente. Il gate segnala, non riscrive.

La copertura era parziale, e dal 2026-08-13 non lo e' piu'
----------------------------------------------------------
Alla nascita di questo gate la prosa legacy misurata era **832 occorrenze in 72
file**: molto piu' di quanto una sola PR potesse correggere onestamente. Un gate
rosso ovunque non protegge niente — diventa rumore che si impara a ignorare, e il
repository ha gia' pagato questo difetto (vedi `docs/archive/src/README.md` sul
conteggio dei sorgenti). Percio' partiva con una **allowlist** di tre file.

Quella ragione e' scaduta: l'arretrato bonificabile e' stato chiuso, e cio' che
resta sono **solo** i registri datati, che non si bonificano per definizione. Una
allowlist ora farebbe il danno opposto — elencherebbe i file gia' bonificati e
lascerebbe **muta** una regressione in tutti gli altri, che sono la maggioranza.

⚠️ Nessun conteggio e' scritto qui, ed e' deliberato. La prima stesura ne fissava
due — «47 file» e «175» — ed erano **sbagliati**: derivati a mano sommando i file
toccati da un solo passaggio, mentre i file passati da sporchi a puliti erano
**52** (72 sporchi alla base, meno i 20 registri esenti che restano tali). Il
referto stampa i numeri veri a ogni esecuzione: si leggono li'.

Quindi il gate e' invertito: **tutto e' protetto tranne cio' che e' esente**, e
l'esenzione e' esplicita e motivata. Un file nuovo nasce protetto senza che nessuno
si ricordi di aggiungerlo a una lista.

Il numero dell'arretrato esente si **rimisura** eseguendo lo script: non si copia
da un documento.

Uso
---
    python scripts/check-docs-naming.py             # referto completo
    python scripts/check-docs-naming.py --check     # gate: esce 1 se un file protetto e' sporco
    python scripts/check-docs-naming.py --all       # elenca anche i registri esenti, file per file
    python scripts/check-docs-naming.py --check --wiki-root <clone>   # anche la Wiki pubblicata

La Wiki e' un repository separato (D-076: il clone E' la fonte), quindi non viene
guardata se non le si passa il percorso. Il 2026-08-13, con `docs/` gia' pulita,
dichiarava ancora il roster legacy in 193 punti su 24 pagine: e' il posto che il
giocatore legge davvero, ed era l'ultimo senza gate.
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DOCS = REPO / "docs"

# La prosa citata nel referto contiene emoji e accenti; la console Windows e' cp1252
# e alzerebbe UnicodeEncodeError a meta' elenco, cioe' proprio quando il gate ha
# qualcosa da dire. Il referto degrada il carattere, non si interrompe.
for _stream in (sys.stdout, sys.stderr):
    if hasattr(_stream, "reconfigure"):
        _stream.reconfigure(errors="replace")

# I quattro identificatori declassati da D-120.
LEGACY = ("Flux", "Riva", "Bastion", "Vektor")

# ✅ **VUOTA dal 2026-08-17 (`#756`), ed e' la meta' che mancava.**
#
# Erano `("archive", "src")`, con una ragione che valeva finche' quei testi conservavano i
# nomi ritirati: «lo storico non si riscrive», «input non ancora recepito». D-130 ha deciso
# l'opposto — *«il perimetro e' ovunque, e include l'archivio: e' una scelta esplicita con
# un costo esplicito»* — e la fetta 6 l'ha eseguita: 1749 occorrenze sostituite, 54 file
# d'archivio con un banner che dichiara la sostituzione, sei righe marcate.
#
# ⚠️ **Conseguenza operativa, e va detta a chi archivia invece che scoperta al primo rosso**:
# archiviare un documento che nomina il roster con i nomi ritirati ora fa **fallire il gate**.
# Non e' un effetto collaterale: e' come si impedisce che l'archivio torni a riempirsi. Chi
# archivia bonifica prima, oppure marca la riga con `rename-exempt` se il nome ritirato e' il
# soggetto della frase.
EXCLUDED_DIRS: tuple[str, ...] = ()

# Esenzioni: gli unici documenti dove un nome legacy in prosa NON e' un difetto.
#
# ⚠️ Sono **registri datati**, e l'esenzione non e' una dimenticanza. Il Decision
# Log, OPEN_DECISIONS, il changelog, gli ADR e i referti di `roadmap/plans/` sono
# *log*: ogni riga e' un'affermazione con una data, e D-037, D-041, D-058, D-069
# descrivono il roster com'era chiamato quando furono scritte. D-120 le supera nella
# parte nominale; non le rende false a posteriori. Riscriverle per far passare un
# gate sarebbe la stessa falsificazione che `docs/archive/src/README.md` vieta per i
# sorgenti — la correzione e' una **nota accanto all'affermazione**, non una modifica
# del paragrafo.
#
# Tutto il resto e' protetto: e' il roster **corrente** letto da chi apre il file oggi.
# ✅ **VUOTE dal 2026-08-17 (`#755`), e la ragione e' che il meccanismo e' cambiato.**
#
# Erano cinque file e due prefissi — Decision Log, OPEN_DECISIONS, changelog, gli ADR,
# `roadmap/plans/` — esenti **per intero**, cioe' 89 file su 240 e una copertura del 63%.
# La giustificazione era buona e resta vera: sono registri datati, e riscrivere «al
# 2026-08-13 c'erano 1600 occorrenze di `Flux`» lo rende falso.
#
# Ma un'esenzione per FILE protegge molto piu' di cio' che deve: nel Decision Log erano
# esenti anche le voci nuove, quelle che descrivono il roster **corrente**, e nessuno lo
# avrebbe saputo. La sostituisce il marcatore `rename-exempt`, che esenta una **riga** e
# **fallisce quando la sua ragione cade** — le due proprieta' che una lista di nomi di
# file non puo' avere.
#
# Misura del cambio: 89 file esenti -> 0, copertura 63% -> 100%, con 76 righe marcate.
EXEMPT_FILES: tuple[str, ...] = ()
EXEMPT_PREFIXES: tuple[str, ...] = ()


def is_exempt(rel: str) -> bool:
    return bool(EXEMPT_FILES) and rel in EXEMPT_FILES or bool(EXEMPT_PREFIXES) and rel.startswith(EXEMPT_PREFIXES)

# --- maschere: ciò che NON è prosa player-facing -----------------------------
# L'ordine conta: i blocchi recintati spariscono prima degli inline, altrimenti un
# backtick dentro un blocco sbilancia il conteggio degli inline.
#
# ⚠️ La chiusura e' opzionale (`|\Z`). Con `` ```.*?``` `` un blocco **aperto e mai
# chiuso** non veniva mascherato affatto: la regex non trovava il delimitatore finale
# e lasciava tutto il resto del file come prosa, facendo fallire il gate per una
# svista di formattazione invece che per un problema di naming.
FENCED = re.compile(r"```.*?(?:```|\Z)", re.DOTALL)

# ⚠️ **Non esiste una maschera per i blocchi di codice indentati, ed e' deliberato.**
# La versione precedente aveva `^(?: {4,}|\t).*$`, che in Markdown non distingue un
# blocco di codice dalla **continuazione di una lista annidata** — e questo repository
# scrive prosa dentro sotto-punti a ogni pagina. Il risultato misurato era un falso
# negativo silenzioso: un sotto-punto che nominava `Bastion` come personaggio passava
# il gate con exit 0.
#
# Per un gate le due direzioni non hanno lo stesso costo. Un falso **positivo** e'
# rumoroso e si chiude mettendo il token in backtick, cioe' facendo esattamente cio'
# che la convenzione chiede. Un falso **negativo** non lo nota nessuno, ed e' il modo
# in cui un gate smette di proteggere restando verde. I blocchi recintati coprono gia'
# il codice che questi documenti scrivono davvero.
INLINE_CODE = re.compile(r"`[^`\n]*`")
HTML_COMMENT = re.compile(r"<!--.*?-->", re.DOTALL)
# Target di link e immagini: `](qualcosa)` — il testo dell'etichetta resta visibile.
LINK_TARGET = re.compile(r"\]\([^)\n]*\)")
BARE_URL = re.compile(r"https?://\S+")
# Percorsi nudi tipo docs/characters/v0.1/flux.md, anche senza backtick.
BARE_PATH = re.compile(r"\b[\w./-]+\.(?:md|cpp|h|py|ts|json|ya?ml|uasset|umap)\b")


def mask(text: str) -> str:
    """Sostituisce i contesti tecnici con spazi, preservando gli offset di riga."""

    def blank(match: re.Match) -> str:
        # Le newline restano: il numero di riga deve continuare a tornare.
        return re.sub(r"[^\n]", " ", match.group(0))

    for pattern in (
        HTML_COMMENT,
        FENCED,
        INLINE_CODE,
        LINK_TARGET,
        BARE_URL,
        BARE_PATH,
    ):
        text = pattern.sub(blank, text)
    return text


NAME_RE = re.compile(r"\b(" + "|".join(LEGACY) + r")\b")


# --- il marcatore di esenzione, riga per riga (#755) --------------------------
#
# ⚠️ **Esiste perche' «zero occorrenze» e' un obiettivo IMPOSSIBILE per una parte dei
# documenti, e chiederlo lo stesso produce o una falsificazione o un gate spento.**
# Misurato eseguendo `#755`: delle 1055 occorrenze nei documenti vivi, **88** stanno
# in frasi che parlano della rinomina STESSA — `D-130` dichiara «`Flux` -> **Gadget**»,
# e bonificarla la fa diventare «`Gadget` -> **Gadget**», cioe' una decisione che non
# si puo' piu' leggere. Altre **169** sono misure datate: «al 2026-08-13 c'erano 1600
# occorrenze di `Flux`» e' un fatto di quel giorno, e riscriverlo lo rende falso.
#
# La forma e' un commento HTML sulla riga PRECEDENTE:
#
#     <!-- rename-exempt: D-130 dichiara la mappatura -->
#     | **D-130** | ... `Flux` -> **Gadget** ... |
#
# 🔑 **Due proprieta' lo distinguono dalle esenzioni per file che sostituisce**:
#   1. e' per **riga**, quindi il resto del documento resta protetto — il Decision Log
#      era esente per intero, e ogni voce nuova poteva nominare un nome ritirato senza
#      che nulla lo dicesse;
#   2. un marcatore su una riga **senza** nomi ritirati e' un **ERRORE**. Un'esenzione
#      che sopravvive al proprio motivo e' esattamente il modo in cui questo gate ha
#      perso il 37% di copertura: qui non puo' succedere in silenzio.
# Due forme, perche' il gate copre due sintassi (#1109):
#   · `<!-- rename-exempt: ... -->`  nei Markdown e nelle NOTE dei YAML, che in questo
#     repository sono markdown incorporato — le `note:` di `parallel-batch.yaml` usano
#     grassetti, backtick e liste esattamente come un `.md`;
#   · `# rename-exempt: ...`         nei COMMENTI YAML, dove un commento HTML sarebbe
#     testo inerte e chi legge il file non capirebbe perche' e' li'.
# La seconda forma non e' ammessa nei Markdown: `#` a inizio riga e' un titolo, e
# accettarla trasformerebbe ogni titolo che contiene «rename-exempt» in un'esenzione.
EXEMPT_MARKER = re.compile(r"<!--\s*rename-exempt:\s*(.+?)\s*-->")
# ⚠️ **Due vincoli, e ognuno chiude un falso negativo misurato.**
#
#   1. Il `#` deve **aprire la riga** (solo spazi prima). Accettandolo ovunque, un valore
#      che *documenta* il marcatore — `note: "per esentare scrivi # rename-exempt: ..."`,
#      la cosa piu' naturale che qualcuno scriva — diventava un marcatore vero ed esentava
#      la riga successiva. Un `#` dentro una stringa quotata **non e' un commento YAML**, e
#      distinguerlo davvero vorrebbe un parser: qui vince la regola conservativa, e la
#      forma «in fondo alla riga» resta disponibile come `<!-- -->`, che ha delimitatori
#      chiusi e non soffre del problema.
#   2. `[^\S\n]` invece di `\s` — spazi ma **non** newline. Con `\s*`, un marcatore senza
#      ragione (`# rename-exempt:`) assorbiva la riga SEGUENTE come propria ragione, e
#      `mask_for` la cancellava: un nome ritirato li' sotto spariva senza che nulla lo
#      dicesse. E' il caso peggiore — un'esenzione che nessuno ha chiesto, su una riga che
#      nessuno stava esentando.
EXEMPT_MARKER_YAML = re.compile(
    r"^[^\S\n]*#[^\S\n]*rename-exempt:[^\S\n]*(.+?)[^\S\n]*$", re.M
)

# Negli YAML valgono **entrambe** le forme, e l'alternanza non e' generosita': le `note:`
# di `parallel-batch.yaml` sono blocchi markdown, quindi chi ci scrive dentro usa la forma
# HTML per riflesso — e trovarla ignorata darebbe un gate rosso senza spiegazione, con
# `stale_markers` cieco nello stesso punto. Nei `.md` invece la forma `#` resta VIETATA:
# `#` a inizio riga e' un titolo, e accettarla trasformerebbe ogni titolo che contiene
# «rename-exempt» in un'esenzione su 407 file.
EXEMPT_MARKER_ANY = re.compile(
    EXEMPT_MARKER.pattern + "|" + EXEMPT_MARKER_YAML.pattern, re.M
)


def exempt_marker_for(path: Path) -> re.Pattern:
    """Il pattern del marcatore che vale per QUESTO file."""
    return EXEMPT_MARKER_ANY if path.suffix in YAML_SUFFIXES else EXEMPT_MARKER

# «Questa riga ha ancora bisogno del marcatore?» — una domanda diversa da «questa riga
# ha prosa player-facing sbagliata?», e vuole un pattern diverso.
#
# Confine a sinistra SEMPRE; a destra o un confine o una MAIUSCOLA. Le tre condizioni
# insieme sono l'unico modo di prendere ciò che serve senza prendere ciò che no:
#
#   `bastion` in un tag di scenario      -> confine su entrambi i lati        ✅
#   `BastionIsPushedLikeAnyone`          -> confine a sx, maiuscola a dx      ✅
#   `FluxRiva` (coppia di eroi)          -> idem                              ✅
#   `Conflux` (fazione)                  -> nessun confine a sinistra         ❌ giusto
#   `rivaluta`, `rivalida`, `privato`    -> minuscola a destra                ❌ giusto
#
# ⚠️ L'ultima riga e' un difetto misurato, non un'ipotesi: con il solo confine a
# sinistra questo pattern contava `rivalidazione` come occorrenza di `Riva`, e la
# verifica di `#755` riportava 46 residui di cui 40 inesistenti.
# ⚠️ **Due rami e nessun `IGNORECASE`**, e la ragione e' un difetto misurato: con
# `re.IGNORECASE` la classe `[A-Z]` matcha **anche le minuscole**, quindi `(?=[A-Z])`
# accettava la `l` di `rivaluta` e il pattern tornava a contare `rivalidazione`.
# Il ramo maiuscolo ammette la maiuscola seguente (nomi composti), quello minuscolo no.
ANY_FORM = re.compile(
    r"\b(?:" + "|".join(LEGACY) + r")(?:\b|(?=[A-Z]))"
    r"|\b(?:" + "|".join(n.lower() for n in LEGACY) + r")\b"
)


def marked_lines(raw_lines: list[str], marker: re.Pattern) -> dict[int, str]:
    # ⚠️ `marker` e' OBBLIGATORIO, e la ragione e' un difetto vero: con un default
    # `EXEMPT_MARKER` questa firma ha lasciato passare un call site su tre — il conteggio
    # dei marcatori in `main()` — che continuava a leggere solo la forma markdown. Un
    # parametro richiesto lo avrebbe reso un `TypeError` invece di un numero sbagliato.
    """`{numero_riga_1based: ragione}` per le righe marcate.

    Il marcatore vale in **due** posizioni, e la seconda non e' una comodita':
      · sulla riga PRECEDENTE — la forma normale, leggibile;
      · in FONDO ALLA RIGA STESSA — obbligatoria dentro una tabella, perche' un
        commento HTML fra due righe di tabella markdown la **spezza in due**. Il
        Decision Log e' una tabella sola e ogni decisione e' una riga: senza questa
        seconda forma, marcare `D-130` significherebbe rompere il documento che si
        sta cercando di preservare.

    Si legge dal RAW e non dal mascherato: `mask()` cancella i commenti HTML, quindi
    dopo la maschera il marcatore non esiste piu'.
    """
    marked: dict[int, str] = {}
    for i, line in enumerate(raw_lines):
        m = marker.search(line)
        if not m:
            continue
        # Se la riga contiene ANCHE un nome ritirato, il marcatore vale per se stessa:
        # e' la forma in-tabella. Altrimenti vale per la riga dopo.
        #
        # ⚠️ Con ANY_FORM e non `NAME_RE`, per la stessa ragione di `stale_markers`:
        # `NAME_RE` non vede `BastionIgnoresAllPushes` — concatenato, nessun confine a
        # destra — quindi attribuiva il marcatore alla riga SUCCESSIVA, che era vuota,
        # e lo dichiarava stantio. Un marcatore giusto, letto male, segnalato come rotto.
        senza_marcatore = marker.sub("", line)
        # Con l'alternanza i gruppi sono due e uno dei due e' `None`: la ragione e' il
        # primo non nullo. Con il pattern singolo il comportamento non cambia.
        reason = next((gr for gr in m.groups() if gr is not None), "")
        if ANY_FORM.search(senza_marcatore):
            marked[i + 1] = reason
        elif i + 1 < len(raw_lines):
            marked[i + 2] = reason
        else:
            marked[i + 1] = reason  # ultima riga del file: vale per se'
    return marked


def mask_for(path: Path, text: str) -> str:
    """`mask()`, piu' la RAGIONE del marcatore YAML.

    ⚠️ **Simmetria con la forma HTML, non un'aggiunta.** `mask()` azzera i commenti HTML,
    quindi la ragione di un `<!-- rename-exempt: D-130 dichiara «Flux» -> Gadget -->` non
    viene mai scansionata. Il marcatore YAML vive in un commento `#`, che `mask()` non
    tocca: senza questa riga la ragione **piu' naturale** — quella che nomina la mappatura,
    cioe' l'esempio che il modulo stesso porta piu' sopra — renderebbe rosso il gate
    proprio mentre si esenta una riga. L'autore non avrebbe modo di capirlo, perche' il
    gate indicherebbe la riga del marcatore invece della riga esentata.
    """
    text = mask(text)
    if path.suffix in YAML_SUFFIXES:
        text = EXEMPT_MARKER_YAML.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)
    return text


def scan(path: Path) -> list[tuple[int, str, str]]:
    """Ritorna [(riga, nome, testo)] delle occorrenze player-facing NON marcate."""
    try:
        raw = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return []

    masked = mask_for(path, raw)
    raw_lines = raw.splitlines()
    marked = marked_lines(raw_lines, exempt_marker_for(path))
    hits: list[tuple[int, str, str]] = []
    for lineno, line in enumerate(masked.splitlines(), start=1):
        if lineno in marked:
            continue
        for match in NAME_RE.finditer(line):
            original = raw_lines[lineno - 1].strip()
            hits.append((lineno, match.group(1), original[:120]))
    return hits


def stale_markers(path: Path) -> list[tuple[int, str, str]]:
    """Marcatori la cui riga NON contiene piu' un nome ritirato: vanno tolti.

    E' la meta' che rende il meccanismo onesto. Senza, un marcatore messo per una
    ragione vera resterebbe dopo che la ragione e' caduta, e il gate avrebbe di nuovo
    delle zone cieche — solo piu' piccole e piu' difficili da trovare.
    """
    try:
        raw = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return []
    raw_lines = raw.splitlines()
    stale = []
    for lineno, reason in marked_lines(raw_lines, exempt_marker_for(path)).items():
        riga = raw_lines[lineno - 1] if lineno - 1 < len(raw_lines) else ""
        # Sul RAW, non sul mascherato: un token in backtick e' comunque una ragione
        # valida per marcare, ed e' il caso piu' frequente (`Flux.ArcPulse`).
        #
        # ⚠️ E con ANY_FORM, non con `NAME_RE`. Misurato eseguendo `#755`: questo
        # controllo dichiarava stantii due marcatori legittimi, perche' `NAME_RE` e'
        # case-sensitive e con confini su entrambi i lati — non vedeva `` `bastion` ``
        # minuscolo ne' `BastionIsPushedLikeAnyone` concatenato. Un falso positivo qui
        # e' peggio di un'imprecisione: costringe a togliere un marcatore che serve.
        if not ANY_FORM.search(riga):
            stale.append((lineno, reason, riga.strip()[:100]))
    return stale


# ⚠️ I file di governance stanno FUORI da `docs/`, e questo gate li ha mancati fino
# al 2026-08-13: `AGENTS.md` ha continuato a dichiarare «Roster v0.1 corrente: Flux ·
# Riva · Bastion · Vektor» per un giorno intero dopo D-120, mentre ogni documento
# sotto `docs/` era stato corretto — ed e' il primo file che CLAUDE.md §1 impone di
# leggere. Un gate che copre la periferia e non il centro e' peggio di nessun gate:
# rassicura. Si elencano per nome perche' la root contiene anche kit datati, che
# sarebbero esenti per la stessa ragione dei registri.
ROOT_GOVERNANCE = ("CLAUDE.md", "AGENTS.md", "README.md")


# Le estensioni protette. `.yaml` entra con `#1109`, e la ragione e' misurata: sotto
# `docs/` i file YAML contengono **prosa lunga** — le `note:` di `parallel-batch.yaml`,
# gli `steps` di `editor-sessions.yaml`, i `notes` del feature registry — scritta in
# markdown e letta da persone. Il gate ne ha ignorate 5 per giorni, e in una di esse un
# nome ritirato sopravviveva in una frase che descriveva **il presente**.
#
# 🔑 **La maschera markdown funziona sugli YAML senza modifiche, ed e' un fatto misurato
# invece che una speranza**: delle 14 occorrenze grezze trovate all'apertura di `#1109`,
# **10** erano gia' dentro backtick o fence e cadevano da sole. Non serve una maschera
# YAML-specifica perche' in questo repository lo YAML *e'* un contenitore di markdown.
# ⚠️ **`.yml` e `.yaml` insieme, e non e' zelo.** Oggi sotto `docs/` non c'e' nessun
# `.yml` — misurato — ed e' esattamente la condizione in cui la lacuna non e' rossa e si
# legge come «pulito»: la stessa forma per cui `#1109` e' esistita. `BARE_PATH` conosce
# gia' entrambe le grafie (`ya?ml`), quindi il repository sa che la seconda e' viva.
PROTECTED_SUFFIXES = ("*.md", "*.yaml", "*.yml")
YAML_SUFFIXES = (".yaml", ".yml")

# ⚠️ **Limite noto, e l'escape non e' quello dei Markdown.** Negli YAML la maschera
# markdown copre commenti e note, che e' dove vive la prosa di questo repository — ma un
# **valore scalare** che debba legittimamente contenere un id legacy (un `tests:` che
# cita `Heroes.Vektor.InterceptShotIsPredictive`, per dire) non puo' essere messo fra
# backtick: cambierebbe il dato che il tooling legge. Oggi non capita — misurato, il gate
# e' verde — ma il giorno che capita l'unica via e' il marcatore `# rename-exempt:` sulla
# riga precedente. Che funziona **anche** se la ragione nomina l'eroe: la sua ragione e'
# mascherata come quella della forma HTML, ed e' il difetto che `#1170` ha corretto prima
# che qualcuno lo incontrasse.


def docs_files() -> list[Path]:
    files = []
    for name in ROOT_GOVERNANCE:
        path = REPO / name
        if path.is_file():
            files.append(path)
    for pattern in PROTECTED_SUFFIXES:
        for path in sorted(DOCS.rglob(pattern)):
            rel_parts = path.relative_to(DOCS).parts
            if rel_parts and rel_parts[0] in EXCLUDED_DIRS:
                continue
            files.append(path)
    return sorted(set(files))



# --- la Wiki pubblicata (repo separato, D-076: il clone E' la fonte) ----------
#
# La Wiki e' cio' che un giocatore legge davvero, ed era rimasta indietro: al
# 2026-08-13, con `docs/` gia' bonificata, dichiarava ancora il roster legacy in
# **193** punti su 24 pagine. Nessun gate la guardava.
#
# ⚠️ Qui esiste un token che nel repository non esiste: nei wiki-link
# `[[Etichetta|target]]` il **target e' il nome del file della pagina**, e resta
# legacy per scelta — rinominarlo romperebbe i link e la cronologia della pagina
# pubblicata, esattamente come per le schede di `docs/characters/v0.1/`. Senza
# questa maschera il gate sarebbe rosso per sempre su ~48 link corretti, e un
# gate che non puo' diventare verde viene disattivato.
WIKI_LINK_TARGET = re.compile(r"\[\[[^\]\n]*?\\?\|([^\]\n]+)\]\]")


def mask_wiki(text: str) -> str:
    """`mask()` piu' i target dei wiki-link, che sono nomi di pagina, non prosa."""
    text = mask(text)

    def blank_target(match: re.Match) -> str:
        whole, target = match.group(0), match.group(1)
        start = match.start(1) - match.start(0)
        return whole[:start] + " " * len(target) + whole[start + len(target):]

    return WIKI_LINK_TARGET.sub(blank_target, text)


def scan_wiki(path: Path) -> list[tuple[int, str, str]]:
    try:
        raw = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return []
    raw_lines = raw.splitlines()
    hits: list[tuple[int, str, str]] = []
    for lineno, line in enumerate(mask_wiki(raw).splitlines(), start=1):
        for match in NAME_RE.finditer(line):
            hits.append((lineno, match.group(1), raw_lines[lineno - 1].strip()[:120]))
    return hits


def wiki_files(root: Path) -> list[Path]:
    return [p for p in sorted(root.rglob("*.md")) if ".git" not in p.parts]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="esce 1 se un file protetto contiene prosa legacy",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="elenca anche le occorrenze nei registri esenti, file per file",
    )
    parser.add_argument(
        "--wiki-root",
        metavar="PATH",
        help="clone della Wiki pubblicata: le sue pagine sono tutte protette",
    )
    args = parser.parse_args()

    files = docs_files()
    enforced_hits: dict[str, list[tuple[int, str, str]]] = {}
    backlog_hits: dict[str, list[tuple[int, str, str]]] = {}
    enforced_files = 0

    for path in files:
        rel = path.relative_to(REPO).as_posix()
        exempt = is_exempt(rel)
        if not exempt:
            enforced_files += 1
        hits = scan(path)
        if not hits:
            continue
        if exempt:
            backlog_hits[rel] = hits
        else:
            enforced_hits[rel] = hits

    total_files = len(files)
    backlog_count = sum(len(h) for h in backlog_hits.values())

    perimetro = ("governance di root + docs/, **nessuna cartella esclusa**"
                 if not EXCLUDED_DIRS
                 else f"governance di root + docs/, esclusi {'/'.join(EXCLUDED_DIRS)}")
    # ⚠️ **Per ESTENSIONE, e non e' cosmesi** (`#1109`): questo gate ha stampato per giorni
    # un totale unico che sommava 375 `.md` e **zero** `.yaml`, e quel numero si leggeva
    # come copertura. Un conteggio aggregato rende «nessuna occorrenza» e «nessun file
    # letto» indistinguibili — che e' la definizione di un gate cieco che rassicura.
    per_suffix = Counter(f.suffix or "(senza estensione)" for f in files)
    dettaglio = " · ".join(f"{n} {s}" for s, n in sorted(per_suffix.items()))
    print(f"File normativi analizzati: {total_files} ({perimetro})")
    print(f"  per estensione: {dettaglio}")
    if total_files:
        print(f"Protetti dal gate: {enforced_files}/{total_files} file"
              f" — copertura {enforced_files / total_files:.0%}"
              f" · esenti (registri datati): {total_files - enforced_files}")
    else:
        # Checkout parziale o `docs/` spostata: meglio dirlo che dividere per zero.
        print("Nessun file normativo trovato sotto docs/ — controlla il checkout."
              f" (estensioni cercate: {', '.join(PROTECTED_SUFFIXES)})")
    print()

    if enforced_hits:
        print("ERRORE — prosa legacy in file protetti dal gate:")
        for rel, hits in enforced_hits.items():
            for lineno, name, text in hits:
                print(f"  {rel}:{lineno}: {name} -> {text}")
        print()
    else:
        print("OK — nessun nome legacy come prosa player-facing nei file protetti.")
        print()

    print(f"Nei registri esenti: {backlog_count} occorrenze in {len(backlog_hits)} file.")
    if backlog_hits:
        ordered = sorted(backlog_hits.items(), key=lambda kv: -len(kv[1]))
        if args.all:
            for rel, hits in ordered:
                print(f"  {rel} ({len(hits)})")
                for lineno, name, text in hits:
                    print(f"      :{lineno}: {name} -> {text}")
        else:
            print("  primi dieci per densita' (--all per l'elenco completo):")
            for rel, hits in ordered[:10]:
                print(f"      {len(hits):4d}  {rel}")
    print()
    print("Le occorrenze nei registri datati non fanno fallire il gate e non si"
          " bonificano: si annotano accanto all'affermazione. Il numero qui sopra si"
          " rimisura eseguendo lo script.")

    # --- Wiki pubblicata, se il clone e' stato indicato ----------------------
    wiki_hits: dict[str, list[tuple[int, str, str]]] = {}
    if args.wiki_root:
        root = Path(args.wiki_root)
        if not root.is_dir():
            print(f"\nERRORE — clone Wiki non trovato: {root}")
            return 2
        pages = wiki_files(root)
        for path in pages:
            hits = scan_wiki(path)
            if hits:
                wiki_hits[path.relative_to(root).as_posix()] = hits
        print()
        print(f"Wiki pubblicata: {len(pages)} pagine, tutte protette"
              f" (i target dei wiki-link sono nomi di pagina, non prosa).")
        if wiki_hits:
            n = sum(len(h) for h in wiki_hits.values())
            print(f"ERRORE — prosa legacy in {n} punti su {len(wiki_hits)} pagine:")
            for rel, hits in sorted(wiki_hits.items(), key=lambda kv: -len(kv[1])):
                for lineno, name, text in hits:
                    print(f"  {rel}:{lineno}: {name} -> {text}")
        else:
            print("OK — nessun nome legacy come prosa player-facing nella Wiki.")

    # --- marcatori stantii: la meta' che rende il meccanismo onesto (#755) ---------
    #
    # Un marcatore su una riga senza nomi ritirati non e' innocuo: e' una zona cieca
    # che nessuno vede. Si controlla su TUTTI i file analizzati, esenti compresi —
    # anzi soprattutto li', perche' e' dove i marcatori vivono.
    stale: dict[str, list[tuple[int, str, str]]] = {}
    marked_total = 0
    for path in files:
        rel = path.relative_to(REPO).as_posix()
        try:
            marked_total += len(marked_lines(
                path.read_text(encoding="utf-8").splitlines(), exempt_marker_for(path)))
        except (OSError, UnicodeDecodeError):
            pass
        s = stale_markers(path)
        if s:
            stale[rel] = s

    print()
    if stale:
        n = sum(len(v) for v in stale.values())
        print(f"ERRORE — {n} marcatori `rename-exempt` STANTII: la riga che esentano non")
        print("         contiene piu' un nome ritirato, quindi il marcatore va TOLTO.")
        for rel, voci in sorted(stale.items()):
            for lineno, reason, testo in voci:
                print(f"  {rel}:{lineno}  «{reason}»")
                print(f"      riga: {testo}")
    elif marked_total:
        print(f"Marcatori `rename-exempt`: {marked_total}, tutti su righe che ne hanno ancora")
        print("bisogno. Esentano una RIGA, non un file: il resto del documento resta protetto.")

    if args.check and (enforced_hits or wiki_hits or stale):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
