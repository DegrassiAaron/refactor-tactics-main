# Sedute componibili — piano d'implementazione, fetta 0

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** costruire i mattoni delle sedute e il generatore che li legge, **accanto** al registro attuale e senza cancellarlo, fino a poter confrontare le aperture calcolate con le sedute scritte a mano.

**Architecture:** cinque moduli Python con una responsabilità ciascuno — il registro dei mattoni, lo stato letto dal suo owner, gli oracoli dei prerequisiti, il calcolo dell'ordine del giorno, e i due eseguibili da riga di comando. Nessun modulo conosce l'esito di un check: quello appartiene a `docs/technical/test-manuali-pie.md`. Il confronto col vecchio registro è l'ultimo passo, ed è il gate che decide se il modello regge.

**Tech Stack:** Python 3.12.10 · pyyaml 6.0.3 · `unittest` della standard library · `gh` per la cache GitHub. Nessuna dipendenza da installare.

**Spec:** [`sedute-componibili-design-2026-09-11.md`](sedute-componibili-design-2026-09-11.md)

## Global Constraints

Ogni task eredita questi vincoli.

- **`docs/technical/test-manuali-pie.md` resta l'unico owner dell'esito di un check.** Nessuna struttura dati di questo piano ha un campo dove scrivere un esito atteso o raggiunto. Se ti trovi ad aggiungerlo, stai scrivendo nel file sbagliato.
- **Fino alla fetta 5, `docs/roadmap/editor-sessions.yaml` resta l'owner delle sedute.** Il file nuovo porta in testa il marcatore `DRAFT — NON OWNER`. Nessun task di questo piano cancella, modifica o sposta il vecchio registro.
- **Rifiutare, non ignorare.** Un difetto del registro esce con un errore che nomina il difetto, sul precedente di `D-182` (`sys.exit(2)` con un messaggio, invece di lasciar cadere l'argomento). Un input malformato che produce un verde è il difetto peggiore di questa famiglia di strumenti.
- **Un `requires` non soddisfatto non è un errore.** È un bloccante: si stampa, col suo motivo. Farlo sparire è il difetto che questo lavoro esiste per rimuovere.
- **`build/` non si committa** (`.gitignore:317`).
- **Niente totali volatili** in ciò che lo strumento scrive e in ciò che scrivi tu — issue, commit, README (`AGENTS.md` §14). Si elencano gli **ID**. Un numero è ammesso solo come esito del passaggio corrente, col comando accanto.
- **Nessun rename d'eroe.** `D-321` ha differito la migrazione a `#2297`. I `requires` di tipo `asset` usano i nomi **dei file che esistono** — `BP_Unit_Aevik` — perché sono un fatto di filesystem.
- **Determinismo dell'output.** Ogni ordinamento è totale: a parità di chiave si ordina per ID. Due esecuzioni sugli stessi input producono byte identici.

---

## File Structure

| File | Responsabilità |
|---|---|
| `docs/roadmap/sedute-mattoni.yaml` | i dati: `setups`, `wiring`. Marcato `DRAFT — NON OWNER` |
| `tools/editor-sessions/registry.py` | legge i mattoni, rifiuta i difetti, risolve `extends` |
| `tools/editor-sessions/registry_test.py` | test del sopra |
| `tools/editor-sessions/pie_status.py` | lo stato di ogni voce PIE, letto dal registro che lo possiede |
| `tools/editor-sessions/pie_status_test.py` | test del sopra |
| `tools/editor-sessions/oracles.py` | un `requires` è soddisfatto o no, e chi lo dice |
| `tools/editor-sessions/oracles_test.py` | test del sopra |
| `tools/editor-sessions/agenda.py` | il calcolo: scarta i verdi, raggruppa, ordina |
| `tools/editor-sessions/agenda_test.py` | test del sopra |
| `tools/editor-sessions/build_agenda.py` | CLI: scrive `build/ordine-del-giorno.md` |
| `tools/editor-sessions/seed_wiring.py` | CLI: semina la bozza del wiring dal vecchio registro |
| `tools/editor-sessions/compare_legacy.py` | CLI: il gate della fetta 0 |
| `tools/editor-sessions/README.md` | come si lancia, cosa produce, cosa non fa |

Comando dei test, valido da ogni task in poi:

```bash
python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v
```

---

## Task 1: il registro dei mattoni

**Files:**
- Create: `tools/editor-sessions/registry.py`
- Test: `tools/editor-sessions/registry_test.py`

**Interfaces:**
- Consumes: niente.
- Produces: `RegistryError` · `Setup(id: str, extends: str | None, fields: dict)` · `Wire(check: str, setup: str, requires: tuple[str, ...], issue: int | None)` · `load(path: Path) -> tuple[dict[str, Setup], list[Wire]]` · `root_of(setup_id: str, setups: dict[str, Setup]) -> str`.

- [ ] **Step 1: scrivi il test che fallisce**

```python
# tools/editor-sessions/registry_test.py
"""Il registro accetta i mattoni corretti e rifiuta i difetti per nome."""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import registry


def scrivi(testo: str) -> Path:
    f = tempfile.NamedTemporaryFile("w", suffix=".yaml", delete=False, encoding="utf-8")
    f.write(testo)
    f.close()
    return Path(f.name)


BUONO = """
setups:
  - id: SET-HEX-MATCH
    map: L_HexArena
  - id: SET-HEX-TURN
    extends: SET-HEX-MATCH
    given: "un turno pianificato e risolto"
wiring:
  - check: PIE-HEXPLAY-4
    setup: SET-HEX-TURN
    issue: 33
  - check: PIE-V01-LOG
    setup: SET-HEX-MATCH
    requires: [ "mount:WBP_RT_EventLogRight#2697" ]
"""


class LetturaTest(unittest.TestCase):
    def test_legge_setup_e_wiring(self):
        setups, wires = registry.load(scrivi(BUONO))
        self.assertEqual(sorted(setups), ["SET-HEX-MATCH", "SET-HEX-TURN"])
        self.assertEqual(setups["SET-HEX-TURN"].extends, "SET-HEX-MATCH")
        self.assertEqual(setups["SET-HEX-MATCH"].fields["map"], "L_HexArena")
        self.assertEqual([w.check for w in wires], ["PIE-HEXPLAY-4", "PIE-V01-LOG"])
        self.assertEqual(wires[1].requires, ("mount:WBP_RT_EventLogRight#2697",))
        self.assertEqual(wires[0].issue, 33)
        self.assertIsNone(wires[1].issue)

    def test_extends_risale_alla_radice(self):
        setups, _ = registry.load(scrivi(BUONO))
        self.assertEqual(registry.root_of("SET-HEX-TURN", setups), "SET-HEX-MATCH")
        self.assertEqual(registry.root_of("SET-HEX-MATCH", setups), "SET-HEX-MATCH")


class RifiutoTest(unittest.TestCase):
    def test_setup_inesistente_nomina_il_difetto(self):
        testo = "setups:\n  - id: SET-A\nwiring:\n  - check: PIE-X\n    setup: SET-FANTASMA\n"
        with self.assertRaises(registry.RegistryError) as e:
            registry.load(scrivi(testo))
        self.assertIn("SET-FANTASMA", str(e.exception))
        self.assertIn("PIE-X", str(e.exception))

    def test_check_rivendicato_due_volte(self):
        testo = (
            "setups:\n  - id: SET-A\n  - id: SET-B\n"
            "wiring:\n  - check: PIE-X\n    setup: SET-A\n  - check: PIE-X\n    setup: SET-B\n"
        )
        with self.assertRaises(registry.RegistryError) as e:
            registry.load(scrivi(testo))
        self.assertIn("PIE-X", str(e.exception))

    def test_setup_duplicato(self):
        with self.assertRaises(registry.RegistryError):
            registry.load(scrivi("setups:\n  - id: SET-A\n  - id: SET-A\n"))

    def test_extends_verso_il_nulla(self):
        with self.assertRaises(registry.RegistryError) as e:
            registry.load(scrivi("setups:\n  - id: SET-A\n    extends: SET-NULLA\n"))
        self.assertIn("SET-NULLA", str(e.exception))

    def test_ciclo_in_extends(self):
        testo = "setups:\n  - id: SET-A\n    extends: SET-B\n  - id: SET-B\n    extends: SET-A\n"
        setups, _ = registry.load(scrivi(testo))
        with self.assertRaises(registry.RegistryError):
            registry.root_of("SET-A", setups)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: lancia il test e verifica che fallisca**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: FAIL con `ModuleNotFoundError: No module named 'registry'`

- [ ] **Step 3: scrivi l'implementazione minima**

```python
# tools/editor-sessions/registry.py
"""I mattoni delle sedute: allestimenti e cablaggio.

Legge `docs/roadmap/sedute-mattoni.yaml`. Non sa niente di esiti: quelli
appartengono a `docs/technical/test-manuali-pie.md`, e in questo schema manca
il campo dove scriverli.

Ogni difetto del registro viene RIFIUTATO col proprio nome, mai lasciato
cadere: un input malformato che produce un verde e' il difetto peggiore di
questa famiglia di strumenti (precedente: D-182).
"""
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

import yaml

MATTONI = Path("docs/roadmap/sedute-mattoni.yaml")


class RegistryError(Exception):
    """Un difetto del registro. Si rifiuta, non si ignora."""


@dataclass(frozen=True)
class Setup:
    """Un allestimento: cosa si apre, con quale comando, chi e' umano."""

    id: str
    extends: str | None = None
    fields: dict = field(default_factory=dict)


@dataclass(frozen=True)
class Wire:
    """Il cablaggio di un check: dove si guarda, e cosa serve perche' si possa."""

    check: str
    setup: str
    requires: tuple[str, ...] = ()
    issue: int | None = None


def load(path: Path = MATTONI) -> tuple[dict[str, Setup], list[Wire]]:
    raw = yaml.safe_load(path.read_text(encoding="utf-8")) or {}

    setups: dict[str, Setup] = {}
    for r in raw.get("setups") or []:
        sid = r["id"]
        if sid in setups:
            raise RegistryError(f"setup dichiarato due volte: {sid}")
        setups[sid] = Setup(
            id=sid,
            extends=r.get("extends"),
            fields={k: v for k, v in r.items() if k not in ("id", "extends")},
        )

    for s in setups.values():
        if s.extends is not None and s.extends not in setups:
            raise RegistryError(f"{s.id} estende un setup inesistente: {s.extends}")

    wires: list[Wire] = []
    padrone: dict[str, str] = {}
    for r in raw.get("wiring") or []:
        check = r["check"]
        if check in padrone:
            raise RegistryError(
                f"check rivendicato due volte: {check} da {padrone[check]} e da {r['setup']}"
            )
        if r["setup"] not in setups:
            raise RegistryError(f"{check} cita un setup inesistente: {r['setup']}")
        padrone[check] = r["setup"]
        wires.append(
            Wire(
                check=check,
                setup=r["setup"],
                requires=tuple(r.get("requires") or []),
                issue=r.get("issue"),
            )
        )

    return setups, wires


def root_of(setup_id: str, setups: dict[str, Setup]) -> str:
    """L'allestimento di base della catena `extends`.

    Due check la cui catena finisce nella stessa radice cadono nella stessa
    apertura: `SET-HEX-TURN` non riapre l'Editor rispetto a `SET-HEX-MATCH`,
    aggiunge uno stato.
    """
    visti: list[str] = []
    cur = setup_id
    while True:
        if cur in visti:
            raise RegistryError("ciclo in extends: " + " -> ".join(visti + [cur]))
        visti.append(cur)
        nxt = setups[cur].extends
        if nxt is None:
            return cur
        cur = nxt
```

- [ ] **Step 4: lancia i test e verifica che passino**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: PASS, sette test.

- [ ] **Step 5: commit**

```bash
git add tools/editor-sessions/registry.py tools/editor-sessions/registry_test.py
git commit -m "feat(sedute): il registro dei mattoni rifiuta i difetti per nome"
```

---

## Task 2: lo stato letto dal suo owner

**Files:**
- Create: `tools/editor-sessions/pie_status.py`
- Test: `tools/editor-sessions/pie_status_test.py`

**Interfaces:**
- Consumes: niente.
- Produces: `parse(text: str) -> dict[str, dict]` dove ogni valore è `{"stato": str, "release": bool}` · `load(path: Path) -> dict[str, dict]` · `VERDE: str`.

Il registro marca lo stato con un'emoji nell'ultima cella della riga, e il subset di release con `` `RELEASE-V01` `` nella prima. Questo modulo **legge** e basta: non giudica, non scrive, non deduce.

- [ ] **Step 1: scrivi il test che fallisce**

```python
# tools/editor-sessions/pie_status_test.py
"""Lo stato delle voci PIE si legge dalla tabella del registro, non si deduce."""
from __future__ import annotations

import unittest

import pie_status

REGISTRO = """
Testo di prosa che non e' una riga di tabella e va ignorato.

| **PIE-V01-ROSTER** `RELEASE-V01` | Roster dei 4 eroi | nessun asset | ✅ **Giocata il 2026-09-06** |
| **PIE-HEXPLAY-6** `RELEASE-V01` | Muro attraversabile | scenario | ❌ da rifare in pianificazione |
| **PIE-TD-CLEAN** | Il pannello non sporca | nessuna | ⏳ mai eseguita |
| **PIE-VIS-DEFLECT** | La parata si vede | cue assenti | ⛔ non eseguibile prima delle cue |

> `PIE-V01-ROSTER` citata in prosa: non e' una riga di tabella e non cambia lo stato.
"""


class ParseTest(unittest.TestCase):
    def test_legge_una_voce_per_riga_di_tabella(self):
        st = pie_status.parse(REGISTRO)
        self.assertEqual(
            sorted(st),
            ["PIE-HEXPLAY-6", "PIE-TD-CLEAN", "PIE-V01-ROSTER", "PIE-VIS-DEFLECT"],
        )

    def test_lo_stato_e_la_prima_emoji_dell_ultima_cella(self):
        st = pie_status.parse(REGISTRO)
        self.assertEqual(st["PIE-V01-ROSTER"]["stato"], "✅")
        self.assertEqual(st["PIE-HEXPLAY-6"]["stato"], "❌")
        self.assertEqual(st["PIE-TD-CLEAN"]["stato"], "⏳")
        self.assertEqual(st["PIE-VIS-DEFLECT"]["stato"], "⛔")

    def test_il_subset_di_release_si_legge_dalla_prima_cella(self):
        st = pie_status.parse(REGISTRO)
        self.assertTrue(st["PIE-V01-ROSTER"]["release"])
        self.assertTrue(st["PIE-HEXPLAY-6"]["release"])
        self.assertFalse(st["PIE-TD-CLEAN"]["release"])

    def test_la_prosa_non_produce_voci(self):
        st = pie_status.parse("Una riga che nomina `PIE-FANTASMA` senza essere una tabella.")
        self.assertEqual(st, {})

    def test_verde_e_il_marcatore_che_il_calcolo_scarta(self):
        self.assertEqual(pie_status.VERDE, "✅")


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: lancia il test e verifica che fallisca**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: FAIL con `ModuleNotFoundError: No module named 'pie_status'`

- [ ] **Step 3: scrivi l'implementazione minima**

```python
# tools/editor-sessions/pie_status.py
"""Lo stato di ogni voce PIE, letto dal suo unico owner.

`docs/technical/test-manuali-pie.md` possiede l'esito atteso e lo stato
raggiunto. Questo modulo li LEGGE: non scrive, non giudica, non deduce. Una
riga di tabella dichiara lo stato con un'emoji nell'ultima cella, e
l'appartenenza al subset del gate con `RELEASE-V01` nella prima.
"""
from __future__ import annotations

import re
from pathlib import Path

REGISTRO = Path("docs/technical/test-manuali-pie.md")
RIGA = re.compile(r"^\|\s*\*\*(PIE-[A-Z0-9-]+)\*\*")
STATI = "✅🟡⏳❌⛔🔴"
VERDE = "✅"
SCONOSCIUTO = "?"


def parse(text: str) -> dict[str, dict]:
    out: dict[str, dict] = {}
    for line in text.splitlines():
        m = RIGA.match(line)
        if m is None:
            continue
        celle = [c.strip() for c in line.split("|")]
        ultima = celle[-2] if len(celle) > 2 else ""
        stato = next((c for c in ultima if c in STATI), SCONOSCIUTO)
        out[m.group(1)] = {"stato": stato, "release": "RELEASE-V01" in celle[1]}
    return out


def load(path: Path = REGISTRO) -> dict[str, dict]:
    return parse(path.read_text(encoding="utf-8"))
```

- [ ] **Step 4: lancia i test e verifica che passino**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: PASS, dodici test in totale.

- [ ] **Step 5: verifica il parser contro il registro vero**

Run:
```bash
cd tools/editor-sessions && python -c "
import sys, collections, pathlib, pie_status
sys.stdout.reconfigure(encoding='utf-8')
st = pie_status.load(pathlib.Path('../../docs/technical/test-manuali-pie.md'))
print('voci lette:', len(st))
print(collections.Counter(v['stato'] for v in st.values()))
print('nel subset RELEASE-V01:', sum(1 for v in st.values() if v['release']))
print('sconosciuti:', [k for k, v in st.items() if v['stato'] == pie_status.SCONOSCIUTO])
"
```

⛔ **`sys.stdout.reconfigure(encoding='utf-8')` non e' facoltativo su Windows.** La console e'
`cp1252` e le emoji di stato la fanno esplodere con `UnicodeEncodeError` — misurato il 2026-09-11,
su `⏳`. Vale per qualunque comando di questo repository che stampi uno stato; i file invece si
scrivono gia' con `encoding='utf-8'` esplicito e non hanno il problema.

Expected: `sconosciuti` è la lista vuota, e `RELEASE-V01` dà il numero che il registro dichiara per il gate G9. **Se compare anche un solo sconosciuto, fermati e guarda la riga che lo produce**: significa che il registro usa una forma che il parser non conosce, e dedurla al posto suo è il modo di sbagliare in silenzio.

Misurato il 2026-09-11, per confronto: `voci lette: 230`, `⏳ 126 · ✅ 71 · 🟡 27 · ❌ 4 · ⛔ 2`, `RELEASE-V01: 17`, `sconosciuti: []`.

- [ ] **Step 6: commit**

```bash
git add tools/editor-sessions/pie_status.py tools/editor-sessions/pie_status_test.py
git commit -m "feat(sedute): lo stato dei check si legge dal registro che lo possiede"
```

---

## Task 3: gli oracoli dei prerequisiti

**Files:**
- Create: `tools/editor-sessions/oracles.py`
- Test: `tools/editor-sessions/oracles_test.py`

**Interfaces:**
- Consumes: niente.
- Produces: `Esito(soddisfatto: bool, perche: str)` · `TIPI: tuple[str, ...]` · `TIPI_CON_ISSUE: tuple[str, ...]` · `asset_tracciati() -> set[str]` · `issue_chiuse(cache: dict) -> set[int]` · `issue_note(cache: dict) -> set[int]` · `valuta(req: str, inventario: set[str], chiuse: set[int], note: set[int]) -> Esito`.

Forma di un `requires`: `<tipo>:<argomento>` per `asset`, e `<tipo>:<argomento>#<issue>` per gli altri. Un prerequisito che non c'è **non si dichiara**: l'assenza di una riga significa «non serve», e non serve un tipo per dirlo.

- [ ] **Step 1: scrivi il test che fallisce**

```python
# tools/editor-sessions/oracles_test.py
"""Un requires e' soddisfatto o no, e l'oracolo dice sempre chi lo ha stabilito."""
from __future__ import annotations

import unittest

import oracles

INVENTARIO = {"BP_Unit_Aevik", "L_HexArena", "WBP_RT_TacticalHUD"}
CHIUSE = {2723}
NOTE = {2697, 2723, 2454}


class AssetTest(unittest.TestCase):
    def test_asset_presente_e_soddisfatto(self):
        e = oracles.valuta("asset:BP_Unit_Aevik", INVENTARIO, CHIUSE, NOTE)
        self.assertTrue(e.soddisfatto)
        self.assertIn("BP_Unit_Aevik", e.perche)

    def test_asset_assente_blocca_e_dice_dove_manca(self):
        e = oracles.valuta("asset:WBP_RT_PauseMenu", INVENTARIO, CHIUSE, NOTE)
        self.assertFalse(e.soddisfatto)
        self.assertIn("WBP_RT_PauseMenu", e.perche)
        self.assertIn("Content/", e.perche)


class IssueTest(unittest.TestCase):
    def test_issue_aperta_blocca_e_nomina_la_issue(self):
        e = oracles.valuta("mount:WBP_RT_EventLogRight#2697", INVENTARIO, CHIUSE, NOTE)
        self.assertFalse(e.soddisfatto)
        self.assertIn("#2697", e.perche)

    def test_issue_chiusa_sblocca(self):
        e = oracles.valuta("feature:ReactionWindow#2723", INVENTARIO, CHIUSE, NOTE)
        self.assertTrue(e.soddisfatto)
        self.assertIn("#2723", e.perche)

    def test_issue_fuori_cache_blocca_e_lo_dichiara(self):
        e = oracles.valuta("cue:Deflect#9999", INVENTARIO, CHIUSE, NOTE)
        self.assertFalse(e.soddisfatto)
        self.assertIn("cache", e.perche)


class FormaTest(unittest.TestCase):
    def test_tipo_sconosciuto_viene_rifiutato(self):
        with self.assertRaises(ValueError) as e:
            oracles.valuta("colore:rosso", INVENTARIO, CHIUSE, NOTE)
        self.assertIn("colore", str(e.exception))

    def test_tipo_con_issue_senza_issue_viene_rifiutato(self):
        with self.assertRaises(ValueError) as e:
            oracles.valuta("mount:WBP_RT_EventLogRight", INVENTARIO, CHIUSE, NOTE)
        self.assertIn("#", str(e.exception))


class CacheTest(unittest.TestCase):
    def test_issue_chiuse_legge_la_forma_della_cache_del_decision_log(self):
        cache = {
            "repo": "DegrassiAaron/refactor-tactics-main",
            "items": {
                "2723": {"state": "closed", "title": "view model"},
                "2697": {"state": "open", "title": "il feed non e' montato"},
            },
        }
        self.assertEqual(oracles.issue_chiuse(cache), {2723})


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: lancia il test e verifica che fallisca**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: FAIL con `ModuleNotFoundError: No module named 'oracles'`

- [ ] **Step 3: scrivi l'implementazione minima**

```python
# tools/editor-sessions/oracles.py
"""Un prerequisito e' soddisfatto oppure no, e chi lo stabilisce.

Due oracoli soli, per scelta:

  asset:<Nome>                 il file esiste fra quelli tracciati in Content/.
                               In questo clone e' un oracolo valido: disco e
                               indice coincidono, e .gitignore:48 ignora
                               Content/**/*.uasset con eccezioni negate, quindi
                               «non tracciato» significa «non esiste».

  <tipo>:<Nome>#<issue>        lo stabilisce chi possiede la issue. Chiusa =
                               soddisfatto. E' anche il modo in cui un
                               bloccante smette di nascondere un check per
                               sempre: quando la issue chiude, il check torna
                               nell'ordine del giorno da solo.

Un prerequisito che non serve NON si dichiara. L'assenza di una riga e' gia'
la dichiarazione, e un tipo per dire «non serve» sarebbe un campo in piu' che
puo' mentire.
"""
from __future__ import annotations

import subprocess
from dataclasses import dataclass

TIPI_CON_ISSUE = ("mount", "cue", "anim", "feature")
TIPI = ("asset",) + TIPI_CON_ISSUE


@dataclass(frozen=True)
class Esito:
    soddisfatto: bool
    perche: str


def asset_tracciati() -> set[str]:
    """I nomi (senza estensione) dei package versionati sotto `Content/`."""
    out = subprocess.run(
        ["git", "ls-files", "Content"], capture_output=True, text=True, check=True
    ).stdout
    nomi: set[str] = set()
    for riga in out.splitlines():
        nome = riga.rsplit("/", 1)[-1]
        for ext in (".uasset", ".umap"):
            if nome.endswith(ext):
                nomi.add(nome[: -len(ext)])
    return nomi


def issue_chiuse(cache: dict) -> set[int]:
    """I numeri chiusi nella cache GitHub, nella forma di `tools/decision-log/`."""
    return {
        int(n) for n, v in (cache.get("items") or {}).items() if v.get("state") == "closed"
    }


def issue_note(cache: dict) -> set[int]:
    """I numeri che la cache conosce, chiusi o aperti."""
    return {int(n) for n in (cache.get("items") or {})}


def valuta(req: str, inventario: set[str], chiuse: set[int], note: set[int]) -> Esito:
    tipo, _, resto = req.partition(":")
    if tipo not in TIPI:
        raise ValueError(f"tipo di requires sconosciuto: {tipo} (in {req!r})")

    if tipo == "asset":
        presente = resto in inventario
        return Esito(
            presente,
            f"{resto} tracciato" if presente else f"{resto} non esiste in Content/",
        )

    nome, sep, numero = resto.partition("#")
    if not sep or not numero.isdigit():
        raise ValueError(f"{tipo} richiede una issue owner nella forma nome#numero: {req!r}")
    n = int(numero)
    if n not in note:
        return Esito(False, f"{tipo} {nome}: #{n} non e' nella cache — rilancia il fetch")
    if n in chiuse:
        return Esito(True, f"{tipo} {nome}: #{n} chiusa")
    return Esito(False, f"{tipo} {nome}: #{n} aperta")
```

- [ ] **Step 4: lancia i test e verifica che passino**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: PASS, venti test in totale.

- [ ] **Step 5: commit**

```bash
git add tools/editor-sessions/oracles.py tools/editor-sessions/oracles_test.py
git commit -m "feat(sedute): due oracoli per i prerequisiti, asset tracciato e issue owner"
```

---

## Task 4: il calcolo dell'ordine del giorno

**Files:**
- Create: `tools/editor-sessions/agenda.py`
- Test: `tools/editor-sessions/agenda_test.py`

**Interfaces:**
- Consumes: `registry.Setup`, `registry.Wire`, `registry.root_of`, `pie_status.VERDE`, `oracles.Esito`.
- Produces: `Voce(check: str, bloccanti: tuple[str, ...], release: bool)` · `Gruppo(setup: str, liberi: list[Voce], bloccati: list[Voce])` · `calcola(setups, wires, stato, valuta) -> tuple[list[Gruppo], list[str]]` dove il secondo elemento è la **coda scoperta**, cioè gli ID del registro PIE che nessuna riga di `wiring` cabla.

La firma di `valuta` attesa da `calcola` è `Callable[[str], oracles.Esito]`: il chiamante lega inventario e cache, così il calcolo resta puro e i test non toccano né git né la rete.

- [ ] **Step 1: scrivi il test che fallisce**

```python
# tools/editor-sessions/agenda_test.py
"""Il calcolo raggruppa per allestimento, scarta i verdi, e non fa sparire nessuno."""
from __future__ import annotations

import unittest

import agenda
import oracles
from registry import Setup, Wire

SETUPS = {
    "SET-HEX-MATCH": Setup(id="SET-HEX-MATCH", extends=None, fields={"map": "L_HexArena"}),
    "SET-HEX-TURN": Setup(id="SET-HEX-TURN", extends="SET-HEX-MATCH", fields={}),
    "SET-FRONTEND": Setup(id="SET-FRONTEND", extends=None, fields={}),
}

STATO = {
    "PIE-A": {"stato": "⏳", "release": False},
    "PIE-B": {"stato": "⏳", "release": False},
    "PIE-VERDE": {"stato": "✅", "release": True},
    "PIE-REL": {"stato": "🟡", "release": True},
    "PIE-BLOCCATA": {"stato": "⏳", "release": False},
    "PIE-SCOPERTA": {"stato": "⏳", "release": False},
}


def valuta(req: str) -> oracles.Esito:
    if req == "asset:MANCANTE":
        return oracles.Esito(False, "MANCANTE non esiste in Content/")
    return oracles.Esito(True, f"{req} ok")


WIRES = [
    Wire(check="PIE-A", setup="SET-HEX-MATCH"),
    Wire(check="PIE-B", setup="SET-HEX-TURN"),
    Wire(check="PIE-VERDE", setup="SET-HEX-MATCH"),
    Wire(check="PIE-REL", setup="SET-FRONTEND"),
    Wire(check="PIE-BLOCCATA", setup="SET-FRONTEND", requires=("asset:MANCANTE",)),
]


class CalcoloTest(unittest.TestCase):
    def setUp(self):
        self.gruppi, self.scoperta = agenda.calcola(SETUPS, WIRES, STATO, valuta)
        self.per_id = {g.setup: g for g in self.gruppi}

    def test_i_verdi_escono_dal_calcolo(self):
        tutti = [v.check for g in self.gruppi for v in g.liberi + g.bloccati]
        self.assertNotIn("PIE-VERDE", tutti)

    def test_extends_cade_nella_stessa_apertura(self):
        self.assertEqual(sorted(self.per_id), ["SET-FRONTEND", "SET-HEX-MATCH"])
        self.assertEqual(
            sorted(v.check for v in self.per_id["SET-HEX-MATCH"].liberi), ["PIE-A", "PIE-B"]
        )

    def test_un_bloccato_resta_stampato_col_motivo(self):
        g = self.per_id["SET-FRONTEND"]
        self.assertEqual([v.check for v in g.bloccati], ["PIE-BLOCCATA"])
        self.assertIn("MANCANTE", g.bloccati[0].bloccanti[0])
        self.assertEqual([v.check for v in g.liberi], ["PIE-REL"])

    def test_release_va_per_prima(self):
        self.assertEqual(self.gruppi[0].setup, "SET-FRONTEND")

    def test_la_coda_scoperta_elenca_chi_nessuno_cabla(self):
        self.assertEqual(self.scoperta, ["PIE-SCOPERTA"])

    def test_la_coda_scoperta_ignora_i_verdi(self):
        stato = dict(STATO, **{"PIE-SCOPERTA": {"stato": "✅", "release": False}})
        _, scoperta = agenda.calcola(SETUPS, WIRES, stato, valuta)
        self.assertEqual(scoperta, [])

    def test_un_wire_che_cabla_un_check_sconosciuto_viene_rifiutato(self):
        wires = WIRES + [Wire(check="PIE-FANTASMA", setup="SET-FRONTEND")]
        with self.assertRaises(agenda.AgendaError) as e:
            agenda.calcola(SETUPS, wires, STATO, valuta)
        self.assertIn("PIE-FANTASMA", str(e.exception))

    def test_l_ordine_e_deterministico(self):
        a, _ = agenda.calcola(SETUPS, WIRES, STATO, valuta)
        b, _ = agenda.calcola(SETUPS, list(reversed(WIRES)), STATO, valuta)
        self.assertEqual([g.setup for g in a], [g.setup for g in b])
        self.assertEqual(
            [[v.check for v in g.liberi] for g in a],
            [[v.check for v in g.liberi] for g in b],
        )


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: lancia il test e verifica che fallisca**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: FAIL con `ModuleNotFoundError: No module named 'agenda'`

- [ ] **Step 3: scrivi l'implementazione minima**

```python
# tools/editor-sessions/agenda.py
"""Da mattoni a ordine del giorno.

Il calcolo in cinque mosse:

  1. per ogni check, lo stato e i requires non soddisfatti;
  2. si scartano i verdi — non c'e' piu' niente da guardare;
  3. si raggruppa per la RADICE della catena `extends`, perche' e' l'apertura
     dell'Editor a costare, non il Play;
  4. un gruppo con almeno un check libero e' una apertura; i bloccati restano
     stampati sotto, col motivo;
  5. si ordina: prima chi porta il subset di release, poi per numero di check
     liberi, poi per ID — perche' l'ordine dev'essere totale.

Non tocca ne' git ne' la rete: `valuta` arriva gia' legata al suo inventario.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable

import oracles
import pie_status
from registry import Setup, Wire, root_of


class AgendaError(Exception):
    """Un cablaggio che non corrisponde al registro. Si rifiuta."""


@dataclass(frozen=True)
class Voce:
    check: str
    bloccanti: tuple[str, ...]
    release: bool


@dataclass
class Gruppo:
    setup: str
    liberi: list[Voce] = field(default_factory=list)
    bloccati: list[Voce] = field(default_factory=list)


def calcola(
    setups: dict[str, Setup],
    wires: list[Wire],
    stato: dict[str, dict],
    valuta: Callable[[str], oracles.Esito],
) -> tuple[list[Gruppo], list[str]]:
    gruppi: dict[str, Gruppo] = {}
    cablati: set[str] = set()

    for w in wires:
        if w.check not in stato:
            raise AgendaError(
                f"{w.check} e' cablato ma non esiste nel registro PIE"
            )
        cablati.add(w.check)
        if stato[w.check]["stato"] == pie_status.VERDE:
            continue

        bloccanti = tuple(
            e.perche for e in (valuta(r) for r in w.requires) if not e.soddisfatto
        )
        voce = Voce(check=w.check, bloccanti=bloccanti, release=stato[w.check]["release"])
        radice = root_of(w.setup, setups)
        g = gruppi.setdefault(radice, Gruppo(setup=radice))
        (g.bloccati if bloccanti else g.liberi).append(voce)

    for g in gruppi.values():
        g.liberi.sort(key=lambda v: v.check)
        g.bloccati.sort(key=lambda v: v.check)

    ordinati = sorted(
        gruppi.values(),
        key=lambda g: (
            0 if any(v.release for v in g.liberi) else 1,
            -len(g.liberi),
            g.setup,
        ),
    )

    scoperta = sorted(
        c
        for c, v in stato.items()
        if c not in cablati and v["stato"] != pie_status.VERDE
    )
    return ordinati, scoperta
```

- [ ] **Step 4: lancia i test e verifica che passino**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: PASS, ventotto test in totale.

- [ ] **Step 5: commit**

```bash
git add tools/editor-sessions/agenda.py tools/editor-sessions/agenda_test.py
git commit -m "feat(sedute): il calcolo raggruppa per allestimento e non fa sparire i bloccati"
```

---

## Task 5: i dati — gli otto allestimenti e la semina del cablaggio

**Files:**
- Create: `tools/editor-sessions/seed_wiring.py`
- Create: `docs/roadmap/sedute-mattoni.yaml`
- Test: `tools/editor-sessions/seed_wiring_test.py`

**Interfaces:**
- Consumes: niente dei task precedenti — questo eseguibile produce dati che `registry.load` dovrà poi accettare.
- Produces: `MAPPA_SETUP: tuple[tuple[str, str], ...]` · `CAMPI_PROSA: tuple[str, ...]` · `deduci_setup(blob: str) -> str | None` · `semina(sessioni: list[dict]) -> tuple[list[dict], dict[str, list[str]]]` che ritorna le righe di wiring e, per ogni **seduta** di cui non ha saputo dedurre l'allestimento, i suoi check.

La semina è meccanica e fallibile per costruzione. Ciò che non sa dedurre finisce a parte, **non** in una riga con un setup inventato: `registry.load` rifiuta i setup sconosciuti, quindi una riga seminata male non può passare inosservata.

⚠️ **Sappi in anticipo quanto copre, perché è circa metà.** Misurato il 2026-09-11 su `1b9f6f36`: la
semina produce **83** righe su **164** check citati, e lascia il resto orfano. Allargare gli indizi non
aiuta — provato con `DA_HexMap_*`, `Tactical Designer`, `partita hex` e `Board 2v2`: **+2**. Non è un
difetto dell'euristica: **la prosa del vecchio registro non contiene l'allestimento**, che è esattamente
la premessa del design. Il resto è giudizio, e ha un task suo (Task 6). Rimisura: il numero è l'esito di
quel passaggio, e cambia col commit di base.

- [ ] **Step 1: scrivi il test che fallisce**

```python
# tools/editor-sessions/seed_wiring_test.py
"""La semina deduce l'allestimento dalla prosa, e dichiara cio' che non sa dedurre."""
from __future__ import annotations

import unittest

import seed_wiring


class DeduzioneTest(unittest.TestCase):
    def test_riconosce_il_frontend(self):
        self.assertEqual(seed_wiring.deduci_setup("si parte da L_Frontend e si preme PLAY"), "SET-FRONTEND")

    def test_riconosce_lo_scenario(self):
        self.assertEqual(seed_wiring.deduci_setup("lancia rt.Test.Scenario Visual.Map.X"), "SET-SCEN")

    def test_riconosce_l_autobattle(self):
        self.assertEqual(
            seed_wiring.deduci_setup("partita con rt.Match.Autobattle=1 su L_HexArena"), "SET-HEX-BOT"
        )

    def test_il_piu_specifico_vince_sull_arena(self):
        self.assertEqual(
            seed_wiring.deduci_setup("apri L_GrayKitPlayground, poi torna su L_HexArena"), "SET-GRAYKIT"
        )

    def test_senza_indizi_non_inventa(self):
        self.assertIsNone(seed_wiring.deduci_setup("una seduta descritta senza nominare mappe"))


class SeminaTest(unittest.TestCase):
    def test_una_riga_per_check_col_setup_dedotto(self):
        sessioni = [
            {"id": "U49", "verifies": ["PIE-V01-SCREENHUD"], "issues": [613],
             "steps": "aprire L_Frontend e premere PLAY"},
        ]
        righe, orfani = seed_wiring.semina(sessioni)
        self.assertEqual(orfani, {})
        self.assertEqual(
            righe,
            [{"check": "PIE-V01-SCREENHUD", "setup": "SET-FRONTEND", "issue": 613}],
        )

    def test_un_check_cablato_due_volte_compare_una_volta_sola(self):
        sessioni = [
            {"id": "U4", "verifies": ["PIE-HEXPLAY-6"], "steps": "rt.Test.Scenario X"},
            {"id": "U46", "verifies": ["PIE-HEXPLAY-6"], "steps": "rt.Test.Scenario X"},
        ]
        righe, _ = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-HEXPLAY-6"])

    def test_cio_che_non_sa_dedurre_resta_raggruppato_per_seduta(self):
        sessioni = [
            {"id": "U17", "verifies": ["PIE-MISTERO", "PIE-ALTRO"], "steps": "nessun indizio"},
            {"id": "U53", "verifies": ["PIE-TERZO"], "steps": "nemmeno qui"},
        ]
        righe, orfani = seed_wiring.semina(sessioni)
        self.assertEqual(righe, [])
        self.assertEqual(orfani, {"U17": ["PIE-ALTRO", "PIE-MISTERO"], "U53": ["PIE-TERZO"]})

    def test_una_seduta_muta_non_produce_orfani_se_un_altra_cabla_lo_stesso_check(self):
        sessioni = [
            {"id": "U1", "verifies": ["PIE-X"], "steps": "L_HexArena"},
            {"id": "U17", "verifies": ["PIE-X"], "steps": "nessun indizio"},
        ]
        righe, orfani = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-X"])
        self.assertEqual(orfani, {})

    def test_l_uscita_e_ordinata_per_check(self):
        sessioni = [{"id": "U1", "verifies": ["PIE-Z", "PIE-A"], "steps": "L_HexArena"}]
        righe, _ = seed_wiring.semina(sessioni)
        self.assertEqual([r["check"] for r in righe], ["PIE-A", "PIE-Z"])


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: lancia il test e verifica che fallisca**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: FAIL con `ModuleNotFoundError: No module named 'seed_wiring'`

- [ ] **Step 3: scrivi l'implementazione minima**

```python
# tools/editor-sessions/seed_wiring.py
"""Semina la bozza del cablaggio dal vecchio registro delle sedute.

Meccanica e fallibile per costruzione: deduce l'allestimento dagli indizi che
le sedute lasciano nella prosa. Cio' che NON sa dedurre lo dichiara fra gli
orfani, e non lo scrive: `registry.load` rifiuta un setup sconosciuto, quindi
una riga seminata male non puo' passare inosservata.

    python3 tools/editor-sessions/seed_wiring.py --out build/wiring-seminato.yaml
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import yaml

VECCHIO = Path("docs/roadmap/editor-sessions.yaml")

# Dal piu' specifico al piu' generico: la prima corrispondenza vince.
MAPPA_SETUP: tuple[tuple[str, str], ...] = (
    ("rt.Match.Autobattle", "SET-HEX-BOT"),
    ("L_GrayKitPlayground", "SET-GRAYKIT"),
    ("L_Frontend", "SET-FRONTEND"),
    ("rt.Test.Scenario", "SET-SCEN"),
    ("L_DevSandbox", "SET-SANDBOX"),
    ("L_HexArena", "SET-HEX-MATCH"),
)

CAMPI_PROSA = ("title", "produces", "steps", "notes", "done_when")


def deduci_setup(blob: str) -> str | None:
    for indizio, setup in MAPPA_SETUP:
        if indizio in blob:
            return setup
    return None


def semina(sessioni: list[dict]) -> tuple[list[dict], dict[str, list[str]]]:
    """Le righe seminate, e i check orfani RAGGRUPPATI PER SEDUTA.

    Il raggruppamento non e' cosmetico: una seduta ERA un allestimento per
    costruzione, quindi chi assegna decide una volta per seduta invece che una
    volta per check. Sul registro del 2026-09-11 la differenza e' 24 giudizi
    invece di 95.
    """
    righe: dict[str, dict] = {}
    muti: dict[str, list[str]] = {}
    for s in sessioni:
        blob = " ".join(str(s.get(k) or "") for k in CAMPI_PROSA)
        setup = deduci_setup(blob)
        issue = (s.get("issues") or [None])[0]
        for check in s.get("verifies") or []:
            if check in righe:
                continue
            if setup is None:
                muti.setdefault(s["id"], []).append(check)
                continue
            riga = {"check": check, "setup": setup}
            if issue is not None:
                riga["issue"] = issue
            righe[check] = riga

    orfani = {
        sid: sorted(set(cs) - set(righe))
        for sid, cs in muti.items()
        if set(cs) - set(righe)
    }
    return [righe[c] for c in sorted(righe)], orfani


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sessioni", default=VECCHIO, type=Path, help="il vecchio registro")
    ap.add_argument("--out", default=Path("build/wiring-seminato.yaml"), type=Path)
    a = ap.parse_args()

    if not a.sessioni.exists():
        sys.exit(f"registro non trovato: {a.sessioni}")
    vecchio = yaml.safe_load(a.sessioni.read_text(encoding="utf-8")) or {}
    righe, orfani = semina(vecchio.get("sessions") or [])

    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(
        yaml.safe_dump({"wiring": righe}, allow_unicode=True, sort_keys=False),
        encoding="utf-8",
    )
    print(f"righe seminate: {len(righe)} -> {a.out}")
    if orfani:
        quanti = sum(len(cs) for cs in orfani.values())
        print(f"SENZA ALLESTIMENTO: {quanti} check da {len(orfani)} sedute.")
        print("Decidi una volta per SEDUTA: era un allestimento per costruzione.")
        for sid in sorted(orfani):
            print(f"  {sid}: {' '.join(orfani[sid])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 4: lancia i test e verifica che passino**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: PASS, trentotto test in totale (dieci in questo file).

- [ ] **Step 5: semina davvero**

Run:
```bash
python tools/editor-sessions/seed_wiring.py --out build/wiring-seminato.yaml
```
Expected: il file esiste, e sotto compare l'elenco delle sedute mute coi loro check. **Non assegnarli adesso**: è il Task 6, e ha bisogno di un passaggio suo.

- [ ] **Step 6: scrivi il registro vero, con il solo cablaggio seminato**

Crea `docs/roadmap/sedute-mattoni.yaml` con l'intestazione e gli otto allestimenti qui sotto, poi incolla sotto `wiring:` il contenuto di `build/wiring-seminato.yaml`. Gli orfani restano fuori: il registro dev'essere valido a ogni passo, e una riga senza setup non lo è.

```yaml
# Sedute — i MATTONI: allestimenti e cablaggio.
#
# ⛔ DRAFT — NON OWNER. L'owner delle sedute resta `editor-sessions.yaml` fino alla fetta 5 del
#    piano `plans/sedute-componibili-piano-fetta-0-2026-09-11.md`. Finche' questa riga e' qui, un
#    disaccordo fra i due file si risolve a favore del vecchio.
#
# 🔑 Qui NON si scrive l'esito di un check, ne' quello atteso ne' quello raggiunto: li possiede
#    `docs/technical/test-manuali-pie.md`. In questo schema manca il campo dove scriverli, ed e'
#    deliberato.
#
# Forma di un `requires`:
#   asset:<Nome>              il package esiste fra quelli tracciati sotto Content/
#   mount:<Nome>#<issue>      il widget esiste e nessuno lo monta; lo sblocca la chiusura della issue
#   cue:<Nome>#<issue>        l'evento esiste e nessuno lo disegna
#   anim:<Nome>#<issue>       la clip serve al verdetto e non c'e'
#   feature:<Nome>#<issue>    la condizione non e' ottenibile
# Un prerequisito che non serve NON si dichiara: l'assenza della riga e' gia' la dichiarazione.

setups:
  - id: SET-HEX-MATCH          # ex «A — partita hex avviata»
    map: L_HexArena
    format: Format.Skirmish2v2
    who: { human: [team0], bot: [team1] }
  - id: SET-HEX-TURN           # ex «B — un turno pianificato e risolto»
    extends: SET-HEX-MATCH
    given: "piani impostati, lock-in con Spazio, un turno con un fallback e una modifica ambientale"
  - id: SET-SCEN               # ex «C» — una apertura, molti Play
    command: "rt.Test.Scenario <scenario_id>"
    note: "lo scenario porta arena, unita' e piani; il comando si riemette senza chiudere l'Editor"
  - id: SET-HEX-BOT            # ex «D — partita lunga col bot»
    extends: SET-HEX-MATCH
    cvars: [ "rt.Match.Autobattle=1" ]
  - id: SET-FRONTEND
    map: L_Frontend
    given: "premere PLAY, poi Home; non aprire direttamente L_HexArena"
  - id: SET-SANDBOX
    map: L_DevSandbox
    given: "tool Geometry e griglia di lavoro"
  - id: SET-GRAYKIT
    map: L_GrayKitPlayground
  - id: SET-TD
    given: "il pannello Tactical Designer, aperto dal dock"

wiring:
  # incolla qui il contenuto di build/wiring-seminato.yaml
```

- [ ] **Step 7: verifica che il registro vero sia accettato**

Run:
```bash
cd tools/editor-sessions && python -c "
import registry, pathlib
setups, wires = registry.load(pathlib.Path('../../docs/roadmap/sedute-mattoni.yaml'))
print('allestimenti:', sorted(setups))
print('righe di wiring:', len(wires))
"
```
Expected: nessuna eccezione, e gli otto allestimenti elencati. **Se `registry.load` solleva `RegistryError`, il messaggio nomina il difetto: correggilo nel yaml, non nel codice.**

- [ ] **Step 8: commit**

```bash
git add tools/editor-sessions/seed_wiring.py tools/editor-sessions/seed_wiring_test.py docs/roadmap/sedute-mattoni.yaml
git commit -m "feat(sedute): gli otto allestimenti, e il cablaggio seminato dal vecchio registro"
```

---

## Task 6: assegnare l'allestimento alle sedute mute

**Files:**
- Modify: `docs/roadmap/sedute-mattoni.yaml` — sezione `wiring:`

**Interfaces:**
- Consumes: l'elenco stampato da `seed_wiring.py`, e `registry.load` come verifica.
- Produces: un `wiring` che copre ogni check non verde del registro PIE, oppure una lista dichiarata di check che **deliberatamente** non si cablano.

Questo task non produce codice. È il lavoro vero della fetta: **scrivere l'allestimento che non è mai stato scritto.** Misurato il 2026-09-11, riguarda **95** check da **24** sedute — `U2 U3 U4 U5 U6 U7 U9 U13 U14 U15 U16 U19 U28 U29 U30 U35 U37 U42 U43 U44 U48 U51 U52 U53`. Rimisura col comando dello Step 1: il numero e' l'esito di quel passaggio.

⛔ **Non assegnare per prefisso dell'ID.** È stato provato e va respinto: i due prefissi più numerosi fra gli orfani sono `PIE-V01-*` (30) e `PIE-VIS-*` (19), che sono famiglie **semantiche** e coprono allestimenti diversi. Mapparli in blocco riprodurrebbe la falsa equivalenza di `shares_setup_with` che questo lavoro esiste per rimuovere.

- [ ] **Step 1: rileggi l'elenco, raggruppato per seduta**

Run:
```bash
python tools/editor-sessions/seed_wiring.py --out build/wiring-seminato.yaml
```
L'uscita elenca `<seduta>: <check> <check> ...`. **Decidi una volta per seduta**, non una volta per check: una seduta era un allestimento per costruzione, ed è la ragione per cui il seminatore raggruppa così.

- [ ] **Step 2: per ogni seduta muta, apri il suo record e decidi**

Run, sostituendo l'ID:
```bash
python -c "
import yaml
d = yaml.safe_load(open('docs/roadmap/editor-sessions.yaml', encoding='utf-8'))
r = {s['id']: s for s in d['sessions']}['U42']
for k in ('title', 'produces', 'done_when', 'steps'):
    print('==', k, '==')
    print(str(r.get(k) or '')[:1500])
"
```

Scegli **uno** degli otto ID di `setups`. Tre casi che incontrerai, e cosa farne:

1. **La seduta nomina una mappa o un comando** che il seminatore non ha riconosciuto → assegna quello, e valuta se aggiungere l'indizio a `MAPPA_SETUP` (utile solo se ricorre: misurato, allargare gli indizi valeva `+2`, quindi non farlo per un caso singolo).
2. **La seduta descrive una partita normale senza nominare la mappa** → `SET-HEX-MATCH`, o `SET-HEX-TURN` se il criterio richiede un turno già pianificato e risolto.
3. **La seduta ha check che appartengono ad allestimenti diversi** → è il caso interessante: significa che quella seduta ne mescolava due, e i suoi check si separano su due righe con setup diversi. **Annotalo**, perché è una cosa che il vecchio registro non poteva dire.

- [ ] **Step 3: aggiungi le righe e verifica che il registro regga**

Run:
```bash
cd tools/editor-sessions && python -c "
import registry, pathlib
setups, wires = registry.load(pathlib.Path('../../docs/roadmap/sedute-mattoni.yaml'))
print('allestimenti:', sorted(setups))
print('righe di wiring:', len(wires))
"
```
Expected: nessuna eccezione. `RegistryError` nomina il difetto — correggilo nel yaml.

- [ ] **Step 4: dichiara ciò che NON cabli**

Se decidi che un check non va cablato, non lasciarlo sparire: aggiungi in coda a `sedute-mattoni.yaml` una sezione di commento

```yaml
# Deliberatamente NON cablati, con la ragione:
#   PIE-XXX   <perché — per esempio: la sua precondizione è una decisione aperta, owner #nnnn>
```

⚠️ Un check che sparisce senza una riga qui è indistinguibile da uno dimenticato, e ricomparirà nella coda scoperta a ogni esecuzione.

- [ ] **Step 5: commit**

```bash
git add docs/roadmap/sedute-mattoni.yaml
git commit -m "feat(sedute): l'allestimento delle sedute mute, scritto invece che dedotto"
```

---

## Task 7: l'ordine del giorno

**Files:**
- Create: `tools/editor-sessions/build_agenda.py`
- Create: `tools/editor-sessions/README.md`

**Interfaces:**
- Consumes: `registry.load`, `pie_status.load`, `oracles.asset_tracciati`, `oracles.issue_chiuse`, `oracles.issue_note`, `oracles.valuta`, `agenda.calcola`.
- Produces: l'eseguibile. Nessun modulo successivo lo importa.

- [ ] **Step 1: scrivi l'eseguibile**

```python
# tools/editor-sessions/build_agenda.py
"""L'ordine del giorno della prossima apertura dell'Editor.

Risponde a una domanda sola: apro l'Editor adesso — cosa allestisco e cosa
guardo. L'uscita NON si committa: `build/` e' ignorato, e un ordine del giorno
committato invecchierebbe in silenzio, che e' il modo in cui e' morta la vista
rimossa da D-181.

    python3 tools/decision-log/fetch_github_cache.py     # aggiorna la cache (serve `gh`)
    python3 tools/editor-sessions/build_agenda.py --out build/ordine-del-giorno.md
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import agenda
import oracles
import pie_status
import registry

CACHE = Path("tools/decision-log/github-cache.json")


def rendi(gruppi, scoperta, setups) -> str:
    righe = ["# Ordine del giorno — la prossima apertura dell'Editor", ""]
    righe.append("> Rigenerato da `tools/editor-sessions/build_agenda.py`. **Non si committa.**")
    righe.append("> L'esito di ogni voce appartiene a `docs/technical/test-manuali-pie.md`.")
    righe.append("")

    aperture = [g for g in gruppi if g.liberi]
    sospesi = [g for g in gruppi if not g.liberi]

    for n, g in enumerate(aperture, start=1):
        s = setups[g.setup]
        righe.append(f"## Apertura {n} — `{g.setup}`")
        righe.append("")
        for k, v in s.fields.items():
            righe.append(f"- **{k}**: `{v}`")
        righe.append("")
        righe.append("Da guardare:")
        for v in g.liberi:
            marca = " `RELEASE-V01`" if v.release else ""
            righe.append(f"- `{v.check}`{marca}")
        if g.bloccati:
            righe.append("")
            righe.append("Bloccati in questa stessa apertura:")
            for v in g.bloccati:
                righe.append(f"- `{v.check}` — {'; '.join(v.bloccanti)}")
        righe.append("")

    if sospesi:
        righe.append("## Nessuna apertura: ogni voce e' bloccata")
        righe.append("")
        for g in sospesi:
            righe.append(f"### `{g.setup}`")
            for v in g.bloccati:
                righe.append(f"- `{v.check}` — {'; '.join(v.bloccanti)}")
            righe.append("")

    righe.append("## Coda scoperta — voci che nessuna riga di wiring cabla")
    righe.append("")
    if scoperta:
        for c in scoperta:
            righe.append(f"- `{c}`")
    else:
        righe.append("Nessuna.")
    righe.append("")
    return "\n".join(righe)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mattoni", default=registry.MATTONI, type=Path)
    ap.add_argument("--registro", default=pie_status.REGISTRO, type=Path)
    ap.add_argument("--cache", default=CACHE, type=Path)
    ap.add_argument("--out", default=Path("build/ordine-del-giorno.md"), type=Path)
    a = ap.parse_args()

    for p in (a.mattoni, a.registro, a.cache):
        if not p.exists():
            sys.exit(f"file non trovato: {p}")

    try:
        setups, wires = registry.load(a.mattoni)
    except registry.RegistryError as e:
        sys.exit(f"registro dei mattoni rifiutato: {e}")

    stato = pie_status.load(a.registro)
    cache = json.loads(a.cache.read_text(encoding="utf-8"))
    inventario = oracles.asset_tracciati()
    chiuse = oracles.issue_chiuse(cache)
    note = oracles.issue_note(cache)

    try:
        gruppi, scoperta = agenda.calcola(
            setups, wires, stato, lambda r: oracles.valuta(r, inventario, chiuse, note)
        )
    except (agenda.AgendaError, registry.RegistryError, ValueError) as e:
        sys.exit(f"calcolo rifiutato: {e}")

    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(rendi(gruppi, scoperta, setups), encoding="utf-8")
    print(f"scritto {a.out}")
    print(f"aperture: {len([g for g in gruppi if g.liberi])}")
    print(f"voci in coda scoperta: {len(scoperta)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 2: aggiorna la cache GitHub e genera**

Run:
```bash
gh auth status
python tools/decision-log/fetch_github_cache.py
python tools/editor-sessions/build_agenda.py --out build/ordine-del-giorno.md
```
Expected: il file esiste, e le aperture sono in numero molto minore delle sedute del vecchio registro. **Se un `requires` dice «non e' nella cache», la cache del decision-log non contiene quella issue** — è normale, perché quella cache segue i riferimenti del Decision Log, non i tuoi. Registralo come limite noto: la fetta 4 gli darà una cache propria.

- [ ] **Step 3: leggi l'uscita e giudicala**

Apri `build/ordine-del-giorno.md`. Tre domande, e le risposte vanno scritte nel messaggio di commit:
1. la prima apertura è quella che avresti scelto tu?
2. ogni voce bloccata ha un motivo che *sta in piedi*, o ce ne sono di bloccate da un `requires` sbagliato?
3. la coda scoperta contiene voci che dovrebbero essere cablate?

- [ ] **Step 4: scrivi il README**

```markdown
# Sedute componibili — il generatore

La seduta non e' un dato: e' il raggruppamento dei check che condividono un allestimento e i cui
prerequisiti sono soddisfatti. Questo strumento lo calcola.

```bash
python3 tools/decision-log/fetch_github_cache.py            # serve `gh` autenticata
python3 tools/editor-sessions/build_agenda.py               # -> build/ordine-del-giorno.md
python3 tools/editor-sessions/compare_legacy.py             # il gate della fetta 0
python3 -m unittest discover -s tools/editor-sessions -p '*_test.py'
```

Dipendenze: Python 3.12 e `pyyaml`. Nient'altro.

## Cosa NON fa

- **Non possiede l'esito di un check.** Quello e' di `docs/technical/test-manuali-pie.md`, e in questo
  schema manca il campo dove scriverlo.
- **Non e' l'owner delle sedute** finche' `docs/roadmap/sedute-mattoni.yaml` porta il marcatore
  `DRAFT — NON OWNER`.
- **Non committa la propria uscita.** `build/` e' ignorato: o rigeneri, o non esiste.
- **Non apre issue.** Quello arriva alla fetta 4, e solo con `--apply`.

## Perche' l'uscita non si committa

`editormap.shortlist.md` era una vista generata e committata, ed e' uscita dal repository con D-181
perche' nessuno la leggeva mentre invecchiava. Un file che devi rigenerare per vedere non puo'
mentirti sulla propria eta'.
```

- [ ] **Step 5: commit**

```bash
git add tools/editor-sessions/build_agenda.py tools/editor-sessions/README.md
git commit -m "feat(sedute): l'ordine del giorno della prossima apertura"
```

---

## Task 8: il gate della fetta 0

**Files:**
- Create: `tools/editor-sessions/compare_legacy.py`
- Test: `tools/editor-sessions/compare_legacy_test.py`

**Interfaces:**
- Consumes: `registry.load`, `pie_status.load`, `oracles.*`, `agenda.calcola`.
- Produces: `confronta(gruppi, scoperta, sessioni, stato) -> dict` con le chiavi `persi`, `guadagnati`, `aperture`, `sedute`.

`persi` sono i check che una seduta scritta convocava e che l'agenda non convoca: è **la misura che può falsificare il modello**. `guadagnati` sono quelli che l'agenda convoca e che nessuna seduta nominava.

- [ ] **Step 1: scrivi il test che fallisce**

```python
# tools/editor-sessions/compare_legacy_test.py
"""Il confronto dice cosa il calcolo perde rispetto alle sedute scritte a mano."""
from __future__ import annotations

import unittest

import compare_legacy
from agenda import Gruppo, Voce

STATO = {
    "PIE-A": {"stato": "⏳", "release": False},
    "PIE-B": {"stato": "⏳", "release": False},
    "PIE-C": {"stato": "⏳", "release": False},
    "PIE-VERDE": {"stato": "✅", "release": False},
}

GRUPPI = [Gruppo(setup="SET-X", liberi=[Voce("PIE-A", (), False)], bloccati=[Voce("PIE-B", ("x",), False)])]

SESSIONI = [
    {"id": "U1", "verifies": ["PIE-A", "PIE-C", "PIE-VERDE"]},
    {"id": "U2", "verifies": ["PIE-B"]},
]


class ConfrontoTest(unittest.TestCase):
    def setUp(self):
        self.r = compare_legacy.confronta(GRUPPI, ["PIE-SCOP"], SESSIONI, STATO)

    def test_un_check_convocato_solo_dal_vecchio_e_perso(self):
        self.assertEqual(self.r["persi"], ["PIE-C"])

    def test_un_bloccato_stampato_non_conta_come_perso(self):
        self.assertNotIn("PIE-B", self.r["persi"])

    def test_un_verde_non_conta_come_perso(self):
        self.assertNotIn("PIE-VERDE", self.r["persi"])

    def test_la_coda_scoperta_non_e_un_guadagno(self):
        self.assertEqual(self.r["guadagnati"], [])

    def test_riporta_quante_aperture_contro_quante_sedute(self):
        self.assertEqual(self.r["aperture"], 1)
        self.assertEqual(self.r["sedute"], 2)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: lancia il test e verifica che fallisca**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: FAIL con `ModuleNotFoundError: No module named 'compare_legacy'`

- [ ] **Step 3: scrivi l'implementazione minima**

```python
# tools/editor-sessions/compare_legacy.py
"""Il gate della fetta 0: il calcolo riproduce le sedute scritte a mano?

`persi` sono i check che una seduta convocava e che l'agenda non nomina, ne'
fra i liberi ne' fra i bloccati. Sono la misura che puo' FALSIFICARE il
modello: se non e' vuota, o manca una riga di wiring, o il raggruppamento e'
sbagliato. Un check verde non e' perso — e' finito. Un check bloccato e
stampato non e' perso — e' dichiarato.

    python3 tools/editor-sessions/compare_legacy.py
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import yaml

import agenda
import oracles
import pie_status
import registry

VECCHIO = Path("docs/roadmap/editor-sessions.yaml")
CACHE = Path("tools/decision-log/github-cache.json")


def confronta(gruppi, scoperta, sessioni: list[dict], stato: dict[str, dict]) -> dict:
    nominati = {v.check for g in gruppi for v in g.liberi + g.bloccati}
    vecchi = {
        c
        for s in sessioni
        for c in (s.get("verifies") or [])
        if stato.get(c, {}).get("stato") not in (pie_status.VERDE, None)
    }
    return {
        "persi": sorted(vecchi - nominati),
        "guadagnati": sorted(nominati - vecchi),
        "aperture": len([g for g in gruppi if g.liberi]),
        "sedute": len(sessioni),
        "scoperta": sorted(scoperta),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mattoni", default=registry.MATTONI, type=Path)
    ap.add_argument("--vecchio", default=VECCHIO, type=Path)
    ap.add_argument("--registro", default=pie_status.REGISTRO, type=Path)
    ap.add_argument("--cache", default=CACHE, type=Path)
    a = ap.parse_args()

    setups, wires = registry.load(a.mattoni)
    stato = pie_status.load(a.registro)
    cache = json.loads(a.cache.read_text(encoding="utf-8"))
    inventario = oracles.asset_tracciati()
    chiuse, note = oracles.issue_chiuse(cache), oracles.issue_note(cache)
    gruppi, scoperta = agenda.calcola(
        setups, wires, stato, lambda r: oracles.valuta(r, inventario, chiuse, note)
    )

    vecchio = yaml.safe_load(a.vecchio.read_text(encoding="utf-8")) or {}
    r = confronta(gruppi, scoperta, vecchio.get("sessions") or [], stato)

    print(f"aperture calcolate: {r['aperture']}   sedute scritte: {r['sedute']}")
    print("PERSI — convocati da una seduta e non dall'agenda:")
    for c in r["persi"] or ["  (nessuno)"]:
        print(f"  {c}")
    print("GUADAGNATI — convocati dall'agenda e da nessuna seduta:")
    for c in r["guadagnati"] or ["  (nessuno)"]:
        print(f"  {c}")
    print("CODA SCOPERTA — nel registro PIE e in nessuna riga di wiring:")
    for c in r["scoperta"] or ["  (nessuna)"]:
        print(f"  {c}")

    if r["persi"]:
        sys.exit(
            "\nIl calcolo perde dei check. Prima di proseguire con le fette 1-5, "
            "per ognuno stabilisci se manca una riga di wiring o se il raggruppamento e' sbagliato."
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 4: lancia i test e verifica che passino**

Run: `python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v`
Expected: PASS, quarantatre test in totale (cinque in questo file).

- [ ] **Step 5: commit**

```bash
git add tools/editor-sessions/compare_legacy.py tools/editor-sessions/compare_legacy_test.py
git commit -m "feat(sedute): il confronto che puo falsificare il modello"
```

---

## Task 9: eseguire il gate e registrare l'esito

**Files:**
- Modify: `docs/roadmap/plans/sedute-componibili-design-2026-09-11.md` — aggiungere una sezione `## 12. Esito della fetta 0`

Questo task non produce codice. Produce **un verdetto misurato**, che è il deliverable della fetta 0.

- [ ] **Step 1: esegui il confronto**

Run:
```bash
python tools/editor-sessions/compare_legacy.py 2>&1 | tee build/fetta-0-esito.txt
```

- [ ] **Step 2: chiudi ogni check perso**

Per **ogni** ID nella lista `PERSI`, apri la seduta che lo nominava in `editor-sessions.yaml` e stabilisci quale delle due cose è vera:

1. **manca una riga di wiring** → aggiungila a `sedute-mattoni.yaml` e rilancia;
2. **il raggruppamento è sbagliato** → l'allestimento dedotto non è quello giusto; correggi il `setup` di quella riga e rilancia.

Se scopri una terza possibilità — un check che *non deve* essere convocato — scrivila, perché è un'informazione che il vecchio registro non conteneva.

⚠️ **Non svuotare la lista abbassando il criterio.** La lista vuota vale solo se ogni voce è stata guardata.

- [ ] **Step 3: scrivi l'esito nella spec**

Aggiungi in fondo a `sedute-componibili-design-2026-09-11.md`:

```markdown
---

## 12. Esito della fetta 0

**Misurato il <data>** su `<sha>`, col comando:

```bash
python tools/editor-sessions/compare_legacy.py
```

- **Aperture calcolate contro sedute scritte**: `<n>` contro `<m>`.
- **Persi**: `<elenco per ID, oppure «nessuno»>`. <Per ciascuno: cosa si è deciso e perché.>
- **Guadagnati**: `<elenco per ID>` — check che l'agenda convoca e che nessuna seduta nominava.
- **Coda scoperta**: `<elenco per ID>` — voci del registro PIE che nessuna riga di wiring cabla.

**Verdetto sul modello**: `REGGE` / `NON REGGE`, e la ragione.

⚠️ I numeri qui sopra sono l'esito di questo passaggio, non un totale da mantenere. Chi li rilegge
rilancia il comando.
```

- [ ] **Step 4: commit**

```bash
git add docs/roadmap/plans/sedute-componibili-design-2026-09-11.md
git commit -m "docs(sedute): l'esito misurato della fetta 0"
```

- [ ] **Step 5: fermati qui**

Le fette 1-5 — tipizzare i `requires`, migrare la prosa per setup, la coda scoperta, le issue convocanti, la dismissione — **non** appartengono a questo piano. Si pianificano dopo aver letto il verdetto del passo 3, perché è quel verdetto a dire se il modello va corretto prima di costruirci sopra.

---

## Cosa questo piano non copre

Dalla spec, e deliberatamente fuori dalla fetta 0:

- **Le issue convocanti** (§4.3) — fetta 4. Servono una cache propria, perché quella del decision-log segue i riferimenti del Decision Log e non i nostri.
- **La migrazione della prosa** (§5.2) — fetta 2. Nessuno dei task qui cancella `steps` o `notes`.
- **La dismissione delle cinque rappresentazioni** (§5.3, fetta 5) — incluso l'archiviazione del vecchio yaml e il marcatore di supersessione su `S0`–`S9`.
- **La voce di Decision Log** (§10) — si scrive quando il modello ha retto, non prima: una decisione su un modello falsificabile non ancora falsificato sarebbe una previsione.
- **Il gate CI** (§6) — escluso dal design, e comunque senza posto dove vivere: `.github/workflows/` non contiene workflow.
- **Gli oracoli alternativi di `anim` e `feature`.** La spec (§3.2) assegna ad `anim` la riga «Animazioni:» del piano `S0`–`S9`, e a `feature` un `git grep` misurato oppure lo stato di un checkpoint. **Nella fetta 0 tutti e quattro i tipi non-`asset` si risolvono con l'issue owner**, perché è l'unico oracolo già disponibile senza costruirne altri. È una semplificazione dichiarata, non una dimenticanza: chi cabla un `anim` o un `feature` in questa fetta gli dà una issue, e la fetta 1 aggiunge gli altri due oracoli.
