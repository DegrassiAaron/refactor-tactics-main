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

⛔ **Non decide se la misura sia VALIDA.** Quel giudizio e' di `misura.verdetto()`, che confronta le
istantanee dell'albero git prima e dopo e sa dire se il sorgente e' cambiato sotto la run. Qui si
esegue una suite e si riferisce cosa ha detto; per una verifica di mutazione si usano i gate.

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
    _termina_albero,
    decide_attesa,
    grazia_scaduta,
    suite_finita,
)

LOG_PREDEFINITO = os.path.join(RADICE, "Saved", "Logs", "suite-a-mano.log")


def rossi_dal_log(testo):
    """PURA. I nomi dei test falliti, nell'ordine in cui il log li riporta."""
    return re.findall(r"Test Completed\. Result=\{Fail\} Name=\{([^}]+)\}", testo)


def referto(finita, trovati, avviati, completati, rossi, appeso, secondi):
    """PURA. Le righe del referto finale. Separata dall'esecuzione per poterla provare."""
    righe = []
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


def esegui(filtro, log_path, grazia, timeout_minuti, campionamento, verboso):
    """Lancia, attende sul log, termina se appeso. Torna `(codice_uscita, righe_referto)`."""
    if os.path.exists(log_path):
        try:
            os.remove(log_path)  # un log vecchio darebbe i rossi di una run che non e' questa
        except OSError as e:
            return 2, ["log       non rimovibile (%s): un processo lo tiene aperto."
                       % e.__class__.__name__]

    # ⚠️ `;Quit`, mai `+Quit` — vedi il docstring di modulo. Su FILE e non su `PIPE`: senza drenare,
    # `PIPE` va in deadlock appena il buffer di sistema si riempie, e una suite intera emette
    # megabyte. Il log vero e' il file `-abslog=`; qui basta non bloccare il figlio.
    scarto = tempfile.TemporaryFile()
    avvio = subprocess.Popen(
        [ENGINE_CMD, UPROJECT,
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
    righe = referto(finita, trovati, avviati, completati, rossi, appeso, trascorso)
    righe.append("log       %s" % log_path)

    # 🔑 Il codice di uscita porta l'esito dei TEST, non quello del processo — che nel caso appeso
    # non esiste, e che il motore rende `-1` su una suite con dei rossi anche quando ha funzionato.
    codice = 0 if (finita and not rossi) else 1
    return codice, righe


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
