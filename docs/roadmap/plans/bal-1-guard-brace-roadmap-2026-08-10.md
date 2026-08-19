# `BAL-1` — il confine fra `Guard` e `Brace`: roadmap

> `CURRENT` · **Creato**: 2026-08-10 · **Owner di una sola domanda**: cosa serve *prima* che `BAL-1` sia
> decidibile, e cosa cambia dopo.
>
> **Origine**: [D-066](../../decisions/RT_PDR_00_Decision_Log.md) · voce `BAL-1` di
> [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) · [conflict report del 2026-08-10](handoff-geometry-reazioni-conflict-report-2026-08-10.md) §C6
>
> Questo file **non decide**. `BAL-1` si chiude con una partita, non con un documento: qui c'è solo ciò che
> rende la partita capace di rispondere.

## 1. La domanda è cambiata dopo la misura

Un handoff proponeva la separazione netta *«`Guard` mitiga il danno e non tocca la spinta · `Brace` lascia il
danno e contesta la spinta»*. `D-066` l'ha registrata come **non applicata** perché non è il modello in
vigore. Leggendo il codice invece del catalogo, però, il quadro è più stretto di così.

### 1.1 Le tre costanti

| | Danno | Spinta | Altro |
|---|---|---|---|
| `Guard` | `GuardFirstHitReduction = 15` — solo il **primo** colpo diretto | `GuardResistedPushDistance = 1` | — |
| `Brace` | `BraceDamageReduction = 10` — **ogni** colpo diretto | prima spinta, **senza limite di distanza** | blocca il movimento volontario (CP 5.2) |

Tutte e tre in `RTCombatLibrary.h`; i due rami che le consumano sono in `RTTurnManager` — il danno dove si
applica `TAG_Status_Guarded`/`TAG_Status_Braced`, la spinta nel passaggio unico del knockback.

### 1.2 Il difetto: metà del modello non è osservabile

> **Nel gioco non esiste una spinta maggiore di 1.**

Misurato sul catalogo eroi: **due soli effetti `Push`, entrambi di valore `1`** — quello di
`Hero.Phase.PressureJet` e quello dell'impatto di Riktor. Quindi la clausola che distingue `Brace` da `Guard`
sulla spinta — *«senza limite di distanza»* — **non è raggiungibile in partita**. `GuardResistedPushDistance = 1`
copre per intero lo spazio degli spostamenti esistenti.

L'asse dello spostamento, che è quello su cui l'handoff voleva costruire la separazione, oggi è morto.

### 1.3 Cosa fanno davvero, in numeri

Bersaglio con `PushResistance = 0` (Gadget, Phase, Wraith) colpito da `Hero.Phase.PressureJet` — 16 danni **e** spinta 1
nello stesso colpo:

| Difesa | 1 colpo | 2 colpi |
|---|---|---|
| nessuna | 16 danni, spostato | 32, spostato |
| **`Guard`** | **1 danno**, non spostato | 17, non spostato |
| **`Brace`** | 6 danni, non spostato | **12**, non spostato |

Sul colpo singolo `Guard` **domina**: stessa immunità alla spinta e 5 danni in meno. `Brace` recupera solo dal
secondo colpo in poi.

> **Il trade-off reale non è *danno contro spostamento*: è *primo colpo pesante* contro *colpi ripetuti*.**

Il catalogo e l'handoff descrivono, in modi diversi, un gioco che non è questo.

### 1.4 Effetto collaterale che nessuno ha deciso

**Riktor ha `PushResistance = 1` nativo** — l'unico del roster (`Gadget`, `Phase`, `Wraith` = 0) — e la
resistenza è una **soglia**, non una sottrazione ([D-038](../../decisions/RT_PDR_00_Decision_Log.md), pinnata
da `RefactorTactics.Actions.PushResistanceIsAThreshold`). Siccome ogni spinta del gioco vale 1:

> Riktor è **immune a ogni spostamento del gioco, sempre, senza spendere nulla**.

Il commento nel catalogo lo motiva in parte — *«compra HP e stabilità con movimento e vista»* — ma
l'immunità **totale** è una conseguenza del fatto che nessuna spinta supera 1, non una scelta dichiarata. Va
guardata insieme a `BAL-1` perché ne condivide la causa.

---

## 2. Fase 0 — rendere l'asse osservabile · **prerequisito**

`BAL-1` non è decidibile finché la differenza non si vede. Due uscite, **entrambe legittime**, e la scelta è
dell'autore:

