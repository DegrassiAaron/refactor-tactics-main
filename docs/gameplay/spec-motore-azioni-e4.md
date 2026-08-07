# Spec E4 — Motore azioni data-driven

> **Stato**: proposta di design, **da approvare** · **Data**: 2026-08-06 · **Epic**: E4 (`#42`–`#46`)
> **Prerequisiti già in `main`**: `FRTActionDef` + `URTCatalogLibrary` (CP 1.3/1.4), cataloghi in
> [`balance/`](../balance) (CP 1.2), turno interamente esagonale (M6).
> **Decisione abilitante**: [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md).
>
> ⚠️ Le issue di E4 dichiarano una dipendenza da **`#40`** (rimozione del gameplay quadrato), che a sua volta
> attende il playtest **CP 6.8**. Questa spec si può leggere e correggere subito; l'implementazione parte quando
> quella catena si sblocca.

## 1. Dov'è il problema, oggi

Il turno funziona ed è testato, ma **cosa fa un'azione è scritto in `ARTTurnManager`**, non nei dati:

| Dove | Cosa decide il codice |
|---|---|
| `ResolvePrep` | «se l'abilità è `bSelfTarget`, aggiunge scudo pari a `Power`» |
| `ResolveDash` | «se l'abilità è `bDash`, sposta entro `RangeCells`» |
| `ResolveCombat` | «altrimenti è un attacco: forma da `Shape`, danno `Power`, più i flag `bKnockback` / `bIgnites` / `StatusToApply`» |

Finché le azioni sono 8 (due archetipi × 4) il costo è basso. Il catalogo v0.1 ne prevede **~35**, con
`Counter`, `Intercept`, `Push`, `Pull`, `Root`, `Interrupt`, `Slow`, `CreateWater`, `ModifyArc`… Ognuna
aggiungerebbe un `if` in una delle tre funzioni, e il catalogo elenca fra gli errori da evitare proprio
«creare una classe C++ diversa per ogni variante numerica».

Il secondo problema è l'**ordine**. Oggi l'ordine intra-fase è implicito: gli attacchi si sommano per
bersaglio (ordine-indipendente per costruzione), il che regge finché tutti gli effetti sono additivi. Con
spinte, interruzioni e reazioni **l'ordine conta**, e il catalogo lo definisce: priorità intera crescente,
poi `ActionDefinitionId`, `SourceUnitId`, `EventSequence`.

## 2. Decisioni proposte

| # | Decisione | Perché |
|---|---|---|
| **D1** | Un'azione è **dati + un effetto nominato**, non una sottoclasse per azione | Il catalogo vieta una classe per variante numerica. `FRTActionDef` porta i parametri; l'effetto è un `FName` (`Effect.Damage`, `Effect.Shield`, `Effect.Push`, `Effect.Move`…) risolto da un registry |
| **D2** | Il registry mappa `EffectId → funzione pura` `(Snapshot, ActionInstance) → TArray<FRTActionEvent>` | Le funzioni restano testabili senza Actor, come tutto lo strato hex. Un'azione **non muta** lo stato: **produce eventi**, che un applicatore applica in blocco (invariante #3, «raccogli poi applica») |
| **D3** | L'ordinamento è **una sola funzione pura** `SortActionInstances`, con ordine totale `MacroFase → Priority → ActionId → SourceUnitId → EventSequence` | È la richiesta di CP 4.1. Una sola sede, testabile per permutazione, mai l'ordine di una `TMap` |
| **D4** | Gli **effetti di controllo** (Push/Root/Interrupt) risolvono nel Blast **prima** del danno, per priorità (10–50), non in una macro-fase nuova | ADR-0003 §3: una macro-fase in più cambierebbe TurnLog e playback senza aggiungere espressività |
| **D5** | Il **fallback** si valuta in un solo punto, prima dell'esecuzione: `ValidateInstance` → se non valida, `ApplyFallback` sostituisce l'istanza secondo `FRTActionDef::Fallback` | CP 4.3. Concentrare la validazione evita che ogni effetto reinventi «e se il bersaglio è morto?» |
| **D6** | Le azioni esistenti dei due archetipi **non vengono riscritte**: acquisiscono un `EffectId` e passano dal registry, con gli stessi numeri | Il gioco deve restare giocabile a ogni fetta. Il ribilanciamento è E6, non E4 |
| **D7** | Il budget passa a **5 MP** in una fetta dedicata (CP 4.2), **dopo** il registry | Cambiare budget e motore insieme renderebbe impossibile attribuire una regressione del bot all'uno o all'altro |

## 3. Modello proposto

