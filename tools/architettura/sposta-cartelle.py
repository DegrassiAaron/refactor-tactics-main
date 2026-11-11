# -*- coding: utf-8 -*-
"""Applica una mappa di spostamento dei file C++ di `Source/`: `git mv`, include e percorsi citati.

    Uso:  python tools/architettura/sposta-cartelle.py <mappa.json> [--scrivi]
          python tools/architettura/sposta-cartelle.py <mappa.json> --solo-riferimenti [--scrivi]

La mappa e' quella di `mappa-cartelle.py --json` (percorso vecchio -> percorso nuovo, relativi alla radice del
repository). Senza `--scrivi` stampa cosa farebbe e non tocca niente.

## Le due modalita'

* **Spostamento** (default): sposta i file con `git mv`, poi riscrive gli include e i percorsi citati. Si usa UNA
  volta, sull'albero da riorganizzare.
* **`--solo-riferimenti`**: non sposta niente, riscrive soltanto include e percorsi che nominano ancora un percorso
  VECCHIO. Serve a un branch nato prima della riorganizzazione: dopo aver portato dentro `main`, i suoi file nuovi
  o modificati includono ancora `Turn/RTHexSimLibrary.h`, e questa modalita' li porta a
  `Turn/Resolution/RTHexSimLibrary.h`.

## Come risolve un include, e perche' non e' una sostituzione di testo

🔑 Un include si risolve come lo risolve il compilatore, poi si decide: prima la cartella del file che include, poi
le radici del modulo (`Source/RefactorTactics/` per il runtime, che e' `PublicIncludePaths`; `Public/` e `Private/`
per l'Editor, piu' la radice del runtime da cui l'Editor include). Si riscrive solo un include che punta a un file
spostato, o che funzionava perche' stava nella stessa cartella di chi lo includeva e adesso non ci sta piu' (i
53 `#include "RTWorldFixtures.h"` dei test). La forma nuova e' sempre relativa a una radice, mai alla cartella del
file: cosi' sopravvive al prossimo spostamento di chi include.

⛔ Una sostituzione di testo di `"RTWorldFixtures.h"` non saprebbe da quale file e' scritta, e non saprebbe se
`"RTHexEditorMode.h"` e' l'header di `Public/` o di `Private/`.

## I percorsi citati

Nei file tracciati da git (documenti, `tools/`, commenti e letterali del codice) si riscrivono le forme di un
percorso spostato che compaiono intere: dalla radice del repository (`Source/RefactorTactics/Turn/RTHexSim.h`),
dalla radice del modulo (`Turn/RTHexSim.h`, anche senza estensione, come nella mappa del codice), e per l'Editor
dalla radice di `Public/` o `Private/`. Il confine e' di parola, quindi `Turn/RTTurnLog` non tocca
`Turn/RTTurnLogLibrary`. I numeri di riga accanto a un percorso restano validi: lo spostamento non cambia i file,
e riscrivere un include non aggiunge righe.

🔴 **Nei documenti storici si aggiornano solo i link** (`storici.py`): l'archivio, le evidenze, il changelog, il
Decision Log, i referti datati di `docs/roadmap/plans/` e i piani di `docs/superpowers/plans/`. La loro prosa e il
loro codice inline raccontano il mondo di quel giorno, spesso come output di un comando a uno SHA, e riscriverli lo
falsifica: la prima riscrittura di #4061 l'aveva fatto in 168 file, e la review l'ha trovato.

⚠️ **Cosa NON fa**:
* non riscrive i corpi delle issue di GitHub;
* non tocca i file binari (un file con un byte NUL nei primi 8 KB);
* non adatta il codice che COMPONE un percorso a pezzi (`TEXT("Turn")` piu' un nome a parte, `os.path.join(...,
  "UI", ...)`), ne' i comandi con un jolly su una cartella divisa (`Tests/*.cpp`): quei punti si cercano a mano;
* `--solo-riferimenti` confronta stringhe d'include esatte: un include nudo di un file accanto, scritto in un file
  NUOVO del branch (`#include "RTHexSim.h"` in un nuovo `Turn/X.cpp`), non si riconosce. Fallisce in compilazione,
  non in silenzio;
* guarda solo i file **tracciati** da git: un file nuovo non ancora aggiunto (`git add`) viene saltato.
"""

