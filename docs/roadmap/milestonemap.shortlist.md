# Milestone map — shortlist

> `GENERATA` · i blocchi §1 e §3 li riscrive `python scripts/feature_registry.py shortlist`, leggendo lo
> stato da [`roadmap-checkpoint.md`](roadmap-checkpoint.md) e i gate da
> [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md).
> **Cosa è**: l'elenco corto delle milestone, in **entrambe** le gerarchie che il repository usa.
> **Cosa non è**: una fonte di stato. Le fette GitHub (§2) restano scritte a mano: vivono nelle
> `description` su GitHub, e questo script non parla con la rete.

⚠️ **Due spazi di numerazione, e collidono.** «Milestone» significa due cose in questo repository: le
**release** (v0.1–v0.4) e le **milestone di esecuzione** (M6–M11). `CP 10.1` è *«Activate e Interact»* in E10 e
*«listen server»* in M10. Per questo le milestone GitHub **non si chiamano mai `M<n>`**: userebbero un terzo
spazio dentro uno strumento che non ha modo di disambiguare.

---

## 1. Milestone di esecuzione — M6→M11

La vista che risponde a *«a che punto è il lavoro»*. Nessuna data: il progetto è a dev singolo.
La colonna *Feature tracciate qui* elenca ciò che il Feature Registry assegna alla milestone **invece che a
un'epic** — cioè il lavoro che non è contenuto della release.

<!-- RT_SHORTLIST_MILESTONES:BEGIN -->

| Milestone | Stato | Feature tracciate qui | In una riga |
|---|:--:|--:|---|
| **M6** | 🟡 | — | Parità hex: la partita 2v2 gira **interamente** su esagoni. Resta il **playtest M6.8** in editor (`PIE-HEXPLAY-1..9`) |
| **M7** | 🟡 | — | Dismissione del quadrato, con punto di ritorno (`pre-hex-only`) e release packaged. Restano 2 KPI su 4: FPS e preview richiedono rendering |
| **M8** | 🟡 | `RT-FEAT-CHAR-PRESENTATION` | Presentazione e identità. Le regole degli eroi ci sono; manca il lavoro in editor — personaggi animati, anelli, Ghost Timeline, showcase |
| **M9** | 🟡 | `RT-FEAT-TOOL-MAP-EDITOR`, `RT-FEAT-TOOL-MAP-GEOMETRY`, `RT-FEAT-TOOL-SCENARIO-COMPOSER`, `RT-FEAT-TOOL-SKILL-WORKBENCH` | Ambienti tattici ed editor maturo. E8 ed E9 chiuse; restano gli obiettivi di **E10** e il residuo editor **H5** |
| **M10** | ⏳ | `RT-FEAT-NET-AUTHORITY` | Rete e privacy: **fuori dalla v0.1**. E13/E14 la preparano; ADR-0004 le aggiunge N round-trip per turno |
| **M11** | ⏳ | — | Production readiness: validator in CI, soak su packaged, replay audit, budget su mappa grande |

<!-- RT_SHORTLIST_MILESTONES:END -->

**Archiviate**: **M0–M5** (MVP quadrato, fase tutorial) e **H0–H6.5** (fondamenta esagonali). Non si
riaprono e non si estendono.

**DoD di milestone in una riga**: M6 = 9 voci `PIE-HEXPLAY` verdi + partita 2v2 completa su mappa multilivello ·
M7 = un solo substrato, budget misurati, packaged giocabile · M8 = sessione C di PIE verde, nessun cilindro ·
M9 = un incremento ambientale cambia l'esito di un turno, nessuna cache stantia · M10 = **intent leak = 0**
dimostrato dal canary · M11 = budget rispettati o deviazioni registrate, validator che blocca la CI, soak senza crash.

---

## 2. Milestone GitHub — le undici fette di release *(dal 2026-08-09)*

La vista che si guarda **su GitHub**. Tagliate lungo la sequenza consigliata della v0.1: più grandi di
un'epic, più piccole di una release — rispondono a *«cosa posso dichiarare consegnato»*.
**197 issue, 0 senza milestone.** `due_on` vuoto su tutte: una data inventata degrada in *scaduta*.

> Questa tabella **non è generata**: lo stato vive nelle `description` su GitHub. I conteggi sono quelli
> registrati all'applicazione (2026-08-09) — per rimisurarli, `gh issue list --milestone "<nome>"`.

| # | Milestone | Stato | Contenuto | Chiude quando |
|--:|---|:--:|---|---|
| 1 | `v0.1 · Fondamenta` | ✅ **chiusa** 52/52 | E1–E6, E8, E9, E16 | chiusa il 2026-08-09 |
| 2 | `v0.1 · Mondo giocabile` | 1/16 | E7 equipaggiamento · E10 obiettivi · E19 classe di mappa | DoD E7 + E10 |
| 3 | `v0.1 · Leggibilità` | 0/16 | E11 HUD/log/debug · E20 icone · E21 presentazione | gli 8 `rt.Debug.*` in PIE + certezza a 3 livelli + voci `PIE-AS*` registrate |
| 4 | `v0.1 · Percezione e reazioni` | 1/22 | E13 vista e udito · E14 overwatch, Clash, Time Bank · E18 predictive | DoD E13 + E14: no finestre annidate, `Timeout → HOLD`, DTO avversario pulito |
| 5 | `v0.1 · Prova integrata` | 3/12 | E15 showcase · E17 stress 4v4 · harness | golden replay a hash stabile, e lo scenario **nell'harness**, non in una seconda pipeline |
| 6 | `v0.1 · Gate di release` | 0/13 | E12 · residui E2.8 (playtest) ed E3.3 (KPI) · epic master `#14` | **G1–G15 verdi con evidenza** |
| 7 | `v0.1 · Difetti e bilanciamento` | 19/26 | difetti trasversali e debito dei test | nessuna issue aperta che invalidi un gate o un DoD di epic |
| 8 | `v0.2 · Struttura e finestre` | 0/10 | E22, E23, E24, E25, E26, E35 | gate di release v0.2 |
| 9 | `v0.3 · Informazione` | 0/4 | E27, E28, E29, E33 | gate di release v0.3 |
| 10 | `v0.4 · Operations` | 0/19 | E30, E31, E32, E34 | gate di release v0.4 |
| 11 | `Debito documentale e decisioni aperte` | 0/7 | propagazioni, cluster di consolidamento, `FAC-*`, `INT-*` | **mai** — è un contenitore permanente; il segnale è la **dimensione** (>~10 issue aperte) |

