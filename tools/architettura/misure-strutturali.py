# -*- coding: utf-8 -*-
"""Le misure di E50, con il comando che le genera — e un asse che l'audit non aveva.

    Uso:  python tools/architettura/misure-strutturali.py [--ref <git-ref>] [--base <git-ref>]
                                                          [--soglia N] [--markdown] [--json <file>]

Senza argomenti misura l'albero di lavoro. Con `--base` stampa il confronto fra due commit, che e'
la forma in cui #1818 chiede di riportare ogni fetta.

## Perche' esiste

L'audit del 2026-08-30 (`docs/roadmap/plans/architecture-hardening-spec-panel-2026-08-30.md`) chiude
dicendo che la metrica che conta per E50 non e' il punteggio SOLID: e' **quanti test hanno bisogno di un
mondo**, «74 su 153», e che «va rimisurata a ogni fetta e riportata in #1818».

Il numero c'e'. Il **comando** che lo produce no. Rimisurando su `b45c314b` — lo stesso commit dell'audit —
il denominatore torna esatto (153 file di test), i numeratori no: `SpawnActor` da' 67, `UWorld` da' 80,
`ARTTurnManager` da' 62 contro i 66 dichiarati. Otto varianti provate, nessuna riproduce 74 e 66.

Non e' un errore di chi ha misurato: e' che una misura senza il suo comando **non e' confrontabile con la
prossima**. Il delta fra due misure prese con due grep leggermente diversi e' rumore che sembra un
progresso. Questo file fissa i comandi, cosi' che «prima» e «dopo» misurino la stessa cosa.

E' lo stesso difetto che #1818 ha gia' trovato da se' nel commento del 2026-08-31 — «l'audit dichiara 19
`GetAllActorsOfClass`, le chiamate vere sono 11, le altre 8 sono in commento» — applicato alle metriche
rimaste.

## 🔴 LE RIGHE DEL FILE NON SONO LE RIGHE DEL CODICE

Misurato su `b45c314b..bbb20f16` (2026-08-30 -> 2026-09-03), sui tre file di `ARTTurnManager`:

    righe aggiunte      1 616
      commento            949   (58 %)
      vuote               103   ( 6 %)
      codice              564   (34 %)

**Il 58 % della crescita e' documentazione.** In questo repository il commento accanto al codice cita le
`D-nnn` e porta la ragione della regola: e' un bene, non debito. Ma la metrica «righe del file» — quella
che l'Epic usa per dire `10 412` — sale allo stesso modo se aggiungi una regola o se spieghi quella che
c'e' gia'. Una fetta che toglie 100 righe di codice e ne aggiunge 150 di spiegazione **peggiora il
numero** mentre migliora il codice.

Per questo ogni misura di ampiezza viene data due volte: `righe` e `righe di codice`. Chi confronta due
fette guardi la seconda.

## L'asse che l'audit non aveva: la lunghezza delle funzioni

L'audit del 2026-08-30 ha misurato l'**isolamento** (zero `GetWorld` nelle library di regole) e
l'**ampiezza** (righe e metodi per classe). Non ha misurato la **complessita' interna**: una funzione pura
puo' essere perfettamente isolata e lunga 249 righe. `URTHexBotLibrary::ScorePlan` lo e'.

Sul modulo, 91 funzioni su 1 643 superano le 100 righe e contengono il **32 %** di tutto il codice di
produzione. 31 stanno in Actor — dove il principio #1 di `architettura-codice.md` dice che la matematica
non dovrebbe esserci — e 28 in library gia' dichiarate a posto dall'audit.

⚠️ **La lunghezza da sola produce falsi positivi.** `URTCatalogLibrary::GetCoreActionCatalog` misura 589
righe e sarebbe il quarto imputato di qualunque classifica: contiene **4 rami** in 589 righe, perche' e'
una tabella di dati con la ragione di ogni voce scritta accanto. Spezzarla non toglierebbe complessita':
sposterebbe dati. Per questo la classifica riporta `rami` accanto a `righe`, e chi la legge scarta le voci
a densita' di rami quasi nulla prima di farne una worklist.

## Cosa NON misura

- **la frequenza di modifica**: una funzione lunga che nessuno tocca non costa niente. Il selettore vero e'
  `righe x churn`, e il churn sta in `git log`, non qui;
- **la complessita' ciclomatica reale**: `rami` conta le parole chiave, non i cammini;
- **se una fetta e' una buona idea**: dice dov'e' il peso, non cosa farne.
"""

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# -- I comandi, dichiarati una volta sola --------------------------------------
# Cambiare uno di questi invalida il confronto con le misure precedenti: se lo fai,
# scrivilo nel commento della issue insieme al numero nuovo.

