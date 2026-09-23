#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Il gate di `docs/roadmap/bot-competence.yaml`: ogni riga risolve, o non e' evidenza.

    python tools/bot-competence/check.py --check
    python tools/bot-competence/check.py --check --yaml <percorso>
    python tools/bot-competence/check.py --autotest     # le funzioni pure, senza leggere l'albero

Perche' esiste
--------------
`D-102` chiede che ogni voce di competenza sia legata a **uno scenario o a un test reale, mai a un
giudizio** — e `#543` aggiunge la condizione che rende la richiesta eseguibile: *«un validator lo
verifica, non una review»*. Una review guarda una riga alla volta e si stanca; un token che smette di
risolvere perche' qualcuno ha rinominato un test non lo vede nessuno, e lo schema diventa un documento
che sembra corrente.

Il precedente e' misurato e recente: `RefactorTactics.Heroes.Phase.*` e' tuttora il nome LETTERALE dei
test di `Hero.Muiren`. Uno schema che derivasse il nome del test dall'id dell'eroe nascerebbe rosso su
codice sano — ed e' la ragione per cui qui l'evidenza e' un token letterale e il gate lo confronta coi
letterali veri.

Cosa NON copre
--------------
⛔ **Non dice che i test siano VERDI.** Dice che esistono. E' la stessa distinzione che
`tools/asset-provenance/check.ts` dichiara per se': un verde significa «registrato», mai «consentito».
Che un token passi lo dice la suite, e la data dell'ultima run sta in `meta.suite` dello schema.

⛔ **Non giudica lo STATO.** Che una riga meriti `PASS` invece di `PARTIAL` e' una valutazione umana, e
nessun comando la puo' produrre: dipende da cosa quel test esercita davvero. Il gate verifica la FORMA
— che uno stato diverso da `UNTESTED` porti evidenza, e che l'evidenza risolva — non il merito.

⛔ **Non verifica il vocabolario contro una fonte esterna**, perche' non ne esiste una: le dieci
capability vengono dalla provenienza dichiarata da `#543` e vivono nello schema stesso. Cio' che il gate
impedisce e' che una riga ne inventi un'undicesima in silenzio.

⛔ **Sui CONSUMATORI verifica che l'annotazione esista, non che dica il vero.** Che un documento citi
`bot-competence.yaml` non prova che gli stati riportati accanto al numero siano quelli correnti: e' un
controllo di presenza, e serve a impedire che l'annotazione sparisca in silenzio. Chi cambia uno stato
rilegge i consumatori. E non riconosce una conclusione di bilanciamento NUOVA scritta altrove: nessun
comando legge l'intenzione di una frase, e chi pubblica una metrica bot-contro-bot in un file nuovo lo
aggiunge a `consumers` dello schema.

