# -*- coding: utf-8 -*-
"""Le cinque mutazioni della caduta devono far CADERE un test - e il gate lo dimostra.

    Uso:  python tools/mutation/caduta-gate.py <esiti.md> [--solo ID ...] [--taratura]
                                               [--filtro <prefisso>] [--dry-run] [--self-test]

Per ognuna: muta -> verifica che sia ATTERRATA -> ricostruisce -> esegue la suite filtrata ->
registra QUALI test cadono -> confronta con i bersagli attesi -> ripristina.

## Perche' esiste

`#2406` non chiede di far girare la suite: chiede di dimostrare che i test della caduta **possono
fallire**. Una suite verde su test che non sanno diventare rossi non e' un gate, e' una decorazione.

## 🔴 Le tre risposte, e perche' vanno tenute distinte

Un gate ingenuo ne conosce due - verde e rosso - e le confonde entrambe con la terza:

    CADUTA             la mutazione ha fatto fallire i bersagli attesi        -> il test regge
    SOPRAVVISSUTA      la mutazione e' atterrata e NIENTE e' diventato rosso  -> il test e' cieco
    NON APPLICABILE    il pattern non esiste nel sorgente                     -> niente da mutare

**La terza e' quella che inganna.** Se il ramo di `#2402` non e' ancora integrato, i pattern delle
mutazioni 1-5 non si trovano, e uno strumento scritto male stamperebbe «0 sopravvissute» - cioe'
verde - su un gate che non ha misurato nulla. Qui `NON APPLICABILE` e' un esito **bloccante** e
distinto, e il gate esce con codice 2: `BLOCKED`, mai `PASS`.

## 🔴 IL VERSO DELLA MISURA

`#2406` avverte: «se la quantita' misurata sale invece di scendere, stai misurando il complemento».
Qui il verso e' controllato in modo esplicito, e in due punti:

1. i nuovi rossi si ottengono per DIFFERENZA di insiemi contro la baseline, non contando i totali:
   un rosso preesistente che sparisce non puo' mascherare una mutazione sopravvissuta;
2. i nuovi rossi devono **includere i bersagli dichiarati**. Se cade altro, l'esito e'
   `CADUTA (BERSAGLI DIVERSI)` e va letto a mano: una mutazione puo' far cadere un test per la
   ragione sbagliata, ed e' indistinguibile da quella giusta se si conta soltanto.

⛔ **Contare i rossi non basta.** Si confrontano gli INSIEMI, per nome di test, contro una baseline.

## 🔴 La mutazione piu' vera e' il file prima del fix

La riscrittura per pattern e' verificata due volte - che il testo sia cambiato, e che sia cambiato
di piu' della sola spaziatura - perche' `#2406` avverte che una mutazione scritta a mano spesso non
compila, e una che compila puo' **appendere** invece di far cadere.

## Le scelte che rendono la misura onesta

🔴 **Baseline VALIDA obbligatoria.** Senza, un rosso preesistente verrebbe attribuito alla mutazione.
E non basta che la suite finisca: `rt-suite` dichiara `VALIDA` solo se `HEAD`, working tree, binario
e stato del motore non cambiano durante la misura. Un `NON VALIDA` ferma il gate.

🔴 **Guardia sul non committato.** Il ripristino usa i byte letti in memoria, non `git checkout --`:
`#2406` avverte che quel comando **cancella il non committato**, e questo strumento gira anche
accanto al lavoro di qualcun altro. Se il file bersaglio e' sporco, il gate si ferma PRIMA di
toccarlo.

🔴 **Si verifica che la mutazione sia ATTERRATA.** Una sostituzione no-op farebbe scattare l'allarme
piu' forte (`SOPRAVVISSUTA`) su un binario non mutato.

🔴 **Ripristino a INIZIO ciclo e in `finally`.** Una corsa interrotta lascia la mutazione sul disco:
senza il ripristino iniziale diventerebbe la base di tutte le misure seguenti.

🔴 **Il self-test.** `--self-test` esercita la logica dello strumento - i tre esiti, il controllo di
atterraggio, la differenza di insiemi - senza motore e senza sorgenti, in un secondo. Non prova che
il gate parli correttamente con Unreal: prova che, dati quei numeri, li classifica come dichiarato.
E' la stessa convenzione di `rt-suite.ps1 -SelfTest`.

🔴 **La taratura.** `--taratura` esegue una sesta mutazione su codice che esiste **oggi**
(`URTHexLedgeLibrary::IsEdgeOpen`, `#2401`, gia' su `main`): sopprime la guardia del parapetto, e il
bersaglio e' `Map.OpenEdge.GuardSuppressesOpenness`. Risponde alla domanda che precede tutte le altre
- *questo strumento sa distinguere un verde da un rosso?* - senza dipendere da `#2402`.
⛔ Un gate mai tarato che stampa «tutte cadute» non e' evidenza.

## ⛔ Cosa NON copre

- **Non giudica se il test e' giusto.** Dice che un test cade, non che asserisce la regola della
  spec. Una mutazione puo' far cadere un test per un motivo diverso da quello dichiarato in
  `bersagli`, e il gate lo puo' solo segnalare (`BERSAGLI DIVERSI`), non risolvere.
- **Non copre le mutazioni che non sono in tabella.** Cinque mutazioni sono cinque, non «il codice
  della caduta e' coperto».
- **Non vede il binario.** Come `rt-suite`: se non stampa `GATE COMPLETO`, il DLL sul disco puo'
  essere ancora quello mutato - **ricostruire prima di qualunque altra misura**.
- **Non misura gli effetti numerici della caduta** (`FallEffects`/`ImpactEffects`): non esistono come
  dato, e `#2402` D001 lo dichiara. Sono materia di `#2430`.
- **Non sostituisce l'accettazione in Editor** (`#2408`) ne' l'evidenza packaged (`#2407`).

## ⛔ Il rischio, dichiarato

Modifica un sorgente e occupa il motore: **ogni altra misura in parallelo e' NON VALIDA**. Un ciclo
e' un build piu' una suite filtrata per mutazione, piu' la baseline.
"""