TURNMANAGER_FILES = [
    "Source/RefactorTactics/Turn/RTTurnManager.h",
    "Source/RefactorTactics/Turn/RTTurnManager.cpp",
    "Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp",
]
PATTERN_MONDO = "SpawnActor"          # un test che spawna un Actor ha bisogno di un mondo
PATTERN_TURNMANAGER = "ARTTurnManager"
RE_TEST_NAME = re.compile(r'"RefactorTactics\.[A-Za-z0-9_.]+"')
RE_METODO_TM = re.compile(r"ARTTurnManager::[A-Za-z0-9_~]+")
RE_COMMENTO = re.compile(r"^\s*(//|/\*|\*)")
RE_RAMO = re.compile(r"\b(if|for|while|switch|else)\b")
RE_FIRMA = re.compile(r"([A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z0-9_~]+)?)\s*\(")
RE_NON_FUNZIONE = re.compile(r"\b(namespace|class|struct|enum|union)\b")
NON_FUNZIONI = {"if", "for", "while", "switch", "return", "sizeof", "catch", "do"}


def leggi(path):
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def conta_righe(righe):
    """(totali, codice) — codice = ne' commento ne' vuote. Vedi la docstring, sezione righe."""
    commento = sum(1 for r in righe if RE_COMMENTO.match(r))
    vuote = sum(1 for r in righe if not r.strip())
    return len(righe), len(righe) - commento - vuote


def spoglia(riga):
    riga = re.sub(r"\\.", "", riga)
    riga = re.sub(r'"[^"]*"', "", riga)
    riga = re.sub(r"'[^']*'", "", riga)
    return re.sub(r"//.*$", "", riga)


def funzioni_di(path):
    """Estensione di ogni funzione contando le graffe. Restituisce (righe, rami, nome, riga_inizio)."""
    testo = leggi(path)
    profondita = 0
    in_commento = False
    stack = []
    esiti = []
    recenti = []
    for i, grezza in enumerate(testo, 1):
        riga = grezza
        if in_commento:
            if "*/" in riga:
                riga = riga.split("*/", 1)[1]
                in_commento = False
            else:
                continue
        while "/*" in riga:
            prima, resto = riga.split("/*", 1)
            if "*/" in resto:
                riga = prima + resto.split("*/", 1)[1]
            else:
                riga = prima
                in_commento = True
                break
        pulita = spoglia(riga)
        apre = pulita.count("{")
        chiude = pulita.count("}")
        if apre > 0 and not stack:
            ctx = " ".join(recenti[-3:] + [pulita])
            if not RE_NON_FUNZIONE.search(ctx):
                cand = [c for c in RE_FIRMA.findall(ctx) if c not in NON_FUNZIONI]
                if cand:
                    stack.append([cand[-1], i, profondita, 0])
        if stack:
            stack[-1][3] += len(RE_RAMO.findall(pulita))
        profondita += apre - chiude
        if stack and profondita <= stack[-1][2]:
            nome, inizio, _, rami = stack.pop()
            esiti.append((i - inizio + 1, rami, nome, inizio))
        if pulita.strip():
            recenti.append(pulita)
            if len(recenti) > 4:
                recenti.pop(0)
    return esiti


def contenitore(nome):
    cls = nome.split("::")[0] if "::" in nome else ""
    if cls.startswith("A"):
        return "Actor (A*)"
    if cls.endswith("Library"):
        return "Function Library (pura)"
    if cls.startswith("U"):
        return "UObject/Asset (U*)"
    if cls.startswith("F"):
        return "struct/classe pura (F*)"
    if cls.startswith("S"):
        return "Slate widget (S*)"
    if not cls:
        return "funzione libera / ns anonimo"
    return "altro"


