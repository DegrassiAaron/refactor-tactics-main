#!/usr/bin/env python3
"""Gate anti-deriva: un link markdown punta a qualcosa che esiste, ed e' etichettato per quello che e'.

Gemello di `check-docs-symbols.py`. Quello nasce dal difetto D1 — un documento che cita classi
inesistenti; questo dal consolidamento del 2026-08-08 (PR #239), che spostando 25 sorgenti in
`docs/archive/src/` ha rotto link in **tre** modi, di cui solo il primo e' ovvio:

  1. i riferimenti **entranti** puntavano ancora alla vecchia posizione;
  2. i file spostati erano scesi di un livello, quindi anche i loro link **uscenti** erano sbagliati;
  3. **36 etichette** mostravano il path vecchio mentre il target era gia' corretto — link funzionante,
     testo bugiardo. Nessun controllo che guardi solo i target lo vede.

Piu' una quarta categoria, che il gate controlla per prevenzione: un target che esiste **solo in locale**.
Il documento funziona sulla macchina di chi l'ha scritto e su nessun'altra.

Controlla anche se stesso, ed e' il motivo per cui ignora i link **citati**: un documento *sui link*
contiene link d'esempio, e alla prima esecuzione questo gate ha segnalato i due esempi nel proprio
README. Stessa ragione per cui salta i blocchi recintati — dove un `![img](...)` e' un modello da
incollare altrove, con i path relativi alla **destinazione**, non a chi lo contiene.

Uso:
    python scripts/check-docs-links.py             # dalla radice del repo
    python scripts/check-docs-links.py --known     # elenca il debito dichiarato e la sua ragione

Piu' una quinta, che non riguarda i link ma vive qui perche' questo e' l'unico gate che **legge** i
documenti: i **marker di conflitto git** rimasti dentro un file. Vedi `CONFLITTO_RE` per il perche' sta
in questo script e per quale dei tre marker e' deliberatamente escluso.

E una sesta, per la stessa ragione: due voci del **Decision Log con lo stesso `D-nnn`**. Nessun altro
controllo puo' vederla — entrambe le righe esistono e ogni link risolve — ma rende ambiguo ogni rimando
del repository. Vedi `DECISIONE_RE`.

Esce con codice 1 se un link e' rotto, se un'etichetta mente, se un target esiste solo in locale,
se un documento contiene marker di conflitto, se un ID di decisione e' duplicato, oppure se una voce
di DEBITO_NOTO non e' piu' vera.

Cosa NON controlla, deliberatamente:
  - gli **ancoraggi** (`#sezione`): la slugificazione di GitHub su titoli con accenti, emoji e codice
    inline non e' riproducibile in poche righe, e un gate che sbaglia viene disattivato al terzo falso
    positivo. Meglio stretto e creduto che largo e ignorato;
  - gli **URL esterni**: richiederebbero rete, quindi un gate non deterministico;
  - i file **non tracciati**: si controlla cio' che e' nel repository, non la propria copia di lavoro.
    E' anche cio' che rende sensato il controllo #3 (vedi `NON_VERSIONATO`);
  - ⛔ **il clone della Wiki**, da **D-180** (2026-08-21). Dal 2026-08-10 (D-076) le pagine di gioco
    vivono in un repository separato che `git ls-files` non elenca, e fino al 2026-08-20 `--wiki-root`
    estendeva il gate a quell'area. Non ci arriva piu'. Escono **quattro** regole, non una:

      * i `[[Nome|slug]]` risolti **per nome** e non per percorso;
      * i **nomi di pagina globalmente unici** — due pagine omonime si contendono lo stesso URL
        `/wiki/<stem>`. ⚠️ Ci si appoggiava `wiki_page_by_name()` in `feature_registry.py`, che
        sceglie la prima in ordine alfabetico: una collisione manda il blocco `RT_FEATURE_STATUS`
        sulla pagina sbagliata, e adesso **nessuno la segnala**;
      * le **immagini** risolte dalla radice del clone, perche' l'URL di una pagina Wiki e' piatto;
      * le **fonti normative** citate in backtick — la regola nata il 2026-08-20 dopo che lo split
        `080d2e98` aveva lasciato **11 percorsi morti su 12 pagine**.

    Sul clone resta il solo `check-docs-naming.py --check --wiki-root <clone>`, che guarda i **nomi**
    dei file e non i link. Non riproporre `--wiki-root` qui senza riaprire D-180: ricomparirebbe come
    «piccola aggiunta» la superficie che e' stata tolta di proposito.
"""
import argparse
import os
import re
import subprocess
import sys
import urllib.parse

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# [testo](target)  ·  ![alt](img)  ·  [`path`](target) — con titolo opzionale fra virgolette.
# Il `!` dell'immagine sta PRIMA della parentesi quadra, e va incluso nel match: se resta fuori,
# `_is_quoted` guarda il `!` invece del backtick che lo precede e un'immagine citata come esempio
# viene scambiata per un link vero. Difetto trovato dal gate su se stesso, di nuovo.
LINK_RE = re.compile(r'(!?)\[(`?)([^\]]*?)(`?)\]\(\s*<?([^)>\s]+)>?(?:\s+"[^"]*")?\s*\)')

