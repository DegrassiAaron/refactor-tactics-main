# Scenari di validazione visiva

> **Owner** del corpus di scenari che si aprono **per guardare**, non per far girare un'assertion.
> L'identità e i tag stanno in [`scenario-index-e-tag.md`](../tooling/scenario-index-e-tag.md); come si scrive ed esegue
> uno scenario sta in [`test-e-diagnosi.md`](test-e-diagnosi.md); **chi esegue cosa** — quali verifiche sono
> automatiche e quali richiedono una persona — sta in [`scenario-map.md`](../tooling/scenario-map.md), dove questo corpus
> è la **classe B**. Qui c'è **quali scenari servono, cosa si guarda in ciascuno, e cosa oggi non è guardabile**.
>
> Definito il **2026-08-08**, issue `#231`. Stato al **2026-08-09**: **21 scenari** in `Scenarios/Visual/`,
> **21** voci `PIE-VIS-*` in [`test-manuali-pie.md`](../test-manuali-pie.md) — corrispondenza 1:1, verificata col
> comando di §9. Nessuno ancora eseguito in PIE: i valori numerici vengono dal catalogo e dal codice, e il
> primo run li conferma.
>
> Le feature v0.1 che il corpus **non** può mostrare, e le estensioni di formato che servirebbero, sono
> tracciate in `#233` — il dettaglio tecnico resta in §8.2.

## 1. Il patto

L'oracolo è l'occhio. Nessuno di questi scenari verifica la grafica in automatico: si sceglie lo scenario nel
Details Panel, si preme Play, si guarda. La validazione la fa una persona.

Le assertion restano lo stesso, e non sono un residuo: servono a garantire che **ciò che stai guardando sia lo
stato giusto**. Uno scenario visivo senza assertion può mostrarti un'animazione bellissima di un colpo che ha
mancato, e tu non lo sapresti. Il minimo è `TurnsCompleted` più una `UnitAtCell` o `UnitHpEquals`: se la
logica devia, lo scenario diventa rosso prima che tu perda un pomeriggio a chiederti perché il VFX parte dal
punto sbagliato.

Regola pratica: **un'assertion per ogni cosa che stai guardando**. Guardi un colpo? Verifica gli HP. Guardi
una scivolata? Verifica la cella finale.

## 2. Come si esegue

`ARTGameMode`, sezione `RefactorTactics|Test`:

```
Scenario Filter A   [animation ▼]     ← la lente: "voglio guardare"
Scenario Filter B   [environment ▼]   ← il dominio
Scenario To Run     [...       ▼]     ← solo chi passa entrambi
```

