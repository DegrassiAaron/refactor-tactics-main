# Rassegna dei `requires` — verbale, 2026-09-12

> `ESITO DI PASSAGGIO` · Fetta **1** di
> [`sedute-componibili-design-2026-09-11.md`](sedute-componibili-design-2026-09-11.md) ·
> Piano: [`sedute-componibili-piano-fetta-1-2026-09-12.md`](sedute-componibili-piano-fetta-1-2026-09-12.md)
>
> ⛔ **Questo documento non possiede niente.** Non è un registro e non va tenuto aggiornato.
> Dice una cosa sola: **al 2026-09-12 questi check erano stati esaminati, e con quale esito.**
> Lo stato di un check appartiene a `docs/technical/test-manuali-pie.md`; il cablaggio a
> `docs/roadmap/sedute-mattoni.yaml`; le sedute, fino alla fetta 5, a `docs/roadmap/editor-sessions.yaml`.
>
> 🔑 **Convenzione sui numeri.** Ogni conteggio è un esito di questo passaggio e porta il comando
> che lo produce. Chi rilegge **rimisura**.

## Cos'è stato esaminato

Il bacino: i check **cablati** e **non verdi** — un check verde è finito, un check non cablato
appartiene alla coda scoperta, che è un'altra misura.

```bash
python - <<'PY'
import sys; sys.path.insert(0, 'tools/editor-sessions')
import registry, pie_status
_, w = registry.load(); s = pie_status.load()
print(len([x for x in w if x.check in s and s[x.check]["stato"] != pie_status.VERDE]))
PY
```

→ **118** al 2026-09-12, su `c8a2116a`.

Il Task 6 esamina il bacino a lotti, per allestimento: il primo lotto copre `SET-FRONTEND`,
`SET-GRAYKIT`, `SET-HEX-TURN`; il secondo aggiunge `SET-SANDBOX` e `SET-GEN-ARENA`. I lotti
successivi coprono il resto (vedi la riga in coda alla tabella).

## Il criterio

Una domanda sola, per ogni check: **qualcuno che apre l'Editor in quell'allestimento, oggi, può
arrivare a un verdetto?** Se sì, nessun prerequisito — l'assenza della riga *è* la dichiarazione.
Se no, il prerequisito si dichiara col tipo del proprio oracolo, e con l'owner che può toglierlo.

Un `requires` non verificato non si scrive: un falso positivo toglie un check dall'ordine del
giorno **in silenzio**, un falso negativo si vede aprendo l'Editor. Gli errori non costano uguale.

## Il verbale

