# Mandato di integrazione v0.1 — riscrittura del prompt «DIR-A» · spec-panel

> `CURRENT` · **2026-08-28** · Panel `/sc:spec-panel` in modalità `critique`
> **Cosa è**: la riscrittura eseguibile di un mandato operativo («DIR-A · MAIN / EDITOR / UI /
> INTEGRATION v0.1») le cui premesse portanti erano false su HEAD. Conserva le sette prescrizioni corrette
> dell'originale e sostituisce quelle basate su strutture rimosse per decisione.
> **Cosa non è**: un owner. Lo stato della consegna si legge in
> [`../v0.1-definition-of-done.md`](../v0.1-definition-of-done.md); le decisioni nel
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md); il lavoro nelle issue.

**Base di misura**: `f5b4af6f` (branch `feat/1499-soggetto-esplicito-verdetto-congelato`), `origin/main` =
`01aac418` dopo `git fetch --prune`. ⚠️ Il branch di misura è **57 avanti / 204 dietro** `origin/main`:
ogni conteggio qui sotto va rifatto su `origin/main` prima di essere usato per decidere.

---

## 0. Perché il mandato originale non era eseguibile

Tre strutture su cui il prompt poggiava sono state **rimosse per decisione d'autore**, e il prompt le usava
come impalcatura invece che come ipotesi da verificare.

| # | Il prompt assumeva | HEAD | Fonte |
|---|---|---|---|
| **P1** | Tre worktree paralleli — `DIR-A` integra, `DIR-B` fa il core, `DIR-C` la QA | **Lo sviluppo è sequenziale**: una sessione, una working directory, un branch alla volta. `parallel-batch.yaml` e il suo tooling sono usciti — 8720 righe | [**D-178**](../../decisions/RT_PDR_00_Decision_Log.md) *(2026-08-20)* · [`CLAUDE.md`](../../../CLAUDE.md) §4 |
| **P2** | «Applicare le regole correnti di Binary Asset Lease» | La Lease **non serve più**: non c'è nessuno con cui contendere. Resta il fatto fisico — due `.uasset` non si fondono — e la regola che ne discende | [**D-178**](../../decisions/RT_PDR_00_Decision_Log.md), coda |
| **P3** | Gate eseguibili da script | `scripts/` **non esiste**, `.github/workflows/` **non esiste**, per scelta. Ogni gate è un atto umano | [**D-181**](../../decisions/RT_PDR_00_Decision_Log.md) · [**D-182**](../../decisions/RT_PDR_00_Decision_Log.md) *(2026-08-21)* |

∴ le sezioni §6, §22 e §23 del prompt — l'intero protocollo di handoff fra directory — **non hanno una
controparte**, e §1 vale solo per metà. Sono circa un quarto del mandato, e qui non vengono riscritte:
vengono rimosse.

⚠️ **La rimozione non è una confutazione.** D-178 dichiara che il problema che il lavoro parallelo
affrontava era reale: cambia il regime di lavoro, non la fisica del merge. Se un giorno tornassero più
sessioni contemporanee, il rationale è leggibile in D-135 e D-139 e non va riscoperto da zero.

---

## 1. Regime di lavoro

Una sessione esecutiva, una working directory, un branch alla volta. Un task troppo grosso **si spezza in
issue che si fanno in fila**, non in sessioni che convivono.

- I `.uasset` / `.umap` si modificano **attraverso Unreal**, mai a mano, **da un lavoro solo per volta** —
  non per protocollo, ma perché due binari non si fondono.
- L'evidenza di un difetto trovato durante l'integrazione atterra su una **issue**, che è l'unica casella
  di posta rimasta. Non esiste un destinatario `DIR-B` / `DIR-C` a cui restituirla.
- `D-nnn` si legge dall'ultimo assegnato nel Decision Log — oggi **D-223** — e si **riverifica prima del
  merge** con `git fetch --prune origin` e `gh pr list --state open`.

---

## 2. Ordine delle fonti

```text
HEAD corrente  →  codice/test/dati  →  issue/PR live  →  ADR / Decision Log
→  DoD v0.1 / roadmap / checkpoint  →  storico  →  questo documento
```

Un `.pdf` non è mai autoritativo (**D-009**). `docs/research/` è input non ancora consumato.
`docs/archive/` è storico. **Questo documento sta in fondo**: è un piano, non un owner.

