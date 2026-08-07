# Spec — Terreno dinamico, fuoco e acqua (E8, CP 8.4)

> **Issue**: `#67` · **Epic**: `#22` (E8) · **Dipende da**: `#66` (CP 8.3, chiusa) · **Data**: 2026-08-07
> **Branch**: `feat/67-fuoco-acqua` (worktree isolato) · **Baseline misurata**: 359 test in 53 file
> Fonti: [`RT_TerrainCatalog_v0.1.md`](balance/RT_TerrainCatalog_v0.1.md) §2 ·
> [`RT_ActionCatalog_v0.1.md`](balance/RT_ActionCatalog_v0.1.md) §6 ·
> [`spec-stati-temporanei-cp82.md`](spec-stati-temporanei-cp82.md) §4 ·
> [`spec-propagazione-elettrica-cp83.md`](spec-propagazione-elettrica-cp83.md)

## 1. Obiettivo

Fuoco e acqua si annullano in modo prevedibile, e **la mappa cambia stato durante la partita**. È il pezzo che
CP 8.3 aveva dichiarato assente: fino a qui la mappa era un asset immutabile, e «l'acqua rimuove la cella di
fuoco» non era esprimibile.

## 2. Decisioni

### D1 — Il terreno dinamico vive in due pezzi, divisi per natura

| Cosa | Dove | Perché |
|---|---|---|
| Superficie **corrente** | nella mappa (`URTHexMapAsset`) | è ciò che **tutti** leggono già: costi di movimento, blocco Dash, scivolata sul ghiaccio, cap di targeting del fumo, effetti d'ingresso, conduttività elettrica. Un secondo posto da consultare sarebbe un secondo modello di verità, e basta un lettore dimenticato per avere due mappe diverse nello stesso turno |
| Superficie **originale** + turni rimanenti | `ARTTurnManager::DynamicSurfaces` | è **stato di partita**, non dato di mappa: due partite sulla stessa arena non devono ereditarsi il fuoco |

*Alternativa scartata*: un overlay `TMap<FRTCellId, …>` consultato da tutti i lettori. Sarebbe stato corretto
solo se **tutti e otto** i punti di lettura fossero stati aggiornati insieme — e il costo di sbagliarne uno è
un difetto silenzioso che si manifesta come «il costo di quella cella non torna».

### D2 — La partita lavora su una COPIA della mappa d'autore

`ApplyMapSource` duplica l'asset del livello. Le due arene generate (`MakeTestArena`, `MakeDemoArena`)
costruivano già un oggetto nuovo a ogni partita: qui si allinea il terzo caso.

*Perché*: modificare l'asset su disco sporcherebbe il contenuto del progetto — in PIE le modifiche
sopravvivrebbero allo Stop, e due partite di fila non partirebbero dallo stesso campo. Cioè addio determinismo,
e nel modo peggiore: silenzioso e dipendente da quante volte hai premuto Play.

### D3 — `Action.Ignite` e `Action.CreateWater` entrano nel catalogo

Senza un'azione che le inneschi, le interazioni non avrebbero consumatore. È la stessa ragione per cui
`Action.Electrify` è entrata in CP 8.3, ed è il difetto ricorrente di questo repository.

Nessuna delle due dichiara `Effects`: il loro esito non è un effetto su un'**unità** (danno, cura, stato) ma
una modifica della **cella**, che `FRTActionEffectSpec` non sa esprimere. La coppia azione→superficie è oggi
l'unico punto in cui l'orchestratore guarda un `ActionId`; quando le azioni ambientali saranno molte (CP 8.5),
il posto giusto è un campo del catalogo, non un terzo `if`.

### D4 — Che cosa brucia è una proprietà della superficie

`FRTTerrainDef::bIsFlammable`, letto dal catalogo. Il fuoco non attecchisce su acqua e metallo perché **quelle
superfici dichiarano di non bruciare**, non perché il resolver conosca un elenco di eccezioni.

**Limite dichiarato del v0.1**: il catalogo elenca come combustibili «vegetazione, olio, gas», che **non
esistono** fra le otto superfici. Oggi bruciano solo `Floor` e `Rough` (terreno neutro), quindi il fuoco **non
si propaga da solo**: la propagazione è vuota *per costruzione del catalogo*, non per codice mancante. Un test
conta le superfici combustibili, così il giorno in cui ne verrà aggiunta una il conteggio lo dirà.

### D5 — Ordine nel Cleanup: la scadenza precede le nuove modifiche

1. **scadenza** delle modifiche dei turni precedenti · 2. **azioni ambientali** di questo turno (scarica,
fuoco, acqua) · 3. danno di `Burning` · 4. revoca stati di cella · 5. durate · 6. energia/cooldown ·
7. conteggio vivi.

