# RT — Matrice di test v0.1

> `CURRENT` · **Ultimo aggiornamento**: 2026-08-08
> **Fonte**: `docs/src/prd/catalogo-e-bilanciamento-v0.1.pdf` §§10–12, §§14–16 · `docs/archive/pdr-v0.1/RT_PDR_12_Catalog_v0.1.pdf`
> **Checkpoint**: CP 1.2 (issue `#28`) · **Consumatori**: CP 12.2 (matrice manuale, `#82`) e CP 12.3 (suite automatica, `#83`)
> Le voci manuali confluiscono in [`test-manuali-pie.md`](../technical/test-manuali-pie.md) (sessione E); quelle
> automatiche diventano test dell'Automation Framework.
>
> ## Cosa possiede questo documento, e cosa no
>
> **Possiede**: `requisito → test → tipo di test → criterio di accettazione`. È una mappa di *cosa va
> verificato*, ed è stabile.
>
> **Non possiede lo stato.** Le note «stato attuale: questo non esiste ancora» sparse qui sotto erano vere
> quando furono scritte e sono invecchiate una per una — per esempio «la precedenza di `Charge` non esiste
> ancora: arriva con E4», scritta prima che E4 fosse completata. Lo stato si legge da due posti, e **solo** da
> quelli:
>
> - se un test **esiste**: la suite → comando di misura in [`../README.md`](../README.md);
> - se una feature **è fatta**: [`../roadmap/roadmap-checkpoint.md`](../roadmap/roadmap-checkpoint.md).
>
> Dove sotto trovi ancora una frase di stato, leggila come **contesto storico**, non come verità corrente.

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
| `RefactorTactics.Simulation.DeterministicReplay` → **`RefactorTactics.Replay.Verifier.ResimulationIsDeterministic`** | checksum identico | ✅ TurnLog hash + serializzazione con checksum · ⚠️ **nome divergente dal catalogo**, vedi sotto |

**Protocollo di determinismo** (CP 12.1, `#81`): ogni test esegue la stessa simulazione con **stesso snapshot,
stesso seed, stesse definizioni, stesso ordine**, per almeno **100 ripetizioni**; il checksum finale deve essere
identico.