| check | allestimento | decisione | prova |
|---|---|---|---|
| `PIE-V01-FRONTEND-MAIN` | `SET-FRONTEND` | — | `WBP_RT_MainMenu` tracciato (PR #1178, `git ls-files Content` conferma) e cablato (`PLAY→StartMatch`, `SETTINGS→PushScreen`, `QUIT→QuitGame`); già **ESEGUITA il 2026-09-04** con verdetto — la nota che diceva «manca l'asset» era superata |
| `PIE-V01-FRONTEND-NAV` | `SET-FRONTEND` | — | i cinque `WBP_RT_*` della precondizione (seduta `U24`: `WBP_RT_FallbackBanner`, `-ErrorModal`, `-LoadingScreen`, `-FrontendRoot`, `-ModalLayer`) sono tutti tracciati (`oracles.asset_tracciati()`); la catena `BeginPlay → StartFrontendForThisGame → StartFrontend → InitializeFrontend` esiste in codice ed è coperta dal test `FrontendGameModeStartsTheFrontend` — misurato chiudendo #938 |
| `PIE-V01-FRONTEND-PAUSE` | `SET-FRONTEND` | `asset:WBP_RT_PauseMenu` | non tracciato in `Content/` (`oracles.asset_tracciati()` → `False`); la riga di registro e la seduta `U30` lo nominano `*** NON TRACCIATO ***` |
| `PIE-V01-FRONTEND-RESULT` | `SET-FRONTEND` | `asset:WBP_RT_ResultScreen` | non tracciato in `Content/` (`oracles.asset_tracciati()` → `False`); la riga di registro e la seduta `U29` lo nominano `*** NON TRACCIATO ***` |
| `PIE-V01-SCREENHUD` | `SET-FRONTEND` | — | già **ESEGUITA il 2026-09-11 su `43e7126b`** con verdetto reale (quattro criteri verdi, uno non eseguibile, uno fallito, uno bloccato dal fallito) — la voce è dimostrabilmente raggiungibile. `WBP_RT_EventLogRight` non è un package: compare solo in un commento di test (`RTMatchWidgetAssetTests.cpp:987`) come nome di **nodo** dentro l'albero del widget, e il mount report della seduta lo conferma costruito (`[ok] EventLog: 1`) dalla classe tracciata `WBP_RT_EventLog`. I due difetti aperti — #2963 (catalogo icone, blocca il criterio Dock) e #2964 (`Il feed e' montato e il turno produce eventi, ma a schermo non compare nessuna riga`, blocca Feed) — sono già registrati come tali nel registro PIE stesso, non impediscono l'accesso al check. #2697 (`gh issue view 2697`, aperta) parla di zero consumatori del feed: non è più il caso qui, dove un consumatore (`WBP_RT_EventLogRight`) risulta montato |
| `PIE-TD-CLEAN` | `SET-GRAYKIT` | — | nessun artifact nelle sedute che lo convocano (`U31`, `U32`: `artifacts: []`); il crash che aveva interrotto `U31` (#2115) è `CLOSED`, corretto lo stesso giorno; `U32` l'ha già confermato in parte il 2026-08-30 |
| `PIE-TD-DOCK` | `SET-GRAYKIT` | — | nessun artifact (`U31: artifacts: []`); il difetto che lo rendeva rosso (#2168) è `CLOSED` dal 2026-09-03, mai riverificato ma non bloccato — la precondizione (`Load Layout → Default Editor Layout`) è procedurale, non un prerequisito mancante |
| `PIE-TD-PRESENT` | `SET-GRAYKIT` | — | nessun artifact (`U31: artifacts: []`); `L_DevSandbox` tracciato; la parte confermata il 2026-09-02 resta valida, il resto fu interrotto dallo stesso #2115, `CLOSED` lo stesso giorno |
| `PIE-PREVIEW-GHOST` | `SET-HEX-TURN` | — | nessun artifact (`U52: artifacts: []`); il codice è in `main` da #2941, coperto headless da sette `Preview.*`; la precondizione è CVar da console + un piano costruito a mano, nessun asset o widget mancante — non ancora eseguita perché nessuna sessione precedente aveva uno schermo da guardare, non perché manchi un prerequisito |
| `PIE-AS2` | `SET-SANDBOX` | — | eseguibile col roster tracciato; nome ritirato, difetto di prosa, owner [#2297] |
| `PIE-AS4b` | `SET-SANDBOX` | — | stessa condizione di `PIE-AS2`: i quattro `BP_Unit_*` citati sono nomi ritirati, difetto di prosa, owner [#2297]; seduta `U8` ha `artifacts: []` **di proposito** — dal 2026-09-05 i tre ruoli (Cast/Hit/Death) sono dati nel CDO di `URTUnitAnimInstance`, non `.uasset`; eseguibile su una partita 2v2 normale (stessa configurazione su cui `PIE-AS4a` è ✅), non sul banco del corpus |
| `PIE-AS4c` | `SET-SANDBOX` | — | stessa condizione di `PIE-AS4b`, stesso banco (`Visual.Combat.Defeat`, seduta `U8`); nome ritirato, difetto di prosa, owner [#2297] |
| `PIE-FMT-01` | `SET-SANDBOX` | — | precondizione «CP 19.1 atterrato»: `#215` chiusa 2026-08-09, Epic `E19` chiusa 2026-09-02 (`gh issue view 215`; `docs/roadmap/roadmap-v0.1.md:217`); la riga di registro non nomina asset o widget mancanti |
| `PIE-GBX-COVER` | `SET-SANDBOX` | — | `SM_Graybox_Cover_Low` e `SM_Graybox_Cover_High` tracciati (`oracles.asset_tracciati()` → `True`); seduta `U25`: `artifacts: []`; `FixtureId = CoverYard` è una fixture di codice (`RTHexMapActor.h:270`, `URTMatchSetupLibrary::MakeFixtureArena`), non un asset |
| `PIE-GBX-DOOR` | `SET-SANDBOX` | — | `SM_Graybox_Door_Panel` e `SM_Graybox_Door_Locked` tracciati; il comando `RTSetCellDoor` (da #2337, mergiata) posa le quattro celle senza toccare il Details; nessun package mancante |
| `PIE-GBX-FIT` | `SET-SANDBOX` | — | riusa `FixtureId = CoverYard`, la stessa fixture di codice di `PIE-GBX-COVER`; nessun asset separato nominato mancante |
| `PIE-GBX-SURFACE` | `SET-SANDBOX` | — | `SM_Graybox_Surface_Water` e `SM_Graybox_Surface_Ice` tracciati; il dato `ERTHexSurface` esiste già e si dipinge col tool Paint, nessun package mancante |
| `PIE-GBX-VOLUME` | `SET-SANDBOX` | — | `BP_Graybox_CellPlacementVolume` tracciato; nessun asset o widget nominato mancante |
| `PIE-GBX-ZOOM` | `SET-SANDBOX` | — | dipende dalle cinque voci `PIE-GBX-*` sopra, tutte eseguibili oggi (asset tracciati); la scena di `U25` non aspetta altro |
| `PIE-HEX-COORD-COSTO` | `SET-SANDBOX` | — | seduta `U39`: `artifacts: []`; la mappa si genera col gesto `Generate Into Asset` sull'`ARTHexMapActor` già in livello (`grep -n 'void ARTHexMapActor::GenerateIntoAsset' Source/RefactorTactics/Map/RTHexMapActor.cpp` → riga 2264; una citazione precedente non ricontrollata diceva `:1736`, corretta in questo giro); resta solo il giudizio a schermo sulla fluidità, non un prerequisito mancante |
| `PIE-HEX-MOVEMENT-PROBE` | `SET-SANDBOX` | — | `ProbeYard` è una fixture di codice (`URTMatchSetupLibrary::MakeFixtureArena`, `RTHexMapActor.h:270`, testata in `RTProbeYardFixtureTests.cpp`), non un asset; seduta `U26`: `artifacts: []` |
| `PIE-HEX-MOVEMENT-PROBE-FLUIDITA` | `SET-SANDBOX` | — | stesso `ProbeYard` di codice della voce sopra; il fix di #1900 è già in `main`, resta solo il giudizio a schermo sull'attraversamento di molte celle escluse |
| `PIE-HEX-MOVEMENT-PROBE-SURFACE` | `SET-SANDBOX` | — | stesso `ProbeYard` di codice più il tool Paint, già disponibile; nessun asset o widget mancante |
| `PIE-ICON-01` | `SET-SANDBOX` | — | `DA_IconCatalog` tracciato (`oracles.asset_tracciati()` → `True`) e popolato — 61 chiavi/62 texture, cinque categorie v0.1 (`docs/roadmap/roadmap-v0.1.md:218`); i widget lo consumano dal 2026-08-28 (#1556, mergiata); `#219`/`#220` restano `OPEN` (`gh issue view`) ma solo perché questa stessa voce non è ancora stata eseguita, non per un prerequisito mancante |
| `PIE-SCEN-COMPOSER` | `SET-SANDBOX` | — | il candidato `asset:WBP_RT_ScenarioComposer` del brief è superato dalla riga stessa: quell'asset si è ritirato il 2026-09-09 (#2789) — non tracciato, confermato (`oracles.asset_tracciati()` → `False`, `git ls-files 'Content/RT/Editor/**/WBP_RT_ScenarioComposer*'` → vuoto) — ma non è più il soggetto del DoD. Il DoD si esegue su `SRTLauncherScenarioPanel` (classe C++/Slate in `Source/RefactorTacticsEditor/`, non un asset), tab Tactical Designer su `L_DevSandbox`; seduta `U50` (`artifacts: []`) lo rivendica dal 2026-09-10 |
| `PIE-TD-FILTERS` | `SET-SANDBOX` | — | stessa precondizione di `PIE-SCEN-COMPOSER` («come sopra»); seduta `U32`: `artifacts: []`, già confermata nell'insieme il 2026-08-30 |
| `PIE-V01-COVEREDIT` | `SET-SANDBOX` | — | `DA_HexMap_Sandbox` tracciato (`oracles.asset_tracciati()` → `True`) ma vuoto (0 celle); il gap che questo check giudica — copertura bassa su un bordo esposto — è dichiarato **fuori scope** da `U13.done_when` perché `#1738` non lo copre e oggi **non ha una issue propria**. Nessuno dei cinque tipi di `requires` lo esprime: `asset:` non si applica (il package c'è), e i quattro tipi con issue pretendono un `#numero` che qui non esiste. Non si dichiara — dettagli in coda al verbale |
| `PIE-AI-01` | `SET-GEN-ARENA` | — | seduta `U5`: `artifacts: []`, allestimento `MapSource = GeneratedTestArena` (codice, non asset); nessun package o widget nominato mancante nella riga di registro |
| `PIE-AI-02` | `SET-GEN-ARENA` | — | stessa seduta `U5`, stesso allestimento di `PIE-AI-01`; nessun asset o widget mancante |
| `PIE-AI-03` | `SET-GEN-ARENA` | — | stessa seduta `U5`, stesso allestimento; nessun asset o widget mancante |
| `PIE-AI-04` | `SET-GEN-ARENA` | — | stessa seduta `U5`, stesso allestimento; nessun asset o widget mancante |
| `PIE-AI-05` | `SET-GEN-ARENA` | — | stessa seduta `U5`, stesso allestimento; nessun asset o widget mancante |
| `PIE-HEX-MODE-H` | `SET-GEN-ARENA` | — | seduta `U1`: `L_HexArena` e `DA_HexMap_Arena` entrambi tracciati; lo stato ❌ è un esito osservato in editor (due comportamenti del gizmo), non un blocco d'accesso — `#931` e `#996` tracciano i difetti trovati, non l'eseguibilità del check |
| `PIE-HEXPLAY-4` | `SET-GEN-ARENA` | — | seduta `U2`: `artifacts: []`, `MapSource = GeneratedTestArena`; il verdetto di questa voce è già ✅ (2026-08-29); il richiamo a #2370 nella riga riguarda un'osservazione diversa dallo stesso banco, dichiarata come tale («non è un residuo di questa voce e non ne cambia il glifo») |
| `PIE-HEXPLAY-7` | `SET-GEN-ARENA` | — | seduta `U5`: `artifacts: []`; free-run `AutoBattle.ArenaV01` già eseguita (22 turni, `PASS`); nessun asset mancante, resta solo il giudizio a schermo sul comportamento, già dichiarato ⏳ nella riga |
| `PIE-PC-GYM` | `SET-GEN-ARENA` | — | il candidato `asset:L_CameraFeatureLab` del brief non serve: non compare né nella riga di registro né in `U37.artifacts` (`[]`); `U37` dichiara `MapSource = GeneratedTestArena`, stesso allestimento di `U2..U6`, «si esegue senza riavviare»; `L_CameraFeatureLab` non è tracciato (`oracles.asset_tracciati()` → `False`) ma non è il soggetto del check |
| `PIE-PREVIEW-PERSIST` | `SET-GEN-ARENA` | — | seduta `U3`: `artifacts: []`; la riga di registro descrive un difetto di comportamento già riprodotto («oggi spariscono»), non un asset o widget assente — il check è già eseguibile e il suo esito atteso è proprio osservare quel difetto |
| `PIE-GRID-CONFINE` | `SET-HEX-MATCH` | — | eseguibile dal 2026-08-30 (seduta `U35`); `FixtureId = CoverYard` (esagono pieno raggio 3) è la stessa fixture di codice già usata da `PIE-HEX-VIZ-BORDI` e dalle `PIE-GBX-*`; coperto in parte headless da `HexMapActor.GridToggleChangesNothingButVisibility` — restano i quattro giudizi a schermo che nessun test può dare, non un prerequisito mancante |
| `PIE-OBJ-PUNTI` | `SET-HEX-MATCH` | — | eseguibile dal 2026-08-29 (`#75`, `CLOSED`, `gh issue view 75`; commit `d070a1a4`); `FRTHexCellData::bIsObjective` è un dato della mappa, non una regola derivata — nessun asset o widget mancante, il punteggio si legge dal combat log |
| `PIE-V01-ARENA` | `SET-HEX-MATCH` | — | `DA_HexMap_Arena`/`L_HexArena` tracciati (`.gitignore:148-149`, confermati anche dalla voce `PIE-HEX-MODE-H` del lotto precedente su `SET-GEN-ARENA`); l'ostacolo residuo è disegnare/verificare l'arena con `RefactorTactics.exe -dpcvars=rt.Map.Source=LevelAsset` — lavoro d'editor e giudizio umano, non un asset o una issue bloccante |
| `PIE-V01-BLINDFIRE` | `SET-HEX-MATCH` | — | eseguibile ora: `Hero.Muiren.MistVeil` è nel roster e il percorso è quello del giocatore (`HandleTargetCell`), non l'harness. `WBP_RT_EventLogRight` **non è un package** — stessa conclusione già scritta per `PIE-V01-SCREENHUD` in questo verbale (riga `SET-FRONTEND`): compare solo come nome di nodo in un commento di test, non genera un `asset:` |
| `PIE-V01-BOARD` | `SET-HEX-MATCH` | — | `#956` e `#1665` entrambe `CLOSED` (`gh issue view`); `SurfaceGlyphs[4]`, `GlyphCells[4]` e `GetCellGlyphMesh(int32)` esistono in `RTHexMapActor.h`; serve solo `FixtureId = RelayBasin` + `rt.Map.Source = LevelAsset` e un giudizio a picco/scala di grigi — nessun asset o widget mancante |
| `PIE-V01-COLL` | `SET-HEX-MATCH` | — | già **ESEGUITA il 2026-09-04** (seduta `U14`, `Movement.CollisionChoke`, `PASS 6/6`) con verdetto reale sulle tre clausole — due ✅, una ❌ (il denial resta muto in partita normale); il difetto trovato è tracciato da [#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79) (`OPEN`), che registra l'esito e non impedisce l'accesso al check |
| `PIE-V01-DASHCOVER` | `SET-HEX-MATCH` | — | coperto headless dal 2026-08-06 (`HexSim.DashIsLinear`, `HexMove.DashRefusesBlockedDestination`, `HexMatch.HeroDashResolvesLinearly`); resta solo il giudizio a schermo che lo scatto non si pianifichi — nessun asset o feature mancante |
| `PIE-V01-DEBUG` | `SET-HEX-MATCH` | — | gli otto comandi `rt.Debug.*` sono tutti registrati — sette in `Source/RefactorTactics/Debug/RTDebugConsole.cpp`, `DrawCells` in `Map/RTHexOverlayConsole.cpp` — rimisurato il 2026-08-29; resta solo la parte visiva dei cinque `Draw*`, che nessun test headless vede — nessun prerequisito mancante |
| `PIE-V01-DOOR` | `SET-HEX-MATCH` | — | `DA_HexMap_Arena` porta una porta `Closed` su `(q=-3,r=2,L=0)` dal 2026-09-06 (`#2330`); `Action.Interact` **commuta** la porta dal 2026-09-05 (`INT-7`, `#2380`, `CLOSED`, `gh issue view 2380`; `grep -n "Action.Interact" Source/RefactorTactics/Ability/RTCatalogLibrary.cpp` → riga 1123) — nessun asset o feature mancante |
| `PIE-V01-ELEC` | `SET-HEX-MATCH` | — | entrambi i lati sono cablati nel roster: `Gadget.Sprinkler` = `Action.CreateWater` assegnato a `Hero.Muiren` (`grep -n 'Hero.Muiren.*Gadget.Sprinkler' Source/RefactorTactics/Ability/RTCatalogLibrary.cpp` → riga 776); `Hero.Aevik.ConductiveNode` legge il grafo conduttivo con `PropagationLimit` preso dal core (`RTHeroCatalogLibrary.cpp:381`, `D-064`, 2026-08-10). CP 8.3/8.4 chiusi il 2026-08-07 (`roadmap-v0.1.md:855-856`), coperti headless da `Environment.WaterElectricPropagation` e affini — nessun asset o feature mancante, solo da eseguire |
| `PIE-V01-FALLBACK` | `SET-HEX-MATCH` | — | coperto headless da otto `Actions.Fallback.*`; resta solo il giudizio di leggibilità sul combat log — nessun prerequisito mancante |
| `PIE-V01-FF` | `SET-HEX-MATCH` | — | l'anteprima esiste (cella alleata in arancione, `Preview.AllyInAreaIsFlagged`); resta solo il giudizio se si nota prima del lock-in — nessun prerequisito mancante |
| `PIE-V01-FIREWATER` | `SET-HEX-MATCH` | — | `ERTHexSurface::Fire` è un valore dell'enum (`Source/RefactorTactics/Map/RTHexCellData.h:22`) dipingibile col tool Paint (`ERTHexSurface Surface` in `Source/RefactorTacticsEditor/Private/Tools/RTHexPaintTool.h:40`), esattamente come si dipingono già Water/Ice (`PIE-GBX-SURFACE`); `Gadget.Sprinkler`/Muiren crea l'acqua sopra. CP 8.4 chiuso il 2026-08-07, coperto headless da `Environment.WaterExtinguishesFire` — nessun asset o feature mancante |
| `PIE-V01-INTERCEPT` | `SET-HEX-MATCH` | — | coperto headless (`Reactions.Intercept*`) e cablato su tre abilità d'eroe reali dal 2026-08-07; resta solo la leggibilità a schermo di chi ha incassato il colpo — nessun prerequisito mancante |
| `PIE-V01-LOWCOVER` | `SET-HEX-MATCH` | — | ⚠️ **la riga di registro è stale**: dice «oggi nessuna mesh rappresenta il riparo», ma `ARTHexMapActor::RebuildInstances` disegna un pannello per bordo per ogni `FRTHexCover` (`grep -n 'for (const FRTHexCover' Source/RefactorTactics/Map/RTHexMapActor.cpp` → riga 2014, `AddEdgePanel`) ad altezze **distinte** — `RTCoverLowHeight = 22.f` contro `RTCoverHighHeight = 55.f` (`RTHexMapActor.cpp:128-129`). Il canale visivo esiste: il check è eseguibile sulla leggibilità reale, non su un dato assente |
| `PIE-V01-MAPSCALE` | `SET-HEX-MATCH` | — | `L_HexArena`/`DA_HexMap_Arena` tracciati (vedi `PIE-V01-ARENA`); il criterio delle due rotte è definito in U1 passo 7 (`docs/roadmap/editor-sessions.yaml:443-450`) e questa voce lo **misura**, non lo ridefinisce. ⚠️ Il rimando «(vedi sotto)» nella riga di registro non punta a nessun testo successivo — verificato leggendo la tabella fino alla riga finale (`docs/technical/test-manuali-pie.md:1541-1548`): è un riferimento morto, non un prerequisito mancante. L'osservazione resta eseguibile sulla mappa esistente, quale che sia il suo esito |
| `PIE-V01-MATCHLEN` | `SET-HEX-MATCH` | — | una partita 2v2 completa è già stata giocata fino in fondo (`PIE-V01-MATCHEND`, ✅ con riserva, seduta `U18`, `#1013`); l'unico avvertimento della riga riguarda la comparabilità del campione del turno 1 fra date diverse (`D-314`, `#2102`), non l'eseguibilità del check — nessun prerequisito mancante |
| `PIE-V01-MORTAR` | `SET-HEX-MATCH` | — | eseguibile con la formazione descritta nella riga stessa (`Hero.Branth` in `Team0Heroes`, `BotAllyCount = 0`); `Hero.Branth.MortarShot` è nel roster e il percorso è quello del giocatore (`HandleTargetCell`), non l'harness — nessun asset o feature mancante |
| `PIE-V01-OVERWATCH` | `SET-HEX-MATCH` | — | la UI esiste dal 2026-09-09: `URTReactionWindowViewModel::Hook` fa `BindUObject` e `ARTGameMode::BeginPlay` chiama `HookReactionWindow()` (`#2723`); la finestra si apre in PIE. Il «gate circolare» di [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) (`OPEN`) riguarda solo la **chiusura del CP 14.6**, non l'eseguibilità del check — si dichiara il commit (`5a560500` o successivo) invece dello stato dell'epic |
| `PIE-V01-PUSH` | `SET-HEX-MATCH` | — | coperto headless (`Actions.Push.InvalidDestination`, `Actions.Pull`); resta solo il giudizio a schermo che l'unità non si sposti affatto — nessun prerequisito mancante |
| `PIE-V01-READY` | `SET-HEX-MATCH` | — | `RequestLockIn`/`CancelLockIn` esistono dal 2026-09-04 (`#2193`); il countdown si vede a schermo dal 2026-09-05 (`#2358`, riga di stato `Ready - <n>s (RMB: annulla)`) — nessun asset o feature mancante |
| `PIE-V01-REPLAY` | `SET-HEX-MATCH` | — | ⚠️ **la riga di registro è stale**: dice che i comandi «non esistono ancora (CP 11.4)», ma `rt.Debug.DumpTurnLog` e `rt.Debug.VerifyReplay` sono registrati come `FAutoConsoleCommandWithWorldArgsAndOutputDevice` (`grep -n 'GRTDebugDumpTurnLog\|GRTDebugVerifyReplay' Source/RefactorTactics/Debug/RTDebugConsole.cpp` → righe 358, 363) — la stessa contraddizione che la riga gemella `PIE-V01-DEBUG`, nello stesso file, aveva già corretto (CP 11.4 è chiuso nel codice, il roadmap non è stato aggiornato). Nessun prerequisito mancante |
| `PIE-V01-ROUGH` | `SET-HEX-MATCH` | — | coperto headless (`HexSim.ReachableRespectsTerrainCost`, `MatchSetup.TestArenaHasTheFeaturesItPromises`); resta solo il giudizio sul budget mostrato — nessun prerequisito mancante |
| `PIE-HEX-VIZ-BLOCCHI` | `SET-HEX-BOT` | — | già **ESEGUITA** con verdetto ❌ reale (seduta `U18`, 2026-08-20 sulla metà «dall'alto»; rimisurata il 2026-09-05 con `MakeBlockYardArena` + `rt.Test.Scenario Visual.Map.BlockVolumes`, che costruisce i tre casi in fila): la voce è dimostrabilmente raggiungibile. Il difetto trovato è tracciato da [#1246](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1246) (`OPEN`), che registra l'esito e non blocca l'accesso al check |
| `PIE-HEX-VIZ-PORTE` | `SET-HEX-BOT` | — | `FixtureId = GrayKitYard` porta già i quattro stati di porta; `URTSetCellDoorCommandlet` (`Source/RefactorTacticsEditor/Private/Content/RTSetCellDoorCommandlet.cpp`) li posa da riga di comando dal 2026-09-04 (`#2337`, mergiata) — nessun asset o widget mancante |
| `PIE-HEX-VIZ-UNDO` | `SET-HEX-BOT` | — | prima metà già ✅ (Ctrl+Z non lascia geometria orfana); la seconda richiede un livello usa-e-getta. ⚠️ **Correzione rispetto al candidato letterale della riga di registro**: `L_Throwaway.umap` sotto `Content/RT/Maps/Dev/` NON è più ignorato — `git check-ignore -q "Content/RT/Maps/Dev/L_Throwaway/L_Throwaway.umap"` → **1** (tracciato), perché `.gitignore:79` (`!Content/RT/Maps/**/*.umap`) re-include l'intero albero dal 2026-08-22. Il posto sicuro è **sotto `_Scratch/`**, riescluso da `.gitignore:86` (`Content/RT/Maps/**/_Scratch/`): `git check-ignore -q "Content/RT/Maps/Dev/_Scratch/L_Throwaway.umap"` → **0** (ignorato). Nessun asset mancante — solo un percorso da correggere, non un `requires` |
| `PIE-TEST-CONSOLE` | `SET-HEX-BOT` | — | `#1154` (`CLOSED`) ha corretto `FindLatestRunDirectory`; `rt.Test.List`, `rt.Test.Run` e `rt.Test.DumpResult` esistono e rispondono tutti e tre, resta solo da rieseguire in una partita — nessun prerequisito mancante |
| `PIE-V01-KIT-HOTKEYS` | `SET-HEX-BOT` | — | `ARTPlayerController::GenericHotkeys()` (`grep -n 'GenericHotkeys' Source/RefactorTactics/Player/RTPlayerController.cpp` → righe 341, 363) cabla le cinque generiche su tasti propri dal 2026-08-31 (`#1439`, `MERGED`); i dieci tasti e il kit d'eroe sono raggiungibili in PIE, resta solo da premerli |
| `PIE-V01-REACTCOND` | `SET-HEX-BOT` | — | il bloccante (`Ability5Action` inesistente) è caduto il 2026-08-26 con `#1439` (`MERGED`): i tasti abilità coprono `1`–`9`/`0`, `Hero.Aevik.ReactiveCapacitor` (indice 4) si arma col tasto **5** nella formazione di default — nessun prerequisito mancante |
| `PIE-V01-SHIELD` | `SET-HEX-BOT` | — | già **PARZIALE, eseguita il 2026-09-05** (`e1ed997f`): tasto e ramo self confermati, effetto e cooldown confermati; resta solo da leggere la barra scudo nei tre momenti (`5 → 30 → 5`). `BP_Unit_Phase_C_0` è un nome d'istanza dell'Outliner, non un package — non genera un `asset:`. Il portatore (`Action.Shield`) è legale da [D-226](../decisions/RT_PDR_00_Decision_Log.md) |
| `PIE-ACC-ENVIRONMENT` | `SET-SCEN` | — | scenario `Visual.Environment.Acceptance` tracciato (`Scenarios/Visual/Environment/Acceptance.json`); la riga di registro dichiara esplicitamente «nessun asset da preparare» — `rt.Test.Scenario Visual.Environment.Acceptance` porta con sé arena, unità e piani |
| `PIE-ACC-GUARDBRACE` | `SET-SCEN` | — | scenario `Visual.Combat.GuardVsBraceUnderSmallHits` tracciato (`Scenarios/Visual/Combat/GuardVsBraceUnderSmallHits.json`), già verde headless (`PASS 7/7` il 2026-09-03); «nessun asset da preparare» nella riga di registro |
| `PIE-ACC-MAP` | `SET-SCEN` | — | scenario `Visual.Map.Acceptance` tracciato (`Scenarios/Visual/Map/Acceptance.json`); «nessun asset da preparare» nella riga di registro |
| `PIE-BAL1` | `SET-SCEN` | — | non in `RELEASE-V01`. Lo scenario `Spec.Brace.GuardAndBraceOnMixedHit` (`Scenarios/Spec/Brace/GuardAndBraceOnMixedHit.json`) è tracciato e dà oggi un esito **baseline**, legittimo secondo la riga stessa («uso legittimo prima dell'implementazione: girarla sulla coppia vecchia come baseline»). La domanda vera — Guard e Brace leggibili ora che hanno lo **stesso prezzo** — resta non ottenibile: verificato in `Source/RefactorTactics/Ability/RTCatalogLibrary.cpp` che `Action.Brace` applica ancora `Status.Braced`+`Status.Root` (non `Status.Slow`, righe 1493-1499) e `Action.Guard` non si auto-infligge `Status.Slow` (righe 1103-1106) — D-205/D-208 non sono atterrate in codice. Nessun `requires` dichiarato: l'unica issue trovata, [#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403), è il gate umano di **questo stesso check** («resta il gate umano U20/PIE-BAL1», `docs/OPEN_DECISIONS.md:1710`) e chiuderebbe solo eseguendo questa voce — non è un owner che possa sbloccarla dall'esterno. Dettagli in coda al verbale |
| `PIE-GEO-ANCHOR` | `SET-SCEN` | — | precondizione procedurale (tool Geometry attivo, gesto iniziato), nessun asset o scenario nominato; coperto headless da `Anchor.PaletteIsThirteen`, `Anchor.InexpressiblePairsAreRefused`, `Anchor.SnapNeverInventsTheInexpressible`, `Anchor.RefusesWhatItCannotSay`, `AnchorReadout.IncidenceObeysTheVerdict` — il residuo è il gesto a schermo |
| `PIE-GEO-CALPESTABILE` | `SET-SCEN` | — | i due scenari che allestiscono i due versi sono tracciati: `Spec.Map.WallCrossesCellStillStandable` e `Spec.Map.FootprintCollisionBlocksCell` (`Scenarios/Spec/Map/`); precondizione procedurale (tool Geometry, `bShowOverlay`) |
| `PIE-GEO-CENTRO` | `SET-SCEN` | — | precondizione procedurale (tool Geometry, overlay occupancy) più il comando `rt.Debug.DumpCellPlacement`, della stessa famiglia degli `rt.Debug.*` già confermati registrati per `PIE-V01-DEBUG` nel lotto precedente; nessun asset nominato mancante |
| `PIE-GEO-INCIDENZA` | `SET-SCEN` | — | scenario `Spec.Map.IncidentWallsStillPlay` tracciato (`Scenarios/Spec/Map/IncidentWallsStillPlay.json`); precondizione procedurale (tool Geometry, un muro interno già disegnato) |
| `PIE-HEXPLAY-6` | `SET-SCEN` | — | `RELEASE-V01`. Già **giudicata ❌** con evidenza estesa (log, `runId`, tre riaperture, verdetti d'autore verbatim). `WBP_RT_EventLogRight` è un nome di istanza dentro `WBP_RT_TacticalHUD`, non un package — stessa conclusione già scritta per `PIE-V01-SCREENHUD`/`PIE-V01-BLINDFIRE` in questo verbale; il montaggio è confermato (`[ok] EventLog: 1`, classe tracciata `WBP_RT_EventLog`, `oracles.asset_tracciati()` → `True`). Un check già giudicato è per definizione raggiungibile — resta ❌ finché una nuova seduta non lo rigiudica |
| `PIE-HEXPLAY-8` | `SET-SCEN` | — | `RELEASE-V01`. Già **giudicata 🟡** con verdetto esplicito su entrambe le metà: la salita è ✅ (quota `z=140,0`, `LayerHeight` esatto, misurata il 2026-09-04), il crollo del ponte è **dichiarato NON OSSERVABILE in v0.1** — non in attesa di seduta — perché `Action.ModifyArc` resta senza esecutore per decisione (`D-046`); lo scenario `Spec.Map.BridgeBreaksThePath` (tracciato, `Scenarios/Spec/Map/BridgeBreaksThePath.json`) è stato corretto il 2026-09-08 per dichiararsi `BLOCKED` invece di passare a vuoto. Nessun asset mancante, nessun blocco d'accesso |
| `PIE-ICON-02` | `SET-SCEN` | — | `DA_IconCatalog` tracciato e a 64 chiavi (`#2253` mergiata); scenario `Visual.Combat.UnbalancedAmplifiesPush` tracciato (`Scenarios/Visual/Combat/UnbalancedAmplifiesPush.json`) per produrre entrambe le icone nello stesso fotogramma; resta solo il giudizio a schermo da vicino |
| `PIE-KNOW1` | `SET-SCEN` | — | scenario `Visual.Perception.Acceptance` tracciato (`Scenarios/Visual/Perception/Acceptance.json`), via alternativa `-dpcvars` documentata e verificata funzionante su un altro banco dello stesso corpus (`Visual.Environment.FireOnEnter`, banner `PASS` osservato); `ARTHUD::DrawHUD` esiste in codice — nessun asset mancante |
| `PIE-KNOW2` | `SET-SCEN` | — | stesso allestimento di `PIE-KNOW1`; `ARTUnit::RefreshComponentVisibility` cablata da `ARTHUD::DrawHUD` e coperta headless da `Unit.RediscoveryDoesNotSelect` — nessun asset mancante, resta il cablaggio del ciclo di disegno da giudicare a schermo |
| `PIE-KNOW3` | `SET-SCEN` | — | stesso allestimento; `M_LastContactGhost` **tracciato** (`Content/RT/Characters/Shared/Materials/M_LastContactGhost.uasset`, `oracles.asset_tracciati()` → `True`) e `ContactGhostMaterial` è un campo opzionale già letto (`Source/RefactorTactics/Unit/RTUnit.cpp:980-982`); nessuna riga dichiara il materiale non assegnato sui `BP_Unit_*` — nessun `requires` verificabile |
| `PIE-KNOW4` | `SET-SCEN` | — | stesso allestimento; il comportamento atteso (`D-223`, `#1497`) è già atterrato, coperto headless da `Knowledge.OverlayAndGhostAreComplementary`; il comando di verifica `rt.Debug.DrawPaths 0` esiste — nessun asset mancante |
| `PIE-KNOW5` | `SET-SCEN` | — | precondizione «`#1525` in main» verificata: PR #1875 (`fix/1525-tronca-playback-al-tratto-osservato`) mergiata, commit `42f33ba2` in storia (`git log --oneline --all \| grep 1525`); nessun asset mancante, il comando `rt.Debug.DrawPaths 0` è lo stesso di `PIE-KNOW4` |
| `PIE-SCEN-PLAYBACK` | `SET-SCEN` | — | tab Tactical Designer eseguibile dal 2026-09-04 (`URTScenarioAuthoring::Run`, prima solo negli automation test); catena headless completa (trasporto #2095, stato da traccia #2173, identità id #2176, traduzione viste #2185); due difetti già trovati e corretti via MCP (#2224); nessun asset o widget mancante, resta solo il giudizio di leggibilità |
| `PIE-V01-DOCKCLICK` | `SET-SCEN` | — | owner [#2826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826) scope 1; scenario `Visual.Hud.FirstPlayable` tracciato (`Scenarios/Visual/Hud/FirstPlayable.json`); `WBP_RT_ActionDock`/`WBP_RT_ActionSlot` entrambi tracciati (`oracles.asset_tracciati()` → `True`); il caso fail-closed (slot vuoto) è dichiarato `N/A` **nella riga stessa** per assenza di kit col buco — non un `requires`, è un criterio già circoscritto dalla voce |
| `PIE-V01-DOCKKEYS` | `SET-SCEN` | — | owner #2826 scope 3, stesso allestimento di `PIE-V01-DOCKCLICK`; il caso decimo-slot esiste solo su Muiren (10 voci nel roster tracciato) — precondizione già circoscritta nella riga, non un asset mancante; il fix del `10`→`0` (#2987) è già atterrato |
| `PIE-V01-DOCKPROMPT` | `SET-SCEN` | — | owner #2826 scope 5, stesso allestimento; il cablaggio del dock (PR #2933) è mergiato, i cinque `HudViewModel.TargetPrompt*` sono verdi — nessun asset o widget mancante, resta il giudizio a schermo delle tre frasi |
| `PIE-V01-DOCKREBUILD` | `SET-SCEN` | — | origine [#2989](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2989); stesso allestimento, con due unità di lunghezza kit diversa (Muiren 10, Aevik/Branth/Ivrin 9) già verificate nel roster tracciato; la riga stessa dichiara **coperta una sola metà** (il caso fra mouse-down e mouse-up è dichiarato non riproducibile a gesto, nessun input di selezione alternativo esiste — `CycleUnit`/`SelectNextUnit` danno zero in `Source/`) — non un `requires`, è un limite già circoscritto della voce |
| `PIE-V01-DOCKTICK` | `SET-SCEN` | — | origine #2989, stesso allestimento di `PIE-V01-DOCKCLICK`; si giudica dal log di output, nessun asset o widget mancante — il difetto noto (#2963, 16388 righe per 4 chiavi) è già registrato come tale e non impedisce l'accesso |
| `PIE-V01-INSPECT` | `SET-SCEN` | — | owner #705 (gesto) e #613 (pannello), atterrati con PR #3014 (`issue/705-resolve-outcome`, commit `f050b3f2` confermato in storia); stesso allestimento delle tre sorelle del dock; i quattro test citati sono verdi e validati per mutazione — nessun asset mancante |
| `PIE-V01-PACKAGED` | `SET-SCEN` | — | già **ESEGUITA il 2026-09-04** (#959), due volte (PIE e packaged Development), con evidenza puntuale (correlazione al secondo fra scatto e log, banner AUTOBATTLE a schermo, materiale in `Saved/CP476/` e `Saved/CP476-PIE/`); i quattro blocchi storici (#1088, #1069, #79 fetta A, #1296) sono tutti chiusi — la voce è dimostrabilmente raggiungibile e già giudicata |
| `PIE-V01-POINTER` | `SET-SCEN` | — | owner `spec-pointer-interaction.md`, CP 11.8; precondizione procedurale (partita in PIE, unità propria selezionata); nessun asset o widget nominato mancante |
| `PIE-V01-PREPWINDOW` | `SET-SCEN` | — | condivide l'allestimento di `PIE-V01-PACKAGED` (`rt.Match.Autobattle=1`, `E47.1` già sbloccata); gli otto Automation Test citati sono verdi; `PrepWindowSeconds` di default è 3s — nessun asset mancante, mai eseguita ma non bloccata |
| `PIE-V01-SPECTATOR-ROSTER` | `SET-SCEN` | — | precondizione `WBP_RT_TacticalHUD` montato — tracciato (`oracles.asset_tracciati()` → `True`); stesso allestimento di `PIE-V01-PACKAGED`. ⚠️ **La riga di registro è stale sul criterio (d)**: dice «`WBP_RT_EventLog` non è in `Content/`», ma `Content/RT/UI/Match/WBP_RT_EventLog.uasset` **è tracciato** (`oracles.asset_tracciati()` → `True`) — (d) non è quindi `N/A` per asset assente. Questo toglie un vincolo, non ne aggiunge uno: nessun `requires` |
| `PIE-VELO-VIEWER` | `SET-SCEN` | — | eseguibile dal 2026-08-29 (`#1535`, commit `a5283719` confermato in storia); `ARTGameMode::ApplyKnowledgeVeilForViewer()` sottoscrive `OnTeamKnowledgeRefreshed`, coperto headless da `RTVeilTests.cpp` — nessun asset mancante |
| `PIE-VIS-AREAGUARD` | `SET-SCEN` | — | scenario `Visual.Combat.AreaGuardFromImpactCenter` tracciato (`Scenarios/Visual/Combat/AreaGuardFromImpactCenter.json`); logica coperta da due test C++ e due scenari `Spec.*`; nata `NOT RUN` per assenza di riga nel corpus (#2062), non per un asset mancante |
| `PIE-VIS-BRACE` | `SET-SCEN` | — | scenario `Visual.Combat.BraceReducesEveryHit` tracciato (`Scenarios/Visual/Combat/BraceReducesEveryHit.json`); descrive il comportamento **attuale** di `Action.Brace` (-10 a ogni colpo, `RTCatalogLibrary.cpp:1493-1499`) — non ancora toccato da D-204/D-208 (vedi `PIE-BAL1`), quindi lo scenario è valido oggi |
| `PIE-VIS-CHARGE` | `SET-SCEN` | — | scenario `Visual.Movement.Charge` tracciato (`Scenarios/Visual/Movement/Charge.json`); nessun asset mancante |
| `PIE-VIS-COMBO` | `SET-SCEN` | — | scenario `Visual.Combat.WaterElectric` tracciato (`Scenarios/Visual/Combat/WaterElectric.json`); nessun asset mancante |
| `PIE-VIS-COORD` | `SET-SCEN` | — | scenario `Visual.Combat.WaterElectricCoordinated` tracciato (`Scenarios/Visual/Combat/WaterElectricCoordinated.json`); nessun asset mancante |
| `PIE-VIS-COVER` | `SET-SCEN` | — | scenario `Visual.Map.LowCoverEdge` tracciato (`Scenarios/Visual/Map/LowCoverEdge.json`); nessun asset mancante |
| `PIE-VIS-DOOR` | `SET-SCEN` | — | scenario `Visual.Map.ClosedDoor` tracciato (`Scenarios/Visual/Map/ClosedDoor.json`); nessun asset mancante |
| `PIE-VIS-FALLBACK` | `SET-SCEN` | — | scenario `Visual.Combat.FallbackTargetMoved` tracciato (`Scenarios/Visual/Combat/FallbackTargetMoved.json`); nessun asset mancante |
| `PIE-VIS-GUARD` | `SET-SCEN` | — | scenario `Visual.Combat.GuardReducesFirstHit` tracciato (`Scenarios/Visual/Combat/GuardReducesFirstHit.json`); descrive il comportamento attuale di `Action.Guard` (pool 15, `RTCatalogLibrary.cpp:1096-1106`, D-292+D-206 già atterrate) — nessun asset mancante |
| `PIE-VIS-HIGH` | `SET-SCEN` | — | scenario `Visual.Map.HighGroundNoBonus` tracciato (`Scenarios/Visual/Map/HighGroundNoBonus.json`); nessun asset mancante |
| `PIE-VIS-HIGHCOVER` | `SET-SCEN` | — | scenario `Visual.Map.HighCoverBlocks` tracciato (`Scenarios/Visual/Map/HighCoverBlocks.json`, fixture `CoverYard`); nessun asset mancante |
| `PIE-VIS-ICE` | `SET-SCEN` | — | scenario `Visual.Environment.IceSlide` tracciato (`Scenarios/Visual/Environment/IceSlide.json`); nessun asset mancante |
| `PIE-VIS-KO` | `SET-SCEN` | — | scenario `Visual.Combat.Defeat` tracciato (`Scenarios/Visual/Combat/Defeat.json`), già eseguito headless il 2026-08-31 (`PASS 4/4`, `turnsPlayed: 6`); nessun asset mancante, resta il giudizio sul tempo della sparizione |
| `PIE-VIS-LEVEL` | `SET-SCEN` | — | scenario `Visual.Map.MultiLevel` tracciato (`Scenarios/Visual/Map/MultiLevel.json`); nessun asset mancante |
| `PIE-VIS-PHASES` | `SET-SCEN` | — | scenario `Visual.Core.PhaseOrder` tracciato (`Scenarios/Visual/Core/PhaseOrder.json`), già eseguito headless il 2026-08-31 (`PASS 3/3`); nessun asset mancante |
| `PIE-VIS-PRONE` | `SET-SCEN` | — | scenario `Visual.Movement.ProneStandUpCosts` tracciato (`Scenarios/Visual/Movement/ProneStandUpCosts.json`); la regola numerica è coperta headless (`Status.StandUpCostsOneMovePoint`); nessun asset mancante |
| `PIE-VIS-ROUGH` | `SET-SCEN` | — | scenario `Visual.Movement.RoughRefusesCharge` tracciato (`Scenarios/Visual/Movement/RoughRefusesCharge.json`); nessun asset mancante |
| `PIE-VIS-SIGHTLINE` | `SET-SCEN` | — | scenario `Visual.Environment.BlindFireThroughSightWall` tracciato (`Scenarios/Visual/Environment/BlindFireThroughSightWall.json`), nuovo il 2026-09-11; nessun asset mancante — mai eseguita, non bloccata |
| `PIE-VIS-SIGHTWALL` | `SET-SCEN` | — | già **giudicata ❌**, con quattro riaperture e verdetti d'autore verbatim registrati; scenario `Visual.Map.SightWallIsWalkable` tracciato (`Scenarios/Visual/Map/SightWallIsWalkable.json`) — un check già giudicato è per definizione raggiungibile |
| `PIE-VIS-SLIDESTATE` | `SET-SCEN` | — | stesso banco di `PIE-VIS-ICE` (`Visual.Environment.IceSlide`, stesso file tracciato); nessuno scenario nuovo da preparare — la riga stessa lo dichiara («non aggiungere uno scenario: è lo stesso file») |
| `PIE-VIS-SMOKE` | `SET-SCEN` | — | scenario `Visual.Combat.SmokeCapsTargeting` tracciato (`Scenarios/Visual/Combat/SmokeCapsTargeting.json`); nessun asset mancante |
| `PIE-VIS-UNBAL` | `SET-SCEN` | — | scenario `Visual.Combat.UnbalancedAmplifiesPush` tracciato (`Scenarios/Visual/Combat/UnbalancedAmplifiesPush.json`, lo stesso di `PIE-ICON-02`); nessun asset mancante |
| `PIE-VIS-WETFIRE` | `SET-SCEN` | — | scenario `Visual.Environment.WetExtinguishesFire` tracciato (`Scenarios/Visual/Environment/WetExtinguishesFire.json`); nessun asset mancante |

## Cosa questa rassegna non ha deciso

- `SET-TD` è dichiarato e nessun check lo usa. È una domanda sull'**allestimento**, non sui
  prerequisiti: resta il follow-up che §12 della spec ha aperto.
- La **coda scoperta** — i check che nessuna riga di `wiring` cabla — non è entrata nel bacino.
  Fra essi `PIE-VIS-DEFLECT` e `PIE-VIS-INTERPOSE`, che la spec cita come casi `cue` canonici:
  non si può dichiarare il prerequisito di un check che non ha ancora un allestimento.
- **Con questo lotto (`SET-SCEN`) la rassegna copre l'intero bacino**: nessun allestimento resta
  fuori da questo verbale. Il conteggio esatto — quanti check, quanti `requires` — si rimisura
  con `python tools/editor-sessions/compare_rassegna.py`, non si ripete qui in prosa perché
  invecchierebbe.

## Dubbi sopravvissuti alla rassegna

- **`PIE-V01-COVEREDIT`** (`SET-SANDBOX`): il package `DA_HexMap_Sandbox` esiste ma è vuoto
  (0 celle). Il criterio che questo check giudica — copertura bassa su un bordo esposto,
  insieme a `Terrain.Rough`, acqua e porta — è dichiarato **fuori scope** dal `done_when` di
  `U13` (`docs/roadmap/editor-sessions.yaml:1183-1191`): `#1738` lo mette nel proprio *Out of
  Scope* e la riga dice apertamente **«oggi non hanno una issue propria»**. Questo non è un
  `asset:` mancante (il package c'è, solo vuoto), e non è nessuno dei quattro tipi con issue:
  tutti e quattro pretendono un `#numero`, e qui non esiste un'issue che copra proprio questo
  gap — dichiararne uno significherebbe inventare un bloccante che nessun owner può chiudere.
  **Correzione rispetto alla prova scritta nel primo giro di questo lotto**: avevo indicato
  `Generate Into Asset` come rimedio disponibile nella stessa apertura. Non lo è —
  `ARTHexMapActor::GenerateIntoAsset()` (`Source/RefactorTactics/Map/RTHexMapActor.cpp`, riga
  2264 per `grep -n 'void ARTHexMapActor::GenerateIntoAsset'`) **sostituisce** il contenuto con
  celle native generate, non ricostruisce i dati pre-migrazione che `U13` descrive da autorare
  a mano (`Terrain.Rough`, coperture, porta). E su un asset vuoto il criterio della voce non è
  nemmeno falsificabile: `Source/RefactorTactics/Tests/RTHexMapTests.cpp:1016` annota che
  «migra senza perdere nulla» è banalmente vero del nulla. **La decisione resta `—`**, ma per
  l'unica ragione onesta disponibile: lo schema dei `requires` non ha un token per un gap che
  il repository stesso dichiara privo di issue owner — non perché il check sia oggi pienamente
  giudicabile su questo asse.
- **`PIE-BAL1`** (`SET-SCEN`): la voce chiede se `Guard` e `Brace` si leggano come scelte diverse
  «ora che hanno lo stesso prezzo» — e oggi non ce l'hanno. Verificato in
  `Source/RefactorTactics/Ability/RTCatalogLibrary.cpp`: `Action.Brace` applica ancora
  `Status.Braced` + `Status.Root` (righe 1493-1499, il comportamento pre-`D-204`), `Action.Guard`
  non si auto-infligge `Status.Slow` (righe 1096-1106) — `D-205` e `D-208`
  (`docs/decisions/RT_PDR_00_Decision_Log.md:218,221`, «Consolidata» il 2026-08-27) non sono
  atterrate in codice. La riga di registro lo dice in chiaro: *«non eseguibile finché D-204…D-208
  non sono implementate»*, e concede un solo uso legittimo oggi — una corsa **baseline** sulla
  coppia vecchia, registrata come tale e mai come risposta a `BAL-1`. Non è quindi un `asset:`
  mancante (lo scenario `Spec.Brace.GuardAndBraceOnMixedHit` è tracciato ed eseguibile), ed è un
  caso che sembra fatto apposta per `feature:` — ma l'unica issue trovata per questa domanda,
  [#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403), non è un owner
  esterno: `docs/OPEN_DECISIONS.md:1710` la nomina esplicitamente come *«il gate umano, non la
  scelta»* di **questa stessa voce** — chiude quando `U20`/`PIE-BAL1` viene giudicata, non prima.
  Dichiararla come `requires` significherebbe bloccare il check con la propria stessa
  conclusione: un `#nnn` che nessuno può chiudere dall'esterno è esattamente il bloccante che il
  divieto 2 del brief vieta di inventare. Nessuna issue distinta traccia l'atterraggio di
  `D-205`/`D-208` nel catalogo (cercato in `docs/OPEN_DECISIONS.md`, nella roadmap
  `action-phases-economy-handoff-2026-08-26.md` e via `gh issue list` locale: nessun candidato).
  **La decisione resta `—`**, per la stessa ragione onesta di `PIE-V01-COVEREDIT`: non perché il
  check sia oggi pienamente giudicabile sulla sua domanda vera, ma perché lo schema non ha un
  token per «la decisione è presa e non ancora scritta in codice, e nessuno lo traccia
  separatamente da questo stesso gate». Non è `RELEASE-V01`, quindi non tiene nulla fuori dalla
  consegna.
