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

## I quattro termini, e come sono coperti

`istantanea()` fotografa i primi tre prima e dopo la run: `HEAD`, il **contenuto**
dell'albero, la firma dei binari. Il quarto — *nessun processo del motore estraneo durante
la run* — lo osserva `esegui_suite()`, campionando i processi e scartando quelli che
discendono dal proprio (`#2672`).

## Cosa NON copre, dichiarato

* **Il campionamento e' discreto.** Una suite altrui che nasce e muore fra due campioni non
  viene vista: copre la finestra lunga, non l'istante. Non c'e' piu' un lease, e questo
  modulo non ne e' il sostituto — vede cio' che passa, non impedisce che passi.
* **Il codice di uscita del motore non entra nel verdetto**, e non e' una dimenticanza:
  `UnrealEditor-Cmd` esce `-1` quando la suite ha dei rossi. Vedi `verdetto()`.
* **Il terminatore `**** TEST COMPLETE ****` e' un avviso, non un verdetto** — e ora si sa
  perche'. Misurato il 2026-09-12 su sei log: compare nei sani, ed e' ASSENTE in tutti e tre
  quelli in cui il processo non e' uscito. Quindi e' confermato su un log di `esegui_suite()`
  (`automation-mutazione.log`, `mut-M1.log`), ma **non e' un oracolo di fine**: attenderlo per
  sapere se la suite e' finita significa attendere per sempre proprio nel caso da coprire.
  Cio' che dice la fine sono i CONTEGGI — vedi `suite_finita()` (`#3048`).
* **Ne' lo e' `...Automation Test Queue Empty N tests performed.`**, che nei log di
  `esegui_suite()` non compare affatto: e' una riga della modalita' `+Quit`, e questo modulo
  usa `;Quit` dentro `-ExecCmds`. Un oracolo misurato su una modalita' non si trasferisce
  all'altra — la correlazione `+Quit` / processo che non esce sta in `#3049`.
"""

import hashlib
import io
import os
import re
import subprocess
import tempfile
import time

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


def processi_motore():
    """I PID dei processi del motore vivi ORA. `None` se l'enumerazione non e' riuscita.

    🔴 I **PID**, non il conteggio: il gate avvia lui stesso un motore, quindi durante la
    run il numero e' sempre >= 1 e non dice niente. Sapere QUALI permette di sottrarre il
    proprio e rispondere alla sola domanda che conta: ne e' comparso uno che non e' mio?

    🔴 `None` NON e' «nessun processo»: un'enumerazione fallita che tornasse `[]` sarebbe
    un'invariante che fallisce APERTA, indistinguibile dal caso sano.
    """
    alberi = _alberi_processi()
    return None if alberi is None else sorted(alberi["motori"])


def _alberi_processi():
    """`{'motori': {pid}, 'padre': {pid: ppid}}`, oppure `None` se non si e' potuto leggere.

    🔴 Serve il **padre**, non solo il conteggio. `UnrealEditor-Cmd` genera processi figli:
    contarli e confrontare due numeri classifica un figlio legittimo come estraneo — e con
    l'ordine dei campioni invertito assorbe un motore ALTRUI fra i propri. Entrambe le
    direzioni sono sbagliate, e una e' un fail-open. La parentela e' l'unico dato che
    risponde davvero a «questo processo l'ho avviato io?».

    ⚠️ `Get-CimInstance` costa piu' di `tasklist` (misurato altrove nel progetto: 8 ms
    contro 117). Si paga una volta per campione, ed e' il prezzo di una risposta giusta.
    """
    try:
        esito = subprocess.run(
            [PWSH, "-NoProfile", "-NonInteractive", "-Command",
             "Get-CimInstance Win32_Process | "
             "Select-Object ProcessId,ParentProcessId,Name,CommandLine | "
             "ForEach-Object { '{0};{1};{2};{3}' -f $_.ProcessId,$_.ParentProcessId,$_.Name,"
             "$(if ($_.Name -like 'Unreal*') { $_.CommandLine }) }"],
            capture_output=True, text=True, errors="replace")
    except OSError:
        # Interprete non risolvibile: PATH senza System32, container, host non-Windows.
        # Senza questo ramo la funzione sollevava invece di rispettare il proprio contratto.
        return None
    if esito.returncode != 0:
        return None
    return _parse_processi((esito.stdout or "").splitlines())


def _parse_processi(righe):
    """PURA. `{'motori': {pid}, 'padre': {pid: ppid}, 'livecoding': {pid: ppid}}`.

    🔴 **Separata da `_alberi_processi` perche' i due filtri sono la sostanza, e senza
    questa firma nessun test li vede**: `classifica_livecoding` riceve il dict GIA' costruito,
    quindi togliere un filtro da qui non farebbe cadere nessun caso che parte dal dict.
    """
    motori, padre, livecoding, motori_info = set(), {}, {}, {}
    for riga in righe:
        # ⚠️ `maxsplit=3`: una `CommandLine` contiene `;`, e rispezzarla la troncherebbe.
        # Il quarto campo si prende INTERO. Tre campi restano accettati: e' la forma che la query
        # emetteva prima, e i casi del self-test che non hanno bisogno della CommandLine la usano.
        campi = riga.strip().split(";", 3)
        if len(campi) < 3 or not campi[0].isdigit():
            continue
        pid, ppid, nome = int(campi[0]), int(campi[1]) if campi[1].isdigit() else 0, campi[2]
        cmdline = campi[3] if len(campi) > 3 else None
        padre[pid] = ppid
        if "UnrealEditor" in nome:
            motori.add(pid)
            # #3044: il NOME dice interattivo contro headless, la `CommandLine` dice
            # `-NoLiveCoding`. Sono i due dati che `D-400` richiede per poter terminare.
            motori_info[pid] = (nome, cmdline)
        # 🔴 #2392: `LiveCodingConsole` NON contiene `UnrealEditor`, e tiene lo stesso lock
        # di compilazione. Letto QUI, nella stessa passata, perche' due query correlate non
        # concordano: fra due campioni un processo nasce o muore.
        if "LiveCoding" in nome:
            livecoding[pid] = ppid
    return {"motori": motori, "padre": padre, "livecoding": livecoding,
            "motori_info": motori_info}


def _discende_da(pid, radici, padre, profondita=24):
    """True se `pid` e' `radici` o un loro discendente. `profondita` spezza i cicli."""
    corrente = pid
    for _ in range(profondita):
        if corrente in radici:
            return True
        successivo = padre.get(corrente)
        if not successivo or successivo == corrente:
            return False
        corrente = successivo
    return False


