# Milestone GitHub — struttura, criteri di chiusura e mappatura

> `CURRENT` · **Applicato**: 2026-08-09 · **HEAD**: `d41d36c` (`origin/main`)
> **Cosa è**: la quarta vista del progetto — quella che si guarda **su GitHub**, non nei documenti.
> **Cosa non è**: una nuova fonte di stato. Lo stato di una feature resta in
> `feature-registry.yaml`, quello delle epic in [`roadmap-v0.1.md`](../roadmap-v0.1.md) §2.1,
> quello delle milestone di esecuzione in [`roadmap-checkpoint.md`](../roadmap-checkpoint.md). Qui si dichiara solo
> **come le issue sono raggruppate** e **quando una milestone si chiude**.

---

## 1. Il difetto che ha motivato l'intervento

Prima del 2026-08-09 il repository aveva **una sola milestone GitHub**, `v0.1`, creata con le issue `#14`–`#85`
e mai estesa. Nel frattempo sono arrivate E13–E21 (`#151`+), E25 ed E34.

| Misura | Valore prima |
|---|---:|
| Issue aperte **senza** milestone | **77 su 101** |
| Issue aperte con label `v0.1` | 70 |
| …di cui **dentro** la milestone `v0.1` | **24** |
| Avanzamento **dichiarato** dalla milestone | 49/73 = **67 %** |
| Avanzamento **reale** sulla label | 49/119 = **41 %** |

La label ha continuato a essere applicata, la milestone no. Una barra che si muove verso il verde mentre il
lavoro cresce fuori dal suo perimetro non è un indicatore: **è rumore con una percentuale sopra**.

Ed erano scoperte **dieci issue senza alcuna label di release** — fra cui `#135`, che dichiara che i world di
test non chiamano `BeginPlay()` e che **i cooldown delle abilità non sono verificabili**. Un difetto che
invalida una categoria di test, invisibile a ogni vista del progetto.

---

## 2. Perché *fette di release* e non *release* né *epic*

Tre schemi erano possibili. La scelta è stata registrata perché non è ovvia.

| Schema | Perché scartato / scelto |
|---|---|
| **Una milestone per release** (`v0.1`, `v0.2`…) | Fedele ai documenti, ma la barra di `v0.1` resterebbe ferma per mesi con 79 issue aperte. Una milestone che non si muove non si guarda |
| **Una milestone per epic** (~19) | Duplica il raggruppamento che le issue `[EPIC]` **già fanno**. Diciannove barre che nessuno legge |
| ✅ **Fette di release** (7 per la v0.1) | La milestone risponde a *«cosa posso dichiarare consegnato»* — più grande dell'epic, più piccola della release. Tagliate lungo la *sequenza consigliata* di [`roadmap-v0.1.md`](../roadmap-v0.1.md) §3 |

### La regola sui nomi, che non è estetica

Le milestone GitHub **non si chiamano mai `M6`…`M11`**.

Il repository ha già due gerarchie che si chiamano entrambe «milestone» — le release v0.1–v0.4 e le milestone
di esecuzione M6–M11 — e `feature-registry.yaml` lo dichiara: *«i due spazi di
numerazione **collidono** — CP 10.1 è "Activate e Interact" in E10 e "listen server" in M10»*. Introdurre
`M8` come nome di milestone GitHub aggiungerebbe una **terza** ambiguità in uno strumento che non ha modo di
disambiguare. I nomi sono quindi `<release> · <fetta>`.

### Nessuna data

`due_on` è vuoto su tutte le milestone. Il progetto è a dev singolo e non ha scadenze esterne: una data
inventata degrada in *scaduta* e insegna a ignorare il campo.

---

## 3. Le undici milestone