def misura(radice, soglia):
    m = {"soglia": soglia}

    tot = 0
    cod = 0
    m["turnmanager_file"] = {}
    for rel in TURNMANAGER_FILES:
        p = radice / rel
        if not p.exists():
            continue
        t, c = conta_righe(leggi(p))
        m["turnmanager_file"][Path(rel).name] = {"righe": t, "righe_codice": c}
        tot += t
        cod += c
    m["turnmanager_righe"] = tot
    m["turnmanager_righe_codice"] = cod

    metodi = set()
    for p in (radice / "Source/RefactorTactics/Turn").glob("*.cpp"):
        metodi.update(RE_METODO_TM.findall("\n".join(leggi(p))))
    m["turnmanager_metodi"] = len(metodi)

    test_file = sorted((radice / "Source/RefactorTactics/Tests").glob("*.cpp"))
    nomi = set()
    con_mondo = 0
    con_tm = 0
    for p in test_file:
        testo = "\n".join(leggi(p))
        nomi.update(RE_TEST_NAME.findall(testo))
        if PATTERN_MONDO in testo:
            con_mondo += 1
        if PATTERN_TURNMANAGER in testo:
            con_tm += 1
    n_file = len(test_file)
    m["test_file"] = n_file
    m["test_unici"] = len(nomi)
    m["test_file_con_mondo"] = con_mondo
    m["test_file_con_turnmanager"] = con_tm
    m["quota_mondo"] = round(100.0 * con_mondo / n_file, 1) if n_file else 0.0
    m["quota_turnmanager"] = round(100.0 * con_tm / n_file, 1) if n_file else 0.0

    righe_prod = 0
    lunghe = []
    totali = 0
    for p in (radice / "Source").rglob("*.cpp"):
        if "Tests" in p.parts:
            continue
        righe_prod += len(leggi(p))
        rel = p.relative_to(radice).as_posix().replace("Source/RefactorTactics/", "")
        for lung, rami, nome, inizio in funzioni_di(p):
            totali += 1
            if lung >= soglia:
                lunghe.append({"righe": lung, "rami": rami, "nome": nome, "file": rel, "linea": inizio})
    lunghe.sort(key=lambda r: -r["righe"])
    m["produzione_righe_cpp"] = righe_prod
    m["funzioni_totali"] = totali
    m["funzioni_lunghe"] = len(lunghe)
    m["righe_in_funzioni_lunghe"] = sum(r["righe"] for r in lunghe)
    m["quota_righe_in_funzioni_lunghe"] = (
        round(100.0 * m["righe_in_funzioni_lunghe"] / righe_prod, 1) if righe_prod else 0.0
    )
    m["per_contenitore"] = {}
    for r in lunghe:
        v = m["per_contenitore"].setdefault(contenitore(r["nome"]), {"funzioni": 0, "righe": 0})
        v["funzioni"] += 1
        v["righe"] += r["righe"]
    m["classifica"] = lunghe[:25]
    return m


def albero_di(ref, radice_repo):
    """Estrae `ref` in una directory temporanea: misurare un commit non deve toccare l'albero di lavoro."""
    tmp = Path(tempfile.mkdtemp(prefix="rt-misure-"))
    tar = tmp / "albero.tar"
    with tar.open("wb") as fh:
        subprocess.run(["git", "archive", ref, "Source"], cwd=str(radice_repo), stdout=fh, check=True)
    subprocess.run(["tar", "-xf", str(tar), "-C", str(tmp)], check=True)
    tar.unlink()
    return tmp


def delta(dopo, prima):
    d = dopo - prima
    return ("%+d" % d) if isinstance(d, int) else ("%+.1f" % d)


