#!/usr/bin/env python3
"""Gate anti-deriva: un documento normativo non cita simboli C++ che non esistono.

Nasce dal difetto **D1** del 2026-08-07: `piano-canonico-mvp.md` §5 elencava quattro classi su dieci
rimosse dal codice mesi prima. Il documento restava leggibile e sbagliato, ed era dichiarato vincolante.
I link rotti si vedono; un simbolo inesistente no.

Uso:
    python scripts/check-docs-symbols.py            # dalla radice del repo
    python scripts/check-docs-symbols.py --list     # elenca anche i documenti esentati

Esce con codice 1 se un documento normativo cita un simbolo assente da Source/.

Esenzioni, per costruzione e non per elenco a mano:
  - le cartelle di storico (`archive/`, `src/`, `roadmap/plans/`);
  - i documenti che dichiarano in testa di essere storici (banner ⚠️/📦/ℹ️ delle prime righe).
Un documento che vuole citare API rimosse deve dirlo: e' esattamente la disciplina che il gate impone.
"""
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCS = os.path.join(REPO, "docs")
SOURCE = os.path.join(REPO, "Source")

EXEMPT_DIRS = ("docs/archive/", "docs/src/", "docs/roadmap/plans/")

# Documenti che si dichiarano storici o propositivi: non descrivono il codice di oggi.
EXEMPT_MARKERS = (
    "riferimento storico, non normativo",
    "Piano di esecuzione consegnato",
    "Regola vigente, esempi datati",
    "brief di requisiti",
    "brief di scope",
)

# Una riga che parla di un simbolo al passato, o che lo propone, non sta affermando
# che esista oggi. Senza questo filtro il gate segnala chi documenta correttamente
# una rimozione — e un gate rumoroso viene disattivato al terzo falso positivo.
NOT_A_CLAIM = re.compile(
    r"rimoss|rimuov|non esiste|non c'è|non c'e'|assente|superat|storic|elimin|sostituit"
    r"|mai realizzat|non più|non piu'|dismission|deprecat|~~|north-star|propost|futur"
    r"|da costruire|⏳|⌫|sparisce|via con|migra", re.IGNORECASE)

# Il gate controlla SOLO le righe di tabella la cui PRIMA cella e' un simbolo: la forma
# «| `URTFoo` | Tipo | Responsabilita' |», cioe' un *inventario di classi*. E' la forma
# esatta del difetto D1, ed e' l'unica in cui il documento afferma senza ambiguita'
# «questo esiste, oggi, e fa questo».
#
# Deliberatamente NON controlla la prosa: li' un simbolo puo' essere citato come storia
# («FRTGridCoord e' stato rimosso»), come proposta («il DoD introduce FRTReactionOpportunity»)
# o come north-star. Distinguerli lessicalmente non e' affidabile, e un gate che sbaglia
# viene disattivato. Meglio uno stretto che tiene di uno largo che nessuno guarda.
ROW_RE = re.compile(
    r"^\s*\|\s*`([UAFE]RT[A-Za-z0-9_]+)(?:::[A-Za-z0-9_]+)?`"
    r"(?:\s*(?:·|,|/)\s*`[UAFE]RT[A-Za-z0-9_]+`)*\s*\|")
SYMBOL_RE = re.compile(r"`([UAFE]RT[A-Za-z0-9_]+)(?:::[A-Za-z0-9_]+)?`")


