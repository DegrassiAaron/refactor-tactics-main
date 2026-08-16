#!/usr/bin/env python3
"""Gate: ogni capability NON disponibile dichiara un owner, e quell'owner e' ancora APERTO.

Perche' esiste, misurato e non ipotizzato
-----------------------------------------
Il 2026-08-16 uno spec panel su `#170` ha trovato che `InterceptRevalidation` era fra le
`KnownUnavailableCapabilities()` mentre il suo owner — `#200`, «[FIX] Intercept: rivalidare la
geometria sul bersaglio effettivo (D-017)» — era **CLOSED con 4 voci di DoD su 4**, il codice era in
`main` e tre test lo pinnavano. La capability diceva «il gioco non lo sa ancora fare» di qualcosa che
sapeva fare. Nessun gate l'ha vista: `ShippedScenariosRequireKnownCapabilities` verifica che ogni
`requires` nomini una capability **esistente**, e nessuno verificava il verso opposto.

Una su nove era scaduta. Questo script e' cio' che l'avrebbe trovata senza uno spec panel.

Le due meta', e perche' sono separate
-------------------------------------
1. **Strutturale (offline, sempre)** — ogni riga dell'elenco dichiara un `// owner: #N` oppure
   `// owner: none`. Prende il caso piu' comune: qualcuno aggiunge una capability e non dice di chi
   e'. Non serve rete, quindi non ha scuse per non girare.
2. **Stato (online, `--online`)** — per ogni owner chiede a `gh` se la issue e' ancora aperta. E' la
   meta' che ha trovato `#200`, e richiede rete: senza `--online` il gate lo **dichiara** invece di
   fingere di averlo controllato.

⚠️ `owner: none` e' legittimo per **una sola** capability e lo script lo verifica: `NeverAvailable` e'
il nome riservato che per costruzione non atterra mai. Se un giorno una seconda capability si
dichiarasse orfana, e' un errore — l'esenzione e' un caso, non una categoria.

Cosa significa `owner:`, e non e' ovvio
---------------------------------------
E' **chi la spostera' fra le disponibili**, non chi ha scritto la feature. Per sette voci su otto le
due cose coincidono e la distinzione non si vede; su `InterceptRevalidation` no, ed e' il caso che ha
costretto a deciderlo: la feature e' di `#200` (CLOSED), lo spostamento e' di `#170` insieme al
contenuto del T6. Con la prima lettura il gate restava rosso su un difetto gia' capito, il che lo
avrebbe reso rumore entro un giorno; con la seconda resta verde e continua a dire il vero.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

SOURCE = Path("Source/RefactorTactics/ScenarioHarness/RTScenarioSession.cpp")
BLOCK_START = "const TSet<FString>& KnownUnavailableCapabilities()"
# `TEXT("Nome"),` seguito, sulla stessa riga, da `// owner: #123` oppure `// owner: none`.
ENTRY = re.compile(r'^\s*TEXT\("(?P<cap>[A-Za-z0-9_]+)"\),\s*(?://\s*owner:\s*(?P<owner>#\d+|none))?\s*$')
# La sola capability che puo' dichiararsi senza owner, e la ragione sta nel commento accanto a lei.
EXEMPT = "NeverAvailable"


def leggi_capability(percorso: Path) -> list[tuple[int, str, str | None]]:
    """Le righe dell'elenco: (numero_di_riga, capability, owner o None)."""
    testo = percorso.read_text(encoding="utf-8")
    inizio = testo.find(BLOCK_START)
    if inizio < 0:
        raise SystemExit(
            f"{percorso}: `{BLOCK_START}` non trovato. Se la funzione e' stata rinominata, questo "
            f"gate va aggiornato: fallire e' giusto, tacere no."
        )
    # Il blocco finisce alla chiusura dell'inizializzatore, non alla graffa della funzione.
    fine = testo.find("};", inizio)
    if fine < 0:
        raise SystemExit(f"{percorso}: l'elenco non si chiude — sorgente malformato")

    voci: list[tuple[int, str, str | None]] = []
    prima_riga = testo.count("\n", 0, inizio) + 1
    for offset, riga in enumerate(testo[inizio:fine].splitlines()):
        m = ENTRY.match(riga)
        if m:
            voci.append((prima_riga + offset, m.group("cap"), m.group("owner")))
    return voci


