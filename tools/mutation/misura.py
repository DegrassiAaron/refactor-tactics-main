# -*- coding: utf-8 -*-
"""Quando una misura vale, e quando no. Sede UNICA, e per la meta' che decide PURA.

## Perche' questo file esiste

Fino al 2026-09-08 questa regola viveva in `scripts/rt-suite.ps1`, che i due gate di
mutazione invocavano come subprocess. Rimosso lo script (`D-347`), la regola e' stata
riscritta **dentro** `suite()` di ciascun gate: due copie, e gia' divergenti — una
tornava tre valori e l'altra quattro, una lanciava `pwsh` e l'altra `powershell`.

Lo script che se n'e' andato lo diceva di se stesso, ed e' la ragione di questo modulo:

> *«Non ha una seconda sede: il flusso principale CHIAMA questa funzione, non ne tiene
> una copia. Due copie di una regola divergono.»*

## Perche' `verdetto()` non tocca niente

Un motore che muore a meta' non si fabbrica a comando. Finche' la regola vive dentro il
flusso che avvia l'Editor, la voce di DoD «un crash non esce VALIDA» si chiude su un
aneddoto: nessun test puo' produrre quel log. Qui la regola prende un log come STRINGA,
e il self-test gliene passa uno scritto a mano — vedi `--self-test` dei due gate.

## Cosa NON copre, dichiarato

`istantanea()` fotografa `HEAD`, il **contenuto** dell'albero e la firma dei binari.
Il quarto termine dell'invariante di `AGENTS.md` — *nessun processo del motore estraneo
durante la run* — non e' osservabile da qui senza un polling che questo modulo non fa:
`motori_vivi()` risponde **prima** di partire, e cio' che parte dopo si rileva solo se
tocca i binari. Non c'e' piu' un lease, e questo modulo non ne e' il sostituto.
"""

import hashlib
import os
import re
import subprocess

# I quattro marcatori con cui Unreal dichiara di essere morto. `#2530`: una mutazione
# out-of-bounds ha ucciso il motore a meta' suite, e la run e' uscita VALIDA con zero
# test completati — il gate ha gridato SOPRAVVISSUTA su una misura mai avvenuta.
MARCATORI_FATALI = (
    "appError called",
    "=== Critical error: ===",
    "Assertion failed:",
    "Fatal error:",
)


def _git(radice, *a):
    r = subprocess.run(["git"] + list(a), cwd=radice,
                       capture_output=True, text=True, errors="replace")
    return (r.stdout or "").strip()


def istantanea(radice, dll_glob=None):
    """`HEAD` + CONTENUTO dell'albero + firma dei binari. Tre termini su quattro.

    🔴 **Il contenuto, non i path.** `git status --porcelain` elenca lettere di stato e
    percorsi: due modifiche diverse dello STESSO file danno la stessa riga ` M pippo.cpp`.
    Un gate di mutazione gira sempre con l'albero sporco — la mutazione e' sua — quindi
    e' esposto proprio al caso che il porcelain non vede: un'altra sessione che riscrive
    un file gia' modificato durante la run. Si confronta `git diff HEAD`, che e' il
    contenuto, piu' l'hash degli untracked.

    🔴 **I binari.** `Binaries/` e' gitignorato: un `Build.bat` di un altro checkout
    riscrive il DLL condiviso senza muovere ne' `HEAD` ne' l'albero. Senza questa riga
    la suite misurerebbe codice diverso da quello che il gate crede di aver mutato.
    """
    tracciati = _git(radice, "diff", "HEAD")
    untracked = _git(radice, "ls-files", "--others", "--exclude-standard")
    firme = []
    for p in sorted(untracked.splitlines()):
        assoluto = os.path.join(radice, p)
        try:
            with open(assoluto, "rb") as fh:
                firme.append("%s %s" % (p, hashlib.sha1(fh.read()).hexdigest()))
        except (IOError, OSError):
            firme.append("%s (non leggibile)" % p)   # non e' «assente»: e' ignoto, e cambia
    albero = hashlib.sha1(
        (tracciati + "\n--untracked--\n" + "\n".join(firme)).encode("utf-8", "replace")
    ).hexdigest()

    dll = []
    if dll_glob:
        import glob
        for d in sorted(glob.glob(dll_glob)):
            try:
                st = os.stat(d)
                dll.append("%s=%d/%d" % (os.path.basename(d), st.st_mtime_ns, st.st_size))
            except OSError:
                dll.append("%s=(assente)" % os.path.basename(d))
    return (_git(radice, "rev-parse", "HEAD"), albero, " ".join(dll))