⚠️ **Non legge la rete e non legge GitHub.** Funziona offline, come ogni radar di questo repository
tranne `issue-refs.ts`.
"""
from __future__ import annotations

import argparse
import os
import re
import sys

try:
    import yaml
except ImportError:  # pragma: no cover - dipendenza dichiarata nel docstring
    print("Serve pyyaml: pip install pyyaml", file=sys.stderr)
    raise

# La console Windows di questo repository e' `cp1252` e un marcatore la fa esplodere a meta' referto —
# cioe' proprio quando il gate ha qualcosa da dire. Si riconfigura una volta, con `errors="replace"`:
# un carattere che non passa diventa un punto interrogativo, non una `UnicodeEncodeError`.
for _flusso in (sys.stdout, sys.stderr):
    try:
        _flusso.reconfigure(encoding="utf-8", errors="replace")
    except (AttributeError, ValueError):  # pragma: no cover — flusso gia' avvolto, o non riconfigurabile
        pass

STATI = ("PASS", "PARTIAL", "FAIL", "UNTESTED")

RADICE = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
YAML_DEFAULT = os.path.join(RADICE, "docs", "roadmap", "bot-competence.yaml")


class SchemaError(Exception):
    """Una riga che non rispetta la forma. Si rifiuta col nome del difetto."""


# ---------------------------------------------------------------------------------------------------
# Le funzioni PURE: decidono, e non leggono il disco. `--autotest` le prova senza un albero.
# ---------------------------------------------------------------------------------------------------

def chiave(riga: dict) -> str:
    return f"{riga.get('hero')} x {riga.get('capability')}"


def valida_forma(schema: dict, eroi: set[str]) -> list[str]:
    """I difetti di FORMA, uno per riga. Lista vuota = forma sana.

    ⚠️ Non guarda l'albero: un token qui puo' essere ben formato e non risolvere. Le due domande sono
    separate apposta — la forma si puo' provare senza un repository, la risoluzione no.
    """
    difetti: list[str] = []

    grezzo = [v.get("name") for v in schema.get("vocabulary") or []]
    if not grezzo:
        difetti.append("`vocabulary` assente o vuoto: senza un vocabolario nessuna riga e' verificabile")
    # ⚠️ **Le voci senza `name` escono dal vocabolario, non solo dall'insieme dei nomi noti.** Tenerle
    # dentro faceva nascere una capability chiamata `None`, e con lei quattro difetti fantasma: due righe
    # `Hero.X x None` «assenti» e una voce di vocabolario «che nessuna riga usa». Un difetto vero che ne
    # genera tre falsi manda a cercare nel posto sbagliato — e l'ha trovato l'`--autotest`.
    vocabolario = [v for v in grezzo if v]
    if len(vocabolario) != len(grezzo):
        difetti.append("`vocabulary` contiene una voce senza `name`: sarebbe una capability anonima")
    noti = set(vocabolario)

    # 🔴 **`consumers` vuoto disarma meta' del gate, e disarmarlo dev'essere ROSSO.** La prima stesura lo
    # trattava come informazione: svuotando il blocco, `check.py` usciva 0 stampando «consumatori
    # sorvegliati: NESSUNO» — cioe' nominava il difetto che `#543` esiste per chiudere e lo lasciava
    # passare. Un gate che descrive la propria disattivazione senza fallire e' peggio di un gate assente,
    # perche' il referto continua a sembrare un verde.
    if not (schema.get("consumers") or []):
        difetti.append("`consumers` assente o vuoto: nessun documento e' sorvegliato, e una conclusione di "
                       "bilanciamento torna producibile senza lo stato di competenza accanto")

    viste: set[tuple] = set()
    for riga in schema.get("competence") or []:
        h, c = riga.get("hero"), riga.get("capability")
        if h not in eroi:
            difetti.append(f"{chiave(riga)}: eroe fuori dal roster ({sorted(eroi)})")
        if c not in noti:
            difetti.append(f"{chiave(riga)}: capability fuori dal vocabolario dello schema")
        if (h, c) in viste:
            difetti.append(f"{chiave(riga)}: coppia duplicata — due righe per lo stesso fatto divergono")
        viste.add((h, c))

        stato = riga.get("state")
        if stato not in STATI:
            difetti.append(f"{chiave(riga)}: stato '{stato}' non e' fra {STATI}")

        ev = riga.get("evidence") or []
        if not isinstance(ev, list):
            difetti.append(f"{chiave(riga)}: `evidence` deve essere una lista")
            ev = []

        applicabile = riga.get("applicable", True)
        if applicabile is False:
            # La domanda non si pone: il ROSTER non da' la cosa a questo eroe. Va motivato, o diventa
            # il modo piu' comodo di far sparire una riga scomoda.
            if not (riga.get("motivo") or "").strip():
                difetti.append(f"{chiave(riga)}: `applicable: false` senza `motivo`")
            if stato != "UNTESTED" or ev:
                difetti.append(
                    f"{chiave(riga)}: `applicable: false` vuole `state: UNTESTED` e `evidence` vuota — "
                    "se c'e' evidenza, la domanda si poneva")
        elif stato == "UNTESTED":
            if ev:
                difetti.append(f"{chiave(riga)}: `UNTESTED` con evidenza — allora non e' non testata")
        elif not ev:
            difetti.append(f"{chiave(riga)}: stato '{stato}' senza evidenza e' un giudizio, non un fatto")

        for t in ev:
            if not re.fullmatch(r"(test|scenario):\S+", str(t)):
                difetti.append(f"{chiave(riga)}: token malformato '{t}' (atteso `test:<nome>` o `scenario:<id>`)")

        if not (riga.get("why") or "").strip():
            difetti.append(f"{chiave(riga)}: `why` vuoto — uno stato senza ragione non e' contestabile")

    # La griglia deve essere PIENA: `UNTESTED` e' il default e si scrive. Una coppia assente si legge
    # come «non si applica», che e' un'altra cosa e ha un campo suo.
    for h in sorted(eroi):
        for c in vocabolario:
            if (h, c) not in viste:
                difetti.append(f"{h} x {c}: riga ASSENTE — `UNTESTED` si scrive, non si omette")

    # Una capability che non compare per nessun eroe e' una voce di vocabolario morta.
    for c in vocabolario:
        if not any(k[1] == c for k in viste):
            difetti.append(f"capability '{c}': nessuna riga la usa")

    return difetti


def separa(evidenze: set[str]) -> tuple[set[str], set[str]]:
    """I token in due insiemi per tipo. Sconosciuti gia' scartati da `valida_forma`."""
    test = {t[len("test:"):] for t in evidenze if t.startswith("test:")}
    scen = {t[len("scenario:"):] for t in evidenze if t.startswith("scenario:")}
    return test, scen


