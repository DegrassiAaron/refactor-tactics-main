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
import io
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


class GitNonLeggibile(Exception):
    """L'albero non e' stato letto. NON e' «albero pulito»."""


def _git(radice, *a, **kw):
    """🔴 L'exit code si controlla. Un git che fallisce e restituisce stringa vuota
    farebbe combaciare le due istantanee, e l'invariante fallirebbe **APERTA** — che e'
    il modo peggiore: indistinguibile dal caso sano. Basta un'altra sessione che tiene
    `.git/index.lock` a meta' di un `checkout` mentre la suite gira.

    `byte=True` NON decodifica: serve ai PERCORSI. Con `text=True` Python usa la codepage
    locale, e un nome come `citta'.cpp` torna `cittÃ .cpp` — che non si apre. I byte si
    passano a `os.fsdecode`, che su Windows usa UTF-8 (PEP 529) ed e' l'unica via che
    riapre davvero il file."""
    r = subprocess.run(["git"] + list(a), cwd=radice, capture_output=True,
                       **({} if kw.get("byte") else {"text": True, "errors": "replace"}))
    if r.returncode != 0:
        err = r.stderr if not kw.get("byte") else (r.stderr or b"").decode("utf-8", "replace")
        raise GitNonLeggibile("git %s -> exit %d: %s"
                              % (" ".join(a), r.returncode, (err or "").strip()[:120]))
    if kw.get("byte"):
        return r.stdout or b""
    return r.stdout if kw.get("grezzo") else r.stdout.strip()


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
    # 🔴 `-z` **e** byte grezzi, non `splitlines()` su testo decodificato. Due trappole
    # distinte sullo stesso nome, e vanno tolte entrambe: col default git QUOTA i nomi
    # non-ASCII (`"citt\303\240.cpp"`, virgolette e ottali), e `text=True` li decodifica
    # con la codepage locale, che ne fa `cittÃ .cpp`. In un caso o nell'altro `open()`
    # fallisce, la firma degrada a un valore che NON cambia col contenuto, e l'invariante
    # diventa cieca proprio sui file che il repository ha (`#166`). Misurato: senza i
    # byte, riscrivere `citta'-prova.cpp` da capo lasciava l'istantanea identica.
    untracked = _git(radice, "-c", "core.quotepath=false",
                     "ls-files", "--others", "--exclude-standard", "-z", byte=True)
    firme = []
    for p in sorted(os.fsdecode(x) for x in untracked.split(b"\0") if x):
        assoluto = os.path.join(radice, p)
        try:
            with open(assoluto, "rb") as fh:
                firme.append("%s %s" % (p, hashlib.sha1(fh.read()).hexdigest()))
        except (IOError, OSError) as e:
            # ⚠️ Non e' «assente» e non e' un valore costante: un placeholder uguale per
            # due letture diverse renderebbe invisibile una modifica. Ci si mette dentro
            # l'errore, che cambia se cambia la ragione.
            firme.append("%s (non leggibile: %s)" % (p, e.__class__.__name__))
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

    # I due INSIEMI servono ai gate per la differenza contro la baseline; i CONTEGGI
    # servono alla regola. ⚠️ Non sono la stessa cosa: `rossi` nasce da un match che
    # pretende `Name={…} Path={…}` sulla stessa riga, e una riga di riepilogo
    # `Result={Fail}` nuda non vi rientra — contando l'insieme, l'esito scritto nel
    # referto sarebbe un limite inferiore spacciato per il numero di fallimenti.
    rossi = set(re.findall(r"Result=\{Fail\} Name=\{[^}]*\} Path=\{([^}]+)\}", testo_log))
    eseguiti = set(re.findall(r"Test Started\. Name=\{[^}]*\} Path=\{([^}]+)\}", testo_log))
    avviati = len(re.findall(r"Test Started\.", testo_log))
    completati = len(re.findall(r"Test Completed\.", testo_log))
    falliti = len(re.findall(r"Result=\{Fail\}", testo_log))
    trovati = None
    m = re.search(r"Found (\d+) automation tests", testo_log)
    if m:
        trovati = int(m.group(1))

    # Il termine cambiato si NOMINA: un `Build.bat` di un altro checkout e una `HEAD` che
    # si e' mossa richiedono due reazioni diverse, e con un messaggio solo la diagnosi
    # costa un'altra run.
    for indice, termine in enumerate(("HEAD", "contenuto dell'albero", "binari")):
        if prima[indice] != dopo[indice]:
            problemi.append("albero    %s cambiato durante la run: %r -> %r"
                            % (termine, str(prima[indice])[:60], str(dopo[indice])[:60]))

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
            # L'ultima riga scritta e' la sola diagnosi disponibile: senza, `misura()`
            # ricostruisce e riprova, spendendo mezz'ora su una condizione che non e' un
            # problema di build.
            for riga in reversed(testo_log.split("\n")):
                if riga.strip():
                    r = riga.strip()
                    problemi.append("          ultima riga: " + (r[:157] + "..." if len(r) > 160 else r))
                    break
            if "Installing collected packages" in testo_log or "PipInstall" in testo_log:
                problemi.append("          ^ e' il bootstrap di Intermediate/PipInstall: 4,8 GB di dipendenze")
                problemi.append("            Python al PRIMO avvio di un checkout nuovo (`#2393`). Scaldare il")
                problemi.append("            worktree con un avvio dedicato e rimisurare.")
    # 🔴 **La soglia e' `avviati`, non `completati`, e la differenza e' misurata.** Su una
    # suite intera l'ULTIMO test perde regolarmente la riga di conclusione nel flush di
    # shutdown: 1232 avviati e 1231 conclusi e' una run perfettamente sana. Invalidare su
    # `trovati` dichiarerebbe NON VALIDA **ogni** suite completa — cioe' renderebbe questi
    # gate inutili proprio nel caso per cui esistono, perche' la baseline non sarebbe mai
    # misurabile. Cio' che conta e' quanti test sono PARTITI.
    elif avviati < trovati:
        troncata = True
        problemi.append("copertura %d/%d avviati: la run e' stata troncata (%d non partiti,"
                        " %d fallimenti)" % (avviati, trovati, trovati - avviati, falliti))
    elif trovati == 1 and completati < 1:
        # 🔴 **La tolleranza vale solo se c'e' una coda da tollerare.** La sua
        # giustificazione e' che l'ULTIMO test perde la conclusione: su 1232, uno e'
        # rumore. Su un filtro che seleziona ESATTAMENTE un test, lo stesso uno e' il
        # 100% della misura, e «l'ultimo» e' anche «l'unico».
        # ⚠️ Ed e' il caso che conta di piu' qui, non un caso limite: la verifica per
        # mutazione RICHIEDE filtri stretti — si muta una riga e si esegue il test che
        # deve cadere — quindi `trovati` vale 1 o 2 per costruzione.
        troncata = True
        problemi.append("copertura %d/1 completati: un test solo avviato e mai concluso non"
                        " ha una lettura benigna" % completati)
        problemi.append("          (la coda di shutdown copre l'ULTIMO test di una suite,"
                        " non l'UNICO)")
    elif "**** TEST COMPLETE. EXIT CODE:" not in testo_log:
        # `roadmap-main-v0.1.md` §7 chiede DUE righe nel log, non una: `Found N` in testa
        # e questa in fondo. Senza, una run uccisa dopo l'ultimo `Test Completed.` e prima
        # del terminatore passerebbe per conclusa.
        troncata = True
        problemi.append("copertura il log non porta «**** TEST COMPLETE. EXIT CODE: n ****»:"
                        " la run non e' terminata")

    if crash or drift or troncata:
        v = "NON VALIDA"
    elif trovati is None or not eseguiti:
        v = "NON AVVIATA"
    else:
        v = "VALIDA"

    # 🔴 Il denominatore e' `Found N`, non il numeratore. Scriverlo come
    # `"%d/%d" % (len(eseguiti), len(eseguiti))` produce «40/40 completati» per una suite
    # di 300 troncata a 40 — e quella stringa e' cio' che un umano legge come evidenza.
    # E i fallimenti si CONTANO, non si deducono dalla cardinalita' di `rossi`.
    esito = "esito %d/%s completati, %d fallimenti" % (
        completati, trovati if trovati is not None else "?", falliti)
    return v, esito, rossi, eseguiti, problemi


