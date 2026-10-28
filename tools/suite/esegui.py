#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Esegue una suite Automation e ATTENDE SUL LOG, non sull'uscita del processo.

    python tools/suite/esegui.py RefactorTactics.ScreenHud
    python tools/suite/esegui.py RefactorTactics.HexSim --grazia 180
    python tools/suite/esegui.py RefactorTactics.ScreenHud.PanelsLeaveTheCenterFree -v

Perche' esiste
--------------
`docs/technical/runbooks/test-e-diagnosi.md` §2 prescriveva di lanciare `UnrealEditor-Cmd.exe` a
mano, e chi lo faceva non aveva **niente** quando il processo non usciva: ne' un modo per
accorgersene senza aspettare, ne' un terminatore. Il rimedio esisteva — `misura._termina_albero()`
fa `taskkill /T /F` sull'albero — ma lo chiamava solo `esegui_suite()`, dentro i gate di mutazione,
e solo al timeout ([#3049](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3049)).

🔴 **Non e' un fastidio: e' un difetto di correttezza.** Il 2026-08-27 un processo appeso ha fatto
fallire la `Build.bat` successiva in 1,5 s con *«Live Coding is active»*, e il test che seguiva e'
girato sul binario **precedente** alla mutazione dichiarando `Success` dove la rimisura dava `Fail`.
Sei occorrenze in una seduta. Un motore appeso non fa perdere tempo soltanto: falsa la misura dopo.

Cosa fa, e cosa NON fa
----------------------
Fa tre cose che l'invocazione a mano non fa:

1. **Usa `;Quit`, mai `+Quit`.** `+` separa i **filtri** dentro `RunTests`, non i comandi (misurato:
   `RunTests A+B` rende `Found 2`). Con `+Quit` la parola diventa un secondo filtro che non
   corrisponde a nessun test, il comando non viene **mai** eseguito, e il processo non esce. I test
   girano lo stesso, quindi niente lo segnala.
2. **Attende sul LOG.** La fine si legge dai conteggi — `suite_finita()`, `#3048` — non dall'uscita
   del processo, che nel caso appeso non arriva mai.
3. **Termina l'albero** quando il log dichiara finito e il processo resta vivo oltre la grazia, e lo
   **dichiara nel referto** invece di nasconderlo.

🔑 **Non blocca la build dell'Editor di nessuno, e per due ragioni** ([#3522](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3522)).
In UE 5.8.1:

- **non avvia il Live Coding.** Si avvia da solo soltanto in un processo che **non** e' `-unattended`
  (`FLiveCodingModule::StartupModule` chiede `FApp::IsUnattended()`), salvo `-LiveCoding` esplicito, e
  la suite lo e'. Misurato il 2026-10-07: nei log scritti dal 2026-09-20 nessuna run `-unattended`
  contiene «Starting LiveCoding»;
- **e anche se lo avviasse, il suo lock non e' quello che UBT controlla.** Il mutex prende il nome
  dall'eseguibile del processo, qui `UnrealEditor-Cmd.exe` (`FLiveCodingModule::StartLiveCoding`);
  per il target Editor UBT interroga quello di `UnrealEditor.exe` (`HotReload.IsLiveCodingSessionActive`).

⌫ **`-NoLiveCoding` non c'e' piu'** ([`D-469`], [#3536](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3536)).
E' un'opzione di **UBT** (`TargetRules.bWithLiveCoding`: compila il target senza Live Coding), e l'Editor
non la legge: gli Editor interattivi aperti con quel flag hanno scritto «Starting LiveCoding» lo stesso.
Stava nella riga perche' i gate di `tools/mutation/` la cercavano per decidere se terminare una console
orfana ([`D-400`]). `D-469` ha superato quella voce — una console orfana non blocca le build, e nessun
gate la termina —, e il flag ha perso il suo ultimo lettore.
Per spegnere il Live Coding in un processo che non e' `-unattended`, la riga che il motore legge e'
`-LiveCoding=false`: misurato su tre avvii di `UnrealEditor-Cmd` senza `-unattended`. Senza flag e con
`-NoLiveCoding` il log dice «Starting LiveCoding»; con `-LiveCoding=false` non c'e' nessuna riga
`LogLiveCoding`.

⛔ **Non decide se la misura sia VALIDA.** Quel giudizio e' di `misura.verdetto()`, che confronta le
istantanee dell'albero git prima e dopo e sa dire se il sorgente e' cambiato sotto la run. Qui si
esegue una suite e si riferisce cosa ha detto; per una verifica di mutazione si usano i gate.

🔴 **Ma non tace su un verde che non e' dell'albero** ([#2656](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2656)).
Se i DLL sono piu' vecchi dei sorgenti sul disco (`misura.binari_stantii()`), il referto lo dice in CIMA e
l'uscita non e' `0`. Non blocca: una run su una build vecchia puo' essere voluta, ma non deve sembrare una
misura dell'albero.

⚠️ **Non copre il processo appeso di QUALCUN ALTRO.** `_termina_albero()` vuole il `Popen` del
proprio figlio, e uccidere per pid un motore altrui e' esattamente il fail-open che `#2392` ha
chiuso. Se un motore che non hai lanciato tu tiene il mutex, si legge la sua `CommandLine` e si
decide — non si termina alla cieca.
"""
import argparse
import io
import os
import re
import subprocess
import sys
import tempfile
import time

# Le funzioni che DECIDONO vivono in `misura.py` e sono pure. Sono importate, non ricopiate: una
# seconda implementazione dell'oracolo divergerebbe dalla prima, ed e' proprio il difetto che
# `#3048` ha tolto misurando che i segnali «ovvi» sono disgiunti fra le due modalita' di
# invocazione. Qui resta l'orchestrazione, che e' genuinamente diversa — niente istantanee git,
# niente campionamento dei processi estranei.
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "mutation"))
from misura import (  # noqa: E402
    ENGINE_CMD,
    RADICE,
    UPROJECT,
    _radice_finta,
    _rimuovi,
    _termina_albero,
    binari_stantii,
    decide_attesa,
    descrivi_stantii,
    grazia_scaduta,
    leggi_sorgenti_e_binari,
    suite_finita,
)

LOG_PREDEFINITO = os.path.join(RADICE, "Saved", "Logs", "suite-a-mano.log")


def rossi_dal_log(testo):
    """PURA. I nomi dei test falliti, nell'ordine in cui il log li riporta."""
    return re.findall(r"Test Completed\. Result=\{Fail\} Name=\{([^}]+)\}", testo)


def referto(finita, trovati, avviati, completati, rossi, appeso, secondi, stantii=()):
    """PURA. Le righe del referto finale. Separata dall'esecuzione per poterla provare.

    `stantii`: i moduli il cui DLL non contiene i sorgenti (`misura.binari_stantii()`, #2656). L'avviso
    sta in CIMA: chi legge `rossi nessuno` e smette di leggere non deve perderlo."""
    righe = list(descrivi_stantii(stantii))
    if stantii:
        righe.append("          La suite e' girata su una build PRECEDENTE all'albero: l'esito riguarda"
                     " quella build, non l'albero.")
    stato = "APPESO" if appeso else ("finita" if finita else "INTERROTTA")
    righe.append("suite     %s in %d s" % (stato, secondi))
    righe.append("conteggi  trovati %s, avviati %d, completati %d"
                 % ("?" if trovati is None else trovati, avviati, completati))
    if rossi:
        righe.append("rossi     %d" % len(rossi))
        righe.extend("  %s" % r for r in rossi)
    else:
        righe.append("rossi     nessuno")
    if appeso:
        righe.append("motore    NON e' uscito da se': albero terminato. La misura sopra resta")
        righe.append("          VALIDA - il log aveva gia' i conteggi completi. Vedi #3049.")
    if not finita:
        righe.append("ATTENZIONE il log NON dichiara la suite finita: l'esito sopra e' parziale.")
    return righe


def codice_di_uscita(finita, rossi, stantii=(), controllo_ok=True):
    """PURA. `0` solo se la suite e' finita, senza rossi, su un binario che contiene i sorgenti.

    🔑 Porta l'esito dei TEST, non quello del processo — che nel caso appeso non esiste, e che il motore
    rende `-1` su una suite con dei rossi anche quando ha funzionato. 🔴 E un verde su un binario
    stantio (#2656) non e' un verde: riguarda una build che non e' quella dell'albero. Lo stesso se il
    controllo NON ha potuto girare (`controllo_ok=False`): «non so se il binario e' quello giusto» non
    e' «lo e'», e un `0` direbbe l'opposto di cio' che il referto scrive."""
    return 0 if (finita and not rossi and not stantii and controllo_ok) else 1


def esegui(filtro, log_path, grazia, timeout_minuti, campionamento, verboso,
           radice=RADICE, motore=ENGINE_CMD, uproject=UPROJECT):
    """Lancia, attende sul log, termina se appeso. Torna `(codice_uscita, righe_referto)`.

    `radice`, `motore` e `uproject` sono iniettabili solo perche' il cablaggio si possa provare con un
    motore finto (vedi `self_test()`): chi lo usa davvero non li passa."""
    # 🔴 #2656: i binari devono contenere i sorgenti. Non blocca: una run a mano su una build vecchia
    # puo' essere voluta (una baseline), ma il referto lo dice in cima e l'uscita non e' `0`. Se i
    # sorgenti non si leggono lo si dice, invece di tacere, e anche allora l'uscita non e' `0`:
    # l'assenza di un avviso non e' un binario sano.
    stantii = []
    nota_binari = None
    try:
        stantii = binari_stantii(*leggi_sorgenti_e_binari(radice))
    except OSError as e:
        nota_binari = ("build     controllo dei binari NON eseguito (%s): l'esito non dice se i binari"
                       " contengono i sorgenti, e l'uscita non e' `0`." % e.__class__.__name__)
    if stantii and verboso:
        print("  i binari non contengono i sorgenti: il referto lo dira' in cima")

    if os.path.exists(log_path):
        try:
            os.remove(log_path)  # un log vecchio darebbe i rossi di una run che non e' questa
        except OSError as e:
            return 2, ["log       non rimovibile (%s): un processo lo tiene aperto."
                       % e.__class__.__name__]

    # ⚠️ `;Quit`, mai `+Quit` — vedi il docstring di modulo. Su FILE e non su `PIPE`: senza drenare,
    # `PIPE` va in deadlock appena il buffer di sistema si riempie, e una suite intera emette
    # megabyte. Il log vero e' il file `-abslog=`; qui basta non bloccare il figlio.
    # 🔑 `-unattended` e' anche cio' che tiene spento il Live Coding (#3522). ⌫ `-NoLiveCoding` non c'e'
    # piu': il motore non lo legge, e i gate che lo cercavano non esistono dopo [D-469] (docstring di modulo).
    scarto = tempfile.TemporaryFile()
    avvio = subprocess.Popen(
        [motore, uproject,
         "-ExecCmds=Automation RunTests " + filtro + ";Quit",
         "-unattended", "-nopause", "-nosplash", "-nullrhi", "-NoSound",
         "-abslog=" + log_path],
        stdout=scarto, stderr=subprocess.STDOUT)

    trascorso = 0
    finita_da = None
    appeso = False

    while True:
        vivo = avvio.poll() is None
        log_finito = False
        if os.path.exists(log_path):
            try:
                log_finito = suite_finita(
                    io.open(log_path, encoding="utf-8", errors="replace").read())[0]
            except OSError:
                log_finito = False  # il motore lo tiene aperto: si riprova al campione dopo
        if log_finito and finita_da is None:
            finita_da = trascorso
            if verboso:
                print("  [%4d s] il log dichiara la suite finita" % trascorso)

        stato = decide_attesa(log_finito, vivo, grazia_scaduta(finita_da, trascorso, grazia))
        if stato == "uscito":
            break
        if stato == "appeso":
            if verboso:
                print("  [%4d s] processo vivo oltre la grazia: termino l'albero" % trascorso)
            _termina_albero(avvio)
            appeso = True
            break
        if trascorso >= timeout_minuti * 60:
            _termina_albero(avvio)
            scarto.close()
            return 2, ["motore    nessuna risposta dopo %d minuti, e il log non dichiara la suite"
                       " finita: albero terminato." % timeout_minuti]

        time.sleep(campionamento)
        trascorso += campionamento
        if verboso and trascorso % 60 == 0:
            print("  [%4d s] in corso" % trascorso)

    if not appeso:
        avvio.wait()
    scarto.close()

    testo = ""
    if os.path.exists(log_path):
        testo = io.open(log_path, encoding="utf-8", errors="replace").read()

    finita, trovati, avviati, completati = suite_finita(testo)
    rossi = rossi_dal_log(testo)
    righe = referto(finita, trovati, avviati, completati, rossi, appeso, trascorso, stantii)
    if nota_binari:
        righe.insert(0, nota_binari)
    righe.append("log       %s" % log_path)

    return codice_di_uscita(finita, rossi, stantii, nota_binari is None), righe


def self_test():
    """Prova le due funzioni PURE. Torna `(passati, totale, righe)`.

    🔴 **Il caso che conta e' `rossi-formato-cambiato`.** Se la riga del log cambia forma,
    `rossi_dal_log` rende `[]` **in silenzio**, il referto dice «rossi nessuno» e chi legge crede a
    un verde su una suite che aveva dei rossi. E' lo stesso modo di guasto che questo strumento
    esiste per togliere — un segnale assente letto come segnale buono — e senza questo caso
    rientrerebbe dalla finestra.
    """
    casi = []

    def caso(nome, condizione, dettaglio=""):
        casi.append((nome, bool(condizione), dettaglio))

    riga_rossa = ("LogAutomationController: Error: Test Completed. Result={Fail} "
                  "Name={Tizio} Path={RefactorTactics.Tizio}")
    riga_verde = "LogAutomationController: Display: Test Completed. Result={Success} Name={Caio}"

    caso("rossi: trova un fallimento", rossi_dal_log(riga_rossa) == ["Tizio"],
         str(rossi_dal_log(riga_rossa)))
    caso("rossi: un log senza fallimenti ne rende zero", rossi_dal_log(riga_verde) == [])
    caso("rossi: ne trova due su due righe",
         len(rossi_dal_log(riga_rossa + "\n" + riga_rossa.replace("Tizio}", "Sempronio}"))) == 2)
    # Se questo passasse, la regex starebbe leggendo qualcosa di piu' lasco del formato reale.
    caso("rossi-formato-cambiato: una riga simile ma diversa NON conta come rosso",
         rossi_dal_log("Test Completed - Result: Fail - Name: Tizio") == [])

    r_appeso = referto(True, 3, 3, 3, [], True, 42)
    caso("referto: dichiara APPESO", any("APPESO" in x for x in r_appeso))
    caso("referto: su appeso dice che la misura resta valida",
         any("VALIDA" in x for x in r_appeso))

    r_parziale = referto(False, 100, 40, 39, [], False, 10)
    caso("referto: su suite non finita avverte che l'esito e' parziale",
         any("parziale" in x for x in r_parziale))
    caso("referto: una suite finita e pulita non avverte di niente",
         not any("parziale" in x for x in referto(True, 3, 3, 3, [], False, 5)))

    r_rossi = referto(True, 3, 3, 3, ["Tizio", "Caio"], False, 5)
    caso("referto: elenca i rossi", any("Tizio" in x for x in r_rossi))

    # --- #2656: un verde su un binario che non contiene i sorgenti non e' un verde --------------------
    # 🔴 Il referto sopra dice «rossi nessuno» anche quando la build e' fallita e la suite ha girato sul
    # binario di un'ora fa. L'avviso sta in CIMA: chi legge `rossi nessuno` e smette di leggere non deve
    # perderlo, e un codice di uscita `0` non deve accompagnarlo.
    stantii = [("A", "Source/A/a.cpp", 2000 * 10 ** 9, 1000 * 10 ** 9)]
    r_stantii = referto(True, 3, 3, 3, [], False, 5, stantii)
    caso("referto: binari stantii avvertono, in CIMA, che l'esito riguarda una build PRECEDENTE",
         bool(r_stantii) and r_stantii[0].startswith("build") and "non contengono" in r_stantii[0],
         str(r_stantii[:2]))
    # Niente avviso di NESSUN genere: non solo le righe `build`, anche la riga di contesto, che e' scritta
    # in modo diverso e che un `if` reso incondizionato farebbe comparire su un referto pulito.
    r_freschi = referto(True, 3, 3, 3, [], False, 5, [])
    caso("referto: binari aggiornati non avvertono di niente",
         not any(x.startswith("build") or "PRECEDENTE" in x for x in r_freschi), str(r_freschi))
    caso("uscita: suite finita e pulita su binari STANTII non e' 0",
         codice_di_uscita(True, [], stantii) == 1)
    caso("uscita: suite finita e pulita su binari aggiornati e' 0",
         codice_di_uscita(True, [], []) == 0)
    caso("uscita: i rossi restano 1", codice_di_uscita(True, ["Tizio"], []) == 1)
    caso("uscita: una suite non finita resta 1", codice_di_uscita(False, [], []) == 1)
    caso("uscita: se il controllo NON ha potuto girare, non e' un verde",
         codice_di_uscita(True, [], [], False) == 1)
    caso("referto: l'avviso dice che la suite e' GIA' girata, non che girerebbe",
         any("e' girata" in x for x in r_stantii) and not any("girerebbe" in x for x in r_stantii),
         str(r_stantii))

    # 🔴 **Il cablaggio di `esegui()`, non solo le funzioni pure.** Le righe sopra restano verdi anche
    # se `esegui()` non chiama mai il controllo, o lo chiama e ne scarta l'esito: e' il pezzo che le
    # sessioni usano a mano. Il motore e' un processo Python che scrive il log e esce — `Popen` lo
    # lancia come lancerebbe `UnrealEditor-Cmd` — su una radice finta con mtime fissati a mano.
    log_verde = ("LogAutomationController: Found 1 automation tests based on X\n"
                 "Test Started. Name={t} Path={t}\n"
                 "Test Completed. Result={Success} Name={t} Path={t}\n"
                 "**** TEST COMPLETE. EXIT CODE: 0 ****\n")
    log_rosso = log_verde.replace("Result={Success}", "Result={Fail}")
    log_incompleto = ("LogAutomationController: Found 2 automation tests based on X\n"
                      "Test Started. Name={t} Path={t}\n")

    def lancia_finto(sorgenti, dll, log=log_verde):
        """`(codice, righe)` di `esegui()` su una radice e un motore finti, che scrive `log`."""
        radice = _radice_finta(sorgenti, dll)
        try:
            copione = os.path.join(radice, "motore_finto.py")
            with io.open(copione, "w", encoding="utf-8") as fh:
                fh.write("import sys\n"
                         "for a in sys.argv:\n"
                         "    if a.startswith('-abslog='):\n"
                         "        with open(a[len('-abslog='):], 'w') as fh:\n"
                         "            fh.write(%r)\n" % log)
            return esegui("X", os.path.join(radice, "suite.log"), 5, 1, 0.2, False,
                          radice=radice, motore=sys.executable, uproject=copione)
        finally:
            _rimuovi(radice)

    # ⛔ **Una guardia contro il test che lancia Unreal VERO.** Se `esegui()` smettesse di usare il
    # `motore` che riceve e tornasse a `ENGINE_CMD`, questo self-test avvierebbe il motore della macchina
    # — che e' uno, e che le altre sessioni stanno usando per misurare. Qui `ENGINE_CMD` e `UPROJECT`
    # puntano a un percorso che non esiste: la regressione diventa un `FileNotFoundError` rumoroso.
    motore_vero, progetto_vero = globals()["ENGINE_CMD"], globals()["UPROJECT"]
    globals()["ENGINE_CMD"] = globals()["UPROJECT"] = os.path.join(
        tempfile.gettempdir(), "rt-esegui-autotest-NON-il-motore-vero.exe")
    try:
        fresco, stantio = ({"Source/A/a.cpp": 1000}, {"A": 3000}), ({"Source/A/a.cpp": 3000}, {"A": 1000})
        codice, righe = lancia_finto(*fresco)
        caso("esegui: binari che contengono i sorgenti, suite verde: uscita 0 e nessun avviso",
             codice == 0 and not any(x.startswith("build") or "PRECEDENTE" in x for x in righe)
             and any(x.startswith("suite     finita") for x in righe), "%s %s" % (codice, righe))
        codice, righe = lancia_finto(*stantio)
        caso("esegui: binari stantii, suite verde: uscita 1 e l'avviso in CIMA",
             codice == 1 and bool(righe) and righe[0].startswith("build")
             and "non contengono" in righe[0], "%s %s" % (codice, righe[:2]))
        caso("esegui: ...e la suite e' girata lo stesso (non blocca)",
             any(x.startswith("suite     finita") for x in righe), str(righe))
        codice, righe = lancia_finto({}, {})
        caso("esegui: se il controllo NON gira, lo dice in cima e l'uscita non e' 0",
             codice == 1 and bool(righe) and righe[0].startswith("build") and "NON eseguito" in righe[0],
             "%s %s" % (codice, righe[:2]))
        # I binari freschi non bastano a fare un verde: il codice d'uscita resta quello dei TEST. Senza
        # questi due casi un'uscita che ignorasse i rossi, o la suite non finita, passerebbe.
        codice, righe = lancia_finto(*fresco, log=log_rosso)
        caso("esegui: binari freschi ma un test rosso: uscita 1 e il rosso e' contato",
             codice == 1 and any(x.startswith("rossi     1") for x in righe), "%s %s" % (codice, righe))
        codice, righe = lancia_finto(*fresco, log=log_incompleto)
        caso("esegui: binari freschi ma suite non finita: uscita 1 e ATTENZIONE",
             codice == 1 and any(x.startswith("ATTENZIONE") for x in righe), "%s %s" % (codice, righe))
    finally:
        globals()["ENGINE_CMD"], globals()["UPROJECT"] = motore_vero, progetto_vero

    righe = ["  %-62s %s%s" % (n, "ok" if v else "FALLITO", (" (%s)" % d) if d and not v else "")
             for n, v, d in casi]
    return sum(1 for _, v, _ in casi if v), len(casi), righe


def main(argv=None):
    p = argparse.ArgumentParser(
        description="Esegue una suite Automation attendendo sul log, non sul processo.")
    p.add_argument("filtro", nargs="?", help="es. RefactorTactics.ScreenHud")
    p.add_argument("--autotest", action="store_true",
                   help="prova le funzioni pure di questo file e esce")
    p.add_argument("--log", default=LOG_PREDEFINITO,
                   help="dove scrivere il log (predefinito: Saved/Logs/suite-a-mano.log)")
    p.add_argument("--grazia", type=int, default=120,
                   help="secondi di attesa DOPO che il log dichiara finito, prima di terminare "
                        "l'albero (predefinito: 120)")
    p.add_argument("--timeout", type=int, default=180,
                   help="minuti oltre i quali si rinuncia se il log non dichiara finito")
    p.add_argument("--campionamento", type=int, default=10, help="secondi fra due letture del log")
    p.add_argument("-v", "--verboso", action="store_true", help="stampa l'avanzamento")
    a = p.parse_args(argv)

    if a.autotest:
        passati, totale, righe = self_test()
        for r in righe:
            print(r)
        print("autotest  %d/%d" % (passati, totale))
        return 0 if passati == totale else 1

    if not a.filtro:
        p.error("serve un filtro (o --autotest)")

    codice, righe = esegui(a.filtro, a.log, a.grazia, a.timeout, a.campionamento, a.verboso)
    for r in righe:
        print(r)
    return codice


if __name__ == "__main__":
    sys.exit(main())