def motori_vivi():
    """Quanti processi del motore sono gia' in piedi. `0` e' la condizione per partire.

    ⚠️ Risponde **all'avvio**, non durante: e' una precondizione, non una guardia. Due
    gate che partono nello stesso secondo si vedono entrambi liberi. Cio' che non c'e'
    piu' — il mutex e il lease di `rt-suite.ps1` — non e' rimpiazzato da questa funzione
    (`D-347`); serve a non lanciare una suite SOPRA una che sta gia' girando, che era il
    caso comune e costava due misure invece di una.
    """
    esito = subprocess.run(["tasklist", "/NH", "/FO", "CSV"],
                           capture_output=True, text=True, errors="replace")
    if esito.returncode != 0:
        return -1        # 🔴 enumerazione fallita NON e' «nessun processo»: sarebbe un fail-OPEN
    return len([r for r in (esito.stdout or "").splitlines() if "UnrealEditor" in r])


def verdetto(prima, dopo, testo_log, filtro=""):
    """PURA. Torna `(verdetto, esito, rossi, eseguiti, problemi)`.

    Ordine dei controlli, e non e' indifferente: un log crashato che ha anche visto
    l'albero cambiare e' `NON VALIDA` per entrambe le ragioni, e le riporta entrambe.

    * `NON VALIDA`  la misura non e' registrabile: drift, crash, o suite troncata;
    * `NON AVVIATA` non ha misurato niente: motore morto in avvio, o filtro a vuoto;
    * `VALIDA`      registrabile — verde o rossa che sia.
    """
    problemi = []

    rossi = set(re.findall(r"Result=\{Fail\} Name=\{[^}]*\} Path=\{([^}]+)\}", testo_log))
    eseguiti = set(re.findall(r"Test Started\. Name=\{[^}]*\} Path=\{([^}]+)\}", testo_log))
    completati = len(re.findall(r"Test Completed\.", testo_log))
    trovati = None
    m = re.search(r"Found (\d+) automation tests", testo_log)
    if m:
        trovati = int(m.group(1))

    if prima != dopo:
        problemi.append("albero    HEAD, contenuto o binari cambiati durante la run")

    for marcatore in MARCATORI_FATALI:
        i = testo_log.find(marcatore)
        if i < 0:
            continue
        inizio = testo_log.rfind("\n", 0, i) + 1
        fine = testo_log.find("\n", i)
        riga = testo_log[inizio:fine if fine >= 0 else len(testo_log)].strip()
        problemi.append("motore    crash nel log: %s" % marcatore)
        problemi.append("          " + (riga[:157] + "..." if len(riga) > 160 else riga))
        break

    crash = any(p.startswith("motore") for p in problemi)
    drift = any(p.startswith("albero") for p in problemi)
    troncata = False

    if trovati is None:
        if "LogAutomationController" in testo_log:
            problemi.append("copertura il log non dichiara «Found N automation tests», ma la fase di")
            problemi.append("          automation e' stata raggiunta: il filtro '%s' non corrisponde a"
                            " nessun test" % filtro)
        else:
            problemi.append("avvio     l'Editor non ha raggiunto la fase di automation: il log si ferma")
            problemi.append("          prima. Non e' un filtro sbagliato e non e' una suite rossa.")
    elif completati < trovati:
        troncata = True
        problemi.append("copertura %d/%d completati (%d avviati): la run e' stata troncata"
                        % (completati, trovati, len(eseguiti)))

    if crash or drift or troncata:
        v = "NON VALIDA"
    elif trovati is None or not eseguiti:
        v = "NON AVVIATA"
    else:
        v = "VALIDA"

    # 🔴 Il denominatore e' `Found N`, non il numeratore. Scriverlo come
    # `"%d/%d" % (len(eseguiti), len(eseguiti))` produce «40/40 completati» per una suite
    # di 300 troncata a 40 — e quella stringa e' cio' che un umano legge come evidenza.
    esito = "esito %d/%s completati, %d fallimenti" % (
        completati, trovati if trovati is not None else "?", len(rossi))
    return v, esito, rossi, eseguiti, problemi