*Perché il tick è per primo*: se venisse dopo, mangerebbe subito un turno di durata a una cella appena
incendiata — il fuoco durerebbe 1 turno invece dei 2 dichiarati. **Trovato da un test**, non a tavolino.

### D6 — «Si allaga» e «si spegne» sono due esiti diversi nel TurnLog

`ERTEnvironmentOutcome`: `SurfaceChanged`, `SurfaceRestored`, `SurfaceRejected`, `SurfaceExtinguished`. Per chi
legge il replay non sono lo stesso evento, e un'azione che **non** ha effetto (il fuoco rifiutato dall'acqua)
deve comparire: un'azione che non fa nulla in silenzio è peggio di una che fallisce a voce alta.

`ERTLogCategory::Environment` è aggiunta **in coda** all'enum, come `Fallback` e `Reaction` prima di lei: le
tracce già scritte restano leggibili.

## 3. Test

| Test | Cosa fisserebbe se cadesse |
|---|---|
| `Environment.WaterExtinguishesFire` *(nome vincolante, catalogo §15)* | l'acqua non spegne il fuoco, o il log non lo distingue |
| `Environment.Fire.DoesNotIgniteWaterOrMetal` | il fuoco attecchisce dove non deve; conta anche le superfici combustibili del catalogo |
| `Environment.ChangesAppearInTurnLog` | accensione, durata (2 turni) e ripristino non sono osservabili |

`MatchSetup.MapSourceLevelAssetKeepsAuthoredMap` è stato **sostituito, non cancellato**: verificava l'identità
del puntatore (`MapAsset == Authored`), ora verifica il **contenuto** (stesso `ComputeHash`) e in più che la
partita lavori su una copia. La garanzia del checkpoint originale — «la mappa d'autore non viene rimpiazzata
dall'arena demo» — resta verificata.

### 3.1 Verifiche di mutazione eseguite

| Mutazione | Test caduto | Atteso |
|---|---|---|
| `bIsFlammable` ignorato (brucia tutto) | `Fire.DoesNotIgniteWaterOrMetal` | ✅ |
| la scadenza non ripristina la superficie originale | `ChangesAppearInTurnLog` | ✅ |
| l'acqua sul fuoco non è distinta dall'allagamento | `WaterExtinguishesFire` | ✅ |
| nessuna copia della mappa d'autore | `MapSourceLevelAssetKeepsAuthoredMap` | ✅ |

Quattro mutazioni **disgiunte** in un solo giro (colpiscono test diversi), con l'implementazione già committata
e `Result: Succeeded` verificato sull'output completo della build — le due lezioni dei checkpoint precedenti.

**Suite**: **362 test unici in 54 file** (da 359 in 53), 0 fallimenti, entrambi i target verdi.

## 4. Fuori scope dichiarato

- **Propagazione del fuoco fra celle**: vuota per costruzione (vedi D4). Quando esisterà una superficie
  combustibile, l'algoritmo è lo stesso BFS di CP 8.3.
- **Targeting per cella**: `Ignite`/`CreateWater` colpiscono la cella del **bersaglio**, come `Electrify`.
  Incendiare una cella vuota richiede il bersaglio-cella dell'HUD (E11).
- **`Flux.ConductiveNode`**: ora sarebbe rappresentabile (il terreno dinamico esiste), ma cablarla è **CP 8.5**
  insieme alle altre azioni ambientali — qui sarebbe un cambio al catalogo eroi fuori scope.
- **Congelare l'acqua / evaporare**: il catalogo le elenca fra le interazioni, ma nessuna azione del v0.1 le
  produce. `Ice` esiste come superficie e scivola già (CP 8.1).
- **Rendering del cambiamento**: `RebuildInstances` reagisce a `OnMapChanged`, quindi il colore della cella
  cambia; la presentazione vera (VFX di fuoco e acqua) è M8.

## 5. File coinvolti

| File | Modifica |
|---|---|
| `Turn/RTTurnLog.h` | `ERTLogCategory::Environment`, `ERTEnvironmentOutcome` |
| `Terrain/RTTerrainData.h`, `RTTerrainLibrary.cpp` | `bIsFlammable` nel catalogo (Floor, Rough) |
| `Ability/RTCatalogLibrary.cpp` | `Action.Ignite`, `Action.CreateWater` |
| `Turn/RTTurnManager.{h,cpp}` | `DynamicSurfaces`, `ApplyDynamicSurface`, `TickDynamicSurfaces`, cablaggio nel Cleanup |
| `RTGameMode.cpp` | copia di lavoro della mappa d'autore |
| `Tests/RTFireWaterTests.cpp` (nuovo), `Tests/RTMatchSetupWorldTests.cpp` | i tre test + quello sostituito |