import io
import os
import re
import subprocess
import sys
import time

# 🔴 Lo stdout di Windows e' `cp1252`, e un `print` con un carattere fuori tabella solleva
# `UnicodeEncodeError`: misurato su questo stesso strumento, che moriva sul simbolo del verdetto
# BLOCKED dopo aver scritto correttamente il file degli esiti. Un gate che termina per un carattere
# non stampabile riporta un fallimento che non c'e', ed e' il tipo di rumore che fa ignorare i gate.
# Il file degli esiti resta UTF-8: qui si difende solo la console.
try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
except AttributeError:
    pass   # Python < 3.7: i print ASCII passano comunque

RADICE = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOG = os.path.join(RADICE, "Saved", "Logs", "rt-suite.log")
UPROJECT = os.path.join(RADICE, "RefactorTactics.uproject")
BUILD_BAT = r"D:\EpicGames\UE_5.8\Engine\Build\BatchFiles\Build.bat"

# Filtro della suite. Il mutex del motore ferma ogni checkout, e una suite intera per mutazione sono
# ore: #2406 chiede un filtro stretto. Resta comunque un prefisso, non una lista di nomi, perche' una
# mutazione che fa cadere un test FUORI dal filtro e' esattamente cio' che non si vuole non vedere.
FILTRO = "RefactorTactics"