# ---------------------------------------------------------------------------------------------------
# Le tre sorgenti, lette dall'albero
# ---------------------------------------------------------------------------------------------------

def corpo_di(testo: str, firma: re.Pattern) -> str:
    """Il corpo di una funzione C++, delimitato contando le graffe BILANCIATE.

    🔴 **Non con `\\{(.*?)\\}`, ed e' il difetto che questa funzione ha avuto.** Quel regex non-greedy si
    ferma alla PRIMA graffa chiusa: basta un inizializzatore, un lambda o un blocco annidato prima della
    lista perche' il corpo letto sia un moncone — e la conseguenza sarebbe silenziosa, perche' un roster
    piu' corto produce «riga ASSENTE» su eroi veri oppure, peggio, non produce niente su un eroe nuovo.
    """
    m = firma.search(testo)
    if not m:
        return ""
    i = testo.find("{", m.end() - 1)
    if i < 0:
        return ""
    livello, j = 0, i
    while j < len(testo):
        if testo[j] == "{":
            livello += 1
        elif testo[j] == "}":
            livello -= 1
            if livello == 0:
                return testo[i + 1:j]
        j += 1
    return ""


def senza_commenti(testo: str) -> str:
    """Via i commenti, perche' un `TEXT("Hero.…")` citato in una nota non e' un eroe del roster."""
    testo = re.sub(r"/\*.*?\*/", " ", testo, flags=re.S)
    return re.sub(r"//[^\n]*", " ", testo)


def eroi_dal_catalogo(radice: str) -> set[str]:
    """Gli id del roster da `URTHeroCatalogLibrary::GetHeroIds`, non da un elenco riscritto qui."""
    p = os.path.join(radice, "Source", "RefactorTactics", "Ability", "RTHeroCatalogLibrary.cpp")
    with open(p, encoding="utf-8", errors="replace") as f:
        testo = f.read()
    corpo = corpo_di(testo, re.compile(r"GetHeroIds\s*\(\s*\)\s*"))
    if not corpo:
        raise SchemaError(f"GetHeroIds non trovata in {p}: senza roster non si valida niente")
    eroi = set(re.findall(r'TEXT\("(Hero\.[A-Za-z0-9_]+)"\)', senza_commenti(corpo)))
    if not eroi:
        raise SchemaError(
            f"GetHeroIds trovata in {p} ma non dichiara nessun `Hero.*`: un roster vuoto renderebbe la "
            "griglia vuota, e il gate verde su zero righe")
    return eroi


# Il nome Automation si prende dalla MACRO che lo DICHIARA, non da una stringa qualunque fra virgolette.
# 🔴 La prima stesura cercava `"(RefactorTactics\.[A-Za-z0-9_.]+)"` ovunque nel file, quindi un nome
# citato in un COMMENTO risolveva come se il test esistesse — e un test rinominato, il cui nome vecchio
# sopravvive in una nota storica, avrebbe tenuto in piedi un token morto. Misurato sull'albero: la forma
# larga da' 2627 nomi, questa 2626, e tutti i token dello schema continuano a risolvere. La differenza e'
# esattamente cio' che non doveva contare.
MACRO_TEST = re.compile(
    r'IMPLEMENT_[A-Z_]*AUTOMATION_TEST\s*\(\s*\w+\s*,\s*"(RefactorTactics\.[\w.]+)"', re.S)


