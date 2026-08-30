# Identità di squadra del giocatore — spec

> Scritto il **2026-08-30**. Fetta di **preparazione**: sposta l'identità di squadra dal controller al
> `PlayerState` e la deriva dal formato. Il gioco offline resta **identico**, e non si aggiunge una riga di
> replicazione.

## 1. La domanda, e dove vive oggi la risposta

«Di che squadra è questo giocatore» è una domanda che il codice fa in **cinque punti**, e tutti e cinque
leggono lo stesso campo:

| Sito | Cosa decide |
|---|---|
| `Camera/RTCameraPawn.cpp` — `FrameOwnTeam` | quale squadra inquadrare all'avvio |
| `Player/RTPlayerController.cpp` — due siti in `OnSelect` | cosa si può selezionare e comandare |
| `Perception/RTKnowledgeVeilPresenter.cpp` — `ViewerTeamId` | di chi è la vista che il velo disegna |
| `UI/RTHUD.cpp` — `ViewerTeamIdOf` | **sei** consumatori dell'HUD, **quattro** dei quali filtri di privacy |

Il campo è `ARTPlayerController::PlayerTeamId`, un `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)` che vale
`0` e che **nessuno assegna a runtime**. Con due client varrebbe `0` per entrambi: è il primo pezzo del
gioco che si rompe appena i giocatori sono due, e si rompe **in silenzio**, perché `0` è una risposta
plausibile.

⚠️ Nessun `.uasset` cita `PlayerTeamId` — verificato su tutto `Content/` il 2026-08-30. La proprietà si può
togliere senza rompere un Blueprint.

## 2. Cosa questo documento decide

1. L'identità di squadra vive su **`ARTPlayerState`**, non sul controller, e **non è editabile**.
2. Si **deriva dal formato**: `SeatsPerTeam = UnitsPerTeam / UnitsPerPlayer`.
3. Si legge da **una funzione sola**, `ARTPlayerState::TeamIdOf(const APlayerController*)`, che ripiega a
   `0` in modo dichiarato.
4. L'assegnazione è **idempotente** e non dipende dall'ordine fra `OnPostLogin` e `BeginPlay`.

## 3. ⛔ Cosa NON fa, e va letto prima del resto

**Questa fetta NON chiude il debito di privacy di rete** dichiarato in `URTKnowledgeVeilPresenter`. Non
sanifica niente, non replica niente, non tocca la sottomissione degli intenti. Sposta *chi risponde alla
domanda dell'identità*; il canale per la conoscenza resta esattamente dove sta.

È un **prerequisito**, non un pagamento. Sta scritto qui perché fra un mese qualcuno conterebbe altrimenti
un debito come chiuso solo perché il pezzo accanto si è mosso — ed è la stessa classe di errore che
`D-242` punto (5) ha già dovuto correggere una volta, quando i consumatori di un letterale si sono rivelati
sei invece di due.

Restano fuori, e sono la fetta **P0** del piano canonico (*«multiplayer server-authoritative: replica
azioni/GameState, timeout/reconnect, lobby privata»*):

- replicazione del `TeamId` (owner-only) e del DTO di conoscenza sanificato per squadra;
- sottomissione degli intenti come RPC validata — oggi il controller scrive **direttamente** su `ARTUnit`
  (`PlannedWaypoints`, `PlannedAbilityIndex`, `PlannedAttackTarget`, `PlannedDashCell`,
  `PlannedReactionAbility`), quindi in-process il client **è** l'autorità;
- sessione, lobby, viaggio e riconnessione.

## 4. Architettura

### 4.1 `ARTPlayerState`

Nuova classe in `Player/RTPlayerState.{h,cpp}`, derivata da `APlayerState`.

Porta un `TeamId` che **non è una `UPROPERTY` editabile**: è stato di runtime, scritto da
`AssignTeam(int32)`. Il nome del metodo dichiara l'autorità — quando la replicazione arriverà, il campo
diventa `Replicated` con condizione owner-only e il seam è già al posto giusto, senza riprogettare niente.

`ARTGameMode` registra `PlayerStateClass` nel costruttore, accanto a `DefaultPawnClass`,
`PlayerControllerClass` e `HUDClass`. Sono i **framework defaults**, cioè una delle responsabilità che il
refactor SOLID ha deliberatamente lasciato nel composition root: la fetta ci si incastra senza allargarlo.

### 4.2 Il lettore unico

```cpp
/** La squadra del giocatore dietro questo controller. Senza PlayerState ripiega su 0, dichiaratamente. */
static int32 ARTPlayerState::TeamIdOf(const APlayerController* Controller);
```

Statica e pura, sul modello di `ARTHUD::ViewerTeamIdOf` — che **assorbe**.

