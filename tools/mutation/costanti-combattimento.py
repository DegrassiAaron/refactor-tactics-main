# -*- coding: utf-8 -*-
"""Quali costanti di combattimento si possono cambiare senza che NIENTE diventi rosso.

    Uso:  python tools/mutation/costanti-combattimento.py <esiti.md> [+3|-3] [NomeCostante ...]

Per ognuna: muta -> ricostruisce -> esegue la suite INTERA -> registra QUALI test cadono -> ripristina.
Senza nomi le prende tutte.

## Perche' esiste

`#2105` ha misurato che `DeflectDamageReduction` si poteva cambiare da 20 a 17 e **due soli** test se ne
accorgevano — nessuno dal catalogo, nessuno da un turno vero. Lo scenario che ne porta il nome era verde
per qualunque riduzione >= 17, perche' da [D-224] lo scudo base assorbe interi i punti residui. Il
meccanismo non e' specifico di quella costante: **ogni volta che una riduzione porta il danno sotto i 5
dello scudo, l'esito smette di dipendere dal valore**, e lo scenario resta verde raccontando di misurarlo.

## 🔴 LA DIREZIONE DELLA MUTAZIONE DECIDE LA RISPOSTA

Misurato su `DeflectDamageReduction`, la costante che ha motivato lo strumento:

    20 -> 23  (+3)    9 test se ne accorgono
    20 -> 17  (-3)    2 test se ne accorgevano prima di #2105

**Non e' rumore, e' struttura.** Per una costante di RIDUZIONE, alzarla spinge il danno residuo sotto i 5
dello scudo base — dove ogni valore da' lo stesso esito e la misura diventa muta; abbassarla lo tiene
sopra. Per una costante di BONUS la direzione che nasconde e' l'altra. ⛔ **Un audit in una direzione sola
non risponde alla domanda**: la prima stesura di questo strumento mutava solo di `+3` e avrebbe dichiarato
«ben coperta» proprio la costante il cui buco l'ha fatto nascere.

## Le scelte che rendono la misura onesta

🔴 **La guardia sul build.** Un `Build.bat` fallito lascia il binario VECCHIO: la suite girerebbe sul codice
**non** mutato e riporterebbe «0 rossi», la peggiore risposta possibile perche' indistinguibile da una
lacuna vera. Successo due volte il 2026-09-02 («Unable to build while Live Coding is active», cioe' un
Editor su un altro checkout). Qui si ritenta su QUALUNQUE fallimento — non solo su quella frase, perche'
un Editor aperto produce invece `Result: Failed (OtherCompilationError)` (`#971`).

🔴 **Si RICOSTRUISCE dopo l'attesa, non prima.** La suite puo' restare in attesa del motore fino a 90
minuti, e in quella finestra un'altra sessione puo' riscrivere il DLL condiviso: l'invariante di validita'
copre il binario che cambia DURANTE la run, non uno gia' stantio all'avvio. E' la regola di `AGENTS.md` e
di `CLAUDE.md` §6, e qui vale il doppio.

🔴 **Si verifica che la mutazione sia ATTERRATA.** Una scrittura no-op — spaziatura diversa, letterale che
compare prima in un commento — farebbe scattare l'allarme piu' forte dello strumento (*«nessun test se ne
accorge»*) su un binario non mutato.

🔴 **Baseline prima del ciclo.** Senza, un rosso preesistente o un test instabile verrebbe attribuito alla
mutazione. Si sottrae, e se la baseline non e' pulita non si parte.

🔴 **Ripristino a INIZIO ciclo, non solo alla fine**, e `try/finally`. Una corsa interrotta lascia la
mutazione sul disco: senza il ripristino iniziale diventerebbe la nuova base di tutte le misure seguenti.

## ⛔ Il rischio, dichiarato

Modifica un sorgente. ⚠️ **Un'interruzione lascia mutato anche il BINARIO**, che e' la meta' che il
confronto su `HEAD` e albero **non** vede: `git checkout --` rimette a posto l'header e non il DLL. Se lo
strumento non stampa `AUDIT COMPLETO`, **ricostruire prima di qualunque altra misura**.

E mentre gira il motore e' occupato: ogni altra misura in parallelo e' NON VALIDA.

## Costo

Un ciclo = un build completo + una suite intera, con attesa fino a 90 minuti per il motore. Con 11 costanti
e una baseline, **una direzione sono ore, non minuti** — e le direzioni sono due.

## Cosa NON dice

- Nulla sulle costanti fuori da `RTCombatLibrary.h` (sono decine, in una ventina di file: il numero si
  conta, non si cita a memoria).
- Nulla sulla QUALITA' della copertura: 19 test che cadono dicono che la costante e' intrecciata ovunque,
  non che sia ben misurata.
- Nulla sui valori che non sono `constexpr int32` — cataloghi, `Parameters`, dati degli eroi.
- ⚠️ E `Scenario.EveryShippedScenarioRuns` compariva in **8** righe su 11: e' il singolo test che regge piu'
  costanti dell'intero perimetro. Un suo indebolimento non toglierebbe una copertura, ne toglierebbe otto.
"""
import io, os, re, subprocess, sys, time