**Il criterio di chiusura è scritto dentro GitHub**, nella `description` di ogni milestone:

```
Given  gh issue list --milestone "<nome>" --state open   →  vuoto
And    il gate dichiarato nella description è ✅ con evidenza
Then   la milestone si chiude
```

**Regola di manutenzione**: ogni issue nuova nasce con una milestone; label di release e milestone non
divergono (è così che il difetto è nato — 77 issue aperte su 101 erano fuori milestone).

```bash
gh issue list --state open --label v0.1 --json number,milestone \
  --jq '.[] | select(.milestone == null or (.milestone.title | startswith("v0.1") | not)) | .number'
```

Se restituisce qualcosa, le due viste hanno ripreso a divergere.

---

## 3. I gate di release della v0.1

La v0.1 non si chiude perché «sembra pronta»: si chiude quando questi sono **verdi con evidenza**.

<!-- RT_SHORTLIST_MILESTONES_GATES:BEGIN -->

**15 gate** · verdi: **1**. Stato letto da [`v0.1-definition-of-done.md`](v0.1-definition-of-done.md) §3.

| Gate | Cosa chiede | Stato |
|:--:|---|---|
| **G1** | Build **Editor** + **Game Development** + **Game Shipping** senza warning nuovi | ⏳ |
| **G2** | Suite automation completa verde | ⏳ |
| **G3** | I 10 test nominati dal catalogo esistono — **nove con quei nomi, uno rinominato di proposito** | ⏳ |
| **G4** | Determinismo: 100 ripetizioni, checksum identico | ⏳ |
| **G5** | Nessun gameplay quadrato residuo | ⏳ |
| **G6** | ID stabili e unici per azioni, terreni, equipaggiamento, eroi | ⏳ |
| **G7** | Nessun float in costi, priorità, danni | ⏳ |
| **G8** | Nessun intento avversario replicato | ⏳ |
| **G9** | Il **subset `RELEASE-V01`** delle verifiche manuali è eseguito | ⏳ **2 verdi · 7 parziali · 8 aperte** (2026-08-09) |
| **G10** | Partita completa 2v2 su mappa multilivello, dall'avvio alla vittoria | ⏳ |
| **G11** | KPI misurati e registrati (anche fuori target) | ⏳ |
| **G12** | Packaging Windows Development **e** Shipping | ✅ **2026-08-16** (#923): `BUILD SUCCESSFUL`, un cook e due binari · Development 336 MB · Shipping 167 MB · `.pak` 10,8 MB |
| **G13** | Partita giocabile **senza editor** dalla build packaged | 🟡 **2026-08-10**: partita completa fino alla vittoria sul pacchetto Development (`round 6/12`, per eliminazione, zero crash) — ma sull'**arena di test**. Riserva sotto |
| **G14** | Documentazione allineata | ⏳ |
| **G15** | Tracciabilità delle feature verificabile | ⏳ |

<!-- RT_SHORTLIST_MILESTONES_GATES:END -->

---

## 4. Come le viste si rapportano

| Vista | Risponde a | Owner |
|---|---|---|
| **Milestone** M6–M11 | *a che punto è il lavoro?* | [`roadmap-checkpoint.md`](roadmap-checkpoint.md) |
| **Epic** E1–E21 | *quando si lavora a questo?* | [`roadmap-v0.1.md`](roadmap-v0.1.md) §2.1 |
| **Feature** `RT-FEAT-*` | *questa cosa esiste?* | [`feature-registry.yaml`](feature-registry.yaml) |
| **Milestone GitHub** | *cosa posso dichiarare consegnato?* | le `description` su GitHub |

Corrispondenze note: **E2 ≡ M6** (stesso lavoro, issue `#31`–`#38`) · **E3 ≡ M7** meno il packaging (in E12) ·
E8+E9 ≈ M9 · E6/E11/E21 ⊂ M8 · E12 anticipa parte di M11. **Chi lavora su M6 sta lavorando su E2**: le issue
si chiudono **una volta**, aggiornando entrambe le viste — aggiornarne una sola è il modo in cui la deriva si crea.

**Mappatura con le fasi PDR**: M6+M7 ≈ F0 su hex · M8 ≈ parte di F4 · M9 ≈ F3+F2 · M10 ≈ F1 · M11 ≈ F5–F6.
Le due divergenze consolidate, dove prevale il canone: **rete differita** e **no-GAS**.
