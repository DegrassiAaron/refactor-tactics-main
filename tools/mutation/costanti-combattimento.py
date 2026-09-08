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
Editor su un altro checkout). ⚠️ Si ritenta sulla CONTESA del motore, non su qualunque fallimento: un
errore di compilazione non cambia col tempo, e ritentarlo costava mezz'ora di silenzio. La distinzione
sta in `misura.CONTESA`, e `OtherCompilationError` non vi appartiene — UBT lo emette anche per un errore
C++ vero, che e' il caso comune quando una mutazione scritta a mano non compila.

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
import atexit, io, os, re, subprocess, sys

# Stessa sede della regola di validita' dell'altro gate: `regola.verdetto()` e' pura, e i due
# strumenti la CHIAMANO invece di tenerne una copia a testa. Le due copie precedenti erano gia'
# divergenti — arita' diversa e interprete diverso — dopo un solo commit di vita.
import misura as regola   # 'misura' e' gia' il nome di una funzione, qui sotto

# La radice si deriva dal file, non si scrive: su questa macchina esistono tre copie del repository, e una
# costante scritta a mano muterebbe l'albero di qualcun altro lasciando pulito il proprio.
RADICE = regola.RADICE
H = os.path.join(RADICE, "Source", "RefactorTactics", "Combat", "RTCombatLibrary.h")
LOG = regola.LOG
UPROJECT = regola.UPROJECT
ENGINE_CMD = regola.ENGINE_CMD
DLL_GLOB = regola.DLL_GLOB



# --- le due decisioni PURE di questo file, e quindi le uniche che un test puo' esercitare ---------

def dichiarazioni(testo):
    """I `(nome, valore)` delle costanti dichiarate. PURA: prende testo, torna coppie.

    Era una `re.findall` a livello di modulo, quindi non esercitabile da nessun test — e decide
    COSA viene mutato: un pattern che non aggancia una dichiarazione la salta in silenzio, e la
    costante risulterebbe «non misurata» senza che si sappia perche'."""
    return re.findall(r"static constexpr int32 (\w+)\s*=\s*(-?\d+)\s*;", testo)


def muta(byte_originali, nome, valore, nuovo):
    """Sostituisce UNA dichiarazione, in byte. Torna i byte mutati, o `None` se non aggancia.

    🔴 PURA e in BYTE, come la scrittura, e per la stessa ragione. Questo header e' CRLF: una
    rilettura in modalita' TESTO applica le universal newlines e restituisce `\\n`, che non sara'
    mai uguale al testo atteso in `\\r\\n`. La verifica falliva su OGNI costante, e l'audit
    dichiarava undici `NON MISURATA` mutando benissimo il codice: la guardia contro la scrittura
    no-op scattava su se' stessa, e la direzione `-3` non e' mai stata misurabile. Misurato il
    2026-09-03: 17603 byte decodificati contro 17276 caratteri riletti, differenza 327,
    esattamente le righe del file.

    ⚠️ Quel difetto e' vissuto in un ramo che nessun test poteva raggiungere, perche' la logica
    stava dentro il ciclo che muta i sorgenti. Ora e' qui, e il self-test la esercita (`#2672`).

    🔴 **L'ancora si DERIVA dal testo, non si ricostruisce.** Scrivere `"... %s = %s;"` impone
    una spaziatura sola, mentre `dichiarazioni()` ne accetta qualunque (`\\s*`): una riga come
    `int32 BaseShield  =  5 ;` veniva trovata dal parser e poi non agganciata dal mutatore, e
    l'audit la dichiarava `NON MISURATA` — una costante selezionata e mai misurata, con la
    ragione sbagliata scritta accanto. I due devono leggere la stessa grammatica."""
    ancora = re.compile((r"(static\s+constexpr\s+int32\s+%s\s*=\s*)(%s)(\s*;)"
                         % (re.escape(nome), re.escape(str(valore)))).encode("utf-8"))
    mutati, quante = ancora.subn(lambda m: m.group(1) + str(nuovo).encode("utf-8") + m.group(3),
                                 byte_originali, count=1)
    return mutati if quante else None