# Stessa sede della regola di validita' dell'altro gate: `regola.verdetto()` e' pura, e i due
# strumenti la CHIAMANO invece di tenerne una copia a testa. Le due copie precedenti erano gia'
# divergenti — arita' diversa e interprete diverso — dopo un solo commit di vita.
import misura as regola   # 'misura' e' gia' il nome di una funzione, qui sotto

# La radice si deriva dal file, non si scrive: su questa macchina esistono tre copie del repository, e una
# costante scritta a mano muterebbe l'albero di qualcun altro lasciando pulito il proprio.
RADICE = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
H = os.path.join(RADICE, "Source", "RefactorTactics", "Combat", "RTCombatLibrary.h")
LOG = os.path.join(RADICE, "Saved", "Logs", "automation-mutazione.log")
UPROJECT = os.path.join(RADICE, "RefactorTactics.uproject")
ENGINE_CMD = r"D:\EpicGames\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
BUILD_BAT = r"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat"
DLL_GLOB = os.path.join(RADICE, "Binaries", "Win64", "UnrealEditor-RefactorTactics*.dll")

# 🔴 `pwsh`, non `powershell`: e' l'interprete su cui il quoting di `build()` e' stato misurato.
# Questo file usava `powershell` (5.1) con lo STESSO quoting che l'altro gate dichiara provato
# solo sotto `pwsh` — e qui non c'era preflight: una differenza di parsing non da' `Result:
# Succeeded`, che `build()` legge come motore occupato e ritenta 40 volte a 45 s, mezz'ora.
PWSH = "pwsh"

# Il self-test non tocca ne' sorgenti ne' motore: deve poter girare senza argomenti, e PRIMA
# del blocco che li pretende. Un test di una funzione pura non si fa cadere da un'installazione.
if "--self-test" in sys.argv:
    _casi = regola.self_test()
    for _nome, _ok, _dett in _casi:
        print("  %s %s%s" % ("ok  " if _ok else "FAIL", _nome, "" if _ok else "   -> " + _dett))
    _falliti = [c for c in _casi if not c[1]]
    print("\nself-test: %d/%d" % (len(_casi) - len(_falliti), len(_casi)))
    sys.exit(1 if _falliti else 0)

if len(sys.argv) < 2:
    print(__doc__.split("\n\n")[1].strip())
    sys.exit(2)
ESITI = sys.argv[1]
resto = sys.argv[2:]
DELTA = 3
if resto and re.fullmatch(r"[+-]\d+", resto[0]):
    DELTA = int(resto[0]); resto = resto[1:]

os.chdir(RADICE)


def scrivi(dati):
    """Scrittura ATOMICA, e in BYTE.

    ⚠️ In byte perche' il ripristino deve rimettere il file **identico**: leggendo in testo e riscrivendo,
    i CRLF diventano LF e `git status` mostra il file modificato dopo un audit che non ha cambiato niente
    — misurato, e basta a rendere NON VALIDA la misura di chiunque altro.

    🔴 E atomica perche' un `open(...,'wb')` TRONCA prima di valutare cosa scrivere: un errore in quella
    finestra lascerebbe l'header del combattimento a zero byte, e la cosa dopo e' un build."""
    tmp = H + ".tmp"
    with io.open(tmp, "wb") as f:
        f.write(dati)
    os.replace(tmp, H)


# 🔴 **Il preflight viene PRIMA del `git checkout --` qui sotto**, e l'ordine e' la sostanza:
# quel comando cancella il non committato (`#2406`). Un audit che si ferma perche' il motore e'
# occupato, o perche' un percorso e' sbagliato, avrebbe gia' distrutto le modifiche locali
# all'header — cioe' avrebbe fatto danno proprio nel caso in cui ha deciso di non misurare.
for _etichetta, _percorso in (("motore", ENGINE_CMD), ("Build.bat", BUILD_BAT)):
    if not os.path.exists(_percorso):
        print("\n⛔ FERMO: %s non esiste al percorso cablato:\n   %s\n"
              "   Correggere la costante in testa a questo file." % (_etichetta, _percorso))
        sys.exit(2)

