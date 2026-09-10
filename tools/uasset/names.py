#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Legge la TABELLA DEI NOMI di un pacchetto Unreal (`.uasset`) senza aprire l'Editor.

    python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro WBP_
    python tools/uasset/names.py --rev origin/main Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset
    python tools/uasset/names.py --rev cc5ca967 <path> --rev bbca9a36 <path>   # confronto

Perche' esiste
--------------
`docs/technical/runbooks/guida-screen-hud-umg.md` §3 ha dichiarato per mesi che *«un `.uasset` e'
compresso e `strings` non lo legge: questo tree si verifica solo dall'Editor»*. E' falso, e la sua
falsita' e' costata **tre** dichiarazioni di montaggio sbagliate (#2697, #2760, #2784): ogni volta si e'
creduto alla risposta dello strumento che aveva scritto, invece di rileggere il pacchetto.

La tabella dei nomi **non e' compressa**. E' una sequenza di `FString` con prefisso di lunghezza
(`int32 len` + `len` byte + terminatore), e contiene il nome di **ogni** widget, classe e import
dell'albero. Se un widget e' montato, il suo nome e' qui; se non c'e', non e' montato.

⛔ **Cosa NON dice**: il layout. Questo strumento risponde a *«chi c'e' nell'albero?»*, non a *«com'e'
disposto?»* — quella resta una domanda per l'Editor e per `PIE-V01-SCREENHUD`.

⚠️ **Il confronto fra due revisioni e' la forma piu' utile.** Due pacchetti con byte diversi e tabella
dei nomi **identica** sono lo stesso albero ricompilato: e' cosi' che si e' scoperto che `cc5ca967`
aveva risalvato `WBP_RT_TacticalHUD` nello stato precedente al fix di #2760.
"""
import argparse
import struct
import subprocess
import sys


def nomi(dati: bytes, minlen: int = 2, maxlen: int = 200):
    """Le `FString` con prefisso di lunghezza contenute nel pacchetto, in ordine di offset.

    Non e' un parser del formato: e' una scansione. Un parser dovrebbe seguire `NameOffset` dal
    summary, il cui layout cambia fra versioni dell'Engine — e per la domanda «questo nome c'e'?» la
    scansione risponde uguale su tutte, senza invecchiare a ogni upgrade.
    """
    fuori = []
    i, n = 0, len(dati)
    while i + 4 < n:
        (l,) = struct.unpack_from("<i", dati, i)
        if minlen <= l <= maxlen and i + 4 + l <= n:
            s = dati[i + 4: i + 4 + l]
            if s.endswith(b"\x00") and all(32 <= c < 127 for c in s[:-1]):
                fuori.append(s[:-1].decode("ascii"))
                i += 4 + l
                continue
        i += 1
    return fuori


def leggi(percorso: str, rev: str | None) -> bytes:
    if rev is None:
        with open(percorso, "rb") as f:
            return f.read()
    fine = subprocess.run(["git", "show", f"{rev}:{percorso}"], capture_output=True)
    if fine.returncode != 0:
        sys.exit(f"git show {rev}:{percorso} → {fine.stderr.decode('utf-8', 'replace').strip()}")
    return fine.stdout


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("percorso", help="percorso del .uasset, relativo alla radice del repository")
    p.add_argument("--rev", help="legge il blob da questa revisione Git invece che dal working tree")
    p.add_argument("--filtro", default="", help="stampa solo i nomi che contengono questa sottostringa")
    p.add_argument("--unici", action="store_true", help="ordina e deduplica, per confrontare due revisioni")
    a = p.parse_args()

    ns = nomi(leggi(a.percorso, a.rev))
    if a.filtro:
        ns = [x for x in ns if a.filtro in x]
    if a.unici:
        ns = sorted(set(ns))

    dove = f"{a.rev}:{a.percorso}" if a.rev else a.percorso
    print(f"=== {dove} — {len(ns)} nomi ===")
    for x in ns:
        print(f"   {x}")
    # Un elenco vuoto e' una risposta, non un errore: dice che il filtro non trova nulla nel pacchetto.
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