🔴 **`ViewerTeamIdOf` non resta come inoltro.** È già la funzione giusta nel posto sbagliato: vive
sull'HUD per ragioni storiche mentre la domanda ora la fanno in cinque. Tenerla come forwarder lascerebbe
**due porte** per la stessa domanda, che è precisamente ciò che [#1730](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1730)
ha appena pagato per chiudere. I suoi **tre test** in `RTHudViewerTeamTests` si spostano con lei, e la prosa
che porta la ragione di `D-242` punto (5) va **trasferita**, non lasciata evaporare: è la spiegazione del
perché un letterale non ha un modo di fallire chiuso.

⚠️ **Il ripiego resta `0` e non diventa un sentinella.** `ViewerTeamIdOf` documenta già il perché:
*«nessuno dei sei consumatori ha una risposta per "nessuna squadra"»*. Con `INDEX_NONE` tutti e sei
dovrebbero gestire un caso che non sanno gestire — e `URTIntentPrivacyLibrary::FilterForTeam` decide con
`Intent.TeamId == ObserverTeamId`, quindi un osservatore invalido non nasconde **di meno**: rovescia la
simmetria, e gli intenti non rivelati dell'avversario diventano «alleati».

### 4.3 L'assegnazione, e il problema d'ordine

I posti si derivano dal formato: `SeatsPerTeam = Rules.UnitsPerTeam / Rules.UnitsPerPlayer`. In
`Format.Skirmish2v2` fa `2 / 2 = 1` — un posto per squadra, un solo giocatore locale, squadra `0`: identico
a oggi.

**L'ordine di riempimento è ALTERNATO**, non «prima una squadra e poi l'altra»: `Team = ArrivalIndex % 2`.
Con un posto per squadra il secondo arrivato prende la squadra `1`, che è il caso utile; riempire prima la
squadra `0` metterebbe due persone dalla stessa parte e lascerebbe l'altra vuota. In v0.1 c'è un arrivo solo
e le due regole coincidono, quindi la scelta si dichiara **adesso** — quando non costa — invece di essere
decisa per caso dal primo che scrive il ciclo.

**Due guardie, entrambe fail-closed:**

- `UnitsPerPlayer <= 0` ⇒ nessun posto è derivabile e **non si assegna niente**, dichiarandolo. Il default
  del campo è `0` e una divisione non guardata sarebbe un crash, non un ripiego.
- arrivi **oltre** i posti disponibili ⇒ non ricevono squadra e restano al ripiego, e il caso si dichiara.
  In v0.1 offline non può accadere; scriverlo costa una riga e toglie una domanda a chi aprirà la lobby.

🔴 **`OnPostLogin` e `BeginPlay` non hanno un ordine garantito**, e il formato è risolto solo dentro
`SetupHexMatch`. Un'assegnazione scritta solo nel login leggerebbe regole non ancora risolte, e il difetto
si vedrebbe *a volte*.

∴ `AssignSeats()` è **idempotente** e viene chiamata da **entrambi** i lati: da `OnPostLogin`, e da
`SetupHexMatch` dopo che il bootstrapper ha risposto. Finché le regole non ci sono non fa nulla. È la stessa
forma di `AddUniqueDynamic`, che il progetto già usa contro la stessa incertezza.

Questo richiede una modifica al bootstrapper: **`FRTMatchBootstrapOutcome` riporta le `FRTMatchRules`
risolte**. È un'aggiunta legittima — le regole sono un output che il composition root ha ragione di
conoscere — e non sposta la linea *«il GameMode risolve COSA, il bootstrapper costruisce COME»*.

### 4.4 `UnitsPerPlayer` riceve il suo primo consumatore

Il campo esiste dal `D-155` (CP 19.3) e il suo docstring ammette di essere **oggi un tripwire e non un
controllo vivo**: *«nessun percorso di risoluzione legge questo campo, quindi quel test passerebbe anche se
il campo non esistesse (verificato per mutazione). Cade il giorno in cui qualcuno ci scrivesse sopra un
ramo, ed è lì che sta il suo valore.»*

Questa fetta è quel giorno. Dopo, `MatchFormat.ResolverIsInvariantToControlCount` smette di essere un
tripwire e diventa un controllo su un campo che qualcuno legge davvero.

## 5. Il ripiego headless, e il verde vacuo

`APlayerController::PostInitializeComponents()` chiama `InitPlayerState()`, che **crea il PlayerState solo
se `World->GetAuthGameMode()` esiste** (`Controller.cpp:652`, UE 5.8). Nei mondi di prova del progetto il
GameMode viene spawnato con `SpawnActor<ARTGameMode>()`, che **non** lo registra come game mode autorevole.

∴ in quei mondi — e nello Scenario Harness — il controller ha con ogni probabilità `PlayerState == nullptr`,
e `TeamIdOf` ripiega a `0`.

🔴 **È il punto in cui questa fetta può fallire in silenzio.** `0` è esattamente ciò che i test misurano
oggi: la migrazione passerebbe tutta verde mentre il meccanismo che dichiara di coprire non viene mai
attraversato. La board risponderebbe `0` perché non c'è nessuno a rispondere, non perché il giocatore sia
della squadra `0`. È la famiglia di difetto che il repository ha già pagato con `#1467` — cinque test verdi
su un velo che nessuno stendeva — e con `UnitsPerPlayer` qui sopra.

⚠️ **Questa deduzione viene dal sorgente del motore e NON è stata misurata.** Il primo passo
dell'implementazione è un test che la accerti; costruire il resto su una supposizione sarebbe fondare la
fetta sullo stesso silenzio che vuole chiudere.

### 5.1 La fixture

`RTWorldFixtures::MakePlayerOnTeam(UWorld*, int32 TeamId)` spawna controller e `ARTPlayerState` e li lega
con `AController::SetPlayerState` — API pubblica `ENGINE_API`, verificata a `Controller.h:51`. Una sola
sede, come gli altri fixture di quel file.

I due siti che oggi vogliono davvero la squadra `1` passano di lì: `RTCameraPawnTests` e
`RTHudViewerTeamTests`.

## 6. Test richiesti

**La coppia che discrimina**, e nessuno dei due basta da solo:

| Test | Cosa impedisce |
|---|---|
| controller **con** PlayerState su squadra 1 → `TeamIdOf` risponde `1` | che l'intera migrazione restituisca `0` ovunque restando verde |
| controller **senza** PlayerState → risponde `0`, **dichiarato come ripiego** | che il ripiego sia ereditato dal silenzio invece che scelto |

Più:

- l'assegnazione deriva dal formato: due formati con `UnitsPerPlayer` diverso producono un numero di posti
  diverso;
- l'assegnazione è **idempotente**: chiamarla due volte non sposta nessuno;
- l'assegnazione non dipende dall'ordine: `AssignSeats` prima delle regole non fa nulla e non sporca niente;
- i tre test di `RTHudViewerTeamTests` migrati alla nuova sede, invariati nelle attese.

## 7. Il presidio, e il suo limite dichiarato

Un test di **reflection**: `TFieldIterator<FProperty>` su `ARTPlayerController` che asserisce che non esista
nessuna `UPROPERTY` il cui nome contenga `TeamId`. Non è una lista scritta nel test — interroga la classe
**reale**, come già fanno `Heroes.AbilityIdsAreNamespacedUnderTheirHero` e
`Unit.CanonicalHeroIdHasNoLegacyName` sul roster. Chi riaprisse il campo trova rosso senza che nessuno
aggiorni niente.

⚠️ **Coglie il campo riaperto; NON coglie un lettore nuovo** che inlinei
`Cast<ARTPlayerState>(PC->PlayerState)->TeamId` duplicando il ripiego. Contro quello la difesa è più debole:
la prosa su `TeamIdOf` che si dichiara l'unica porta, e il fatto che il ripiego viva in un posto solo.
È una difesa **parziale**, e scriverla come totale sarebbe la stessa disonestà che il docstring di
`UnitsPerPlayer` ha dovuto ammettere.

⛔ **Nessun gate `tools/radar/`.** Non c'è CI (`D-182`), quindi *«li esegue chi committa, o non li esegue
nessuno»*: un test dentro la suite gira sempre, un gate a mano no.

## 8. Impatto sui file

**Nuovi**: `Player/RTPlayerState.{h,cpp}` · test dell'identità di squadra · voce in `RTWorldFixtures.h`.

**Modificati**: `RTPlayerController.{h,cpp}` (campo rimosso, due letture) · `RTGameMode.{h,cpp}`
(`PlayerStateClass`, `OnPostLogin`, `AssignSeats`) · `Match/RTMatchBootstrapper.h` (le regole nell'esito) ·
`Camera/RTCameraPawn.cpp` · `Perception/RTKnowledgeVeilPresenter.cpp` · `UI/RTHUD.{h,cpp}`
(`ViewerTeamIdOf` assorbita) · `Tests/RTCameraPawnTests.cpp` · `Tests/RTHudViewerTeamTests.cpp`.

**Nessun `.uasset`.**

## 9. Verbale

Serve un **`D-nnn`** nel Decision Log, con il controllo anti-collisione sui **ref remoti** e non su
`gh pr list`: fra il commit che prende un ID e l'apertura della sua PR c'è una finestra in cui l'ID è preso
e invisibile, e il progetto ha già pagato diciassette collisioni.

Contenuto: l'identità di squadra vive sul `PlayerState`, si deriva dal formato, ha un lettore solo, il
ripiego a `0` è dichiarato e testato, `UnitsPerPlayer` riceve il suo primo consumatore vivo — e questa fetta
**non** paga il debito di rete del presenter.
