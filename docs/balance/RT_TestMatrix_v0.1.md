# RT — Matrice di test v0.1

> **Fonte**: `docs/src/RefactorTactics — Catalogo e bilanciamento v0.1.pdf` §§10–12, §§14–16 · `docs/archive/pdr-v0.1/RT_PDR_12_Catalog_v0.1.pdf`
> **Checkpoint**: CP 1.2 (issue `#28`) · **Consumatori**: CP 12.2 (matrice manuale, `#82`) e CP 12.3 (suite automatica, `#83`)
> Le voci manuali confluiscono in [`test-manuali-pie.md`](../technical/test-manuali-pie.md) (sessione E); quelle automatiche
> diventano test dell'Automation Framework.

## 1. Regole di collisione simultanea

Sono la parte più delicata dei turni simultanei: **nessun esito può dipendere dal Player ID, dal ping o
dall'ordine dei pacchetti**.

| Situazione | Esito |
|---|---|
| Due unità entrano nella stessa cella, stessa priorità e stessa massa | **Entrambe** si fermano nella cella precedente |
| Una usa `Charge`, l'altra `Move` | `Charge` **prevale**: entra nella cella contesa, l'altra resta nella precedente |
| Cella occupata da un'unità immobile | L'unità in movimento si ferma **prima** della cella |
| Due `Charge` opposte | **Entrambe** si fermano prima della cella contesa; nessun danno aggiuntivo nella v0.1 |

Stato attuale: le prime tre regole valgono già sul substrato esagonale (`URTHexSimLibrary::ResolveHexPaths`,
microstep sincroni, punto fisso monotono). La precedenza di `Charge` **non esiste ancora**: arriva con le
priorità intere del motore azioni (epic E4).

## 2. Combo da verificare

### Acqua + elettricità
1. Riva crea acqua.
2. Un'unità entra o si trova nell'acqua.
3. Flux usa `Electrify` o `LinearDischarge`.
4. La propagazione segue le celle conduttive (max 3, ordine: distanza → `CellId` → `UnitId`).
5. Ogni bersaglio è colpito **una sola volta**.

**Rischio**: il friendly fire è attivo.
**Lettura in UI**: *confermato* = bersaglio già `Wet` e collegamento certo · *previsto* = propagazione valida
nello snapshot corrente · *incerto* = una cella potrebbe essere occupata o modificata durante il movimento.

### Copertura + tiro d'intercetto
1. Bastion crea un pannello.
2. Il pannello chiude il percorso più sicuro.
3. Vektor prepara `InterceptShot`.
4. Il nemico sceglie se attraversare la linea o perdere posizione.

### Push + hazard
1. Riva o Bastion applicano `Push`.
2. Il bersaglio entra in acqua, fuoco o area elettrificata.
3. L'effetto della cella si applica **al termine** dello spostamento.
4. La propagazione ambientale si risolve nella fase 50 (→ `Cleanup`, dopo il Move).

## 3. Test manuali minimi

| # | Test | Risultato atteso | Copertura oggi |
|---:|---|---|---|
| 1 | Due unità entrano nella stessa cella | Entrambe si fermano | ✅ `HexMove.ContestedCellStopsBoth` + `PIE-HEXPLAY-5` |
| 2 | `Move` attraversa terreno accidentato | Consuma 2 MP | ⏳ E8 |
| 3 | `Dash` incontra una copertura alta | Si ferma o viene invalidato | ⏳ E9 (`PIE-V01-DASHCOVER`) |
| 4 | `Push` verso una cella occupata | Nessuno spostamento illegale | 🟡 spinta esagonale fatta (CP 6.5), `Action.Push` da fare in E4 |
| 5 | Acqua colpita da elettricità | Propagazione deterministica | ⏳ E8 (`PIE-V01-ELEC`) |
| 6 | Fuoco colpito da acqua | Fuoco rimosso | ⏳ E8 (`PIE-V01-FIREWATER`) |
| 7 | Basic Attack attraverso copertura bassa | Danno ridotto di 10 | ⏳ E9 (`PIE-V01-LOWCOVER`) |
| 8 | `Intercept` protegge l'alleato | L'intercettore diventa il bersaglio | ⏳ E5 (`PIE-V01-INTERCEPT`) |
| 9 | AoE colpisce un alleato | Friendly fire applicato | ⏳ E4 (`PIE-V01-FF`) |
| 10 | Il bersaglio si sposta prima dell'attacco | Fallback applicato | ⏳ E4 |
| 11 | Porta chiusa durante il turno | Revisione del grafo aggiornata | ⏳ E9 |
| 12 | Replay dello stesso turno | TurnLog e risultato identici | ✅ `HexSim.ReplayDivergenceZero` |