def stampa(m, base=None, markdown=False):
    def riga(etichetta, chiave, suffisso=""):
        v = m[chiave]
        if base is None:
            corpo = "%s%s" % (v, suffisso)
            if markdown:
                print("| %s | %s |" % (etichetta, corpo))
            else:
                print("  %-44s %s" % (etichetta, corpo))
            return
        b = base[chiave]
        if markdown:
            print("| %s | %s%s | %s%s | **%s** |" % (etichetta, b, suffisso, v, suffisso, delta(v, b)))
        else:
            print("  %-44s %8s%s -> %8s%s   %s" % (etichetta, b, suffisso, v, suffisso, delta(v, b)))

    if markdown:
        testa = ["Metrica", "prima", "dopo", "delta"] if base else ["Metrica", "valore"]
        print("| " + " | ".join(testa) + " |")
        print("|" + "|".join(["---"] * len(testa)) + "|")
    else:
        print("=== MISURE STRUTTURALI (%s) ===" % m["ref"])

    riga("`ARTTurnManager` righe (3 file)", "turnmanager_righe")
    riga("`ARTTurnManager` righe di **codice**", "turnmanager_righe_codice")
    riga("metodi `ARTTurnManager::`", "turnmanager_metodi")
    riga("file di test", "test_file")
    riga("test unici", "test_unici")
    riga("file di test che spawnano un Actor", "test_file_con_mondo")
    riga("quota test con mondo", "quota_mondo", " %")
    riga("file di test che dipendono dal TurnManager", "test_file_con_turnmanager")
    riga("quota test col TurnManager", "quota_turnmanager", " %")
    riga("funzioni >= %d righe" % m["soglia"], "funzioni_lunghe")
    riga("righe dentro quelle funzioni", "righe_in_funzioni_lunghe")
    riga("quota del codice di produzione", "quota_righe_in_funzioni_lunghe", " %")

    print()
    ordinati = sorted(m["per_contenitore"].items(), key=lambda kv: -kv[1]["righe"])
    if markdown:
        print("**Funzioni >= %d righe, per contenitore**\n" % m["soglia"])
        print("| contenitore | funzioni | righe |")
        print("|---|---|---|")
        for k, v in ordinati:
            print("| %s | %d | %d |" % (k, v["funzioni"], v["righe"]))
        print()
        print("**Le 12 piu' lunghe** — `rami` quasi nullo = tabella di dati, non complessita'\n")
        print("| righe | rami | funzione | file:linea |")
        print("|---|---|---|---|")
        for r in m["classifica"][:12]:
            print("| %d | %d | `%s` | `%s:%d` |" % (r["righe"], r["rami"], r["nome"], r["file"], r["linea"]))
    else:
        print("=== FUNZIONI >= %d RIGHE, PER CONTENITORE ===" % m["soglia"])
        for k, v in ordinati:
            print("  %-32s %4d funzioni %7d righe" % (k, v["funzioni"], v["righe"]))
        print()
        print("=== LE 12 PIU' LUNGHE (rami quasi nullo = dati, non complessita') ===")
        print("  %6s %5s  %-48s %s" % ("RIGHE", "RAMI", "FUNZIONE", "FILE:LINEA"))
        for r in m["classifica"][:12]:
            print("  %6d %5d  %-48s %s:%d" % (r["righe"], r["rami"], r["nome"][:48], r["file"], r["linea"]))


def main():
    ap = argparse.ArgumentParser(description="Misure strutturali di E50 — #1816, #1818")
    ap.add_argument("--ref", help="misura questo commit invece dell'albero di lavoro")
    ap.add_argument("--base", help="commit di confronto: stampa prima/dopo/delta")
    ap.add_argument("--soglia", type=int, default=100, help="righe oltre le quali una funzione e' lunga")
    ap.add_argument("--markdown", action="store_true", help="tabelle da incollare in un commento di issue")
    ap.add_argument("--json", help="scrive le misure grezze in questo file")
    a = ap.parse_args()

    repo = Path(__file__).resolve().parents[2]
    temporanee = []
    try:
        radice = albero_di(a.ref, repo) if a.ref else repo
        if a.ref:
            temporanee.append(radice)
        m = misura(radice, a.soglia)
        m["ref"] = a.ref or "albero di lavoro"

        base = None
        if a.base:
            rb = albero_di(a.base, repo)
            temporanee.append(rb)
            base = misura(rb, a.soglia)
            base["ref"] = a.base
            if a.markdown:
                print("Misurato con `tools/architettura/misure-strutturali.py`, da `%s` a `%s`.\n"
                      % (a.base, m["ref"]))

        stampa(m, base, a.markdown)

        if a.json:
            Path(a.json).write_text(
                json.dumps({"dopo": m, "prima": base}, indent=2, ensure_ascii=False), encoding="utf-8"
            )
            sys.stderr.write("\nmisure grezze: %s\n" % a.json)
    finally:
        for t in temporanee:
            shutil.rmtree(str(t), ignore_errors=True)


if __name__ == "__main__":
    main()