# --------------------------------------------------------------------------------------------------
# Le mutazioni.
#
# `cerca`/`sostituisci` sono espressioni regolari applicate al testo del file. `bersagli` sono i nomi
# dei test che DEVONO diventare rossi: derivati da `docs/gameplay/spec-caduta-e-bordi.md`, non
# dall'implementazione - che per 1-5 non esiste ancora.
#
# ⚠️ I pattern di 1-5 sono una PREVISIONE della forma che #2402 dara' al ramo, e vanno riletti contro
# il codice reale quando quello arriva. Se non si trovano, il gate riporta NON APPLICABILE e si ferma:
# e' l'esito corretto, non un verde.
# --------------------------------------------------------------------------------------------------
MUTAZIONI = [
    {
        "id": "0-taratura",
        "titolo": "sopprimere la guardia del parapetto",
        "prova": "che QUESTO strumento sa distinguere un verde da un rosso (#2401, codice esistente)",
        "file": "Source/RefactorTactics/Map/RTHexLedgeLibrary.cpp",
        "cerca": r"if \(Data->HasGuardOn\(Edge\)\)",
        "sostituisci": "if (false && Data->HasGuardOn(Edge))",
        "bersagli": ["RefactorTactics.Map.OpenEdge.GuardSuppressesOpenness"],
        "solo_con": "--taratura",
    },
    {
        "id": "1-passi-residui",
        "titolo": "conservare i passi di spinta residui dopo la caduta",
        "prova": "#2402 - i passi residui sono persi (spec §3.1)",
        "file": "Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp",
        "cerca": r"(//[^\n]*passi residui[^\n]*\n(?:[^\n]*\n){0,6}?\s*)break;",
        "sostituisci": r"\1continue;",
        "bersagli": ["RefactorTactics.Fall.DisplacementEndsAtOpenEdge"],
    },
    {
        "id": "2-effetti-saturo",
        "titolo": "sopprimere gli effetti di caduta nel fallback saturo",
        "prova": "#2402 - gli effetti non sono la posizione: si applicano SEMPRE (spec §4.3, §5)",
        "file": "Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp",
        "cerca": r"(\w*ApplyFallEffects\w*\([^;]*\);)",
        "sostituisci": r"/* MUT2 */ ;",
        "bersagli": ["RefactorTactics.Fall.EffectsApplyEvenWhenSaturated"],
    },
    {
        "id": "3-ordine-adiacente",
        "titolo": "cambiare l'ordine adiacente canonico",
        "prova": "#2402 - determinismo: l'anello e' E->NE->NW->W->SW->SE (spec §4.2)",
        "file": "Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp",
        "cerca": r"for \(int32 (\w+) = 0; \1 < 6; \+\+\1\)",
        "sostituisci": r"for (int32 \1 = 5; \1 >= 0; --\1)",
        "bersagli": ["RefactorTactics.Fall.AlternativeFollowsCanonicalRing"],
    },
    {
        "id": "4-sovrapposizione",
        "titolo": "permettere la sovrapposizione",
        "prova": "#2402 - una unita' per cella, in tutti e tre gli esiti (spec §4.2.3, §4.3.7)",
        "file": "Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp",
        "cerca": r"if \((\w*IsOccupied\w*|\w*Occupant\w*) *(\([^)]*\))?\)",
        "sostituisci": r"if (false)",
        "bersagli": ["RefactorTactics.Fall.NoCellSharingInAnyOutcome"],
    },
    {
        "id": "5-percorso-volontario",
        "titolo": "non annullare il percorso volontario",
        "prova": "#2402 - niente auto-reroute dalla nuova posizione (spec §3.2)",
        "file": "Source/RefactorTactics/Turn/RTTurnManager_Blast.cpp",
        "cerca": r"(\w+\.(?:Path|PlannedPath|Remaining\w*)\.(?:Empty|Reset)\(\);)",
        "sostituisci": r"/* MUT5 */ ;",
        "bersagli": ["RefactorTactics.Fall.VoluntaryPathIsCancelled"],
    },
]


def uso():
    print(__doc__.split("\n\n")[1].strip())
    sys.exit(2)


# ==================================================================================================
# Le due decisioni dello strumento, isolate come funzioni PURE.
#
# Sono qui, e non dentro il ciclo, per una ragione sola: un gate che nessuno puo' esercitare senza
# occupare il motore per ore non viene mai esercitato. `--self-test` le prova in un secondo.
# ==================================================================================================
def atterrata(prima, dopo):
    """La mutazione ha davvero cambiato il sorgente?

    🔴 Una sostituzione no-op farebbe scattare l'allarme piu' forte dello strumento
    (`SOPRAVVISSUTA` = «nessun test se ne accorge») su un binario NON mutato: la peggiore risposta
    possibile, perche' indistinguibile da una lacuna vera. La spaziatura non conta come mutazione."""
    if dopo == prima:
        return False
    return dopo.replace(" ", "").replace("\t", "") != prima.replace(" ", "").replace("\t", "")


def classifica(verdetto, nuovi, attesi):
    """I tre esiti, piu' il quarto che si vede solo confrontando gli INSIEMI.

    `nuovi` sono i rossi che la baseline non aveva. Si passa la DIFFERENZA, mai un totale: contare i
    fallimenti lascerebbe un rosso preesistente che sparisce compensare una mutazione sopravvissuta,
    ed e' il modo in cui si finisce a misurare il complemento (#2406)."""
    if verdetto != "VALIDA":
        return "MISURA " + verdetto, False
    if not nuovi:
        return "SOPRAVVISSUTA", False           # l'esito che il gate esiste per trovare
    if attesi & nuovi:
        return "CADUTA", True
    return "CADUTA (BERSAGLI DIVERSI)", True    # cade qualcosa, ma non cio' che si diceva di misurare