| # | Milestone | Contenuto | Chiude quando |
|---:|---|---|---|
| 1 | `v0.1 · Fondamenta` | E1–E6, E8, E9, E16 — **chiusa 52/52** | ✅ chiusa il 2026-08-09 |
| 2 | `v0.1 · Mondo giocabile` | E7 equipaggiamento · E10 obiettivi · E19 classe di mappa | DoD E7 + E10 |
| 3 | `v0.1 · Leggibilità` | E11 HUD/log/debug · E20 icone · E21 presentazione | gli 8 `rt.Debug.*` in PIE + certezza a 3 livelli + voci `PIE-AS*` registrate |
| 4 | `v0.1 · Percezione e reazioni` | E13 vista e udito · E14 overwatch, Clash, Time Bank · E18 predictive | DoD E13 + E14 (no finestre annidate, `Timeout → HOLD`, DTO avversario pulito) |
| 5 | `v0.1 · Prova integrata` | E15 showcase · E17 stress 4v4 · harness | golden replay a hash stabile, scenario nell'harness e non in una seconda pipeline |
| 6 | `v0.1 · Gate di release` | E12 · residui E2.8 (playtest) ed E3.3 (KPI) · epic master `#14` | **G1–G15 verdi con evidenza** |
| 7 | `v0.1 · Difetti e bilanciamento` | difetti trasversali e debito dei test | nessuna issue aperta che invalidi un gate o un DoD di epic |
| 8 | `v0.2 · Struttura e finestre` | E22, E23, E24, E25, E26, E35 | gate di release v0.2 |
| 9 | `v0.3 · Informazione` | E27, E28, E29, E33 | gate di release v0.3 |
| 10 | `v0.4 · Operations` | E30, E31, E32, E34 | gate di release v0.4 |
| 11 | `Debito documentale e decisioni aperte` | propagazioni, cluster di consolidamento, `FAC-*`, `INT-*` | **mai** — è un contenitore permanente |

### Il criterio di chiusura è scritto **dentro** GitHub

Ogni milestone porta il proprio criterio nella `description`, nella forma:

```
Given  gh issue list --milestone "<nome>" --state open   →  vuoto
And    il gate dichiarato nella description è ✅ con evidenza
Then   la milestone si chiude
```

Prima, la condizione di uscita della v0.1 (i 15 gate `G1`–`G15`) esisteva solo in
[`v0.1-definition-of-done.md`](../v0.1-definition-of-done.md): dentro GitHub la milestone non aveva **alcun**
test di uscita e sarebbe chiusa quando qualcuno avesse deciso che era ora.

### La milestone 11 non si chiude, si tiene corta

`Debito documentale e decisioni aperte` è deliberatamente senza criterio di chiusura. Il segnale da
sorvegliare è la **dimensione**: se supera ~10 issue aperte, il consolidamento sta accumulando debito più in
fretta di quanto lo smaltisca.

---

## 4. Stato all'applicazione (2026-08-09)

```
 1  closed  52/52   v0.1 · Fondamenta
 2  open     1/16   v0.1 · Mondo giocabile
 3  open     0/16   v0.1 · Leggibilità
 4  open     1/22   v0.1 · Percezione e reazioni
 5  open     3/12   v0.1 · Prova integrata
 6  open     0/13   v0.1 · Gate di release
 7  open    19/26   v0.1 · Difetti e bilanciamento
 8  open     0/10   v0.2 · Struttura e finestre
 9  open     0/ 4   v0.3 · Informazione
10  open     0/19   v0.4 · Operations
11  open     0/ 7   Debito documentale e decisioni aperte
```

**197 issue, 0 senza milestone** (aperte e chiuse). Anche le chiuse sono state assegnate alla fetta di
competenza: una barra che parte da zero dove il lavoro è già stato fatto misura male quanto una che mente.

---

## 5. Le venti issue create