def test_dichiarati(radice: str) -> set[str]:
    """I nomi Automation DICHIARATI da un `IMPLEMENT_*_AUTOMATION_TEST`, nei due moduli."""
    nomi: set[str] = set()
    for sotto in (
        os.path.join("Source", "RefactorTactics", "Tests"),
        os.path.join("Source", "RefactorTacticsEditor", "Private", "Tests"),
    ):
        base = os.path.join(radice, sotto)
        if not os.path.isdir(base):
            continue
        for dirpath, _dirs, files in os.walk(base):
            for nome in files:
                if not nome.endswith((".cpp", ".h")):
                    continue
                with open(os.path.join(dirpath, nome), encoding="utf-8", errors="replace") as f:
                    nomi.update(MACRO_TEST.findall(f.read()))
    return nomi


def scenari_dichiarati(radice: str) -> set[str]:
    """Gli `scenarioId` del corpus. Lettura testuale: un JSON malformato non deve fermare il gate."""
    ids: set[str] = set()
    base = os.path.join(radice, "Scenarios")
    if not os.path.isdir(base):
        return ids
    for dirpath, _dirs, files in os.walk(base):
        for nome in files:
            if not nome.endswith(".json"):
                continue
            with open(os.path.join(dirpath, nome), encoding="utf-8", errors="replace") as f:
                ids.update(re.findall(r'"scenarioId"\s*:\s*"([^"]+)"', f.read()))
    return ids


# ---------------------------------------------------------------------------------------------------