def bersagli_mancanti(attesi, eseguiti):
    """Quali bersagli la baseline non ha nemmeno ESEGUITO.

    🔴 Un bersaglio che non gira non puo' cadere. Se manca, l'esito della sua mutazione sarebbe
    `SOPRAVVISSUTA` — l'allarme piu' forte dello strumento — puntando pero' su un filtro sbagliato o
    su un Editor morto all'avvio (`#2393`), non su un test cieco. Sono due diagnosi opposte, e
    confonderle manda a riscrivere un test che stava benissimo."""
    return set(attesi) - set(eseguiti)


def self_test():
    """Esercita `atterrata` e `classifica`. Nessun motore, nessun sorgente toccato."""
    casi = []

    def c(nome, ottenuto, atteso):
        casi.append((nome, ottenuto == atteso, ottenuto, atteso))

    c("no-op e' non atterrata", atterrata("if (X)", "if (X)"), False)
    c("sola spaziatura e' non atterrata", atterrata("if (X)", "if  (X)"), False)
    c("sola tabulazione e' non atterrata", atterrata("\tif (X)", "if (X)"), False)
    c("cambio vero e' atterrata", atterrata("if (X)", "if (false)"), True)

    c("baseline non valida non e' una caduta",
      classifica("NON VALIDA", {"A"}, {"A"}), ("MISURA NON VALIDA", False))
    c("nessun nuovo rosso e' SOPRAVVISSUTA",
      classifica("VALIDA", set(), {"A"}), ("SOPRAVVISSUTA", False))
    c("il bersaglio atteso cade",
      classifica("VALIDA", {"A", "B"}, {"A"}), ("CADUTA", True))
    c("cade altro, non il bersaglio",
      classifica("VALIDA", {"B"}, {"A"}), ("CADUTA (BERSAGLI DIVERSI)", True))

    # 🔴 Il caso che motiva la differenza di insiemi: la mutazione non fa cadere niente di nuovo, e
    # nel frattempo un rosso preesistente e' passato. Contando i totali (1 -> 1, oppure 2 -> 1) si
    # leggerebbe «nessun peggioramento» o addirittura un miglioramento; per insiemi resta scoperta.
    rossi_base = {"Preesistente", "AltroPreesistente"}
    rossi_dopo = {"Preesistente"}
    c("un rosso preesistente che guarisce non maschera la sopravvissuta",
      classifica("VALIDA", rossi_dopo - rossi_base, {"A"}), ("SOPRAVVISSUTA", False))

    # 🔴 #2393: Editor morto all'avvio su worktree nuovo -> `0/?, 0 fail`. Zero rossi su zero test
    # eseguiti supera qualunque controllo che guardi solo i rossi.
    c("nessun test eseguito: il bersaglio risulta mancante",
      bersagli_mancanti({"A"}, set()), {"A"})
    c("filtro che esclude il bersaglio",
      bersagli_mancanti({"A"}, {"B", "C"}), {"A"})
    c("bersaglio eseguito: niente da segnalare",
      bersagli_mancanti({"A"}, {"A", "B"}), set())

    falliti = [x for x in casi if not x[1]]
    for nome, ok, ottenuto, atteso in casi:
        print("  %s %s" % ("ok  " if ok else "FAIL", nome))
        if not ok:
            print("       ottenuto=%r atteso=%r" % (ottenuto, atteso))
    print("\nself-test: %d/%d" % (len(casi) - len(falliti), len(casi)))
    return 1 if falliti else 0


if "--self-test" in sys.argv:
    sys.exit(self_test())

if len(sys.argv) < 2 or sys.argv[1].startswith("--"):
    uso()

ESITI = sys.argv[1]
ARGS = sys.argv[2:]
TARATURA = "--taratura" in ARGS
DRY = "--dry-run" in ARGS
SOLO = []
if "--solo" in ARGS:
    SOLO = [a for a in ARGS[ARGS.index("--solo") + 1:] if not a.startswith("--")]
if "--filtro" in ARGS:
    FILTRO = ARGS[ARGS.index("--filtro") + 1]

os.chdir(RADICE)