def attendi_motore_libero(minuti=90, campionamento=45, stampa=None):
    """Attende che il motore si liberi. Torna True se e' libero, False se scade il tempo.

    🔑 **Attesa, non rifiuto.** Un gate che esce `2` perche' la macchina e' occupata butta
    via il lavoro di preparazione — e su questa macchina «occupata» e' la condizione
    normale, non l'eccezione. `rt-suite.ps1` attendeva in coda fino a 90 minuti
    (`-WaitMinutes`), e la prosa dei due gate ha continuato a descrivere quell'attesa anche
    dopo la sua rimozione. Qui l'attesa esiste di nuovo, ed e' quella che la prosa dichiara.

    ⚠️ Resta una precondizione: due gate che iniziano ad attendere insieme si vedono
    entrambi liberi nello stesso istante. Cio' che copre la finestra *durante* la run e'
    `esegui_suite()`, che campiona i PID estranei.
    """
    scadenza = minuti * 60
    atteso = 0
    while True:
        pid = processi_motore()
        if pid is None:
            return False          # enumerazione fallita: fail-closed, non si parte
        if not pid:
            return True
        if atteso >= scadenza:
            return False
        if stampa and atteso % (campionamento * 4) == 0:
            stampa("   motore occupato da %d process%s, attendo (%d min di %d)"
                   % (len(pid), "o" if len(pid) == 1 else "i", atteso // 60, minuti))
        time.sleep(campionamento)
        atteso += campionamento


def suite_finita(testo_log):
    """PURA. `(finita, trovati, avviati, completati)` — la suite ha finito di girare?

    🔑 **L'oracolo sono i CONTEGGI, e non e' una scelta di stile: e' l'unico segnale
    presente in tutti i log misurati.** Il 2026-09-12, su quattro log e due modalita' di
    invocazione (`;Quit` dentro `-ExecCmds` come fa `esegui_suite()`, `+Quit` separato per chi
    lancia a mano), i due segnali che sembravano ovvi sono risultati disgiunti:

    * `**** TEST COMPLETE. EXIT CODE:` — nei due log sani **si**, nei due appesi **no**;
    * `...Automation Test Queue Empty N tests performed.` — nei due appesi **si**, nei log di
      `esegui_suite()` **no**.

    Le due modalita' scrivono righe diverse, quindi un oracolo misurato su una non si trasferisce
    all'altra. I conteggi invece c'erano in tutti e quattro.

    ⛔ **La soglia e' quella di `verdetto()`, non una piu' stretta.** `completati >= avviati - 1`
    tollera UNA conclusione mancante, per la ragione che `verdetto()` documenta: nessuna suite
    intera arriva con `completati == trovati`. Un oracolo `completati == trovati` direbbe «mai
    finita» su ogni suite reale, e farebbe aspettare il gate per sempre — cioe' il difetto che
    questa funzione esiste per togliere.

    ⚠️ Dice **finita**, non **valida**: il verdetto resta di `verdetto()`.
    """
    completati = len(re.findall(r"Test Completed\.", testo_log))
    avviati = len(re.findall(r"Test Started\.", testo_log))
    m = re.search(r"Found (\d+) automation tests", testo_log)
    trovati = int(m.group(1)) if m else None
    finita = (trovati is not None and avviati >= trovati and completati >= avviati - 1)
    return finita, trovati, avviati, completati


def grazia_scaduta(finita_da, trascorso, grazia):
    """PURA. La grazia si misura DA quando il log ha dichiarato la suite finita.

    ⚠️ Estratta dal loop di `esegui_suite()` perche' una verifica di mutazione l'ha trovata
    scoperta: `grazia_scaduta = False` lasciava il self-test verde, dato che i casi di
    `decide_attesa` ricevono la grazia come INGRESSO e non vedono chi la calcola.

    🔑 L'origine e' `finita_da`, non l'avvio della run: una suite di due ore che finisce
    e non esce va terminata due minuti dopo la FINE, non due minuti dopo l'avvio.
    """
    return finita_da is not None and trascorso - finita_da >= grazia


def decide_attesa(log_finito, processo_vivo, grazia_scaduta):
    """PURA. `attendi` | `uscito` | `appeso`.

    🔴 **`appeso` e' lo stato che prima non esisteva**, e costava `timeout_minuti` interi:
    il log dichiara la suite finita, il processo non esce, e il gate lo leggeva come «nessuna
    risposta». Ha risposto — e nel referto i due casi vanno distinti, perche' il primo e' un
    motore morto e il secondo una misura **valida** che si puo' ancora raccogliere.

    ⚠️ La grazia esiste perche' un processo sano esce qualche istante DOPO l'ultima riga di
    log: terminare appena i conteggi tornano ucciderebbe run che stavano per uscire da se'.
    """
    if not processo_vivo:
        return "uscito"
    if log_finito and grazia_scaduta:
        return "appeso"
    return "attendi"


def verdetto(prima, dopo, testo_log, filtro="", estranei=(), uscita_motore=0):
    """PURA. Torna `(verdetto, esito, rossi, eseguiti, problemi)`.

    Ordine dei controlli, e non e' indifferente: un log crashato che ha anche visto
    l'albero cambiare e' `NON VALIDA` per entrambe le ragioni, e le riporta entrambe.

    * `NON VALIDA`  la misura non e' registrabile: drift, crash, o suite troncata;
    * `NON AVVIATA` non ha misurato niente: motore morto in avvio, o filtro a vuoto;
    * `VALIDA`      registrabile — verde o rossa che sia.

    `estranei` sono i PID di motori NON avviati da questa misura, visti mentre girava:
    e' il quarto termine dell'invariante di `AGENTS.md`, e senza di esso il verdetto
    dichiarerebbe valida una run attraversata dalla suite di un'altra sessione.

    `uscita_motore` e' il codice di uscita del processo: un crash allo shutdown puo' non
    lasciare nessuno dei quattro marcatori nel log e uscire comunque diverso da zero.
    """
    problemi = []

    if estranei:
        problemi.append("motore    %d process%s estrane%s durante la run (PID %s): la misura"
                        " ha condiviso la macchina"
                        % (len(estranei), "o" if len(estranei) == 1 else "i",
                           "o" if len(estranei) == 1 else "i",
                           ", ".join(str(p) for p in sorted(estranei)[:6])))

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

    # ⛔ **Il codice di uscita NON entra nel verdetto, ed e' misurato.** `UnrealEditor-Cmd`
    # esce `-1` quando la suite ha dei rossi: sui log di questa macchina, 45 run a `0` e
    # **22 a `-1`**. Farne una condizione di `NON VALIDA` INVERTE i due gate — una mutazione
    # che fa cadere il suo bersaglio, cioe' il successo che il gate esiste per produrre,
    # uscirebbe non registrabile e finirebbe fra le sopravvissute. La DoD della v0.1 lo
    # dichiara gia': «un gate automatico non puo' leggere il verdetto dal codice di uscita —
    # si contano i `Result={Success}`».
    #
    # Resta come DIAGNOSTICA: si stampa quando c'e' gia' un problema, per aiutare a capirlo.
    if uscita_motore not in (0, None) and problemi:
        problemi.append("          (il motore e' uscito con codice %s — da solo non dice"
                        " niente: `-1` e' anche una suite con dei rossi)" % uscita_motore)

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
    elif completati < avviati - 1:
        # 🔴 **La tolleranza vale per UNA conclusione, non per un numero qualsiasi.** La sua
        # giustificazione e' che l'ULTIMO test perde la riga nel flush di shutdown: uno, non
        # dieci. ⚠️ `rt-suite.ps1` si fermava a `avviati < trovati` e qui era piu' largo del
        # proprio motivo — `5 avviati, 0 conclusi` passava. Non e' una regressione di questa
        # PR: e' un limite ereditato, stretto qui alla misura che lo giustifica.
        troncata = True
        problemi.append("copertura %d/%d completati su %d avviati: mancano %d conclusioni, e la"
                        " coda di shutdown ne spiega UNA"
                        % (completati, trovati, avviati, avviati - completati))
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
    if trovati is not None and "**** TEST COMPLETE. EXIT CODE:" not in testo_log:
        # `roadmap-main-v0.1.md` §7 chiede DUE righe nel log, non una: `Found N` in testa e
        # questa in fondo, e una run uccisa dopo l'ultimo `Test Completed.` non ha la seconda.
        #
        # ⚠️ **AVVISO, non verdetto — e ora si sa perche' il contro-esempio non lo aveva.**
        #
        # Misurato il 2026-09-08 con un motore vero: le due run prodotte da `esegui_suite()`
        # — una verde (`10/10`) e una con un rosso (`exit -1`) — portano ENTRAMBE la riga
        # `LogAutomationCommandLine: Display: **** TEST COMPLETE. EXIT CODE: n ****`.
        # `Saved/Logs/830-final.log`, che non ce l'ha pur avendo eseguito 824 test, era stato
        # lanciato con `-ExecCmds="Automation RunTests RefactorTactics+Quit"`: col `+`, che
        # nei filtri Automation e' un separatore, `Quit` finisce DENTRO il filtro invece di
        # essere un comando, e la sequenza non si conclude mai. Qui si usa `;Quit`.
        #
        # Resta un avviso perche' due run non bastano a rendere BLOCCANTE una condizione che,
        # se sbagliata, ferma entrambi i gate sul nascere: il costo di lasciarla avviso e'
        # quasi nullo — si stampa comunque — e quello di sbagliarla e' che non si misura piu'
        # niente. Si promuove quando i casi saranno molti. Vedi `#2672`.
        problemi.append("avviso    il log non porta «**** TEST COMPLETE. EXIT CODE: n ****»."
                        " Se la run sembra completa, verificare a mano che sia terminata")

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


def esegui_suite(radice, engine_cmd, uproject, log_path, filtro, dll_glob=None,
                 timeout_minuti=180, campionamento=20, grazia=120):
    """Lancia la suite e torna `(verdetto, esito, rossi, eseguiti, problemi)`.

    Sede UNICA dell'invocazione del motore: gli argomenti di `UnrealEditor-Cmd`, la
    rimozione del log stantio e le due istantanee vivevano in copia in entrambi i gate,
    e sarebbero tornate a divergere come tutto il resto.

    ⚠️ NON e' pura — avvia un processo. La parte che DECIDE e' `verdetto()`, che e' pura
    ed e' cio' che il self-test esercita: qui si RACCOLGONO i segnali (istantanee, log,
    PID estranei, codice di uscita), la' si decide.
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

    # 🔑 **`Popen` e non `run`, per due ragioni che `run` non permette.**
    #
    # (1) Il TIMEOUT. Un `UnrealEditor-Cmd` appeso bloccherebbe il gate per sempre, con un
    #     sorgente mutato sul disco: e' un caso registrato su questa macchina, non teorico.
    # (2) Il QUARTO TERMINE. Mentre la run gira si campionano i processi del motore e si
    #     scarta cio' che discende dal proprio: il resto e' ESTRANEO. E' l'unica finestra in
    #     cui quel termine sia osservabile — a run finita il processo altrui puo' essere gia'
    #     morto, e prima di partire non e' ancora nato.
    #
    # 🔴 **Su FILE, non su `PIPE`.** `Popen(stdout=PIPE)` senza drenare va in deadlock appena
    # il buffer del sistema si riempie — poche decine di KB, e una suite intera ne emette
    # megabyte: il figlio si blocca in scrittura, `poll()` resta `None` per sempre e il gate
    # aspetta il timeout intero per ogni run. `subprocess.run(capture_output=True)`, che
    # questo codice ha sostituito, drenava le pipe con thread propri. Il log vero e' comunque
    # il file `-log=`; qui basta una coda per diagnosticare il caso «log non prodotto».
    #
    # ⚠️ Resta un limite dichiarato: il campionamento e' discreto. Una suite altrui che nasce
    # e muore fra due campioni non viene vista. Copre la finestra lunga, non l'istante.
    uscita_grezza = tempfile.TemporaryFile()
    avvio = subprocess.Popen(
        [engine_cmd, uproject,
         "-ExecCmds=Automation RunTests " + filtro + ";Quit",
         "-unattended", "-nopause", "-nosplash", "-nullrhi", "-NoLiveCoding",
         "-log=" + os.path.basename(log_path)],
        stdout=uscita_grezza, stderr=subprocess.STDOUT)

    # 🔴 **La parentela, non il conteggio.** Un processo e' MIO se discende dal mio: e' la
    # sola domanda a cui si possa rispondere senza sbagliare in una delle due direzioni.
    # Confrontare cardinalita' marcava un figlio legittimo come estraneo (ogni run NON
    # VALIDA) e, a campione invertito, assorbiva un motore altrui fra i propri — un
    # fail-open, cioe' esattamente cio' che questo campionamento esiste per chiudere.
    estranei = set()
    trascorso = 0
    finita_da = None        # l'istante in cui il LOG ha dichiarato la suite finita
    appeso = False
    while True:
        vivo = avvio.poll() is None
        # 🔴 #3048: la fine si legge dal LOG, non dall'uscita del processo. Un
        # `UnrealEditor-Cmd` che non esce dopo `Quit` e' un caso registrato e frequente, e
        # attendere `poll()` costava `timeout_minuti` interi su una suite gia' finita, per poi
        # dichiarare «nessuna risposta» — mentre il log portava i conteggi completi.
        log_finito = False
        if vivo and os.path.exists(log_path):
            try:
                log_finito = suite_finita(
                    io.open(log_path, encoding="utf-8", errors="replace").read())[0]
            except OSError:
                log_finito = False   # il motore lo tiene aperto: si riprova al campione dopo
        if log_finito and finita_da is None:
            finita_da = trascorso
        esito_attesa = decide_attesa(log_finito, vivo,
                                     grazia_scaduta(finita_da, trascorso, grazia))
        if esito_attesa == "uscito":
            break
        if esito_attesa == "appeso":
            _termina_albero(avvio)
            appeso = True
            break
        if trascorso >= timeout_minuti * 60:
            _termina_albero(avvio)
            return guasto("motore    nessuna risposta dopo %d minuti, e il log non dichiara la"
                          " suite finita: albero di processi terminato. Il sorgente mutato e'"
                          " ancora sul disco, ricostruire." % timeout_minuti)

        alberi = _alberi_processi()
        if alberi is not None:
            for pid in alberi["motori"]:
                if not _discende_da(pid, {avvio.pid}, alberi["padre"]):
                    estranei.add(pid)
        time.sleep(campionamento)
        trascorso += campionamento

    avvio.wait()
    uscita_grezza.flush()
    uscita_grezza.seek(0)
    coda_output = uscita_grezza.read().decode("utf-8", "replace")
    uscita_grezza.close()

    testo = ""
    if os.path.exists(log_path):
        testo = io.open(log_path, encoding="utf-8", errors="replace").read()
    else:
        # 🔴 Nessun log e nessuna diagnosi sarebbe la peggiore combinazione: `misura()`
        # ricostruirebbe e riproverebbe per mezz'ora senza sapere perche'.
        righe = [r for r in coda_output.strip().split("\n") if r.strip()]
        return guasto("motore    log non prodotto: uscito con codice %d. Ultima riga: %s"
                      % (avvio.returncode, (righe[-1] if righe else "(nessun output)")[:120]))

    try:
        dopo = istantanea(radice, dll_glob)
    except GitNonLeggibile as e:
        return guasto("albero    non letto DOPO la run: %s" % e)

    v, esito, rossi, eseguiti, problemi = verdetto(prima, dopo, testo, filtro, estranei,
                                                   avvio.returncode)
    if appeso:
        # ⚠️ AVVISO, non guasto, ed e' la distinzione che #3048 esiste per fare: il log
        # porta i conteggi completi, quindi la misura e' raccoglibile. Prima questo caso
        # diventava «nessuna risposta dopo 180 minuti», cioe' una misura persa.
        problemi = list(problemi) + [
            "avviso    la suite era FINITA e il processo non e' uscito: albero terminato dopo"
            " %d s di grazia. I conteggi del log sono completi, quindi la misura vale." % grazia]
    return v, esito, rossi, eseguiti, problemi


def _termina_albero(processo):
    """Termina il processo E i suoi discendenti.

    🔴 `kill()` da solo uccide il figlio diretto: cio' che ha generato resta vivo, e su
    questa macchina «ogni run lascia un processo appeso» e' un caso registrato. Un motore
    orfano fa attendere 90 minuti la run successiva, che poi rinuncia: un solo blocco ne
    costerebbe due."""
    try:
        subprocess.run(["taskkill", "/T", "/F", "/PID", str(processo.pid)],
                       capture_output=True)
    except OSError:
        pass
    try:
        processo.kill()
    except OSError:
        pass
    processo.wait()


# --- cio' che i due gate condividono, e che era in copia -----------------------------------------
# Il motore, l'interprete e i percorsi erano ripetuti in entrambi i file, e le copie avevano
# gia' iniziato a divergere: il controllo del motore era scritto in due modi, e un gate
# ri-derivava il path del progetto pur avendone la costante. Vale qui la stessa regola di
# `verdetto()`: una sede sola.

RADICE = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
UPROJECT = os.path.join(RADICE, "RefactorTactics.uproject")
LOG = os.path.join(RADICE, "Saved", "Logs", "automation-mutazione.log")
BUILD_BAT = r"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat"
ENGINE_CMD = r"D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
DLL_GLOB = os.path.join(RADICE, "Binaries", "Win64", "UnrealEditor-RefactorTactics*.dll")

# 🔴 `pwsh`, non `powershell`: e' l'interprete su cui il quoting di `build()` e' stato
# misurato. ⚠️ La ragione storica — il parsing di `rt-suite.ps1` — e' SCADUTA con lo script
# (2026-09-08); questa resta, e con `powershell` non e' stata riprovata.
PWSH = "pwsh"

# Frasi che dicono «il motore e' occupato», non «il codice non compila». Sulla prima si
# ritenta, sulla seconda ritentare costa mezz'ora per un esito che non cambia.
#
# ⛔ **`OtherCompilationError` NON e' qui, ed e' misurato.** UBT lo emette anche per un
# errore C++ vero: la DoD della v0.1 registra `Result: Failed (OtherCompilationError)`
# causato da sei asserzioni che chiamavano `GetBoolMetaData`. Includerlo faceva ritentare
# quaranta volte in silenzio proprio il caso che questa lista esiste per far fallire subito
# — e una mutazione scritta a mano spesso non compila, quindi e' il caso comune.
LOCK_LIVE_CODING = "Unable to build while Live Coding is active"
CONTESA = (LOCK_LIVE_CODING, "waiting for another instance", "mutex")


def classifica_livecoding(livecoding, vivi):
    """PURA. Chi tiene il lock di Live Coding. Torna `(stato, orfani)`.

    `livecoding`: `{pid: ppid}` dei processi `LiveCoding*`, o `None` se l'enumerazione e' fallita.
    `vivi`: i pid visti nello STESSO campione. `stato`: `sconosciuto` | `assente` | `orfano` |
    `attivo`. `orfani`: `[(pid, ppid)]` ordinati, vuoto quando lo stato non e' `orfano`.

    🔴 **`sconosciuto` NON e' `assente`**, per la stessa ragione per cui `processi_motore()`
    torna `None` invece di `[]`: un'enumerazione fallita che dicesse «nessun LiveCoding» direbbe
    «non c'e' un orfano» per dire «non lo so».

    🔑 **Un orfano accanto a un attivo vince.** Due cloni, uno ha chiuso l'Editor e l'altro
    no: il lock dell'orfano resta comunque, e nessuna attesa lo rilascia.
    """
    if livecoding is None:
        return "sconosciuto", []
    if not livecoding:
        return "assente", []
    orfani = sorted((pid, ppid) for pid, ppid in livecoding.items() if ppid not in vivi)
    return ("orfano" if orfani else "attivo"), orfani


def puo_terminare_orfano(motori):
    """PURA. `(True, ragione)` se NESSUN motore vivo puo' star usando Live Coding — `D-400`.

    `motori`: `{pid: (nome, cmdline)}` dei processi `UnrealEditor*` vivi.

    🔑 **Il criterio e' la `CommandLine`, non il nome, e la differenza e' il caso che ha
    motivato #2392**: il 2026-09-05 era vivo esattamente un motore, un `UnrealEditor-Cmd` di
    automation lanciato con `-NoLiveCoding`. Un processo che non usa Live Coding non e' una
    ragione per conservare l'orfano — e leggere «nessun `UnrealEditor*` vivo» alla lettera
    avrebbe lasciato quel caso irrisolto.

    ⛔ **`D-400` NON autorizza a terminare un `LiveCodingConsole` col padre vivo**, in nessun
    caso: quella e' la seduta di qualcuno. Questa funzione risponde solo alla seconda domanda,
    su un processo gia' classificato `orfano`.

    ⚠️ Fail-closed due volte: una `CommandLine` illeggibile e una run headless senza il
    flag valgono entrambe «non posso escluderlo», non «non lo usa».
    """
    for pid, (nome, cmdline) in sorted(motori.items()):
        if "-Cmd" not in nome:
            return False, "Editor interattivo vivo: pid %d (%s)" % (pid, nome)
        if not cmdline:
            return False, "pid %d: CommandLine illeggibile, non lo posso escludere" % pid
        if "-nolivecoding" not in cmdline.lower():
            return False, "pid %d: run headless senza -NoLiveCoding" % pid
    return True, "nessun motore vivo puo' usare Live Coding"


def decide_ritentativo(testo, stato, puo_terminare=False):
    """PURA. Cosa fare di un build fallito: `riprova` | `riprova-al-buio` | `ferma-orfano` |
    `ferma-compilazione`.

    🔴 **Il caso che questa funzione esiste per distinguere** e' `LOCK_LIVE_CODING` con
    `stato` `orfano`: prima finiva in `riprova` e costava `tentativi × pausa` — mezz'ora sui
    valori di default — su un lock che nessuno rilascera', perche' il processo che lo tiene non ha
    piu' un padre che possa chiuderlo (#2392).

    ⚠️ Il mutex e l'altra istanza restano `riprova` anche con Live Coding `assente`: non
    sono lo stesso guasto, e su quelli l'attesa funziona.
    """
    if not any(f in testo for f in CONTESA):
        return "ferma-compilazione"
    if LOCK_LIVE_CODING not in testo:
        return "riprova"
    if stato == "orfano":
        return "termina-orfano" if puo_terminare else "ferma-orfano"
    if stato == "sconosciuto":
        return "riprova-al-buio"
    return "riprova"


def stato_livecoding():
    """Un campione, due risposte: `(stato, orfani, puo_terminare, ragione)`. Impura.

    Un solo `_alberi_processi()`: chi tiene il lock e chi potrebbe usarlo si leggono dallo
    STESSO istante, altrimenti si decide di terminare sulla base di un motore gia' morto.
    """
    alberi = _alberi_processi()
    if alberi is None:
        stato, orfani = classifica_livecoding(None, set())
        return stato, orfani, False, "enumerazione fallita"
    stato, orfani = classifica_livecoding(alberi["livecoding"], set(alberi["padre"]))
    ok, ragione = puo_terminare_orfano(alberi["motori_info"])
    return stato, orfani, ok, ragione


def build(tentativi=40, pausa=45, stampa=None):
    """Ricostruisce. True solo su `Result: Succeeded`.

    🔴 Un build fallito lascia il binario VECCHIO: la suite girerebbe sul codice NON mutato
    e riporterebbe «nessun test se ne accorge» — indistinguibile da una lacuna vera, e la
    peggiore risposta possibile.

    🔴 **Ma non si ritenta all'infinito su un errore che non cambia.** Prima si ritentava su
    QUALUNQUE fallimento: un errore di compilazione vero costava 40 tentativi a 45 secondi —
    mezz'ora — e non stampava niente. Si distingue la CONTESA (un Editor altrui, il mutex:
    si aspetta) dall'errore di compilazione (si esce subito, con la coda del compilatore).
    """
    terminati = set()
    for tentativo in range(tentativi):
        r = subprocess.run([PWSH, "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command",
                            "& '" + BUILD_BAT + "' RefactorTacticsEditor Win64 Development "
                            "-Project='" + UPROJECT + "' -WaitMutex"],
                           capture_output=True, text=True, errors="replace")
        testo = (r.stdout or "") + (r.stderr or "")   # UBT manda alcune righe su stderr
        if "Result: Succeeded" in testo:
            return True
        # Il campione si paga SOLO quando la frase di Live Coding c'e': sul caso comune — una
        # mutazione scritta a mano che non compila — non si legge nessun processo.
        # 🔑 E si ricampiona a OGNI tentativo, non una volta: un Editor che muore a
        # metà attesa lascia dietro il proprio `LiveCodingConsole`, e dal giro dopo quella
        # che era una contesa legittima è un orfano. Costa un campione per tentativo.
        stato, orfani, puo_term, ragione = ("assente", [], False, "")
        if LOCK_LIVE_CODING in testo:
            stato, orfani, puo_term, ragione = stato_livecoding()
        # ⛔ Un pid gia' terminato che ricompare NON si ritenta: se il `taskkill` non ha preso,
        # riprovarlo quaranta volte non cambia esito e nasconde il vero motivo dietro un ciclo.
        if any(pid in terminati for pid, _ in orfani):
            puo_term, ragione = False, "gia' terminato una volta, e il lock c'e' ancora"
        esito = decide_ritentativo(testo, stato, puo_term)

        if esito == "ferma-compilazione":
            if stampa:
                coda = [x for x in testo.strip().split("\n") if x.strip()][-6:]
                stampa("   build FALLITO, e non e' contesa del motore:")
                for riga in coda:
                    stampa("     " + riga.strip()[:150])
            return False
        if esito == "termina-orfano":
            if stampa:
                stampa("   lock di Live Coding tenuto da un ORFANO, e %s: lo termino e ritento"
                       " subito (`D-400`)." % ragione)
            for pid, ppid in orfani:
                if stampa:
                    stampa("     taskkill LiveCodingConsole pid %d (il padre %d non esiste)"
                           % (pid, ppid))
                subprocess.run(["taskkill", "/F", "/PID", str(pid)], capture_output=True)
                terminati.add(pid)
            continue            # il lock e' libero: si ritenta senza aspettare la pausa
        if esito == "ferma-orfano":
            if stampa:
                stampa("   build FERMO: il lock di Live Coding e' tenuto da un processo ORFANO.")
                for pid, ppid in orfani:
                    stampa("     LiveCodingConsole pid %d — il processo %d che lo ha aperto"
                           " non esiste piu'" % (pid, ppid))
                stampa("   ⛔ Non c'e' un Editor da chiudere, e nessuna attesa lo risolve: il")
                stampa("   messaggio di Unreal manda a cercare una finestra che non esiste.")
                stampa("   ⛔ Non lo termino: %s — `D-400` lo vieta finche' un motore vivo"
                       " potrebbe" % ragione)
                stampa("   star usando Live Coding. Va terminato a mano (`taskkill /PID %d`)"
                       " quando quel" % orfani[0][0])
                stampa("   motore ha finito. Vedi #2392 e #3044.")
            return False
        if stampa and tentativo == 0:
            if esito == "riprova-al-buio":
                stampa("   motore conteso, ma l'enumerazione dei processi NON ha risposto: non")
                stampa("   so se il lock sia di un Editor vivo o di un orfano. Ritento comunque.")
            else:
                stampa("   motore conteso, attendo e ritento (fino a %d volte)" % tentativi)
        time.sleep(pausa)
    return False


def preflight_rapido(stampa):
    """Le precondizioni che costano MILLISECONDI. Torna True se si puo' proseguire.

    🔴 Separate dall'attesa del motore, e l'ordine e' la sostanza: un rifiuto conoscibile
    subito non si fa aspettare un'ora e mezza. Prima si esclude tutto cio' che e' certo e
    istantaneo — percorsi cablati, interprete, e le guardie proprie di ciascun gate — poi si
    entra nell'attesa lunga, che ha senso solo se tutto il resto e' a posto.
    """
    for etichetta, percorso in (("motore", ENGINE_CMD), ("Build.bat", BUILD_BAT)):
        if not os.path.exists(percorso):
            stampa("\n⛔ FERMO: %s non esiste al percorso cablato:\n   %s\n"
                   "   Correggere la costante in `tools/mutation/misura.py`."
                   % (etichetta, percorso))
            return False

    prova = subprocess.run([PWSH, "-NoProfile", "-Command", "exit 0"],
                           capture_output=True, text=True, errors="replace")
    if prova.returncode != 0:
        stampa("\n⛔ FERMO: `%s` non e' eseguibile, e `build()` invoca `Build.bat` da li'.\n"
               "   Installare PowerShell 7, oppure cambiare PWSH dopo aver rimisurato che il\n"
               "   quoting di `build()` regga sull'interprete scelto." % PWSH)
        return False

    return True


def attesa_motore(stampa, attesa_minuti=90):
    """L'attesa lunga, separata da `preflight_rapido` perche' costa fino a un'ora e mezza.

    🔑 Si ATTENDE, non si rifiuta: su questa macchina «motore occupato» e' la condizione
    normale, e uscire subito butta via la preparazione gia' fatta.

    🔴 Va chiamata per ULTIMA, dopo ogni guardia istantanea. Bloccare novanta minuti per poi
    rifiutare a causa di un file sporco — che si sapeva in partenza — non misura niente e
    costa un'ora e mezza a chi guarda.
    """
    if not attendi_motore_libero(attesa_minuti, stampa=stampa):
        stampa("\n⛔ FERMO: il motore non si e' liberato entro %d minuti, oppure l'enumerazione\n"
               "   dei processi e' fallita — che non e' «nessun processo».\n"
               "   Una misura presa sopra un'altra non e' una misura." % attesa_minuti)
        return False
    return True


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
    # 🔴 La tolleranza copre UNA conclusione, non un numero qualsiasi: `rt-suite.ps1` era
    # piu' largo del proprio motivo e lasciava passare questi due.
    caso("5 partiti e 0 conclusi e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=5, avviati=5, completati=0))
    caso("10 partiti e 2 conclusi e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=10, avviati=10, completati=2))
    caso("2 partiti e 1 concluso resta VALIDA: manca UNA conclusione", "VALIDA",
         _A, _A, _log(trovati=2, avviati=2, completati=1))
    caso("assertion fallita e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=4, avviati=4, completati=4,
                      coda="LogCore: Error: Assertion failed: Check(bValid) [Line: 12]"))
    # ⚠️ AVVISO, non verdetto: la stringa non e' ancora confermata su un log prodotto da
    # `esegui_suite()`, e bloccare su di essa fermerebbe i gate sul nascere (`#2672`).
    caso("senza il terminatore il verdetto NON cambia", "VALIDA",
         _A, _A, _log(trovati=4, avviati=4, completati=4, terminatore=False))
    _, _, _, _, avvisi = verdetto(_A, _A, _log(trovati=4, avviati=4, completati=4,
                                               terminatore=False))
    casi.append(("...ma l'assenza del terminatore e' segnalata",
                 any(p.startswith("avviso") for p in avvisi), str(avvisi)))
    caso("albero cambiato e' NON VALIDA", "NON VALIDA",
         _A, _B, _log(trovati=2, avviati=2, completati=2))

    # 🔑 **Il quarto termine dell'invariante**, che fino a `#2672` non era osservato: un
    # motore estraneo durante la run rende la misura non registrabile anche se il log e'
    # perfetto e l'albero non si e' mosso. Questi due casi CADONO sulla regola precedente,
    # che ignorava l'argomento — e' la ragione per cui sono qui.
    caso("un motore estraneo durante la run e' NON VALIDA", "NON VALIDA",
         _A, _A, _log(trovati=2, avviati=2, completati=2), estranei=(4242,))
    caso("nessun estraneo: la stessa run e' VALIDA", "VALIDA",
         _A, _A, _log(trovati=2, avviati=2, completati=2), estranei=())
    # ⛔ **L'uscita `-1` e' una suite CON DEI ROSSI, non un crash**: 22 run su 67 nei log di
    # questa macchina. Farne un `NON VALIDA` invertiva i due gate — la mutazione che fa cadere
    # il bersaglio, cioe' il loro successo, sarebbe finita fra le sopravvissute. La DoD della
    # v0.1 lo dichiara: «un gate automatico non puo' leggere il verdetto dal codice di uscita».
    caso("uscita -1 con dei rossi resta VALIDA: e' il caso che i gate cercano", "VALIDA",
         _A, _A, _log(trovati=2, avviati=2, completati=2, rossi=("b",)), uscita_motore=-1)
    caso("uscita 3 su una run sana non cambia il verdetto", "VALIDA",
         _A, _A, _log(trovati=2, avviati=2, completati=2), uscita_motore=3)
    caso("l'editor morto in avvio resta NON AVVIATA, non NON VALIDA", "NON AVVIATA",
         _A, _A, "LogInit: avvio", uscita_motore=-1)
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

    # --- #2392: chi tiene il lock di Live Coding, e se lo si deve aspettare --------------------
    # 🔑 **Il caso che porta il peso e' l'ULTIMO**: con lo stato `assente` — cio' che il filtro
    # `"UnrealEditor" in nome` produceva su un orfano — la decisione torna `riprova`, cioe' mezz'ora
    # di ritentativi su un lock che nessuno rilascera'. Se rispondesse `ferma-orfano` in entrambi i
    # casi, la distinzione non porterebbe niente e questi test sarebbero vacui.
    LC = "Unable to build while Live Coding is active"

    def livecoding(nome, atteso, processi, vivi):
        stato, orfani = classifica_livecoding(processi, vivi)
        casi.append((nome, stato == atteso,
                     "atteso %s, ottenuto %s (orfani: %s)" % (atteso, stato, orfani)))

    livecoding("LiveCoding col padre morto e' ORFANO", "orfano", {9: 7}, {9})
    livecoding("LiveCoding col padre vivo e' ATTIVO", "attivo", {9: 7}, {7, 9})
    livecoding("nessun LiveCoding e' ASSENTE", "assente", {}, {7, 9})
    livecoding("enumerazione fallita e' SCONOSCIUTO, non assente", "sconosciuto", None, set())
    # Due cloni: uno ha chiuso l'Editor, l'altro no. Il lock resta, quindi vince l'orfano.
    livecoding("un orfano accanto a un attivo vince", "orfano", {9: 7, 11: 10}, {9, 10, 11})

    _, orfani = classifica_livecoding({9: 7, 11: 10}, {9, 10, 11})
    casi.append(("l'orfano e' NOMINATO col suo pid e col padre che non c'e'",
                 orfani == [(9, 7)], str(orfani)))

    def decide(nome, atteso, testo, stato, puo_terminare=False):
        v = decide_ritentativo(testo, stato, puo_terminare)
        casi.append((nome, v == atteso, "atteso %s, ottenuto %s" % (atteso, v)))

    decide("Live Coding tenuto da un orfano: NON si ritenta", "ferma-orfano", LC, "orfano")
    decide("Live Coding tenuto da un Editor vivo: si attende, come prima", "riprova", LC, "attivo")
    decide("un errore di compilazione esce subito, come prima", "ferma-compilazione",
           "error C2065: 'bKnowledgeDebug' non dichiarato", "assente")
    decide("il mutex senza Live Coding si ritenta, come prima", "riprova",
           "waiting for another instance", "assente")
    # ⚠️ `sconosciuto` non e' `assente`: si ritenta lo stesso — un'enumerazione fallita non dice che
    # c'e' un orfano — ma l'esito e' distinto perche' `build()` lo deve DIRE invece di tacerlo.
    decide("enumerazione fallita: si ritenta, ma non in silenzio", "riprova-al-buio", LC,
           "sconosciuto")
    decide("col filtro vecchio lo stato era `assente` e si ritentava: e' il difetto di #2392",
           "riprova", LC, "assente")

    # 🔴 **E la RACCOLTA va esercitata, non solo la classificazione.** `classifica_livecoding`
    # riceve il dict gia' costruito: togliere il filtro `"LiveCoding" in nome` da `_parse_processi`
    # non farebbe cadere nessuno dei casi qui sopra — verificherebbero il proprio scaffolding.
    # Questa riga e' l'unica che lega la classificazione a CIO' CHE LEGGE la macchina.
    righe = ["7;496;UnrealEditor.exe", "9;7;LiveCodingConsole.exe", "13;1;explorer.exe"]
    letto = _parse_processi(righe)
    casi.append(("la raccolta vede il LiveCodingConsole accanto al motore",
                 letto["livecoding"] == {9: 7} and letto["motori"] == {7}
                 and set(letto["padre"]) == {7, 9, 13}, str(letto)))
    casi.append(("una riga malformata non entra e non solleva",
                 _parse_processi(["x;y;z", "", "9;7"])["livecoding"] == {}, "ok"))

    # --- #3044 / `D-400`: si termina un orfano solo se nessun motore vivo puo' usare Live Coding --
    # 🔑 **Il caso che porta il peso e' `-NoLiveCoding`**: il 2026-09-05 era vivo esattamente
    # un motore, un `UnrealEditor-Cmd` di automation lanciato con quel flag. Se lo trattassimo come un
    # utente di Live Coding, `D-400` non risolverebbe il caso che l'ha motivata — e questi test
    # passerebbero comunque, perche' nessun altro caso distingue le due letture.
    def terminabile(nome, atteso, motori):
        ok, ragione = puo_terminare_orfano(motori)
        casi.append((nome, ok == atteso,
                     "atteso %s, ottenuto %s (%s)" % (atteso, ok, ragione)))

    terminabile("nessun motore vivo: l'orfano e' detrito, si termina", True, {})
    terminabile("una suite con -NoLiveCoding non lo usa: si termina", True,
                {4242: ("UnrealEditor-Cmd.exe",
                        "UnrealEditor-Cmd.exe X.uproject -unattended -NoLiveCoding -log=s.log")})
    terminabile("un Editor interattivo vivo: NON si termina", False,
                {7: ("UnrealEditor.exe", "UnrealEditor.exe X.uproject")})
    terminabile("una suite SENZA -NoLiveCoding: non si esclude, NON si termina", False,
                {4242: ("UnrealEditor-Cmd.exe", "UnrealEditor-Cmd.exe X.uproject -log=s.log")})
    terminabile("CommandLine illeggibile: fail-closed, NON si termina", False,
                {4242: ("UnrealEditor-Cmd.exe", None)})
    terminabile("un Editor interattivo accanto a una suite esente: NON si termina", False,
                {7: ("UnrealEditor.exe", "UnrealEditor.exe X.uproject"),
                 4242: ("UnrealEditor-Cmd.exe", "UnrealEditor-Cmd.exe X.uproject -NoLiveCoding")})

    decide("orfano e nessuno che possa usarlo: si TERMINA", "termina-orfano", LC, "orfano", True)
    decide("orfano ma un motore che puo' usarlo: si esce, non si termina", "ferma-orfano", LC,
           "orfano", False)
    decide("`puo_terminare` non cambia nulla quando l'Editor e' VIVO", "riprova", LC, "attivo", True)

    # La raccolta porta nome e CommandLine dei motori, nella stessa passata dei `LiveCoding*`.
    pieno = _parse_processi([
        "7;496;UnrealEditor.exe;UnrealEditor.exe D:/X.uproject",
        "9;7;LiveCodingConsole.exe;LiveCodingConsole.exe",
        "13;1;explorer.exe;C:/Windows/explorer.exe"])
    casi.append(("la raccolta porta nome e CommandLine dei motori",
                 pieno["motori_info"] == {7: ("UnrealEditor.exe",
                                              "UnrealEditor.exe D:/X.uproject")},
                 str(pieno.get("motori_info"))))
    # ⚠️ Una CommandLine contiene `;`: il quarto campo si prende INTERO, non si rispezza.
    conpv = _parse_processi(["7;496;UnrealEditor-Cmd.exe;cmd.exe /c a;b;c -NoLiveCoding"])
    casi.append(("la CommandLine con `;` dentro non viene troncata",
                 conpv["motori_info"][7][1] == "cmd.exe /c a;b;c -NoLiveCoding",
                 str(conpv["motori_info"])))

    # --- #3048: quando la suite e' FINITA, e quando il processo e' APPESO ---------------------
    # 🔑 **Misurato su quattro log veri, due modalita' di invocazione** (`;Quit` dentro
    # `-ExecCmds` per `esegui_suite()`, `+Quit` separato per chi lancia a mano). I due segnali
    # che sembravano ovvi NON reggono:
    #   `**** TEST COMPLETE. EXIT CODE:`  presente nei sani, ASSENTE nei due appesi;
    #   `...Queue Empty N tests performed.`  presente nei due appesi, ASSENTE nei log di
    #   `esegui_suite()` — le due modalita' scrivono righe diverse, e l'oracolo non si trasferisce.
    # Cio' che c'e' in TUTTI E QUATTRO sono i conteggi, ed e' la stessa coppia che `verdetto()`
    # usa per decidere se una suite e' troncata.
    #
    # ⛔ **La soglia e' quella di `verdetto()`, non una piu' stretta**: `completati >= avviati - 1`
    # tollera UNA conclusione mancante, perche' nessuna suite intera arriva con
    # `completati == trovati`. Un oracolo `completati == trovati` direbbe «mai finita» su ogni
    # suite reale — ed e' il caso `1232/1232/1231` qui sotto.

    def finita(nome, atteso, log):
        v = suite_finita(log)[0]
        casi.append((nome, v == atteso, "atteso %s, ottenuto %s (%s)" % (atteso, v,
                                                                        suite_finita(log))))

    # I quattro log veri, coi loro numeri misurati il 2026-09-12.
    finita("appeso 38/38/38 senza terminatore: la suite E' FINITA", True,
           _log(trovati=38, avviati=38, completati=38, terminatore=False))
    finita("appeso 1/1/1 senza terminatore: FINITA", True,
           _log(trovati=1, avviati=1, completati=1, terminatore=False))
    finita("sano 2/2/2 col terminatore: FINITA", True,
           _log(trovati=2, avviati=2, completati=2))
    finita("sano 6/6/6 col terminatore: FINITA", True,
           _log(trovati=6, avviati=6, completati=6))
    # ⚠️ IL CASO DELLA TOLLERANZA: se fosse `completati == trovati`, questo direbbe NON finita
    # e il gate aspetterebbe per sempre ogni suite intera.
    finita("1232/1232/1231 — manca UNA conclusione: FINITA come per `verdetto()`", True,
           _log(trovati=1232, avviati=1232, completati=1231))
    finita("troncata 100 trovati, 60 avviati: NON finita", False,
           _log(trovati=100, avviati=60, completati=60))
    finita("senza `Found N`: non si sa, quindi NON finita", False,
           _log(avviati=3, completati=3))
    finita("log vuoto: NON finita", False, "")

    def attesa(nome, atteso, fin, vivo, grazia):
        v = decide_attesa(fin, vivo, grazia)
        casi.append((nome, v == atteso, "atteso %s, ottenuto %s" % (atteso, v)))

    attesa("log incompleto e processo vivo: si attende", "attendi", False, True, False)
    attesa("processo uscito: si esce dal loop, qualunque sia il log", "uscito", False, False,
           False)
    attesa("log finito, processo vivo, grazia non scaduta: si attende ancora", "attendi",
           True, True, False)
    # 🔑 Il caso che l'issue esiste per coprire: oggi questo aspetta fino al timeout.
    attesa("log finito, processo vivo, grazia scaduta: APPESO", "appeso", True, True, True)

    # ⛔ **Questi sei casi esistono perche' una mutazione e' SOPRAVVISSUTA.** Il calcolo della
    # grazia viveva dentro il loop di `esegui_suite()`, e `grazia_scaduta = False` non faceva
    # cadere nulla: i casi di `decide_attesa` ricevono la grazia come INGRESSO, quindi non
    # vedevano chi la calcola. Estratto e misurato, la stessa mutazione ora cade.
    def grazia(nome, atteso, finita_da, trascorso, secondi):
        v = grazia_scaduta(finita_da, trascorso, secondi)
        casi.append((nome, v == atteso, "atteso %s, ottenuto %s" % (atteso, v)))

    grazia("il log non ha ancora dichiarato la fine: non scade", False, None, 9999, 120)
    grazia("dichiarata adesso: non scade", False, 0, 0, 120)
    grazia("un campione prima della soglia: non scade", False, 0, 119, 120)
    grazia("esattamente alla soglia: scade", True, 0, 120, 120)
    grazia("la soglia si misura DA quando il log ha dichiarato, non dall'avvio", False, 60, 179,
           120)
    grazia("...e scade 120 s dopo quel momento", True, 60, 180, 120)

    return casi