SKIP_SCHEMES = ("http://", "https://", "mailto:", "ftp://", "tel:", "data:")

FENCE_RE = re.compile(r"^([ \t]*)(`{3,}|~{3,}).*?^\1?\2[ \t]*$", re.M | re.S)

# Marker di conflitto git rimasti in un documento. Aggiunto il 2026-08-10, dopo che tre di essi hanno
# vissuto in `docs/OPEN_DECISIONS.md` su `main` per cinque ore e mezza, 117 commit e 34 PR mergiate:
# li ha introdotti un merge risolto a mano (`cc5f0e5`) e non li ha visti nessuno, perche' i `.md` sono
# l'unico formato del repository che nessun parser attraversa. Un marker in un `.cpp` non compila, uno
# in `feature-registry.yaml` fa fallire il validator; uno in un documento si legge come testo.
#
# Cerca SOLO le due righe che in Markdown non significano niente. La terza — sette segni di uguale —
# e' deliberatamente esclusa: e' anche la sottolineatura di un titolo **Setext**, e un gate che sbaglia
# viene disattivato al terzo falso positivo. Se ci sono le altre due, il conflitto c'e' comunque.
CONFLITTO_RE = re.compile(r"^(?:<{7}|>{7}) ", re.M)

# Sesta categoria, stessa ragione della quinta: questo e' l'unico gate che legge i documenti.
# Un ID di decisione duplicato nel Decision Log rende **ambiguo ogni rimando** `[D-nnn]` del
# repository — e non lo vede nessun altro controllo, perche' entrambe le righe esistono e ogni link
# risolve al file giusto. Aggiunto il 2026-08-11 dopo l'undicesima collisione di contatore, che e'
# stata anche la prima ad **atterrare su `main`**: due decisioni diverse hanno portato `D-091` per
# quattro commit. Le dieci precedenti erano state trovate perche' qualcuno stava scrivendo la
# decisione successiva, cioe' dal prossimo autore e sempre in ritardo di un merge — vedi la nota di
# `D-094`. Il rimedio raccomandato allora ("rileggere `main` prima del merge") e' una disciplina
# umana; questo e' tre righe di script.
# ⚠️ Il grassetto e la barratura sono OPZIONALI, e non e' un dettaglio di stile.
# La prima stesura di questa regex chiedeva `**` subito dopo la barra, e copriva **93 righe su 102**:
# perdeva `D-001`..`D-008` (righe antiche, scritte senza grassetto) e `~~**D-044**~~` (ritirata,
# barrata). Trovato in code review prima del merge.
# Le due categorie perse erano **le peggiori possibili** per questo gate: `D-044` e' un numero
# lasciato deliberatamente vuoto perche' un riuso non sovrascrivesse la storia, cioe' esattamente il
# buco che invita un `D-044` nuovo — e li' il gate avrebbe visto una riga sola e riportato zero
# duplicati. Un falso negativo proprio sul caso per cui e' stato scritto.
DECISIONE_RE = re.compile(r"^\|\s*~{0,2}\s*\*{0,2}(D-\d{3})\*{0,2}\s*~{0,2}\s*\|", re.M)
DECISION_LOG = "docs/decisions/RT_PDR_00_Decision_Log.md"