DA_ESEGUIRE = []
for m in MUTAZIONI:
    if m.get("solo_con") == "--taratura" and not TARATURA:
        continue
    if SOLO and m["id"] not in SOLO:
        continue
    DA_ESEGUIRE.append(m)

if not DA_ESEGUIRE:
    print("nessuna mutazione selezionata")
    sys.exit(2)


def scrivi(percorso, dati):
    """Scrittura ATOMICA, e in BYTE.

    In byte perche' il ripristino deve rimettere il file **identico**: leggendo in testo e
    riscrivendo, i CRLF diventano LF e `git status` mostra modificato un file che non e' cambiato -
    basta a rendere NON VALIDA la misura di chiunque altro sullo stesso checkout.

    Atomica perche' `open(...,'wb')` TRONCA prima di valutare cosa scrivere: un errore in quella
    finestra lascerebbe il sorgente a zero byte, e la cosa dopo e' un build."""
    tmp = percorso + ".tmp"
    with io.open(tmp, "wb") as f:
        f.write(dati)
    os.replace(tmp, percorso)


def sporco(rel):
    """True se il file ha modifiche non committate. Il gate NON scrive sopra il lavoro di un altro."""
    r = subprocess.run(["git", "status", "--porcelain", "--", rel],
                       capture_output=True, text=True, errors="replace")
    return bool((r.stdout or "").strip())


def build(tentativi=40):
    """Ricostruisce. True solo su `Result: Succeeded`; ritenta su QUALUNQUE fallimento.

    🔴 Un build fallito lascia il binario VECCHIO: la suite girerebbe sul codice NON mutato e
    riporterebbe «nessun test se ne accorge» - indistinguibile da una lacuna vera, e la peggiore
    risposta possibile. Si ritenta perche' la causa tipica e' un Editor su un altro checkout
    (`Unable to build while Live Coding is active`, oppure `Failed (OtherCompilationError)` di #971)."""
    for _ in range(tentativi):
        r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command",
                            "& '" + BUILD_BAT + "' RefactorTacticsEditor Win64 Development "
                            "-Project='" + UPROJECT + "' -WaitMutex"],
                           capture_output=True, text=True, errors="replace")
        testo = (r.stdout or "") + (r.stderr or "")   # UBT manda alcune righe su stderr
        if "Result: Succeeded" in testo:
            return True
        time.sleep(45)   # e' il motore occupato: si aspetta, non si termina il processo di un altro
    return False


def suite():
    """Coda per il motore, poi misura. Torna (verdetto, esito, rossi, eseguiti).

    🔴 `eseguiti` non e' un di piu': e' il modo di accorgersi che la suite non ha misurato niente.
    `#2393` descrive un Editor che muore durante l'avvio su un **worktree nuovo**, con `rt-suite` che
    riporta `0/?, 0 fail`. Zero fallimenti su zero test eseguiti supera qualunque controllo che guardi
    solo i rossi, e una mutazione sembrerebbe SOPRAVVISSUTA su una misura mai avvenuta."""
    r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                        "scripts/rt-suite.ps1", "-Filter", FILTRO, "-WaitMinutes", "90"],
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
    eseguiti = set()
    if os.path.exists(LOG):
        testo = io.open(LOG, encoding="utf-8", errors="replace").read()
        rossi = set(re.findall(r"Result=\{Fail\} Name=\{[^}]*\} Path=\{([^}]+)\}", testo))
        eseguiti = set(re.findall(r"Test Started\. Name=\{[^}]*\} Path=\{([^}]+)\}", testo))
    return verdetto, esito, rossi, eseguiti


def misura():
    """Ricostruisce, poi misura. Se la run non ha nemmeno preso il motore RICOSTRUISCE e riprova: in
    una coda lunga un'altra sessione puo' aver riscritto il DLL condiviso (AGENTS.md, CLAUDE.md §6)."""
    if not build():
        return None
    v, e, rossi, eseguiti = suite()
    if v == "NON AVVIATA":
        if not build():
            return None
        v, e, rossi, eseguiti = suite()
    return v, e, rossi, eseguiti


def applicabile(m):
    """Si misura PRIMA di toccare il motore.

    🔴 E' il controllo che separa «il test e' cieco» da «non c'e' niente da mutare». Senza, un gate
    lanciato prima che #2402 sia integrata stamperebbe zero sopravvissute su zero mutazioni."""
    percorso = os.path.join(RADICE, m["file"])
    if not os.path.exists(percorso):
        return False, "il file non esiste: " + m["file"]
    testo = io.open(percorso, encoding="utf-8", errors="replace").read()
    if not re.search(m["cerca"], testo):
        return False, "il pattern non si trova in " + m["file"]
    return True, ""


