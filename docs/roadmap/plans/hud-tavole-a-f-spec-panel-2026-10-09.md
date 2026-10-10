# Le tavole HUD A–F — spec panel sul pacchetto del 2026-10-04, e che cosa ne è entrato

> `SNAPSHOT` · fotografia del 2026-10-09: vale finché è l'ultima misura del suo oggetto.
>
> - **Data**: 2026-10-09.
> - **Misurato su** `origin/main` = `a6bf857bf` per l'audit H1–H16. Le misure che sostengono [D-479](../../decisions/RT_PDR_00_Decision_Log.md) e i numeri delle voci sono rifatte su `ae805683a`; fra i due `Source/` non cambia.
> - **Oggetto**: il pacchetto «HUD — pacchetto design (2026-10-04)» consegnato dall'autore su Drive, con `SPECIFICA-ZONE.md`, `dati/zone.json`, `dati/schermate.json` e un `PROMPT.md`.
>   - Le immagini A–F **mancavano**: la cartella `immagini/` era vuota.
>   - L'autore le ha fatte rigenerare il 2026-10-09 con un prompt scritto in questa sessione, già sulla griglia di [D-456](../../decisions/RT_PDR_00_Decision_Log.md).
>   - La sorgente di design entra in [`../../research/design/hud/hud-screens-2026-10/`](../../research/design/hud/hud-screens-2026-10/). Il `PROMPT.md` è un work order e **non si versiona** ([`AGENTS.md`](../../../AGENTS.md) §8).
> - **Issue**: [#3605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3605).
> - **Panel**: Wiegers (lead) · Fowler · Adzic · Crispin · Nygard. **Modo**: critique. **Focus**: requirements, architecture, testing.
>
> ⛔ Nessuna riga di `Source/` e di `Content/` cambia con questo referto.
>
> Convenzione: niente totali che cambiano da soli. Dove un numero serve, sta accanto al comando che lo produce.

---

## 1. Il verdetto in una riga

**Il pacchetto dice bene come l'HUD deve apparire, ma è stato scritto prima di cinque giorni di decisioni.**
- Dove il mockup diceva **dove** stanno le zone, il canone diceva già altro ([D-456](../../decisions/RT_PDR_00_Decision_Log.md), [D-458](../../decisions/RT_PDR_00_Decision_Log.md)).
- Due premesse del work order erano false: il pulsante di conferma «assente» e il comando che misura l'ultima voce di registro.
- Entrano:
  - la sorgente di design;
  - sei voci di registro ([D-477](../../decisions/RT_PDR_00_Decision_Log.md)…[D-482](../../decisions/RT_PDR_00_Decision_Log.md));
  - una settima voce, [D-488](../../decisions/RT_PDR_00_Decision_Log.md), nata dal chiarimento dell'autore: **le tavole mostrano la vista strategica**;
  - l'allineamento di [`progettazione-hud.md`](../../technical/systems/progettazione-hud.md) e di [`spec-tactical-camera.md`](../../technical/systems/spec-tactical-camera.md).
- Il codice resta fuori, con i suoi seguiti (§9).

## 2. Audit H1–H16

Sono i comandi del work order, rieseguiti su `a6bf857bf`. Le divergenze sono state riportate all'autore **prima** di qualunque scrittura.

| # | Atteso dal pacchetto | Misurato | |
|---|---|---|---|
| H1 | I dieci widget nominati | Ci sono tutti, più `URTPlanCommitWidget`, `URTActionTooltipWidget`, `URTHudZoneWidget`, `URTHeroProfileWidget`, `URTHeroRadarWidget` e la base `URTScreenHudWidgetBase` | ⚠️ |
| H2 | `FRTMatchHeaderView` senza macrofase | I campi attesi ci sono. Ma `Phase` è un `ERTMatchPhase`, che ha `Prep`…`Cleanup`: la macrofase **logica** c'è già, quella **riprodotta** no (§5, D-479) | ⚠️ |
| H3 | `FRTUnitCardView` senza stato né reazione | Come atteso | ✅ |
| H4 | Punteggio «composto, non disegnato» | `ARTHUD::ComposeMatchStatusLine` lo compone (`RTHUD.cpp`), e nessun consumatore UMG lo disegna ([#2764](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2764)) | ✅ |
| H5 | Token di danno con dato e percorso | `URTHudViewModel::BuildDamageToken` → `RTUnit.cpp` → `URTUnitOverlayWidget::PushDamageToken` | ✅ |
| H6 | `FRTPlayerEventLineView` senza reason code | Come atteso, più `bHasBlocker` | ✅ |
| H7 | Reazione rapida con `GetRemainingSeconds()`, `Options`, `SafeResponse` | Come atteso. ⚠️ Ma l'opzione, `FRTReactionWindowOptionView` (`Turn/RTReactionWindowView.h`), porta solo `Response` e `TargetSnapshotIndex`: niente tasto, niente descrizione (§6, T2) | ✅ |
| H8 | `FRTUnitSlotsView` / `FRTPlannedSlotView` | Come atteso | ✅ |
| **H9** | **Nessun widget di conferma** | **`URTPlanCommitWidget`** ([#3471](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3471), [D-458](../../decisions/RT_PDR_00_Decision_Log.md)): `Conferma` = `Invio` (`TogglePlanDeclaration`), `Annulla` = il Back (`UndoStep`, `Backspace`). Le due occorrenze di `CONFIRM PLAN` sono commenti di `RTContextInspector.h` | ❌ |
| H10 | Riproduzione: comandi da spettatore, pausa inerte | Come atteso | ✅ |
| H11 | `bFriendlyFire` per unità, nessun avviso di piano | Come atteso: nessuna occorrenza di `FRTWarning` né di `PlanWarning` in `Source/` | ✅ |
| H12 | Nessuna traccia di Ghost Timeline, intenti alleati, WHY? | Il `grep -rln` del pacchetto → 0 | ✅ |
| H13 | `ERTAwareness` logico, nessuna superficie in `UI/` | `Hidden` · `Uncertain` · `Detected`; 0 file in `UI/` | ✅ |
| H14 | La famiglia `ScreenHud`/`HudViewModel` | Rieseguito il comando del pacchetto. Il numero è cresciuto rispetto al 2026-10-04: è crescita della suite, non un difetto | ⚠️ |
| **H15** | Ultima seduta, ultima voce di registro | Ultima seduta `U71`. ❌ **Il comando del pacchetto restituisce `D-999`**, cioè l'esempio scritto dentro D-433. L'ultima voce reale si legge dalle **righe** della tabella del registro, non dal testo: **D-476** | ❌ |
| H16 | Stato delle issue citate | #2455 **chiusa** · #2744 aperta · #1879 **chiusa** · #1881 aperta · #1936 aperta | ⚠️ |

## 3. La classificazione delle zone, corretta

La posizione è quella della griglia di [`guida-screen-hud-umg.md`](../../technical/runbooks/guida-screen-hud-umg.md) §3 e delle tavole del 2026-10-09; [`dati/zone.json`](../../research/design/hud/hud-screens-2026-10/dati/zone.json) resta la fotografia del 2026-10-04.

| Zona | Classe nel pacchetto | Classe misurata | Posizione | Owner |
|---|---|---|---|---|
| Z1 Header turno | PARZIALE | PARZIALE: in Risoluzione serve la fase riprodotta (D-479) | `TopCenter` | [#613](https://github.com/DegrassiAaron/refactor-tactics-main/issues/613) |
| Z2 Roster | PARZIALE | PARZIALE: manca il dato del chip `REAZ.` (D-478) | `TopLeft` | #613 · [#2744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2744) |
| Z3 Obiettivo | PARZIALE, «alto-destra» | PARZIALE, **in `TopCenter`** (D-481) | `TopCenter` | [#2281](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2281) · [#2764](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2764) |
| Z4 Unità selezionata | PARZIALE, «basso-sinistra» | PARZIALE, **in `MiddleLeft`** | `MiddleLeft` | #613 |
| Z5 Barra dei comandi | → skill bar | CURRENT nel C++; resa nelle sedute della skill bar | `Bottom` | [#2826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826) |
| Z6 Ghost Timeline | DESIGNED, «sopra la dock» | DESIGNED, **in `TopCenter`** | `TopCenter` | [#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172) |
| Z7 Conferma / Annulla | PARZIALE, «basso-destra» | **CURRENT** nel C++, in `TopRight` (H9) | `TopRight` | [#3471](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3471) |
| Z8 Avvisi | PARZIALE | PARZIALE: l'elenco è deciso (D-480), non scritto | `MiddleLeft` + contatore in `TopRight` | seguito T4 |
| Z9 Intenti alleati | FUTURE | FUTURE (D-478) | `MiddleRight`, linguetta chiusa | — |
| Z10 Registro + WHY? | PARZIALE | CURRENT il registro, FUTURE il WHY? | `MiddleRight` | [#1937](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1937) |
| Z11 Evento corrente | DESIGNED | DESIGNED: nessun indice né totale d'evento in `UI/*.h` | `TopCenter` | seguito T5 |
| Z12 Riproduzione | FUTURE | **non si costruisce** (D-477) | — | [#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881) |
| Z13 Danno fluttuante | CURRENT | CURRENT (H5) | nel mondo | [#2453](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2453) |
| Z14 Reazione rapida | CURRENT, «nessun C++» | CURRENT, **ma con un buco di dati** (T2) | centro, l'unica esenzione | [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) |
| Z15 Conoscenza parziale | DESIGNED | DESIGNED: logica sì, superficie no | nel mondo + `MiddleRight` | [#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160) |
| Z16 Overlay nel mondo | CURRENT | CURRENT | nel mondo | #613 |

## 4. Il panel

### 4.1 WIEGERS — due premesse negative erano false

❌ **CRITICAL.**
- Z7 era classificata «comando sì, pulsante no», ma il pulsante c'era da [#3471](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3471).
- H15 misurava l'ultima voce con un `grep` che cattura il primo `D-nnn` scritto in un testo, e restituiva un numero mai assegnato.

È la stessa lezione del [referto della skill bar](skill-bar-mockup-spec-panel-2026-10-04.md) §4.1: un requisito che dice «manca X» va scritto col comando che lo dimostra, **e il comando va provato** su un caso che ha la risposta nota.

### 4.2 FOWLER — lo stile non ha un posto nel codice

⚠️ **MAJOR.**
- La palette di §32 vive solo nei `WBP_*`: `grep -rn "00E0FF\|RT_UI_Cyan" Source/ | wc -l` → 0.
- Rivestire i widget a mano produce tante copie della palette quanti sono i widget.
- [D-482](../../decisions/RT_PDR_00_Decision_Log.md) aggiunge i token a §32 e dichiara aperta la loro sede a runtime: è il seguito T6, da fare **prima** delle sedute di stile. → Sciolto da [D-489](../../decisions/RT_PDR_00_Decision_Log.md) (2026-10-10, [#3610](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3610)).

### 4.3 ADZIC — gli esempi delle tavole sono coerenti fra loro, salvo uno

- Le sei tavole raccontano lo stesso scenario, con le stesse unità e gli stessi PV.
- L'eccezione è §6 D3: D ed E sono due varianti incompatibili dello stesso turno 04.
- Come esempi vanno bene, purché lo si sappia: la tavola è un esempio, non una sequenza.

### 4.4 CRISPIN — il criterio di accettazione delle sedute

Il criterio di ogni seduta Editor è doppio:
- **(a)** la zona corrispondente della tavola a 1920×1080, confrontando **solo** lo screen-space (§6, nota sulla scena);
- **(b)** la stessa schermata portata in scala di grigi (§47-bis.1).

La verifica (b) sulle tavole stesse è stata fatta in questa sessione, con un render in grigi: ogni stato resta leggibile (§6).

### 4.5 NYGARD — l'ordine dei rischi

- Tutto il rivestimento visivo cambia `.uasset`, quindi va in seduta Editor dal clone principale, col motore libero e una voce in [`editor-sessions.yaml`](../editor-sessions.yaml).
- Il C++ dei seguiti arriva prima e separato.
- Il rischio più alto non è visivo, è di **privacy**. Il registro delle tavole D ed E mostra azioni del nemico (§6 D4), e il chip `REAZ.` (D-478) e gli avvisi (D-480) devono restare muti su un'unità non comandata.

## 5. Le decisioni

| Domanda del pacchetto | Esito | Voce |
|---|---|---|
| `DEC-HUD-1` Controlli di riproduzione | Non entrano nell'HUD del giocatore | [D-477](../../decisions/RT_PDR_00_Decision_Log.md) |
| `DEC-HUD-2` UI di squadra con un comandante | Z9 ed Editing/Ready/Locked `FUTURE`; il roster ha PV, scudo, `REAZ.` | [D-478](../../decisions/RT_PDR_00_Decision_Log.md) |
| `DEC-HUD-3` Macrofase nell'header | Quattro celle sulla fase **riprodotta**, in un campo proprio della vista | [D-479](../../decisions/RT_PDR_00_Decision_Log.md) |
| `DEC-HUD-4` Stati del pulsante | **Già chiusa** da [D-458](../../decisions/RT_PDR_00_Decision_Log.md); le tavole disegnano il riposo e `Ritira` | — |
| `DEC-HUD-5` Avvisi di piano | Li riporta il view-model; la legalità la decide il validatore | [D-480](../../decisions/RT_PDR_00_Decision_Log.md) |
| `DEC-HUD-6` *(nuova, da D-456 punto 9)* Dove va l'obiettivo | In `TopCenter`, nell'header | [D-481](../../decisions/RT_PDR_00_Decision_Log.md) |
| `DEC-HUD-7` *(nuova)* Colori del testo del mockup | Entrano in §32 | [D-482](../../decisions/RT_PDR_00_Decision_Log.md) |
| *(dalle tavole)* `Guardia reattiva` in `REAZIONE` | **Vince il codice**: `Action.Overwatch` è `ERTActionSlot::Main` (`RTCatalogLibrary.cpp`) | — |
| *(dal chiarimento dell'autore)* Che cosa sono le tavole | Mostrano la **vista strategica**. `Tab` porta lo zoom alla soglia di [D-252](../../decisions/RT_PDR_00_Decision_Log.md) e ritorno, al centro la 3D diventa un'isometrica semplificata, l'HUD resta lo stesso. Il ciclo della selezione passa da `Tab` a `N` | [D-488](../../decisions/RT_PDR_00_Decision_Log.md) |

⚠️ **D-488 era un `CONTRACT CONFLICT`, ed è stato portato all'autore prima di scrivere.** La richiesta era «si attiva con Tab». Contro c'erano due fatti:
- [D-252](../../decisions/RT_PDR_00_Decision_Log.md) lega la vista strategica allo zoom, e nega una modalità rigida;
- `Tab` era già di `CycleSelectionAction` ([D-421](../../decisions/RT_PDR_00_Decision_Log.md), `RTPlayerController.cpp`).

La risposta concilia le due voci senza superarle: `Tab` muove la distanza, e lo stato resta della distanza.
La numerazione salta da D-482 a D-488 perché D-483…D-487 li ha presi [#3606](https://github.com/DegrassiAaron/refactor-tactics-main/pull/3606) nel frattempo.

⚠️ **D-479 è più stretta della domanda del pacchetto, e lo è per una misura.**
- Il pacchetto chiedeva «un campo per la macrofase», come se mancasse.
- Il campo c'è, ma è la fase logica. Senza sospensioni `RunPhaseLoop` risolve tutto **prima** del playback e riporta `Phase` a `Planning`: un header costruito su `Phase` avrebbe detto PIANIFICAZIONE per tutta la riproduzione.
- La fase riprodotta è già esposta, come nome (`GetPlaybackPhaseName()`) e come evento (`OnPhasePlaybackStarted`), e nessun widget la legge. *(Corretto dopo la review indipendente, §11: la prima stesura diceva che la leggeva solo un accessor di test.)*

## 6. Le tavole del 2026-10-09

Ho ricavato le tavole dal PDF a 1920×1080 e le ho portate in scala di grigi per la verifica.

🎬 **Che cosa mostra la scena — corretto due volte, e la seconda vale.**
- **La prima lettura** (mia, di questo panel) diceva che la scena isometrica era «illustrativa», perché la vista tattica è la 3D attuale. Quella frase è finita come nota a piè di pagina sulle tavole annotate.
- **L'autore l'ha precisata subito dopo.** Le tavole sono la **vista strategica**: con `Tab` il centro passa all'isometrica semplificata, e l'HUD resta lo stesso ([D-488](../../decisions/RT_PDR_00_Decision_Log.md)).
- **La scena è quindi un bersaglio, non un'illustrazione.** Per scelta dell'autore le tavole non si rifanno: la nota superata è dichiarata nel README e in `SPECIFICA-ZONE.md`.

**✅ Che cosa reggono:**
- tutte e sei rispettano la griglia di D-456;
- il centro è libero, e la finestra di reazione è 600×200 centrata;
- in Risoluzione spariscono Conferma, Ghost Timeline e avvisi, e il registro è promosso;
- gli avvisi dicono cosa · perché · costo;
- il WHY? dichiara *«valori riportati dall'evento, non ricalcolati»*;
- i contatti si contano per categoria, senza coordinate;
- in scala di grigi ogni stato si distingue: tratteggio, triangolo, barra sotto lo slot, numero del cooldown, angolo del pianificato, «SCELTA SICURA».

**Che cosa non torna:**

| # | Dove | Divergenza | Esito |
|---|---|---|---|
| D1 | E, F | `Guardia reattiva` nel chip `REAZIONE`, con `PRINCIPALE` vuoto. Nel catalogo occupa lo slot principale | Vince il codice (§5). `FRTUnitSlotsView` mostrerà `PRINCIPALE Guardia reattiva`, `REAZIONE —`. ✅ Corretto nella riesportazione |
| D2 | E | Badge `F`/`H` e descrizioni («Attacca Ivrin», «Non attaccare») sulle opzioni: `FRTReactionWindowOptionView` non ha né tasto né descrizione | Buco **vero**: §47-bis.2 vuole il percorso da tastiera. Seguito T2 |
| D3 | D, E | Due turni 04 incompatibili: in D Aevik spara Arc Pulse in BLAST, in E ha armato Guardia reattiva in PREP, e sono entrambe principali | Varianti, non una sequenza (§4.3) |
| D4 | D, E | Il registro mostra azioni di Branth (`Irrigidimento`, `Interagisci → Nodo`) | Corretto **solo** se gli eventi sono visibili alla squadra. Il widget legge la proiezione autorizzata, e la tavola non lo dimostra |
| D5 | D, E | In Risoluzione l'header perde `/12` | ✅ Corretto nella riesportazione: `TURNO 04 / 12` |
| D6 | — | Il PDF scaricato da Drive e quello conservato in locale sono **due esportazioni diverse**, ed entrambi **precedono** le correzioni D1, D5, D7 e D8 | Non entra nessun PDF: entrano le PNG della riesportazione, che è l'unica con le correzioni, e la tavola G come PNG |
| D7 | C | Il costo del conflitto su G7 l'ha scritto il generatore delle tavole: «uno dei due movimenti può non arrivare». **L'ho corretto in «nessuno dei due arriva», e la correzione era sbagliata.** La contesa si valuta **per microstep** (`ArrivaOra`, `RTHexSimLibrary.cpp`). Si fermano entrambi (`BlockedContested`) solo se entrano in G7 nello stesso passo; chi arriva prima occupa la cella, e l'altro si ferma (`Movement.EarlyArrivalBlocksALaterPasser`). Con 3 celle contro 6, Aevik arriva e Muiren no | ❌ **Errore della tavola C, introdotto da questo referto.** Dichiarato nel README e in `SPECIFICA-ZONE.md`. La frase giusta: «Aevik arriva prima (3 celle contro 6): Muiren si ferma prima di G7» |
| D8 | annotate | Anche le definizioni della legenda le ha scritte il generatore. «Già presente nel gioco» per `CURRENT` promette più del vero: il widget e il dato esistono, la resa delle tavole no | ✅ Riscritte nella riesportazione, con la voce `DECISA` per ciò che è deciso il 2026-10-09 e non ancora nel codice |

## 7. Che cosa il work order chiedeva e non è stato fatto

| Richiesta | Perché non si fa qui |
|---|---|
| P1 Reazione rapida «senza C++» | La premessa è falsa (D2): serve un campo nella vista. È il seguito T2 |
| P2 Z11 «evento 3/5», dock collassata | Codice: seguiti T5 e T7 |
| P3 Z1, Z6, Z7 | Z7 era già fatta (H9); Z1 è il seguito T1, Z6 è di [#172](https://github.com/DegrassiAaron/refactor-tactics-main/issues/172) |
| P4 Avvisi | Deciso (D-480), non scritto: seguito T4 |
| P5 Nota di design per C e F | La nota è questo referto, §3 e §6. La tabella fra simboli e `ERTAwareness` resta a [#160](https://github.com/DegrassiAaron/refactor-tactics-main/issues/160) |
| Voci `U<n>` in `editor-sessions.yaml` | Si scrivono quando il C++ di ciascun passo è su `main` (D-403), non prima |
| Numerare le voci come `D-4xx` «dopo l'ultima in HEAD» | Fatto, ma con la misura di §2 H15 e non col comando del work order (AGENTS §12) |

## 8. Che cosa entra con #3605

| Dove | Che cosa |
|---|---|
| [`research/design/hud/hud-screens-2026-10/`](../../research/design/hud/hud-screens-2026-10/) | README, `SPECIFICA-ZONE.md` con l'intestazione di statuto, `dati/`, `immagini/` (le PNG della riesportazione), `sorgente-mockup/` |
| [`decisions/RT_PDR_00_Decision_Log.md`](../../decisions/RT_PDR_00_Decision_Log.md) | D-477…D-482, D-488 e la nota sui numeri |
| [`technical/systems/progettazione-hud.md`](../../technical/systems/progettazione-hud.md) | §3.2, §6.1, §6.2, §6.3, §6.5, §6.6, §15 e §32 allineate alle voci |
| [`technical/systems/spec-tactical-camera.md`](../../technical/systems/spec-tactical-camera.md) | §3.2, la riga `Tab` → Strategic View al posto di `M`; §5, D-488 |
| [`skill-bar-2026-10/dati/tokens.json`](../../research/design/hud/skill-bar-2026-10/dati/tokens.json) | La nota *«NON esistono in §32»* cita D-482 |
| [`CHANGELOG_DOCUMENTATION.md`](../../CHANGELOG_DOCUMENTATION.md) | Una voce |

## 9. Seguiti

Proposti, **non** creati con questo referto: ciascuno diventa una issue quando l'autore lo apre, dopo una ricerca dell'owner.

| # | Seguito | Tipo | Owner probabile |
|---|---|---|---|
| T1 | La fase riprodotta in `FRTMatchHeaderView` (D-479), alimentata dalla sorgente di `GetPlaybackPhaseName()` / `OnPhasePlaybackStarted` e non da un'altra, con un test che la tiene distinta da `Phase`. ✅ **Aperto come [#3612](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3612)** (2026-10-10): `FRTMatchHeaderView::PlaybackPhase` da `ARTTurnManager::GetPlaybackPhase()` | C++ + test | #613 |
| T2 | Il tasto e la descrizione dell'opzione in `FRTReactionWindowOptionView` (D2). ✅ **Aperto come [#3615](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3615)** (2026-10-10): [D-491](../../decisions/RT_PDR_00_Decision_Log.md) sceglie i tasti `1`, `2`, `3`… e una frase generica; tasto e frase stanno nello strato UI, non nel DTO del core | C++ + test | [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) |
| T3 | ✅ **Aperto come [#3618](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3618)** (2026-10-10): `FRTUnitSlotsView::bReactionArmed`, `URTCatalogLibrary::ArmsReaction`, `URTTeamRosterWidget::IsReactionArmed`. Il chip `REAZ.` (D-478): la lettura del piano per ogni alleato comandato, da `FRTUnitSlotsView` e **non** da `FRTUnitCardView`, e il predicato che conta l'`Overwatch`. Muto per un'avversaria | C++ + test di privacy | #613 |
| T4 | ✅ **Prima metà aperta come [#3620](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3620)** (2026-10-10, [D-492](../../decisions/RT_PDR_00_Decision_Log.md)): i segni di fuoco amico derivano dall'anteprima per piano, anche sui bersagli-cella. ✅ **Seconda metà aperta come [#3622](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3622)** ([D-494](../../decisions/RT_PDR_00_Decision_Log.md)): l'elenco e il contatore per livello. L'elenco degli avvisi di piano (D-480), con l'attribuzione di ogni avviso al piano che lo causa: `bFriendlyFire` dice chi è colpito, non da quale piano | C++ + test | #613 |
| T5 | Indice e totale dell'evento riprodotto, per la striscia Z11 | C++ + test | [#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881) |
| T6 | La sede a runtime dei token di §32 (D-482): una sola fonte di stile per i widget. ✅ **Aperto come [#3610](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3610)** (2026-10-10): [D-489](../../decisions/RT_PDR_00_Decision_Log.md) la mette in una libreria C++, `URTUIPalette` | decisione + C++ | #613 |
| T7 | La barra dei comandi collassata in Risoluzione: riepilogo dei tre slot da `FRTUnitSlotsView` | asset | [#2826](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2826) |
| T8 | Le sedute Editor di stile, una per cluster, dopo T6: header e obiettivo · roster e pannello · registro · finestra di reazione · conferma | asset | #613 |
| T9 | ✅ **Aperta in quattro pezzi** (2026-10-10, [D-495](../../decisions/RT_PDR_00_Decision_Log.md)), con il [brief](vista-strategica-brainstorm-2026-10-10.md): nucleo [#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774) · inclinazione [#3630](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3630) · prova ortografica [#3631](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3631) · separazione dei piani [#3632](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3632). D-488. `Tab` porta lo zoom alla soglia strategica e ritorno; il ciclo della selezione va su `N` **nello stesso commit**. La presentazione isometrica semplificata al centro, sola presentazione | C++ + asset + test | [#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774) · [#3145](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3145) |

⚠️ **Un documento owner è indietro, e T9 lo deve riscrivere.** [`spec-pointer-interaction.md`](../../technical/systems/spec-pointer-interaction.md)
§6.6 dice ancora *«Nessun ciclo di selezione. `git grep -n "EKeys::Tab"` non stampa nulla»*. Da
[D-421](../../decisions/RT_PDR_00_Decision_Log.md) il binding esiste (`RTPlayerController.cpp`), e con D-488 cambia di nuovo tasto (`STALE ROADMAP`).

## 10. Gate di questo passaggio

I gate eseguiti, con il loro esito, sono nel corpo della PR di [#3605](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3605).

## 11. Dalla review indipendente

Prima del merge un revisore separato, con lo stesso albero, ha verificato sul codice le affermazioni delle voci
D-478, D-479, D-480, D-488, i punti H2, H5, H6, H7, H9 e H11 dell'audit e le divergenze D1, D2 e D7. Non ha
rifatto H12, H13, H14 né H16. Ha fatto due passate, la seconda sulle correzioni della prima. Ciò che ha
trovato è stato corretto **in questo referto e nelle voci**, non nascosto. I rilievi che cambiavano il contenuto:

| Rilievo | Dove | Correzione |
|---|---|---|
| La fase riprodotta non la legge «solo un accessor di test»: esistono `GetPlaybackPhaseName()` («per la HUD») e `OnPhasePlaybackStarted` | D-479, T1 | La voce le nomina, e il campo si alimenta da lì: Search → Reuse |
| Senza sospensioni `Phase` torna a `Planning` prima del playback: l'header avrebbe detto PIANIFICAZIONE, non BLAST | D-479 | Il meccanismo è descritto come misurato |
| La contesa di una cella si valuta per microstep: «si fermano entrambi» vale solo per arrivi simultanei | §6 D7 | La tavola C ha ora un costo sbagliato, dichiarato come errore |
| `Invalid` è un rifiuto, non un piano «accettato ma degradato» | D-480, §15 | `Invalid` è Critical, e solo nella lettura del piano illegale: il rifiuto del click resta della dock |
| `bFriendlyFire` non dice quale piano colpisce | D-480, T4 | L'attribuzione è lavoro dichiarato |
| `FRTUnitCardView` per contratto non porta piani; il dato è in `FRTUnitSlotsView`; l'`Overwatch` è `Main` | D-478, T3 | Il chip legge il piano, e la voce dice che l'`Overwatch` conta |

Gli altri rilievi erano di precisione, e sono corretti nei rispettivi file:
- la sezione di `spec-tactical-camera.md` (§3.2, non §2);
- l'origine del tasto `M` (D-456 punto 3, non D-457);
- la citazione «cosa · perché · costo», che viene da `SPECIFICA-ZONE.md` e non da §15;
- un totale in prosa;
- `Tab` al presente per una cosa non cablata;
- le frasi di §3.2 superate da D-252 e D-488;
- l'emendamento di D-421, che ora è dichiarato in entrambe le voci;
- le affermazioni del corpo di `SPECIFICA-ZONE.md` superate da D-478.