# Solo DICHIARAZIONI, non occorrenze. Un commento che spiega una rimozione
# («Era `URTAbilityData`: rinominata al CP 1.3») non deve far risultare esistente
# la classe rimossa — altrimenti il gate si autoassolve proprio sui simboli che
# gli interessano di piu'. Scoperto con un test di mutazione: la prima versione
# lasciava passare `URTGridLibrary`, viva solo dentro un commento.
# La KEYWORD non decide il prefisso, e legarli e' costato un falso positivo il 2026-08-17:
# `FRTScenarioSession` e' dichiarata `class REFACTORTACTICS_API FRTScenarioSession`, quindi il
# pattern `class` (che accettava solo `[AU]RT`) non la vedeva e quello `struct` (che accetta solo
# `FRT`) nemmeno. Il gate la dichiarava «inesistente in Source/» mentre esisteva, e l'effetto pratico
# era una censura: nessun documento normativo poteva citarla in una tabella-inventario.
# In C++ `class` e `struct` differiscono per il solo accesso di default — la convenzione UE sul
# prefisso e' ortogonale alla parola chiave, e il gate ora lo rispecchia.
# Misurato quando la correzione e' stata scritta: 3 `class F*` (`FRTScenarioSession`,
# `FRTHexEditorModeCommands`, `FRTHexEditorModeToolkit`) e 0 `struct [AU]*`, quindi il difetto era
# unidirezionale — ma il pattern copre entrambi i versi, perche' l'asimmetria era la causa.
DECL_RES = (
    re.compile(r"^\s*(?:class|struct)\s+(?:\w+_API\s+)?([AUF]RT[A-Za-z0-9_]+)\b", re.M),
    re.compile(r"^\s*enum\s+class\s+(ERT[A-Za-z0-9_]+)\b", re.M),
    re.compile(r"^\s*using\s+([UAFE]RT[A-Za-z0-9_]+)\s*=", re.M),
    re.compile(r"^\s*typedef\s+.*\b([UAFE]RT[A-Za-z0-9_]+)\s*;", re.M),
)


def source_symbols():
    """I simboli [UAFE]RT* effettivamente DICHIARATI in Source/."""
    found = set()
    for root, dirs, files in os.walk(SOURCE):
        dirs[:] = [d for d in dirs if d not in ("Binaries", "Intermediate")]
        for fn in files:
            if fn.endswith((".h", ".cpp", ".cs")):
                with open(os.path.join(root, fn), encoding="utf-8", errors="ignore") as f:
                    text = f.read()
                for pat in DECL_RES:
                    found.update(pat.findall(text))
    return found


def is_exempt(rel, head):
    if rel.startswith(EXEMPT_DIRS):
        return True
    return any(m in head for m in EXEMPT_MARKERS)


def main():
    known = source_symbols()
    if not known:
        print("ERRORE: nessun simbolo trovato in Source/ — percorso sbagliato?", file=sys.stderr)
        return 2

    missing, checked, exempt = {}, 0, []
    for root, dirs, files in os.walk(DOCS):
        for fn in sorted(files):
            if not fn.endswith(".md"):
                continue
            abs_p = os.path.join(root, fn)
            rel = os.path.relpath(abs_p, REPO).replace("\\", "/")
            text = open(abs_p, encoding="utf-8").read()
            if is_exempt(rel, "\n".join(text.split("\n")[:14])):
                exempt.append(rel)
                continue
            checked += 1
            for n, line in enumerate(text.split("\n"), 1):
                if not ROW_RE.match(line) or NOT_A_CLAIM.search(line):
                    continue
                # solo la prima cella: le altre descrivono, non inventariano
                first_cell = line.split("|")[1]
                for sym in SYMBOL_RE.findall(first_cell):
                    if sym not in known:
                        missing.setdefault(rel, set()).add((sym, n))

    if "--list" in sys.argv:
        print(f"Esentati ({len(exempt)}):")
        for e in exempt:
            print("   ", e)
        print()

    print(f"Documenti normativi controllati: {checked} · simboli noti in Source/: {len(known)}")
    if not missing:
        print("OK — nessun simbolo inesistente nei documenti normativi.")
        return 0

    print(f"\nFALLITO — {sum(len(v) for v in missing.values())} simboli inesistenti "
          f"in {len(missing)} documenti:\n")
    for rel in sorted(missing):
        print(f"  {rel}")
        for sym, n in sorted(missing[rel], key=lambda x: x[1]):
            print(f"      :{n}  `{sym}`  non esiste in Source/")
    print("\nTre esiti possibili, in ordine di probabilita':")
    print("  1. il simbolo e' stato rinominato o rimosso  -> aggiorna il documento;")
    print("  2. la riga parla di una rimozione o di una proposta -> riformulala"
          " (il gate riconosce 'rimosso', 'superato', 'proposto', ...);")
    print("  3. il documento e' storico -> aggiungi il banner di stato in testa.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