print("=" * 94)
print("GATE DELLA CADUTA - #2406")
print("=" * 94)

STATO = []
for m in DA_ESEGUIRE:
    ok, motivo = applicabile(m)
    STATO.append((m, ok, motivo))
    print("  [%-22s] %s" % (m["id"], "APPLICABILE" if ok else "NON APPLICABILE - " + motivo))

APPLICABILI = [m for (m, ok, _) in STATO if ok]
NON_APPLICABILI = [(m, motivo) for (m, ok, motivo) in STATO if not ok]

if DRY:
    print("\n--dry-run: nessun build, nessuna suite, nessuna scrittura sui sorgenti.")
    sys.exit(0 if not NON_APPLICABILI else 2)

for m in APPLICABILI:
    if sporco(m["file"]):
        print("\n⛔ FERMO: %s ha modifiche non committate.\n"
              "   Il gate non scrive sopra il lavoro di qualcun altro." % m["file"])
        sys.exit(2)

if not APPLICABILI:
    with io.open(ESITI, "w", encoding="utf-8") as f:
        f.write("# Gate della caduta - #2406\n\n")
        f.write("## ⛔ BLOCKED - nessuna mutazione applicabile\n\n")
        f.write("Il gate non ha misurato niente, e questo **non e' un verde**.\n\n")
        f.write("| # | Mutazione | Perche' non e' applicabile |\n|---|---|---|\n")
        for m, motivo in NON_APPLICABILI:
            f.write("| %s | %s | %s |\n" % (m["id"], m["titolo"], motivo))
        f.write("\nLe mutazioni 1-5 mutano il ramo di **#2402**. Finche' quello non e' integrato non\n"
                "c'e' niente da mutare, e nessun test della caduta da far cadere.\n")
    print("\n⛔ BLOCKED: nessuna mutazione applicabile. Esiti in " + ESITI)
    sys.exit(2)

ORIGINALI = {}
for m in APPLICABILI:
    ORIGINALI[m["file"]] = io.open(os.path.join(RADICE, m["file"]), "rb").read()


def ripristina_tutto():
    for rel, byte in ORIGINALI.items():
        scrivi(os.path.join(RADICE, rel), byte)


sopravvissute = []
bersagli_diversi = []
cadute = []
completo = False

