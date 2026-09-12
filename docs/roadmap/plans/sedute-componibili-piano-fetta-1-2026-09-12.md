# Sedute componibili — piano d'implementazione, fetta 1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** dichiarare i prerequisiti dei check cablati, tipizzati e con un oracolo ciascuno, così che un check che oggi non si può guardare **compaia bloccato** nell'ordine del giorno invece di essere convocato a vuoto.

**Architecture:** nessun modulo nuovo nel calcolo. La fetta aggiunge **dati** (`requires` nel registro dei mattoni), **rende eseguibili gli oracoli con issue** (la cache GitHub oggi non conosce quelle issue), **rende l'ignoranza un errore invece di un bloccante finto**, e **protegge il lavoro d'autore dal prossimo riseminio** (`seed_wiring --into`). Un solo modulo nuovo, `compare_rassegna.py`, che è il gate del verbale.

**Tech Stack:** Python 3.12.10 · pyyaml 6.0.3 · `unittest` della standard library · `gh` per la cache GitHub. Nessuna dipendenza da installare.

**Spec:** [`sedute-componibili-design-2026-09-11.md`](sedute-componibili-design-2026-09-11.md) — fetta **1** della tabella §5.3.

**Piano precedente:** [`sedute-componibili-piano-fetta-0-2026-09-11.md`](sedute-componibili-piano-fetta-0-2026-09-11.md)

> 🔑 **Convenzione sui numeri, dichiarata una volta.** Ogni conteggio in questo piano porta accanto
> il comando che lo produce ed è un **esito del passaggio corrente**, non un totale da mantenere.
> Chi rilegge **rimisura**. Dove un'enumerazione è possibile, si usano i **nomi**.

---

## Global Constraints

Ogni task eredita questi vincoli. I primi sei vengono dalla fetta 0 e non sono negoziabili; gli ultimi quattro nascono qui.

- **`docs/technical/test-manuali-pie.md` resta l'unico owner dell'esito di un check.** Nessuna struttura dati di questo piano ha un campo dove scrivere un esito atteso o raggiunto. Se ti trovi ad aggiungerlo, stai scrivendo nel file sbagliato.
- **Fino alla fetta 5, `docs/roadmap/editor-sessions.yaml` resta l'owner delle sedute.** `sedute-mattoni.yaml` porta in testa il marcatore `DRAFT — NON OWNER`, e **nessun task di questo piano lo rimuove**. Nessun task cancella, modifica o sposta il vecchio registro.
- **Rifiutare, non ignorare.** Un difetto esce con un errore che lo nomina, sul precedente di `D-182`. Un input malformato che produce un verde è il difetto peggiore di questa famiglia di strumenti.
- **`build/` non si committa** (`.gitignore:317`).
- **Niente totali volatili** in ciò che lo strumento scrive e in ciò che scrivi tu — issue, commit, PR, README (`AGENTS.md` §14). Si elencano gli **ID**. Un numero è ammesso solo come esito del passaggio corrente, col comando accanto.
- **Determinismo dell'output.** Ogni ordinamento è totale: a parità di chiave si ordina per ID. Due esecuzioni sugli stessi input producono byte identici.
- **Un tipo, un oracolo.** `asset:` interroga `git ls-files Content`; `mount|cue|anim|feature:` interrogano la cache GitHub e **nient'altro**. Quando i fatti veri su un nome sono due, si scrivono **due righe**, non un tipo che ne indovina due.
- **Un `requires` che non si può provare non si scrive.** Il default dello schema è «nessun prerequisito», e `oracles.py` lo dichiara: *«un prerequisito che non serve NON si dichiara»*. In quella direzione l'errore è reversibile — un check convocato di troppo si vede aprendo l'Editor. Nell'altra, un check **sparisce in silenzio**, ed è il rischio che §7 della spec nomina per primo.
- **Nessun rename d'eroe.** `D-321` ha differito la migrazione a `#2297`. Un `requires` non si usa per segnalare che il registro PIE cita un nome ritirato: quello è un difetto di prosa, e ha già un owner.
- **Su Windows, ogni comando che stampa uno stato apre con `sys.stdout.reconfigure(encoding='utf-8')`.** La console è `cp1252` e le emoji del registro la fanno esplodere con `UnicodeEncodeError`. I file si scrivono già con `encoding='utf-8'` esplicito.

---

## Il difetto che questa fetta chiude

Misurato il 2026-09-12 su `394e7f5a`:

```bash
python - <<'PY'
import sys; sys.path.insert(0, 'tools/editor-sessions')
import registry
_, w = registry.load()
print(sum(1 for x in w if x.requires), "requires dichiarati su", len(w), "righe di wiring")
PY
```

→ **0** su **169**. La promessa centrale del design — *«`U30` non venga convocata finché `WBP_RT_PauseMenu` non esiste»* — non è mai stata esercitata. `PIE-V01-FRONTEND-PAUSE` è cablato su `SET-FRONTEND`, il suo widget non è tracciato, e oggi l'ordine del giorno lo convoca lo stesso.

E tre difetti che la ricognizione ha trovato, tutti da chiudere qui:

1. **Gli oracoli con issue non sono eseguibili.** `tools/decision-log/github-cache.json` è popolata dai `#nnn` che compaiono nel Decision Log, e non conosce `#2697` né `#2454`. Con l'oracolo attuale un `mount:…#2697` non dice «bloccato perché la issue è aperta»: dice «non è nella cache». Un bloccante per colpa dell'attrezzatura è il gemello del verde falso.
2. **La spec descrive un codice che non esiste.** §3.2 classifica `WBP_RT_EventLogRight` come `mount`, ma il widget **non è tracciato** (esistono `WBP_RT_EventLog` e `WBP_RT_EventLine`); e l'esempio di §3.3, `requires: [ mount:WBP_RT_TacticalHUD ]`, viene **rifiutato** da `oracles.valuta`, che pretende `nome#numero`.
3. **Il prossimo riseminio cancellerebbe questa fetta.** `seed_wiring.py` emette righe con `check`/`setup`/`issue` e nessun `requires`; l'innesto nel registro è manuale, ed è già stato fatto due volte durante la fetta 0. Rifarlo dopo questa fetta distrugge il lavoro d'autore.

---

## File Structure

| File | Responsabilità | In questa fetta |
|---|---|---|
| `docs/roadmap/plans/sedute-componibili-design-2026-09-11.md` | la spec | **rettifica** §3.2, §3.3 |
| `docs/roadmap/sedute-mattoni.yaml` | i dati: `setups`, `wiring` | **rettifica** l'intestazione · **riceve** i `requires` |
| `docs/roadmap/plans/sedute-componibili-rassegna-requires-2026-09-12.md` | il verbale della rassegna: una riga per check esaminato, con la prova | **crea** |
| `tools/editor-sessions/oracles.py` | un `requires` è soddisfatto o no | **`OracoloError`** sulla cache-miss |
| `tools/editor-sessions/oracles_test.py` | test del sopra | aggiorna |
| `tools/editor-sessions/build_agenda.py` | CLI: `build/ordine-del-giorno.md` | cattura `OracoloError` |
| `tools/editor-sessions/compare_legacy.py` | CLI: il gate della fetta 0 | cattura `OracoloError` e `RegistryError` |
| `tools/editor-sessions/seed_wiring.py` | CLI: semina il wiring | **`--into`** che fonde e preserva i `requires` |
| `tools/editor-sessions/seed_wiring_test.py` | test del sopra | aggiorna |
| `tools/editor-sessions/compare_rassegna.py` | CLI: il gate del verbale | **crea** |
| `tools/editor-sessions/compare_rassegna_test.py` | test del sopra | **crea** |
| `tools/editor-sessions/README.md` | come si lancia, cosa produce, cosa non fa | aggiorna |
| `tools/decision-log/fetch_github_cache.py` | la cache GitHub | **`--also`** |
| `tools/decision-log/fetch_github_cache_test.py` | test del sopra | **crea** |
| `tools/decision-log/github-cache.json` | la cache, versionata | **rigenera** |

Due comandi di test, perché due cartelle:

```bash
python -m unittest discover -s tools/editor-sessions -p '*_test.py' -v
python -m unittest discover -s tools/decision-log   -p '*_test.py' -v
```

---

## Task 1: le rettifiche — la spec smette di descrivere un codice che non esiste

Nessun codice, nessun test. È il primo task perché ogni task successivo cita la regola che qui viene scritta.

**Files:**
- Modify: `docs/roadmap/plans/sedute-componibili-design-2026-09-11.md` (§3.2, §3.3)
- Modify: `docs/roadmap/sedute-mattoni.yaml` (il blocco di commento in testa, sopra `setups:`)

**Interfaces:**
- Consumes: niente.
- Produces: la regola «un tipo, un oracolo; due fatti, due righe», che i Task 5 e 6 applicano.

- [ ] **Step 1: rettifica la tabella di §3.2**