def decisioni_duplicate():
    """Gli ID che compaiono piu' di una volta come RIGA della tabella del Decision Log.

    Solo l'inizio riga: `[D-091](...)` dentro il corpo di un'altra decisione e' un rimando, ed e'
    esattamente cio' che deve continuare a funzionare — il duplicato da trovare e' la voce, non la
    citazione.

    E solo FUORI dai blocchi recintati, per la stessa ragione di ogni altro controllo di questo file:
    un documento che *spiega il formato della tabella* contiene righe d'esempio, e un esempio che
    riusa un ID vivo verrebbe contato come duplicato. E' il difetto che questo gate ha gia' trovato
    su se stesso due volte — i link d'esempio nel proprio README, e il `!` dell'immagine citata — e
    che qui e' stato **trovato in code review invece che in produzione**: `meglio stretto e creduto
    che largo e ignorato`, e un gate che sbaglia viene disattivato al terzo falso positivo.

    Restituisce [(id, [righe])], vuoto se il file non c'e' o non e' leggibile: un repository senza
    Decision Log non fallisce per questo, e un file illeggibile non deve uccidere l'intero gate
    prima che possa riportare i link rotti — stessa clausola del ciclo principale."""
    abs_log = os.path.join(REPO, DECISION_LOG)
    try:
        text = open(abs_log, encoding="utf-8").read()
    except (OSError, UnicodeDecodeError):
        return []
    fences = _fence_spans(text)
    visti = {}
    for m in DECISIONE_RE.finditer(text):
        if any(a <= m.start() < b for a, b in fences):
            continue
        visti.setdefault(m.group(1), []).append(text[: m.start()].count("\n") + 1)
    return sorted((k, v) for k, v in visti.items() if len(v) > 1)


def _fence_spans(text):
    """Le regioni dentro un blocco di codice recintato: li' un link e' un esempio, non un link."""
    return [(m.start(), m.end()) for m in FENCE_RE.finditer(text)]


def _is_quoted(text, start, end):
    """Il link e' avvolto per intero in un code span — cioe' e' citato, non usato.

    Serve perche' un documento *sui link* contiene link d'esempio: questo gate ne ha due nel
    proprio README, e alla prima esecuzione si e' segnalato da solo. Regola volutamente stretta:
    solo il caso «backtick subito prima di `[` e subito dopo `)`». Non prova a interpretare la
    grammatica dei code span, che con le etichette in backtick — `[`path`](path)`, la forma piu'
    diffusa nel repo — diventa ambigua e cancellerebbe proprio i controlli che servono.
    """
    before = text[start - 1] if start else ""
    after = text[end] if end < len(text) else ""
    return before == "`" and after == "`"

# Debito dichiarato. Una voce qui NON e' un'esenzione: e' una promessa datata, e il gate
# la verifica al contrario — se il link torna a funzionare, la voce va tolta e il gate lo
# pretende. Un allowlist che non sa invalidarsi diventa il posto dove i difetti vanno a
# morire in silenzio.
#
# **Oggi e' vuoto, e va tenuto vuoto.** Il meccanismo resta perche' un gate senza via d'uscita
# viene disattivato *per intero* al primo blocco legittimo, e perche' e' verificato per mutazione.
# La prima voce candidata si e' rivelata inesistente: le dieci infografiche di
# `CLAUDE_INTEGRATION_PROMPT.md` sembravano link rotti a uno script che non gestiva i blocchi
# recintati. Stanno dentro ```md, sono *markdown suggerito* da incollare nelle pagine wiki, e i
# loro path sono relativi alla **destinazione**. Erano corrette dall'inizio.
DEBITO_NOTO = {}


def tracked_files():
    """I file versionati, secondo git. Non la working copy: quella e' solo tua."""
    out = subprocess.run(
        ["git", "-C", REPO, "ls-files", "-z"],
        capture_output=True, check=True, text=True, encoding="utf-8",
    ).stdout
    return {p.replace("\\", "/") for p in out.split("\0") if p}


def rel(abs_path):
    return os.path.relpath(abs_path, REPO).replace("\\", "/")


def looks_like_path(label):
    """Un'etichetta che *afferma* un percorso, e che quindi puo' mentire."""
    return label.startswith(("../", "./", "docs/"))


def label_resolves(label, base):
    """Un'etichetta-percorso e' vera se esiste — dalla radice del repo se parte per `docs/`,
    altrimenti dalla cartella del documento. Entrambi gli stili sono in uso e leciti."""
    root = REPO if label.startswith("docs/") else base
    return os.path.exists(os.path.normpath(os.path.join(root, label)))