> ⚠️ **Una divergenza deliberata dal catalogo, dichiarata qui perché non si scopra dal `grep`.**
> Il catalogo (`docs/src/prd/catalogo-e-bilanciamento-v0.1.pdf`, p.24 §15) nomina il decimo test
> `RefactorTactics.Simulation.DeterministicReplay`, e il DoD di CP 12.1 (`#81`) lo chiamava «nome vincolante».
> Dal 2026-08-11 il test si chiama **`RefactorTactics.Replay.Verifier.ResimulationIsDeterministic`**
> ([D-103](../decisions/RT_PDR_00_Decision_Log.md), [#538](https://github.com/DegrassiAaron/refactor-tactics-main/issues/538)).
> Il catalogo non sbagliava nel suo contesto — descrive «100 ripetizioni, checksum identico», cioè una
> ri-simulazione, e usa «replay» nel senso comune di «rigioca» — ma [ADR-0009](../decisions/adr-0009-replay-logico-canonico.md)
> ha reso «replay» una parola riservata: riproduzione di una traccia **senza ricalcolo**. Con quella
> definizione il vecchio nome diceva l'opposto di ciò che il test fa. **La proprietà verificata non è
> cambiata**: cambia il nome, non il protocollo.

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

---

## 8. Copertura da costruire — requisito → test

Aggiunta il **2026-08-08**. Aree decise ma **senza test corrispondenti**: finché la riga è vuota nella colonna
«test», la regola esiste solo nei documenti.

### 8.1 Ambiente e coperture (E8/E9 — regole già in codice)

| Requisito | Tipo | Criterio di accettazione |
|---|---|---|
| Copertura **bassa** riduce il danno diretto | automatico | riduzione applicata sul bordo corretto; **decade** se l'origine è fuori dall'arco frontale ([ADR-0005](../decisions/adr-0005-orientamento.md) §4a) |
| Copertura **alta** chiude il bordo | automatico | lo **stesso** bordo è impercorribile per il path **e** opaco per la LOS: un solo predicato, `BlocksTraversal` |
| Distruzione di struttura | automatico | `CoverDamaged` con integrità residua, poi `CoverDestroyed`; dopo il crollo il bordo si attraversa e si vede |
| **Nessun bonus danno da quota** | automatico | un'unità in alto e una in piano infliggono lo **stesso** danno a parità di tutto il resto ([D-024](../decisions/RT_PDR_00_Decision_Log.md)) |

### 8.2 Decision Boundary e Overwatch (E14 — prima di implementare)

| Requisito | Tipo | Criterio di accettazione |
|---|---|---|
| Nessuna finestra annidata | automatico | una risposta non può aprire una seconda finestra |
| `Timeout → HOLD` | automatico | scaduti i 3,0 s l'esito è `HOLD`, deterministico |
| `FIRE` / `HOLD` | automatico | entrambi gli esiti producono un TurnLog che li registra **come dato** |
| Opportunity **simultanee** nello stesso micro-step | automatico | raccolte in **una sola** opportunity, non N in sequenza |
| Opportunity multiple **nel tempo** | automatico | rispettano `MaxPromptsPerReaction = 3` |
| Risposta **stale** | automatico | una risposta a una finestra già chiusa viene rifiutata, non applicata in ritardo |
| Risposta **non autorizzata** | automatico | solo il proprietario della reaction può rispondere; il server rifiuta gli altri |
| **Ripresa deterministica** | automatico | stessa risposta ⇒ stesso `StateHash`, su N ripetizioni |
| **Nessuna informazione futura** nel DTO | automatico | il payload non contiene trigger non ancora avvenuti |
| **Privacy temporale** ([D-021](../decisions/RT_PDR_00_Decision_Log.md)) | automatico + canary M10 | il DTO avversario non contiene trigger, opportunity, responder o timeout; **e** la resolution osservata da un avversario ha la stessa forma con e senza finestra aperta |
| `Intercept` rivalida sul bersaglio effettivo ([D-017](../decisions/RT_PDR_00_Decision_Log.md)) | automatico | **test discriminante**: A e B a copertura *diversa*; il colpo redirezionato usa la copertura di **B**. Senza la differenza il test passerebbe anche col comportamento sbagliato |

### 8.3 Facing (E16)

| Requisito | Tipo | Criterio di accettazione |
|---|---|---|
| `Dash → Blast` | automatico | il Dash orienta; il Blast su un altro bersaglio **ri**orienta prima di risolvere |
| Cambio di bersaglio | automatico | il facing segue l'ultimo bersaglio dichiarato |
| Cono Overwatch | automatico | il cono della reazione **è** il facing, non una direzione dichiarata a parte |
| `Move` finale | automatico | il `Move` fissa `FacingFinalAfterMove`, che diventa il `FacingStartOfRound` del round dopo |

### 8.4 Conoscenza parziale (E13)

| Requisito | Tipo | Criterio di accettazione |
|---|---|---|
| Tre livelli | automatico | `Nascosto` · `ContattoIncerto` · `Rilevato`, più `UltimoContatto` |
| Unione per squadra | automatico | ciò che vede un alleato entra nella Team Knowledge dell'intera squadra |
| **Rumore** | automatico | propagazione **intera** sul grafo (flood fill), mai `SphereOverlap`; deterministica |
| Il **bot** non bara | automatico | il bot decide sulla Team Knowledge della propria squadra, mai su stato nemico nascosto |

### 8.5 Scenario Harness

| Requisito | Tipo | Criterio di accettazione |
|---|---|---|
| **Nessun bypass** | automatico | ogni scenario passa da `LockInAndResolve` e dal resolver; nessun percorso alternativo |
| `Error` ≠ `Fail` | automatico | uno scenario invalido produce `ERROR`, non `FAIL` |
| Determinismo | automatico | stesso scenario ⇒ stesso `StateHash`, **permutazione-invariante** rispetto all'ordine degli intent |