🔴 **Vocabolario ritirato da non usare**: `RELEASE_READY`, `DONE`, `last_verified` erano campi del Feature
Registry, uscito con **D-181**. Uno snapshot che dice «Planning HUD RELEASE_READY» sta citando un sistema
che non esiste: non c'è più un posto in cui quello stato si scriva.

---

## 3. Preflight

```sh
git status --short
git branch --show-current
git fetch --prune origin
git rev-parse HEAD && git rev-parse origin/main
gh pr list --state open
```

Prima di creare un asset, un widget o una mappa: **SEARCH → REUSE → UPDATE → CREATE**, e il §4 qui sotto è
il risultato di quella ricerca alla data di questo documento.

---

## 4. Inventario misurato — cosa esiste davvero

### 4.1 Il contratto UI del prompt §8 è già implementato

`Runtime → ViewModel sanitizzato → UMG` non è un requisito da introdurre: è il codice attuale.

```text
Source/RefactorTactics/UI/RTHudViewModel.{h,cpp}
Source/RefactorTactics/UI/RTIconLibrary.{h,cpp} + RTIconCatalogData.h   ← l'Icon Catalog
Source/RefactorTactics/Frontend/RTMatchResultViewModel.h
Source/RefactorTactics/Replay/RTReplayViewModel.{h,cpp}
Source/RefactorTactics/UI/RTScreenHudWidgets.h  → 7 classi base UMG
```

### 4.2 Widget versionati: **15** `WBP_RT_*` — e la distribuzione è la mappa dei buchi

| Presenti | Assenti — nessun `.uasset` |
|---|---|
| **Framework (8)** · `FrontendRoot` `MainMenu` `MenuEntry` `ModalLayer` `LoadingScreen` `ErrorModal` `SettingsPanel` `FallbackBanner` | **Reaction Window** (§7) |
| **Match (7)** · `TacticalHUD` `TurnHeader` `TeamRoster` `SelectedUnitPanel` `ActionDock` `ActionSlot` `UnitCard` | **Combat Log** — esistono solo `RTCombatLogTests.cpp` |
| | **World Overlay / Ghost** |
| | **Replay UI** — c'è il ViewModel, non il widget |

### 4.3 Mappe versionate: **4** `.umap`

`L_DevSandbox` · `L_HexArena` · `L_Prototype` · `L_Frontend`.

⛔ **`RelayBasin` non è una mappa**: è una fixture C++ (`MakeShowcaseRelayBasinArena` in
`RTMatchSetupLibrary.cpp`), pinnata cella per cella da
`RefactorTactics.ShowcaseRelay.BasinLayoutMatchesSpec`. Cercarla come `.umap` porta a concludere che manchi.

### 4.4 Il catalogo azioni regge, ma lo slot non è la fase

Le sette azioni generiche di [**D-025**](../../decisions/RT_PDR_00_Decision_Log.md) esistono tutte:
`Action.Move` · `BasicAttack` · `Overwatch` · `Guard` · `Brace` · `Wait` · `Interact`.

⚠️ Il catalogo live ne contiene molte altre — `Sprint` `Dash` `Charge` `Counter` `Intercept` `Push`
`HeavyAttack` `ModifyArc` `CreateCover` `Ignite` `Deflect` `Interrupt` `Electrify` `CreateWater` — e
[`CLAUDE.md`](../../../CLAUDE.md) §2 avverte che **`Sprint` è un profilo di `Move`, non di `Dash`**. Una
grammatica dell'Action Dock a tre slot (`Movement` / `Main` / `Reaction`) non può derivare lo slot dalla
macro-fase: sono due assi.

### 4.5 La certezza esiste, e dichiararla mancante è un errore già registrato

`ERTIntentCertainty` (`Source/RefactorTactics/Turn/RTIntentPrivacyLibrary.h:21`) ·
`ERTTargetKnowledge` · `ERTKnowledgeVisibility`, più `RTIntentPrivacyLibrary` e i suoi test.

🔴 La nota di `G9` del **2026-08-24** ha già registrato questo identico difetto: *«tre di quelle quattro
portavano note false, tutte nella stessa direzione — dichiaravano mancante ciò che nel frattempo era stato
costruito: i tre livelli di certezza "che non esistono ancora" esistono»*. Un mandato che li rimette fra le
cose da fare fa rifare un lavoro chiuso.