def esegui_suite(radice, engine_cmd, uproject, log_path, filtro, dll_glob=None):
    """Lancia la suite e torna `(verdetto, esito, rossi, eseguiti, problemi)`.

    Sede UNICA dell'invocazione del motore: gli argomenti di `UnrealEditor-Cmd`, la
    rimozione del log stantio e le due istantanee vivevano in copia in entrambi i gate,
    e sarebbero tornate a divergere come tutto il resto.

    ⚠️ NON e' pura — avvia un processo. La parte che DECIDE e' `verdetto()`, che e' pura
    ed e' cio' che il self-test esercita.
    """
    def guasto(messaggio):
        return "NON VALIDA", "esito ?/? completati, ? fallimenti", set(), set(), [messaggio]

    try:
        prima = istantanea(radice, dll_glob)
    except GitNonLeggibile as e:
        return guasto("albero    non letto PRIMA della run: %s" % e)

    if os.path.exists(log_path):
        try:
            os.remove(log_path)   # un log vecchio darebbe i rossi di una run che non e' questa
        except OSError as e:
            # Un Editor zombie che tiene il handle aperto: senza questa guardia il gate
            # muore di traceback invece di dire cosa lo ferma.
            return guasto("log       non rimovibile (%s): un processo lo tiene aperto. "
                          "Il log della run precedente falserebbe questa." % e.__class__.__name__)

    esecuzione = subprocess.run(
        [engine_cmd, uproject,
         "-ExecCmds=Automation RunTests " + filtro + ";Quit",
         "-unattended", "-nopause", "-nosplash", "-nullrhi", "-NoLiveCoding",
         "-log=" + os.path.basename(log_path)],
        capture_output=True, text=True, errors="replace")

    testo = ""
    if os.path.exists(log_path):
        testo = io.open(log_path, encoding="utf-8", errors="replace").read()
    else:
        # 🔴 Nessun log e nessuna diagnosi sarebbe la peggiore combinazione: `misura()`
        # ricostruirebbe e riproverebbe per mezz'ora senza sapere perche'. Il codice di
        # uscita del motore e' l'unica cosa rimasta da dire.
        coda = ((esecuzione.stderr or "") + (esecuzione.stdout or "")).strip().split("\n")
        return guasto("motore    log non prodotto: uscito con codice %d. Ultima riga: %s"
                      % (esecuzione.returncode, (coda[-1] if coda else "(nessun output)")[:120]))

    try:
        dopo = istantanea(radice, dll_glob)
    except GitNonLeggibile as e:
        return guasto("albero    non letto DOPO la run: %s" % e)

    return verdetto(prima, dopo, testo, filtro)


