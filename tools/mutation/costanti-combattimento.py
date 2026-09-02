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

🔴 **Si RICOSTRUISCE dopo l'attesa, non prima.** `rt-suite` puo' restare in coda fino a 90 minuti, e in
quella finestra un'altra sessione puo' riscrivere il DLL condiviso: l'invariante di `rt-suite` copre il
binario che cambia DURANTE la run, non uno gia' stantio all'avvio. E' la regola di `AGENTS.md` e di
`CLAUDE.md` §6, e qui vale il doppio.

🔴 **Si verifica che la mutazione sia ATTERRATA.** Una scrittura no-op — spaziatura diversa, letterale che
compare prima in un commento — farebbe scattare l'allarme piu' forte dello strumento (*«nessun test se ne
accorge»*) su un binario non mutato.

🔴 **Baseline prima del ciclo.** Senza, un rosso preesistente o un test instabile verrebbe attribuito alla
mutazione. Si sottrae, e se la baseline non e' pulita non si parte.

🔴 **Ripristino a INIZIO ciclo, non solo alla fine**, e `try/finally`. Una corsa interrotta lascia la
mutazione sul disco: senza il ripristino iniziale diventerebbe la nuova base di tutte le misure seguenti.

## ⛔ Il rischio, dichiarato

Modifica un sorgente. ⚠️ **Un'interruzione lascia mutato anche il BINARIO**, che e' la meta' che `rt-suite`
**non** sa vedere: `git checkout --` rimette a posto l'header e non il DLL. Se lo strumento non stampa
`AUDIT COMPLETO`, **ricostruire prima di qualunque altra misura**.

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

# La radice si deriva dal file, non si scrive: su questa macchina esistono tre copie del repository, e una
# costante scritta a mano muterebbe l'albero di qualcun altro lasciando pulito il proprio.
RADICE = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
H = os.path.join(RADICE, "Source", "RefactorTactics", "Combat", "RTCombatLibrary.h")
LOG = os.path.join(RADICE, "Saved", "Logs", "rt-suite.log")

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
        r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command",
                            r"& 'D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat' RefactorTacticsEditor "
                            r"Win64 Development -Project='" + os.path.join(RADICE, "RefactorTactics.uproject") +
                            r"' -WaitMutex"],
                           capture_output=True, text=True, errors="replace")
        testo = (r.stdout or "") + (r.stderr or "")   # UBT manda alcune righe su stderr
        if "Result: Succeeded" in testo:
            return True
        time.sleep(45)   # e' l'Editor di un altro checkout, o il motore occupato: si aspetta, non si termina
    return False


def suite():
    """Coda per il motore, POI ricostruisce, poi misura. Torna (verdetto, esito, rossi)."""
    r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                        "scripts/rt-suite.ps1", "-WaitMinutes", "90"],
                       capture_output=True, text=True, errors="replace")
    out = (r.stdout or "") + (r.stderr or "")
    if "[RT-MEASURE] NON VALIDA" in out:
        verdetto = "NON VALIDA"
    elif "[RT-MEASURE] VALIDA" in out:
        verdetto = "VALIDA"
    elif "NON AVVIATA" in out:
        verdetto = "NON AVVIATA"
    else:
        verdetto = "ALTRO"
    m = re.search(r"esito\s+(\d+)/(\d+) completati, (\d+) fallimenti", out)
    esito = m.group(0) if m else "(nessun esito)"
    rossi = set()
    if os.path.exists(LOG):
        testo = io.open(LOG, encoding="utf-8", errors="replace").read()
        rossi = set(re.findall(r"Result=\{Fail\} Name=\{[^}]*\} Path=\{([^}]+)\}", testo))
    return verdetto, esito, rossi


def misura():
    """Ricostruisce DOPO aver ottenuto il motore non si puo': rt-suite lo prende da se'. Si ricostruisce
    prima, e si RICOSTRUISCE ANCORA se la run e' rimasta in coda a lungo — la regola di AGENTS.md."""
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
            mutato = ORIGINALE.replace(atteso, "static constexpr int32 %s = %s;" % (nome, nuovo), 1)
            if mutato == ORIGINALE:
                # 🔴 Una scrittura no-op farebbe scattare l'allarme piu' forte su codice NON mutato.
                f.write("## %s = %s -> %s\n**NON MISURATA**: la sostituzione non ha agganciato la "
                        "dichiarazione (spaziatura diversa?).\n\n" % (nome, val, nuovo))
                sospese.append(nome); f.flush(); continue
            scrivi(BYTE_ORIGINALI.replace(atteso.encode("utf-8"),
                                          ("static constexpr int32 %s = %s;" % (nome, nuovo)).encode("utf-8"), 1))
            if io.open(H, encoding="utf-8").read() != mutato:
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
          "   Ricostruire PRIMA di qualunque altra misura — `rt-suite` non vede un binario gia' stantio.")
    sys.exit(1)
print("AUDIT COMPLETO" + (" (con %d costanti senza misura valida)" % len(sospese) if sospese else ""))