def _self_test_locale():
    """I casi di QUESTO file. Quelli della regola di validita' stanno in `misura.self_test`."""
    casi = []

    def c(nome, ok, dettaglio=""):
        casi.append((nome, bool(ok), str(dettaglio)))

    sorgente = ("// commento\r\n"
                "static constexpr int32 DeflectDamageReduction = 20;\r\n"
                "static constexpr int32 BaseShield  =  5 ;\r\n"
                "static constexpr int32 Negativo = -3;\r\n"
                "static constexpr float NonIntero = 1.5f;\r\n")
    trovate = dict(dichiarazioni(sorgente))
    c("il parser trova le dichiarazioni int32", trovate.get("DeflectDamageReduction") == "20", trovate)
    c("tollera la spaziatura larga", trovate.get("BaseShield") == "5", trovate)
    c("legge i valori negativi", trovate.get("Negativo") == "-3", trovate)
    c("ignora cio' che non e' int32", "NonIntero" not in trovate, trovate)

    byte = sorgente.encode("utf-8")
    # 🔑 Il caso del 2026-09-03: su un file CRLF la mutazione DEVE agganciare. Se questo cade,
    # l'audit dichiara «NON MISURATA» ogni costante mutando benissimo il codice.
    mutati = muta(byte, "DeflectDamageReduction", "20", "23")
    c("la mutazione aggancia su un file CRLF", mutati is not None and b"= 23;" in mutati)
    # 🔑 Il parser accetta la spaziatura larga: il mutatore deve accettarla anche lui, o la
    # costante viene selezionata e poi dichiarata NON MISURATA per una ragione falsa.
    largo = muta(byte, "BaseShield", "5", "8")
    c("aggancia anche la spaziatura larga che il parser tollera",
      largo is not None and b"=  8 ;" in largo, largo)
    c("e non tocca il resto del file",
      mutati is not None and mutati.count(b"\r\n") == byte.count(b"\r\n"))
    c("un valore che non esiste non aggancia", muta(byte, "DeflectDamageReduction", "99", "102") is None)
    c("un nome che non esiste non aggancia", muta(byte, "Inesistente", "1", "4") is None)
    c("muta UNA sola occorrenza",
      muta((sorgente + sorgente).encode("utf-8"), "Negativo", "-3", "0").count(b"= 0;") == 1)
    return casi


# Il self-test non tocca ne' sorgenti ne' motore: deve poter girare senza argomenti, e PRIMA
# del blocco che li pretende. Un test di una funzione pura non si fa cadere da un'installazione.
if "--self-test" in sys.argv:
    _casi = _self_test_locale() + regola.self_test()
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
# quel comando cancella il non committato (`#2406`). Un audit che si ferma perche' un percorso e'
# sbagliato avrebbe gia' distrutto le modifiche locali all'header — cioe' avrebbe fatto danno
# proprio nel caso in cui ha deciso di non misurare.
if not regola.preflight_rapido(print):
    sys.exit(2)

# 🔴 **E la guardia sul non committato viene prima del ripristino.** `git checkout --` non
# distingue la mutazione di una corsa interrotta dal lavoro di qualcuno: cancella entrambi. Il
# gate gemello si ferma in questo caso da sempre (`sporco()`), questo lo faceva soltanto DOPO
# aver gia' scritto. ⚠️ Chi vuole ripartire da una corsa interrotta ripristina lui l'header e
# rilancia: e' un comando in piu' contro un lavoro perso. (`#2672`)
_stato = subprocess.run(["git", "status", "--porcelain", "--", H],
                        cwd=RADICE, capture_output=True, text=True, errors="replace")
if _stato.returncode != 0:
    print("\n⛔ FERMO: `git status` non risponde (exit %d). Non e' «albero pulito»:\n"
          "   senza saperlo, il ripristino qui sotto cancellerebbe alla cieca." % _stato.returncode)
    sys.exit(2)
if (_stato.stdout or "").strip():
    print("\n⛔ FERMO: `%s` ha modifiche non committate.\n"
          "   L'audit ripristina quel file con `git checkout --`, che le cancellerebbe.\n"
          "   Committarle, metterle da parte, oppure ripristinarlo a mano se sono i resti\n"
          "   di una corsa interrotta." % os.path.relpath(H, RADICE).replace("\\", "/"))
    sys.exit(2)

# 🔑 Ripristino PRIMA di leggere la base: se una corsa precedente e' stata interrotta, l'header sul disco
# porta ancora la sua mutazione, e senza questa riga diventerebbe la base di tutto l'audit.
# ⚠️ L'exit code si legge: un `.git/index.lock` altrui fa fallire il checkout, e l'audit
# partirebbe da un header ancora mutato credendolo pulito.
_H_REL = os.path.relpath(H, RADICE).replace("\\", "/")   # un solo modo di dire dov'e' l'header
_ripristino = subprocess.run(["git", "checkout", "--", _H_REL],
                             cwd=RADICE, capture_output=True, text=True, errors="replace")
if _ripristino.returncode != 0:
    print("\n⛔ FERMO: il ripristino iniziale e' fallito (exit %d): %s\n"
          "   La baseline sarebbe presa su un header che non e' quello di `HEAD`."
          % (_ripristino.returncode, (_ripristino.stderr or "").strip()[:120]))
    sys.exit(2)
