# ADR-0008 — La rotazione è una capacità del personaggio, e il facing si dichiara per azione

> `CANONICAL` · **Stato**: Accettato · **§1 implementata** il 2026-09-03 da
> [#1605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1605) · **§2 e §3 non ancora** ·
> **Data**: 2026-08-10 · **Decisore**: utente (dev singolo)
>
> ✅ **Chi possiede l'implementazione: la §1 è di [#1605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1605), `v0.1 · Percezione e reazioni`.**
> `MoveEndPivotMaxSteps` e `DashEndPivotMaxSteps` sono in `URTHeroData`, valorizzati dal catalogo eroi C++, e
> `URTFacingLibrary::LegalFacings` decide sul **budget della famiglia di movimento** invece che sullo stile.
> Coperta da `Facing.PivotBudgetLimitsLegalFacings`, `Facing.PivotBudgetZeroKeepsMovementDirection`,
> `Facing.MoveAndDashBudgetsAreIndependent`, `Facing.StationaryRotationIsUniversal` e
> `Facing.CatalogPivotBudgetsMatchAdr0008`.
>
> ✅ **La §2 e' a runtime dal 2026-09-03** da
> [#2131](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2131), `v0.1 · Percezione e reazioni`.
> `URTFacingLibrary::FacingAtMicroStep` deriva il facing dal prefisso della rotta e i tre consumatori del
> decision boundary lo leggono: gli **osservatori** che alimentano `TeamAwareness` (ADR-0004 §6), la
> **copertura** del colpo di Overwatch (CP 16.2) e la **voce direzionale** di `HitCameFromSide`. Coperta da
> `Facing.MicroStepFacingIsLastCompletedStep`, `Facing.MicroStepZeroKeepsEntryFacing`,
> `Facing.MicroStepFacingMatchesFinalAtLastStep`, `Facing.FinalPivotIsNotRetroactive`,
> `Overwatch.TriggerReadsMicroStepFacing` e dallo scenario `Spec.Facing.OverwatchHitCameFromSide`.
>
> ⛔ **La §3 NON si implementa, ed e' una decisione, non un residuo**: la §Revisione qui sotto ne fissava la
> rilettura «alla chiusura di `CP 14.3`», che e' chiusa dal **2026-08-12**, e la condizione che avrebbe
> giustificato il costo continua a non esistere. Registrata in
> [`D-318`](RT_PDR_00_Decision_Log.md); i suoi due test non si scrivono, e l'assenza e' **dichiarata**.
>
> 🔎 **Perche' questa nota esiste.** Fino al 2026-09-03 la tabella *Verifica* elencava **sette** test come se
> esistessero — ne esistevano **zero** — e [`D-295`](RT_PDR_00_Decision_Log.md) §(2) ne citava uno,
> `Facing.FinalPivotIsNotRetroactive`, come pin di una decisione. La riga e' stata corretta il 2026-09-03
> ([#2137](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2137)) e il test **ora esiste
> davvero**. La colonna *Stato* resta per impedire che l'equivoco si ripeta.
>
> 🔴 **Perché era orfano, e resta scritto perché la forma del difetto è riusabile.** Misurato il 2026-08-28
> su `f8ea244b`: `E16` ([#175](https://github.com/DegrassiAaron/refactor-tactics-main/issues/175)) era
> **chiusa il 2026-08-09**, cioè il giorno prima che questo ADR fosse accettato; `E11`
> ([#25](https://github.com/DegrassiAaron/refactor-tactics-main/issues/25)) è *HUD, log e debug* e non
> possiede una regola del resolver. Per diciotto giorni il runtime ha applicato la tabella per **stile** di
> ADR-0005 §1 — quella che questo documento dichiara di superare — senza che **nessun gate** lo vedesse:
> non esiste un controllo che confronti un ADR accettato con i simboli del codice.
>
> ⚠️ **Il trigger di revisione dei numeri è già passato**: la §Revisione fissa la prima rilettura «alla
> chiusura di `CP 16.2`», e [#177](https://github.com/DegrassiAaron/refactor-tactics-main/issues/177) è
> **CLOSED** — cioè è scaduto su un'implementazione che allora non esisteva. Ora esiste, e i suoi otto numeri
> **non sono ancora stati tarati**: restano le ipotesi della §1, e la revisione è dovuta.
>
> **Chiude**: `FAC-1`, `FAC-2`, `FAC-4` di [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) · issue [#341](https://github.com/DegrassiAaron/refactor-tactics-main/issues/341) e [#339](https://github.com/DegrassiAaron/refactor-tactics-main/issues/339) (parziale)
> **Supera**: [ADR-0005](adr-0005-orientamento.md) §1 (tabella delle direzioni legali) e §3 (regola universale dello spostamento subìto)
> **Lascia invariato** il resto di ADR-0005: il facing come stato di gioco, l'arco frontale unico (§4), il determinismo e la privacy (§5), e la timeline di [D-020](RT_PDR_00_Decision_Log.md) (§2-bis)
> **Fonte**: handoff `../archive/src/handoff/2026-08-08-azioni-base-e-facing.md` §8, §9, §10, §23.1

## Contesto

ADR-0005 lega la rotazione **al movimento**: lo stile decide quante direzioni sono legali a fine Move — una per
i `Linear*`, tre (`D`, `D±1`) per il `Budget`, sei da fermo. Il suo pregio dichiarato è **zero numeri nuovi**.

L'handoff del 2026-08-08 propone un modello diverso su tre punti, che il consolidamento ha registrato come
`FAC-1`, `FAC-2` e `FAC-4`. Le prime due sono **proposte di modifica** dell'ADR; la terza è una **lacuna**:
ADR-0005 copre l'inizio e la fine del Move, non il mezzo, e i decision boundary di
[ADR-0004](adr-0004-finestre-di-reazione.md) cadono proprio nel mezzo.

Le tre vanno decise insieme. Con `FAC-1` accettata la rotazione finale diventa un budget invece di un insieme
di tre direzioni, e la domanda `FAC-4` — *cosa legge il trigger a metà movimento* — cambia forma: non si può
definire il facing intermedio senza sapere se il pivot finale è ancora un evento separato.

**Decisione dell'utente (2026-08-10)**: accettare il modello dell'handoff su `FAC-1` e `FAC-2`; su `FAC-4`,
adottare il facing derivato dall'**ultimo passo compiuto**.

## Decisione

### 1. La rotazione finale è una capacità del personaggio, misurata in step (`FAC-1`)

La tabella «direzioni legali per stile» di ADR-0005 §1 è **sostituita** da un budget di rotazione a fine
movimento, espresso in step esagonali:

```text
0 step  = nessuna rotazione finale
1 step  = max 60°
2 step  = max 120°
3 step  = max 180° (qualsiasi facing esagonale)
```

Due valori per personaggio — uno per la famiglia `Move`, uno per la famiglia `Dash`, perché le due mobilità
hanno identità diverse e l'handoff §10 chiede esplicitamente policy separate:

| Personaggio | Identità del Move | `MoveEndPivotMaxSteps` | Identità del Dash | `DashEndPivotMaxSteps` |
|---|---|---:|---|---:|
| Gadget | standard/tecnico | **2** | corto/tecnico | **2** |
| Phase | fluido | **2** | molto manovrabile | **3** |
| Riktor | pesante, forte stabilità | **1** | charge/ram, direzionale | **0** |
| Wraith | agile/predittivo | **3** | reposition rapido | **3** |

> ⚠️ **Valori iniziali, non bilanciamento approvato.** La fonte (§23.1) li dà come «ipotesi iniziali da
> scenario/playtest» e chiede di non canonizzarli come definitivi senza conferma. Sono canonici **come punto di
> partenza**: entrano nel catalogo perché il modello ha bisogno di un valore per esistere, non perché siano
> stati misurati. La revisione è in fondo a questo ADR.

Due scelte dentro la tabella meritano di essere dette, perché non vengono dalla fonte:

- **Riktor, Dash = 0.** La fonte dà «0–60°», che è un intervallo, non un valore. Lo 0 è l'estremo che
  **conserva** il comportamento di ADR-0005 per i `Linear*` (una sola direzione, quella del movimento) ed è
  coerente con l'identità dichiarata «pesante, direzionale». Alzarlo a 1 è un cambio di dato, non di modello.
- **`StationaryPivotMaxSteps` resta universale a 3**, cioè le sei direzioni libere di ADR-0005 §1. L'handoff lo
  segnava come «decisione ancora da chiudere»: chiuderlo per eroe aggiungerebbe un **terzo** numero a testa
  senza che nessun caso lo richieda. Chi non si è mosso ruota liberamente, come oggi.

**Costo dichiarato, senza attenuarlo.** Questo introduce **otto numeri** che ADR-0005 non aveva, cioè un asse di
bilanciamento nuovo. È il prezzo esplicito della decisione: in cambio, la stessa cella d'arrivo vale
diversamente a seconda del lato da cui ci si entra, e Riktor e Wraith acquistano un'identità di movimento che
oggi non hanno.

### 2. Il facing durante i micro-step è quello dell'**ultimo passo compiuto** (`FAC-4`)

> ✅ **A runtime dal 2026-09-03** — [#2131](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2131). `URTFacingLibrary::FacingAtMicroStep` è la formula qui sotto, e i consumatori del boundary la leggono.

Al decision boundary che cade dopo il micro-step `k`, il facing dell'unità in movimento è:

```text
FacingAt(k) = URTFacingLibrary::FacingFromPath( Path[0..k], FacingAtMoveStart )
```

cioè la funzione **già esistente e testata**, applicata al **prefisso** del percorso già percorso.

| Boundary | Facing letto dai consumatori |
|---|---|
| `k = 0` (nessun passo ancora compiuto) | il facing d'ingresso nel Move — il prefisso è vuoto e `FacingFromPath` restituisce `Current` |
| `k` intermedio | la direzione del passo da `Path[k-1]` a `Path[k]` |
| ultimo passo | coincide **per costruzione** con `FacingFinalAfterMove` di D-020 |

Tre proprietà, tutte derivate e nessuna aggiunta:

- **nessun caso speciale al primo passo**: il prefisso vuoto è già gestito da `FacingFromPath`;
- **continuità col facing finale garantita**: l'ultimo prefisso *è* il percorso intero, quindi non esiste un
  salto fra l'ultimo boundary e la fine del Move;
- **nessun numero, nessuno stato nuovo**: il facing intermedio non si memorizza, si calcola dal percorso già
  risolto.

Il **pivot finale** della §1 si applica **dopo** l'ultimo micro-step, e **non retroattivamente** (handoff §9):
i boundary che sono già passati hanno letto il facing derivato, e nessuna rotazione successiva li rilegge.

**Durante il movimento il facing non passa dal budget di pivot.** Il budget della §1 governa la rotazione
**dichiarata** a fine movimento; il facing intermedio è **derivato puro**. Confonderli renderebbe illegale un
facing che l'unità assume soltanto perché sta camminando.

#### Perché non le altre due opzioni

- **Direzione del prossimo passo pianificato** — **scartata perché viola un requisito di privacy già
  dichiarato**. Il facing *assunto* è pubblico (ADR-0005 §5: «è una posa osservabile»), quindi un avversario
  che lo osservasse a metà movimento dedurrebbe **dove l'unità sta per andare**. Contro l'invariante **#6** e
  contro ADR-0004 §7-bis, che estende la privacy oltre il payload.

  > ⚠️ **Precisazione, 2026-08-10 (review post-merge).** La prima stesura di questa riga diceva «viola un
  > invariante **già testato**» e citava `Overwatch.OpportunityLeaksNoFuture` come se esistesse. **Non
  > esiste**: è un test *pianificato* nella tabella Verifica di ADR-0004, che appartiene a **E14** e non è
  > implementata — `grep -rl OpportunityLeaksNoFuture Source/` non restituisce nulla. L'argomento regge
  > comunque, perché l'invariante #6 è canonico e il requisito è scritto; ma «testato» era falso, ed è
  > precisamente il difetto — *una regola data per verificata che nessuno ha verificato* — che questa stessa
  > PR apriva come riga 66 di `DOC_CONFLICT_MATRIX.md` a proposito di `FR-RESOLVE-02`.
  >
  > ✅ **Aggiornamento, 2026-08-10 (CP 14.3, PR #494).** Il test **ora esiste**:
  > `RefactorTactics.Overwatch.OpportunityLeaksNoFuture` in `Tests/RTReactionOpportunityTests.cpp`, e
  > `grep -rl OpportunityLeaksNoFuture Source/` restituisce quel file. Verifica che il DTO
  > `FRTReactionOpportunity` non esponga campi fuori da un elenco chiuso, interrogando la reflection.
  > ⚠️ Con una portata **piu' stretta** di quella che la prima stesura gli attribuiva: difende la **forma
  > del DTO**, non il comportamento del resolver — che non costruisce ancora opportunity (wiring rimandato
  > a CP 14.4). Quindi «l'invariante e' testato» resta **falso per il facing**: qui si verifica che un campo
  > di informazione futura non possa entrare nel DTO, non che il facing intermedio non ne trapeli.
- **Facing invariato fino a fine Move** — scartata: contraddice D-020, per cui il facing cambia più volte dentro
  il round e ogni consumatore legge il valore autorevole più recente. Renderebbe inoltre l'aggiramento durante
  il movimento privo di effetto: un'unità che percorre un corridoio offrirebbe lo stesso lato all'Overwatch per
  tutto il tragitto, e il facing salterebbe visibilmente alla fine.

### 3. Il facing si dichiara per azione e per effetto (`FAC-2`)

> ⛔ **DECISA E NON COSTRUITA** — [`D-318`](RT_PDR_00_Decision_Log.md), 2026-09-03. La §Revisione di questo ADR fissava la rilettura «alla chiusura di `CP 14.3`»: il checkpoint è chiuso dal 2026-08-12, nessuna azione dichiara una policy — i due enum non esistono in `Source/` — e il caso reale è stato **chiesto** all'autore su `FAC-16`, che il 2026-09-01 ha risposto *«al momento no»*. Il modello resta canonico e i default restano descrittivi del comportamento vigente; ciò che si sospende è l'implementazione. I due test di §3 **non si scrivono**, e l'assenza è dichiarata invece che lasciata aperta.

Le due regole universali — D-020 («un'azione con bersaglio orienta prima di risolvere») e ADR-0005 §3
(«chi è spostato si gira verso la sorgente») — smettono di essere **implicite nel resolver** e diventano il
**valore di default** di una policy dichiarata sul dato.

```text
ERTActionFacingPolicy      (sull'azione)
  TurnToTarget    <- DEFAULT: riproduce D-020, il comportamento di oggi
  TurnToDirection            per le azioni direzionali senza bersaglio
  KeepFacing                 l'azione non orienta
  Free                       l'orientamento è scelto in planning, entro il budget della §1

ERTDisplacementFacingPolicy (sull'effetto di spostamento)
  FaceSource      <- DEFAULT per `Forced`:        riproduce ADR-0005 §3
  Keep            <- DEFAULT per `Environmental`: riproduce ADR-0005 §3
```

**La regola che rende questo cambio economico**: un'azione che **non dichiara** nulla si comporta **esattamente
come oggi**. I default non sono una cortesia, sono il meccanismo con cui il modello nuovo non richiede di
ricompilare l'intero catalogo prima di funzionare, e con cui nessun test esistente cambia esito.

Questo risponde all'obiezione registrata in `OPEN_DECISIONS.md` — *«il costo si paga se esiste un caso reale che
la regola universale sbaglia: nessuno è stato prodotto»*. Il caso continua a non esistere: la policy si adotta
**per avere dove scriverlo quando esisterà**, e nel frattempo costa un enum con un default, non una compilazione.

### 4. Vocabolario (`FAC-10`, risolto come conseguenza)

Il canone diceva «rotazione dichiarata», l'handoff dice `Pivot`. Non sono sinonimi, e tenerli entrambi è
corretto purché nominino cose diverse:

| Termine | Significato | Dove vive |
|---|---|---|
| **pivot** | la **capacità** di ruotare a fine movimento, misurata in step | `MoveEndPivotMaxSteps`, `DashEndPivotMaxSteps` — §1 |
| **rotazione dichiarata** | l'**atto** di scegliere una direzione in planning, dentro quel budget | `FRTPlannedIntent`, `TryApplyDeclaredFacing`, `DeclaredInPlanning` |

Il codice aveva già scelto `Declared*` per il secondo (`DeclarationRejected`, capability `DeclaredRotation`) e
non va toccato. `Reposition` resta una **mobilità** a catalogo e non entra in questo vocabolario.

### 5. Cosa **non** cambia

Il facing resta stato di gioco intero a sei valori · l'arco frontale resta `HexCone` e resta **uno solo** per
difesa, percezione e reazioni (ADR-0005 §4) — ⚠️ **emendato il 2026-08-13 da
[D-126](RT_PDR_00_Decision_Log.md)**: `HexCone` resta la **geometria d'area** dei tre consumatori, che non
cambiano, ma **non è più la primitiva semantica** del facing; quella sono le sei direzioni relative, e
l'insieme di lati che un'abilità dichiara appartiene al consumatore. Il cono è **strettamente contenuto**
nell'insieme dei tre lati frontali (**45** celle di divergenza a raggio `1..10`, tutte nello stesso verso —
diceva `50`, cifra della regola a linea poi scartata, corretta da [D-147](RT_PDR_00_Decision_Log.md)),
quindi sostituirlo cambierebbe il bilanciamento: vedi [ADR-0005 §4-bis](adr-0005-orientamento.md) e
[#726](https://github.com/DegrassiAaron/refactor-tactics-main/issues/726) · la difesa direzionale (§4a), la consapevolezza ravvicinata a 2
celle (§4b) e il cono dell'Overwatch derivato dal facing (§4c) sono **invariati** · la rotazione **non consuma
slot** · la timeline di D-020 e i suoi sei punti restano quelli · `Deflect`, `Brace`, `Shield` continuano a
proteggere da ogni direzione — `FAC-3` **non è decisa da questo ADR** e resta aperta.

## Alternative considerate

| Alternativa | Esito |
|---|---|
| Mantenere ADR-0005 §1 (tre direzioni per stile, zero numeri nuovi) | **Scartata dall'utente** a favore del modello dell'handoff |
| Pivot per **archetipo** (Heavy/Standard/Agile) invece che per eroe | Scartata: l'handoff §8 dice esplicitamente «NON deve essere automaticamente uguale per ruolo» |
| Un solo `PivotMaxSteps` per eroe, valido per Move e Dash | Scartata: annulla la distinzione fra le due mobilità, che è il motivo per cui l'handoff §10 chiede due policy |
| `StationaryPivotMaxSteps` per eroe | Scartata: terzo numero a testa senza un caso che lo richieda |
| Policy dichiarative **senza** default, compilate per ogni azione | Scartata: costringerebbe a compilare l'intero catalogo prima che il modello funzioni |
| Facing intermedio = prossimo passo pianificato | **Scartata**: viola l'invariante #6 e `Overwatch.OpportunityLeaksNoFuture` |
| Facing intermedio invariato fino a fine Move | Scartata: contraddice D-020 e annulla l'aggiramento durante il movimento |

## Conseguenze

**Positive**: la stessa cella d'arrivo ha valore tattico diverso a seconda del lato da cui la si raggiunge, e un
percorso più lungo può essere migliore perché consente un orientamento favorevole · Riktor e Wraith acquistano
un'identità di movimento leggibile · il facing intermedio è definito, quindi il DoD di **E16** («snapshot e
TurnLog dicono *quale* facing ha usato ciascun consumatore») torna verificabile · `CP 14.2`, `CP 14.4` e
`CP 14.7` non sono più bloccati da una lacuna.

**Negative / costi**:

- **otto numeri nuovi** (2 × 4 eroi), cioè un asse di bilanciamento che prima non esisteva. Vanno misurati, e
  finché non lo sono restano ipotesi scritte nel catalogo;
- `URTFacingLibrary::LegalFacings` cambia semantica: non basta più lo **stile**, serve il **budget dell'eroe**.
  I test `Facing.BudgetMoveAllowsLastStepPlusMinusOne` e `Facing.StationaryUnitRotatesFreely` vanno riletti —
  il primo descrive un comportamento che ora è quello di un eroe con `MoveEndPivotMaxSteps = 1`, non la regola;
- il **pathfinding** guadagna un motivo per essere orientation-aware, che `FAC-9` teneva fuori dalla v0.1: la
  preview del percorso deve mostrare il facing ottenibile, altrimenti il giocatore non può usare la capacità che
  gli si è appena data. `FAC-9` **resta fuori** — il ripiego (path geometrico, facing derivato, pivot validato
  alla fine) regge — ma la pressione su di esso aumenta;
- due enum di policy entrano nel dato delle azioni e degli effetti, quindi nella serializzazione del catalogo;
- il **bot** deve considerare il proprio budget di pivot quando sceglie la cella d'arrivo, altrimenti pianifica
  orientamenti che non può assumere. Ricade su [#202](https://github.com/DegrassiAaron/refactor-tactics-main/issues/202).

**Invarianti**: **#1** invariato (la presentazione atterra sul facing logico) · **#2** rispettato (step interi,
nessun angolo continuo) · **#4** rispettato (interi, ordine stabile; il facing intermedio è una funzione pura del
percorso) · **#6** **rispettato e usato come criterio**: è la ragione per cui il facing intermedio non anticipa
il passo successivo · **#7** rispettato (`FacingFromPath` è già una funzione pura testata).

## Verifica

> **Stato misurato il 2026-09-03**, dopo #1605 e #2131. Questa tabella era una lista di test **attesi** e non
> lo dichiarava: il referto del 2026-08-31 aveva misurato **0 occorrenze** per tutte le righe, e `D-295` §(2)
> ne citava una come se esistesse — riga **corretta il 2026-09-03**. La colonna *Stato* esiste perché
> quell'equivoco non si ripeta.
>
> ⚠️ **Un `✅` qui dice che il test esiste ed è verde, non che il verde sia informativo.** Le tre righe marcate
> *caratterizzazione* pinnano la formula della §2 — `FacingFromPath` applicata a un prefisso — che era corretta
> prima di #2131 e lo è dopo: erano verdi al primo colpo, per costruzione. Ciò che #2131 ha cambiato è **chi
> legge quel valore**, e a misurarlo sono le due righe di *integrazione* più lo scenario. La distinzione è
> scritta perché la somma «cinque test verdi» non racconta cosa è stato verificato.

| Test | Cosa dimostra | Stato |
|---|---|---|
| `Facing.PivotBudgetLimitsLegalFacings` | con `MoveEndPivotMaxSteps = 1` sono legali `D` e `D±1`; con 3 tutte e sei | ✅ §1 |
| `Facing.PivotBudgetZeroKeepsMovementDirection` | budget 0 (Riktor in Dash) ⇒ una sola direzione, quella del movimento — il comportamento di ADR-0005 conservato | ✅ §1 |
| `Facing.MoveAndDashBudgetsAreIndependent` | il budget del Dash non è quello del Move sullo stesso eroe | ✅ §1 |
| `Facing.StationaryRotationIsUniversal` | da fermo tutte e sei restano legali per ogni eroe | ✅ §1 |
| `Facing.CatalogPivotBudgetsMatchAdr0008` | gli otto numeri della §1 letti dal **catalogo**, non da costanti — aggiunto da #1605, non era in questa tabella | ✅ §1 |
| `Facing.UnconfiguredUnitKeepsAdr0005Behaviour` | il default `Move 1 / Dash 0` conserva ADR-0005 per un'unità senza eroe — idem | ✅ §1 |
| `Facing.MicroStepFacingIsLastCompletedStep` | al boundary `k` il facing è la direzione di `Path[k-1] → Path[k]` | ✅ §2 — ⚠️ **caratterizzazione dichiarata**: pinna la formula, non chi la legge |
| `Facing.MicroStepZeroKeepsEntryFacing` | al primo boundary il facing è ancora quello d'ingresso | ✅ §2 — ⚠️ caratterizzazione dichiarata |
| `Facing.MicroStepFacingMatchesFinalAtLastStep` | l'ultimo boundary e `FacingFinalAfterMove` coincidono | ✅ §2 — ⚠️ caratterizzazione dichiarata |
| `Facing.FinalPivotIsNotRetroactive` | il pivot finale non cambia il facing che i boundary precedenti hanno letto | ✅ §2 — **integrazione**, passa dal ciclo dei micro-step vero. È il test che `D-295` §(2) dava per esistente (riga corretta il 2026-09-03) e che ora esiste |
| `Overwatch.TriggerReadsMicroStepFacing` | il trigger valuta l'arco sul facing del boundary, non su quello iniziale né su quello finale | ✅ §2 — l'arco è quello degli **osservatori** che alimentano `TeamAwareness`, non il cono dichiarato del watcher: quello è `FRTArmedOverwatch::Facing` e non si rilegge (§«dopo l'impegno»). [#1933](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1933) resta confinante e non è mai stata bloccante |
| ~~`Facing.DefaultActionPolicyMatchesD020`~~ | un'azione che non dichiara policy orienta verso il bersaglio come oggi | ⛔ §3 — **non si scrive**: la §3 non si implementa ([`D-318`](RT_PDR_00_Decision_Log.md)). Il comportamento resta quello di `D-020`, coperto da `Facing.TargetChangeWithinRoundReorients` |
| ~~`Facing.DefaultDisplacementPolicyMatchesAdr0005`~~ | `Forced` ⇒ verso la sorgente, `Environmental` ⇒ invariato, senza dichiarazioni | ⛔ §3 — **non si scrive** ([`D-318`](RT_PDR_00_Decision_Log.md)). Il default è già pinnato da `Facing.ForcedMovementFacesSource` e `Facing.EnvironmentalDisplacementKeepsFacing`, che misurano la regola e non la policy |
| suite `Facing.*` e `HexMove.*` esistenti | nessun esito cambia per le azioni che non dichiarano policy | ✅ regressione — nessuna policy è stata introdotta, quindi nessun esito poteva cambiare per quella via |
| `Spec.Facing.OverwatchHitCameFromSide` | il facing del micro-step arriva fino al TurnLog di una partita, e il pivot finale non lo rilegge | ✅ §2 — scenario, aggiunto alla tabella da #2131 |

## Revisione

**Prima revisione — i numeri.** Alla chiusura di **CP 16.2**, con i dati del playtest. I valori della §1 sono
ipotesi: la domanda da porre è se il pivot sia diventato un asse di scelta reale o solo un numero da consultare.

**Soglia di allarme**: se il pivot alto risulta sempre preferibile — cioè se Wraith e Phase dominano il
posizionamento per la sola rotazione — la via di rientro è **comprimere la scala** (portare tutti a 1–2 step),
non rimuovere il modello.

**Seconda revisione — le policy.** Alla chiusura di **CP 14.3**. Se a quel punto **nessuna** azione del catalogo
ha dichiarato una policy diversa dal default, la §3 va riconsiderata: significherebbe che il caso reale che
giustifica il costo continua a non esistere, e l'obiezione registrata in `OPEN_DECISIONS.md` era fondata.

> ✅ **Eseguita il 2026-09-03, con esito: la §3 NON si implementa.** Registrata in
> [`D-318`](RT_PDR_00_Decision_Log.md). Le tre misure che la chiudono:
>
> 1. **`CP 14.3` è chiusa dal 2026-08-12** ([#163](https://github.com/DegrassiAaron/refactor-tactics-main/issues/163)) — il trigger di revisione era già scaduto da ventidue giorni;
> 2. **nessuna azione del catalogo dichiara una policy diversa dal default**, e non per omissione: `ERTActionFacingPolicy` e `ERTDisplacementFacingPolicy` hanno **0** occorrenze in `Source/`, quindi non esiste nemmeno la sede in cui dichiararla;
> 3. **il caso reale continua a non esistere, e stavolta è stato *chiesto***: `FAC-16` di [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) ha posto all'autore la domanda *«esiste un caso di gioco in cui girarsi verso la sorgente produce un esito sbagliato?»* e la risposta del **2026-09-01** è *«al momento no»*.
>
> ∴ La condizione che questa sezione fissa è **soddisfatta**, e la conseguenza che essa stessa prescrive è di
> riconsiderare. La §3 resta scritta perché la decisione `FAC-2` è accettata e il modello è quello giusto il
> giorno in cui servirà; ciò che si sospende è la **costruzione**. La via di rientro è in `FAC-16` e non
> richiede una decisione nuova: *«se un caso emerge, si dichiara `Keep` su quell'effetto»*.