def esegui(percorso: str, radice: str) -> int:
    with open(percorso, encoding="utf-8") as f:
        schema = yaml.safe_load(f)

    eroi = eroi_dal_catalogo(radice)
    difetti = valida_forma(schema, eroi)

    righe = schema.get("competence") or []
    tutte: set[str] = set()
    for r in righe:
        tutte.update(str(t) for t in (r.get("evidence") or []))
    nomi_test, ids_scenario = separa(tutte)

    dichiarati = test_dichiarati(radice)
    corpus = scenari_dichiarati(radice)

    # I CONSUMATORI: chi pubblica una metrica prodotta da partite automatiche deve portarsi accanto lo
    # stato di competenza che la determina. La lista la dichiara lo schema; qui si verifica soltanto che
    # l'annotazione ci sia ancora — se sparisce, la conclusione torna producibile senza.
    #
    # ⚠️ Si cerca l'ANCORA dichiarata dallo schema, non il nome del file: un documento puo' nominare
    # `bot-competence.yaml` in una nota storica a mille righe dall'annotazione, e il gate sarebbe verde su
    # un consumatore che la metrica la pubblica senza competenza accanto. L'ancora e' la prima riga
    # dell'annotazione stessa, quindi trovarla significa che l'annotazione c'e'.
    # ⛔ Resta un controllo di PRESENZA: non dice che gli stati citati siano correnti.
    ancora = (schema.get("ancora") or "").strip()
    nome_schema = os.path.basename(percorso)
    consumatori_rotti: list[str] = []
    if not ancora:
        consumatori_rotti.append("`ancora` assente: senza, il controllo dei consumatori cercherebbe il solo "
                                 "nome del file e passerebbe su una citazione qualunque")
    for rel in schema.get("consumers") or []:
        assoluto = os.path.join(radice, rel)
        if not os.path.isfile(assoluto):
            consumatori_rotti.append(f"{rel}: dichiarato consumatore, ma il file non esiste")
            continue
        with open(assoluto, encoding="utf-8", errors="replace") as f:
            testo_consumatore = f.read()
        if ancora and ancora not in testo_consumatore:
            consumatori_rotti.append(
                f"{rel}: pubblica una metrica da partite automatiche e non porta l'annotazione "
                f"(ancora non trovata: «{ancora[:48]}…»)")
        elif nome_schema not in testo_consumatore:
            consumatori_rotti.append(f"{rel}: porta l'annotazione ma non rimanda a `{nome_schema}`")

    rotti: list[str] = []
    for r in righe:
        for t in (r.get("evidence") or []):
            t = str(t)
            if t.startswith("test:") and t[5:] not in dichiarati:
                rotti.append(f"{chiave(r)}: {t} non e' dichiarato da nessun IMPLEMENT_*_AUTOMATION_TEST")
            elif t.startswith("scenario:") and t[9:] not in corpus:
                rotti.append(f"{chiave(r)}: {t} non e' uno `scenarioId` del corpus")

    # LA RIGA DI COPERTURA, stampata SEMPRE — anche quando tutto e' verde, e col comando per ricontarla.
    # Un gate che tace quando passa non dice mai quanto ha guardato, ed e' il modo in cui uno scope che si
    # restringe resta verde: e' il difetto che `un gate stampa i fallimenti, non lo scope` descrive.
    conteggio = {s: sum(1 for r in righe if r.get("state") == s) for s in STATI}
    non_applicabili = [r for r in righe if r.get("applicable") is False]
    # `relpath` alza `ValueError` fra due unita' disco diverse su Windows. E' una ETICHETTA: non deve
    # poter uccidere il gate proprio quando lo si punta a un file fuori dal repository — che e' cio' che
    # fa chi verifica una mutazione su una copia.
    try:
        etichetta = os.path.relpath(percorso, radice)
    except ValueError:
        etichetta = percorso
    print(f"bot-competence: {len(righe)} righe lette da {etichetta} "
          f"({len(eroi)} eroi x {len(schema.get('vocabulary') or [])} capability)")
    print(f"  stati: " + " · ".join(f"{s} {conteggio[s]}" for s in STATI))
    print(f"  evidenza: {len(nomi_test)} token `test:` e {len(ids_scenario)} token `scenario:` distinti, "
          f"confrontati con {len(dichiarati)} nomi Automation e {len(corpus)} scenari dell'albero")
    consumatori = schema.get("consumers") or []
    print("  consumatori sorvegliati: " + (", ".join(consumatori) if consumatori else
          "NESSUNO — lo schema non lo legge nessun documento, che e' il difetto che `#543` chiude"))

    # INFORMAZIONE, non difetto: una riga esentata dal roster e' un'attesa legittima, e va STAMPATA invece
    # che nascosta — e' la stessa asimmetria che gli altri radar dichiarano.
    for r in non_applicabili:
        print(f"  ℹ️  {chiave(r)}: la domanda non si pone — {r.get('motivo', '').strip()[:120]}")

    if difetti or rotti or consumatori_rotti:
        print()
        for d in difetti:
            print(f"  ❌ FORMA        {d}")
        for d in rotti:
            print(f"  ❌ RISOLUZIONE  {d}")
        for d in consumatori_rotti:
            print(f"  ❌ CONSUMATORE  {d}")
        n = len(difetti) + len(rotti) + len(consumatori_rotti)
        print(f"\n{n} difetti. Un'annotazione che sparisce e un token che non risolve sono lo stesso "
              "difetto: qualcosa che sembra sorvegliato e non lo e'.")
        return 1

    print("\n✅ ogni riga ha la sua forma e ogni token risolve. "
          "⚠️ Questo NON dice che i test siano verdi: vedi `meta.suite` dello schema.")
    return 0


