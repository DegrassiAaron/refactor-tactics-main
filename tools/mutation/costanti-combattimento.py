# -*- coding: utf-8 -*-
"""Quali costanti di combattimento si possono cambiare senza che NIENTE diventi rosso.

    Uso:  python tools/mutation/costanti-combattimento.py <esiti.md> [NomeCostante ...]

Per ognuna: muta di +3 -> ricostruisce -> esegue la suite INTERA -> registra QUALI test cadono ->
ripristina. Senza nomi le prende tutte.

## Perche' esiste

`#2105` ha misurato che `DeflectDamageReduction` si poteva cambiare da 20 a 17 e **due soli** test se ne
accorgevano — nessuno dal catalogo, nessuno da un turno vero. Lo scenario che ne porta il nome era verde
per qualunque riduzione >= 17, perche' da [D-224] lo scudo base assorbe interi i punti residui. Il
meccanismo non e' specifico di quella costante: **ogni volta che una riduzione porta il danno sotto i 5
dello scudo, l'esito smette di dipendere dal valore**, e lo scenario resta verde raccontando di misurarlo.

`#2118` ha posto la domanda per esteso. Esito del 2026-09-02, su `1817/1817`:

    BaseShield                 5    19 test        BurningCleanupDamage       8     5
    DeflectDamageReduction    20     9             ExposedFirstHitBonus       5     3
    GuardFirstHitReduction    15     7             GuardResistedPushDistance  1     3
    LowCoverDamageReduction   10     3             PropagatedElectricDamage  12     2
    GadgetWetDischargeBonus    8     2             MarkedFirstHitBonus        6     1
    BraceDamageReduction      10     1

**Nessuna scoperta.** ⚠️ Ma la risposta invecchia a ogni test tolto e a ogni costante aggiunta: e' una
misura da rifare, non un verdetto da citare.

## Le due scelte che rendono la misura onesta

🔴 **La guardia sul build.** Un `Build.bat` fallito — tipicamente *«Unable to build while Live Coding is
active»*, cioe' un Editor aperto su un altro checkout — lascia il binario VECCHIO: la suite girerebbe sul
codice **non** mutato e riporterebbe «0 rossi», che e' la peggiore risposta possibile perche' e'
indistinguibile da una lacuna vera. Successo due volte il 2026-09-02. Qui il build si ripete finche' non
riesce, e se non riesce la costante si dichiara `NON MISURATA`.

⚠️ **La mutazione e' `+3` e non `+5`.** Uno scarto multiplo di 5 puo' essere riassorbito dallo scudo base
e produrre lo stesso esito — cioe' un falso «nessuno se ne accorge», che e' esattamente il difetto che
questo strumento esiste per trovare.

## ⛔ Il rischio, dichiarato

Questo strumento **modifica un sorgente** e lo ripristina dopo ogni costante. Un'interruzione a meta' lascia
l'header mutato: `git checkout -- Source/RefactorTactics/Combat/RTCombatLibrary.h` rimette a posto. E
mentre gira il motore e' occupato e l'albero e' sporco, quindi **ogni altra misura avviata in parallelo e'
NON VALIDA**.

## Cosa NON dice

- Nulla sulle **34** `static constexpr int32` fuori da `RTCombatLibrary.h`.
- Nulla sulla QUALITA' della copertura: 19 test che cadono dicono che la costante e' intrecciata ovunque,
  non che sia ben misurata.
- Nulla sui valori che non sono `constexpr int32` — cataloghi, `Parameters`, dati degli eroi.
- ⚠️ E `Scenario.EveryShippedScenarioRuns` compariva in **8** righe su 11: e' il singolo test che regge piu'
  costanti dell'intero perimetro. Un suo indebolimento non toglierebbe una copertura, ne toglierebbe otto.
"""
import io, os, re, subprocess, sys, time

RADICE = "D:/Repositories/refactor-tactics-technical-designer/refactor-tactics-main"
H = os.path.join(RADICE, "Source/RefactorTactics/Combat/RTCombatLibrary.h")
LOG = os.path.join(RADICE, "Saved/Logs/rt-suite.log")
ESITI = sys.argv[1]

os.chdir(RADICE)
ORIGINALE = io.open(H, encoding="utf-8").read()
COST = [c for c in re.findall(r"static constexpr int32 (\w+)\s*=\s*(-?\d+);", ORIGINALE)
        if len(sys.argv) < 3 or c[0] in sys.argv[2:]]


def ripristina():
    io.open(H, "wb").write(ORIGINALE.encode("utf-8"))


def build(tentativi=40):
    """Ricostruisce. Torna True solo su `Result: Succeeded`; attende se Live Coding blocca."""
    for i in range(tentativi):
        r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command",
                            r"& 'D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat' RefactorTacticsEditor "
                            r"Win64 Development -Project='" + RADICE.replace("/", "\\") +
                            r"\RefactorTactics.uproject' -WaitMutex"],
                           capture_output=True, text=True, errors="replace")
        if "Result: Succeeded" in (r.stdout or ""):
            return True
        if "Live Coding" in (r.stdout or ""):
            time.sleep(45)   # e' l'Editor di un altro checkout: si aspetta, non si termina
            continue
        return False
    return False


def suite():
    """Esegue la suite intera e torna (verdetto, esito, elenco dei test rossi)."""
    r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                        "scripts/rt-suite.ps1", "-WaitMinutes", "90"],
                       capture_output=True, text=True, errors="replace")
    out = r.stdout or ""
    verdetto = "VALIDA" if "[RT-MEASURE] VALIDA" in out else \
               ("NON AVVIATA" if "NON AVVIATA" in out else "ALTRO")
    m = re.search(r"esito\s+(\d+)/(\d+) completati, (\d+) fallimenti", out)
    esito = m.group(0) if m else "(nessun esito)"
    rossi = []
    if os.path.exists(LOG):
        testo = io.open(LOG, encoding="utf-8", errors="replace").read()
        rossi = sorted(set(re.findall(r"Result=\{Fail\} Name=\{[^}]*\} Path=\{([^}]+)\}", testo)))
    return verdetto, esito, rossi


with io.open(ESITI, "w", encoding="utf-8") as f:
    f.write("# Quali costanti si possono cambiare senza che niente diventi rosso\n\n")
    f.flush()
    for nome, val in COST:
        nuovo = str(int(val) + 3)
        io.open(H, "wb").write(
            ORIGINALE.replace("static constexpr int32 %s = %s;" % (nome, val),
                              "static constexpr int32 %s = %s;" % (nome, nuovo), 1).encode("utf-8"))
        if not build():
            f.write("## %s = %s -> %s\n**NON MISURATA**: il build non e' riuscito.\n\n" % (nome, val, nuovo))
            f.flush(); ripristina(); continue

        verdetto, esito, rossi = suite()
        f.write("## %s = %s -> %s\n" % (nome, val, nuovo))
        f.write("- misura: **%s** — %s\n" % (verdetto, esito))
        if verdetto != "VALIDA":
            f.write("- ⚠️ misura non valida: non conta\n\n")
        elif rossi:
            f.write("- **%d test se ne accorgono**:\n" % len(rossi))
            for t in rossi:
                f.write("  - `%s`\n" % t)
            f.write("\n")
        else:
            f.write("- 🔴 **NESSUN test se ne accorge.**\n\n")
        f.flush()
        ripristina()

ripristina()
build()
print("AUDIT COMPLETO")