def main(args):
    tracked = tracked_files()
    if not tracked:
        print("ERRORE: git non elenca file — percorso sbagliato?", file=sys.stderr)
        return 2
    tracked_dirs = {d for p in tracked for d in _parents(p)}

    rotti, etichette, non_versionati, conflitti = [], [], [], []
    controllati = 0
    debito_visto = {k: 0 for k in DEBITO_NOTO}

    for src in sorted(f for f in tracked if f.endswith(".md")):
        abs_src = os.path.join(REPO, src)
        try:
            text = open(abs_src, encoding="utf-8").read()
        except (OSError, UnicodeDecodeError):
            continue
        base = os.path.dirname(abs_src)
        fences = _fence_spans(text)

        # Dentro un blocco recintato un marker e' un esempio, non un residuo: stessa esenzione dei link.
        for m in CONFLITTO_RE.finditer(text):
            if any(a <= m.start() < b for a, b in fences):
                continue
            riga = text[: m.start()].count("\n") + 1
            conflitti.append((src, riga, text[m.start():].split("\n", 1)[0][:40]))

        for m in LINK_RE.finditer(text):
            _, _, label, _, target = m.groups()
            if target.startswith(SKIP_SCHEMES) or target.startswith("#"):
                continue
            if _is_quoted(text, m.start(), m.end()):
                continue
            if any(a <= m.start() < b for a, b in fences):
                continue
            bare = urllib.parse.unquote(target.split("#", 1)[0])
            if not bare:
                continue
            controllati += 1
            riga = text[: m.start()].count("\n") + 1
            risolto = os.path.normpath(os.path.join(base, bare))
            rel_target = rel(risolto)

            if not os.path.exists(risolto):
                if src in DEBITO_NOTO:
                    debito_visto[src] += 1
                else:
                    rotti.append((src, riga, target))
                continue

            # Esiste sul disco: ma esiste anche per gli altri?
            if rel_target not in tracked and rel_target not in tracked_dirs:
                non_versionati.append((src, riga, target))

            # L'etichetta afferma un percorso diverso dal target, e quel percorso non esiste.
            # `[../balance/](../balance/README.md)` e' uno stile legittimo — la cartella esiste —
            # e non va segnalato: sarebbe il falso positivo che fa disattivare il gate.
            lab = label.strip()
            if looks_like_path(lab) and lab.split("#")[0] != bare:
                if not label_resolves(lab.split("#")[0], base):
                    etichette.append((src, riga, lab, target))

    if args.known:
        print(f"Debito dichiarato ({len(DEBITO_NOTO)}):")
        for f, why in DEBITO_NOTO.items():
            print(f"    {f}\n        {why}\n        link rotti tollerati ora: {debito_visto[f]}")
        print()

    duplicate = decisioni_duplicate()

    stale = [f for f, n in debito_visto.items() if n == 0]
    print(f"Link relativi controllati: {controllati} · file markdown versionati: "
          f"{sum(1 for f in tracked if f.endswith('.md'))}")

    if not (rotti or etichette or non_versionati or stale or conflitti or duplicate):
        tollerati = sum(debito_visto.values())
        coda = f" ({tollerati} rotti tollerati come debito dichiarato)" if tollerati else ""
        print(f"OK — ogni link risolve, ogni etichetta dice il vero{coda}.")
        return 0

    print()
    # Per primo: gli altri difetti rendono un documento impreciso, questo lo rende rotto.
    if conflitti:
        print(f"FALLITO — {len(conflitti)} marker di conflitto git rimasti in un documento:\n")
        for f, n, testo in conflitti:
            print(f"  {f}:{n}\n      {testo}")
        print("\n  Un merge e' stato risolto a meta' e committato. Apri il file, tieni cio' che serve")
        print("  di entrambi i lati — spesso sono due sezioni diverse che devono coesistere, non due")
        print("  versioni della stessa — e togli le righe di marcatura.\n")

    if duplicate:
        print(f"FALLITO — {len(duplicate)} ID di decisione duplicati nel Decision Log:\n")
        for did, righe in duplicate:
            print(f"  {DECISION_LOG}\n      {did} alle righe {', '.join(str(r) for r in righe)}")
        print("\n  Due decisioni con lo stesso ID rendono ambiguo ogni rimando [D-nnn] del repository.")
        print("  Rinumera quella atterrata per SECONDA al primo ID libero, spostala in coda alla tabella")
        print("  e riscrivi solo i SUOI rimandi, per coppia (file, riga): una sed globale rompe in")
        print("  silenzio quelli dell'altra. Poi registra la rinumerazione nella sezione 'Note'.\n")

    if rotti:
        print(f"FALLITO — {len(rotti)} link a un target inesistente:\n")
        for f, n, t in rotti:
            print(f"  {f}:{n}\n      -> {t}")
        print("\n  Se hai spostato un file, ricorda i tre passaggi: riferimenti entranti,")
        print("  link uscenti dai file spostati (scendono di un livello), etichette.\n")

    if etichette:
        print(f"FALLITO — {len(etichette)} etichette che dichiarano un percorso inesistente:\n")
        for f, n, lab, t in etichette:
            print(f"  {f}:{n}\n      etichetta: {lab}\n      target:    {t}")
        print("\n  Il link funziona, il testo no. Allinea l'etichetta al target — oppure usa")
        print("  un'etichetta che non sia un percorso, se volevi scrivere altro.\n")

    if non_versionati:
        print(f"FALLITO — {len(non_versionati)} target esistono in locale ma NON sono versionati:\n")
        for f, n, t in non_versionati:
            print(f"  {f}:{n}\n      -> {t}")
        print("\n  Funziona sulla tua macchina e su nessun'altra. O lo committi, o togli il link.\n")

    if stale:
        print(f"FALLITO — {len(stale)} voci di DEBITO_NOTO non sono piu' vere:\n")
        for f in stale:
            print(f"  {f}\n      non ha piu' link rotti: togli la voce da DEBITO_NOTO.")
        print("\n  Un debito che si e' risolto e resta dichiarato nasconde il prossimo.\n")

    return 1