```
FRTActionInstance          // un'azione pianificata da un'unità in questo turno
├─ FRTActionDef Def        // dal catalogo (ID, fase, priorità, range, costo, cooldown, fallback)
├─ int32 SourceUnitId      // chi la esegue (indice stabile nello snapshot)
├─ int32 TargetUnitId      // bersaglio, se l'azione ne ha uno
├─ FRTCellId TargetCell    // cella bersaglio (AoE, movimento, creazione terreno)
└─ int32 EventSequence     // ordine di dichiarazione: ultimo tie-break, mai casuale

FRTActionEvent             // effetto ELEMENTARE prodotto da un'azione, già risolto
├─ ERTActionEffect Kind    // Damage · Heal · Shield · Push · Status · MoveTo · TerrainChange
├─ int32 TargetUnitId / FRTCellId Cell
├─ int32 Amount            // intero, sempre
└─ FGameplayTag Status     // solo per Kind == Status
```

Il flusso di una macro-fase diventa lo stesso per tutte:

```
1. RACCOGLI   istanze della fase          → TArray<FRTActionInstance>
2. ORDINA     SortActionInstances          (ordine totale, D3)
3. VALIDA     ValidateInstance/ApplyFallback per ciascuna (D5)
4. PRODUCI    registry: istanza → eventi   (funzioni pure, D2)
5. APPLICA    gli eventi in blocco sullo snapshot iniziale
6. REGISTRA   TurnLog + eventi di playback
```

I passi 1–4 sono **puri e testabili headless**; solo il 5–6 tocca gli `ARTUnit`. È lo stesso schema già
usato per il Blast esagonale (`CollectHexAttacks` → `ResolveAttacks`), esteso a tutte le fasi.

## 4. Fette

| CP | Contenuto | Verifica |
|---|---|---|
| **4.1** | `FRTActionInstance`, `SortActionInstances`, registry vuoto ma cablato; `ResolveCombat` continua a funzionare passando dal nuovo ordinamento | `Actions.OrderByPriority` · `Actions.PermutationInvariant` · `Actions.PhaseMappingRespectsAtlas` |
| **4.2** | Budget **5 MP**, costi per cella (1/2/2), `Sprint` 8 MP con `Exposed`; **riparametrizzazione dei pesi del bot** | `Actions.Move.BudgetCosts` · `Actions.Sprint.AppliesExposed` · suite bot verde |
| **4.3** | I sei fallback, applicati in un solo punto e registrati nel TurnLog | un test per fallback · `Actions.Move.PathBlocked` (nome vincolante) |
| **4.4** | Le sei azioni fondamentali (`Wait`, `Move`, `BasicAttack`, `Guard`, `Activate`, `Interact`) come dati | `Actions.Guard.FirstHitOnly` · `Actions.Wait.AllowsFacingAndReaction` |
| **4.5** | Migrazione delle 8 azioni degli archetipi al registry, a numeri invariati | la suite esistente resta verde **senza modifiche ai test** |

L'ordine 4.1 → 4.2 non è negoziabile (D7). 4.3 e 4.4 possono procedere in parallelo dopo 4.1.

## 5. Rischi

| Rischio | P/I | Mitigazione |
|---|---|---|
| Il cambio di budget invalida i pesi del bot tarati su 4 celle | **H/M** | Fetta separata (CP 4.2) e suite del bot come gate, esattamente come chiede l'issue `#43` |
| Il registry diventa un `switch` travestito | M/M | Il criterio d'accettazione è: **aggiungere `Action.Push` non deve toccare `ARTTurnManager`**. Se lo tocca, il registry non serve |
| Doppia verità fra `URTActionData` (effetti legacy) e `FRTActionDef` | M/H | CP 4.5 chiude la migrazione: dopo, i flag `bDash`/`bSelfTarget`/`bKnockback` spariscono a favore di `EffectId` |
| «Raccogli poi applica» violato da un effetto che legge stato già mutato | M/**H** | Gli effetti ricevono lo **snapshot iniziale** della fase, mai lo stato vivo. Test di permutazione per ogni fase, non solo per il Blast |
| Scope: E4 tira dentro reazioni (E5) e terreni (E8) | M/M | `Counter`/`Intercept` restano fuori: hanno un trigger, non una fase. Sono E5 |

## 6. Cosa NON fa questa epic

Reazioni con trigger (**E5**) · terreni attivi e propagazione (**E8**) · strutture e coperture direzionali
(**E9**) · i quattro eroi (**E6**) · equipaggiamento (**E7**). E4 costruisce il **motore**; il contenuto arriva
dopo, ed è il motore a renderlo aggiungibile senza toccare il `TurnManager`.

## 7. Domande aperte per l'approvazione

1. **`EffectId` come `FName` o come `UENUM`?** L'enum è più veloce e verificabile a compile-time; l'`FName`
   permette effetti definiti da dati senza ricompilare. Proposta: **enum** finché gli effetti sono <20 — il
   modding è north-star, non v0.1.
2. **Lo scudo scade nel Cleanup?** Il catalogo dice di sì, il codice attuale no: è la issue `#96`, e la
   risposta cambia `Action.Guard` e `Action.Shield` in CP 4.4. Va decisa **prima** di quella fetta.
3. **`Action.Wait` risolve in `Move` o è fuori dalle fasi?** Il catalogo le dà fase 20 e priorità 100 (ultima).
   Proposta: macro-fase `Move`, priorità 100 — non fa nulla, ma resta osservabile nel TurnLog.