Nel file della spec, **sostituisci** la tabella dei tipi con questa. Cambiano: `asset` (dice *package tracciato*, che è ciò che l'oracolo guarda, non *file*), `mount` (perde la pretesa che il widget esista), `anim` e `feature` (perdono oracoli alternativi che nessuno ha implementato). Solo `cue` resta com'era.

````markdown
| tipo | significato | oracolo |
|---|---|---|
| `asset` | il package non esiste fra quelli tracciati | `git ls-files Content`; i riferimenti *interni* ai `.uasset` restano di `tools/asset-refs` |
| `mount` | il widget non è montato dove il check lo cerca | l'issue owner |
| `cue` | l'evento esiste e nessuno lo disegna | l'issue owner — `PIE-VIS-DEFLECT`, `-INTERPOSE` → [#2454] |
| `anim` | la clip serve al verdetto e non c'è | l'issue owner |
| `feature` | la condizione non è ottenibile | l'issue owner |

**Un tipo, un oracolo.** `asset` interroga il filesystem; gli altri quattro interrogano la cache
GitHub e nient'altro. Quando i fatti veri su uno stesso nome sono **due** — il widget non esiste
*e* nessuno lo monta — si scrivono **due righe**, non un tipo che ne indovina due:

```yaml
requires: [ asset:WBP_RT_EventLogRight, mount:WBP_RT_EventLogRight#2697 ]
```

Il bloccante stampato nomina l'anello che ha ceduto davvero, e il giorno che il widget compare la
riga `mount` resta in piedi da sé: nessuno deve ricordarsi di riscrivere il prerequisito.

⚠️ **Rettificato il 2026-09-12** (fetta 1). Questa tabella diceva che `WBP_RT_EventLogRight` è un
caso `mount`, cioè un widget *esistente* e non montato. È falso: `git ls-files Content` non lo
traccia — esistono `WBP_RT_EventLog` e `WBP_RT_EventLine`. §1.5 lo diceva già giusto. E dava ad
`anim` e `feature` oracoli alternativi — la riga «Animazioni:» del piano `S0`–`S9`, un `git grep`,
un checkpoint — che né la fetta 0 né la fetta 1 hanno implementato: una promessa che invecchiava.
````

Il caso `feature` con un `git grep` misurato — il runbook `guida-seduta-u46-residui-g9.md`, dove `bHumanPlanning|WaitForPlayer|Interactive|PauseForPlanning` in `ScenarioHarness/` dà **0** — resta nella prosa di §3.2 come *esempio di come si arriva alla decisione*, non come oracolo eseguibile. Non cancellarlo: aggiungi «la misura giustifica il `requires`; a stabilirlo resta l'issue owner».

- [ ] **Step 2: rettifica l'esempio di §3.3**

L'esempio attuale è **rifiutato** dal codice: `oracles.valuta` alza `ValueError` su un tipo con issue senza `#numero`. Sostituisci il blocco yaml con uno che gira:

```yaml
wiring:
  - check: PIE-V01-SCREENHUD
    setup: SET-FRONTEND
    requires: [ asset:WBP_RT_EventLogRight, mount:WBP_RT_EventLogRight#2697 ]
    issue: 613
```

e aggiungi subito sotto:

```markdown
⚠️ **Rettificato il 2026-09-12** (fetta 1). L'esempio diceva `requires: [ mount:WBP_RT_TacticalHUD ]`,
che `oracles.valuta` rifiuta: i tipi con issue pretendono `nome#numero`, perché senza owner un
bloccante non ha nessuno che possa toglierlo. (`WBP_RT_TacticalHUD` esiste davvero ed è un caso
`mount` legittimo — gli mancava solo la issue.)
```

- [ ] **Step 3: rettifica l'intestazione del registro dei mattoni**

In `docs/roadmap/sedute-mattoni.yaml`, **sostituisci** il blocco `# Forma di un requires:` con:

```yaml
# Forma di un `requires` — un tipo, un oracolo:
#   asset:<Nome>              il package NON esiste fra quelli tracciati sotto Content/
#   mount:<Nome>#<issue>      il widget non e' montato dove il check lo cerca
#   cue:<Nome>#<issue>        l'evento esiste e nessuno lo disegna
#   anim:<Nome>#<issue>       la clip serve al verdetto e non c'e'
#   feature:<Nome>#<issue>    la condizione non e' ottenibile
#
# `asset` interroga `git ls-files Content`. Gli altri quattro interrogano la cache GitHub e
# NIENT'ALTRO: chi chiude la issue toglie il bloccante, ed e' l'unico modo in cui un check
# torna nell'agenda da solo. Un numero che la cache non conosce e' un ERRORE DURO, non un
# bloccante: rilancia
#   python tools/decision-log/fetch_github_cache.py --also docs/roadmap/sedute-mattoni.yaml
#
# Quando i fatti veri su uno stesso nome sono DUE, si scrivono DUE righe:
#   requires: [ asset:WBP_RT_EventLogRight, mount:WBP_RT_EventLogRight#2697 ]
#
# Un prerequisito che non serve NON si dichiara: l'assenza della riga e' gia' la dichiarazione.
# Ogni `requires` scritto qui ha una riga che lo giustifica nel verbale
# `plans/sedute-componibili-rassegna-requires-2026-09-12.md`, e `compare_rassegna.py` lo verifica.
```

- [ ] **Step 4: verifica che il registro si legga ancora**

Run:
```bash
python -c "import sys; sys.path.insert(0,'tools/editor-sessions'); import registry; s,w=registry.load(); print(len(s),'setups', len(w),'wire')"
```
Expected: stampa i due conteggi senza eccezioni. Hai toccato solo commenti, quindi devono essere gli stessi di prima.

- [ ] **Step 5: Commit**

```bash
git add docs/roadmap/plans/sedute-componibili-design-2026-09-11.md docs/roadmap/sedute-mattoni.yaml
git commit -m "docs(sedute): un tipo un oracolo, e la spec smette di descrivere un codice che non esiste"
```

---

## Task 2: la cache che non sa è un errore, non un bloccante

**Files:**
- Modify: `tools/editor-sessions/oracles.py`
- Test: `tools/editor-sessions/oracles_test.py`
- Modify: `tools/editor-sessions/build_agenda.py`
- Modify: `tools/editor-sessions/compare_legacy.py`

**Interfaces:**
- Consumes: `oracles.valuta(req, inventario, chiuse, note) -> Esito` (fetta 0).
- Produces: `oracles.OracoloError` · `oracles.COMANDO_FETCH: str`. Firma di `valuta` invariata; cambia solo cosa fa quando la cache non conosce il numero.

- [ ] **Step 1: aggiorna il test che oggi pretende un bloccante**

In `tools/editor-sessions/oracles_test.py`, **sostituisci** `IssueTest.test_issue_fuori_cache_blocca_e_lo_dichiara` con:

```python
    def test_issue_fuori_cache_viene_rifiutata_invece_di_bloccare(self):
        """Un bloccante per colpa dell'attrezzatura e' il gemello del verde falso.

        Se la cache non conosce il numero, l'oracolo non sa rispondere: fingere
        «bloccato» nasconde il check con una ragione che non e' la sua, e
        nessuno rilancia il fetch perche' l'agenda sembra sana.
        """
        with self.assertRaises(oracles.OracoloError) as e:
            oracles.valuta("cue:Deflect#9999", INVENTARIO, CHIUSE, NOTE)
        self.assertIn("#9999", str(e.exception))
        self.assertIn("fetch_github_cache.py", str(e.exception))
        self.assertIn("--also", str(e.exception))
```

- [ ] **Step 2: esegui il test e verifica che fallisca**

Run: `python -m unittest discover -s tools/editor-sessions -p 'oracles_test.py' -v`
Expected: FAIL con `AttributeError: module 'oracles' has no attribute 'OracoloError'`.

- [ ] **Step 3: implementa**

In `tools/editor-sessions/oracles.py`, sotto `TIPI`, aggiungi:

```python
COMANDO_FETCH = (
    "python tools/decision-log/fetch_github_cache.py "
    "--also docs/roadmap/sedute-mattoni.yaml"
)


class OracoloError(Exception):
    """L'oracolo non sa rispondere. Si rifiuta, non si finge un bloccante.

    La cache GitHub e' versionata e popolata dai `#nnn` che i registri citano.
    Se manca il numero che un `requires` nomina, l'unica risposta onesta e'
    fermarsi: «bloccato» sarebbe vero per il motivo sbagliato, il check
    uscirebbe dall'ordine del giorno, e chi legge non avrebbe modo di sapere
    che manca un `fetch`. Precedente: D-182.
    """
```

e nella funzione `valuta`, **sostituisci**:

```python
    if n not in note:
        return Esito(False, f"{tipo} {nome}: #{n} non e' nella cache — rilancia il fetch")
```

con:

```python
    if n not in note:
        raise OracoloError(
            f"{req}: #{n} non e' nella cache GitHub, quindi non si sa se sia aperta o chiusa. "
            f"Rilancia:\n    {COMANDO_FETCH}"
        )
```

- [ ] **Step 4: esegui il test e verifica che passi**

Run: `python -m unittest discover -s tools/editor-sessions -p 'oracles_test.py' -v`
Expected: PASS.

- [ ] **Step 5: fai catturare l'errore ai due eseguibili**

In `tools/editor-sessions/build_agenda.py`, **sostituisci**:

```python
    except (agenda.AgendaError, registry.RegistryError, ValueError) as e:
```

con:

```python
    except (agenda.AgendaError, registry.RegistryError, oracles.OracoloError, ValueError) as e:
```

In `tools/editor-sessions/compare_legacy.py`, **sostituisci** il blocco che oggi non protegge niente:

```python
    setups, wires = registry.load(a.mattoni)
    stato = pie_status.load(a.registro)
    cache = json.loads(a.cache.read_text(encoding="utf-8"))
    inventario = oracles.asset_tracciati()
    chiuse, note = oracles.issue_chiuse(cache), oracles.issue_note(cache)
    gruppi, scoperta = agenda.calcola(
        setups, wires, stato, lambda r: oracles.valuta(r, inventario, chiuse, note)
    )
```

con:

```python
    stato = pie_status.load(a.registro)
    cache = json.loads(a.cache.read_text(encoding="utf-8"))
    inventario = oracles.asset_tracciati()
    chiuse, note = oracles.issue_chiuse(cache), oracles.issue_note(cache)
    try:
        setups, wires = registry.load(a.mattoni)
        gruppi, scoperta = agenda.calcola(
            setups, wires, stato, lambda r: oracles.valuta(r, inventario, chiuse, note)
        )
    except (agenda.AgendaError, registry.RegistryError, oracles.OracoloError, ValueError) as e:
        sys.exit(f"calcolo rifiutato: {e}")
```

Il gate deve **fallire col nome del difetto**, non con un traceback: chi lo lancia in una sessione di verifica legge l'ultima riga.

- [ ] **Step 6: esegui tutti i test e i due gate**

Run:
```bash
python -m unittest discover -s tools/editor-sessions -p '*_test.py'
python tools/editor-sessions/compare_legacy.py
python tools/editor-sessions/build_agenda.py --out build/ordine-del-giorno.md
```
Expected: test `OK`; `compare_legacy` esce 0 con `PERSI (nessuno)`; `build_agenda` scrive il file. Nessun `requires` è ancora dichiarato, quindi il nuovo errore non può scattare: è la prova che il cambiamento è inerte finché i dati non arrivano.

- [ ] **Step 7: Commit**

```bash
git add tools/editor-sessions/oracles.py tools/editor-sessions/oracles_test.py tools/editor-sessions/build_agenda.py tools/editor-sessions/compare_legacy.py
git commit -m "fix(sedute): una cache che non sa il numero si ferma, invece di fingere un bloccante"
```

---

## Task 3: la cache impara a leggere anche i prerequisiti

`fetch_github_cache.py` raccoglie i `#nnn` dal solo Decision Log. I `requires` ne citano altri. Un secondo file di cache sarebbe una seconda rappresentazione dello stesso fatto — il difetto contro cui è nato tutto questo design — quindi si allarga la popolazione di quella che esiste.

**Files:**
- Modify: `tools/decision-log/fetch_github_cache.py`
- Test: `tools/decision-log/fetch_github_cache_test.py` (nuovo)

**Interfaces:**
- Consumes: `ISSUE_RE`, `referenced(log)` (già nel file).
- Produces: `referenced_extra(path: Path) -> list[int]` · l'argomento `--also PATH`, ripetibile.

⚠️ **Nessun default nascosto verso `docs/roadmap/`.** Un tool del Decision Log non deve dipendere di soppiatto da un file di roadmap: chi lo lancia passa `--also` di proposito, e chi lo dimentica lo scopre dall'errore duro del Task 2, che porta il comando completo.

- [ ] **Step 1: scrivi il test che fallisce**

```python
# tools/decision-log/fetch_github_cache_test.py
"""La cache raccoglie i `#nnn` anche fuori dal Decision Log.

Serve a chi dichiara un prerequisito con un issue owner: quel numero DEVE
entrare nella cache, altrimenti l'oracolo che lo legge si rifiuta di
rispondere (`oracles.OracoloError`) e l'ordine del giorno non si calcola.
"""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import fetch_github_cache


class ReferencedExtraTest(unittest.TestCase):
    def scrivi(self, testo: str) -> Path:
        d = tempfile.mkdtemp()
        p = Path(d) / "mattoni.yaml"
        p.write_text(testo, encoding="utf-8")
        return p

    def test_raccoglie_i_numeri_dai_requires(self):
        p = self.scrivi(
            "wiring:\n"
            "  - check: PIE-VIS-SIGHTWALL\n"
            "    requires: [ asset:WBP_RT_EventLogRight, mount:WBP_RT_EventLogRight#2697 ]\n"
            "  - check: PIE-VIS-DEFLECT\n"
            "    requires: [ cue:Deflect#2454 ]\n"
        )
        self.assertEqual(fetch_github_cache.referenced_extra(p), [2454, 2697])

    def test_ordina_e_deduplica(self):
        p = self.scrivi("a #2697\nb #2454\nc #2697\n")
        self.assertEqual(fetch_github_cache.referenced_extra(p), [2454, 2697])

    def test_un_file_senza_riferimenti_non_e_un_errore(self):
        p = self.scrivi("wiring:\n  - check: PIE-X\n    setup: SET-A\n")
        self.assertEqual(fetch_github_cache.referenced_extra(p), [])

    def test_il_campo_issue_senza_cancelletto_non_viene_raccolto(self):
        """`issue: 613` e' un riferimento d'attribuzione, non un oracolo.

        Solo un `#nnn` dichiara che qualcuno deve chiudere qualcosa perche' un
        check torni guardabile. Raccogliere anche `issue:` gonfierebbe la cache
        con numeri che nessun oracolo interroga.
        """
        p = self.scrivi("wiring:\n  - check: PIE-X\n    issue: 613\n")
        self.assertEqual(fetch_github_cache.referenced_extra(p), [])


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: esegui il test e verifica che fallisca**

Run: `python -m unittest discover -s tools/decision-log -p '*_test.py' -v`
Expected: FAIL con `AttributeError: module 'fetch_github_cache' has no attribute 'referenced_extra'`.

- [ ] **Step 3: implementa**

In `tools/decision-log/fetch_github_cache.py`, sotto `referenced`, aggiungi:

```python
def referenced_extra(path: Path) -> list[int]:
    """I `#nnn` che compaiono in un file qualunque.

    `referenced` sopra legge SOLO le righe del Decision Log, perche' quella e'
    la popolazione della vista HTML. Chi dichiara un prerequisito con un issue
    owner — `mount:<Nome>#<issue>` in `docs/roadmap/sedute-mattoni.yaml` — cita
    numeri che il Decision Log non nomina, e senza di essi l'oracolo di
    `tools/editor-sessions/oracles.py` si rifiuta di rispondere.
    """
    return sorted({int(n) for n in ISSUE_RE.findall(path.read_text(encoding="utf-8"))})
```

e in `main`, **sostituisci**:

```python
    ap.add_argument("--repo", default=REPO)
    args = ap.parse_args()

    numbers = referenced(args.log)
    print(f"{len(numbers)} riferimenti citati dal registro")
```

con:

```python
    ap.add_argument("--repo", default=REPO)
    ap.add_argument(
        "--also",
        action="append",
        default=[],
        type=Path,
        help="un altro file da cui raccogliere i #nnn (ripetibile). "
        "Per i prerequisiti delle sedute: --also docs/roadmap/sedute-mattoni.yaml",
    )
    args = ap.parse_args()

    trovati = set(referenced(args.log))
    print(f"{len(trovati)} riferimenti citati dal registro")
    for extra in args.also:
        # Rifiutare, non ignorare: un --also lasciato cadere produrrebbe una cache
        # incompleta che sembra completa. Precedente: D-182, dove --wiki-root esce
        # con sys.exit(2) invece di venire ignorato.
        if not extra.exists():
            sys.exit(f"--also: file non trovato: {extra}")
        aggiunti = set(referenced_extra(extra))
        print(f"  +{len(aggiunti - trovati)} da {extra}")
        trovati |= aggiunti
    numbers = sorted(trovati)
```

- [ ] **Step 4: esegui il test e verifica che passi**

Run: `python -m unittest discover -s tools/decision-log -p '*_test.py' -v`
Expected: PASS, quattro test.

- [ ] **Step 5: verifica che l'argomento mancante si rifiuti**

Run: `python tools/decision-log/fetch_github_cache.py --also docs/roadmap/non-esiste.yaml`
Expected: esce non-zero con `--also: file non trovato: docs/roadmap/non-esiste.yaml`, **senza** aver toccato la rete.

- [ ] **Step 6: Commit**

```bash
git add tools/decision-log/fetch_github_cache.py tools/decision-log/fetch_github_cache_test.py
git commit -m "feat(decision-log): --also, perche' i prerequisiti citano issue che il Decision Log non nomina"
```

---

## Task 4: `--into`, perché il prossimo seme non deve cancellare il giudizio

Durante la fetta 0 l'innesto del wiring nel registro è stato manuale, e si è dovuto rifare a ogni riallineamento con `main`. Dopo questa fetta il registro contiene anche `requires` scritti a mano: **rifare l'innesto a mano li perde**.

**Files:**
- Modify: `tools/editor-sessions/seed_wiring.py`
- Test: `tools/editor-sessions/seed_wiring_test.py`

**Interfaces:**
- Consumes: `semina(sessioni) -> tuple[list[dict], dict, dict]` (fetta 0).
- Produces: `requires_esistenti(testo: str) -> dict[str, list[str]]` · `rendi_righe(righe: list[dict]) -> str` · `innesta(testo: str, righe: list[dict]) -> str` · `INTESTAZIONE_WIRING: str` · l'argomento `--into PATH`.

⚠️ **Non è un round-trip yaml.** `yaml.safe_dump` cancellerebbe tutti i commenti del file — fra cui il marcatore `DRAFT — NON OWNER`, che è l'unica cosa che tiene **un solo owner in ogni istante**. E produce sequenze non indentate, che non è lo stile del file. Quindi la testa si conserva **come testo, byte per byte**, e le righe si rendono a mano.

- [ ] **Step 1: scrivi i test che falliscono**

Aggiungi in coda a `tools/editor-sessions/seed_wiring_test.py`, prima di `if __name__ == "__main__":`

```python
TESTA = """# Sedute — i MATTONI: allestimenti e cablaggio.
#
# ⛔ DRAFT — NON OWNER.

setups:
  - id: SET-A
    map: L_Uno
"""

VECCHIO = TESTA + """
wiring:
  # un commento che il seme riscrive
  - check: PIE-A
    setup: SET-A
    requires: [ asset:WBP_RT_PauseMenu ]
  - check: PIE-B
    setup: SET-A
    issue: 613
"""


class RequiresEsistentiTest(unittest.TestCase):
    def test_legge_i_requires_per_check(self):
        self.assertEqual(
            seed_wiring.requires_esistenti(VECCHIO),
            {"PIE-A": ["asset:WBP_RT_PauseMenu"]},
        )

    def test_un_registro_senza_requires_da_una_mappa_vuota(self):
        self.assertEqual(seed_wiring.requires_esistenti(TESTA + "\nwiring: []\n"), {})


class RendiRigheTest(unittest.TestCase):
    def test_lo_stile_e_quello_del_file_e_i_requires_stanno_in_flusso(self):
        reso = seed_wiring.rendi_righe(
            [
                {"check": "PIE-A", "setup": "SET-A", "requires": ["asset:X", "mount:X#7"]},
                {"check": "PIE-B", "setup": "SET-A", "issue": 613},
            ]
        )
        self.assertEqual(
            reso,
            "  - check: PIE-A\n"
            "    setup: SET-A\n"
            "    requires: [ asset:X, mount:X#7 ]\n"
            "  - check: PIE-B\n"
            "    setup: SET-A\n"
            "    issue: 613\n",
        )

    def test_cio_che_rende_si_rilegge(self):
        """La forma scritta deve tornare dentro `yaml.safe_load` uguale a com'e' uscita."""
        righe = [{"check": "PIE-A", "setup": "SET-A", "requires": ["asset:X", "mount:X#7"]}]
        riletto = yaml.safe_load("wiring:\n" + seed_wiring.rendi_righe(righe))
        self.assertEqual(riletto["wiring"], righe)


class InnestaTest(unittest.TestCase):
    def test_la_testa_si_conserva_byte_per_byte(self):
        nuovo = seed_wiring.innesta(VECCHIO, [{"check": "PIE-A", "setup": "SET-A"}])
        self.assertTrue(nuovo.startswith(TESTA))
        self.assertIn("DRAFT — NON OWNER", nuovo)

    def test_i_requires_esistenti_sopravvivono_al_seme(self):
        """Il seme non sa niente dei prerequisiti: se non li preservasse, li cancellerebbe."""
        nuovo = seed_wiring.innesta(
            VECCHIO,
            [{"check": "PIE-A", "setup": "SET-B"}, {"check": "PIE-B", "setup": "SET-A"}],
        )
        riletto = {r["check"]: r for r in yaml.safe_load(nuovo)["wiring"]}
        self.assertEqual(riletto["PIE-A"]["requires"], ["asset:WBP_RT_PauseMenu"])
        self.assertEqual(riletto["PIE-A"]["setup"], "SET-B")  # il seme aggiorna il setup
        self.assertNotIn("requires", riletto["PIE-B"])

    def test_un_requires_che_perde_la_propria_riga_ferma_tutto(self):
        """Il caso pericoloso: il seme non produce piu' la riga che portava il giudizio.

        Scriverla via sarebbe una perdita silenziosa di lavoro d'autore. Si
        rifiuta col nome del check, e chi legge decide.
        """
        with self.assertRaises(seed_wiring.InnestoError) as e:
            seed_wiring.innesta(VECCHIO, [{"check": "PIE-B", "setup": "SET-A"}])
        self.assertIn("PIE-A", str(e.exception))

    def test_e_idempotente(self):
        righe = [{"check": "PIE-A", "setup": "SET-A"}, {"check": "PIE-B", "setup": "SET-A"}]
        una = seed_wiring.innesta(VECCHIO, righe)
        due = seed_wiring.innesta(una, righe)
        self.assertEqual(una, due)

    def test_un_registro_senza_sezione_wiring_si_rifiuta(self):
        with self.assertRaises(seed_wiring.InnestoError):
            seed_wiring.innesta(TESTA, [{"check": "PIE-A", "setup": "SET-A"}])
```

e in testa al file, accanto agli import esistenti, assicurati che ci siano:

```python
import yaml

import seed_wiring
```

- [ ] **Step 2: esegui i test e verifica che falliscano**

Run: `python -m unittest discover -s tools/editor-sessions -p 'seed_wiring_test.py' -v`
Expected: FAIL. `unittest` esegue le classi in ordine alfabetico, quindi il primo errore è `AttributeError: module 'seed_wiring' has no attribute 'innesta'` — non `requires_esistenti`. I test di `DeduzioneTest` e delle classi della fetta 0 devono restare **verdi**: se uno di loro fallisce, hai rotto qualcosa che c'era già.

- [ ] **Step 3: implementa**

In `tools/editor-sessions/seed_wiring.py`, sotto `CAMPI_PROSA`, aggiungi:

```python
class InnestoError(Exception):
    """L'innesto perderebbe qualcosa. Si rifiuta, non si sovrascrive."""


INTESTAZIONE_WIRING = """wiring:
  # Prodotto da `python tools/editor-sessions/seed_wiring.py --into docs/roadmap/sedute-mattoni.yaml`.
  # Il seme e' l'euristica sulla prosa PIU' due tabelle di giudizio dichiarato che vivono nel
  # sorgente del seminatore, non qui:
  #   ALLESTIMENTO_DICHIARATO  le sedute la cui prosa non dice dove si guarda
  #   CABLAGGIO_DICHIARATO     i check che due sedute rivendicano con allestimenti DIVERSI
  # ⚠️ Correggere `check`, `setup` o `issue` QUI e' inutile: la decisione sta nelle tabelle, e il
  #    prossimo seme ricalcola.
  # 🔑 I `requires` invece si scrivono QUI, e il seme li PRESERVA: sono giudizio d'autore su un
  #    prerequisito, non una deduzione dalla prosa. Ognuno ha la propria riga di prova nel verbale
  #    `plans/sedute-componibili-rassegna-requires-2026-09-12.md`, e `compare_rassegna.py` lo verifica.
"""
```

e in coda al modulo, prima di `def main`:

```python
def requires_esistenti(testo: str) -> dict[str, list[str]]:
    """I `requires` gia' scritti, per check.

    Legge il registro COME DATI: i commenti non contano, conta cio' che
    `registry.load` leggerebbe. Se un giudizio e' scritto in un commento, per
    questo strumento non esiste — ed e' corretto, perche' non esiste nemmeno
    per il calcolo.
    """
    raw = yaml.safe_load(testo) or {}
    return {
        r["check"]: list(r["requires"])
        for r in (raw.get("wiring") or [])
        if r.get("requires")
    }


def rendi_righe(righe: list[dict]) -> str:
    """Le righe nello stile del file: due spazi di rientro, `requires` in flusso.

    Non si usa `yaml.safe_dump`: produce sequenze non indentate, che non e' lo
    stile del registro, e trasformerebbe ogni riseminio in un diff totale
    invece che nel diff di cio' che e' cambiato davvero.
    """
    out: list[str] = []
    for r in righe:
        out.append(f"  - check: {r['check']}")
        out.append(f"    setup: {r['setup']}")
        if r.get("requires"):
            out.append("    requires: [ " + ", ".join(r["requires"]) + " ]")
        if r.get("issue") is not None:
            out.append(f"    issue: {r['issue']}")
    return "\n".join(out) + "\n"


def innesta(testo: str, righe: list[dict]) -> str:
    """Riscrive la SOLA sezione `wiring:`, conservando tutto cio' che sta sopra.

    Un round-trip yaml cancellerebbe i commenti — fra cui il marcatore
    `DRAFT — NON OWNER`, che e' l'unica cosa che tiene UN SOLO owner in ogni
    istante. Quindi la testa si conserva come TESTO, byte per byte.

    I `requires` gia' presenti si riportano sulle righe nuove. Se una riga che
    ne portava uno non viene piu' prodotta dal seme, l'innesto si RIFIUTA: quel
    giudizio e' lavoro d'autore, e perderlo in silenzio e' esattamente cio' che
    questa funzione esiste per impedire.
    """
    i = testo.find("\nwiring:")
    if i < 0:
        raise InnestoError("il registro non contiene una sezione `wiring:`")

    vecchi = requires_esistenti(testo)
    prodotti = {r["check"] for r in righe}
    orfani = sorted(set(vecchi) - prodotti)
    if orfani:
        raise InnestoError(
            "questi check portano un `requires` e il seme non produce piu' la loro riga: "
            + " ".join(orfani)
            + ". Decidi prima di riseminare: o torna la riga, o il giudizio si sposta."
        )

    arricchite = []
    for r in righe:
        nuova = dict(r)
        if r["check"] in vecchi:
            nuova["requires"] = vecchi[r["check"]]
        arricchite.append(nuova)

    return testo[: i + 1] + INTESTAZIONE_WIRING + rendi_righe(arricchite)
```

- [ ] **Step 4: esegui i test e verifica che passino**

Run: `python -m unittest discover -s tools/editor-sessions -p 'seed_wiring_test.py' -v`
Expected: PASS.

- [ ] **Step 5: aggiungi l'argomento `--into`**

In `main`, **sostituisci**:

```python
    ap.add_argument("--out", default=Path("build/wiring-seminato.yaml"), type=Path)
    a = ap.parse_args()
```

con:

```python
    ap.add_argument("--out", default=Path("build/wiring-seminato.yaml"), type=Path)
    ap.add_argument(
        "--into",
        type=Path,
        help="innesta la sezione `wiring:` in questo registro, preservando i `requires` gia' scritti",
    )
    a = ap.parse_args()
```

e, subito prima di `return 0` in `main`, aggiungi:

```python
    if a.into is not None:
        if not a.into.exists():
            sys.exit(f"--into: registro non trovato: {a.into}")
        prima = a.into.read_text(encoding="utf-8")
        try:
            dopo = innesta(prima, righe)
        except InnestoError as e:
            sys.exit(f"innesto rifiutato: {e}")
        if dopo == prima:
            print(f"{a.into}: gia' allineato, nessuna scrittura")
        else:
            a.into.write_text(dopo, encoding="utf-8")
            print(f"innestato in {a.into}: rileggi con `git diff` prima di committare")
```

⚠️ Il seme **non** scrive righe per gli orfani e per i conflitti, e non cambia con `--into`: restano stampati e non cablati. `--into` innesta ciò che `semina` ha prodotto, né più né meno.

- [ ] **Step 6: esegui il seme sul registro vero, due volte**

Run:
```bash
python tools/editor-sessions/seed_wiring.py --into docs/roadmap/sedute-mattoni.yaml
git diff --stat docs/roadmap/sedute-mattoni.yaml
python tools/editor-sessions/seed_wiring.py --into docs/roadmap/sedute-mattoni.yaml
```
Expected: la prima riscrive la sezione (il diff tocca solo `wiring:` e il suo commento, **mai** la testa né `setups:`); la seconda stampa `gia' allineato, nessuna scrittura`. Se il diff tocca `setups:` o il marcatore `DRAFT — NON OWNER`, **fermati**: `innesta` ha sbagliato il punto di taglio.

- [ ] **Step 7: rilancia i gate**

Run:
```bash
python -m unittest discover -s tools/editor-sessions -p '*_test.py'
python tools/editor-sessions/compare_legacy.py
```
Expected: test `OK`; `compare_legacy` esce 0 con `PERSI (nessuno)`.

> 🔑 Se `PERSI` **non** è vuota, non è un difetto del tuo lavoro: è `main` che si è mosso sotto il
> cablaggio. È successo tre volte su tre durante la fetta 0. Riallinea, rilancia `--into`, rilancia
> il gate, e **registra l'accaduto** nell'esito del Task 7 — è il caso d'uso, non un incidente.

- [ ] **Step 8: Commit**

```bash
git add tools/editor-sessions/seed_wiring.py tools/editor-sessions/seed_wiring_test.py docs/roadmap/sedute-mattoni.yaml
git commit -m "feat(sedute): --into innesta il seme e preserva i requires, invece di cancellarli"
```

---

## Task 5: il gate del verbale — un bloccante senza prova non passa

Il verbale della rassegna (Task 6) è **archeologia datata**: dichiara chi è stato esaminato il 2026-09-12, non possiede niente, e non va tenuto aggiornato. Ma un `requires` **senza** una riga che lo giustifichi è un check nascosto senza motivo dichiarato, e quello è un difetto. Il gate separa le due direzioni.

**Files:**
- Create: `tools/editor-sessions/compare_rassegna.py`
- Test: `tools/editor-sessions/compare_rassegna_test.py`

**Interfaces:**
- Consumes: `registry.load` · `pie_status.load` · `pie_status.VERDE`.
- Produces: `VERBALE: Path` · `RIGA: re.Pattern` · `leggi_verbale(testo: str) -> dict[str, str]` · `bacino(wires, stato) -> list[str]` · `confronta(verbale, wires, stato) -> dict`.

**Le due direzioni, e perché non sono simmetriche:**

| situazione | esito | perché |
|---|---|---|
| un `requires` nello yaml, e il verbale non lo riporta per quel check | **errore duro** | un check è fuori dall'agenda e nessuno ha scritto la prova |
| un check nel bacino, e il verbale non lo nomina | **stampato**, esce 0 | è comparso dopo la rassegna: informazione per chi passa dopo, non un difetto |

- [ ] **Step 1: scrivi il test che fallisce**

```python
# tools/editor-sessions/compare_rassegna_test.py
"""Il gate del verbale: ogni bloccante ha la sua prova scritta."""
from __future__ import annotations

import unittest

import compare_rassegna
from registry import Wire

VERBALE = """# Rassegna dei requires

| check | allestimento | decisione | prova |
|---|---|---|---|
| `PIE-A` | `SET-A` | `asset:WBP_RT_PauseMenu` | non tracciato |
| `PIE-B` | `SET-A` | — | eseguibile oggi |
| `PIE-C` | `SET-A` | `asset:X`, `mount:X#7` | non tracciato, e il feed e' di #7 |
"""

STATO = {
    "PIE-A": {"stato": "⏳", "release": False},
    "PIE-B": {"stato": "🟡", "release": False},
    "PIE-C": {"stato": "❌", "release": True},
    "PIE-D": {"stato": "⏳", "release": False},
    "PIE-E": {"stato": "✅", "release": False},
}


class LeggiVerbaleTest(unittest.TestCase):
    def test_raccoglie_la_decisione_per_check(self):
        letto = compare_rassegna.leggi_verbale(VERBALE)
        self.assertEqual(letto["PIE-A"], "`asset:WBP_RT_PauseMenu`")
        self.assertEqual(letto["PIE-B"], "—")
        self.assertIn("mount:X#7", letto["PIE-C"])

    def test_ignora_la_riga_di_separazione_e_l_intestazione(self):
        self.assertEqual(set(compare_rassegna.leggi_verbale(VERBALE)), {"PIE-A", "PIE-B", "PIE-C"})


class BacinoTest(unittest.TestCase):
    def test_sono_i_cablati_non_verdi(self):
        wires = [
            Wire(check="PIE-A", setup="SET-A"),
            Wire(check="PIE-E", setup="SET-A"),  # verde: fuori
        ]
        self.assertEqual(compare_rassegna.bacino(wires, STATO), ["PIE-A"])


class ConfrontaTest(unittest.TestCase):
    def test_un_requires_senza_riga_nel_verbale_e_un_difetto(self):
        wires = [Wire(check="PIE-D", setup="SET-A", requires=("asset:Y",))]
        r = compare_rassegna.confronta(VERBALE, wires, STATO)
        self.assertEqual(r["senza_prova"], ["PIE-D"])

    def test_un_requires_che_il_verbale_non_nomina_nella_decisione_e_un_difetto(self):
        """Il verbale dice «—» e lo yaml blocca: le due fonti si contraddicono."""
        wires = [Wire(check="PIE-B", setup="SET-A", requires=("asset:Y",))]
        r = compare_rassegna.confronta(VERBALE, wires, STATO)
        self.assertEqual(r["senza_prova"], ["PIE-B"])

    def test_un_requires_riportato_nel_verbale_passa(self):
        wires = [Wire(check="PIE-A", setup="SET-A", requires=("asset:WBP_RT_PauseMenu",))]
        r = compare_rassegna.confronta(VERBALE, wires, STATO)
        self.assertEqual(r["senza_prova"], [])

    def test_un_check_comparso_dopo_la_rassegna_si_stampa_e_non_ferma(self):
        wires = [
            Wire(check="PIE-A", setup="SET-A", requires=("asset:WBP_RT_PauseMenu",)),
            Wire(check="PIE-D", setup="SET-A"),
        ]
        r = compare_rassegna.confronta(VERBALE, wires, STATO)
        self.assertEqual(r["non_esaminati"], ["PIE-D"])
        self.assertEqual(r["senza_prova"], [])

    def test_l_ordine_e_totale(self):
        wires = [
            Wire(check="PIE-D", setup="SET-A"),
            Wire(check="PIE-A", setup="SET-A", requires=("asset:WBP_RT_PauseMenu",)),
        ]
        r = compare_rassegna.confronta(VERBALE, wires, STATO)
        self.assertEqual(r["non_esaminati"], sorted(r["non_esaminati"]))


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: esegui il test e verifica che fallisca**

Run: `python -m unittest discover -s tools/editor-sessions -p 'compare_rassegna_test.py' -v`
Expected: FAIL con `ModuleNotFoundError: No module named 'compare_rassegna'`.

- [ ] **Step 3: implementa**

```python
# tools/editor-sessions/compare_rassegna.py
"""Il gate del verbale: ogni bloccante dichiarato ha la sua prova scritta.

Due direzioni, e non sono simmetriche.

  Un `requires` nello yaml che il verbale non giustifica e' un DIFETTO: un
  check e' uscito dall'ordine del giorno e nessuno ha scritto perche'. Si
  esce non-zero.

  Un check nel bacino che il verbale non nomina NON e' un difetto: e'
  comparso dopo la rassegna, che e' archeologia datata e non un registro da
  tenere aggiornato. Si stampa, e si esce zero.

Invertire le due direzioni sarebbe il difetto: un verbale che DEVE restare
aggiornato diventa la sesta rappresentazione che nessuno legge, ed e' il modo
esatto in cui e' morto `editormap.shortlist.md` (D-181).

    python3 tools/editor-sessions/compare_rassegna.py
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import pie_status
import registry
from registry import Wire

VERBALE = Path("docs/roadmap/plans/sedute-componibili-rassegna-requires-2026-09-12.md")

# Una riga di verbale: | `PIE-XXX` | `SET-YYY` | decisione | prova |
# La classe include le MINUSCOLE per la stessa ragione di `pie_status.RIGA`: nove voci
# reali portano un suffisso minuscolo, e una classe `[A-Z0-9-]` le SALTA in silenzio.
RIGA = re.compile(r"^\|\s*`(PIE-[A-Za-z0-9-]+)`\s*\|[^|]*\|([^|]*)\|")


def leggi_verbale(testo: str) -> dict[str, str]:
    """La decisione registrata per ogni check esaminato, come testo."""
    return {m.group(1): m.group(2).strip() for m in map(RIGA.match, testo.splitlines()) if m}


def bacino(wires: list[Wire], stato: dict[str, dict]) -> list[str]:
    """I check cablati e non verdi: quelli che la rassegna doveva esaminare.

    Un check verde e' finito: non c'e' piu' niente da guardare, quindi non ha
    senso chiedersi se sia bloccato. Un check non cablato appartiene alla coda
    scoperta, che e' un'altra misura.
    """
    return sorted(
        w.check
        for w in wires
        if w.check in stato and stato[w.check]["stato"] != pie_status.VERDE
    )


def confronta(verbale: str, wires: list[Wire], stato: dict[str, dict]) -> dict:
    deciso = leggi_verbale(verbale)
    senza_prova = sorted(
        w.check
        for w in wires
        if w.requires
        and not all(r in deciso.get(w.check, "") for r in w.requires)
    )
    esaminati = set(deciso)
    return {
        "senza_prova": senza_prova,
        "non_esaminati": [c for c in bacino(wires, stato) if c not in esaminati],
        "esaminati": len(esaminati),
        "bloccanti": sum(1 for w in wires if w.requires),
    }


def main() -> int:
    sys.stdout.reconfigure(encoding="utf-8")
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mattoni", default=registry.MATTONI, type=Path)
    ap.add_argument("--registro", default=pie_status.REGISTRO, type=Path)
    ap.add_argument("--verbale", default=VERBALE, type=Path)
    a = ap.parse_args()

    for p in (a.mattoni, a.registro, a.verbale):
        if not p.exists():
            sys.exit(f"file non trovato: {p}")

    try:
        _, wires = registry.load(a.mattoni)
    except registry.RegistryError as e:
        sys.exit(f"registro dei mattoni rifiutato: {e}")

    stato = pie_status.load(a.registro)
    r = confronta(a.verbale.read_text(encoding="utf-8"), wires, stato)

    print(f"check esaminati dal verbale: {r['esaminati']}   requires dichiarati: {r['bloccanti']}")
    print("COMPARSI DOPO LA RASSEGNA — cablati, non verdi, e il verbale non li nomina:")
    for c in r["non_esaminati"] or ["  (nessuno)"]:
        print(f"  {c}")

    if r["senza_prova"]:
        print("SENZA PROVA — bloccano un check e il verbale non lo giustifica:")
        for c in r["senza_prova"]:
            print(f"  {c}")
        sys.exit(
            "\nUn bloccante senza prova toglie un check dall'ordine del giorno senza dire perche'. "
            f"Scrivi la riga in {a.verbale}, oppure togli il `requires`."
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 4: esegui i test e verifica che passino**

Run: `python -m unittest discover -s tools/editor-sessions -p 'compare_rassegna_test.py' -v`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/editor-sessions/compare_rassegna.py tools/editor-sessions/compare_rassegna_test.py
git commit -m "feat(sedute): il gate del verbale — un bloccante senza prova non passa"
```

---

## Task 6: la rassegna, e i `requires` che ne escono

È il task d'autore. Gli altri costruiscono l'attrezzatura; questo produce il giudizio.

**Files:**
- Create: `docs/roadmap/plans/sedute-componibili-rassegna-requires-2026-09-12.md`
- Modify: `docs/roadmap/sedute-mattoni.yaml` (i `requires`)
- Modify: `tools/decision-log/github-cache.json` (rigenerata)

**Interfaces:**
- Consumes: `--also` (Task 3) · `compare_rassegna.py` (Task 5) · la regola «un tipo, un oracolo» (Task 1).
- Produces: il verbale, nel formato che `compare_rassegna.RIGA` legge.

- [ ] **Step 1: produci il bacino**

Run:
```bash
mkdir -p build
python - <<'PY' > build/bacino.txt
import sys
sys.path.insert(0, 'tools/editor-sessions')
sys.stdout.reconfigure(encoding='utf-8')
import registry, pie_status
_, wires = registry.load()
stato = pie_status.load()
for w in sorted(wires, key=lambda x: x.check):
    s = stato.get(w.check)
    if s and s["stato"] != pie_status.VERDE:
        print(f"{w.check}\t{w.setup}\t{s['stato']}\t{'RELEASE-V01' if s['release'] else ''}")
PY
wc -l < build/bacino.txt
```
Expected: un file con una riga per check da esaminare. **Il conteggio è l'esito di questo passaggio, non un totale**: al 2026-09-12 dava `118`; se oggi dà altro, va bene — riportalo nel verbale col comando.

- [ ] **Step 2: esamina, un allestimento alla volta**

Per ogni check del bacino, in ordine di allestimento (`SET-SCEN`, `SET-HEX-MATCH`, `SET-SANDBOX`, `SET-GEN-ARENA`, `SET-FRONTEND`, `SET-GRAYKIT`, `SET-HEX-BOT`, `SET-HEX-TURN`), leggi **due fonti** e decidi:

1. la sua riga in `docs/technical/test-manuali-pie.md` — è l'owner dello stato, e nomina spesso ciò che manca;
2. la seduta o le sedute che lo convocavano in `docs/roadmap/editor-sessions.yaml` (campo `verifies`), coi loro `artifacts` e `notes`.

```bash
grep -n 'PIE-V01-FRONTEND-PAUSE' docs/technical/test-manuali-pie.md
grep -n 'PIE-V01-FRONTEND-PAUSE' docs/roadmap/editor-sessions.yaml
```

**La decisione è una sola domanda**: *qualcuno che apre l'Editor in questo allestimento, oggi, può arrivare a un verdetto su questo check?*

- **Sì** → nessun `requires`. Scrivi `—` nel verbale, con una riga di motivo.
- **No, e manca un package** → verifica, non dedurre:
  ```bash
  python -c "import sys; sys.path.insert(0,'tools/editor-sessions'); import oracles; print('WBP_RT_PauseMenu' in oracles.asset_tracciati())"
  ```
  Se è `False`, scrivi `asset:<Nome>`.
- **No, e manca qualcos'altro** — il widget c'è ma non è montato, l'evento non è disegnato, la clip non esiste, la condizione non è ottenibile → serve **una issue owner**, e va trovata, non inventata:
  ```bash
  gh issue list --repo DegrassiAaron/refactor-tactics-main --search "EventLogRight" --state all --limit 10
  ```
  Senza owner, **non si dichiara niente**: un bloccante che nessuno può togliere nasconde il check per sempre, ed è il rischio che §7 della spec nomina per primo.
- **I due fatti sono entrambi veri** → due righe: `asset:<Nome>` e `<tipo>:<Nome>#<issue>`.

⛔ **Tre divieti, ognuno con la propria ragione:**

1. **Non dichiarare un `requires` per un nome d'eroe ritirato.** `PIE-AS2`, `PIE-AS4b`, `PIE-AS4c` citano `BP_Unit_Gadget`, `_Phase`, `_Riktor`, `_Wraith`, `DA_Hero_Flux`, che non esistono e non esisteranno: il roster tracciato è `BP_Unit_Aevik`, `-Branth`, `-Ivrin`, `-Muiren`, `D-130` ha ritirato Flux e `D-321` ha differito la migrazione dei nomi a `#2297`. Quei check **si eseguono oggi** con le unità che esistono: il nome morto è un difetto di *prosa* del registro PIE, che ha già il proprio owner. Nel verbale: decisione `—`, motivo `«nome ritirato, difetto di prosa — owner #2297»`.
2. **Non dichiarare un `requires` che non hai verificato.** Un falso positivo toglie un check dall'ordine del giorno senza che nessuno se ne accorga; un falso negativo si vede aprendo l'Editor. Gli errori non costano uguale.
3. **Non dedurre un bloccante da un `#nnn` citato nella riga di registro.** Nel registro PIE i numeri sono **riferimenti**, non bloccanti: al 2026-09-12 la riga di `PIE-GEO-CENTRO` cita `#1826` senza esserne bloccata. Un `#nnn` diventa un oracolo solo se hai letto la issue e dice che quel check non è guardabile.

Casi già misurati il 2026-09-12, da **confermare** durante la rassegna e non da copiare alla cieca:

| check | candidato | stato della verifica |
|---|---|---|
| `PIE-V01-FRONTEND-PAUSE` | `asset:WBP_RT_PauseMenu` | non tracciato; la riga di registro lo nomina |
| `PIE-V01-FRONTEND-RESULT` | `asset:WBP_RT_ResultScreen` | non tracciato; la riga di registro lo nomina |
| `PIE-SCEN-COMPOSER` | `asset:WBP_RT_ScenarioComposer` | non tracciato |
| `PIE-PC-GYM` | `asset:L_CameraFeatureLab` | non tracciato, **ma il registro PIE non lo nomina**: la fonte è `U37`. Verifica che serva davvero prima di dichiararlo |
| `PIE-HEXPLAY-6`, `PIE-VIS-SIGHTWALL`, `PIE-V01-BLINDFIRE`, `PIE-V01-SCREENHUD` | `asset:WBP_RT_EventLogRight` + `mount:WBP_RT_EventLogRight#2697` | non tracciato; esistono `WBP_RT_EventLog` e `WBP_RT_EventLine`. Verifica su `#2697` che il feed sia davvero il suo oggetto |
| `PIE-AS2`, `PIE-AS4b`, `PIE-AS4c` | **nessuno** | vedi il divieto 1 |

- [ ] **Step 3: scrivi il verbale**

Crea `docs/roadmap/plans/sedute-componibili-rassegna-requires-2026-09-12.md`:

````markdown
# Rassegna dei `requires` — verbale, 2026-09-12

> `ESITO DI PASSAGGIO` · Fetta **1** di
> [`sedute-componibili-design-2026-09-11.md`](sedute-componibili-design-2026-09-11.md) ·
> Piano: [`sedute-componibili-piano-fetta-1-2026-09-12.md`](sedute-componibili-piano-fetta-1-2026-09-12.md)
>
> ⛔ **Questo documento non possiede niente.** Non è un registro e non va tenuto aggiornato.
> Dice una cosa sola: **al 2026-09-12 questi check erano stati esaminati, e con quale esito.**
> Lo stato di un check appartiene a `docs/technical/test-manuali-pie.md`; il cablaggio a
> `docs/roadmap/sedute-mattoni.yaml`; le sedute, fino alla fetta 5, a `docs/roadmap/editor-sessions.yaml`.
>
> 🔑 **Convenzione sui numeri.** Ogni conteggio è un esito di questo passaggio e porta il comando
> che lo produce. Chi rilegge **rimisura**.

## Cos'è stato esaminato

Il bacino: i check **cablati** e **non verdi** — un check verde è finito, un check non cablato
appartiene alla coda scoperta, che è un'altra misura.

```bash
python - <<'PY'
import sys; sys.path.insert(0, 'tools/editor-sessions')
import registry, pie_status
_, w = registry.load(); s = pie_status.load()
print(len([x for x in w if x.check in s and s[x.check]["stato"] != pie_status.VERDE]))
PY
```

→ **xx** al 2026-09-12, su `<SHA>`.

## Il criterio

Una domanda sola, per ogni check: **qualcuno che apre l'Editor in quell'allestimento, oggi, può
arrivare a un verdetto?** Se sì, nessun prerequisito — l'assenza della riga *è* la dichiarazione.
Se no, il prerequisito si dichiara col tipo del proprio oracolo, e con l'owner che può toglierlo.

Un `requires` non verificato non si scrive: un falso positivo toglie un check dall'ordine del
giorno **in silenzio**, un falso negativo si vede aprendo l'Editor. Gli errori non costano uguale.

## Il verbale

| check | allestimento | decisione | prova |
|---|---|---|---|
| `PIE-V01-FRONTEND-PAUSE` | `SET-FRONTEND` | `asset:WBP_RT_PauseMenu` | non tracciato in `Content/`; la riga di registro lo nomina |
| `PIE-AS2` | `SET-SANDBOX` | — | eseguibile col roster tracciato; il nome ritirato è difetto di prosa, owner [#2297] |
<!-- una riga per ogni check del bacino -->

## Cosa questa rassegna non ha deciso

- `SET-TD` è dichiarato e nessun check lo usa. È una domanda sull'**allestimento**, non sui
  prerequisiti: resta il follow-up che §12 della spec ha aperto.
- La **coda scoperta** — i check che nessuna riga di `wiring` cabla — non è entrata nel bacino.
  Fra essi `PIE-VIS-DEFLECT` e `PIE-VIS-INTERPOSE`, che la spec cita come casi `cue` canonici:
  non si può dichiarare il prerequisito di un check che non ha ancora un allestimento.
````

⚠️ Le due righe d'esempio vanno **sostituite** dalla rassegna vera. Il formato è vincolante: `compare_rassegna.RIGA` legge `| \`PIE-XXX\` | <allestimento> | <decisione> | <prova> |`, e la decisione deve contenere **letteralmente** ogni `requires` che lo yaml dichiara per quel check.

- [ ] **Step 4: scrivi i `requires` nel registro**

Aggiungi il campo `requires` alle righe di `docs/roadmap/sedute-mattoni.yaml` corrispondenti alle decisioni non-`—` del verbale, nello stile che `rendi_righe` produce:

```yaml
  - check: PIE-V01-FRONTEND-PAUSE
    setup: SET-FRONTEND
    requires: [ asset:WBP_RT_PauseMenu ]
```

- [ ] **Step 5: rigenera la cache GitHub**

Run:
```bash
gh auth status
python tools/decision-log/fetch_github_cache.py --also docs/roadmap/sedute-mattoni.yaml
git diff --stat tools/decision-log/github-cache.json
```
Expected: la riga `+N da docs/roadmap/sedute-mattoni.yaml` con N ≥ 1, e il file cambia.

⚠️ **Se `gh` non è autenticata**: non aggirare. Il passo si dichiara `NOT RUN` col motivo nel Task 7, e la fetta si chiude senza i tipi con issue — i `requires` di tipo `asset` non toccano la cache e restano validi. `NOT RUN` non equivale a `PASS`.

- [ ] **Step 6: esegui il gate del verbale**

Run: `python tools/editor-sessions/compare_rassegna.py`
Expected: esce 0. `SENZA PROVA` vuota, e `COMPARSI DOPO LA RASSEGNA` vuota — l'hai appena scritta.

Se `SENZA PROVA` non è vuota, hai scritto un `requires` nello yaml che il verbale non riporta **alla lettera**: allinea i due, non abbassare il gate.

- [ ] **Step 7: guarda l'ordine del giorno, che è la prova della promessa**

Run:
```bash
python tools/editor-sessions/build_agenda.py --out build/ordine-del-giorno.md
python -c "import sys; sys.stdout.reconfigure(encoding='utf-8'); print(open('build/ordine-del-giorno.md',encoding='utf-8').read())" | grep -A 12 'SET-FRONTEND'
```
Expected: `PIE-V01-FRONTEND-PAUSE` compare sotto **«Bloccati in questa stessa apertura»**, col motivo `WBP_RT_PauseMenu non esiste in Content/`, e `SET-FRONTEND` **resta un'apertura** perché altri check suoi sono liberi. È la promessa di §12 della spec, esercitata.

- [ ] **Step 8: rilancia il gate della fetta 0**

Run: `python tools/editor-sessions/compare_legacy.py`
Expected: esce 0, `PERSI (nessuno)`.

> 🔑 Questo gate **deve** restare verde, e non per fortuna: `confronta` conta fra i «nominati» anche
> i bloccati — *«un check bloccato e stampato non e' perso — e' dichiarato»*. Se diventasse rosso
> **proprio per effetto dei `requires`**, significherebbe che un check è sparito invece di essere
> stampato bloccato, ed è il difetto peggiore che questa fetta possa produrre. Fermati e indaga.

- [ ] **Step 9: Commit**

```bash
git add docs/roadmap/plans/sedute-componibili-rassegna-requires-2026-09-12.md docs/roadmap/sedute-mattoni.yaml tools/decision-log/github-cache.json
git commit -m "feat(sedute): i requires dichiarati, con il verbale che li giustifica uno per uno"
```

---

## Task 7: i gate, il README, e l'esito della fetta 1

**Files:**
- Modify: `tools/editor-sessions/README.md`
- Modify: `docs/roadmap/plans/sedute-componibili-design-2026-09-11.md` (nuova §13)

**Interfaces:**
- Consumes: tutto il resto del piano.
- Produces: l'esito misurato, che la fetta 2 rimisura invece di ereditare.

- [ ] **Step 1: esegui tutti i gate, in ordine, e prendi nota dell'uscita**

Run:
```bash
python -m unittest discover -s tools/editor-sessions -p '*_test.py'
python -m unittest discover -s tools/decision-log   -p '*_test.py'
python tools/editor-sessions/compare_legacy.py
python tools/editor-sessions/compare_rassegna.py
python tools/editor-sessions/build_agenda.py --out build/ordine-del-giorno.md
```
Expected: `OK`, `OK`, exit 0, exit 0, e l'ordine del giorno scritto. **Copia i numeri veri**: non scrivere nell'esito quelli di questo piano.

- [ ] **Step 2: aggiorna il README**

In `tools/editor-sessions/README.md`, **sostituisci** il blocco di comandi in testa con:

```bash
python3 tools/decision-log/fetch_github_cache.py --also docs/roadmap/sedute-mattoni.yaml
python3 tools/editor-sessions/build_agenda.py               # -> build/ordine-del-giorno.md
python3 tools/editor-sessions/compare_legacy.py             # il gate della fetta 0
python3 tools/editor-sessions/compare_rassegna.py           # il gate del verbale (fetta 1)
python3 -m unittest discover -s tools/editor-sessions -p '*_test.py'
python3 -m unittest discover -s tools/decision-log   -p '*_test.py'
```

⚠️ `--also` non è facoltativo: senza, la cache non conosce le issue che i `requires` citano, e `build_agenda` si **ferma** col comando da rilanciare.

e **sostituisci** la sezione «I mattoni» aggiungendo in coda:

```markdown
### I prerequisiti

`wiring.requires` dichiara perché un check **non si può guardare oggi**. Un tipo, un oracolo:
`asset:<Nome>` interroga `git ls-files Content`; `mount|cue|anim|feature:<Nome>#<issue>`
interrogano la cache GitHub, e chi chiude la issue toglie il bloccante. Quando i fatti veri su
uno stesso nome sono due, si scrivono due righe.

I `requires` **non** si rigenerano: sono giudizio d'autore, ognuno con la propria riga di prova
in `docs/roadmap/plans/sedute-componibili-rassegna-requires-2026-09-12.md`, e
`compare_rassegna.py` rifiuta un bloccante che quel verbale non giustifica. `seed_wiring.py
--into` li **preserva** attraverso il riseminio, e si rifiuta di riseminare se una riga che ne
portava uno non viene più prodotta.
```

e in «Cosa NON fa», aggiungi:

```markdown
- **Non deduce un prerequisito.** La prosa del registro PIE cita `#nnn` come *riferimenti*, non
  come bloccanti, e i nomi tipo-asset che contiene sono spesso istanze (`BP_Unit_Gadget_C_0`) o
  prefissi (`WBP_RT_`). Un `requires` si dichiara dopo aver verificato, mai per euristica: un
  falso positivo toglie un check dall'ordine del giorno in silenzio.
```

- [ ] **Step 3: scrivi §13 nella spec**

In coda a `docs/roadmap/plans/sedute-componibili-design-2026-09-11.md`, **prima** del blocco dei link di riferimento, aggiungi:

```markdown
---

## 13. Esito della fetta 1

**Misurato il <data> su `<SHA>`.** Comandi e uscite:

| | |
|---|---|
| `requires` dichiarati / righe di `wiring` | **xx** / **yy** |
| check esaminati dal verbale / bacino | **xx** / **yy** |
| `compare_legacy.py` | esito |
| `compare_rassegna.py` | esito |
| aperture calcolate | **xx** |
| check stampati come bloccati | **xx** |

**La promessa di §12 è esercitata oppure no**: `PIE-V01-FRONTEND-PAUSE` compare sotto i bloccati
di `SET-FRONTEND` con `WBP_RT_PauseMenu non esiste in Content/` — e `SET-FRONTEND` resta
un'apertura, perché bloccare non è potare.

### Cosa la fetta 1 ha scoperto, e che il design non sapeva

<!-- da riempire con ciò che la rassegna ha trovato e che §1.5 non aveva visto -->

### Follow-up aperti dalla fetta 1

<!-- da riempire -->
```

Riempi le celle con i numeri **veri** dello Step 1, e le due sezioni finali con ciò che la rassegna ha davvero trovato. Se una misura non è stata eseguita — per esempio la cache, perché `gh` non era autenticata — scrivi `NOT RUN` col motivo. **`NOT RUN` non equivale a `PASS`.**

- [ ] **Step 4: Commit**

```bash
git add tools/editor-sessions/README.md docs/roadmap/plans/sedute-componibili-design-2026-09-11.md
git commit -m "docs(sedute): l'esito della fetta 1, misurato, e il README che nomina --also"
```

- [ ] **Step 5: apri la PR**

````bash
git push -u origin design/sedute-componibili-fetta-1
gh pr create --base main --title "Sedute componibili — fetta 1: i requires tipizzati" --body "$(cat <<'CORPO'
Fetta **1** di `docs/roadmap/plans/sedute-componibili-design-2026-09-11.md` §5.3.
Piano: `docs/roadmap/plans/sedute-componibili-piano-fetta-1-2026-09-12.md`.

## Cosa cambia

Un check che oggi non si può guardare **compare bloccato** nell'ordine del giorno, col motivo e
con chi può toglierlo, invece di essere convocato a vuoto. Prima di questa fetta nessuna riga di
`wiring` dichiarava un prerequisito.

- **Un tipo, un oracolo.** `asset:` interroga `git ls-files Content`; `mount|cue|anim|feature:`
  interrogano la cache GitHub. Due fatti sullo stesso nome → due righe.
- **La cache che non conosce un numero si ferma** invece di fingere un bloccante, e il messaggio
  porta il comando che ripara (precedente `D-182`).
- **`fetch_github_cache.py --also`**: i `requires` citano issue che il Decision Log non nomina.
- **`seed_wiring.py --into`** preserva i `requires` attraverso il riseminio, e si rifiuta di
  riseminare se una riga che ne portava uno non viene più prodotta.
- **`compare_rassegna.py`**: un bloccante che il verbale non giustifica non passa.

## Check che hanno ricevuto un `requires`

<!-- gli ID, uno per riga. Mai il loro numero: AGENTS.md §14 -->

## Gate

<!-- gli esiti reali dello Step 1 del Task 7, e i NOT RUN col motivo -->

`test-manuali-pie.md` resta l'unico owner dell'esito; `editor-sessions.yaml` resta l'owner delle
sedute fino alla fetta 5, e il marcatore `DRAFT — NON OWNER` è dov'era.
CORPO
)"
````

⚠️ **La base è `main`**, come la fetta 0 (PR #3032). Nel corpo: gli **ID** dei check che hanno ricevuto un `requires`, mai il loro numero; i gate eseguiti e quelli `NOT RUN` col motivo.

⛔ **Un merge locale su `main` non chiude niente.** Questo clone è condiviso: altre sessioni cambiano branch e riportano `main` a `origin/main`, e un merge locale sparisce lasciando `git status` pulito. Si chiude **con la PR mergiata**, mai altrimenti.

---

## Verification

Da compilare durante il Task 7, con gli esiti reali.

- Compile: `N/A` — nessun `Source/`
- Tests: `PASS / FAIL / NOT RUN`
- Determinism: `N/A` — nessuna simulazione; l'ordinamento dei tool è totale ed è coperto dai test
- Replay: `N/A`
- Privacy: `N/A`
- PIE: `N/A` — questa fetta non apre l'Editor. Produce l'ordine del giorno di **chi** lo aprirà
- Packaged: `N/A`

---

## Cosa questo piano non copre

- **Le issue convocanti** (`--apply`) — fetta 4, e solo dopo che il calcolo è stato creduto.
- **La migrazione della prosa per setup** — fetta 2.
- **La dismissione del vecchio registro** — fetta 5. Fino ad allora `editor-sessions.yaml` resta l'owner, e il marcatore `DRAFT — NON OWNER` resta dov'è.
- **Il cablaggio della coda scoperta.** `PIE-VIS-DEFLECT` e `PIE-VIS-INTERPOSE` sono i casi `cue` canonici della spec e **non hanno un allestimento**: dichiararne il prerequisito richiede prima decidere dove si guardano, che è authoring di `wiring`, non di `requires`.
- **`SET-TD`**, dichiarato e mai usato: è una domanda sull'allestimento. Resta il follow-up aperto da §12.
- **Il «sospetto» di §7** — un `requires` ancora dichiarato dopo la chiusura della sua issue. Non serve: `oracles.valuta` considera **soddisfatto** un requires la cui issue è chiusa, quindi il check torna nell'agenda da sé. Il rischio che §7 descrive non esiste nell'implementazione, e §13 può registrarlo.
- **Un gate CI**, escluso dal design per scelta esplicita (§6).
- **La correzione dei nomi d'eroe** nel registro PIE: perimetro e owner stanno in `#2297`.
