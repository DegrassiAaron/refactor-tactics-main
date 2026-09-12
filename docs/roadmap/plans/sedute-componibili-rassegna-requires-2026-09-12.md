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

<!-- lotti successivi: SET-HEX-MATCH, SET-HEX-BOT, SET-SCEN -->

## Cosa questa rassegna non ha deciso

- `SET-TD` è dichiarato e nessun check lo usa. È una domanda sull'**allestimento**, non sui
  prerequisiti: resta il follow-up che §12 della spec ha aperto.
- La **coda scoperta** — i check che nessuna riga di `wiring` cabla — non è entrata nel bacino.
  Fra essi `PIE-VIS-DEFLECT` e `PIE-VIS-INTERPOSE`, che la spec cita come casi `cue` canonici:
  non si può dichiarare il prerequisito di un check che non ha ancora un allestimento.
- I restanti check del bacino (`SET-HEX-MATCH`, `SET-HEX-BOT`, `SET-SCEN`) appartengono ai
  lotti successivi di questo stesso Task 6, non a questa fetta.

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