def stato_issue(numero: str) -> str | None:
    """`OPEN`/`CLOSED`, oppure None se `gh` non risponde — che non e' un verde."""
    try:
        out = subprocess.run(
            ["gh", "issue", "view", numero.lstrip("#"), "--json", "state"],
            capture_output=True, text=True, timeout=30, check=False,
        )
    except (OSError, subprocess.SubprocessError):
        return None
    if out.returncode != 0:
        return None
    try:
        return json.loads(out.stdout).get("state")
    except json.JSONDecodeError:
        return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--online", action="store_true",
                    help="interroga GitHub sullo stato di ogni owner (richiede `gh` autenticato)")
    args = ap.parse_args()

    if not SOURCE.exists():
        raise SystemExit(f"{SOURCE} non trovato: eseguire dalla radice del repository")

    voci = leggi_capability(SOURCE)
    if not voci:
        raise SystemExit(
            f"{SOURCE}: l'elenco e' stato letto ma non contiene voci. Un gate che non guarda nulla "
            f"e' peggio di nessun gate — fallisce invece di dire OK su zero righe."
        )

    errori: list[str] = []

    # --- 1. strutturale ---------------------------------------------------------------------------
    orfane = [(n, c) for n, c, o in voci if o is None]
    for n, cap in orfane:
        errori.append(
            f"{SOURCE}:{n}: `{cap}` non dichiara un owner. Aggiungi `// owner: #<issue>` accanto "
            f"alla riga, oppure `// owner: none` se e' un nome riservato — e in quel caso spiega "
            f"perche' nel commento."
        )

    esenti = [(n, c) for n, c, o in voci if o == "none"]
    for n, cap in esenti:
        if cap != EXEMPT:
            errori.append(
                f"{SOURCE}:{n}: `{cap}` si dichiara `owner: none`, ma l'unica capability senza owner "
                f"e' `{EXEMPT}` (nome riservato, non atterra mai). Una seconda esenzione trasforma un "
                f"caso in una categoria: se e' voluta, va decisa e questo gate va aggiornato."
            )

    # --- 2. stato ---------------------------------------------------------------------------------
    tracciate = [(n, c, o) for n, c, o in voci if o and o != "none"]
    chiuse: list[str] = []
    non_verificate: list[str] = []
    if args.online:
        for n, cap, owner in tracciate:
            stato = stato_issue(owner)
            if stato is None:
                non_verificate.append(f"{cap} ({owner})")
            elif stato != "OPEN":
                chiuse.append(cap)
                errori.append(
                    f"{SOURCE}:{n}: `{cap}` e' dichiarata NON disponibile, ma il suo owner {owner} e' "
                    f"{stato}. O la feature e' atterrata e la capability va spostata fra le "
                    f"`AvailableCapabilities()` — insieme ai DATI degli scenari che la chiedono, mai "
                    f"da sola — o l'owner e' sbagliato e va corretto."
                )

    # --- referto ----------------------------------------------------------------------------------
    print(f"Capability non disponibili: {len(voci)} · con owner dichiarato: {len(tracciate)} · "
          f"esenti: {len(esenti)} · senza owner: {len(orfane)}")
    if args.online:
        verificate = len(tracciate) - len(non_verificate)
        print(f"Owner interrogati: {verificate}/{len(tracciate)} · chiusi: {len(chiuse)}")
        if non_verificate:
            # Non e' un errore ma non e' nemmeno un verde: dirlo e' la differenza fra «controllato»
            # e «non controllabile da qui».
            print("NOTA  owner non verificati (gh non ha risposto): " + ", ".join(non_verificate))
    else:
        print("NOTA  meta' STATO non eseguita: aggiungi --online per interrogare gli owner. "
              "Senza, questo gate prova che ogni capability DICHIARA un owner, non che sia aperto.")

    if errori:
        print()
        for e in errori:
            print(f"ERROR {e}")
        print(f"\nerrori: {len(errori)}")
        return 1

    print("OK — ogni capability non disponibile dichiara un owner"
          + (", e ogni owner interrogato e' aperto." if args.online else "."))
    return 0


if __name__ == "__main__":
    sys.exit(main())
