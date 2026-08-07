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