---

## 5. Il verdetto di release si legge, non si inventa

La DoD **non è una lista di gate**: ne ha tre livelli, e confonderli produce un verdetto senza significato.

| Livello | Contenuto | Conteggio |
|---|---|--:|
| [§1 DoD trasversale](../v0.1-definition-of-done.md) | Regole per **ogni PR** | **8 vive** (la 9ª ritirata da D-181) |
| [§2 DoD per epic](../v0.1-definition-of-done.md) | Gate di chiusura `E1…E17` | **16 gate + 1 misura** (`E17` 4v4 dichiara di non essere un gate) |
| [§3 Gate di release](../v0.1-definition-of-done.md) | `G1…G15` | **14 vivi** — `G15` ⌫ ritirato, il numero **non si riusa** |

**Stato letto il 2026-08-28 dalla tabella datata 2026-08-24** — *letto, non rimisurato*:

| ✅ verdi (6) | 🟡 parziali (4) | ⏳ aperti (3) |
|---|---|---|
| `G1` build ×3 · `G3` 10/10 test · `G4` determinismo · `G5` no-quadrato · `G6` id unici · `G8` privacy intento | `G2` Editor 1156/1156 verde, **packaged ⏳** · `G7` validator verde, revisione `.uasset` non fatta · `G9` 10 verdi / 5 parziali / 2 aperte su **17** voci `RELEASE-V01` · `G13` packaged gira, ma su `GeneratedTestArena` | `G10` partita completa registrata · `G11` KPI · `G14` documentazione allineata |

🔴 **E c'è un blocker fuori dallo scope di questo mandato, che nessun lavoro di HUD, Reaction o Frontend
scioglie.** La §4 della DoD lo dichiara:

> *«`CP 12.4` è dipendenza di `CP 12.5`, il gate di consegna della v0.1. Quindi **la v0.1 non si chiude
> prima di E21**»*

`Client FPS` e `Durata playback per round` sono marcati **⛔ e non ⏳** — «⏳ suggerisce "presto" mentre
queste aspettano un'epic intera»: dipendono da mesh e animazioni che non esistono, perché le unità sono
**cilindri**. Lo stesso fatto blocca `PIE-FACING-1`: *«su un cilindro "la mesh guarda dove guarda la regola"
non è una domanda ponibile»*.

∴ **il verdetto onesto di questo mandato non è `READY` / `NOT_READY`**, ma: *quali dei 14 gate di release
questo lavoro sposta, e quali restano fuori dalla sua portata.*

---

## 6. Priorità 1 — HUD di partita · sede: [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613)

CP 11.7 «Screen HUD in UMG» è **aperta** ed è la sede. Il piano d'implementazione esiste già:
[`screen-hud-umg-2026-08-26.md`](screen-hud-umg-2026-08-26.md) — che dichiara quali task sono C++ e quali
sono **lavoro d'Editor che nessun agente può fare**. Non se ne apre un secondo.

Le classi base C++ e le viste sanitizzate esistono; manca l'anello *letto*: il layer HUD e i Blueprint che
derivano dalle classi base e **si limitano a disporre e a legare**.

### Criterio di accettazione — sostituisce «il giocatore deve capire»

Un requisito di leggibilità scritto in aggettivi non si valida. La forma verificabile è quella delle voci
`PIE-*` di [`test-manuali-pie.md`](../../technical/test-manuali-pie.md), che ne è l'owner:

```gherkin
Given  L_HexArena, 2v2 offline vs bot, tutti i rt.Debug.* OFF, 1920x1080
When   il giocatore seleziona un'unità e pianifica Move + BasicAttack
Then   sono leggibili senza Debug HUD: round · fase · unità selezionata · HP ·
       i tre slot (Movement/Main/Reaction) · destinazione · path · bersaglio ·
       stato Ready/Confirm
And    l'esito reale è registrato nella voce PIE-V01-HUD del subset RELEASE-V01
```

Il widget **non ricalcola** path, LOS, legalità del bersaglio, danno, certezza, cooldown, obiettivo, reason
code: li consuma dal ViewModel. È la regola 3 della DoD trasversale, non una preferenza di stile.

---

## 7. Priorità 2 — Reaction Window · sede: [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166)