# ⚠️ PowerShell 7 e' un'installazione a parte: `powershell` (5.1) c'e' sempre, `pwsh` no. Senza
# questa prova, `build()` morirebbe di `FileNotFoundError` a meta' audit — dopo il checkout.
_prova = subprocess.run([PWSH, "-NoProfile", "-Command", "exit 0"],
                        capture_output=True, text=True, errors="replace")
if _prova.returncode != 0:
    print("\n⛔ FERMO: `%s` non e' eseguibile, e `build()` invoca `Build.bat` da li'.\n"
          "   Installare PowerShell 7, oppure cambiare PWSH dopo aver rimisurato che il\n"
          "   quoting di `build()` regga sull'interprete scelto." % PWSH)
    sys.exit(2)

# 🔴 Il motore e' uno per macchina: non si lancia una suite sopra una che gira gia'. ⚠️ E' una
# precondizione, non una guardia — cio' che parte DOPO si rileva solo se tocca i binari. Il lease
# che lo impediva davvero e' stato rimosso (`D-347`), e questa riga non lo rimpiazza (`#2672`).
_vivi = regola.motori_vivi()
if _vivi != 0:
    print("\n⛔ FERMO: %s.\n   Una misura presa sopra un'altra non e' una misura."
          % ("enumerazione dei processi fallita — non e' «nessun processo»" if _vivi < 0
             else "%d process%s del motore gia' in esecuzione" % (_vivi, "o" if _vivi == 1 else "i")))
    sys.exit(2)

# 🔑 Ripristino PRIMA di leggere la base: se una corsa precedente e' stata interrotta, l'header sul disco
# porta ancora la sua mutazione, e senza questa riga diventerebbe la base di tutto l'audit.
subprocess.run(["git", "checkout", "--", "Source/RefactorTactics/Combat/RTCombatLibrary.h"],
               capture_output=True, text=True)
BYTE_ORIGINALI = io.open(H, "rb").read()          # i byte veri, per un ripristino identico
ORIGINALE = BYTE_ORIGINALI.decode("utf-8")

TUTTE = re.findall(r"static constexpr int32 (\w+)\s*=\s*(-?\d+)\s*;", ORIGINALE)
COST = [c for c in TUTTE if not resto or c[0] in resto]
ignoti = [n for n in resto if n not in [c[0] for c in TUTTE]]
if ignoti:
    print("NOMI SCONOSCIUTI (nessuna dichiarazione corrisponde): " + ", ".join(ignoti))
    sys.exit(2)
if not COST:
    print("nessuna costante da misurare")
    sys.exit(2)


def ripristina():
    scrivi(BYTE_ORIGINALI)


def build(tentativi=40):
    """Ricostruisce. Torna True solo su `Result: Succeeded`; ritenta su QUALUNQUE fallimento."""
    for i in range(tentativi):
        r = subprocess.run([PWSH, "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command",
                            "& '" + BUILD_BAT + "' RefactorTacticsEditor "
                            r"Win64 Development -Project='" + os.path.join(RADICE, "RefactorTactics.uproject") +
                            r"' -WaitMutex"],
                           capture_output=True, text=True, errors="replace")
        testo = (r.stdout or "") + (r.stderr or "")   # UBT manda alcune righe su stderr
        if "Result: Succeeded" in testo:
            return True
        time.sleep(45)   # e' l'Editor di un altro checkout, o il motore occupato: si aspetta, non si termina
    return False


def suite():
    """Misura. Torna (verdetto, esito, rossi).

    Motore, istantanee e regola stanno in `misura.py`, in una sede sola: erano in copia qui
    e in `caduta-gate.py`, e le due copie avevano gia' iniziato a divergere.

    ⚠️ Il quarto termine dell'invariante — nessun processo estraneo del motore DURANTE la
    run — non e' osservato: si controlla all'avvio (`regola.motori_vivi`), e cio' che parte
    dopo si vede solo se tocca i binari. Vedi `#2672`."""
    verdetto, esito, rossi, _, problemi = regola.esegui_suite(
        RADICE, ENGINE_CMD, UPROJECT, LOG, "RefactorTactics", DLL_GLOB)
    for p in problemi:
        print("   " + p)
    return verdetto, esito, rossi

def misura():
    """Ricostruire DOPO aver ottenuto il motore non si puo': la suite lo occupa da se' appena parte. Si
    ricostruisce prima, e si RICOSTRUISCE ANCORA se la run ha atteso a lungo — la regola di AGENTS.md."""
    if not build():
        return None
    v, e, rossi = suite()
    if v == "NON AVVIATA":                    # non ha nemmeno preso il motore: si riprova, ricostruendo
        if not build():
            return None
        v, e, rossi = suite()
    return v, e, rossi