## 4. Test automatici previsti

| Nome | Verifica | Stato |
|---|---|---|
| `RefactorTactics.Actions.Move.PathBlocked` | percorso bloccato → `Fallback.Stop` | 🟡 esiste l'equivalente hex (`HexMove.RejectsUnreachableCell`) |
| `RefactorTactics.Actions.Move.CellConflict` | destinazione contesa | ✅ `HexMove.ContestedCellStopsBoth` |
| `RefactorTactics.Actions.Dash.BlockedArc` | dash contro arco bloccato | ⏳ E9 |
| `RefactorTactics.Actions.Push.InvalidDestination` | spinta verso destinazione illegale | ✅ `HexCombat.KnockbackStopsBeforeObstacle` |
| `RefactorTactics.Environment.WaterElectricPropagation` | propagazione elettrica deterministica | ⏳ E8 |
| `RefactorTactics.Environment.WaterExtinguishesFire` | acqua spegne il fuoco | ⏳ E8 |
| `RefactorTactics.Cover.DirectionalDamageReduction` | copertura direzionale | ⏳ E9 |
| `RefactorTactics.Reactions.Intercept` | intercetto cambia bersaglio | ⏳ E5 |
| `RefactorTactics.Reactions.SingleActivation` | una sola attivazione per turno | ⏳ E5 |
| `RefactorTactics.Simulation.DeterministicReplay` | checksum identico | ✅ TurnLog hash + serializzazione con checksum |

**Protocollo di determinismo** (CP 12.1, `#81`): ogni test esegue la stessa simulazione con **stesso snapshot,
stesso seed, stesse definizioni, stesso ordine**, per almeno **100 ripetizioni**; il checksum finale deve essere
identico.

## 5. Debug richiesto

Comandi console (CP 11.4, `#80` — oggi **non esiste alcun `FAutoConsoleCommand`** nel progetto):

```
rt.Debug.DrawGrid 1        rt.Debug.DumpSnapshot
rt.Debug.DrawPaths 1       rt.Debug.DumpTurnLog
rt.Debug.DrawCover 1       rt.Debug.VerifyReplay
rt.Debug.DrawIntent 1
rt.Debug.DrawResolution 1
```

Informazioni per **cella**: `CellId` · `TerrainId` · `TraversalCost` · `OccupantId` · `HazardTags` · `CoverEdges` ·
`ChunkRevision`.
Informazioni per **azione**: `ActionId` · `SourceUnitId` · `Phase` · `Priority` · `Target` · `Fallback` ·
`ValidationResult` · `EventSequence`.

## 6. Errori da evitare (dal catalogo)

- float per costi, priorità o danni;
- danno applicato tramite `AnimNotify`;
- ricalcolo di A\* a ogni micro-step senza una regola esplicita;
- ordine di una `TMap` usato come ordine di risoluzione;
- intenti replicati su `GameState`;
- intenti nemici nascosti **solo** tramite UI;
- una classe C++ diversa per ogni variante numerica;
- un equipaggiamento migliore in ogni parametro;
- propagazione elettrica senza limite;
- selezione automatica casuale per i fallback;
- collisioni dipendenti dal ping o dall'ordine dei pacchetti.

Sono gli stessi divieti degli invarianti #1/#3/#4/#6 del canone, visti dal lato del contenuto.

## 7. Definition of Done del catalogo (dal PDF §18)

- [x] Ogni azione possiede un ID stabile
- [x] Ogni azione dichiara fase, priorità e fallback
- [x] Ogni terreno dichiara costo e interazioni
- [x] Ogni variante presenta almeno uno svantaggio
- [x] Le quattro identità degli eroi sono leggibili
- [ ] La combo acqua/elettricità è deterministica *(E8)*
- [x] Le collisioni simultanee hanno una regola *(scritta qui; la precedenza di `Charge` arriva con E4)*
- [x] Esiste un TurnLog verificabile *(hash + serializzazione versionata con checksum)*
- [ ] I test passano in Editor **e** packaged build *(CP 12.3/12.5)*
- [x] Nessun intento avversario viene replicato *(oggi banale: offline; canary in M10)*

Le voci non spuntate non sono debito di questo checkpoint: sono il lavoro delle epic che questo catalogo abilita.