| | Cosa comporta | Costo |
|---|---|---|
| **(A)** introdurre almeno un effetto `Push ≥ 2` | `GuardResistedPushDistance` cede e `Brace` mostra la sua clausola. Rende vera la separazione che l'handoff proponeva | un numero nuovo a catalogo, su un'abilità di **controllo** (non un attacco base) |
| **(B)** accettare che in v0.1 lo spostamento sia sempre 1 | Allora la clausola *«senza limite di distanza»* va **riscritta**: descrive una regola che nessuno può osservare | zero numeri nuovi, ma il catalogo e il commento del codice vanno corretti |

(B) è più economico e più onesto; (A) è ciò che apre davvero l'asse. **Non si può saltare la Fase 0**: senza,
la Fase 1 scriverebbe scenari che passano per la ragione sbagliata.

## 3. Fase 1 — scenari che discriminano

Oggi `Guard` e `Brace` sono verificati **separatamente** (`Visual.Combat.GuardReducesFirstHit`,
`Visual.Combat.PushResistance`): è per questo che il confine può spostarsi senza che nulla diventi rosso.

| ScenarioId | Cosa pinna | Dipende da |
|---|---|---|
| `Spec.Brace.GuardAndBraceOnMixedHit` | Le tre righe della tabella §1.3 — danno **e** spinta nello stesso colpo, contro `Guard`, contro `Brace`, contro nessuno dei due | — |
| `Spec.Brace.BraceWinsOnSecondHit` | `Guard` 17 · `Brace` 12 su due colpi. È **l'unico posto** dove il trade-off reale diventa rosso se cambia | — |
| `Spec.Brace.PushBeyondGuardThreshold` | Spinta 2: `Guard` cede, `Brace` tiene | **Fase 0 (A)** — con (B) non si scrive |
| `Spec.Combat.BastionIgnoresAllPushes` | §1.4 — immunità totale allo spostamento, in un verso o nell'altro | — | <!-- rename-exempt: nome mai esistito: il piano BAL-1 lo prevedeva e la decisione cadde dall'altra parte -->

**L'oracolo esiste già**: bastano `UnitHpEquals` e `UnitAtCell`, le assertion che l'harness ha. A differenza
degli undici `Spec.Clash.*`/`Spec.TimeBank.*` che al 2026-08-10 aspettavano `#318`, qui non manca nessuna
capability. *(aggiornato il 2026-08-13: quei gruppi sono **tredici** dopo la riconciliazione di `#361`, e non
aspettano più — `LogEventAmount` è atterrata il 2026-08-10 con `a7e4677b`. Il contrasto di questa riga vale
ancora per la data in cui fu scritta, non per oggi.)*

**Anche la fixture esiste già**: `Hero.Phase.PressureJet` è a catalogo con 16 danni, `Push 1`, `Wet`, forma a linea,
ed è pinnata da `RTHeroPhaseTests.cpp`. Non c'è niente da costruire — era la fixture che l'handoff proponeva,
e il repository ce l'aveva.

> ⚠️ **Attenzione a un dettaglio del resolver**: il knockback esclude i bersagli spinti da 2+ attaccanti
> (`*Pushes != 1`, forze contraddittorie). Uno scenario che mette due attaccanti sullo stesso bersaglio
> misurerebbe quella regola, non `BAL-1`.

## 4. Fase 2 — la decisione

Con i quattro scenari verdi la scelta è fra tre opzioni già scritte come numeri:

1. **status quo** — *primo colpo* contro *colpi ripetuti*, e la clausola di `Brace` si riscrive (Fase 0-B);
2. **proposta dell'handoff** — `Guard` solo danno, `Brace` solo spostamento: azzera `GuardResistedPushDistance`
   e `BraceDamageReduction`. Richiedeva la **Fase 0-A**, altrimenti `Brace` restava senza mestiere —
   ✅ **e quella condizione è ora soddisfatta**, per una via che nessuno aveva previsto: vedi §4-bis;
3. **ibrido** — si tiene la forma attuale e si separano le **magnitudini**, così che nessuna delle due domini
   sul colpo singolo.

Gli scenari dicono *cosa succede*, non *cosa è divertente*: questa fase è una partita.

### 4-bis. La spinta ≥ 2 è arrivata da un'altra porta *(2026-08-11)*

`D-074` aveva scartato la Fase 0-A — introdurre una spinta `≥ 2` — e su quella premessa aveva **precluso
l'opzione 2**. La premessa è caduta, e non perché qualcuno l'abbia riaperta: **`Weapon.Impact` su
`Hero.Phase.PressureJet`**, che spinge già di 1, produce una spinta di **2** ([D-085](../../decisions/RT_PDR_00_Decision_Log.md)),
ed è il loadout di **default** di Phase ([D-089](../../decisions/RT_PDR_00_Decision_Log.md)).

La spinta forte non è entrata dal catalogo azioni, dove D-074 la stava guardando: è entrata
dall'**equipaggiamento**, sommandosi a una spinta che l'attacco base aveva già.