def _parents(path):
    """Tutte le directory che contengono `path`, in forma relativa al repo."""
    d = os.path.dirname(path)
    while d:
        yield d
        d = os.path.dirname(d)


def _parse_args():
    """Gli argomenti passano da `argparse`, e non da `"--x" in sys.argv`.

    ⛔ **Un argomento sconosciuto deve costare un errore, non essere ignorato.** La lettura a mano
    guardava le stringhe che conosceva e lasciava cadere tutto il resto, quindi `--wikiroot /clone`
    — un refuso di una riga — eseguiva la scansione del solo repository e stampava un verdetto
    normale: chi l'aveva scritto credeva di aver controllato anche il clone. E' la stessa forma di
    difetto che questo gate esiste per trovare, applicata a se stesso.

    Trovato in code review su PR #1256, che ha anche misurato la contraddizione peggiore: il
    comando registrato come **prova** di D-180 era `check-docs-links.py --check`, e `--check` non
    e' mai esistito su questo script. Funzionava perche' veniva buttato via. `parse_args()` lo
    rifiuta, e i gate fratelli — `check-docs-naming.py`, `docs_inventory.py` — lo fanno gia'.
    """
    ap = argparse.ArgumentParser(
        prog="check-docs-links.py",
        description="Gate anti-deriva sui link dei documenti versionati. Senza argomenti esegue "
                    "il controllo ed esce 1 se qualcosa e' rotto.")
    ap.add_argument("--known", action="store_true",
                    help="elenca il debito dichiarato e la sua ragione")
    # `--wiki-root` non e' un argomento: e' un messaggio d'errore. Registrarlo qui invece di
    # lasciarlo cadere fra gli sconosciuti serve a dire **perche'** e' sparito, a chi rilancia il
    # comando documentato fino al 2026-08-20 — vedi D-180.
    ap.add_argument("--wiki-root", metavar="PATH", help=argparse.SUPPRESS)
    args = ap.parse_args()
    if args.wiki_root is not None:
        ap.error("--wiki-root non esiste piu': dal 2026-08-21 (D-180) questo gate si ferma al "
                 "repository. Sul clone della Wiki resta il solo "
                 "`check-docs-naming.py --check --wiki-root`; i link, i nomi duplicati e le fonti "
                 "normative non li controlla piu' nessuno.")
    return args


if __name__ == "__main__":
    sys.exit(main(_parse_args()))