CP 14.6 «Counterplay, UI della finestra e misura del pacing» è **aperta**, dentro
[#152](https://github.com/DegrassiAaron/refactor-tactics-main/issues/152) (E14). Ha criteri propri: si
leggono **prima** di progettare.

### Due fatti che cambiano il progetto, e che il prompt non aveva

1. ⛔ **`FastReactionDuration` non esiste come simbolo.** `git grep -n FastReactionDuration -- Source`
   restituisce **due occorrenze, entrambe dentro commenti**
   (`RTReactionOpportunityTypes.h:326`, `RTOverwatchTriggerTests.cpp:1122`). La baseline «3,0 s» è scritta
   nei documenti e nel commento, ma **non c'è un campo che la UI possa leggere**: chi costruisce il
   countdown deve prima decidere dove quel valore vive.
2. ⚠️ [**#1118**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1118) è aperta — *«la
   risposta di reazione e la sua ragione sono un enum solo: regge Overwatch, non il Reaction Profile»*. È
   esattamente il tipo che la UI consumerebbe.

✅ **Quello che invece c'è**: `FRTReactionDecision` con `ERTReactionDecisionOutcome::HoldTimeout` come
**default**, e `DecisionOnTimeout()`. Il `Timeout → HOLD` è un fatto del codice, non una promessa.

### Invarianti che restano (dal prompt originale, corretti)

- La UI **non calcola** il danno: mostra ciò che il core ha prodotto — `HIT · 14 DANNI · CONFERMATO`.
- Mai `FIRE` automatico allo scadere.
- Lo slow motion è **presentazione**: non cambia la durata logica né l'esito. Niente `Delay`, montage,
  `Tick` o `DeltaTime` per decidere il sequencing.
- La reazione resta un **boundary dentro la Resolution**, non una macro-fase nuova.
- La UI non mostra intento futuro, path futuro, risposta nascosta o opportunity avversarie
  (DoD §1.6 · `G8` ✅ verde: `Reactions.IntentNotVisibleToEnemy` e altri due).

### Pacing — cosa manca perché la misura significhi qualcosa

`ReactionDecisionSeconds` e `ResolutionPlaybackSeconds` si misurano **separatamente**, e il caso minimo
include **due unità dello stesso giocatore armate**, perché più finestre dello stesso player possono
comparire in serie. ⚠️ Ma «registrare p50/p90» richiede una **numerosità dichiarata**: quante finestre,
quanti soggetti. Un solo autore contro un bot non produce un p90 — produce un aneddoto, e va scritto come
tale.

---

## 8. Priorità 3 — Frontend · sede: [#934](https://github.com/DegrassiAaron/refactor-tactics-main/issues/934)

Gli **8** widget Framework esistono già. Il flusso minimo `Boot → Main Menu → Play → slice → Result →
Play Again / Main Menu` non parte da zero.

🔴 **Difetto noto e aperto sul flusso stesso**:
[#1330](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1330) — *«All'avvio il frontend dice
"impossibile avviare", e premendo back la partita parte lo stesso»*. Va letto prima di toccare la
navigazione, non riscoperto dopo.

Il frontend **consuma** vincitore, punteggio e stato partita da `FRTMatchResultViewModel`: non li ricalcola.

⚠️ **Correlato, e già presidiato**: `RefactorTactics.Meta.TestGuardClosesAtEndOfFile` esiste perché quattro
test di `RTFrontendNavigationTests.cpp` erano scritti **dopo** l'`#endif` di `WITH_DEV_AUTOMATION_TESTS` —
compilavano in Editor e Development, non in Shipping. `G1` lo ha trovato il 2026-08-24. È la sola classe di
difetto che **solo** la build Shipping incontra.

---

## 9. Priorità 4 — Leggibilità del tabellone · sede: [#956](https://github.com/DegrassiAaron/refactor-tactics-main/issues/956) e [#1262](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1262)

⚠️ **La grammatica visiva è già decisa**: [**D-146**](../../decisions/RT_PDR_00_Decision_Log.md) fissa
«colore + forma», [**D-183**](../../decisions/RT_PDR_00_Decision_Log.md) sceglie il **contorno inciso nella
corona esterna** e la costante scura `FColor(25,25,25)`, con la banda da ricavare **dal codice e non dal
commento**. Questo mandato non la riapre.

Gli oracoli esistono e vanno usati al posto degli aggettivi:

| Invece di | Usa |
|---|---|
| «le celle devono essere distinguibili» | `RefactorTactics.Hex.SurfaceColorsAreDistinguishable` — soglia **60** in luminanza |
| «verificare a schermo» | [`PIE-V01-BOARD`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1262) — **a picco e in scala di grigi** |

🔴 **E la verifica ha una precondizione a tre passi**: eseguirne solo il primo non falsifica nulla.
D-183 lo misura — l'arena di default (`MakeTestArena`) nomina **una sola superficie**, e le quattro che
collidono vivono solo in `RelayBasin`:

1. un asset **scratch**, mai la mappa d'autore — `GenerateFixtureIntoAsset` fa `ReplaceContent`, cioè
   *sostituisce, non fonde*;
2. `FixtureId = RelayBasin` → `GenerateFixtureIntoAsset`;
3. **`rt.Map.Source = LevelAsset`** — senza cui non si vede niente: `ApplyMapSource` con
   `MapSource = GeneratedTestArena` *«prevale anche su una mappa d'autore presente nel livello»*, e a
   schermo torna l'arena di prova.

Niente `if (Showcase)` nel gameplay: la mappa visuale è consumer, lo scenario alimenta la pipeline normale.

---

## 10. Ciò che il mandato originale diceva bene, e resta

1. **Contratto UI** `Runtime → ViewModel sanitizzato → UMG` — coincide con DoD §1.3, ed è già il codice.
2. **Privacy dell'intento** — coincide con DoD §1.6 e con `G8` ✅.
3. **La reazione non è una macro-fase** — coerente con ADR-0003.
4. **Slow motion solo presentazione** — coerente col divieto di decidere il sequencing col tempo reale.
5. **`SEARCH → REUSE → UPDATE → CREATE`** — questo §4 ne è il risultato.
6. **«Non dichiarare `Done` perché sembra funzionare»** — è testualmente la regola d'apertura della DoD.
7. **Icone semantiche dal catalogo**, non texture hardcoded — `RTIconLibrary` esiste.

---

## 11. Fuori scope

Multiplayer · dedicated server · GAS · 3v3 / 4v4 competitivo · matchmaking · progressione · modding ·
nuovi eroi · nuove macro-meccaniche · art pass.

⚠️ **Reaction Clash** ([#314](https://github.com/DegrassiAaron/refactor-tactics-main/issues/314)) e
**Decision Time Bank** ([#319](https://github.com/DegrassiAaron/refactor-tactics-main/issues/319)) non sono
bloccanti — ma non sono nemmeno ignorabili: #1118 dice che il tipo che la UI della finestra consumerebbe è
**sotto-dimensionato proprio per il Reaction Profile**. Non bloccarli è possibile; progettare come se non
esistessero, no.

---

## 12. Output di una sessione che esegue questo mandato

Al posto del verdetto binario:

```text
HEAD iniziale / HEAD finale
Build:                  Editor · Development · Shipping — esito e log
Asset d'Editor toccati: elenco, uno per volta
Test:                   nome · seed · atteso · reale · verdetto · evidenza
Verifiche PIE:          voce del registro · esito reale (mai «sembra funzionare»)

Gate mossi:      Gxx  da <stato> a <stato>, con evidenza
Gate non mossi:  Gxx  e perché sta fuori dalla portata di questo lavoro
Blocker fuori scope: E21 / cilindri — CP 12.4 → CP 12.5

Issue aggiornate:  #nnn (stato + DoD consuntivato nel commento, non nel body)
Limiti dichiarati: ciò che non è stato misurato, detto come non misurato
```

---

## 13. Limiti di questo documento

- 🔴 **Lo stato dei gate è stato *letto*, non rimisurato**: la tabella porta la data **2026-08-24** e la
  base di lettura è **204 commit dietro** `origin/main`. Va rifatto prima di decidere.
- 🔴 **Nessuna sessione Unreal è stata aperta** per produrlo: nessun `.uasset` è stato ispezionato, nessuna
  PIE eseguita. Tutto ciò che riguarda l'aspetto a schermo è dedotto da codice e documenti.
- ⚠️ Il conteggio delle issue aperte è un **campione** dei primi 200 risultati filtrati per parola chiave,
  non un censimento.
- ⚠️ `G12` (packaging) porta la data **2026-08-16** e la DoD prescrive che sia **ridatata a ogni release**:
  un timbro senza data recente non è un'evidenza, è un ricordo.