import argparse
import json
import os
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path, PurePosixPath

RADICE = Path(__file__).resolve().parents[2]
RT = "Source/RefactorTactics"
ED = "Source/RefactorTacticsEditor"
EST_CPP = (".h", ".cpp", ".inl")
EST_TESTO = (".md", ".py", ".ts", ".js", ".json", ".yml", ".yaml", ".txt", ".ini", ".cs", ".h", ".cpp", ".inl",
             ".ps1", ".sh", ".toml", ".csv")
# `﻿?`: la prima riga di un file con BOM comincia con il BOM, e senza questo il suo include non si vedrebbe.
RE_INCLUDE = re.compile(r'^(﻿?\s*#\s*include\s+")([^"]+)(".*)$')
sys.path.insert(0, str(Path(__file__).resolve().parent))
import storici  # noqa: E402  la regola dei documenti storici, condivisa con chi la applica a mano


def git(*args):
    return subprocess.run(["git", "-C", str(RADICE)] + list(args), capture_output=True, text=True, encoding="utf-8",
                          check=True).stdout


def tracciati():
    return [p for p in git("ls-files", "-z").split("\0") if p]


def radici_per(percorso):
    """Le radici d'include viste da un file, nell'ordine del compilatore (dopo la cartella del file)."""
    if percorso.startswith(ED + "/"):
        return [ED + "/Public", ED + "/Private", RT]
    if percorso.startswith(RT + "/"):
        return [RT]
    if percorso.startswith("Plugins/"):
        parti = percorso.split("/")
        # Plugins/<P>/Source/<Modulo>/...: le radici del plugin, poi quella del runtime
        if len(parti) > 4 and parti[2] == "Source":
            base = "/".join(parti[:4])
            return [base + "/Public", base + "/Private", base, RT]
    return [RT]


def forma_per(destinazione):
    """La stringa d'include di `destinazione`, relativa alla sua radice."""
    for radice in (ED + "/Public", ED + "/Private", RT):
        if destinazione.startswith(radice + "/"):
            return destinazione[len(radice) + 1:]
    return None


def risolvi(include, chi, esistenti):
    """Il percorso (dalla radice del repo) a cui `include` scritto in `chi` si risolve, o None (motore, sistema)."""
    cartella = str(PurePosixPath(chi).parent)
    for base in [cartella] + radici_per(chi):
        cand = os.path.normpath(base + "/" + include).replace("\\", "/")
        if cand in esistenti:
            return cand
    return None


def riscrivi_include(file_rel, testo, mappa, esistenti, nuova_sede):
    """Riscrive gli include di un file che sta (o stara') in `nuova_sede`. Restituisce (testo, n)."""
    righe = testo.split("\n")
    n = 0
    for i, r in enumerate(righe):
        m = RE_INCLUDE.match(r.rstrip("\r"))
        if not m:
            continue
        inc = m.group(2)
        bersaglio = risolvi(inc, file_rel, esistenti)
        if bersaglio is None:
            continue
        dopo = mappa.get(bersaglio, bersaglio)
        # Va riscritto se il bersaglio si sposta, o se si risolveva SOLO dalla cartella di chi include e chi include
        # cambia cartella (l'include relativo smetterebbe di funzionare).
        dalla_cartella = risolvi(inc, file_rel, esistenti) and not any(
            os.path.normpath(b + "/" + inc).replace("\\", "/") == bersaglio for b in radici_per(file_rel))
        cambia_sede = str(PurePosixPath(nuova_sede).parent) != str(PurePosixPath(file_rel).parent)
        if dopo == bersaglio and not (dalla_cartella and cambia_sede):
            continue
        nuovo = forma_per(dopo)
        if nuovo is None or nuovo == inc:
            continue
        cr = "\r" if r.endswith("\r") else ""
        righe[i] = m.group(1) + nuovo + m.group(3) + cr
        n += 1
    return "\n".join(righe), n