Il lavoro documentato ma **non tracciato**: dodici epic esistevano solo in
[`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md), otto voci solo nel triage del consolidamento o in
[`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

| Issue | Cosa | Milestone |
|---|---|---|
| `#322` | E35 · Roster 8 — Sentinel Directorate e Resonance | v0.2 |
| `#323` | E22 · Cover Window: OPEN → FIRE → SEAL | v0.2 |
| `#324` | E23 · Muri, porte e interaction graph | v0.2 |
| `#325` | E24 · Formato Standard 3v3 | v0.2 |
| `#326` | E26 · Tactical Bot v1 | v0.2 |
| `#327` | E27 · Percezione completa: vista, udito, memoria | v0.3 |
| `#328` | E28 · Expert Bot v2 | v0.3 |
| `#329` | E29 · Predictive avanzato | v0.3 |
| `#330` | E33 · Conditional Intent | v0.3 |
| `#331` | E30 · Classe di mappa Operations | v0.4 |
| `#332` | E31 · Obiettivi multipli e logistica | v0.4 |
| `#333` | E32 · Formato 4v4 competitivo | v0.4 |
| `#334` | `D-046` non è propagata: tre documenti la contraddicono in quattro punti | Debito |
| `#335` | Registrare `INT-1`…`INT-4` in `OPEN_DECISIONS.md` | Debito |
| `#336` | Consolidamento cluster Characters & Roster | Debito |
| `#337` | Consolidamento cluster Scenarios / QA / Bots | Debito |
| `#338` | Consolidamento cluster Governance | Debito |
| `#339` | Facing: nove decisioni aperte (`FAC-1`…`FAC-3`, `FAC-5`…`FAC-10`) | Debito |
| `#340` | Collegare `RT-FEAT-MAP-INTERACTIVE-EDGES` e le feature di E10 al nuovo owner spec | Debito |
| `#341` | **`FAC-4`** · qual è il facing durante i micro-step di un Move? | **Percezione e reazioni** |

`FAC-4` è staccata dalle altre nove voci di facing per una ragione dichiarata in `OPEN_DECISIONS.md`: è
**l'unica che blocca lavoro costruibile oggi** — il DoD di E16 *«snapshot e TurnLog dicono quale facing ha
usato ciascun consumatore»* non è verificabile finché il facing intermedio non è definito, e a valle stanno
CP 14.2, CP 14.4 e CP 14.7. Vive quindi nella milestone che la incontrerà, non nel contenitore del debito.

Le epic post-v0.1 dichiarano tutte, nel corpo, il vincolo di apertura: **nessuna si apre prima che i 15 gate
della v0.1 siano verdi** ([`roadmap-post-v0.1.md`](../roadmap-post-v0.1.md) §*Perché esiste*).

---

## 6. Cosa questo documento non fa

- **Non crea i checkpoint delle epic nuove.** `#322`–`#333` sono epic di tracciamento con i CP elencati nel
  corpo; le issue-checkpoint si aprono quando l'epic si apre, non prima. Aprirle ora produrrebbe ~40 issue
  che nessuno può lavorare per mesi.
- **Non tocca le label.** `post-v0.1` resta l'unica label di release oltre a `v0.1`: la milestone codifica già
  la release, e una label `v0.2` sarebbe un secondo posto dove la stessa informazione può divergere.
- **Non collega le milestone al Feature Registry.** Le **17 feature senza issue** del registry restano tali:
  collegarle è lavoro del registry, non della vista GitHub. Fra queste, due meritano attenzione perché sono
  `v0.1`/`M9`–`M10` e non hanno né epic né issue: `RT-FEAT-TOOL-MAP-EDITOR` (residuo editor H5) e
  `RT-FEAT-NET-AUTHORITY`.
- **Non assegna date.** Vedi §2.

---

## 7. Come si mantiene

1. **Ogni issue nuova nasce con una milestone.** Se non se ne trova una, la issue sta nel contenitore del
   debito o la fetta manca — entrambe sono informazioni.
2. **La label di release e la milestone non divergono.** Sono lo stesso fatto scritto due volte; il modo in
   cui è nato questo difetto è esattamente averle lasciate divergere per tre settimane. Controllo:
   ```bash
   gh issue list --state open --label v0.1 --json number,milestone \
     --jq '.[] | select(.milestone == null or (.milestone.title | startswith("v0.1") | not)) | .number'
   ```
   Se restituisce qualcosa, le due viste hanno ripreso a divergere.
3. **Una fetta chiusa si chiude su GitHub**, altrimenti la vista torna a essere un elenco.
