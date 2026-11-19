# Scarica a spline e bersagli del banco — disegno

> **Data**: 2026-11-16. **Stato**: approvato dall'autore il 2026-11-19 («va bene»). **Issue**: [#4136](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4136)
> per la scarica, e una issue nuova per il banco, da aprire. **Decisione**: [`D-621`](../../decisions/RT_PDR_00_Decision_Log.md),
> che questo disegno estende. Il disegno della catena resta quello di
> [§2.10 del profilo FX](2026-10-08-profilo-fx-per-abilita-design.md): qui cambia l'asset di ogni salto, e il banco impara a posare più bersagli.

## 1. Perché

Le parole dell'autore, alla lettera, nella sessione `98b955f2-a994-4269-bd33-59fe4c63697f`:

1. «in FabAsset/_SplineVFX nella mappa demo vedo degli effetti interessanti, vfx su spline. potrei far partire una spline tra i bersagli»
2. «una spline per salto e scegli tu il socket. dobbiamo poter vedere l'effetto con il replay action, su 1 , 2 e 3 target che sono nell'area di effetto»
3. Fra un tasto nel PIE e una scelta nel pannello del banco: «1», cioè il tasto.
4. Sulla provenienza del pack: «va bene, spline vfx non proviene da fab, mi e' stato regalato da un amico», e poi «trovato, e' di
   Dr.Game spliveVfx».

Il «replay action» è il banco Ability / Hero Lab: lo ha stabilito [`D-618`](../../decisions/RT_PDR_00_Decision_Log.md) (1), con la risposta
dell'autore «il replay della singola action dal tool che abbiamo creato».

## 2. Obiettivi, non obiettivi, anti-obiettivi

**Obiettivi**
- Ogni salto della catena di `Hero.Gadget.LinearDischarge` è un effetto su spline, dalla sfera al primo colpito e poi da colpito a colpito.
- Nel banco, in PIE, un tasto porta i bersagli dentro l'area dell'abilità da 1 a 2 a 3, e la scena riparte.

**Non obiettivi**
- Una spline sola lungo tutta la linea.
- Il banco nella partita normale: il tasto agisce solo in una scena `AbilityLab.*`.
- L'aspetto dell'effetto (colore, spessore, durata della crescita): è lavoro d'Editor dell'autore, nella guida.

**Anti-obiettivi**: cose che renderebbero il lavoro sbagliato anche se funzionasse.
- Un secondo calcolo delle celle colpite dentro il banco: le celle le dice `URTHexCombatLibrary::HexHitCells`, la funzione del gioco.
- Un effetto che decide qualcosa: chi viene colpito lo dice il resolver, l'effetto lo mostra.
- Un arco verso un colpito che chi guarda non conosce. La privacy resta in `ChainHopFor`, che non cambia.

## 3. Vincoli

- È presentazione: niente TurnLog, niente hash, niente stato canonico.
- Il prefisso congelato del playback ([`D-355`](../../decisions/RT_PDR_00_Decision_Log.md)) e il fail-closed della privacy restano quelli di `D-621`.
- Gli asset `.uasset` di `Content/` non sono versionati. Senza l'asset il codice non mostra niente, e lo scrive solo nel log a livello `Verbose`.
- L'asset si crea nel clone principale, con l'Editor.
- `_SplineVFX` non è ancora nel registro delle licenze, `docs/technical/asset-licenze.md`. La copia sul disco viene da un amico
  dell'autore. Il pack è [*Free Spline VFX* di Dr.Game](https://www.fab.com/listings/2b923e61-b02d-4cc9-bd0b-b067c9e6056e), gratuito
  su Fab, e la pagina dice *Allows usage with AI: No* (letta il 2026-11-19). La sua descrizione elenca gli stessi dieci effetti dei
  `BP_SplineVFX_*` sul disco.

## 4. Approcci

**Quante spline.** Scelta dell'autore: una per salto.
- *Una per salto*: ogni tratto compare quando il playback mostra quel colpo. Il calcolo dei salti di `D-621` resta com'è.
- *Una sola lungo la linea* (scartata): dovrebbe conoscere tutti i colpiti dal primo colpo, crescere al ritmo dei colpi mostrati uno
  per uno ed essere ricostruita quando il turno riprende a metà. Cambierebbe la regola «un arco per colpito» di `D-621`.

**Forma dell'asset.** Scelta dell'assistente: la copia del Blueprint.
- *Copia del Blueprint del pack*: `BP_SplineVFX_ElectricLightning` contiene già la spline, il Niagara che la percorre e la timeline
  che fa crescere l'effetto (`User.Progress` da 0 a 1). Il codice mette i punti e basta. Cambiare effetto vuol dire cambiare una riga
  della tabella, senza nuovo codice.
- *Attore C++ con spline e Niagara* (scartato): il codice dovrebbe far crescere `Progress` a ogni fotogramma, e ogni effetto del
  pack andrebbe riadattato a mano.