def forme_citate(vecchio, nuovo):
    """Le coppie (forma vecchia, forma nuova) con cui un percorso spostato puo' comparire in un testo."""
    coppie = [(vecchio, nuovo)]
    for radice in (RT, ED + "/Public", ED + "/Private", ED):
        if vecchio.startswith(radice + "/") and nuovo.startswith(radice + "/"):
            v, n = vecchio[len(radice) + 1:], nuovo[len(radice) + 1:]
            coppie.append((v, n))
    # anche senza estensione (`Turn/RTTurnRules` nella mappa del codice), una volta per unita'
    out = []
    for v, n in coppie:
        # 🔴 **Una forma senza `/` e' un NOME, non un percorso, e non si riscrive.** Per un file che stava direttamente
        # in `Public/` o `Private/` dell'Editor la forma relativa alla radice e' il nome nudo (`RTHexSelectionStore.h`):
        # trattata come percorso, la riscrittura l'ha sostituita una seconda volta DENTRO l'include gia' corretto
        # (`"HexMode/HexMode/RTHexSelectionStore.h"`) e in ogni testo che nomina il file. Misurato sulla prima build
        # della riorganizzazione (#4061), che si e' fermata su quattro unita' dell'Editor.
        if "/" not in v:
            continue
        out.append((v, n))
        vs, ns = os.path.splitext(v)[0], os.path.splitext(n)[0]
        if "/" in vs:
            out.append((vs, ns))
    return out


def tabella_citazioni(mappa):
    """{forma vecchia: forma nuova}, scartando le forme ambigue (stessa forma vecchia, destinazioni diverse)."""
    t = {}
    ambigue = set()
    for v, n in mappa.items():
        for fv, fn in forme_citate(v, n):
            if fv in t and t[fv] != fn:
                ambigue.add(fv)
            t[fv] = fn
    for a in ambigue:
        del t[a]
    return t, ambigue