try:
    ripristina_tutto()   # una corsa interrotta lascia la mutazione sul disco

    with io.open(ESITI, "w", encoding="utf-8") as f:
        f.write("# Gate della caduta - #2406\n\n")
        f.write("Filtro: `%s`. Mutazioni eseguite: %d.\n\n" % (FILTRO, len(APPLICABILI)))
        if NON_APPLICABILI:
            f.write("## ⚠️ Non applicabili (il gate NON le ha misurate)\n\n")
            for m, motivo in NON_APPLICABILI:
                f.write("- `%s` - %s\n" % (m["id"], motivo))
            f.write("\n")
        f.flush()

        base = misura()
        if base is None or base[0] != "VALIDA":
            f.write("## ⛔ BASELINE NON MISURABILE (%s)\n\nSenza un punto di riferimento ogni rosso\n"
                    "verrebbe attribuito alla mutazione. Il gate non parte.\n"
                    % (base[0] if base else "build fallito"))
            print("BASELINE NON MISURABILE")
            sys.exit(1)
        ROSSI_BASE = base[2]
        ESEGUITI_BASE = base[3]

        # 🔴 La baseline ha davvero ESEGUITO i bersagli? Senza questo controllo, un Editor morto
        # all'avvio (#2393) o un filtro troppo stretto darebbero `SOPRAVVISSUTA` su tutta la riga.
        TUTTI_ATTESI = set()
        for m in APPLICABILI:
            TUTTI_ATTESI |= set(m["bersagli"])
        mancanti = bersagli_mancanti(TUTTI_ATTESI, ESEGUITI_BASE)
        if mancanti:
            f.write("## ⛔ BASELINE SENZA BERSAGLI\n\nLa baseline e' `%s` ma NON ha eseguito: %s\n\n"
                    "Test eseguiti in totale: **%d**. Un bersaglio che non gira non puo' cadere, e la\n"
                    "sua mutazione risulterebbe SOPRAVVISSUTA puntando su un filtro sbagliato o su un\n"
                    "Editor morto all'avvio (#2393) invece che su un test cieco. Il gate non parte.\n"
                    % (base[0], ", ".join(sorted(mancanti)), len(ESEGUITI_BASE)))
            print("BASELINE SENZA BERSAGLI: " + ", ".join(sorted(mancanti))
                  + " (test eseguiti: %d)" % len(ESEGUITI_BASE))
            sys.exit(1)

        f.write("Baseline: **%s** - %s - test eseguiti: **%d**%s\n\n" % (
            base[0], base[1], len(ESEGUITI_BASE),
            (" - gia' rossi: " + ", ".join(sorted(ROSSI_BASE))) if ROSSI_BASE else ""))
        f.write("| # | Mutazione | Esito | Nuovi rossi | Bersagli attesi |\n|---|---|---|---|---|\n")
        f.flush()

        for m in APPLICABILI:
            ripristina_tutto()
            percorso = os.path.join(RADICE, m["file"])
            prima = ORIGINALI[m["file"]].decode("utf-8")
            dopo = re.sub(m["cerca"], m["sostituisci"], prima, count=1)

            if not atterrata(prima, dopo):
                f.write("| %s | %s | ⛔ NON ATTERRATA | - | - |\n" % (m["id"], m["titolo"]))
                f.flush()
                sopravvissute.append(m["id"] + " (non atterrata)")
                continue

            scrivi(percorso, dopo.encode("utf-8"))
            r = misura()
            ripristina_tutto()

            if r is None:
                f.write("| %s | %s | ⛔ BUILD FALLITO | - | - |\n" % (m["id"], m["titolo"]))
                f.flush()
                sopravvissute.append(m["id"] + " (build fallito)")
                continue

            verdetto, esito, rossi, eseguiti = r
            nuovi = rossi - ROSSI_BASE          # DIFFERENZA di insiemi, mai un totale
            # Se la run mutata non ha eseguito il bersaglio, non e' cieco: e' assente dalla misura.
            if verdetto == "VALIDA" and bersagli_mancanti(m["bersagli"], eseguiti):
                verdetto = "SENZA BERSAGLIO (%d test eseguiti)" % len(eseguiti)
            attesi = set(m["bersagli"])
            etichetta, e_caduta = classifica(verdetto, nuovi, attesi)

            if not e_caduta:
                sopravvissute.append(m["id"] if etichetta == "SOPRAVVISSUTA"
                                     else m["id"] + " (" + etichetta.lower() + ")")
                simbolo = "⛔"
            elif etichetta == "CADUTA":
                cadute.append(m["id"])
                simbolo = "✅"
            else:
                bersagli_diversi.append(m["id"])
                simbolo = "⚠️"

            f.write("| %s | %s | %s %s | %s | %s |\n" % (
                m["id"], m["titolo"], simbolo, etichetta,
                ", ".join(sorted(nuovi)) if nuovi else "nessuno",
                ", ".join(sorted(attesi))))
            f.flush()

        f.write("\n## Verdetto\n\n")
        if sopravvissute:
            f.write("⛔ **GATE ROSSO**: %d mutazione/i non ha fatto cadere nessun test: %s\n"
                    % (len(sopravvissute), ", ".join(sopravvissute)))
        elif NON_APPLICABILI:
            f.write("⚠️ **GATE PARZIALE**: le applicabili sono cadute, ma %d non sono state misurate.\n"
                    % len(NON_APPLICABILI))
        else:
            f.write("✅ **GATE VERDE**: tutte le mutazioni hanno fatto cadere almeno un test.\n")
        if bersagli_diversi:
            f.write("\n⚠️ Da leggere a mano (cade un test diverso da quello dichiarato): %s\n"
                    % ", ".join(bersagli_diversi))
    completo = True
finally:
    ripristina_tutto()
    if completo:
        print("\nGATE COMPLETO - esiti in " + ESITI)
    else:
        print("\n⛔ GATE INTERROTTO: il sorgente e' ripristinato ma il BINARIO puo' essere ancora\n"
              "   quello mutato. RICOSTRUIRE prima di qualunque altra misura.")

if sopravvissute:
    sys.exit(1)
if NON_APPLICABILI:
    sys.exit(2)
sys.exit(0)