**Come scegliere i bersagli nel banco.** Scelta dell'autore: il tasto nel PIE.
- *Tasto nel PIE*: si confrontano 1, 2 e 3 senza uscire dal PIE.
- *Selettore nel pannello* (scartato dall'autore): meno codice, ma per cambiare numero si chiude e si rilancia il PIE.

## 5. Disegno

### 5.1 La scarica su spline

- **La riga della tabella** (`DeclaredChainFxRows`) dice la classe dell'attore al posto del sistema Niagara:
  `/Game/RT/Characters/Gadget/FX/BP_Gadget_LinearDischarge_Arc.BP_Gadget_LinearDischarge_Arc_C`.
- **Il socket è `canister_main`** (scelta dell'assistente). È l'osso della sfera, e `LMB_Fire_B_Slow_V1`, l'animazione dello sparo,
  anima proprio quello: l'arco parte da dove la sfera è in quel momento. `FX_canister_socket_L` e `FX_canister_socket_R` sono nello
  scheletro, ma l'animazione dello sparo non li nomina.
- **Per ogni salto** il director crea l'attore nel punto di partenza, senza collisione. Gli scrive tre punti in coordinate mondo:
  la partenza, un punto a metà alzato, l'arrivo. Poi gli dà una vita breve, così l'attore si toglie da solo. I punti partono dagli
  stessi ancoraggi di oggi, `ARTUnit::ChainFxAnchor`, e l'arco a vuoto arriva al centro dell'esagono mirato.
- **Il punto a metà** lo calcola una funzione pura, verificabile da sola: il centro del tratto, alzato di una frazione fissa della sua
  lunghezza.
- **Il motore lo regge**, letto nel sorgente di Engine 5.8: in modo `Default`, il data interface `Spline` di Niagara trova la spline
  sull'attore a cui il componente è attaccato (`ResolveSplineComponent`). E la rilegge a ogni tick, quando la versione delle sue curve
  cambia (`PerInstanceTick`). Si possono quindi mettere i punti subito dopo la nascita dell'attore.
- **Il codice non chiama più Niagara.** Tolgo la dipendenza dal modulo `Niagara` nel `Build.cs` e la voce del plugin nel `.uproject`,
  che questo ramo aveva aggiunto per il raggio. Il plugin resta acceso di default nel motore.
- **Il lavoro d'Editor dell'autore**, che va nella guida: duplicare `BP_SplineVFX_ElectricLightning` in `Gadget/FX/` col nome della
  riga, e controllare che l'effetto non resti acceso in loop oltre la vita dell'attore.
- **La licenza**: una riga per `/Game/FabAsset/_SplineVFX/` in `docs/technical/asset-licenze.md`, come quella di Free Spells:
  fonte Fab · Dr.Game col link del listing `2b923e61`, Fab Standard License gratuita, ⛔ NoAI §5, consumatore
  `BP_Gadget_LinearDischarge_Arc`. Il registro trascrive la pagina, e la cella della licenza dice che il pack va aggiunto alla
  libreria Fab dell'autore: la copia dell'amico è arrivata dalla libreria di un altro.

### 5.2 I bersagli del banco

- **La spec del banco** (`FRTAbilityLabFixtureSpec`) prende il numero di bersagli voluti, da 1 a 3, con 1 di default. È la posa di
  oggi.
- **Dove si posano.** Le celle sono quelle che `HexHitCells` dà per l'abilità: forma, cella di chi lancia, cella mirata, portata e
  raggio d'area. Se ne tolgono quelle già occupate e quelle fuori dalla mappa del banco, e si ordinano dalla più vicina a chi lancia,
  poi con l'ordine stabile delle celle.
  - Per una **linea** che mira un'unità, se la linea è troppo corta il `TARGET` si sposta sulla stessa direzione, finché la linea non
    tiene tutti i bersagli. Deve restare nella mappa e nella portata. Per LinearDischarge, con la posa di default:
    - 1 bersaglio: il `TARGET` in (2,0);
    - 2 bersagli: un secondo in (1,0);
    - 3 bersagli: il `TARGET` passa in (3,0), e gli altri due vanno in (1,0) e (2,0).
  - Per un'**area**, i bersagli in più vanno nelle celle dell'area attorno alla cella mirata.
  - Un'abilità che colpisce una cella sola, una reazione, uno scatto, un bordo o un'azione su sé stessi restano a un bersaglio.
- **Chi sono i bersagli in più**: `TARGET2` e `TARGET3`. Sono eroi del roster diversi da quelli già in scena, nella squadra del `TARGET`.
  Se le celle o gli eroi non bastano, se ne posano quanti ce ne stanno.
- **Il numero voluto viaggia con la scena**, in un'etichetta dei suoi `Tags`. Così `R`, che ricarica lo stesso file, lo conserva. E
  `,` e `.` lo passano alla scena successiva.
- **Il tasto `U`**, da «unità colpite», nel PIE e solo in una scena del banco: porta il numero voluto da 1 a 2, a 3 e di nuovo a 1,
  ricostruisce la scena con lo stesso costruttore del pannello e la fa ripartire. `U` è libero fra i tasti che il gioco lega, e lo tiene
  libero `PlayerInput.HotkeysDoNotCollide`.
- **La banda del banco** aggiunge `«U» bersagli N`. Se l'area ne tiene meno, aggiunge `(qui ne stanno K)`.
- **Il pannello**: «Esegui in PIE» parte da 1 bersaglio.

### 5.3 Prove

- **Automatiche**:
  - la riga della tabella, con la classe e il socket;
  - il punto a metà;
  - la posa di LinearDischarge con 1, 2 e 3 bersagli: celle, `TARGET` spostato, id, eroi, squadre ed etichetta;
  - un'abilità a cella sola resta a 1, e un'abilità ad area posa i bersagli dentro l'area;
  - stessa spec, stessa scena;
  - il giro del numero e la riga della banda;
  - `PlayerInput.HotkeysDoNotCollide` resta verde.
- **PIE**:
  - `PIE-FX-CHAIN` si guarda con 1, 2 e 3 bersagli, col tasto `U`;
  - una voce nuova per il tasto del banco.
  - Le giudica l'autore.

## 6. Lasciato a chi costruisce

- I nomi delle funzioni nuove, la frazione esatta dell'alzata e la durata della vita dell'attore.
- Come il GameMode rilegge l'etichetta della scena corrente.
- Il testo della riga `D-621`. Deve citare alla lettera i tre messaggi del §1, e il contratto con l'asset passa da `BeamStart`/`BeamEnd`
  alla classe con la spline.