BYTE_ORIGINALI = io.open(H, "rb").read()          # i byte veri, per un ripristino identico
ORIGINALE = BYTE_ORIGINALI.decode("utf-8")

TUTTE = dichiarazioni(ORIGINALE)
COST = [c for c in TUTTE if not resto or c[0] in resto]
ignoti = [n for n in resto if n not in [c[0] for c in TUTTE]]
if ignoti:
    print("NOMI SCONOSCIUTI (nessuna dichiarazione corrisponde): " + ", ".join(ignoti))
    sys.exit(2)
if not COST:
    print("nessuna costante da misurare")
    sys.exit(2)

# 🔑 L'attesa del motore per ULTIMA, quando ogni rifiuto istantaneo e' gia' escluso: nomi
# sconosciuti, header sporco e percorsi cablati si sanno in millisecondi, e farli aspettare
# un'ora e mezza prima di dire «no» non misura niente e lo fa costando un'ora e mezza.
if not regola.attesa_motore(print):
    sys.exit(2)


def ripristina():
    scrivi(BYTE_ORIGINALI)


def build(tentativi=40):
    """Ricostruisce, e distingue la contesa dall'errore di compilazione. Vedi `misura.build`.

    ⚠️ Ri-derivava il path del progetto pur avendo `UPROJECT` due righe sopra: due modi di
    dire la stessa cosa in un file solo. Ora l'invocazione e' una, in `misura.py`."""
    return regola.build(tentativi, stampa=print)


def suite():
    """Misura. Torna (verdetto, esito, rossi).

    Motore, istantanee e regola stanno in `misura.py`, in una sede sola: erano in copia qui
    e in `caduta-gate.py`, e le due copie avevano gia' iniziato a divergere.

    Il quarto termine — nessun processo estraneo del motore DURANTE la run — e' osservato
    campionando i processi mentre la suite gira e scartando quelli che discendono dal
    proprio (`#2672`). ⚠️ Il campionamento e' discreto: copre la finestra lunga, non
    l'istante."""
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

# 🔴 **Il ripristino finale deve avvenire anche se qualcosa esplode.** Il `finally` per
# iterazione rimette a posto il SORGENTE, ma l'ultimo binario costruito resta quello MUTATO:
# un'eccezione qualsiasi — `OSError` da `istantanea`, un `KeyboardInterrupt`, un errore di
# scrittura — saltava la ricostruzione finale, e la prossima suite di chiunque in questo
# checkout avrebbe misurato una mutazione **invisibile nel diff**. E' l'esito che la sezione
# «⛔ Il rischio, dichiarato» di questo file chiama il peggiore. Il gate gemello ha sempre
# avuto questa rete; qui mancava. (`#2672`)
_AUDIT_CONCLUSO = False

# 🔴 **E la rete scatta solo se qualcosa e' stato davvero mutato.** Senza questa guardia
# partiva anche sulle uscite in cui nulla era stato scritto — `BASELINE NON MISURABILE`, che
# esce dopo aver misurato l'header intatto — stampando «il binario e' MUTATO» su un binario
# sano e avviando un build da mezz'ora. Il gate gemello lo dice in una riga: *i falsi allarmi
# sono il modo in cui un avviso vero smette di essere letto*.
_COSTRUITO_MUTATO = False


@atexit.register
def _rete_di_sicurezza():
    if _AUDIT_CONCLUSO or not _COSTRUITO_MUTATO:
        return
    print("\n⚠️ AUDIT INTERROTTO: ripristino il sorgente e ricostruisco, perche' l'ultimo\n"
          "   binario prodotto e' quello MUTATO e non si vede nel diff.")
    try:
        ripristina()
    except Exception as e:                                  # il ripristino non deve mai mancare
        print("   ⛔ ripristino del sorgente FALLITO (%s): rimetterlo a mano." % e)
        return
    if not build():
        print("   ⛔ BINARIO MUTATO SUL DISCO: ricostruire PRIMA di qualunque altra misura.")


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
            byte_mutati = muta(BYTE_ORIGINALI, nome, val, nuovo)
            if byte_mutati is None:
                # 🔴 Una scrittura no-op farebbe scattare l'allarme piu' forte su codice NON mutato.
                f.write("## %s = %s -> %s\n**NON MISURATA**: la sostituzione non ha agganciato la "
                        "dichiarazione (spaziatura diversa?).\n\n" % (nome, val, nuovo))
                sospese.append(nome); f.flush(); continue
            scrivi(byte_mutati)
            _COSTRUITO_MUTATO = True     # da qui in poi il binario puo' portare la mutazione
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
_AUDIT_CONCLUSO = True