`animation` è un tag che **esiste già** nel vocabolario, e il modello dei tag lo prevede esplicitamente come
lente («lo stesso scenario si apre per verificare una regola *oppure* per guardare un'animazione»). Non serve
un asse nuovo, non serve una categoria: questi scenari sono scenari normali che portano `animation`.

Il secondo tag è il dominio, e sono quelli che il corpus già usa o userà: `movement` · `combat` ·
`environment` · `reactions` · `map` · `phases`.

## 3. Il vincolo che decide tutto: quattro eventi

Il canale che porta la simulazione alla presentazione è `FRTResolvedEvent`, e conosce **quattro** tipi:

```
Move          un'unità ha percorso un path (Path = start + celle attraversate)
Attack        un colpo risolto (Source → Target, Amount = danno)
HazardDamage  danno da terreno
Defeated      rimozione visiva di un'unità eliminata
```

Più i delegate `OnPhaseStarted` / `OnUnitMoveStarted` / `OnUnitDefeated`, dichiarati in `RTTurnManager.h`
proprio per agganciarci VFX/SFX in Blueprint.

Questo divide gli effetti in tre categorie, e la divisione è il contenuto principale di questo documento:

| | Cosa vuol dire | Esempi |
|---|---|---|
| **Agganciabile** | Esiste un evento: un VFX può partire da lì, adesso | passo, colpo, danno da terreno, KO, cambio di fase |
| **Deducibile** | Nessun evento proprio, ma la conseguenza è osservabile | push (l'unità è altrove), fallback (il colpo va sulla cella), cover (il danno è minore) |
| **Muto** | Nessun evento e nessuna conseguenza visibile | `Wet` applicato, `Burning` in corso, reazione armata, cella diventata conduttiva |

Gli effetti **muti** sono il motivo per cui vale la pena scrivere questo catalogo prima dei VFX: oggi non
esiste alcun modo di far partire un particellare quando un'unità diventa bagnata, perché nessuno lo dice.
Si vede il terreno d'acqua — che c'era già prima — e nient'altro. La §8 elenca cosa servirebbe.

## 4. Le fixture sono la tavolozza

`FRTScenarioCell` sa dichiarare `bBlocksMovement`, `bBlocksLineOfSight` e `MoveCost`. **Non** sa dichiarare
una superficie. Uno scenario non può quindi dipingere ghiaccio o fuoco su un'arena generata: sceglie una
fixture che li contiene già e ci porta sopra le unità.

Non è una limitazione da aggirare in fretta. Le superfici stanno in `URTMatchSetupLibrary`, protette da un
test sul layout: copiarle in un JSON creerebbe una seconda geometria che nessuno confronta con la prima.

### `RelayLite` — l'arena di servizio (raggio 5, 91 celle, simmetrica)

È la tavolozza giusta per quasi tutta la fascia A: coppie speculari `(q,r)` / `(-q,-r)`, su spazio
abbondante. ⚠️ **Non contiene però *ogni* superficie**, come questa riga ha dichiarato fino al
2026-08-22 — la tabella qui sotto lo smentiva già da sola: manca **`HighGround`**, che sta solo in
`RelayBasin`. È la ragione per cui la grammatica visiva di `CP 47.3` si giudica là e non qui
([#1267](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1267)).

| Superficie | Celle |
|---|---|
| ShallowWater | `(0,0)` `(0,-1)` `(0,1)` |
| Conductive | `(1,-1)` `(-1,1)` |
| Rough | `(-2,-1)` `(2,1)` |
| Ice | `(-2,2)` `(2,-2)` |
| Fire | `(0,-2)` `(0,2)` |
| Smoke | `(-1,-2)` `(1,2)` |

Tutto il resto è Floor. La riga `r=0` da `(-5,0)` a `(-1,0)` è pavimento pulito: è il corridoio dove mettere
un movimento che non deve incontrare nulla.

### `RelayBasin` — la showcase (45 celle, righe `r=-3..3`)

Serve quando l'elemento da guardare è **di bordo** — e, dal 2026-08-22, anche quando è la **grammatica
visiva delle superfici**: è l'unica fixture con tutte e quattro quelle che `D-183` chiede di
distinguere per forma, `Conductive · Ice · Smoke · HighGround`. ⛔ Per vederla in partita non basta
scriverla nell'asset: serve `rt.Map.Source = LevelAsset`, altrimenti il GameMode la sostituisce con
l'arena di prova all'avvio.

Qui sta solo:

- **copertura bassa** sul lato nord di `(0,0)`;
- **porta chiusa** sul bordo `(0,1)` → `(1,1)`;
- HighGround `(2,-2)` `(3,-1)`; Ice `(-1,2)` `(0,2)` `(1,2)`; Fire `(2,-1)` `(1,-1)`;
  Water `(-3,1)`…`(0,1)`; Conductive `(1,1)` `(2,1)`; Rough `(1,0)` `(2,0)`; Smoke `(-3,0)` `(-2,0)`.

Spawn canonici: Gadget `(-4,0)`, Phase `(-4,1)`, Riktor `(4,0)`, Wraith `(4,1)`.

### `CoverYard` — i bordi, e nient'altro (raggio 3, 37 celle)

Nata il 2026-08-09 (#233) perché la copertura **alta** non compariva in nessuna delle altre tre, e senza una
cella dove trovarla non era possibile scrivere uno scenario su CP 9.2.

| Bordo | Tipo | Cosa fa |
|---|---|---|
| `(0,0)` ↔ `(1,0)` | **alta** | nega vista, passo e proiettili **nei due versi**; integrità 50 |
| `(0,1)` ↔ `(1,1)` | **bassa** | lascia passare tutto, toglie 10 al danno diretto dal lato riparato |

I due bordi stanno sulla **stessa direzione a una riga di distanza**: la differenza fra «riduce» e «nega» si
osserva spostando il bersaglio di una cella, senza cambiare mappa. Tutto il resto è pavimento e non ci sono
superfici — in un cortile che serve a studiare i bordi, un terreno offrirebbe una seconda spiegazione a ogni
esito.

**Perché non nel Relay Basin**: è la mappa autorata degli 8 turni, protetta da `BasinLayoutMatchesSpec`.
Aggiungerle una barriera per comodità di test avrebbe cambiato la showcase per una ragione che con la
showcase non c'entra.

### `TestArena` — la geometria (raggio 4)

L'unica con **due livelli**: piattaforma su layer 1 in `(2,-1,1)` `(2,0,1)` `(3,-1,1)` `(3,0,1)`, raggiungibile
da **una sola** transizione `(1,0,0)` → `(2,0,1)`. Più il muro che blocca la vista su `q=0`, `r=-2..2`, gli
ostacoli `(-1,2)` `(1,-2)` `(2,1)` e la fascia Rough a costo 3 su `q=-2`.

## 5. Numeri del roster (verificati nel catalogo, non nei PDF)

| Eroe | HP | MP | Attacco base | Note |
|---|---:|---:|---|---|
| Gadget | 90 | 5 | `Hero.Gadget.ArcPulse` 22, r4 | `LinearDischarge` 24 r5 linea cd2 · `Overload` 18 AoE r1 |
| Phase | 95 | 5 | `Hero.Phase.PressureJet` 16 + Wet(1) + Push 1, r5 linea | fallback `AttackCell` |
| Riktor | 120 | 4 | `Hero.Riktor.ImpactShot` 8 + Slow(1), r3 | PushResistance 1 · `Ram` = carica 20 + Push 1 |
| Wraith | 90 | 6 | `Hero.Wraith.PulseShot` 21, r4 | il più mobile |

I danni da terreno vengono dal catalogo terreni (Fire: 10 + `Burning`) e vanno confermati al primo run.

## 6. Il catalogo

Colonna **Oggi**: `PASS` gira e si guarda · `MUTO` gira ma l'effetto non ha un evento a cui agganciarsi ·
`FORMATO` il gioco lo sa fare, lo scenario non sa dirlo · `BLOCKED` capability assente, esce col nome.

### Fascia A — banco VFX: effetti con un evento già disponibile

Questi sono i primi sei da scrivere. Ognuno isola **un** evento, così un VFX nuovo si giudica senza rumore.

Due dei sei erano già nel corpus. **Scritti** il 2026-08-08: i quattro nuovi, e sono i primi scenari del
progetto a usare una fixture invece di un'arena generata.

| ID | Fixture | Allestimento | Cosa guardi | Assertion | Stato |
|---|---|---|---|---|---|
| `Movement.LongWalk` *(esiste)* | r5 | due unità attraversano l'arena, 3 celle per turno × 2 | passo, orientamento, velocità, camera che segue | *(già sue)* | già `animation` |
| `Combat.BasicAttack` *(esiste)* | r4 | Gadget `ArcPulse` su Riktor a distanza 2 | partenza, volo, impatto, numero di danno | `UnitHpEquals B1 103` — 120 − (22 − 5 di `BaseShield`, D-224) | già `animation` |
| `Visual.Environment.FireOnEnter` | RelayLite | Wraith `(0,-3)` → move `(0,-2)` Fire | **due** momenti: 10 danni all'ingresso, 8 nel Cleanup per `Burning` | `UnitHpEquals V1 72` (i due danni della cella accanto: il danno da terreno e' `Environmental`, e il `BaseShield` non lo ferma — D-224) · `UnitAtCell V1 (0,-2,0)` | scritto |
| `Visual.Combat.Defeat` | r4 | Gadget `(-1,0)`; Riktor `ImpactShot` + Wraith `PulseShot` per **sei turni** | l'unità che incassa, poi sparisce: KO al **sesto** (ADR-0007 lo aveva già portato da due a quattro; D-224 da quattro a sei, e il danno per turno **non** è costante — `Hero.Gadget.ReactiveCapacitor` ha cooldown 3) | `UnitAlive F1 false` · `UnitHpEquals R1 95` · `TurnsCompleted 6` | scritto |
| `Visual.Movement.Charge` | r4 | Riktor `(3,0)` usa `Ram` su Gadget `(1,0)` (distanza 2, portata 3) | la carica **si legge diversa** dal passo: accelerazione, impatto, arresto addosso | `UnitHpEquals F1 75` — 90 − (20 − 5 di `BaseShield`, D-224) | scritto |
| `Visual.Environment.IceSlide` | RelayLite | Gadget `(-2,4)` → move `(-2,3)` `(-2,2)` Ice, restano 3 MP | il passo extra deve leggersi come **scivolata**, non come un passo in più | `UnitAtCell F1 (-2,1,0)` | scritto |

Tre cose emerse verificando i numeri prima di scrivere i file, e nessuna era ovvia:

- **`Burning` costa 8 danni nel Cleanup**, per due turni. Wraith non finisce a 90 ma a **72**, e lo scenario
  del fuoco ha quindi *due* momenti da guardare invece di uno. Un VFX che li copre con la stessa animazione
  sta nascondendo una regola.
- **`Action.Charge` ha portata 3** (catalogo azioni), ereditata da `Ram`: la distanza 2 dell'allestimento
  sta dentro con un margine che sopravvive a un ritocco di bilanciamento.
- **Lo scivolamento richiede budget residuo ≥ 2**, e prosegue *nella direzione dell'ultimo passo*. Gadget ha
  5 MP, il percorso ne costa 2, ne restano 3: scivola. Spostare la cella di partenza spegne l'effetto senza
  che niente sembri rotto.

Resta un'incognita dichiarata: in `Visual.Movement.Charge` le **celle finali** dipendono dall'ordine fra
arresto e spinta. Non sono assertion — stanno in `_nota_celle`, e il primo run le promuove. Scriverle a
indovinare produrrebbe un rosso che accusa il gioco di un errore proprio.

Quello scenario porta anche il **push**: Gadget finisce spinto di una cella senza che nessun evento lo dica. Se
la spinta non si legge, non è un difetto del VFX — è la §8.1.

### Fascia B — leggibilità: si capisce cosa è successo?

| ID | Fixture | Cosa guardi | Stato |
|---|---|---|---|
| `Visual.Combat.WaterElectric` | r5 | Wraith scatta nella spina d'acqua in fase **Dash**, il terreno lo bagna, e la scarica di Gadget nel Blast trova un bersaglio già bagnato: **due regole in un colpo** — la combo firma e l'ordine fra le fasi. `90 − (32 − 5 di BaseShield) = 63`. ⚠️ Phase **non** è in questo scenario: la variante coordinata fra due eroi è `Visual.Combat.WaterElectricCoordinated`, nella tabella delle sorgenti del bagnato | scritto |
| `Visual.Combat.FallbackTargetMoved` | r5 | Riktor lascia la cella nel Dash, la scarica arriva sulla cella **vuota**: il piano è rivalidato, non annullato | già nel corpus |
| `Visual.Core.PhaseOrder` | r4 | tre azioni in tre fasi separate — carica, colpo, camminata: `Dash → Blast → Move` deve **vedersi** | scritto |
| `Visual.Combat.PushResistance` | r4 | la stessa spinta su Riktor (assorbita, PushResist 1) e su Wraith (subìta): una statistica invisibile diventa visibile | scritto |
| `Visual.Combat.SmokeCapsTargeting` | RelayLite | il bersaglio **si vede** e non si può colpire: il fumo accorcia, non acceca | scritto |
| `Visual.Movement.RoughRefusesCharge` | RelayLite | il rifiuto in **pianificazione**: a schermo non deve accadere niente | scritto |
| `Visual.Reaction.Interposition` | r4 | il proiettile **cambia destinatario** a mezz'aria: Riktor incassa al posto di Wraith. Il caso più difficile del corpus — se non si vede, si legge «Gadget ha sbagliato mira» | scritto |
| `Visual.Reaction.Deflection` | r4 | l'opposto: il colpo arriva dove doveva e **non fa piu' niente**: 22 diventano 2, e lo scudo base ([D-224]) assorbe anche quei 2 — Wraith resta a **90 pieni**, la barra non si muove. Se la parata non si vede, si legge un attacco debole invece di una difesa riuscita | scritto |
| `Visual.Combat.GuardReducesFirstHit` | r4 | `Guard` porta un **pool** di 15 assorbibili, e qui il primo colpo da 22 lo esaurisce da solo: 120 → 97 (il `BaseShield` di D-224 ne assorbe altri 5). 🔴 *La cella diceva «toglie 15 al primo colpo e finisce lì», la regola che [D-292] ha superato il 2026-08-31: il numero non cambia — con un colpo sopra la soglia le due regole coincidono — ma la descrizione sì* | scritto |
| `Visual.Combat.BraceReducesEveryHit` | r4 | `Brace` toglie 10 a **ogni** colpo e non finisce mai: 120 → 102 (il `BaseShield` di D-224 ne assorbe altri 5) | scritto |
| `Visual.Combat.GuardVsBraceUnderSmallHits` | r5 | **il confronto affiancato, sotto colpi PICCOLI**: tre difensori identici, tre `ImpactShot` da 8 a testa, tre risposte. Il `Brace` resta **illeso** (120), la Guardia perde 4 (116), chi non si difende perde 19 (101) — il capovolgimento rispetto alle due righe qui sopra, dove con colpi grandi è la Guardia a dominare. Voce ombrello `PIE-ACC-GUARDBRACE`, seduta **U42** | scritto, `PASS 7/7` il 2026-09-03 |
| `Visual.Combat.AreaGuardFromImpactCenter` | r5 | due difensori **identici** in `Guard`, entrambi rivolti a ovest, presi dalla stessa area: la Guardia legge il **centro dell'esplosione** e non chi l'ha lanciata, quindi uno è coperto e l'altro no. Il contrasto è tutto ciò che si guarda (`COV-12`) | scritto, voce `PIE-VIS-AREAGUARD` |
| `Movement.Collision` *(esiste)* | r3 | chi cede la cella contesa, e che si capisca **perché** | già nel corpus |
| `Combat.CounterStrikesBack` *(esiste)* | r4 | la terza grammatica difensiva: lo scudo assorbe **e** restituisce danno | già nel corpus |

`Visual.Reaction.Interposition` è il caso più istruttivo del catalogo. La capability `Reaction` è
**disponibile** — `Hero.Riktor.Interposition` è cablata e automatica. Ma `FRTScenarioIntent` ha `UnitId`, `Move`,
`Ability`, `Target` e nient'altro: **non esiste un campo per armare la reazione pianificata**. Il gioco lo sa
fare, lo scenario non lo sa dire. Vedi §8.2.

### Fascia C — specchio: la grafica dice il vero?

| ID | Fixture | Cosa guardi | Stato |
|---|---|---|---|
| `Visual.Environment.WetExtinguishesFire` | RelayLite | CP 8.4: l'acqua spegne le fiamme. Ciò che si guarda è un'**assenza** — gli 8 danni del Cleanup che non arrivano. **61**, non 53 | scritto |
| `Visual.Map.LowCoverEdge` | RelayBasin | due colpi simultanei sullo stesso bersaglio, **entità diverse**: la copertura è di un bordo. `90 − (11 + 8 − 5 di BaseShield) = 76`. Da ADR-0007 è **Wraith** a tirare dal lato riparato: con ImpactShot a 8 contro una riduzione di 10 il danno si troncava a zero e lo scenario smetteva di misurare la grandezza della copertura | scritto |
| `Visual.Map.ClosedDoor` | RelayBasin | Phase arriva **girando**: la porta è un bordo, e il percorso deve raccontare da sé perché è lungo | scritto |
| `Visual.Map.HighGroundNoBonus` | RelayBasin | due Wraith identici, dalla cresta e dal piano: 21+21, nessun bonus (D-024) | scritto |
| `Visual.Map.HighCoverBlocks` | **CoverYard** | la barriera **alta** nega vista *e* passo in un turno solo: il colpo non parte e il percorso gira | scritto |
| `Visual.Map.MultiLevel` | TestArena | la salita attraverso l'unica transizione: i layer non hanno adiacenza implicita | scritto |
| `Visual.Map.SightWallIsWalkable` | TestArena | il banco di `PIE-HEXPLAY-6`: la colonna `q=0` blocca la **vista** ma non il **passo**, e la domanda è una sola — *il muro si vede?* La doppia natura è ciò che un giocatore non può dedurre da solo | scritto |
| `Visual.Map.TwoLayersSameColumn` | TestArena | due unità sulla **stessa colonna** e su piani diversi: l'unica configurazione in cui un viewport che perde il layer si smaschera. Non verifica una regola — costruisce la scena che rende visibile un difetto di presentazione | scritto |
| `Combat.BlockedByWall` *(esiste)* | r4 | il muro ferma la **vista**, non il passaggio | già nel corpus |
| `Visual.Water.Wet` | RelayLite | *niente*: `Wet` non emette nulla, si vede solo il terreno che c'era già | **MUTO** — non scritto |
| `Visual.Conductive.Network` | RelayLite | *niente*: la rete conduttiva è un dato senza evento e senza consumatore | **MUTO** — non scritto |

I due `MUTO` restano deliberatamente **non scritti**: uno scenario che si apre e non mostra niente di diverso
da prima insegna a diffidare del corpus. Tornano quando esiste `StatusChanged` (§8.1).

Le tre avvertenze della fascia C, per non scoprirle scrivendo i file:

- `Visual.Cover.LowEdge` — l'entità della riduzione (la spec CP 9.1 dice 10 su danno diretto dal lato
  protetto) non è stata verificata nel codice mentre si scriveva. Il valore dell'assertion lo fissa il primo
  run: quello che questo scenario deve dimostrare è che i **due colpi differiscono**, non di quanto.
- `Visual.Map.ClosedDoor` — che i 5 MP di Phase bastino a girare intorno alla porta passando da Rough dipende
  dal costo di quella cella. Se non bastano, il percorso alternativo va accorciato: lo scenario deve mostrare
  una deviazione, non un fallimento di budget.
- `Visual.HighGround.NoBonus` — usa **due unità con lo stesso `HeroId`**, che il formato consente (l'`Id` è
  locale allo scenario, `HeroId` è separato). È la forma più pulita per un confronto a parità di azione, ma
  è la prima volta che il corpus la userebbe.

### Fascia D — dichiarati oggi, accesi domani

> ⚠️ **Questa fascia non è mai atterrata come file, ed è stata riscritta il 2026-08-09.** Diceva «scritti
> adesso con `requires`»: **nessuno degli otto file esiste**, verificato con
> `grep -l '"requires"' Scenarios/Visual/**/*.json` — nel corpus visivo l'unico `requires` è `Reaction`, che è
> disponibile. Quattro dei temi sono poi stati scritti come **`Spec.*`** (§6-bis), che è la forma migliore: una
> specifica eseguibile invece di una vetrina cieca, perché la domanda «la regola è questa?» vale prima della
> domanda «si vede?». I restanti quattro non esistono in nessuna forma.
>
> La colonna **Oggi** dice ora cosa c'è davvero. La tabella resta perché l'intenzione resta valida — quando
> una capability atterra, la sua vetrina va scritta — ma **non promette più file che non ci sono**.

L'idea: uno scenario scritto adesso con `requires` esce `Blocked` col nome della capability mancante, il
catalogo diventa la lista viva di cosa manca, e ogni feature che atterra accende la sua vetrina invece di
richiedere che qualcuno si ricordi di scriverla.

| ID | `requires` | Cosa mostrerà | Origine | **Oggi** |
|---|---|---|---|---|
| `Visual.Overwatch.HoldThenFire` | `DecisionBoundary` | la finestra live: HOLD scarta l'opportunità, FIRE la consuma e tronca il movimento | E14 · ADR-0004 | **coperto da `Spec.Overwatch.HoldThenFire`** |
| `Visual.Predictive.Whiff` | `PredictiveAction` | il valore del turno **è** il colpo che manca: si è sparato a una previsione | showcase T2 | **coperto da `Spec.Predictive.WhiffOnEmptyCell`** |
| `Visual.Objective.Relay` | `Objective` | il punto assegnato nel Cleanup, **dopo** ambiente e KO | E10 · `#75` | **coperto da `Spec.Objective.PointSurvivesKO`** |
| `Visual.Perception.Noise` | `Perception` | il rumore come **seconda fonte di informazione**, non come debuff | non in scope v0.1 | **coperto da `Spec.Perception.HeardNotSeen`** |
| `Visual.Facing.Cone` | `Facing` | il cono di controllo, e cosa ci entra | E16 · `#175` | ⏳ **non scritto** |
| `Visual.Intercept.Revalidation` | `InterceptRevalidation` | la geometria rivalidata sul bersaglio effettivo | D-017 · `#200` | ⏳ **non scritto** |
| `Visual.CoverWindow.OpenFireSeal` | `CoverWindow` | apro → sparo → richiudo, tre unità della stessa squadra | **v0.2** · E22 | ⏳ **non scritto** |
| `Visual.Interaction.DoorGraph` | `Interaction` | la porta come oggetto logico: apertura, revisione del grafo, path che cambia | **v0.2** · E23 | ⏳ **non scritto** |

Le prime quattro **non vanno riscritte come `Visual.*`**: sarebbero un secondo file sullo stesso soggetto, con
le stesse assertion, che esce `BLOCKED` per la stessa ragione. Quando la capability atterra, lo `Spec.*`
diventa verde e *allora* ha senso una vetrina — se il tema ha qualcosa da **mostrare** oltre che da affermare.
`Visual.Facing.Cone` corregge anche un'attribuzione: il cono è **E16**, non E14 (E14 lo *consuma*).

Le capability nuove (`CoverWindow`, `Interaction`, `Perception`) non vanno aggiunte a `IsCapabilityAvailable`
finché il sistema non esiste: l'elenco sta nel codice apposta, perché dichiarare disponibile una capability
inesistente non deve essere una modifica al JSON.

### Fascia E — banchi e compositi: scenari che non mostrano un evento

Le quattro fasce sopra descrivono **cosa accade** e chiedono se si veda. Questi quattro no: o non fanno
accadere niente, o fanno accadere troppo perché un sì/no unico abbia senso. Restano scenari di classe **B** —
l'oracolo è l'occhio — ma la domanda che pongono è diversa, e collassarli nelle fasce esistenti avrebbe
richiesto di inventare per ciascuno un «evento da guardare» che non hanno.

| ID | Fixture | Cosa guardi | Stato |
|---|---|---|---|
| `Visual.Input.PcGym` | TestArena | **allestisce e non gioca**: `turns` è vuoto, la partita resta al round 1 in pianificazione e il controllo è del giocatore. Il soggetto della seduta U37 è il **gesto**, e una partita che avanza da sola glielo toglierebbe | scritto |
| `Visual.Map.GrayKitYard` | GrayKitYard | la **scena di posa** delle sedute U25 e U35: `CoverYard` con tre aggiunte, e la base identica è un requisito — `PIE-GBX-FIT` si decide provando valori di inset, e due letture su scene diverse non sono confrontabili | scritto |
| `Visual.Perception.Acceptance` | VisionSplit | **composito 1↔N**: i tre stati del velo — osservato, ricordato, mai visto — in una scena sola. Voce ombrello `PIE-ACC-PERCEPTION`; i criteri restano nelle cinque `PIE-KNOW*` e in `PIE-VELO-VIEWER` | scritto |
| `Visual.Hud.FirstPlayable` | TestArena | **composito 1↔N**: la partita in uno stato **ricco** — roster pieno, due squadre ferite, uno slot in cooldown, un combat log popolato — perché una persona giudichi se l'HUD lo racconta. Voce ombrello `PIE-ACC-HUD` | scritto |

> 🔴 **Perché questa fascia esiste: i quattro erano nel corpus e in nessun catalogo.** Misurato il
> 2026-09-03 incrociando ogni `scenarioId` sotto `Scenarios/Visual/` con questo documento — sette assenti, di
> cui tre erano scenari-verifica classici e ora stanno nelle fasce B e C. È la stessa classe di buco che il
> registro PIE ha dichiarato «la peggiore» il 2026-09-02 per `Visual.Combat.AreaGuardFromImpactCenter`: un
> file che gira, passa, e che nessun documento nomina. Nessuno lo rivede quando la scena cambia, e chi cerca
> una scena adatta non lo trova.
>
> ⚠️ **`Visual.Map.GrayKitYard` era dichiarato «l'unica assenza legittima» perché è una fixture, non uno
> scenario-verifica.** La distinzione regge — non mostra un evento — ma non giustificava l'assenza: è
> comunque un file che una persona apre in seduta, e la sua riga qui dice *quale* seduta e *perché* la base
> dev'essere identica a `CoverYard`. Una fixture senza catalogo è una scena che si può cambiare senza
> accorgersi di chi la stava usando.
>
> Il comando che li trova, da rieseguire quando nasce uno scenario visivo:
>
> ```bash
> for s in $(grep -rhoE '"scenarioId": "Visual\.[A-Za-z0-9.]+"' Scenarios/Visual >            | grep -oE 'Visual\.[A-Za-z0-9.]+' | sort -u); do
>   grep -q "$s" docs/technical/runbooks/scenari-validazione-visiva.md || echo "ASSENTE dal catalogo: $s"
> done
> # deve stampare NULLA
> ```

## 6-bis. Il corpus come test automatico

### Uno scenario non è un test finché qualcuno non lo esegue

`Scenario.ShippedScenariosAreValid` **carica** ogni scenario versionato e verifica che l'ID risolva al suo
file. Non lo esegue. Uno scenario diventava un test solo se gli si scriveva accanto la propria
`IMPLEMENT_SIMPLE_AUTOMATION_TEST` — `RunnerCombatBasicAttack`, `RunnerCounterStrikesBack`, una per file.

È una convenzione che si dimentica: i diciassette scenari visivi sono stati committati senza, e per un commit
intero sono sembrati coperti senza esserlo. Il verde di `ShippedScenariosAreValid` diceva soltanto che il JSON
era ben formato.

**`Scenario.EveryShippedScenarioRuns`** chiude il buco alla radice: scopre il corpus dall'indice e lo esegue
tutto. Aggiungere un file basta perché venga eseguito — non c'è più un secondo passo da ricordare.

| Esito | Trattamento | Perché |
|---|---|---|
| `PASS` | verde | ha giocato tutti i turni e le assertion tengono |
| `BLOCKED` | **verde**, col motivo nel log | è il meccanismo che permette di versionare uno scenario prima dei suoi sistemi. Trattarlo come rosso renderebbe irrazionale scriverne in anticipo |
| `FAIL` | rosso — difetto del **gioco** | riporta il primo assert caduto col valore reale |
| `ERROR` | rosso — difetto dello **scenario** | la distinzione è già nel tipo di esito, e va conservata invece di appiattirla su «non passa» |

Gli scenari con tag `expected-fail` sono esclusi e verificati al contrario da
**`Scenario.ExpectedFailScenariosReallyFail`**: devono fallire *davvero*, e con `FAIL`, non `ERROR`. Se il
gioco cambiasse in modo da farli passare, l'unica prova che l'harness sa dire «rosso» smetterebbe di provarlo
senza che nulla diventi rosso. È il caso in cui il verde è il difetto.

### Scenari-specifica: scritti prima della feature

`Scenarios/Spec/` contiene scenari che descrivono una feature **che non esiste**. Dichiarano la capability in
`requires`, escono `BLOCKED` nominandola, e si accendono da soli quando atterra.

| ID | `requires` | La domanda che pone |
|---|---|---|
| `Spec.Overwatch.HoldThenFire` | `DecisionBoundary` `Facing` | HOLD scarta senza consumare la carica, FIRE tronca il movimento |
| `Spec.Objective.PointSurvivesKO` | `Objective` | il punto è assegnato **dopo** ambiente e KO, non prima |
| `Spec.Predictive.WhiffOnEmptyCell` | `PredictiveAction` | il colpo su una previsione sbagliata deve **mancare** |
| `Spec.Perception.HeardNotSeen` | `Perception` | la squadra riceve un'**area**, mai la cella esatta |
| `Spec.Cover.TemporaryCoverExpires` | `CreateCover` | una copertura temporanea **scade** — il terzo momento, quello che si dimentica |
| `Spec.Environment.ElectricPropagation` | `EnvironmentalActionOwner` | la scarica segue l'**acqua**, non il raggio: 20 alla sorgente, 12 ai propagati, chi è all'asciutto resta illeso |
| `Spec.Environment.WaterQuenchesFire` | `EnvironmentalActionOwner` | creare acqua su una cella che brucia **spegne il terreno** — non solo il `Burning` di un'unità |
| `Spec.Map.BridgeBreaksThePath` | `EnvironmentalActionOwner` | un arco è **additivo**: romperlo non allunga il percorso, lo **annulla** |

Il valore non è la copertura: è che queste domande vengono poste **prima**. `WhiffOnEmptyCell` è l'esempio
netto — a implementazione finita si testa che il colpo arrivi quando la previsione è giusta, e
un'implementazione che rivaluta il bersaglio al momento dello sparo passerebbe quel test e fallirebbe questo.
Scritto dopo, quel caso non verrebbe in mente.

Ognuno dichiara in `_nota_da_completare` cosa manca per renderlo verde, incluse **due assertion che non
esistono**: `TeamScoreEquals` per gli obiettivi, e un modo di asserire sulla conoscenza di una squadra per la
percezione. Meglio scoprirlo ora che a implementazione finita.

### Una capability che non nomina una feature mancante

`EnvironmentalActionOwner` è diversa dalle altre cinque, e la differenza è il punto: **il sistema esiste ed è
chiuso**. CP 8.3, CP 8.5 e CP 9.4 sono verdi, `Action.Electrify` sta nel catalogo con `PropagationLimit = 3`,
e il `TurnManager` la risolve leggendo `PlannedAbilityIndex`. Quello che manca è **chi la possiede**: nessuna
unità ha quelle azioni nel kit.

È lo stesso difetto che #275 ha chiuso per `Guard` e `Brace`, con una differenza che vieta la stessa
soluzione: quelle erano azioni **generiche** per D-025 e potevano entrare nel kit di tutti. Un'azione
ambientale in mano a ogni eroe sarebbe una decisione di design — «chiunque può incendiare e creare acqua» —
non un cablaggio.

La via canonica è un'altra, e il progetto l'ha già usata per le reazioni: `Hero.Gadget.ConductiveNode`,
`Hero.Phase.MistVeil` e `Hero.Phase.FluidTrail` **esistono nel catalogo eroi con `Effects` vuoti**, e i loro commenti
dichiarano il perché — quando furono scritte, il sistema d'ambiente non c'era. Ora c'è. Cablarle alla
semantica core conservando l'identità dell'eroe è la stessa mossa di `Hero.Riktor.Ram` → `Action.Charge`.

Ogni turno di uno scenario-spec resta con `intents` **vuoti**: la sintassi con cui si dichiarerà una risposta
HOLD/FIRE, o un bersaglio predittivo, non è decisa. Inventarla qui creerebbe un formato che nessuno ha scelto
e che il primo implementatore dovrebbe disfare.

### Cosa ha trovato la prima esecuzione

Il corpus è stato eseguito per la prima volta il 2026-08-08. Build `RefactorTacticsEditor` **Succeeded** al
primo colpo; **cinque scenari su diciassette erano rossi**, e la ripartizione delle cause è la ragione per cui
questo test valeva la pena.

Stato finale, misurato **quel giorno** — è una fotografia, non un valore corrente:

```
corpus eseguito: 28 PASS, 6 BLOCKED, 2 dichiarati expected-fail   (36 scenari, 2026-08-08)
RefactorTactics.Scenario.EveryShippedScenarioRuns            Success
RefactorTactics.Scenario.ExpectedFailScenariosReallyFail     Success
```

> Il corpus è cresciuto a **49** scenari (`find Scenarios -name '*.json' ! -name '_*' | wc -l`, 2026-08-09):
> la ripartizione PASS/BLOCKED va rimisurata eseguendo la suite, non dedotta da questa riga.

**Tre erano difetti miei, con una causa sola.** Lo scatto si dichiara con `dash` + `dashTo`, non con
`ability`: dopo [D-028] occupano slot diversi — lo scatto prende il movimento, l'abilità la principale, e
«schivo e sparo» dev'essere esprimibile. Dichiarato con `ability`, `Hero.Riktor.Ram` finiva nello slot del Blast
e non partiva. Da qui `Charge` (Gadget illeso), `PhaseOrder` (mancavano i 20 della carica) e
`FallbackTargetMoved` (il bersaglio non si spostava).

Il quarto scenario con lo stesso errore, `RoughRefusesCharge`, **era verde**. Si aspettava che non accadesse
nulla, e infatti non accadeva nulla — ma perché la carica non partiva, non perché il Rough la vietasse. Un
verde che tace è peggio di un rosso: senza gli altri quattro rossi a indicare la causa comune, sarebbe
rimasto lì a dare una falsa sicurezza.

**Uno era una mia assunzione sbagliata sulla regola.** `FallbackTargetMoved` faceva uscire Riktor
dall'allineamento restando a quattro celle, e il colpo lo raggiungeva lo stesso. La forma `Line` descrive
*chi altro* viene preso sulla traiettoria, non un vincolo di allineamento del bersaglio: il fallback scatta
sulla **portata**. Lo scenario ora fa caricare Riktor in direzione opposta, fino a sette celle da Gadget.

**Due erano difetti del gioco**, e nessun test esistente li vedeva:

| Difetto | Perché nessuno se n'era accorto |
|---|---|
| `PushResistance` non riduce le spinte ([#241], **chiuso**) | era un **dato senza consumatore**: catalogo → `ARTUnit` → test che ne verificano il *valore*. Nessuno lo leggeva quando applicava una spinta |
| la combo Phase→Gadget non è realizzabile ([#242]) | `Heroes.Hero.Gadget.WetBonus` verifica l'**aritmetica** di `EffectiveAttackPower` senza passare dal `TurnManager`. Il `Wet` di `PressureJet` arriva *durante* il Blast, quando i colpi sono già preparati — e su due turni scade nel Cleanup prima di servire |

Il secondo è il più istruttivo del lotto: la combo firma della v0.1 era documentata, aveva un test verde, ed
era **ineseguibile**.

**Corretto lo stesso giorno** ([D-036], #242): `Wet` ha ricevuto la disciplina che `Status.Marked` aveva già
dal CP 8.2 — ciò che nasce dentro il Blast vale per i colpi a priorità più alta, con l'ordine canonico di
ADR-0003 §3. Nessuna durata è stata toccata: la coordinazione è una questione di **ordine**, non di quanto
dura il bagnato.

Le due sorgenti di `Wet` hanno ora uno scenario ciascuna, e servono entrambe:

| Scenario | Sorgente del bagnato | Cosa dimostra |
|---|---|---|
| `Visual.Combat.WaterElectricCoordinated` | `Hero.Phase.PressureJet`, priorità 50 | la **coordinazione fra due eroi** dentro lo stesso Blast: `90 − (16 + 32 − 5 di BaseShield) = 47` — Wraith ha 90 HP, non 100 |
| `Visual.Combat.WaterElectric` | il **terreno**, attraversato in fase Dash | che il bonus non dipende da chi bagna (D-029): `90 − (32 − 5 di BaseShield) = 63` — Wraith ha 90 HP, non 100 |

### Lacune dichiarate

Tre abilità del kit non hanno scenario, e non per dimenticanza:

| Abilità | Perché no |
|---|---|
| `Hero.Phase.CircularTide` | il routing cura-agli-alleati / `Wet`-ai-nemici è dichiarato **incompleto** nel catalogo. Un'assertion scritta sul design invece che sul comportamento reale produrrebbe un `FAIL` che accusa il gioco di un difetto già noto |
| `Hero.Phase.FluidTrail` | la mobilità è rappresentabile, la scia d'acqua no: resterebbe uno scenario che verifica un Dash e lo chiama `FluidTrail` |
| `Hero.Wraith.Feint` | nessuna delle due metà è dichiarabile — `Status` si applica alle unità e non alle celle, e il `Reposition` passa da `MovementStyle`, non da `Effects` |

Vanno scritte quando il comportamento è **osservabile**, misurando il primo run invece di derivarlo dal
catalogo. Le prime due sono il caso migliore per un test di **caratterizzazione**, non per una specifica.

## 7. Convenzioni

```
scenarioId   Visual.<Dominio>.<Cosa>        prefisso Visual = si apre per guardare
tags         ["animation", "<dominio>", "<eroe>"]
percorso     Scenarios/Visual/<Dominio>/<Cosa>.json
```

Il prefisso `Visual.` non è una categoria — l'indice non ne ha — ma rende l'intento leggibile nella tendina
anche senza filtri attivi. Il tag `animation` resta il meccanismo vero.

Le cartelle sono storage e non promettono nulla: uno scenario può stare altrove e restare trovabile. Metterli
insieme serve solo a chi legge un `git diff`.

## 8. Cosa manca, in ordine di resa

### 8.1 Eventi di playback per gli effetti muti

Senza questi, tre scenari del catalogo sono vetrine cieche. La proposta minima, da introdurre **una alla
volta insieme alla feature che la usa**, mai tutte insieme:

| Evento | Sblocca | Costo |
|---|---|---|
| `Push` | la spinta si legge come spinta, non come teletrasporto | basso: il dato c'è già nella risoluzione |
| `StatusChanged` | Wet, Burning, Shield, e ogni stato futuro di CP 8.2 | medio: serve decidere cosa entra nel DTO |
| `EnvironmentChanged` | celle che cambiano superficie, porte, coperture | medio: dipende da chi muta l'ambiente |
| `ReactionResolved` | la reazione che scatta ha un momento suo | basso, ma ha senso solo dopo §8.2 |

`FRTResolvedEvent` ha già `Amount` e i due riferimenti a unità: per `Push` e `StatusChanged` la struttura
regge senza cambiamenti di forma. Aggiungere un valore all'enum **in coda** non rinumera i precedenti.

### 8.2 Cosa il formato scenario non sa dire

`FRTScenarioSession::BeginTurn` scrive esattamente due cose sull'unità: `PlannedAbilityIndex`, cercato **nel
kit dell'eroe**, e `PlannedAttackTarget`, che dev'essere **un'unità viva**. Da qui segue tutto il resto.

| Non esprimibile | Perché | Feature v0.1 che resta fuori |
|---|---|---|
| Azioni **core diverse dalle tre generiche** | il kit di un'unità è *eroe + `Wait`/`Guard`/`Brace`*: `Electrify`, `Ignite`, `CreateWater`, `ModifyArc`, `Push`, `MarkTarget`, `HeavyAttack` non appartengono a nessuno | **CP 8.3** propagazione elettrica, **CP 8.5** terreno dinamico, **CP 9.4** ponti, e l'**abbattimento** di una copertura (`DamageStructure` è solo su `HeavyAttack`) |

> ✅ **Tre buchi chiusi il 2026-08-09.** Le azioni **con bersaglio-cella** erano già esprimibili (`targetCell`
> esisteva, e l'avevo dichiarato mancante verificando contro una copia vecchia). Le azioni **generiche** —
> `Wait`, `Guard`, `Brace` — ora appartengono a ogni unità: non è stato l'harness a cambiare, è stato il gioco
> (#275). E `surface` **non serviva**: la copertura alta mancava perché nessuna fixture ne conteneva una, non
> perché il formato non sapesse dirla — ora c'è `CoverYard` (#233).
>
> Le restanti azioni core restano fuori **perché nessuna unità le possiede**, che è un fatto di gameplay:
> darle allo scenario e non al giocatore produrrebbe test verdi su regole inesistenti.

> ✅ **Il quarto buco si è chiuso da solo.** Questo documento nasceva dichiarando che le reazioni non erano
> esprimibili — e su una working copy indietro di qualche commit era vero. Su `origin/main` il campo
> `reaction` **esiste già** nell'intent, viene letto dal loader e **validato** (dev'essere nel kit dell'eroe
> e occupare lo slot `Reaction`). Da lì nascono `Visual.Reaction.Interposition` e
> `Visual.Reaction.Deflection`. La lezione è di metodo: un limite va verificato contro il **ramo condiviso**,
> non contro la copia che si ha sotto mano — altrimenti si documenta come mancante ciò che qualcun altro ha
> appena costruito.

Restano tre buchi, in ordine di resa:

1. **un `target` che accetti una cella** — `{"targetCell": [q,r,l]}` accanto a `target`. Sblocca l'ambiente
   e le strutture, cioè la parte più spettacolare della v0.1: la scarica che si propaga sull'acqua;
2. **azioni core nel lookup** — oggi si cerca solo in `Hero->Actions`. Sblocca le sette azioni generiche di
   D-025 e gli stati che nessun kit d'eroe applica;
3. **`surface` in `FRTScenarioCell`** — con cautela: per le geometrie canoniche la fixture è la scelta
   *giusta*, e un campo `surface` va usato solo per ciò che nessuna fixture serve. Vale per la copertura
   alta, non per rifare l'acqua.

Finché restano aperti, «tutte le feature della v0.1» non è un traguardo raggiungibile dal corpus: la parte
mancante non è di scenari da scrivere, è di **formato da estendere**.

### 8.3 Non serve

- **Regia nel dato** (camera, pause, loop): deciso il 2026-08-08 di non introdurla. Camera libera.
- **Replay come artefatto**: «replay» qui significa *rigiocare in Visual*, che l'harness fa già.
- **Validazione automatica del grafico**: fuori scope per scelta. L'occhio è l'oracolo.

## 9. Verifiche in PIE

Ogni scenario di questo catalogo, quando viene scritto, porta una voce ⏳ in
[`test-manuali-pie.md`](../test-manuali-pie.md) con la forma `PIE-VIS-<dominio>`. La voce dice **cosa si deve
vedere**, non «lo scenario passa»: quella parte la dicono già le assertion.

**Fatto il 2026-08-08**: diciassette voci `PIE-VIS-*`, una per scenario, nella sezione *Scenari di validazione
visiva*. Il conteggio è stato **rimisurato col comando del documento**, non aggiornato a mente: da
`83 (25/21/37)` a `100 (25/21/54)`, con `senza-marcatore = 0`. Verdi e parziali non cambiano — le nuove
nascono tutte ⏳ — e la ripartizione per gruppi passa a `2+9+9+4+3+1+2+17 = 47` delle 54 aperte, con le
stesse 7 non assegnate di prima.

> ⚠️ **La convenzione non bastava, e il 2026-08-09 si è vista la falla.** Tre scenari arrivati **dopo** quel
> blocco — `Visual.Combat.GuardReducesFirstHit`, `Visual.Combat.BraceReducesEveryHit` e
> `Visual.Combat.WaterElectricCoordinated` — erano nel corpus **senza voce PIE**: eseguiti, verdi, e mai
> guardati da nessuno. È il modo peggiore in cui un file può mancare, perché *sembra* coperto due volte.
> La regola era scritta qui e non la verificava nessun comando. Ora la verifica una riga, e il posto giusto
> per eseguirla è **quando si aggiunge uno scenario `Visual.*`**:
>
> ```bash
> echo "scenari: $(find Scenarios/Visual -name '*.json' | wc -l)  \
> voci: $(grep -c '^| \*\*PIE-VIS-' docs/technical/test-manuali-pie.md)"
> ```
>
> Al 2026-08-09: **21 e 21**. Registro PIE complessivo: `116 (26/21/69)`, `senza-marcatore = 0`.
>
> ➕ **Al 2026-08-13: ancora 21 e 21** — l'invariante che questa riga protegge **regge**. Il registro
> complessivo è invece cresciuto a **135** voci: il `116` qui sopra è una misura datata e resta com'è, ma il
> totale vivo sta in `scenario-map.md`, dove il 2026-08-13 è stato corretto da `117` a `135`. ⚠️ Le due cose
> si misurano con comandi diversi e **solo la prima ha un innesco**: questa riga si riesegue quando si
> aggiunge uno scenario `Visual.*`, il totale complessivo non lo controlla nessuno — ed è per questo che era
> rimasto indietro di diciotto voci.