def regex_citazioni(tabella):
    chiavi = sorted(tabella, key=len, reverse=True)
    # Confine: niente carattere di identificatore subito prima o subito dopo. Una `/` prima e' ammessa, perche'
    # `Source/RefactorTactics/Turn/X.h` contiene `Turn/X.h` e deve diventare lo stesso percorso nuovo.
    return re.compile(r"(?<![A-Za-z0-9_])(" + "|".join(re.escape(k) for k in chiavi) + r")(?![A-Za-z0-9_])")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mappa")
    ap.add_argument("--scrivi", action="store_true")
    ap.add_argument("--solo-riferimenti", action="store_true")
    a = ap.parse_args()
    for f in (sys.stdout, sys.stderr):
        if hasattr(f, "reconfigure"):
            f.reconfigure(errors="replace")

    mappa = json.loads(Path(a.mappa).read_text(encoding="utf-8"))
    tutti = tracciati()
    esistenti = set(tutti)

    if not a.solo_riferimenti:
        mancanti = [v for v in mappa if v not in esistenti]
        occupati = [n for n in mappa.values() if n in esistenti]
        doppi = [n for n, c in defaultdict(int, {}).items()]
        visti = defaultdict(list)
        for v, n in mappa.items():
            visti[n].append(v)
        doppi = {n: vs for n, vs in visti.items() if len(vs) > 1}
        assert not mancanti, "nella mappa ma non nel repository: %s" % mancanti[:5]
        assert not occupati, "destinazioni gia' occupate: %s" % occupati[:5]
        assert not doppi, "due file sulla stessa destinazione: %s" % list(doppi.items())[:3]

    # 1. Include: calcolati sull'albero VECCHIO, scritti sui file nella sede NUOVA.
    n_inc = 0
    toccati = {}
    for f in tutti:
        if not f.endswith(EST_CPP) or not (f.startswith("Source/") or f.startswith("Plugins/")):
            continue
        testo = (RADICE / f).read_bytes().decode("utf-8", errors="surrogateescape")
        if a.solo_riferimenti:
            nuovo, n = riscrivi_vecchi(testo, mappa)
        else:
            nuovo, n = riscrivi_include(f, testo, mappa, esistenti, mappa.get(f, f))
        if n:
            toccati[f] = nuovo
            n_inc += n
    print("include riscritti: %d in %d file" % (n_inc, len(toccati)))

    # 2. Percorsi citati, in ogni file di testo tracciato (codice compreso: commenti e letterali).
    tabella, ambigue = tabella_citazioni(mappa)
    rx = regex_citazioni(tabella)
    n_cit = 0
    file_cit = 0
    riscrivi = lambda s: rx.sub(lambda m: tabella[m.group(1)], s)  # noqa: E731
    for f in tutti:
        # Gli strumenti dello spostamento citano apposta le forme VECCHIE, come esempi e come dati: non si riscrivono.
        if f.startswith(("tools/architettura/mappa-cartelle", "tools/architettura/sposta-cartelle",
                         "tools/architettura/storici")):
            continue
        p = RADICE / f
        testo = toccati.get(f)
        if testo is None:
            try:
                grezzo = p.read_bytes()
            except OSError:
                continue
            # ⚠️ Ogni file di TESTO tracciato, non un elenco di estensioni: `.gitignore` non ha estensione, e la
            # prima riscrittura l'aveva saltato lasciando un comando che risponde vuoto. Binario = contiene NUL.
            if b"\0" in grezzo[:8192]:
                continue
            testo = grezzo.decode("utf-8", errors="surrogateescape")
        # 🔴 Nei documenti STORICI si aggiornano solo i link: la prosa racconta il mondo di quel giorno (`storici.py`).
        if storici.e_storico(f):
            nuovo, n = storici.solo_link(testo, f, mappa, riscrivi)
        else:
            nuovo, n = rx.subn(lambda m: tabella[m.group(1)], testo)
        if n:
            toccati[f] = nuovo
            n_cit += n
            file_cit += 1
    print("percorsi citati riscritti: %d in %d file · forme ambigue scartate: %d" % (n_cit, file_cit, len(ambigue)))

    if not a.scrivi:
        print("prova a secco: nessun file toccato")
        return 0

    # 3. Scrittura: prima il contenuto (nella sede vecchia), poi lo spostamento, cosi' git vede rinomine pulite.
    for f, testo in toccati.items():
        (RADICE / f).write_bytes(testo.encode("utf-8", errors="surrogateescape"))
    if not a.solo_riferimenti:
        for v, n in sorted(mappa.items()):
            (RADICE / n).parent.mkdir(parents=True, exist_ok=True)
            git("mv", v, n)
        print("spostati: %d file" % len(mappa))
    return 0


def riscrivi_vecchi(testo, mappa):
    """`--solo-riferimenti`: un include che nomina ancora la forma VECCHIA di un file spostato prende la nuova."""
    forme = {}
    for v, n in mappa.items():
        fv, fn = forma_per(v), forma_per(n)
        if fv and fn and fv != fn:
            forme[fv] = fn
        # le fixture dei test si includevano dalla stessa cartella: `"RTWorldFixtures.h"`
        if v.startswith(RT + "/Tests/") and v.count("/") == 3 and v.endswith(".h"):
            forme[PurePosixPath(v).name] = fn
    righe = testo.split("\n")
    n = 0
    for i, r in enumerate(righe):
        m = RE_INCLUDE.match(r.rstrip("\r"))
        if m and m.group(2) in forme:
            cr = "\r" if r.endswith("\r") else ""
            righe[i] = m.group(1) + forme[m.group(2)] + m.group(3) + cr
            n += 1
    return "\n".join(righe), n


if __name__ == "__main__":
    sys.exit(main())