# --- self-test della regola, senza motore e senza sorgenti ---------------------------------------
# Ogni caso qui sotto e' un difetto che la riscrittura del 2026-09-08 aveva introdotto e
# che nessun test poteva vedere finche' la regola viveva dentro `suite()`.

_A = ("head1", "albero1", "dll1")
_B = ("head1", "albero2", "dll1")


def _log(trovati=None, avviati=(), completati=0, rossi=(), coda="", testa=""):
    r = [testa] if testa else []
    if trovati is not None:
        r.append("LogAutomationController: Found %d automation tests based on 'X'" % trovati)
    for n in avviati:
        r.append("Test Started. Name={%s} Path={%s}" % (n, n))
    for n in rossi:
        r.append("Result={Fail} Name={%s} Path={%s}" % (n, n))
    r.extend(["Test Completed."] * completati)
    if coda:
        r.append(coda)
    return "\n".join(r)


def self_test():
    casi = []

    def caso(nome, atteso, *a, **k):
        v = verdetto(*a, **k)[0]
        casi.append((nome, v == atteso, "atteso %s, ottenuto %s" % (atteso, v)))

    caso("run verde completa e' VALIDA", "VALIDA",
         _A, _A, _log(trovati=2, avviati=("a", "b"), completati=2))
    caso("run rossa completa e' comunque VALIDA", "VALIDA",
         _A, _A, _log(trovati=2, avviati=("a", "b"), completati=2, rossi=("b",)))
    caso("albero cambiato e' NON VALIDA", "NON VALIDA",
         _A, _B, _log(trovati=2, avviati=("a", "b"), completati=2))
    caso("crash a meta' suite e' NON VALIDA, non VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=40, avviati=("a",), completati=1,
                      coda="=== Critical error: === Fatal error!"))
    caso("suite troncata e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=300, avviati=("a",), completati=40))
    caso("filtro che non corrisponde e' NON AVVIATA", "NON AVVIATA",
         _A, _A, "LogAutomationController: nessun test")
    caso("editor morto in avvio e' NON AVVIATA", "NON AVVIATA", _A, _A, "LogInit: avvio")
    caso("assertion fallita e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=2, avviati=("a", "b"), completati=2,
                      coda="Assertion failed: Index >= 0"))

    v, esito, _, _, _ = verdetto(_A, _A, _log(trovati=300, avviati=("a",), completati=40))
    casi.append(("l'esito non fabbrica il denominatore", "40/300" in esito, esito))

    v, esito, rossi, _, _ = verdetto(_A, _A, _log(trovati=2, avviati=("a", "b"),
                                                  completati=2, rossi=("b",)))
    casi.append(("i rossi sono nominati", rossi == {"b"}, str(rossi)))

    _, _, _, _, problemi = verdetto(_A, _B, _log(trovati=1, avviati=("a",), completati=1,
                                                 coda="appError called"))
    casi.append(("drift e crash insieme riportano entrambi",
                 any(p.startswith("albero") for p in problemi)
                 and any(p.startswith("motore") for p in problemi), str(len(problemi))))

    return casi