def autotest() -> int:
    """Le funzioni pure, provate senza albero. Dimostra che il gate SA fallire (`D-188`).

    ⚠️ **Prova `valida_forma`, non il gate intero**: la risoluzione dei token e il controllo dei
    consumatori leggono l'albero, e una prova che finge un albero proverebbe la finzione. Quelle due
    meta' si verificano per mutazione sul repository vero, e il modo sta nel corpo della PR di `#543`.
    """
    eroi = {"Hero.A", "Hero.B"}
    voc = [{"name": "Uno"}]
    consumatori = ["docs/roadmap/qualcosa.md"]

    def schema(righe, *, vocabolario=None, cons=None):
        return {"vocabulary": voc if vocabolario is None else vocabolario,
                "competence": righe,
                "consumers": consumatori if cons is None else cons}

    sano = [
        {"hero": "Hero.A", "capability": "Uno", "state": "PASS",
         "evidence": ["test:RefactorTactics.X.Y"], "why": "perche' si'"},
        {"hero": "Hero.B", "capability": "Uno", "state": "UNTESTED", "evidence": [], "why": "nessuna misura"},
    ]
    casi = [
        ("forma sana", schema(sano), 0),
        ("stato senza evidenza", schema([dict(sano[0], evidence=[]), sano[1]]), 1),
        ("UNTESTED con evidenza", schema([sano[0], dict(sano[1], evidence=["test:RefactorTactics.X.Y"])]), 1),
        ("token malformato", schema([dict(sano[0], evidence=["RefactorTactics.X.Y"]), sano[1]]), 1),
        ("stato ignoto", schema([dict(sano[0], state="FORSE"), sano[1]]), 1),
        ("riga assente", schema([sano[0]]), 1),
        # UNO e non due: la riga in piu' e' ben formata, e la griglia resta completa. Scrivere `2` qui
        # era la mia attesa sbagliata, e l'autotest l'ha presa — che e' il suo mestiere.
        ("coppia duplicata", schema(sano + [dict(sano[1], hero="Hero.A", capability="Uno")]), 1),
        ("why vuoto", schema([dict(sano[0], why="  "), sano[1]]), 1),
        ("non applicabile senza motivo", schema([sano[0], dict(sano[1], applicable=False)]), 1),
        ("non applicabile con evidenza", schema([sano[0],
            dict(sano[1], applicable=False, motivo="il roster non gliela da'",
                 evidence=["test:RefactorTactics.X.Y"])]), 1),
        ("non applicabile ben formato", schema([sano[0],
            dict(sano[1], applicable=False, motivo="il roster non gliela da'")]), 0),
        # I rami che la code review ha trovato scoperti. Senza questi, `--autotest` copriva nove difetti
        # su quattordici e i due piu' pesanti — roster e vocabolario — non li provava nessuno.
        # TRE e non uno: senza vocabolario le due righe hanno una capability che non e' fra i noti. Era
        # un'altra mia attesa sbagliata, e l'autotest l'ha presa.
        ("vocabolario vuoto", schema(sano, vocabolario=[]), 3),
        ("voce di vocabolario senza name", schema(sano, vocabolario=[{"name": "Uno"}, {}]), 1),
        ("eroe fuori dal roster",
         schema([dict(sano[0], hero="Hero.Z"), sano[1]]), 2),  # fuori roster + `Hero.A x Uno` assente
        ("capability ignota",
         schema([dict(sano[0], capability="Due"), sano[1]]), 2),  # fuori vocabolario + `Hero.A x Uno` assente
        ("evidence non e' una lista",
         schema([dict(sano[0], evidence="test:RefactorTactics.X.Y"), sano[1]]), 2),  # tipo + stato senza evidenza
        ("consumers vuoto", schema(sano, cons=[]), 1),
        ("consumers assente", {"vocabulary": voc, "competence": sano}, 1),
    ]
    rossi = 0
    for nome, sch, attesi in casi:
        d = valida_forma(sch, eroi)
        esito = "ok" if len(d) == attesi else f"ATTESI {attesi}, TROVATI {len(d)}"
        if len(d) != attesi:
            rossi += 1
            for x in d:
                print(f"      {x}")
        print(f"  {'✅' if len(d) == attesi else '❌'} {nome}: {esito}")
    print()
    # 🔴 Il conteggio lo stampa il COMANDO, e non si scrive a mano da nessuna parte: fino al
    # 2026-09-23 questa riga diceva solo "verde", e il numero dei casi e' stato scritto a mano in
    # quattro sedi — `AGENTS.md`, il Decision Log, un corpo di PR e un commento — sbagliato in
    # tutte e quattro (19 contro 18). Un numero che nessun comando emette invecchia in silenzio.
    print(f"✅ autotest verde — {len(casi)} casi" if rossi == 0 else f"❌ {rossi} casi falliti su {len(casi)}")
    return 1 if rossi else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true", help="valida lo schema contro l'albero")
    ap.add_argument("--autotest", action="store_true", help="prova le funzioni pure, senza albero")
    ap.add_argument("--yaml", default=YAML_DEFAULT)
    ap.add_argument("--radice", default=RADICE)
    a = ap.parse_args()

    if a.autotest:
        return autotest()
    if not a.check:
        ap.print_help()
        return 0
    try:
        return esegui(a.yaml, a.radice)
    except (SchemaError, FileNotFoundError) as e:
        print(f"❌ {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
