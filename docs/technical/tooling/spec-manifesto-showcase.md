# Spec — La forma in cui uno showcase si dichiara

> `CURRENT` · **Stato**: esito di **discovery**, non di implementazione · **Data**: 2026-09-23
> **Origine**: [#2745](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2745), issue di
> discovery aperta dal panel di riconciliazione dell'autobattle
> ([`../../roadmap/plans/autobattle-showcase-long-term-roadmap-2026-09-09.md`](../../roadmap/plans/autobattle-showcase-long-term-roadmap-2026-09-09.md)).
>
> **Cosa è**: l'owner di **una sola domanda** — *in quale forma uno showcase dichiara il proprio
> allestimento*, e quali seam esistenti la compongono.
> **Cosa non è**: l'owner di **quale** showcase si allestisce. Quello è
> [`../../product/showcase-v0.1.md`](../../product/showcase-v0.1.md), che possiede «Relay Basin» e non si
> tocca qui. E non è un secondo Scenario Harness: quello è
> [`scenario-index-e-tag.md`](scenario-index-e-tag.md) più `URTScenarioLoader`.
>
> **Autorità**: subordinata al [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md) e a
> [`../architecture/capability-map.yaml`](../architecture/capability-map.yaml), che su `RT-CAP-AUTOBATTLE`
> dichiara il vincolo con cui questo documento deve convivere: *«Mai una mega-epic Showcase, mai un secondo
> Scenario Harness, mai uno spettatore che allenti la privacy»*.
>
> ⛔ **Questa discovery non ha compilato, non ha aperto Unreal, non ha eseguito Automation.** Dove si legge
> *«il test esiste e asserisce X»* non si legga *«il test passa»*. I seam sono stati letti nel sorgente.
>
> 📌 **I riferimenti al codice sono per SIMBOLO, con il file e senza il numero di riga**, forma di
> [`D-431`](../../decisions/RT_PDR_00_Decision_Log.md) (1)(a). ⚠️ L'acceptance criterion di #2745 chiedeva
> *«i seam misurati con file e **riga**»*: un numero di riga in questo documento nascerebbe scaduto — è
> misurato tre volte in due settimane su
> [#3253](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3253) e
> [#3256](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3256) — e il simbolo è **più**
> preciso, non meno: si trova con un `git grep -n` che nessuno deve aggiornare.

---

## 1. Il risultato: la forma non si sceglie, è già determinata

La domanda di #2745 era: *uno showcase è **uno scenario esteso**, un **`UDataAsset` nuovo**, un **file di
config**, o una **composizione**?*

🔑 **È una composizione, e non è un arbitrato: è una conseguenza di decisioni già prese.** Tre di esse, prese
altrove e per altre ragioni, tolgono dal tavolo tutte le alternative tranne una:

| premessa già stabilita altrove | cosa esclude |
|---|---|
| `RT-CAP-AUTOBATTLE` in `capability-map.yaml` — *«mai un secondo Scenario Harness»* | **lo scenario esteso** come forma NUOVA. Non perché sia sbagliata, ma perché esiste già: estenderla è permesso, duplicarla no |
| La modalità non presidiata è già **risolta**, da `ARTGameMode::ResolveAutobattle`, con una scala di precedenza nota e un `UE_LOG(Warning)` che dichiara chi scavalca chi | **il `UDataAsset` nuovo** come sede dell'allestimento: sarebbe un **quarto gradino** in una scala che oggi ne ha tre — e che sa avvisare proprio perché la scala è nota |
| `FRTMatchBootstrapConfig` — il suo contratto scritto: *«il bootstrapper non legge console variable né riga di comando»* | **il file di config** come quinto ingresso: la separazione fra *chi risolve* e *chi allestisce* è già tracciata, e un file nuovo la attraverserebbe |

∴ **uno showcase non è un artefatto: è un insieme di ingressi esistenti, con un nome.** Ciò che manca non è
una forma nuova — è che l'insieme sia **nominabile** e **ripetibile**.

### 1.1 ✅ E il precedente esiste già, costruito senza che nessuno lo chiamasse manifesto

`Scenarios/RT_Showcase_Relay_v01.json` è una showcase composta esattamente così, ed è la prova che la forma
funziona:

- la **geometria non è nel file**: è riferita per nome (`"fixture": "RelayBasin"`) e vive in
  `MakeShowcaseRelayBasinArena`, protetta da `RefactorTactics.ShowcaseRelay.BasinLayoutMatchesSpec`. Il file
  lo dichiara da sé: *«Quarantacinque celle copiate qui sarebbero una seconda verità che nessuno confronta
  con la prima»*;
- il **seed** è dichiarato nel dato, non risolto altrove;
- la **spec** è citata nel file, e punta a `docs/product/showcase-v0.1.md`;
- 🔑 **e i turni che il gioco non sa ancora giocare esistono lo stesso, come dato**, ciascuno con la
  `requires` della capability che gli manca — *«il runner si ferma sul primo non supportato e restituisce
  `BLOCKED` con il nome, invece di `FAIL`»*.

⚠️ **Quest'ultimo punto è il contributo più importante del precedente, e la discovery non l'aveva previsto**:
è la risposta alla voce *«output diagnostico ripetibile»* della lista PROPOSTA. Uno showcase che dichiara da
sé **fin dove arriva oggi** non ha bisogno di un rapporto scritto a mano che invecchia.

---

## 2. I seam misurati

Nessuno di questi va costruito: vanno **composti**. La tabella elenca i **nomi**; un ingresso in più la
allunga senza che nessuno debba correggere un numero scritto altrove.

| cosa si configura | ingressi | chi risolve | precedenza reale, letta dal codice |
|---|---|---|---|
| modalità non presidiata | `ARTGameMode::bAutobattle` · `rt.Match.Autobattle` · `-RTAutobattle` e `-RTAutobattle=` | `ARTGameMode::ResolveAutobattle` | proprietà **<** riga di comando **<** console — e lo scavalcamento si **dichiara**, con un `UE_LOG(Warning)` |
| compagni al bot | `ARTGameMode::BotAllyCount` · `rt.Match.BotAllies` · `-RTBotAllies` e `-RTBotAllies=` | `ARTGameMode::ResolveBotAllies` | idem, stessa scala e stesso avviso |
| ritmo del Planning | `ARTGameMode::MatchPlanningSeconds` (negativo = non intervenire) · `rt.Match.PlanningSeconds` · `-RTPlanningSeconds=` | `ARTGameMode::ResolveMatchPlanningSeconds` | idem. ⚠️ **Il referto del 2026-09-09 dava questa riga come «proprietà» soltanto, e non era invecchiato: console e riga di comando erano già nel file che quel referto stava leggendo** |
| formato (round limit, soglia obiettivo) | `ARTGameMode::MatchFormat` (asset) · `ARTGameMode::ShippedFormatId` · ripiego | `URTMatchBootstrapper::ApplyMatchFormat` | asset **>** id spedito **>** ripiego. 🔑 **È la sola famiglia senza ingresso fuori dall'asset, e il suo primo gradino è FATALE**: un `MatchFormat` invalido non ripiega, restituisce `ERTStartupOutcome::FormatAssetInvalid` |
| mappa | `ARTGameMode::MapSource` · `rt.Map.Source` · `MapFixtureId` · `rt.Map.Fixture` | `URTMatchBootstrapper::ApplyMapSource` · `ARTGameMode::ResolveMapSource` | ⛔ **NON è una catena a tre.** Sono **due alternative** — fixture per nome, oppure `MapSource` — e `DemoArenaRadius` è un **parametro di un ramo**, non un gradino di ripiego. Il referto del 2026-09-09 la descriveva come *«il più specifico vince»*: descriveva una scala che non esiste |
| raggio dell'arena generata | `ARTGameMode::DemoArenaRadius` · `rt.Match.DemoArenaRadius` | `ARTGameMode::ResolveDemoArenaRadius` | console **>** proprietà |
| scenario | `ARTGameMode::ScenarioToRun` · `rt.Test.Scenario` · `-RTScenario=` · **la seduta PIE in corso** | `ARTGameMode::ResolveScenarioToRun`, che delega a `ChooseScenarioEntry` | 🔴 **Quattro sorgenti, e la quarta vince su tutte**: `URTPieSessionSubsystem::ScenarioImposedBy`. Non entrava nel referto del 2026-09-09 perché non esisteva ancora |
| coda di esecuzione | `rt.Pie.Session` · `rt.Pie.Session.Abort` · `rt.Pie.Verdict` | `URTPieSessionPlaylist` → `URTPieSessionSubsystem` | la seduta comanda finché dura, e **non** azzera la CVar di chi l'ha lanciata |
| velocità di playback | `ARTTurnManager::ViewerPlaybackSpeed` | ciclo del tasto in HUD (`ARTHUD::NextViewerPlaybackSpeed`) · `URTPlaybackLibrary::EffectivePlaybackSpeed` | ⛔ **nessun ingresso di configurazione**: si cambia solo a runtime, dall'HUD |
| playback fermo all'avvio | `rt.Debug.PlaybackStartPaused` · `rt.Debug.PlaybackControls` | console | ⚠️ **è un booleano, non una velocità** — il suo help dice *«0 = scorre subito, 1 = parte in pausa»*. Non copre la voce *«velocità iniziale»* |

### 2.1 🔑 La quarta sorgente dello scenario è una regola, non una gerarchia

`ResolveScenarioToRun` spiega perché la seduta PIE vince, e la ragione va riportata qui perché un manifesto
che la ignorasse riaprirebbe il difetto: *«una `rt.Test.Scenario` rimasta impostata da una prova precedente
dirotterebbe **in silenzio** ogni passo della playlist, e chi guarda crederebbe di giudicare la voce che il
conduttore ha appena annunciato»*.

⚠️ E la seduta **non** entra in `ChooseScenarioEntry`: *«quella funzione decide fra tre CONFIGURAZIONI e
produce l'avviso di chi scavalca chi. Una seduta non è una configurazione che scavalca — è uno stato attivo
che comanda finché dura»*.

### 2.2 L'elenco degli ingressi ha già un oracolo, e va citato invece di ricopiato

`RefactorTactics.GameMode.*` in `Tests/RTShippedGameModeTests.cpp` enumera **per riflessione** le proprietà
editabili di `ARTGameMode` e pretende che ognuna stia in un insieme **deliberato**. Il suo commento dichiara
la stessa disciplina che questo documento adotta:

> *«Non è un conteggio: un numero qui invecchierebbe da solo. È un **insieme di nomi**, e il messaggio nomina
> la proprietà scoperta invece di dire che il totale non torna.»*

∴ **una specifica del manifesto non deve portare la propria lista di proprietà**: ne avrebbe una seconda, che
diverge alla prima aggiunta senza che nessun gate lo dica. Si cita il gate.

---

## 3. La lista PROPOSTA di #2745, voce per voce

#2745 dichiarava le sue voci *«la domanda, non la risposta»*, e chiedeva che ognuna ricevesse un verdetto.

| voce | verdetto | dove |
|---|---|---|
| mappa | ✅ **già coperta** | `ApplyMapSource` — ⚠️ due alternative, non una scala (§2) |
| scenario | ✅ **già coperta** | `ResolveScenarioToRun`, quattro sorgenti |
| composizione Team A e Team B | ✅ **già coperta** | `ARTGameMode::Team0Heroes` · `Team1Heroes` · `HeroUnitClasses` |
| loadout | 🔴 **respinta, e decisa altrove**: [`D-417`](../../decisions/RT_PDR_00_Decision_Log.md) registra che **nessuna UI di loadout esiste**, e che spostare le varianti arma costruirebbe un meccanismo per una scelta che il giocatore non compie | — |
| setup obiettivo | ✅ **già coperta** | dal formato (`URTMatchFormatData`) e dallo scenario |
| round limit | ✅ **già coperta** dal **formato**, non dallo scenario | ⚠️ `FRTTestScenario` ha `MaxTurns` e `bFreeRun`, che **non** sono il round limit competitivo: `RoundLimit` non compare in `ScenarioHarness/`. Confonderli metterebbe due autorità sulla stessa regola |
| prep window | ✅ **già coperta** | `ResolveMatchPlanningSeconds` |
| velocità iniziale | ⏳ **da fare** | ⛔ `rt.Debug.PlaybackStartPaused` **non** la copre: è *se* parte fermo, non *a che velocità*. `ViewerPlaybackSpeed` non ha ingresso di configurazione |
| observer policy | 🔵 **decisa altrove, e aperta**: è `OBS-1` in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) | — |
| camera policy | 🔵 **decisa altrove**: è [#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781) (`CAM-12`), post-v0.1 | — |
| auto-start | ✅ **già coperta** | `bAutobattle` + la seduta PIE |
| auto-restart | ⏳ **da fare** | nessun ingresso: una seduta esegue la propria coda e termina |
| match singolo o playlist | ✅ **già coperta** | `URTPieSessionPlaylist` → `FRTPieSessionPlan` |
| configurazione fuori dall'Editor | ✅ **già coperta** | i token `-RT…` di riga di comando |
| stesso setup in PIE / Standalone / packaged | ⚠️ **parzialmente** | i token di riga di comando valgono ovunque; `rt.Pie.Session` è per costruzione una **seduta PIE**. È il residuo `SHOW-2` |
| output diagnostico ripetibile | ✅ **già coperta, e meglio di come la voce la chiedeva** | il `requires` → `BLOCKED` di `RT_Showcase_Relay_v01.json`, più `FRTPieSessionPlan::Excluded`, che stampa ciò che resta fuori invece di ignorarlo |
| registrazione video | ✅ **già coperta** | [#959](https://github.com/DegrassiAaron/refactor-tactics-main/issues/959), chiusa |
| seed | 🔵 **decisa altrove**: `RNG-1`/`RNG-2` sono **chiuse** dal 2026-08-30 con [`D-263`](../../decisions/RT_PDR_00_Decision_Log.md) — nessuna RNG in gameplay, `Seed` riservato e non consumato | — |

---

## 4. L'alternativa scartata, con la sua ragione

**Un `URTShowcaseManifest : UDataAsset` che dichiari tutto l'allestimento in un solo posto.** È la forma più
attraente, ed è quella che #2745 sembrava chiedere.

⛔ **Va scartata per una ragione misurata, non per prudenza**: un asset che dichiarasse mappa, scenario,
squadre, formato e ritmo diventerebbe un **quarto gradino** in cinque scale di precedenza che oggi ne hanno
tre, e lo diventerebbe in silenzio — `ResolveAutobattle` e le sue sorelle **avvisano** quando qualcuno
scavalca qualcun altro, e quell'avviso è costruito su una scala nota. Aggiungere un gradino senza costruirgli
l'avviso produrrebbe esattamente il difetto che `ResolveScenarioToRun` descrive: *«chi guarda crederebbe di
giudicare la voce che il conduttore ha appena annunciato»*.

🔑 **E il costo non sarebbe simmetrico**: l'asset porterebbe un vantaggio di *authoring* — un posto solo da
aprire — e un costo di *risoluzione* — cinque scale da riscrivere e cinque avvisi da ricostruire. Lo scenario
JSON offre lo stesso vantaggio di authoring **senza** quel costo, perché non entra nelle scale: viene
**imposto** dalla seduta, che è uno stato attivo e non un gradino.

✅ **Quando riaprirla, dichiarato in anticipo**: il giorno in cui un allestimento debba essere selezionabile
**dal gioco spedito** e non dall'Editor o dalla riga di comando. Allora l'asset avrebbe un consumatore che
oggi non ha, e il costo delle cinque scale sarebbe pagato da qualcosa.

---

## 5. Non-duplicazione: il confine, campo per campo

L'acceptance criterion di #2745 chiede che *«nessuna voce della specifica duplichi `RTScenarioLoader` o
`FRTMatchBootstrapConfig`»*. Il confine è netto e passa fra **cosa si allestisce** e **come si risolve**:

| | possiede | non possiede |
|---|---|---|
| `FRTTestScenario` (`ScenarioHarness/`) | la **partita scriptata**: celle, muri, porte, unità, turni, attese, varianti, `fixture`, `seed` | il **formato competitivo**. `RoundLimit` e `FRTMatchRules` non compaiono in `ScenarioHarness/` |
| `FRTMatchBootstrapConfig` (`Match/`) | l'**allestimento**: sorgente mappa, fixture, raggio, formato, id spedito | gli **ingressi**. Il suo contratto lo dichiara: *«il bootstrapper non legge console variable né riga di comando»* |
| `ARTGameMode` | la **risoluzione**: leggere le sorgenti, applicare la precedenza, avvisare chi scavalca | il **dato**. Non descrive una partita |

∴ **un manifesto non aggiunge un quarto contenitore: nomina una combinazione dei tre.** È la ragione per cui
questo documento non propone struct nuove.

---

## 6. Ciò che resta davvero aperto

Registrato in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), nella sezione della riconciliazione
autobattle del 2026-09-09, accanto a `OBS-1`:

- **`SHOW-1`** — il **nome** di un allestimento: uno showcase si nomina con lo `scenarioId` dello scenario
  che lo impone, oppure serve un identificatore che copra anche gli ingressi che lo scenario non porta
  (autobattle, compagni al bot, ritmo del Planning)?
- **`SHOW-2`** — la **parità fra contesti**: `rt.Pie.Session` è per costruzione una seduta PIE, e i token
  `-RT…` valgono ovunque. Serve un conduttore fuori dall'Editor, o il packaged si accontenta della riga di
  comando?

⛔ **Questa discovery non le chiude.** Entrambe sono scelte d'autore con conseguenze su ciò che si spedisce,
e nessuna delle due si deduce dal codice: sono la forma di domanda che
[`CLAUDE.md`](../../../CLAUDE.md) §13 classifica come `BLOCKED — DECISION REQUIRED`.

---

## 7. Cosa questa discovery NON decide

- ⛔ **Non apre un'epic Showcase.** `RT-CAP-AUTOBATTLE` lo vieta per nome;
- ⛔ **Non scrive codice.** #2745 è una issue di discovery, e il suo prodotto è questo documento;
- ⛔ **Non tocca `docs/product/showcase-v0.1.md`**, che resta l'owner di *quale* showcase si allestisce;
- ⛔ **Non decide la observer policy** (`OBS-1`) né la camera director (#1781);
- ⛔ **Non promuove nessun numero di bilanciamento.**

---

## 8. Follow-up candidates

- 🔴 **Un'attribuzione sbagliata in [`../architecture/capability-map.yaml`](../architecture/capability-map.yaml),
  trovata scrivendo questa spec e per questo NON usata qui.** Il rischio dichiarato su `RT-CAP-AUTOBATTLE`
  scrive *«l'autorizzazione e' un DATO: `IsUnattendedSession()`, D-242»*. Misurato il 2026-09-23:
  [`D-242`](../../decisions/RT_PDR_00_Decision_Log.md) parla della **vista che il velo disegna** e non
  nomina la sessione non presidiata — `grep -oc` di `presidiat` e di `Unattended` sulla sua riga risponde
  **zero**; e il docstring di `ARTTurnManager::SetUnattendedSession` dichiara di sé *«non tocca il resolver:
  e' telemetria»*. ⚠️ **È una citazione che sembra fresca e manda nel posto sbagliato** — la stessa classe
  di difetto che [#3256](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3256) chiude sul
  Decision Log. La correzione appartiene a chi possiede la capability map, e va fatta **leggendo** `D-242`,
  non ripuntandola;
- ⏳ **`ARTTurnManager::ViewerPlaybackSpeed` non ha ingresso di configurazione**: è l'unica voce della
  lista PROPOSTA classificata *da fare* che non dipenda da `SHOW-1` o `SHOW-2`. Una console variable
  starebbe accanto a `rt.Debug.PlaybackStartPaused` senza toccare nessuna scala di precedenza;
- ⏳ **auto-restart**: una seduta esegue la propria coda e termina. Chi volesse un ciclo continuo oggi non
  ha dove dichiararlo.