sospese = []
with io.open(ESITI, "w", encoding="utf-8") as f:
    f.write("# Quali costanti si possono cambiare senza che niente diventi rosso\n\n")
    f.write("Direzione della mutazione: **%+d**.\n\n" % DELTA)
    f.flush()

    # --- baseline: senza, un rosso preesistente verrebbe attribuito alla mutazione ---------------
    ripristina()
    base = misura()
    if base is None or base[0] != "VALIDA":
        f.write("⛔ **BASELINE NON MISURABILE** (%s): l'audit non parte, perche' senza un punto di\n"
                "riferimento ogni rosso verrebbe attribuito alla mutazione.\n"
                % (base[0] if base else "build fallito"))
        print("BASELINE NON MISURABILE")
        sys.exit(1)
    ROSSI_BASE = base[2]
    f.write("Baseline: **%s** — %s%s\n\n" % (
        base[0], base[1],
        ("" if not ROSSI_BASE else " ⚠️ con %d rossi preesistenti, che vengono SOTTRATTI" % len(ROSSI_BASE))))
    f.flush()

    for nome, val in COST:
        nuovo = str(int(val) + DELTA)
        try:
            ripristina()   # a INIZIO ciclo: la mutazione precedente non deve sopravvivere
            atteso = "static constexpr int32 %s = %s;" % (nome, val)
            nuova_riga = "static constexpr int32 %s = %s;" % (nome, nuovo)
            # 🔴 Sostituzione e verifica in BYTE, come la scrittura, e per la stessa ragione. Questo
            # header e' CRLF: una rilettura in modalita' TESTO applica le universal newlines e
            # restituisce \n, che non sara' mai uguale al testo atteso in \r\n. La verifica falliva
            # su OGNI costante, e l'audit dichiarava undici `NON MISURATA` mutando benissimo il
            # codice: la guardia contro la scrittura no-op scattava su se' stessa, e la direzione
            # `-3` non e' mai stata misurabile. Misurato il 2026-09-03: 17603 byte decodificati
            # contro 17276 caratteri riletti, differenza 327, esattamente le righe del file.
            byte_mutati = BYTE_ORIGINALI.replace(atteso.encode("utf-8"), nuova_riga.encode("utf-8"), 1)
            if byte_mutati == BYTE_ORIGINALI:
                # 🔴 Una scrittura no-op farebbe scattare l'allarme piu' forte su codice NON mutato.
                f.write("## %s = %s -> %s\n**NON MISURATA**: la sostituzione non ha agganciato la "
                        "dichiarazione (spaziatura diversa?).\n\n" % (nome, val, nuovo))
                sospese.append(nome); f.flush(); continue
            scrivi(byte_mutati)
            if io.open(H, "rb").read() != byte_mutati:
                f.write("## %s = %s -> %s\n**NON MISURATA**: il file riletto non porta la mutazione.\n\n"
                        % (nome, val, nuovo))
                sospese.append(nome); f.flush(); continue

            m = misura()
            if m is None:
                f.write("## %s = %s -> %s\n**NON MISURATA**: il build non e' riuscito.\n\n" % (nome, val, nuovo))
                sospese.append(nome); f.flush(); continue

            verdetto, esito, rossi = m
            nuovi = sorted(rossi - ROSSI_BASE)
            f.write("## %s = %s -> %s\n- misura: **%s** — %s\n" % (nome, val, nuovo, verdetto, esito))
            if verdetto != "VALIDA":
                f.write("- ⚠️ misura non valida: non conta\n\n")
                sospese.append(nome)
            elif nuovi:
                f.write("- **%d test se ne accorgono**:\n" % len(nuovi))
                for t in nuovi:
                    f.write("  - `%s`\n" % t)
                f.write("\n")
            else:
                f.write("- 🔴 **NESSUN test se ne accorge.**\n\n")
            f.flush()
        finally:
            ripristina()

    if sospese:
        f.write("\n## ⚠️ Costanti senza una misura valida\n\n%s\n" %
                "\n".join("- `%s`" % n for n in sospese))

ripristina()
if not build():
    print("⛔ BINARIO MUTATO SUL DISCO: il ripristino finale non ha ricostruito.\n"
          "   Ricostruire PRIMA di qualunque altra misura — il confronto su `HEAD` e albero\n"
          "   non vede un binario gia' stantio.")
    sys.exit(1)
print("AUDIT COMPLETO" + (" (con %d costanti senza misura valida)" % len(sospese) if sospese else ""))