# --- self-test della regola, senza motore e senza sorgenti ---------------------------------------
# Ogni caso qui sotto e' un difetto che la riscrittura del 2026-09-08 aveva introdotto e
# che nessun test poteva vedere finche' la regola viveva dentro `suite()`.

_A = ("head1", "albero1", "dll1")
_B = ("head1", "albero2", "dll1")


def _log(trovati=None, avviati=0, completati=0, rossi=(), coda="", testa="", terminatore=True):
    """Log finto. `avviati` e' un NUMERO — su una suite intera i nomi non contano, contano
    i conteggi; i nomi servono solo ai rossi e ai bersagli."""
    r = [testa] if testa else []
    if trovati is not None:
        r.append("LogAutomationController: Found %d automation tests based on 'X'" % trovati)
    for i in range(avviati):
        r.append("Test Started. Name={t%d} Path={t%d}" % (i, i))
    for n in rossi:
        r.append("Result={Fail} Name={%s} Path={%s}" % (n, n))
    r.extend(["Test Completed."] * completati)
    if coda:
        r.append(coda)
    if terminatore:
        r.append("**** TEST COMPLETE. EXIT CODE: 0 ****")
    return "\n".join(r)


def self_test():
    casi = []

    def caso(nome, atteso, *a, **k):
        v = verdetto(*a, **k)[0]
        casi.append((nome, v == atteso, "atteso %s, ottenuto %s" % (atteso, v)))

    # 🔑 **La coppia che conta e' `coda-sana` / `crash-ultimo-test`**: conteggi IDENTICI,
    # una riga di crash in piu'. Se i due esiti coincidessero, o la tolleranza sulla coda
    # coprirebbe un crash, o l'invalidazione su `trovati` renderebbe questi gate inutili
    # — perche' NESSUNA suite intera arriva con `completati == trovati`.
    caso("coda-sana: 1232/1232 avviati, 1231 conclusi e' VALIDA", "VALIDA",
         _A, _A, _log(trovati=1232, avviati=1232, completati=1231))
    caso("crash-ultimo-test: stessi conteggi + una riga di crash e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=1232, avviati=1232, completati=1231,
                      coda="LogWindows: Error: === Critical error: ==="))

    caso("suite completa con due rossi e' VALIDA", "VALIDA",
         _A, _A, _log(trovati=174, avviati=174, completati=174, rossi=("a", "b")))
    caso("un test solo, concluso, e' VALIDA", "VALIDA",
         _A, _A, _log(trovati=1, avviati=1, completati=1))
    caso("un test solo, avviato e mai concluso, e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=1, avviati=1, completati=0))
    caso("un test solo, crashato, e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=1, avviati=1, completati=0, coda="appError called"))
    caso("troncata: 60 avviati su 100 e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=100, avviati=60, completati=60))
    caso("assertion fallita e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=4, avviati=4, completati=4,
                      coda="LogCore: Error: Assertion failed: Check(bValid) [Line: 12]"))
    caso("senza il terminatore la run non e' conclusa", "NON VALIDA",
         _A, _A, _log(trovati=4, avviati=4, completati=4, terminatore=False))
    caso("albero cambiato e' NON VALIDA", "NON VALIDA",
         _A, _B, _log(trovati=2, avviati=2, completati=2))
    caso("filtro che non corrisponde e' NON AVVIATA", "NON AVVIATA",
         _A, _A, "LogAutomationController: nessun test")
    caso("editor morto in avvio e' NON AVVIATA", "NON AVVIATA", _A, _A, "LogInit: avvio")

    v, esito, _, _, _ = verdetto(_A, _A, _log(trovati=300, avviati=40, completati=40))
    casi.append(("l'esito non fabbrica il denominatore", "40/300" in esito, esito))

    _, esito, rossi, _, _ = verdetto(_A, _A, _log(trovati=2, avviati=2, completati=2,
                                                  rossi=("b",)))
    casi.append(("i rossi sono nominati", rossi == {"b"}, str(rossi)))

    # Due righe `Result={Fail}` per lo STESSO test: l'insieme ne conta una, il referto due.
    doppio = _log(trovati=1, avviati=1, completati=1, rossi=("a", "a"))
    _, esito, _, _, _ = verdetto(_A, _A, doppio)
    casi.append(("i fallimenti si contano, non si deducono dall'insieme",
                 "2 fallimenti" in esito, esito))

    _, _, _, _, problemi = verdetto(_A, _B, _log(trovati=1, avviati=1, completati=1,
                                                 coda="appError called"))
    casi.append(("drift e crash insieme riportano entrambi",
                 any(p.startswith("albero") for p in problemi)
                 and any(p.startswith("motore") for p in problemi), str(len(problemi))))

    return casi