## 6. Rischi

- **Il golden hash cambia ancora**: il TurnLog guadagna la categoria `Environment`. Atteso; il corpus di
  CP 12.6 va generato dopo E8 (già registrato in CP 8.3).
- **La mappa non è più costante durante la partita**: chi catturasse un `const URTHexMapAsset*` a inizio turno
  e lo rileggesse dopo il Cleanup vedrebbe superfici diverse. È il comportamento voluto, ma è un'assunzione
  implicita che qualche funzione pura potrebbe fare: nessuna oggi la fa (tutte ricevono la mappa come parametro
  a ogni chiamata).

---

## 7. CP 8.5 — le azioni ambientali e di supporto *(2026-08-07, issue `#68`, chiude E8)*

L'ultimo checkpoint di E8 completa il catalogo ambientale con le tre azioni che mancavano.

| Azione | Cosa fa | Nota |
|---|---|---|
| `Action.Heal` | 20 HP, portata 3, priorità 70 | risolve nel **Blast dopo i danni**: cura le ferite di *questo* turno |
| `Action.CreateWater` | esteso al **raggio 1** del catalogo, con `Wet` a chi è già sulle celle | CP 8.4 applicava la sola cella del bersaglio |
| `Action.ModifyArc` | apre o chiude il collegamento fra chi la usa e il bersaglio | incrementa la **revisione** della mappa |

### 7.1 Decisioni

**La cura non resuscita.** Tre regole del catalogo verificate insieme: non supera la salute massima, non
rimuove stati (si tocca solo `Health`), e non riporta in piedi chi è caduto in questo turno — un KO
reversibile sarebbe una regola di gioco diversa, che nessun documento dichiara.

**`ERTCombatOutcome::Healed`** registra il valore **effettivo**: curare a salute piena scrive `0`, non `20`.
Un log che dicesse «curato 20» mentre gli HP non cambiano sarebbe un log che mente.

**`ModifyArc` e la revisione.** La revisione è il numero che invalida le cache di percorso. Se cambiare la
topologia non la incrementasse, un percorso calcolato prima resterebbe valido dopo: un'unità camminerebbe su
un ponte che non c'è più. `AddTransition`/`RemoveTransition` la incrementavano già — qui si verifica che
l'azione ci passi davvero.

*Limiti dichiarati*: l'arco è identificato dalla coppia (chi usa l'azione, bersaglio), perché la
pianificazione non ha un bersaglio-**arco** — arriverà con E9/E11; e il ponte creato **non scade**, perché la
durata degli archi è CP 9.4 (ponti e porte).

**`Action.CreateCover` resta fuori**, ed è la stessa decisione presa per `Gadget.Insulator` in CP 8.3: le
coperture non esistono nel modello dati (`FRTHexCellData` non ha bordi protetti) e costruirle è **E9**.
Un'azione che dichiarasse di crearle sarebbe inerte — il difetto che questi checkpoint chiudono, non uno da
aggiungere. Un test fissa l'assenza, così la riga non si perde.

### 7.2 Due difetti trovati dai test

1. **`ResolveCombat` usciva prima di applicare le cure** quando nessuno attaccava (`if (Attacks.Num() == 0)
   return;`). Una cura fuori da uno scontro è il caso **normale** di un supporto: il pass è diventato una
   funzione chiamata da entrambi i punti d'uscita.
2. **Collisione unity build su `RunTurn`**: due file di test avevano un helper omonimo in namespace anonimo, e
   la collisione è comparsa solo aggiungendo un file nuovo — il raggruppamento unity cambia. Rinominato in
   `RunStatusTurn`. È il caso già documentato nei gotcha di build: *un build verde non è garanzia di assenza
   di collisione*.

Un terzo difetto era **del test**, non del codice: senza nemici in campo la partita finisce al primo turno
(`EvaluateOutcome`), quindi il secondo turno non risolveva e la seconda cura non arrivava mai. Il test ora
tiene in campo un avversario inerte, invece di aggirare la regola di fine partita.

### 7.3 Verifiche di mutazione

| Mutazione | Test caduto |
|---|---|
| la cura non viene limitata alla salute massima | `Actions.Heal.RestoresWithoutExceedingMax` |
| `CreateWater` torna alla sola cella bersaglio | `Actions.CreateWater.CoversRadiusAndWetsOccupants` |
| `ModifyArc` non tocca la topologia | `Actions.ModifyArc.BumpsChunkRevision` |

**Suite alla chiusura di E8**: **366 test unici in 55 file**, 0 fallimenti, entrambi i target verdi.
