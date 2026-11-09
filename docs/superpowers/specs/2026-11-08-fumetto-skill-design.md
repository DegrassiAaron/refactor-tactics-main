# Il fumetto della skill — un grido manga sopra l'unità quando il playback mostra un'attivazione

> **Statuto**: design nato dal dialogo con l'autore del 2026-11-08 (`/sc:brainstorm`), **approvato
> dall'autore il 2026-11-09 («procedi»), non implementato.** Issue [#4025](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4025). Realizza la domanda fissa `fumetti` di `G17`
> (ondata 2), decisa dall'autore in [`D-582`](../../decisions/RT_PDR_00_Decision_Log.md) (3) e descritta in
> [`g17-domande-fisse-2026-11-05.md`](../../roadmap/plans/g17-domande-fisse-2026-11-05.md) (voce `fumetti`).
> Copre la **prima fetta**: il fumetto delle skill che si attivano. Le azioni bloccate come fumetto, che la
> stessa domanda richiede, sono la fetta successiva (§9).
>
> ➕ **Implementato il 2026-11-09 su `feat/4025-fumetto-skill`** (⌫ *«non implementato», qui sopra, era lo stato
> all'approvazione*): funzioni pure e vista `7d6c71355`; canale sull'unità e sull'overlay `496e4461d`, `cfaaaadef`;
> cablaggio nel `TurnManager` con l'orologio del playback `99d6a2580`, `e480618fd`; unione di `origin/main`
> `ec7355eae`; scorciatoie del censimento della superficie BP che la unity piena non compilava `d2abb275c`;
> `WBP_RT_UnitOverlay` col fumetto `2ad1a8aa5`; `DrawSize` e contorno nel colore di squadra `0fd13f769`; censimento
> dei tre widget del fumetto `ab05e9b55`; documentazione in `progettazione-hud.md` `4bd839cf1`; `DrawSize` 220×228
> sulla misura della seduta visiva `5167ae133`. **Suite** `RefactorTactics` intera a `ab05e9b55`, build unity piena
> senza esclusioni adattive: `Found 3702 automation tests`, 3702 `Result={Success}`, `TEST COMPLETE. EXIT CODE: 0`.
> **Seduta visiva** del 2026-11-09 (finestra `-game`, non PIE; un passaggio indipendente che non ha scritto codice;
> clone principale a `ab05e9b55`, L1 rimisurata a `5167ae133`):
> [evidenza](../../technical/evidence/4025/seduta-pilotata-2026-11-09.md), esiti nella voce `PIE-HUD-FUMETTO` di
> [`test-manuali-pie.md`](../../technical/test-manuali-pie.md), che è 🟡. Due `FAIL`: il punto (e) nella seconda
> metà, perché il Registro a destra non porta la riga `Attiva:` (premessa di R8 falsa, seguito
> [#4033](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4033)), ed è un buco della fetta, non del
> fumetto; il punto (h) con `ESC`, perché `ESC` non ferma il playback: la premessa di A8 era imprecisa, e che il
> fumetto continui col menu aperto è un `Ruling` (nota sotto A8). Manca il giudizio d'autore sulla resa (§5) e su
> quel `Ruling`.
> ➕ **Review finale del branch (2026-11-09), corretta in `22982c3ef`**: il fumetto ha un orologio suo
> (`SkillCalloutClock`), perché `PlaybackElapsedTotal` torna a zero alla ripresa dopo una finestra di reazione (nota in
> §2, R4); le azioni dell'equipaggiamento non gridano (nota sotto §3.2, R1b). La seduta visiva precede la correzione. Il
> test della finestra non legava l'ascoltatore e non apriva la finestra: corretto in `d9531eb83`, mutazioni
> dichiarate in `7bf3cb63a`. **Suite** `RefactorTactics` intera sul merge `edb759df2` (con `main` a `6b9c41f5f`):
> 3721 `Result={Success}` su 3721, `TEST COMPLETE. EXIT CODE: 0`.
>
> **Stato misurato**: 2026-11-08, `main` = `e2fc03f09`; `Source/` identico sul branch di lavoro
> (`git diff --stat main HEAD -- Source` vuoto) e su `origin/main` = `5eac6115e` del 2026-11-09
> (`git diff --stat e2fc03f09 origin/main -- Source` vuoto). Ogni `file:riga` è stato letto su quel codice;
> chi lo rilegge più tardi lo **rimisura**. Nessun totale volatile: dove serve una misura c'è il comando.
>
> Le scelte prese **dall'autore** stanno in §1, alla lettera. Le scelte prese **da chi scrive** per
> realizzarle sono marcate `Ruling`: revocabili dall'autore, e ognuna dice cosa costa se è sbagliata.
> Un revisore indipendente ha letto la bozza come builder (2026-11-09): le sue tre domande hanno prodotto
> A7, A8 e la revisione di R7.

---

## 1. Le decisioni d'autore (2026-11-08)

| # | Domanda | Decisione, con le parole dell'autore |
|---|---|---|
| A1 | Quando | «dopo aver pianificato, durante l'esecuzione appena si attiva la skill appare un fumetto con il nome della skill per 3 secondi, come se in un manga il personaggio urlasse». |
| A2 | Contenuto | «il nome della skill per il momento». Niente descrizione, niente icona, niente costo. |
| A3 | Dimensione | «con le dimensioni corrette, senza esagerare la grandezza». |
| A4 | Chi | «anche i nemici». |
| A5 | Più fumetti insieme | Messo davanti al fatto che il playback mostra un'attivazione ogni mezzo secondo (§2) e un fumetto da 3 s ne vedrebbe partire altri sei: «accetto più fumetti vivi contemporaneamente». La cadenza del playback **non** cambia. |
| A6 | Perimetro | «parti solo dalle skill che vanno a segno»: la fetta copre le attivazioni, **non** le azioni bloccate. |
| A7 | Quali azioni | Messo davanti al fatto che `AbilityActivated` esce anche per l'attacco base di ogni eroe, per Guardia, Irrigidimento, Guardia reattiva e per le coperture (§2): **«Solo le skill del kit»**, escluso l'attacco base e le azioni generiche. |
| A8 | Orologio | Messo davanti al fatto che il playback si ferma (ESC, `Next Action`) e rallenta nella finestra di reazione (§2): **«Tempo del playback»**. In pausa il fumetto resta fermo; nello slow-motion dura di più a parete. |

> ⚠️ **A8, misurato il 2026-11-09: `ESC` non ferma il playback.** Apre il menu di pausa, che «Non sospende niente»
> (`Frontend/RTFrontendNavigator.h:290-310`: nessun `SetPause`, nessuna dilatazione del tempo, nessun flag nel
> `TurnManager`). La pausa del playback è `K` (`ARTTurnManager::PausePlayback`, con i comandi di playback accesi,
> `rt.Debug.PlaybackControls 1`). Il fumetto segue l'orologio del playback in entrambi i casi, fermo con `K` e in
> corsa con `ESC` (seduta visiva, punto (h): [evidenza](../../technical/evidence/4025/seduta-pilotata-2026-11-09.md)).
>
> `Ruling` Leggere A8 come «il fumetto segue il playback, anche quando `ESC` non lo ferma» è una lettura di chi
> scrive, non dell'autore, che aveva risposto a una premessa sbagliata. Costo se sbagliata: con il menu di `ESC`
> aperto il fumetto continua e si dissolve dietro al menu, e chi torna alla partita non lo trova più. Se l'autore
> vuole che `ESC` fermi tutto, è lavoro del frontend (oggi `ESC` «Non sospende niente», per scelta), non del fumetto,
> che seguirebbe da sé un playback fermo.

Decisioni precedenti che governano, già accettate:

- [`D-582`](../../decisions/RT_PDR_00_Decision_Log.md) (3): fumetto stile manga sopra il personaggio
  all'attivazione; nella **vista strategica non si vede** e l'informazione sta nel log a destra; il fumetto
  **segue la stessa visibilità degli eventi** (privacy, confermata dall'autore).
- [`D-590`](../../decisions/RT_PDR_00_Decision_Log.md) (1): look cel-shaded / manga, recepito da E52.1 (#4005).
- Ondata 5 di `G17`: «stile dei fumetti coerente» col resto dell'interfaccia.

---

## 2. Il problema, misurato

**Il momento esiste, il fumetto no.** Ogni intento con un `ActionId` emette `AbilityActivated`
(`ERTResolvedEventType`, `Turn/RTResolvedEvent.h:160-177`; costruttore unico `EmitAbilityActivated`,
`Turn/RTTurnManager.cpp:418-465`) in Prep, Dash e Blast, con `SourceStableUnitId`, `ActionId`,
`BaseActionId`, `Origin` e il `SourceVerdict` di privacy congelato all'emissione (`RTResolvedEvent.h:283,
:409, :427, :470, :338`). Il consumatore è `ARTTurnManager::ShowActivation` (`RTTurnManager.cpp:9039-9049`):
scrive la riga di log `Attiva: <unità> -> <ActionId>` e chiama `PlayPresentationRole(Cast, ActionId,
BaseActionId)`. Nessun testo compare sopra l'unità.

**La cadenza.** Le attivazioni di Prep e Dash si svelano una per volta, `AttacksToShow(N, Elapsed,
AttackShowSeconds)` = `Min(N, 1 + Floor(t / A))` (`RTTurnManager.cpp:9842-9853`,
`Turn/RTPlaybackLibrary.h:174-188`); il Blast scorre una sequenza per atto allo stesso passo
(`:9868-9886`). Tempi di default in `Turn/RTTurnManager.h`: `AttackShowSeconds` 0,50 s (`:1453`),
`ActivationCueSeconds` 0,35 s (`:1469`), `PhaseBeatSeconds` 0,30 s (`:1449`). **Nessuna formula di durata
della fase riserva tempo a un fumetto da 3 s**: sopravvive alla fase e, senza una regola, al playback.

**L'orologio del playback non è l'orologio di parete.** `TickPlayback` esce senza avanzare quando il
giocatore ha messo in pausa (`bPlaybackPaused`, `RTTurnManager.cpp:9576`) e quando una finestra di reazione
trattiene il playback parziale (`bPlaybackHeldByWindow`, `:9586`); il passo è `DeltaSeconds ×
EffectivePlaybackSpeed(ViewerPlaybackSpeed)` (`:9590`), e la velocità scende durante la finestra
(`RTTurnManager.h:1529-1536`, D-350). `PlaybackElapsedTotal` (`RTTurnManager.h:3950`) accumula quel passo
(`:9604`, `:9614`), si corregge sugli step di `Next Action` (`:9624`), ~~si **conserva** quando il playback
riparte dopo una finestra (`BeginPlayback(bPreserveClock)`, `:8689`, `:8983`)~~ (premessa falsa: nota qui sotto) e torna a zero all'inizio del
turno. È il tempo «che si vede» (`:9597`). Le cue d'attivazione sulla mappa vivono già su questo orologio
(`PushPlaybackCues` → `ActivationCuesAt(…, PlaybackPhaseElapsed, …)`, `:9321-9335`); il token di danno invece
va a parete (`NativeTick`).

> ⚠️ **«Si conserva quando il playback riparte dopo una finestra» è una premessa falsa per `PlaybackElapsedTotal`,
> misurata per lettura nella review finale del 2026-11-09.** `BeginPlayback(true)` conserva il valore che trova, ma
> alla RIPRESA ci si arriva da `ConcludeResolution` (chiamata dalla risposta alla finestra, in
> `RTTurnManager_Movement.cpp`, con `bIsResolving` ancora vero), che **prima** azzera `PlaybackElapsedTotal`: il
> valore conservato è già zero. Un fumetto acceso prima della finestra, con lo `StartAt` su quel tempo, alla ripresa
> leggeva `Now − Start < 0`, cioè opacità 1 e scala 1,15: ogni fumetto del turno, anche già dissolto, tornava pieno e
> ingrandito. Il fumetto ha quindi un **orologio suo**, `ARTTurnManager::SkillCalloutClock`: avanza dello stesso
> passo e si corregge dello stesso scarto di `Next Action`, ma **non** lo azzera `ConcludeResolution`; torna a zero
> solo all'inizio del playback di un turno nuovo (`BeginPlayback(false)`) e alla fine vera (`FinishPlayback`, dopo
> il ramo sospeso). L'azzeramento condiviso resta com'è perché `PlaybackElapsedTotal` alimenta la barra e
> `Pacing.Close` (TurnLog): la barra che torna a zero dopo una finestra è un difetto preesistente, in §9. Pinnato da
> `RefactorTactics.Playback.SkillCallout.CalloutClockSurvivesTheReactionWindow`.

**Il criterio «skill» esiste nel dato, non va dedotto dal nome.** `FRTActionDef::BaseActionId`
(`Ability/RTActionDef.h:531-544`) dice *«di quale azione generica questa è un profilo … `None` per le azioni
che non sono profilo di niente: le generiche stesse, e le abilità firma»*; il doc-comment scarta
esplicitamente la deduzione dal nome. Ogni attacco base d'eroe lo dichiara (`MakeHeroBasicAttack`,
`Ability/RTHeroCatalogLibrary.cpp:163-168`, `BaseActionId = Action.BasicAttack`, pinnato da
`RefactorTactics.Heroes.BasicAttackDeclaresItsBaseAction`). Le generiche hanno `ActionId` nel catalogo core
(`URTCatalogLibrary::FindCoreAction`, `Ability/RTCatalogLibrary.h:512`). I siti che emettono
`AbilityActivated` sono le coperture (`RTTurnManager.cpp:4567`, `:4676`), le generiche di Prep (`:4788`,
`:4852`, `:4932`), il Dash (`:5487`), le reazioni (`:6258`) e ogni colpo del Blast con `ActionId` (`:7394`).
L'evento porta già `BaseActionId` (`RTResolvedEvent.h:427`).

**Il posto esiste.** Sopra ogni unità vive `ARTUnit::OverlayWidget`, un `UWidgetComponent` in **screen
space** ancorato alla sommità dell'unità, `DrawSize` 220×90 (stato misurato; oggi 220×228, vedi §4), pivot (0.5, 1), che nasce spento e che il velo
accende (`Unit/RTUnit.cpp:176-230`; classe di default `/Game/RT/UI/Match/WBP_RT_UnitOverlay`). Il widget
`URTUnitOverlayWidget` (`UI/RTUnitOverlayWidget.h:34`) porta `NameText`, `HealthBar`, `HealthText`,
`ShieldBar`, `StatusBox` e `DamageTokenText` (`:159-204`): il **token di danno** è il precedente più vicino,
`PushDamageToken` (`UI/RTUnitOverlayWidget.cpp:228-258`) posa un testo già composto, riparte da zero a ogni
evento («un evento, un token»), e `NativeTick` (`:266-291`) lo fa svanire in `DamageTokenSeconds` 0,9 s
(`RTUnitOverlayWidget.h:208`) e poi lo **collassa**, non lo lascia trasparente.

**La regola di privacy è già scritta.** `progettazione-hud.md`: *«il token vive dentro la
sovrapposizione, ed è un requisito di privacy prima che di layout»*: il velo spegne l'intero componente
(`ARTHUD::UpdateObserverVeil` → `ARTUnit::SetKnownToObserver` → `RefreshComponentVisibility` →
`OverlayWidget->SetVisibility(bRender, false)`, `RTUnit.cpp:819`), e *«un widget world-space separato non lo
erediterebbe»* (`docs/technical/systems/progettazione-hud.md:188-193`). In più le attivazioni entrano nel
playback solo se `Ev.SourceVerdict.AllowsTeam(ViewerTeamId)` (`RTTurnManager.cpp:8840-8851`;
`BuildBlastSequence`, `RTPlaybackLibrary.h:455`): un'attivazione nemica che non si vede **non arriva** a
`ShowActivation`.

**Il cablaggio osservabile senza rendering ha un modello.** `ARTUnit::ShowDamageToken`
(`RTUnit.cpp:310-330`) scrive **sempre** il campo `Transient` `LastDamageToken` (`RTUnit.h:2143-2164`),
composto dalla funzione pura `URTHudViewModel::BuildDamageToken` (`UI/RTHudViewModel.cpp:1394`), e aggiorna
il widget **se c'è**; in un mondo headless il widget non c'è e il test legge il campo. Nessun controllo sul
velo in quel punto, «ed è deliberato» (`:326-329`).

**Il nome della skill esiste, cablato.** `FText URTActionData::DisplayName` (`Ability/RTActionData.h:105`),
letto da `ARTUnit::KitDisplayNameFor(ActionId)` (`RTUnit.h:909`, `RTUnit.cpp:1930-1940`); i valori vengono
da mappe C++ `FText::FromString` (`Ability/RTHeroCatalogLibrary.cpp:48-88` per gli eroi,
`Ability/RTCatalogLibrary.cpp:2060-2071` per cinque generiche). Non è localizzabile, e per questa fetta
basta (A2).

**La vista strategica non spegne l'overlay.** `OverlayWidget` dipende solo da `bRender`;
`ARTUnit::bStrategicPresentation` esiste (`RTUnit.cpp:505-520`, scritto dall'HUD a `UI/RTHUD.cpp:998`) ma non
lo governa. Il fumetto deve spegnersi da solo per rispettare D-582.

**Non esiste** nessun widget, asset o issue di tipo fumetto, callout o speech bubble (ricognizione su
`Source/`, `Content/RT/UI/`, `docs/`, `gh issue list --search "fumetto"`: solo #4000, che è il campo
`InterruptedByUnitId`).

---

## 3. Il comportamento

### 3.1 Obiettivo

Durante il playback della risoluzione, nell'istante in cui `ShowActivation` mostra un'attivazione, sopra la
testa dell'unità sorgente compare un **fumetto a grido in stile manga** con il **nome della skill**. Resta
**3 secondi** e sparisce. Vale per unità proprie e avversarie con la visibilità che l'evento già ha. Più
fumetti possono essere vivi insieme su unità diverse.

### 3.2 Regole

| # | Regola | Fonte |
|---|---|---|
| R1 | Il fumetto nasce **solo** da `ShowActivation`, cioè da un `AbilityActivated` che il playback ha deciso di mostrare. Nessun altro produttore. | A1, D-582 privacy |
| R1b | Grida **solo una skill del kit**: `BaseActionId == None` **e** `ActionId` non è un'azione del catalogo core. Quindi **non** gridano l'attacco base (`BaseActionId = Action.BasicAttack`), Guardia, Irrigidimento, Guardia reattiva, Interagisci, le coperture e ogni altra `Action.*`. `Ruling` Un'azione d'eroe che è **profilo** di una generica (`BaseActionId` valorizzato, come l'attacco base) non grida: è la generica con un nome. Costo se sbagliato: un eroe futuro con una «Guardia» firmata non grida. Il criterio è una funzione **pura** sui due `FName` più un bool «è core», con test. **Nessun campo nuovo** in `FRTActionDef`. ⚠️ *Anche l'azione di un pezzo d'equipaggiamento non grida: nota sotto la tabella.* | A7 |
| R2 | Il testo è il nome leggibile dell'azione: `KitDisplayNameFor(ActionId)`; se vuoto, `KitDisplayNameFor(BaseActionId)`. | A2 |
| R3 | `Ruling` Se anche quello è vuoto, il fumetto mostra l'**ultimo segmento** di `ActionId` (per `Hero.Aevik.ArcPulse` → `ArcPulse`) e scrive una warning che nomina l'id. Costo se sbagliato: un identificatore inglese a schermo; l'alternativa, nessun fumetto, è un buco che nessun test vede, ed è la forma che il progetto scarta (`ResolveIcon`, `RTUnitOverlayWidget.h:148-156`). | — |
| R4 | Durata **3,0 s di tempo del playback** dalla comparsa, poi il fumetto è **collassato**, non trasparente (come il token, `RTUnitOverlayWidget.cpp:285-291`). L'orologio è `ARTTurnManager::SkillCalloutClock` (~~`PlaybackElapsedTotal`~~, che una finestra di reazione azzera: nota sotto la tabella), che avanza dello stesso passo del playback: alla comparsa si registra l'istante di partenza, e a ogni fotogramma la vita del fumetto è `Now − Start`. In **pausa** e nel **trattenimento** di una finestra il fumetto resta com'è; nello slow-motion dura di più a parete; uno step di `Next Action` lo fa avanzare dello scarto mostrato. Il valore 3,0 è una proprietà `EditDefaultsOnly` del widget; `0` lo spegne. Anche il pop di comparsa e la dissolvenza (§5) corrono su questo orologio. | A1, A8 |
| R5 | `Ruling` Un'unità porta **un solo fumetto**: una seconda attivazione entro i 3 s **sostituisce** la prima e riparte da zero (specchio di `PushDamageToken`). Costo se sbagliato: nel Dash un'unità che attiva due skill nello stesso turno mostra solo la seconda; con `AttackShowSeconds` 0,5 s la prima è stata visibile almeno mezzo secondo. | — |
| R6 | Più unità possono avere il fumetto vivo nello stesso istante. Nessuna coda, nessun rallentamento del playback, nessuna de-sovrapposizione. | A5 |
| R7 | `Ruling` Alla **fine vera del playback** ogni fumetto ancora vivo viene **tagliato**: in `ARTTurnManager::FinishPlayback` (`RTTurnManager.cpp:10052`) **dopo** il ramo `IsResolutionSuspended()` (`:10064-10105`), che è la fine di un **prefisso** su una finestra di reazione e non del turno, e lì il playback resta trattenuto e poi riparte con `BeginPlayback(true)`. Il fumetto dell'attivazione nemica che ha aperto la finestra resta quindi **fermo e visibile** mentre il giocatore decide (R4: l'orologio non avanza), ed è l'informazione su cui decide. Vale anche per il Skip. In pianificazione non c'è mai un fumetto. Costo se sbagliato nel verso opposto: un grido che resta sopra l'unità mentre il giocatore pianifica il turno dopo. | A1, A8 |
| R8 | Nella **vista strategica** il fumetto è **collassato** per tutta la durata della vista; tornando alla vista ravvicinata entro i 3 s ricompare per il tempo che resta. Il log a destra porta già la riga `Attiva:` (`ShowActivation`), quindi l'informazione nella vista strategica c'è senza lavoro nuovo. | D-582 (3) |
| R9 | Il fumetto **vive dentro `WBP_RT_UnitOverlay`**, mai in un componente separato: eredita il velo per costruzione. | progettazione-hud.md:188-193 |
| R10 | Il fumetto **non decide nulla**: nessun campo entra in `MapState`, snapshot, TurnLog o `StateHash`. Il tempo che consuma è quello del playback, che è già dichiarato presentazione e mai decisione (invariante #1). | AGENTS.md §3 |
| R11 | `Ruling` Se `EmitAbilityActivated` viene emesso anche per un intento che poi risulta interrotto o bloccato, in questa fetta il fumetto **compare lo stesso**: è lo stesso beat su cui già suona la clip `Cast`, e distinguere i due casi è la fetta «azioni bloccate» (§9). Il builder **misura** se il caso esiste (`RTTurnManager_Blast.cpp`, `ApplyInterrupts`) e lo scrive nella DoD. Costo se sbagliato: un grido per una skill poi interrotta, coerente con la clip che già parte. | A6 |

> ⚠️ **R8, misurato il 2026-11-09: la premessa è falsa.** In vista strategica il fumetto si collassa come la regola
> dice, ma il Registro a destra **non** porta la riga `Attiva:`. La stringa va solo nel log del motore e in
> `RecentEvents` (`ShowActivation` → `AddLogEvent`); `GetRecentEventsForTeam` non ha nessun chiamante nei sorgenti
> non di test (`git grep -n GetRecentEventsForTeam -- Source` fuori dai `Tests/`); il Registro si compone dal
> `TurnLog` tipizzato, e il canale testuale «non entra in questa catena» (`UI/RTHudViewModel.h:1360-1361`). Quindi
> l'informazione che D-582 (3) vuole nella vista strategica oggi non c'è: seguito
> [#4033](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4033) (seduta visiva, punto (e):
> [evidenza](../../technical/evidence/4025/seduta-pilotata-2026-11-09.md)).
>
> 🔎 **R11, misurato** (sul codice): un **attacco** interrotto non emette `AbilityActivated` e non grida (`ApplyInterrupts` → `InterruptedIntents`, da cui `EmitAttackIntentActivation` esce), uno degradato grida; una **cura** (`DeferredActivations`) non si attiva se il guaritore è stordito, e si attiva se è interrotto (una skill del kit, quindi, grida lo stesso), perché l'interruzione cerca le vittime solo fra gli intenti d'attacco. Le azioni bloccate come fumetto sono [#4034](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4034).
>
> ⚠️ **R1b, review finale del 2026-11-09: l'equipaggiamento gridava.** `URTCatalogLibrary::MakeEquipmentAction`
> scrive `ActionId = EquipmentId` (`Gadget.*`, `Reaction.*`) e copia dal core un `BaseActionId` vuoto; `FindCoreAction`
> non conosce quell'id. Per le due condizioni di R1b un gadget era quindi una skill di firma, e con il loadout di
> default (`EquipLoadout(DefaultLoadoutFor(…))`, come in partita) Muiren gridava «SPRINKLER» e Branth «COPERTURA
> PORTATILE», contro A7 («solo le skill del kit») e contro il `Ruling` di R1b («la generica con un nome» non grida).
> Corretto: grida solo un'azione che **non** è né core né concessa da un pezzo d'equipaggiamento
> (`URTCatalogLibrary::FindEquipment(ActionId) != nullptr`); il bool di `ActionShouts` si chiama ora
> `bIsGenericOrEquipment`. Pinnato da `RefactorTactics.Playback.SkillCallout.EquipmentActionDoesNotShout` (il loadout
> vero di Branth) e da un caso in `HudViewModel.SkillCallout.ShoutsOnlyForASignatureSkill`. Costo se sbagliato: se
> l'autore vuole che un gadget gridi, è una riga.
>
> ⚠️ **R4, review finale del 2026-11-09: l'orologio è `SkillCalloutClock`.** `PlaybackElapsedTotal` torna a zero alla
> ripresa dopo una finestra di reazione (nota in §2), e i fumetti accesi prima tornavano pieni e ingranditi. Pausa,
> trattenimento, slow-motion e `Next Action` valgono per il nuovo orologio come scritto in R4, perché ne segue passo e
> correzione; cambia solo che sopravvive alla finestra.

### 3.3 Non-obiettivi

- Informazioni sulla skill (descrizione, costo, elemento): «magari» dell'autore, fuori (D-582).
- Azioni bloccate come fumetto: fetta successiva (§9).
- Localizzazione dei nomi: i nomi restano le mappe C++ di oggi.
- Cut-in, freeze-frame, onomatopee del «Master GDD» (D-591): input di ricerca, non requisito.
- Cambiare la cadenza del playback (A5).
- Replay Viewer: ha un tick proprio (`Replay/RTReplayScreenWidgets.cpp:232`) e non consuma
  `AbilityActivated`; resta fuori.

### 3.4 Anti-obiettivi (cosa lo rende un fallimento anche se funziona)

- Un fumetto su un'unità avversaria che il giocatore **non vede**: violazione di privacy (D-223, AGENTS.md §4).
- Un fumetto che **copre** la barra della vita o il nome della stessa unità.
- Un fumetto «enorme» (A3): deve stare nella scala del widget che già c'è.
- Un fumetto che resta in **pianificazione** (R7).
- Una seconda fonte di verità: il widget che legge lo stato dell'unità invece di ricevere l'evento.

---

## 4. Dove si aggancia

Stessa strada del token di danno, un canale in più. Nomi proposti, il builder può cambiarli se trova un
precedente più adatto.

```
ARTTurnManager::ShowActivation(Ev)                              Turn/RTTurnManager.cpp:9039
  └─ se URTHudViewModel::ActionShouts(Ev.ActionId, Ev.BaseActionId, bIsCore)   NUOVO, pura (R1b)
       Src->ShowSkillCallout(Ev.ActionId, Ev.BaseActionId, SkillCalloutClock)    NUOVO, Unit/RTUnit.{h,cpp}
         ├─ LastSkillCallout = URTHudViewModel::BuildSkillCallout(DisplayName, ActionId, StableUnitId, StartAt)
         │     (campo Transient, SEMPRE scritto: osservabile headless)            NUOVO, UI/RTHudViewModel
         └─ se c'è il widget: Overlay->PushSkillCallout(LastSkillCallout)        NUOVO, UI/RTUnitOverlayWidget
               ├─ SkillCalloutText / SkillCalloutRoot (BindWidgetOptional)          WBP_RT_UnitOverlay (asset)
               └─ SkillCalloutSeconds = 3.0 (EditDefaultsOnly)
ARTTurnManager::TickPlayback (ogni fotogramma in cui l'orologio avanza)  RTTurnManager.cpp:9562
  └─ per ogni unità con fumetto vivo: Overlay->SetPlaybackClock(SkillCalloutClock)      NUOVO (R4)
       └─ il widget: Alpha = SkillCalloutAlpha(Now − Start, Seconds) (pura) → opacità/scala → Collapsed
          collassato finché bStrategic (R8)
ARTTurnManager::FinishPlayback(), DOPO il ramo IsResolutionSuspended()   RTTurnManager.cpp:10105+
  └─ per ogni unità: ClearSkillCallout()                          NUOVO (R7)
ARTUnit::SetStrategicPresentation(bStrategic)                    RTUnit.cpp:505
  └─ Overlay->SetStrategicPresentation(bStrategic)                NUOVO (R8)
```

> ⚠️ Fino alla review finale del 2026-11-09 questa sezione e §8 nominavano `PlaybackElapsedTotal` come orologio del
> fumetto; ora nominano `ARTTurnManager::SkillCalloutClock` (nota in §2). La regola «una sola fonte, nessun
> `DeltaTime` locale» resta: la fonte è quell'orologio.

- **`FRTSkillCalloutView`** (nuova struct, `UI/`): `SourceStableUnitId`, `ActionId`, `Label` (FString già
  composto), `bFromFallback` (vero se R3 è scattata), `StartAt` (secondi di `SkillCalloutClock` alla
  comparsa). Composta da una funzione **pura** con un test che chiama senza widget, come `BuildDamageToken`
  (`Tests/RTHudViewModelTests.cpp:1145`).
- **Il tempo lo porta il TurnManager, il widget non lo misura.** È il modello delle cue (`PushPlaybackCues`),
  non quello del token: il widget riceve `Now` e calcola l'alpha con una funzione pura; se nessuno gli porta
  il tempo, il fumetto resta fermo, che è il comportamento voluto in pausa. Come `Now` arrivi al widget
  (spinta diretta dal TurnManager alle unità con fumetto vivo, oppure campo in `FRTUnitOverlayView` posato
  da `ARTHUD::UpdateObserverVeil` leggendo il TurnManager) è del builder (§8); la regola è **una sola fonte**,
  `SkillCalloutClock`.
- **`ActionShouts`** riceve «è core» come bool calcolato dal chiamante con `URTCatalogLibrary::FindCoreAction`
  (o un predicato equivalente già esistente): la funzione pura non dipende dal catalogo e si testa a tavolino
  sui tre casi (skill firma, attacco base, generica).
- **Nessun controllo sul velo in `ShowSkillCallout`**, per la stessa ragione dichiarata in
  `ShowDamageToken` (`RTUnit.cpp:326-329`): il velo lo decide `UpdateObserverVeil` spegnendo l'overlay.
- **Il `DrawSize` 220×90 non basta** per il fumetto sopra nome e barra: si alza a 220×228. **Misurato** nella
  seduta visiva di `#4025`, a 1920×1080: a 200, con fumetto, token di danno e una riga di stati insieme, il blocco
  scendeva di 18 px per ~0,9 s; a 228 il salto è 0. Il contenuto di ≈ 218 px è **dedotto** (200 + 18), non
  misurato; la stima da metriche dei font, ≈ 190, era troppo bassa; a 1440p e a 4K non c'è misura. Col pivot (0.5, 1) il widget
  cresce verso l'alto, e il `TopSpacer` in Fill del WBP impila il contenuto in basso: rispetto al layout
  impilato in alto l'ancoraggio cambia una volta, nome e barra stanno a distanza fissa sopra la testa, il
  fumetto comparendo sale senza spostare la barra, e una riga di stati in più spinge su nome e fumetto.
  `Tests/RTHudOverlayClampTests.cpp` non dipende dal valore.
- **L'asset** `WBP_RT_UnitOverlay` si modifica **in Editor, dal clone principale** (CLAUDE.md §10); il C++
  deve restare completo anche se il WBP non porta i nuovi elementi (`BindWidgetOptional`, degrado
  dichiarato come per `DamageTokenText`).

---

## 5. Aspetto

Tutto §5 è `Ruling` salvo A2, A3 e lo stile manga (D-582, D-590). L'autore vedrà il risultato in PIE e
correggerà lì.

- **Forma**: fumetto a grido, contorno a punte irregolari (il «burst» dei manga), riempimento chiaro,
  contorno scuro spesso. Una coda corta che punta verso il basso, all'unità.
- **Testo**: solo il nome, su una riga, **Orbitron Bold** (`Content/RT/UI/Fonts/F_RT_Orbitron_Bold`: la
  style guide lo riserva a header e «valori importanti», `progettazione-hud.md:1798-1812`), maiuscolo, colore
  scuro. Se il nome non entra su una riga alla dimensione scelta, il testo si riduce (auto-size), non il
  fumetto si allarga oltre il widget.
- **Squadra**: il contorno prende il colore di squadra già usato per `NameText` (azzurro alleato, rosso
  avversario, `RTHudViewModel.cpp:607-610`), così chi grida si legge a colpo d'occhio anche con più fumetti
  vivi (A5). Costo se sbagliato: un elemento in più da tenere coerente con la palette `URTUIPalette` (D-489).
- **Misura** (A3): larghezza al massimo quella del widget, altezza di una riga di testo più i bordi;
  posizionato **sopra** `NameText`, mai sopra la barra.
- **Animazione**: comparsa secca con una scala da 1,15 a 1,0 in ~0,1 s (il «pop» del grido), tenuta, poi
  dissolvenza negli ultimi ~0,4 s dei 3 s. Tutto funzione di `Now − Start` sull'orologio del playback (R4),
  nessuna `UMG Animation` obbligatoria.
- **Niente audio** in questa fetta.

---

## 6. Vincoli

- **Determinismo**: solo presentazione. Niente `DeltaTime` nel resolver, nessun campo in hash, snapshot o
  TurnLog. Riprodurre lo stesso turno due volte riscrive `LastSkillCallout` con lo stesso valore.
- **Privacy**: R1 + R9. Il builder **non** aggiunge un secondo giudizio di visibilità.
- **Autorità**: nessun cambiamento a simulazione, resolver, eventi o loro ordine.
- **Asset**: il WBP si tocca solo in Editor, dal clone principale; mai come testo.
- **Motore uno per macchina**: PIE e suite si eseguono nella finestra del protocollo AGENTS.md §11.

---

## 7. Verifica

| Gate | Cosa prova | Stato atteso alla PR |
|---|---|---|
| Automation, funzioni pure | `ActionShouts`: vero per una skill firma (`Hero.X.Y`, `BaseActionId` None, non core), falso per l'attacco base (`BaseActionId = Action.BasicAttack`), falso per una generica core (`Action.Guard`). `BuildSkillCallout`: nome dal kit; fallback a `BaseActionId`; fallback R3 con `bFromFallback`; `StableUnitId` conservato (`0` = nessuno, D-063); `StartAt` conservato. `SkillCalloutAlpha`: 1 nel pieno, 0 a `Seconds`, `Seconds <= 0` → 0 (come `FadeAlpha`). | `PASS` |
| Automation, cablaggio headless | In una partita di test, dopo un playback con un `AbilityActivated` visibile di una **skill**, `LastSkillCallout` della sorgente porta quell'`ActionId` e uno `StartAt` coerente con l'orologio; un'attivazione **filtrata** dal verdetto non lo scrive; un **attacco base** mostrato non lo scrive. Modello: i test che leggono `LastDamageToken` (`Tests/RTHealAreaTests.cpp`, `Tests/RTMatchAutobattleTests.cpp`). | `PASS` |
| Automation, suite intera | Nessuna regressione (`FindMissingBindings` in `RTPresentationBinding.cpp:273` se il builder registra il nuovo consumatore). | `PASS` |
| Compile | Target Editor. | `PASS` |
| PIE | Una partita in cui: (a) una skill propria mostra il fumetto per circa 3 s sopra il nome; (b) una skill nemica **visibile** lo mostra; (c) un nemico **non osservato** non mostra nulla; (d) due unità attivano nello stesso Blast e i due fumetti sono vivi insieme; (e) in vista strategica il fumetto sparisce e la riga `Attiva:` sta nel log; (f) a fine playback nessun fumetto resta in pianificazione; (g) un **attacco base** non grida; (h) con ESC durante il playback il fumetto resta fermo e riprende con il playback; (i) se una skill nemica apre una finestra di reazione, il suo fumetto resta visibile per tutta la finestra. Registrata in `editor-sessions.yaml`. | `PASS` con catture |
| Determinism / Replay / Privacy | Solo presentazione; nessun golden cambia. Il builder lo dimostra con la suite verde, non lo dichiara. | `N/A` con evidenza |
| Packaged | Non richiesto a questa fetta; `G17` lo misura sul pacchetto. | `NOT RUN` dichiarato |

> ⚠️ **Riga PIE, misurata nella seduta visiva del 2026-11-09** (`U99`;
> [evidenza](../../technical/evidence/4025/seduta-pilotata-2026-11-09.md)): (e) è `FAIL` nella seconda metà, per la
> premessa falsa di R8 (seguito [#4033](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4033)); (h)
> com'è scritta non si può avere, perché `ESC` non ferma il playback (nota e `Ruling` sotto A8): si prova con la
> pausa del playback (`K`), ed è `PASS`. Gli esiti di tutti i punti stanno nella voce `PIE-HUD-FUMETTO` di
> [`test-manuali-pie.md`](../../technical/test-manuali-pie.md).
> ⚠️ (i) misura il fumetto **durante** la finestra e la pianificazione dopo. Ciò che fa **dopo** la chiusura, quando
> resta playback da mostrare, a schermo è `NOT RUN`: la seduta precede la correzione dell'orologio (nota in §2), e il
> comportamento lo copre il test headless `Playback.SkillCallout.CalloutClockSurvivesTheReactionWindow`.

Chi scrive il codice **non firma** il verdetto PIE sui punti (b) e (c): li rimisura una seconda passata su
commit dichiarato (CLAUDE.md §6, §10).

---

## 8. Lasciato al builder

- Nomi esatti di struct, funzioni e binding, purché seguano i precedenti del token.
- Se alzare `DrawSize` e di quanto; la posizione precisa del fumetto rispetto a `NameText`.
- Come il widget conosce la vista strategica (chiamata da `SetStrategicPresentation` o campo in
  `FRTUnitOverlayView`): l'unica regola è che **non legga** lo stato da sé a ogni tick da fonti diverse.
- Come l'orologio del fumetto (`SkillCalloutClock`) arriva al widget a ogni fotogramma (§4): spinta dal TurnManager o campo nella
  vista dell'overlay; una sola fonte, nessun `DeltaTime` locale per la vita del fumetto.
- Come il chiamante stabilisce «è core» per `ActionShouts` (`FindCoreAction` o un predicato già esistente nel
  catalogo).
- Se registrare il nuovo consumatore in `RTPresentationBinding.cpp` per `AbilityActivated`.
- La curva esatta di pop e dissolvenza (§5 dà i valori di partenza).
- Il disegno del burst: immagine (`T_RT_*` in `Content/RT/UI/Match/`) o `UBorder` con brush; il budget è
  un asset, non un sistema.

---

## 9. Follow-up candidates (fuori da questa fetta)

- **Azioni bloccate come fumetto** (D-582 (3), seconda metà della domanda `fumetti`): richiede un evento o
  una voce per l'intento interrotto/cancellato e un testo («Bloccato!», o il nome barrato); lo stesso canale
  di §4 con una `FRTSkillCalloutView` diversa. ➕ *Aperta come
  [#4034](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4034); la risposta misurata a R11 è sotto
  §3.2.*
- ➕ **La riga `Attiva:` nel Registro a destra** in vista strategica (D-582 (3)): la premessa di R8 era falsa, e
  l'informazione oggi non c'è. Aperta come [#4033](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4033).
- ➕ **Un curatore interrotto si attiva lo stesso** (`ApplyInterrupts` cerca le vittime solo negli intenti
  d'attacco; R11, sotto §3.2), quindi la sua skill grida: da verificare col dominio Combat se è voluto. È già scritto
  in [#4034](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4034), come domanda da chiarire prima di
  decidere che cosa grida.
- ➕ **La barra di avanzamento del playback torna a 0 dopo una finestra di reazione** (`ConcludeResolution` azzera
  `PlaybackElapsedTotal` anche sulla ripresa): difetto preesistente, fuori fetta. Per lettura, lo stesso valore arriva
  a `Pacing.Close` e alla riga `Risoluzione completata (…s)` in `ConcludeTurn`, che dopo una finestra contano solo il
  tratto dopo la ripresa: la correzione tocca la misura del pacing e va fatta col dominio che la possiede. Il fumetto
  non ne dipende più (nota in §2).
- **«Qualche grido»**: un testo per skill oltre al nome (onomatopea, frase dell'eroe), in `URTActionData`.
- **Localizzazione** dei nomi delle azioni (oggi `FText::FromString`).
- **Leggibilità tattica**: la previsione dell'autore «vista la grandezza degli esagoni» si misura con
  `leggibilita-tattica` sul pacchetto; se due fumetti adiacenti si coprono, de-sovrapposizione o riduzione.
- Cut-in / freeze-frame / audio del grido (input D-591).

---

## 10. Owner, issue, posto nel piano

- **Owner primario**: Domain 08 UI, UX & Coordination (il widget e la vista); supporto Domain 07
  Animation & Presentation (il beat). Il CR di riferimento è CR-PRESENT
  (`docs/roadmap/capability-roadmaps.md:193-211`).
- **Issue**: da aprire, una sola, sotto `G17` ondata 2 (#3997 esegue il gate); titolo proposto
  `[V0.1][UI] Il fumetto della skill: un grido manga sopra l'unità all'attivazione (D-582 (3), fetta 1)`.
  Nel corpo: questa spec, le regole R1–R11, la tabella §7 come DoD. ➕ *Aperta come
  [#4025](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4025).*
- **Branch**: dal nome della issue, PR verso `main`.