Conseguenza misurata, non dedotta — `Equipment.PushTwoSeparatesGuardFromBrace` la pinna: contro una spinta
di 2, **`Guard` cede e `Brace` regge**. Le due difese non differiscono più solo nel danno, e la distanza è
di nuovo un asse che le separa.

**Cosa cambia per la decisione**: le opzioni in campo tornano **tre**. L'opzione 2 non è più preclusa,
perché il mestiere che le mancava adesso esiste. Resta però una domanda nuova che il piano non poneva:
quel mestiere dipende da un **equipaggiamento equipaggiato**, non da una regola del turno — quindi
`Brace` avrebbe un ruolo contro Phase-con-Impatto e non contro chiunque altro. Se sia abbastanza per
fondarci sopra una difesa, è parte di ciò che la partita deve dire.

## 5. Fase 3 — implementazione

Solo dopo la Fase 2, ed è piccola per costruzione: tre costanti in `RTCombatLibrary.h` e due rami in
`RTTurnManager`. **Il catalogo eroi non si tocca** — la regola sta nel punto in cui la distanza di spinta si
registra, non nei produttori di spinta, ed è deliberato: *«il settimo nascerebbe già rotto»*.

---

## 6. Feature, Wiki, Editor

**Feature**: nessuna nuova. La voce è `RT-FEAT-ACTION-GENERIC` (le sette azioni universali), che acquista i
quattro scenari come `planned`. I numeri atterrano in `RT-FEAT-TOOL-BALANCE-GROUND` (v0.1, `IMPLEMENTING`, E1),
il cui owner è [`../../balance/`](../../balance/RT_TestMatrix_v0.1.md).

**Wiki**: la pagina delle azioni aveva **due** difetti trovati durante questo lavoro, entrambi corretti:
la «grammatica comune» elencava **sei** azioni saltando `Guard` — contro le sette di
[D-025](../../decisions/RT_PDR_00_Decision_Log.md) — e `Guard`, che è fra le implementate, **non aveva una
sezione**. Ora ce l'ha, e la sezione `Brace` dice cosa fa davvero invece di rimandare al playtest.

**Editor**: la Fase 2 non si chiude headless. Voce `PIE-BAL1` nel registro
[`../../technical/test-manuali-pie.md`](../../technical/test-manuali-pie.md), classe **C** secondo
[`scenario-map.md`](../../technical/tooling/scenario-map.md) — l'oracolo è il giudizio, non un'assertion.
**Non** entra nel subset `RELEASE-V01`: `BAL-1` non blocca la consegna della v0.1, e un gate che si allarga
senza motivo è il difetto che G9 ha già avuto due volte.

## 7. Issue

| # | Titolo | Bloccata da | DoD in una riga |
|---|---|---|---|
| [**#400**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/400) | Ogni spinta del gioco vale 1: la clausola «senza limite di distanza» di `Brace` non è osservabile | — | Fase 0 decisa (A o B). Se (B): clausola riscritta a catalogo **e** nel commento del codice, così che le due letture non divergano |
| [**#401**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/401) | `Guard` e `Brace` sono verificati separatamente: il confine può spostarsi senza rossi | #400 | I 4 scenari in `Scenarios/Spec/`, verdi, con i numeri della §1.3 pinnati |
| [**#402**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/402) | Riktor è immune a ogni spostamento del gioco senza spendere nulla | — | Deciso se `PushResistance = 1` nativo è voluto in questa forma; scenario che lo pinna in un verso o nell'altro |
| [**#403**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403) | `BAL-1`: decidere il confine fra `Guard` e `Brace` | #400, #401 | Playtest eseguito, opzione scelta, `D-0nn` scritta, `BAL-1` rimossa da `OPEN_DECISIONS.md` |
| [**#404**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/404) | Applicare la decisione `BAL-1` alle costanti di combat | #403 | Costanti aggiornate, i 4 scenari aggiornati e verdi, catalogo §135 allineato |

**#400**, **#401** e **#402** non richiedono l'autore e possono partire subito. La **#403** è l'unica che lo
richiede davvero — ed è anche l'unica che non si chiude scrivendo.

## 8. Perché questo piano esiste in questa forma

Il difetto che ha prodotto `BAL-1` non è un numero sbagliato: è che **due difese progettate per essere
diverse sono state verificate solo separatamente**, e quindi la loro differenza non è mai stata un'assertion.
Finché resta così, qualunque decisione si prenda in Fase 2 può essere annullata dal commit successivo senza
che nessuno se ne accorga.

È lo stesso schema del *dato senza consumatore* già visto in questo repository, ruotato di novanta gradi:
qui il consumatore c'è, ma **nessun test guarda i due consumatori insieme**.
