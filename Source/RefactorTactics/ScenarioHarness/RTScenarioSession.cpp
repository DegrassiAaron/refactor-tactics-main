#include "ScenarioHarness/RTScenarioSession.h"

#include "ScenarioHarness/RTScenarioArena.h" // l'arena la costruisce chi la costruisce anche per l'authoring
#include "Turn/RTMatchStateHash.h"
#include "Turn/RTTurnLogLibrary.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // MapResolutionPhase: un'azione di Prep non ha un bersaglio da dichiarare
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h" // HoldResponse/FireResponse: la traduzione nel token del gioco
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnRules.h"
#include "Unit/RTUnit.h"
#include "Player/RTPlayerController.h"
#include "RefactorTactics.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

namespace
{
	/**
	 * Arena esagonale piena di raggio N, piu' le celle modificate dallo scenario.
	 *
	 * RIUSA l'actor mappa gia' presente se c'e': lo stesso codice gira in un mondo vuoto (test) e in una PIE
	 * dove il GameMode ha gia' allestito. Spawnarne un secondo darebbe due griglie sovrapposte.
	 */
	/**
	 * Le capability che il gioco possiede **oggi**. Un turno che ne chiede una assente non viene giocato, e
	 * lo scenario si dichiara `Blocked` invece di fallire.
	 *
	 * L'elenco sta nel CODICE e non nei dati: se stesse nello scenario, dichiarare disponibile una capability
	 * inesistente sarebbe una modifica al JSON, e il primo scenario verde e bugiardo arriverebbe da li'.
	 */
	const TSet<FString>& AvailableCapabilities()
	{
		static const TSet<FString> Available = {
			TEXT("FixtureReference"),  // lo scenario riferisce la geometria per nome
			// E5: reazioni componibili che scattano o non scattano — il regime `AllowedResponses <= 1` di
			// ADR-0004 §2, e **solo quello**.
			//
			// ✅ **Divisa con `#512` il 2026-08-16, alla condizione che questa riga si era scritta da sola.**
			// Diceva: *«il giorno in cui `ResolveCombat` chiamera' `BuildOverwatchTriggers`, questa riga smette
			// di essere vera e va divisa»*. Quel giorno e' arrivato con CP 14.5. Il grep che il commento
			// nominava — `grep -rn "BuildOverwatchTriggers" Source/ --include=*.cpp | grep -v /Tests/` — da'
			// oggi **cinque** righe, e `RTTurnManager.cpp:5093` e' una chiamata di PRODUZIONE, non un commento.
			// L'affermazione «nessun chiamante» era l'unica parte scaduta.
			//
			// La divisione e' fra i due nomi che esistono gia', non con un terzo: la DECISIONE su
			// un'opportunity a due risposte e' `DecisionBoundary`. Misurato che i tre scenari che chiedono
			// `Reaction` — `Combat/CounterStrikesBack`, `Visual/Reaction/Deflection`,
			// `Visual/Reaction/Interposition` — sono tutti nel regime `<= 1`: un nome nuovo li costringerebbe a
			// cambiare senza che cambi cio' che chiedono.
			//
			// Pinnata da `Scenario.ReactionAndDecisionBoundaryAreDistinct`, che chiede **entrambe** le domande
			// — noto e disponibile — perche' un nome noto e indisponibile e' precisamente l'altro caso.
			TEXT("Reaction"),
			TEXT("Environment"),       // E8: superfici, stati, propagazione
			TEXT("Cover"),             // E9 CP 9.1/9.2: coperture bassa e alta, distruzione
			TEXT("Structures"),        // E9 CP 9.3: porte come bordo, revisione della mappa
			TEXT("CreateCover"),       // E9 CP 9.5: coperture erette in partita, temporanee, spostabili
			// D-046 (#282): un EROE possiede davvero un'azione ambientale. Non basta che il resolver la sappia
			// risolvere — per mesi la sapeva, e nessuna unita' poteva innescarla. Oggi `Gadget.ConductiveNode` e'
			// `Action.Electrify` e `Phase.FluidTrail` e' `Action.CreateWater`.
			//
			// NON copre `Action.Ignite` ne' `Action.ModifyArc`: nessun eroe del roster le possiede, e D-046 ha
			// deciso che restino senza owner in v0.1 (nessuna affinita' col fuoco; i ponti non appartengono a
			// nessun kit). Uno scenario che chieda di accenderle deve restare BLOCKED, ed e' il motivo per cui
			// questa capability non si chiama «Environment»: quella c'e' gia' e dice un'altra cosa.
			TEXT("EnvironmentalActionOwner"),
			// E18 CP 18.2 (D-016): `Wraith.InterceptShot` e' una Predictive Action — cella dichiarata in
			// Planning, verificata al boundary del Move, nessun input durante la Resolution.
			//
			// ✅ **La motivazione di questa riga e' tornata VERA il 2026-08-13 sera, e vale la pena dire come.**
			//
			// Diceva: «un canale che il gioco ha gia'». Misurato la mattina dello stesso giorno, era FALSO:
			// `PlannedAttackCell` e `bAttackTargetsCell` erano letti dal `TurnManager` e scritti SOLO da
			// questo file e dai test — il `PlayerController` sapeva dichiarare un bersaglio-UNITA' e un
			// percorso, mai una cella. L'harness era quindi il primo produttore, esattamente l'asimmetria che
			// tiene fuori `DeclaredRotation` piu' sotto, e il verde NON diceva qualcosa di vero sul giocatore.
			// La capability era rimasta disponibile per SCELTA — quattro scenari la usano su regole vere — col
			// debito tracciato su `#737`.
			//
			// `#737` e' atterrata: `ARTPlayerController::HandleTargetCell` scrive i due campi da
			// `ERTPointerContext::Targeting` con `TargetKind::Cell`, coperto da
			// `PlayerInput.TargetCellProducesPlannedAttackCell`. Il comando che lo verifica:
			//
			//   grep -rn "PlannedAttackCell *=\|bAttackTargetsCell *=" Source/ \
			//     | grep -v "RTScenarioSession\|RTTurnManager\|RTUnit.h\|Tests/"
			//
			// Ora stampa il produttore. Il verde di questi scenari dice qualcosa di vero sul giocatore, e
			// questa riga non e' piu' un debito. Owner: `docs/technical/systems/spec-pointer-interaction.md` §2.2.
			TEXT("PredictiveAction"),
			// CP 13.5 (#160): un'unita' dello scenario puo' essere guidata dal BOT invece che dal file.
			//
			// Si dichiara disponibile per la stessa ragione di `PredictiveAction` e non per quella che tiene
			// fuori `ReactionPlanning`: qui l'harness NON e' il primo produttore. `ARTTurnManager::PlanBots()`
			// gira in ogni partita 2v2, e l'harness lo raggiunge da `PlanBotsForTest()` — l'appiglio che il
			// runner dichiara da sempre come l'unico. Il verde dice quindi qualcosa di vero sul gioco.
			//
			// ⚠️ Non e' il seam dei `DecisionProvider` (D-101, #542): quello serve quando i modi di giocare uno
			// scenario diventano tre. Qui resta uno — file per gli umani, pianificatore per i bot, che e' la
			// composizione della v0.1.
			TEXT("BotPlanning"),
			// `#601`: da oggi `PlannedReactionAbility` ha un produttore che non e' un test — il giocatore lo
			// scrive da `ARTPlayerController::SelectAbilityForCurrent` (slot proprio, nessun targeting: chi
			// subira' la reazione lo decide il trigger) e il bot da `PlanBots`, che arma quella che ha.
			//
			// Finche' quel produttore non c'era, questa riga sarebbe stata una bugia utile a far passare gli
			// scenari: i loro verdi avrebbero detto che il giocatore puo' preparare una parata quando non poteva.
			// Ora dicono il vero, e il perimetro resta lo stesso — dichiarare in pianificazione, non decidere in
			// una finestra: quella e' E14, e non passa da qui.
			TEXT("ReactionPlanning"),
			// `#291`/`#737`: dichiarare una ROTAZIONE in pianificazione (D-020). E' USCITA dall'elenco dei non
			// disponibili il 2026-08-13 sera, ed e' l'ultima delle tre a farlo — le altre due sono
			// `ReactionPlanning` (`#601`) e `PredictiveAction`.
			//
			// Il criterio e' sempre lo stesso: la capability entra quando il campo ha un produttore che NON e'
			// l'harness. Qui e' `ARTPlayerController::HandleFacingSector`, che scrive `PlannedFacing` e
			// `bDeclaresPlannedFacing` dal contesto `ERTPointerContext::Facing` chiedendo la legalita' a
			// `URTFacingLibrary` e RIFIUTANDO una dichiarazione illegale invece di correggerla
			// (`PlayerInput.FacingSectorProducesPlannedFacing`, `PlayerInput.IllegalFacingIsRejectedNotCorrected`).
			//
			// ⚠️ Il perimetro e' `facing` come MOSSA, non come piazzamento: `FRTScenarioUnit::Facing` c'e' da
			// sempre ed e' un'altra cosa. Restano fuori l'insieme legale mostrato a schermo (`#613`) e il bot,
			// che non dichiara rotazioni: nessuno dei due e' un prerequisito di questa chiave, perche' il
			// giocatore un modo di chiederla ce l'ha.
			TEXT("DeclaredRotation"),
			// `#512` fase B: SOSPENDERE la risoluzione a un decision boundary e far rispondere qualcuno. E'
			// l'ultima delle quattro a uscire dai non disponibili, e per lo stesso criterio delle altre tre —
			// il campo ha un produttore che NON e' l'harness: `ARTTurnManager::AskReactionDecision`, chiamato
			// da `ARTTurnManager::ResolveReactionBoundary` dentro la risoluzione di ogni partita e non solo
			// negli scenari. ⚠️ Il simbolo e non il numero di riga: la prima stesura di questo commento diceva
			// `RTTurnManager.cpp:5093`, che e' il chiamante di `BuildOverwatchTriggers` — vero di un'altra
			// funzione, nello stesso file, a settanta righe di distanza.
			//
			// ⚠️ **La fase A l'ha tenuta fuori di proposito, e la ragione va letta prima di spostare la
			// prossima.** Scoprirla senza i DATI che la rendono rispondibile non produce un verde: produce
			// turni con finestre a cui nessuno risponde, cioe' `HOLD` per timeout dove lo scenario si aspetta
			// un `FIRE`. La capability e le `decisions` dei tre scenari che la chiedono atterrano nello
			// stesso commit — separarle sarebbe stato un rosso a giorni alterni.
			//
			// ⚠️ Il perimetro e' la finestra a UNA risposta legale per partecipante. Restano fuori il profilo
			// di reazione a due risposte (`ReactionProfile`) e l'opportunity contested (`ReactionClash`), che
			// sono E14.7 e stanno nell'elenco qui sotto: dichiarare disponibile la finestra non dichiara
			// disponibile la scelta.
			TEXT("DecisionBoundary"),

			// ➕ **`ReactionProfile` SCENDE qui il 2026-08-19 con la fetta 4 di `#314`**, e le tre condizioni
			// che la riga di sopra prescriveva sono soddisfatte tutte e tre — verificate, non assunte:
			//   · il **runtime** esiste: `RTTurnManager_Blast.cpp`, ramo `Status.Braced`, costruisce
			//     l'opportunity da `URTCatalogLibrary::BraceExecutableResponses(Unit->ReactionProfileId)` e
			//     chiede la decisione. Pinnato da `Reactions.Brace.ProfileDecidesInPlay`;
			//   · i **DATI per rispondere** esistono: `decisions.respond` accetta le risposte di profilo da
			//     questa stessa fetta. E' la lezione scritta quattro righe piu' su — scoprire una capability
			//     senza i dati che la rendono rispondibile produce finestre a cui nessuno risponde — e qui
			//     capability e vocabolario atterrano nello **stesso commit**, come fece la fase A;
			//   · lo scenario **esegue davvero cio' che descrive**: `Spec.Brace.ProfileChangesResponse` non ha
			//     piu' un turno con `intents: []`, e la sua `decisions` risponde `SIDESTEP` a una finestra che
			//     si apre per davvero. Senza questo, scoprirla avrebbe fatto passare da `Blocked` a verde un
			//     turno vuoto — il difetto esatto che la nota di `ReactionProfile` nell'elenco di sotto
			//     registrava, e che sarebbe stato ironico ripetere chiudendola.
			//     🔴 **Questa riga diceva «il T2 e' pieno» e il T2 NON ESISTE PIU'**: lo stesso commit che la
			//     scriveva ha ridotto lo scenario a **un turno solo**, perche' `Action.Brace` dura 1 turno e ha
			//     cooldown 1 — al T2 lo stato e' scaduto e l'azione non e' ridichiarabile. Il commento
			//     descriveva il piano, non il file; chi fosse andato a cercare quel T2 per riderivare la
			//     condizione non avrebbe potuto dire se fosse soddisfatta o abbandonata in silenzio.
			//
			// ⚠️ **`ReactionClash` NON scende, e la riga di sotto che dice «chi la chiude sposta ENTRAMBI i
			// nomi» va letta come prescrizione, non come misura**: sposta entrambi chi chiude `#314`, e questa
			// fetta non la chiude. Il Clash ha oggi le sue funzioni pure — `ResolveContestedBoundary`,
			// `MakeClashLogEntries` — e **nessun punto del resolver le chiama**: un turno che chiedesse
			// `ReactionClash` non troverebbe niente da eseguire.
			TEXT("ReactionProfile"),
		};
		return Available;
	}

	/**
	 * L'altra meta' del VOCABOLARIO: i nomi che uno scenario puo' legittimamente chiedere e che il gioco non
	 * sa ancora fare. Un turno che ne chiede uno resta `Blocked`, come sempre.
	 *
	 * ⚠️ Perche' esiste come DATO e non piu' come prosa. Fino a qui l'elenco dei non disponibili viveva nel
	 * commento qui sotto, e nessuno poteva interrogarlo: un nome **sbagliato** — `DecisionBoundry` per
	 * `DecisionBoundary` — produceva un `Blocked` identico a quello di un'attesa legittima, quindi uno
	 * scenario con un refuso restava bloccato per sempre senza che nulla lo segnalasse. Con i due insiemi
	 * separati, «non lo so ancora fare» e «questo nome non esiste» smettono di avere lo stesso esito:
	 * `RefactorTactics.Scenario.UnknownCapabilityIsErrorNotBlocked`.
	 *
	 * ⚠️ La prosa diceva **sei**, e gli scenari in repo ne chiedono **nove**. Le tre che mancavano —
	 * `SpatialTrigger`, `SemanticTrigger`, `Teleport` — non erano un'omissione da poco: erano gli unici tre
	 * nomi che un test non avrebbe potuto distinguere da un refuso. Il numero si rimisura, non si ricopia:
	 *
	 *   python - <<'PY'  (radice del repo)
	 *   import json, os, collections
	 *   req = collections.Counter()
	 *   for root, _, names in os.walk("Scenarios"):
	 *       for n in [x for x in names if x.endswith(".json")]:
	 *           d = json.load(open(os.path.join(root, n), encoding="utf-8-sig"))
	 *           for t in d.get("turns", []):
	 *               for c in (t.get("requires") or []): req[c] += 1
	 *   print(sorted(req))
	 *   PY
	 *
	 * L'ancora che tiene allineati i due elenchi al corpus e' `ShippedScenariosRequireKnownCapabilities`: un
	 * nome nuovo in uno scenario e' rosso subito, non un `BLOCKED` che nessuno legge.
	 */
	const TSet<FString>& KnownUnavailableCapabilities()
	{
		static const TSet<FString> KnownUnavailable = {
			// ➖ `DecisionBoundary` e' USCITA da qui con `#512` fase B ed e' fra le disponibili, sopra.
			TEXT("ReactionClash"),            // owner: #314
			// ➕ **`ReactionProfile` entra con `#512` fase B, e non e' un nome nuovo inventato per comodita':
			// e' il blocco VERO di `Spec/Brace/ProfileChangesResponse`, che fino a oggi ne dichiarava uno
			// falso.** Quello scenario chiedeva `DecisionBoundary` scrivendo, nella propria nota, che «con la
			// sola finestra di CP 14.5 questo file puo' diventare verde». Misurato: **non puo'**. Gli serve
			// che `Hero.Phase` porti `Profile.Sidestep`, cioe' un profilo di reazione con DUE risposte legali,
			// e `grep -rn "Profile.Sidestep\|ReactionProfile" Source/` da' **zero** — il concetto non esiste
			// in nessuna forma, non e' un rename e non e' un campo vuoto da riempire.
			//
			// ⚠️ Senza questa riga la fase B avrebbe prodotto il difetto che vuole impedire: scoprendo
			// `DecisionBoundary`, il T2 di quello scenario sarebbe passato da `Blocked` a **verde** — con
			// `intents: []`, nessuna reazione armata e nessuna finestra aperta. Un turno che si sblocca senza
			// eseguire cio' che descrive e' peggio di un rosso, perche' nessuno va a guardarlo.
			//
			// L'owner e' **E14.7 (`#314`)**, che porta `Reaction Profile` e `Reaction Clash` insieme. Chi la
			// chiude sposta ENTRAMBI i nomi, non solo questo.
			// ➖ **USCITA il 2026-08-19 con la fetta 4: `ReactionProfile` e' fra le DISPONIBILI**, e le ragioni
			// stanno accanto alla sua voce là sopra. Il testo qui resta come registro di ciò che ha bloccato
			// quello scenario per tre giorni, e di come la fase B evitò di sbloccarlo per la ragione sbagliata.
			// ⚠️ **«Chi la chiude sposta ENTRAMBI i nomi» era una prescrizione e resta vera per `#314`**, che è
			// ancora OPEN: `ReactionClash` è tuttora qui sotto perché la sua macchina non ha chiamanti nel
			// resolver. Chi chiude la issue sposta anche quello.
			// 🔵 **`owner:` e' chi la SPOSTERA' fra le disponibili, non chi ha scritto la feature**, e questa
			// riga e' il caso che ha costretto a distinguerlo. La feature esiste ed e' chiusa — `#200`,
			// rivalidazione della geometria sul bersaglio effettivo (D-017), CLOSED 4/4, con tre test che la
			// pinnano (`Cover.InterceptRecalculatesOnEffectiveTarget`, `...RevalidatesFacingOnEffectiveTarget`,
			// `...DoesNotOpenSecondOpportunity`). Cio' che manca e' lo spostamento, e lo deve fare `#170`
			// insieme al contenuto del T6 dello showcase: scoprirla da sola farebbe passare un turno con
			// `intents: []`, cioe' il verde bugiardo che `#512` fase B ha speso un giro a impedire.
			// ⏱️ **L'owner passa da `#170` a `#1060` il 2026-08-16**, e non e' un dettaglio di tracciamento:
			// il T6 e' stato scorporato perche' gli manca il VOCABOLARIO — `OriginalTargetEquals` e
			// `EffectiveTargetEquals` non esistono fra i tipi di assertion, e `ERTReactionOutcome` non ha un
			// esito che distingua il redirect. Chi sposta questa capability deve costruirli prima, altrimenti
			// il turno si accende e nessuno puo' verificare che rediriga sul bersaglio giusto.
			// ⚠️ Con `owner: #200` questa riga era il difetto che `check-capability-owners.py --online` esisteva
			// per trovare: dichiarata non disponibile con l'owner CHIUSO. Il gate la trovo' al primo giro.
			// 🔴 **Quel gate e' uscito con D-182 il 2026-08-21**: un `owner:` che punta a una issue chiusa
			// mentre la capability resta non disponibile oggi non lo segnala piu' nessuno. Chi tocca questi
			// commenti controlli lo stato della issue a mano.
			TEXT("InterceptRevalidation"),    // owner: #1060
			TEXT("Objective"),                // owner: #75
			TEXT("Perception"),               // owner: #151
			// Le tre che la prosa non nominava, chieste da `Spec/Movement/`: `SpatialTrigger` (tripwire che
			// scatta attraversando un bordo), `SemanticTrigger` (trigger che distingue Dash da Move) e
			// `Teleport` (spostamento che non attraversa le celle intermedie). Restano fuori per lo stesso
			// criterio delle altre — nessun produttore in partita.
			// ⚠️ **Non c'e' piu' un documento che le elenchi.** Stavano in `scenariomap.shortlist.md`, una
			// vista GENERATA dal registro degli scenari, uscita con D-181 il 2026-08-21. Verificato: i tre
			// nomi non compaiono in nessun `.md` del repository. Da qui in poi questa lista **e'** la
			// dichiarazione, e `Spec/Movement/` e' chi le chiede.
			TEXT("SpatialTrigger"),           // owner: #704
			TEXT("SemanticTrigger"),          // owner: #704
			TEXT("Teleport"),                 // owner: #704
			// 🔒 RISERVATA AI TEST, e non diventera' MAI disponibile. Non e' una feature: e' il veicolo con cui
			// `BlockedFirstTurnStaysBlocked` prova che un turno bloccato batte le assertion finali.
			//
			// Quel test usava un nome inventato — `CapabilityCheNonEsistera Mai` — proprio perche' non sarebbe
			// mai atterrato: con un nome vero, il giorno in cui la capability diventa disponibile il turno
			// verrebbe giocato e il test misurerebbe un'altra cosa. Da quando un nome sconosciuto vale `Error`
			// quel veicolo non funziona piu', ma la ragione per cui era inventato resta valida — quindi il
			// nome e' dichiarato QUI invece che scomparire, ed e' l'unica riga di questo elenco che non
			// descrive un pezzo di gioco futuro.
			//
			// ⚠️ Non spostarla fra le disponibili per nessun motivo: `AvailableCapabilities()` e' l'insieme di
			// cio' che il gioco sa fare, e questo nome non e' niente.
			TEXT("NeverAvailable"),           // owner: none
		};
		// Le righe che mancano valgono quanto quelle che ci sono. L'elenco e' stato completato con `#582`:
		// prima ne nominava due — e una capability che nessuno documenta produce un `BLOCKED` senza
		// spiegazione, che e' meta' del valore.
		//
		//   `DecisionBoundary` e `ReactionClash` — la FINESTRA: sospendere la risoluzione e chiedere una
		//   risposta. Sono due nomi e non uno perche' separano due cose che possono atterrare in tempi
		//   diversi: fermarsi a un boundary, e risolvere il confronto fra due risposte contese.
		//
		//   🔴 **La ragione per cui `DecisionBoundary` resta indisponibile NON e' piu' quella scritta qui, e
		//   il testo vecchio va letto come registro** (2026-08-16, `#512`). Diceva: «CP 14.4 ha consegnato la
		//   regola ... ma **nessun chiamante di produzione**, e la finestra da 3 s e' CP 14.5». Entrambe le
		//   meta' sono scadute: CP 14.5 e' chiusa, `RTTurnManager.cpp:5093` chiama `BuildOverwatchTriggers` in
		//   partita, e con `#512` un decisore iniettato risponde davvero a una finestra vera
		//   (`ShowcaseRelay.DecisionProviderIsInjectable`).
		//
		//   La ragione VERA e' un'altra, ed e' sui DATI: gli scenari che la chiedono **non hanno le
		//   `decisions`** che li renderebbero rispondibili — `Spec/Overwatch/HoldThenFire` e
		//   `Spec/Brace/ProfileChangesResponse` ne hanno zero, misurate. Scoprirla li farebbe girare senza
		//   nessuno che risponda alle loro finestre.
		//
		//   ⚠️ **Misurato scoprendola per davvero, invece di prevederlo**: il piano diceva che sarebbero
		//   passati «da `BLOCKED` a `FAIL`, che `EveryShippedScenarioRuns` non accetta». Non e' cosi' —
		//   quel test resta VERDE. A cadere sono altri due, e dicono qualcosa di piu' preciso:
		//   `Scenario.ShowcaseRelayV01RunsTurnOne` (`RT_Showcase_Relay_v01` arriva a **cinque** turni invece
		//   di tre: il T2 non si ferma piu') e `Scenario.UnknownCapabilityIsErrorNotBlocked`, che usa
		//   `DecisionBoundary` proprio come **esempio** di «nota ma non disponibile» e ne perderebbe il
		//   soggetto. Si scopre **insieme** ai dati, ed e' fase B.
		//
		//   `Facing` — ✅ **riconciliata il 2026-08-13**: era il doppione di `DeclaredRotation` con un altro
		//   nome, e i due dovevano essere unificati «quando l'input arrivera'» (#291). L'input e' arrivato, e
		//   il nome che resta e' **`DeclaredRotation`**, ora fra i disponibili qui sopra. `Facing` non e' un
		//   nome di capability: chi scrive uno scenario chiede `DeclaredRotation`. Il facing di PIAZZAMENTO
		//   (`FRTScenarioUnit::Facing`) non ha mai avuto bisogno di una capability e continua a non averne.
		//
		//   ⚠️ E infatti `Facing` **non e' in nessuno dei due insiemi**: da qui in avanti chiederlo e' un
		//   `Error`. La riconciliazione era rimasta a meta' — `RT_Showcase_Relay_v01` lo chiedeva ancora al
		//   turno 4, `["DecisionBoundary", "Facing"]` — e non se ne accorgeva nessuno perche' quel turno era
		//   gia' `Blocked` per il primo dei due nomi. Lo scenario ora chiede `DeclaredRotation`, e il suo
		//   esito non cambia di una riga: resta bloccato su `DecisionBoundary`, che e' il punto — la
		//   correzione di un refuso non deve spostare un verdetto.
		//
		//   `InterceptRevalidation`, `Objective`, `Perception` — chieste da scenari gia' in repo. Restano fuori
		//   finche' qualcuno non misura, come si e' fatto qui per `Reaction`, che il gioco le sappia fare in
		//   partita e non solo in una libreria pura.
		//
		//   ⚠️ La CONDIZIONE dichiarata di [D-109] (`TargetHealthAtOrBelowPercent`) non e' ancora implementata,
		//   e quando lo sara' vale per lei la stessa regola: **nessuna chiave di scenario** finche' non esiste
		//   un produttore che non sia l'harness. E' il criterio che ha tenuto fuori `ReactionPlanning` fino a
		//   `#601`, ed e' l'unico che impedisce a un verde di dire che il giocatore puo' dichiarare qualcosa
		//   che non ha modo di chiedere.
		//
		//   `ReactionPlanning` e' USCITA da questo elenco con `#601`: il campo ha finalmente un produttore
		//   nel gioco (controller e bot), quindi darla agli scenari non rende piu' l'harness piu' capace del
		//   gioco. La riga sopra spiega il perimetro; questa resta a ricordare **perche'** era fuori, che e' il
		//   criterio con cui si giudica la prossima.
		//
		//   `DeclaredRotation` — dichiarare una ROTAZIONE in pianificazione (D-020). Dopo #291 la catena esiste
		//   quasi tutta: il campo sta su `ARTUnit`, entra in `FRTPlannedIntent`, passa da `FilterForTeam`, e il
		//   `TurnManager` lo consuma a fine Move producendo `DeclaredInPlanning` o `DeclarationRejected`. Manca
		//   il solo anello che non e' una regola: l'INPUT. Nessun comando lo dichiara e il bot non lo usa, quindi
		//   dare agli scenari una chiave `facing` renderebbe l'harness il primo produttore del campo — la stessa
		//   asimmetria di `ReactionPlanning`, con lo stesso esito: verdi che dicono che il giocatore puo' girarsi
		//   restando fermo, mentre non ha alcun modo di chiederlo. L'input e' lavoro di E11, e senza l'insieme
		//   legale mostrato a schermo il giocatore non saprebbe nemmeno quali tre direzioni gli restano.
		//
		//   🔁 2026-08-13 mattina: `DeclaredRotation` NON era un caso isolato. I campi di piano senza
		//   produttore nel gioco erano TRE — `PlannedFacing`, `PlannedAttackCell`, `PlannedCoverEdge` — e solo
		//   il primo era stato tenuto fuori dalle capability. Il criterio non era cambiato: era stato
		//   applicato a meta'. Il produttore mancante era `#737`.
		//
		//   ✅ **2026-08-13 sera: chiuso il cerchio.** `#737` ha portato i tre produttori, e questa stessa
		//   sessione ha aggiunto la chiave `facing` all'intent di scenario piu' `DeclaredRotation` fra i
		//   disponibili. Delle tre asimmetrie storiche di questo elenco — `ReactionPlanning` (#601),
		//   `PredictiveAction` (motivazione tornata vera), `DeclaredRotation` — non ne resta nessuna aperta.
		//
		//   La regola che le ha governate tutte e tre vale per la prossima, e conviene rileggerla prima di
		//   aggiungere una riga qui sopra: **nessuna chiave di scenario finche' non esiste un produttore che
		//   non sia l'harness.** Il prossimo caso noto e' la CONDIZIONE dichiarata di [D-109]
		//   (`TargetHealthAtOrBelowPercent`), citata poco piu' su.
		return KnownUnavailable;
	}

	/** Il gioco sa fare questa cosa **oggi**? Un `no` e' un'attesa legittima, e vale `Blocked`. */
	bool IsCapabilityAvailable(const FString& Capability)
	{
		return AvailableCapabilities().Contains(Capability);
	}

	/**
	 * Questo nome esiste nel vocabolario, disponibile o no?
	 *
	 * Un `no` non e' un'attesa: e' un REFUSO nello scenario, e vale `Error`. La distinzione e' l'intera
	 * ragione per cui i due insiemi sono separati — vedi `KnownUnavailableCapabilities()`.
	 */
	bool IsCapabilityKnown(const FString& Capability)
	{
		return AvailableCapabilities().Contains(Capability) || KnownUnavailableCapabilities().Contains(Capability);
	}

	/**
	 * L'arena dello scenario: una **fixture riferita per nome** se dichiarata, altrimenti generata dal raggio.
	 *
	 * Riferire invece di duplicare tiene la geometria canonica in UN posto solo, gia' protetto da un test
	 * (`ShowcaseRelay.BasinLayoutMatchesSpec`). Gli override di cella restano validi e si applicano SOPRA la
	 * fixture: le due strade differiscono solo per come nasce la mappa.
	 */
	URTHexMapAsset* BuildScenarioArena(UWorld* World, const FRTTestScenario& Scenario,
		ARTHexMapActor*& OutActor)
	{
		// La mappa NON si costruisce piu' qui: `URTScenarioArenaLibrary::BuildArena` e' la stessa arena che
		// l'authoring interroga per la preview di raggiungibilita' (`#1116`). Se ne esistessero due versioni,
		// l'editor mostrerebbe celle che il runner poi non concede — e nessuno dei due direbbe di sbagliare.
		URTHexMapAsset* Map = URTScenarioArenaLibrary::BuildArena(Scenario, World);
		if (!Map)
		{
			return nullptr;
		}

		// Questa meta' resta della sessione, ed e' presentazione: senza un Actor, in PIE non si vedrebbe niente.
		ARTHexMapActor* Actor = ARTHexMapActor::FindInWorld(World);
		if (!Actor)
		{
			Actor = World->SpawnActor<ARTHexMapActor>();
		}
		if (!Actor)
		{
			return nullptr;
		}

		Actor->MapAsset = Map;
		Actor->RebuildInstances(); // la vista ISM segue l'asset: senza, in PIE resterebbe la mappa precedente
		OutActor = Actor;
		return Map;
	}

	/**
	 * Posizione della PRIMA occorrenza di un evento nel log, o `INDEX_NONE`.
	 *
	 * «Prima occorrenza» e non «una qualsiasi»: `LogEventOrder` chiede se una cosa e' successa prima di
	 * un'altra, e con eventi ripetuti l'unica formulazione che non dipende da quale coppia si sceglie e'
	 * confrontare i due esordi.
	 */
	int32 IndexOfScenarioLogEvent(const TArray<FRTTurnLogEntry>& Log, ERTLogCategory Category, uint8 Outcome)
	{
		for (int32 I = 0; I < Log.Num(); ++I)
		{
			if (Log[I].Category == Category && Log[I].Outcome == Outcome) { return I; }
		}
		return INDEX_NONE;
	}

	/** L'eroe del catalogo con quell'ID stabile, o nullptr. Il roster e' la fonte: nessun elenco duplicato. */
	URTHeroData* FindScenarioHero(FName HeroId)
	{
		for (URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Hero && Hero->HeroId == HeroId)
			{
				return Hero;
			}
		}
		return nullptr;
	}
}

FRTScenarioSession::~FRTScenarioSession()
{
	UnbindOwnDecider();
}

void FRTScenarioSession::UnbindOwnDecider()
{
	// Solo il PROPRIO bind: se lo slot era gia' occupato da un test, sbindarlo distruggerebbe un decisore
	// che questa sessione non ha messo. `DecisionSource` e' l'unico testimone di chi ha legato.
	if (DecisionSource != TEXT("scenario"))
	{
		return;
	}
	if (ARTTurnManager* TM = TurnManager.Get())
	{
		TM->ReactionDecider.Unbind();
	}
	// Idempotente: dopo lo sgancio questa sessione non e' piu' la sorgente, e un secondo giro non tocca il
	// bind di nessun altro.
	DecisionSource = TEXT("none");
}

bool FRTScenarioSession::IsKnownCapability(const FString& Capability)
{
	return IsCapabilityKnown(Capability);
}

bool FRTScenarioSession::IsAvailableCapability(const FString& Capability)
{
	return IsCapabilityAvailable(Capability);
}

bool FRTScenarioSession::Start(UWorld* InWorld, const FRTTestScenario& InScenario)
{
	Scenario = InScenario;
	Result = FRTTestResult();
	Result.ScenarioId = Scenario.ScenarioId;
	Result.Seed = Scenario.Seed;

	auto Fail = [this](const FString& Reason) -> bool
	{
		// Tutto cio' che va storto qui e' ERROR, non FAIL: non si e' potuto eseguire, quindi il difetto e' nel
		// test o nell'ambiente, non nel gioco. Confonderli fa cercare una regressione che non esiste.
		Result.Outcome = ERTTestOutcome::Error;
		Result.ErrorMessage = Reason;
		State = EState::Finished;
		return false;
	};

	if (!InWorld)
	{
		return Fail(TEXT("nessun mondo in cui eseguire lo scenario"));
	}
	World = InWorld;

	FString ValidationError;
	if (!URTScenarioLoader::Validate(Scenario, ValidationError))
	{
		return Fail(FString::Printf(TEXT("scenario non valido: %s"), *ValidationError));
	}

	ARTHexMapActor* MapActor = nullptr;
	Map = BuildScenarioArena(InWorld, Scenario, MapActor);
	if (!Map)
	{
		// Il nome sbagliato si dice, non si aggira: e' un difetto dello SCENARIO, e va nominato.
		return Fail(Scenario.Fixture.IsEmpty()
			? TEXT("impossibile creare l'arena esagonale")
			: *FString::Printf(TEXT("fixture di mappa sconosciuta: '%s'"), *Scenario.Fixture));
	}

	// Origine e scala dall'ACTOR mappa, non dall'origine del mondo: e' la stessa fonte che usa `RebuildInstances`
	// per disegnare la griglia e che usa l'allestimento della partita normale. Prendendo `FVector::ZeroVector`,
	// con un actor mappa spostato nel livello le unita' finivano in un punto e la griglia in un altro — e la
	// camera, inquadrando le unita', lasciava la mappa fuori campo. Osservato in PIE su L_DevSandbox.
	FVector MapOrigin = FVector::ZeroVector;
	float MapHexSize = Map->HexSize;
	float MapLayerHeight = Map->LayerHeight;
	if (MapActor)
	{
		MapActor->GetHexContext(MapOrigin, MapHexSize, MapLayerHeight);
	}

	for (const FRTScenarioUnit& Spec : Scenario.Units)
	{
		URTHeroData* Hero = FindScenarioHero(Spec.HeroId);
		if (!Hero)
		{
			// Validate() lo esclude gia', ma la sessione non si fida di un invariante altrui.
			return Fail(FString::Printf(TEXT("eroe '%s' non nel catalogo"), *Spec.HeroId.ToString()));
		}

		ARTUnit* Unit = InWorld->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit)
		{
			return Fail(FString::Printf(TEXT("spawn fallito per l'unita' '%s'"), *Spec.Id));
		}
		Unit->TeamId = Spec.TeamId;
		Unit->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		// Chi decide l'intent: il FILE per default, il pianificatore del gioco per le unita' dichiarate `bot`.
		// Il default resta quello di prima — uno scenario che non dice niente non guadagna un bot per sbaglio.
		Unit->bIsBotControlled = Spec.bBotControlled;
		if (Spec.bBotControlled)
		{
			bHasBotUnits = true;
		}
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(Spec.Cell, MapOrigin, MapHexSize, MapLayerHeight);
		// Orientamento INIZIALE (CP 13.2): dove guarda la figura appena posata. Dopo `PlaceOnCell`, che non lo
		// tocca. Non e' una rotazione dichiarata — vedi `FRTScenarioUnit::Facing`.
		Unit->Facing = Spec.Facing;

		// Condizione INIZIALE: dopo `ConfigureFromHeroData`, che ha appena scritto i valori del roster. Un
		// valore oltre il tetto e' un errore dello SCENARIO e non si clampa in silenzio: chi ha scritto 200 su
		// un eroe da 120 stava descrivendo un'altra partita, e un clamp gliela farebbe passare per la sua.
		if (Spec.Health != -1)
		{
			if (Spec.Health > Unit->MaxHealth)
			{
				return Fail(FString::Printf(TEXT("unita' '%s': health %d oltre il massimo dell'eroe (%d)"),
					*Spec.Id, Spec.Health, Unit->MaxHealth));
			}
			Unit->Health = Spec.Health;
		}
		if (Spec.Shield != -1)
		{
			Unit->Shield = Spec.Shield;
		}
		if (Spec.VisionRange != -1)
		{
			Unit->VisionRange = Spec.VisionRange;
		}
		// EQUIPAGGIAMENTO. Che i pezzi esistano e che l'insieme sia legale l'ha gia' verificato il loader,
		// che rifiuta lo scenario con un motivo invece di lasciarlo girare a meta'. Qui si sceglie QUALE
		// loadout, e ad applicarlo e' `ARTUnit::EquipLoadout` — la stessa funzione che chiama
		// `ARTGameMode::SpawnHero`. Una seconda copia qui le farebbe divergere, e uno scenario che
		// verifica un equipaggiamento diverso da quello che il gioco monta non prova niente sul gioco.
		//
		// ⚠️ **Tre forme, e la condizione e' sul FLAG, non sul conteggio** (`#1054`, `#602`):
		//   · `loadout` ASSENTE  -> il default dell'eroe, cioe' quello che la partita monta. Senza questo
		//                           ramo l'harness misurerebbe un gioco che nessuno gioca — il difetto da
		//                           cui `#63` e' partita;
		//   · `"loadout": []`    -> l'unita' entra SPOGLIA, e lo scenario lo sta dichiarando. E' la forma
		//                           che tiene ferma una variabile che il gioco muove: quattro scenari
		//                           misurano il confine Guard/Brace *con spinta 1*, e il default di Phase
		//                           la porta a 2;
		//   · lista              -> quei pezzi, e vincono sul default. Non si sommano: due varianti d'arma
		//                           sarebbero un insieme che `ValidateLoadout` rifiuta.
		//
		// `Spec.Loadout.Num() > 0` qui sarebbe **silenziosamente sbagliato**: tratterebbe `[]` come assente
		// e rimonterebbe il default proprio sulle unita' che chiedono di non averlo.
		const TArray<FName> Pezzi = Spec.bLoadoutDeclared
			? Spec.Loadout
			: URTCatalogLibrary::DefaultLoadoutFor(Spec.HeroId);
		Unit->EquipLoadout(Pezzi);

		UnitsById.Add(Spec.Id, Unit);
	}

	ARTTurnManager* TM = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(InWorld, ARTTurnManager::StaticClass()));
	if (!TM)
	{
		TM = InWorld->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	}
	if (!TM)
	{
		return Fail(TEXT("impossibile creare il turn manager"));
	}
	TurnManager = TM;

	// Il timer del turn manager va SPENTO: e' la sessione a decidere quando si chiude la pianificazione. Se
	// scadesse per conto suo risolverebbe un turno che la sessione non ha preparato — ed e' esattamente il
	// difetto dei turni fantasma, visto in PIE prima che questa riga esistesse.
	TM->SetPlanningSeconds(0.f);

	// Il seam esiste dal CP 14.5 e nessuno lo bindava: qui lo scenario diventa il decisore delle finestre.
	//
	// ⚠️ Solo se lo slot e' libero, ed e' cosi' che un test ha la precedenza: chi binda prima vince. Non
	// serve una catena ne' un flag — il delegate e' uno slot solo, e la forma E' la regola.
	//
	// 🔴 **E solo se lo scenario ha davvero qualcosa da rispondere.** Bindare sempre sarebbe una regressione
	// silenziosa su due fronti, entrambi invisibili alla suite: `AskReactionDecision` raggiunge il ramo del
	// bot (`URTHexBotLibrary::DecideReactionResponse`) **soltanto** se il delegate NON e' legato — «il
	// decisore iniettato ha la precedenza su tutto, bot compreso», dice il suo commento — quindi un'unita'
	// del bot con un Overwatch armato smetterebbe di reagire in ogni scenario; e per un proprietario umano
	// la voce del TurnLog passerebbe da `HoldNoDecider` a `HoldTimeout`, che e' una differenza di BYTE per
	// il corpus golden (#178/#170). Uno scenario che non scripta nulla deve lasciare il seam com'era.
	//
	// Il bind puo' stare qui, prima che `UnitsById` esista, perche' la traduzione legge la mappa al momento
	// della CHIAMATA — durante il Move — non al momento del bind.
	// ⚠️ L'ordine delle tre domande conta, e la prima NON e' «lo scenario ha decisioni?». Uno slot gia'
	// occupato significa che **qualcuno risponde**, e va registrato come `test-override` anche se lo scenario
	// non scripta nulla: dire `none` sarebbe falso — il referto lo userebbe per spiegare un esito che quel
	// decisore ha prodotto. La condizione sulle decisioni governa solo se questa sessione binda il PROPRIO.
	const bool bScenarioHasDecisions = InScenario.Turns.ContainsByPredicate(
		[](const FRTScenarioTurn& T) { return T.Decisions.Num() > 0; });

	if (TM->ReactionDecider.IsBound())
	{
		DecisionSource = TEXT("test-override");
	}
	else if (bScenarioHasDecisions)
	{
		TM->ReactionDecider.BindRaw(this, &FRTScenarioSession::DecideScriptedResponse);
		DecisionSource = TEXT("scenario");
	}
	else
	{
		DecisionSource = TEXT("none");
	}

	// Le capability dell'INTERO scenario (free-run, CP 47.4): si valutano una volta qui, perche' un free-run
	// non ha turni su cui appenderle. Stesse due passate e stesso ordine di `BeginTurn` — prima il refuso
	// (`Error`, colpa di chi scrive), poi l'attesa (`Blocked`, colpa di nessuno) — e per la stessa ragione:
	// chiedendo prima la disponibilita', un nome sbagliato si nasconderebbe dietro il `Blocked` di un altro.
	for (const FString& Required : Scenario.Requires)
	{
		if (!IsCapabilityKnown(Required))
		{
			ErroredBy = FString::Printf(
				TEXT("requires di scenario: la capability '%s' non esiste. Non e' un'attesa: e' un nome che ")
				TEXT("nessun elenco dichiara, quindi lo scenario e' scritto male. I nomi validi stanno in ")
				TEXT("`RTScenarioSession.cpp`, `AvailableCapabilities()` e `KnownUnavailableCapabilities()`."),
				*Required);
			Finish();
			return true;
		}
	}
	for (const FString& Required : Scenario.Requires)
	{
		if (!IsCapabilityAvailable(Required))
		{
			BlockedBy = FString::Printf(TEXT("scenario: manca la capability '%s'"), *Required);
			Finish();
			return true;
		}
	}

	// ⚠️ `bFreeRun` esclude questo ramo, e non e' un dettaglio: in free-run `turns` e' vuoto **per
	// costruzione**, quindi senza la condizione lo scenario finirebbe qui — zero turni giocati, nessuna
	// partita, e un `Pass` per assenza di partita.
	if (Scenario.Turns.Num() == 0 && !Scenario.bFreeRun)
	{
		// Uno scenario senza turni e' legittimo: verifica solo lo stato iniziale.
		Finish();
		return true;
	}

	State = EState::PauseBeforeTurn;
	PauseElapsed = 0.f;
	TurnIndex = 0;
	ApplyPreviewSelection();
	return true;
}

void FRTScenarioSession::ApplyPreviewSelection()
{
	if (Scenario.PreviewUnit.IsEmpty() || !Scenario.Turns.IsValidIndex(0))
	{
		return;
	}

	UWorld* W = World.Get();
	// Nessun controller = headless. E' il ramo che garantisce che questo campo non possa spostare un esito:
	// dove si misura, non gira.
	//
	// `GetActorOfClass` e non `GetFirstPlayerController()`: quest'ultima legge la lista di controller del
	// mondo, che si popola solo attraverso il login del GameMode — in un mondo di test un controller
	// spawnato con `SpawnActor` non ci finisce, e il ramo restava non esercitabile. Cercare l'ATTORE e' anche
	// il modo in cui il resto del progetto trova turn manager, mappa e camera.
	ARTPlayerController* PC = W
		? Cast<ARTPlayerController>(UGameplayStatics::GetActorOfClass(W, ARTPlayerController::StaticClass()))
		: nullptr;
	if (!PC)
	{
		return;
	}

	TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Scenario.PreviewUnit);
	ARTUnit* Unit = Found ? Found->Get() : nullptr;
	if (!Unit)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: previewUnit '%s' non esiste, nessuna selezione"),
			*Scenario.ScenarioId, *Scenario.PreviewUnit);
		return;
	}

	// I TRE GESTI del giocatore, nell'ordine in cui li farebbe: scegli l'abilita', seleziona la tua unita',
	// clicca il nemico. Passando dalle stesse funzioni, e non scrivendo lo stato a mano: un'anteprima
	// costruita per scorciatoia assomiglierebbe a quella del gioco senza esserlo, ed e' proprio quella la
	// bugia che questo scenario dovrebbe smascherare.
	//
	// Il primo tentativo chiamava il solo `HandleClickOnUnit(Unit)` credendo che selezionasse. Non seleziona:
	// presuppone una selezione e tratta l'argomento come BERSAGLIO, quindi usciva subito e a schermo non
	// compariva niente — mentre il log qui sotto dichiarava successo. Da li' il `Verify` in fondo.
	ARTUnit* Target = nullptr;
	int32 AbilityIndex = INDEX_NONE;
	for (const FRTScenarioIntent& Intent : Scenario.Turns[0].Intents)
	{
		if (Intent.UnitId != Scenario.PreviewUnit || Intent.Ability.IsNone() || Intent.bTargetsCell)
		{
			continue;
		}
		TWeakObjectPtr<ARTUnit>* FoundTarget = UnitsById.Find(Intent.Target);
		Target = FoundTarget ? FoundTarget->Get() : nullptr;
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* Ability = Unit->GetAbility(I);
			if (Ability && Ability->Def.ActionId == Intent.Ability)
			{
				AbilityIndex = I;
				break;
			}
		}
		break;
	}

	if (!Target || AbilityIndex == INDEX_NONE)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: '%s' non ha un attacco su bersaglio al turno 1, nessuna anteprima"),
			*Scenario.ScenarioId, *Scenario.PreviewUnit);
		return;
	}

	Unit->SelectedAbilityIndex = AbilityIndex;                  // come premere il tasto dell'abilita'
	PC->SelectUnit(Unit, /*bRecordAsPlayerInput=*/ false);      // come cliccare la propria unita'
	PC->HandleClickOnUnit(Target);                              // come cliccare il nemico

	// VERIFICA invece di dichiarare. La riga precedente stampava «selezionata» subito dopo la chiamata, e ha
	// raccontato per un'intera sessione un successo che non c'era: se il piano non e' atterrato, il log deve
	// dirlo qui, dove si sa, invece di lasciarlo scoprire a chi guarda uno schermo che non cambia.
	if (Unit->PlannedAttackTarget == Target)
	{
		UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: '%s' punta '%s' — anteprima attiva, guardala prima che risolva"),
			*Scenario.ScenarioId, *Scenario.PreviewUnit, *Target->GetName());
	}
	else
	{
		UE_LOG(LogRT, Error, TEXT("[RT-Test] %s: l'anteprima NON e' partita — il bersaglio e' stato rifiutato "
			"(portata, linea di tiro o abilita' non pronta). Le righe [RT] qui sopra dicono quale"),
			*Scenario.ScenarioId);
	}
}

void FRTScenarioSession::ApplyScenarioIntents(ARTTurnManager& TurnManagerRef)
{
	ARTTurnManager* TM = &TurnManagerRef;

	for (const FRTScenarioIntent& Intent : Scenario.Turns[TurnIndex].Intents)
	{
		TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Intent.UnitId);
		ARTUnit* Unit = Found ? Found->Get() : nullptr;
		if (!Unit || !Unit->IsAlive())
		{
			continue;
		}

		// L'abilita' si cerca per ActionId: l'indice nel kit si sposta appena qualcuno ne aggiunge una, e uno
		// scenario che punta a un indice continuerebbe a passare verificando l'abilita' sbagliata.
		const auto FindAbilityIndex = [Unit](const FName& ActionId) -> int32
		{
			for (int32 I = 0; I < Unit->NumAbilities(); ++I)
			{
				const URTActionData* Ability = Unit->GetAbility(I);
				if (Ability && Ability->Def.ActionId == ActionId) { return I; }
			}
			return INDEX_NONE;
		};

		// --- scatto (fase Dash, PRIMA del Blast) ------------------------------------------------------------
		// Slot distinto dall'abilita' perche' dopo D-028 lo sono davvero: lo scatto prende il movimento, non la
		// principale. Un intent puo' dichiarare entrambi, ed e' *schivo e sparo*.
		if (!Intent.Dash.IsNone())
		{
			const int32 DashIndex = FindAbilityIndex(Intent.Dash);
			if (DashIndex == INDEX_NONE)
			{
				ErroredBy = FString::Printf(TEXT("'%s' non possiede la mobilita' '%s' (turno %d)"),
					*Intent.UnitId, *Intent.Dash.ToString(), TurnIndex + 1);
				UE_LOG(LogRT, Error, TEXT("[RT-Test] %s: %s"), *Scenario.ScenarioId, *ErroredBy);
			}
			else
			{
				// Si scrive il PIANO, non l'esito: traiettoria, ostacoli e chi viene attraversato li decide il
				// resolver in fase Dash. Anticiparli qui significherebbe verificare le regole della sessione
				// invece di quelle del gioco.
				Unit->PlannedDashAbility = DashIndex;
				Unit->PlannedDashCell = Intent.DashCell;
			}
		}

		// --- abilita' -------------------------------------------------------------------------------------
		// Stessa strada del controller: si scrivono `PlannedAbilityIndex` e `PlannedAttackTarget`, esattamente
		// come dopo un click sul nemico. Portata, LOS, cooldown ed energia li valuta il turn manager al momento
		// della risoluzione — la sessione non li anticipa, altrimenti verificherebbe le proprie regole invece
		// di quelle del gioco.
		// Bersaglio a UNITA': il caso a cella e' il ramo dopo. La condizione porta `!bTargetsCell` perche'
		// altrimenti un intent a cella entrerebbe QUI, non troverebbe l'unita' (`Target` e' vuoto per
		// costruzione) e finirebbe nel ramo del bersaglio abbattuto: una nota al posto di un attacco.
		if (!Intent.Ability.IsNone() && !Intent.bTargetsCell)
		{
			TWeakObjectPtr<ARTUnit>* FoundTarget = UnitsById.Find(Intent.Target);
			ARTUnit* Target = FoundTarget ? FoundTarget->Get() : nullptr;

			const int32 AbilityIndex = FindAbilityIndex(Intent.Ability);

			// Un'azione di Prep agisce su chi la usa, quindi non ha un bersaglio da dichiarare. La domanda si
			// pone al CATALOGO — `MapResolutionPhase` — invece di elencare gli ActionId self: un'azione di Prep
			// aggiunta domani si comporta bene senza che nessuno si ricordi di questa riga.
			const URTActionData* PlannedAbility = Unit->GetAbility(AbilityIndex);
			const bool bResolvesOnSelf = PlannedAbility
				&& URTCatalogLibrary::MapResolutionPhase(PlannedAbility->Def.ResolutionPhase) == ERTMatchPhase::Prep;

			if (AbilityIndex == INDEX_NONE)
			{
				// Un'abilita' che l'eroe non possiede e' un errore di SCRITTURA dello scenario, non un esito di
				// gioco. Prima finiva in un log e l'attacco semplicemente non partiva: l'assertion sui danni
				// cadeva e il report diceva FAIL, cioe' mandava a cercare una regressione che non esisteva.
				//
				// Il validator non puo' prenderlo al caricamento: `Phase.CircularTide` ESISTE nel catalogo, non
				// e' nel kit di Gadget — e il kit lo si conosce solo quando le unita' sono state costruite.
				ErroredBy = FString::Printf(TEXT("'%s' non possiede l'abilita' '%s' (turno %d)"),
					*Intent.UnitId, *Intent.Ability.ToString(), TurnIndex + 1);
				UE_LOG(LogRT, Error, TEXT("[RT-Test] %s: %s"), *Scenario.ScenarioId, *ErroredBy);
			}
			else if (bResolvesOnSelf)
			{
				// Azione che risolve su CHI LA USA (`Action.Guard`, `Action.Brace`, e ogni azione di Prep del
				// vertical slice): il `TurnManager` si bersaglia da solo — `Instance.TargetUnitId = i`, e il
				// `PlannedAttackTarget` non lo guarda nemmeno. Pretendere un bersaglio qui sarebbe una regola
				// dell'HARNESS che il gioco non ha, e costringerebbe a scrivere «Riktor si mette in guardia
				// bersagliando se stesso» per ottenere quel che il gioco chiama semplicemente mettersi in guardia.
				Unit->PlannedAbilityIndex = AbilityIndex;
			}
			else if (!Target || !Target->IsAlive())
			{
				// Non e' un errore: e' una partita andata cosi'. Ma tacerlo lascia senza spiegazione l'assertion
				// che cadra' subito dopo — «perche' non ha attaccato?» e' la domanda che il report deve evitare
				// di far nascere.
				//
				// `!Target` copre il caso REALE, e non era ovvio: `DestroyDefeatedUnits` rimuove le unita'
				// abbattute a fine turno, quindi un morto non si osserva come `IsAlive() == false` ma come weak
				// pointer NULLO. Il controllo su `IsAlive()` da solo non scattava mai. L'id, invece, e' garantito
				// dichiarato: il loader rifiuta un intent che nomini un'unita' inesistente, quindi qui un
				// puntatore nullo significa «c'era e non c'e' piu'», non «non e' mai esistita».
				Notes.Add(FString::Printf(TEXT("turno %d: '%s' bersagliava '%s', gia' abbattuto: l'azione non parte"),
					TurnIndex + 1, *Intent.UnitId, *Intent.Target));
			}
			else
			{
				Unit->PlannedAbilityIndex = AbilityIndex;
				Unit->PlannedAttackTarget = Target;
			}
		}
		else if (!Intent.Ability.IsNone() && Intent.bTargetsCell)
		{
			// Bersaglio a CELLA: nessun controllo sull'esistenza di un'unita' li' sopra, ed e' il punto —
			// un'area si centra dove si vuole, anche su un varco vuoto. Che poi colpisca qualcuno lo decide
			// il raggio, in fase Blast.
			const int32 AbilityIndex = FindAbilityIndex(Intent.Ability);
			if (AbilityIndex == INDEX_NONE)
			{
				ErroredBy = FString::Printf(TEXT("'%s' non possiede l'abilita' '%s' (turno %d)"),
					*Intent.UnitId, *Intent.Ability.ToString(), TurnIndex + 1);
				UE_LOG(LogRT, Error, TEXT("[RT-Test] %s: %s"), *Scenario.ScenarioId, *ErroredBy);
			}
			else
			{
				Unit->PlannedAbilityIndex = AbilityIndex;
				Unit->PlannedAttackTarget = nullptr;
				Unit->PlannedAttackCell = Intent.TargetCell;
				Unit->bAttackTargetsCell = true;

				// Il BORDO, per le azioni che agiscono su una struttura di bordo (CP 9.5). Si scrive solo se
				// lo scenario lo dichiara: senza flag, il resolver rifiuta con `CoverRejected` invece di
				// scegliere un lato — ed e' cio' che deve succedere se il file lo dimentica.
				if (Intent.bHasCoverEdge)
				{
					Unit->PlannedCoverEdge = Intent.CoverEdge;
					Unit->bHasPlannedCoverEdge = true;
				}
			}
		}

		// --- reazione -------------------------------------------------------------------------------------
		// Slot diverso dall'abilita': la stessa unita' puo' attaccare E tenere armata una reazione nello
		// stesso turno. Nessun bersaglio da impostare — chi subira' la reazione lo decide il trigger durante
		// la risoluzione, non lo scenario.
		if (!Intent.Reaction.IsNone())
		{
			for (int32 I = 0; I < Unit->NumAbilities(); ++I)
			{
				const URTActionData* Action = Unit->GetAbility(I);
				if (Action && Action->Def.ActionId == Intent.Reaction)
				{
					Unit->PlannedReactionAbility = I;
					break;
				}
			}
			// Nessun ramo d'errore: che l'eroe possieda la reazione e che sia davvero una reazione lo ha gia'
			// verificato il loader, che rifiuta lo scenario con un motivo invece di lasciarlo girare a vuoto.
		}

		// --- condizione dichiarata sulla reazione ([D-109], #583) -------------------------------------------
		// DOPO il blocco qui sopra, e l'ordine non e' cosmetico: `SetPlannedReactionCondition` rifiuta se
		// `PlannedReactionAbility` e' ancora `INDEX_NONE`, quindi applicarla prima significherebbe scrivere una
		// condizione che non entra — in silenzio, che e' il modo in cui questo campo ha gia' fallito una volta.
		//
		// Passa dal SETTER e non dal campo: e' la stessa porta di `rt.Reaction.Condition`, il produttore reale in
		// partita, quindi lo scenario esercita la validazione vera invece di scavalcarla. Scrivere
		// `Unit->PlannedReactionCondition = ...` direttamente farebbe passare condizioni che il gioco rifiuta —
		// un harness che sa fare piu' del gioco produce verdi che non descrivono nessuna partita.
		if (Intent.Condition.IsDeclared())
		{
			if (!Unit->SetPlannedReactionCondition(Intent.Condition))
			{
				// ERROR, non un `ensure` che poi lascia proseguire. Il loader ha gia' verificato entrambi i motivi
				// di rifiuto — reazione armata e condizione ammessa — quindi un `false` qui significa che le due
				// validazioni si sono disallineate: e' un difetto di CHI HA SCRITTO il codice, non un esito di
				// gioco. Proseguendo, lo scenario girerebbe SENZA condizione, l'opportunity non collasserebbe, e
				// il report direbbe FAIL su un'assertion del TurnLog — mandando a cercare una regressione che non
				// esiste. E' la stessa ragione, e lo stesso meccanismo, del ramo «non possiede l'abilita'» qui
				// sopra: `ErroredBy` fa uscire `Finish()` con ERROR, che ha la precedenza su FAIL.
				ErroredBy = FString::Printf(
					TEXT("la condizione '%s' (%d) di '%s' e' stata rifiutata dal piano dopo essere passata dal ")
					TEXT("loader: le due validazioni non dicono piu' la stessa cosa (turno %d)"),
					*Intent.Condition.Id.ToString(), Intent.Condition.Param, *Intent.UnitId, TurnIndex + 1);
				UE_LOG(LogRT, Error, TEXT("[RT-Test] %s: %s"), *Scenario.ScenarioId, *ErroredBy);
			}
		}

		// --- rotazione dichiarata (D-020, #291) ------------------------------------------------------------
		// Si scrive e basta: la LEGALITA' non si valuta qui. Il resolver la verifica a fine Move su
		// `MovementStyleThisTurn` e `WalkedThisTurn` — cioe' su quel che e' successo davvero — e produce
		// `DeclaredInPlanning` oppure `DeclarationRejected` nel TurnLog.
		//
		// ⚠️ **Un rifiuto e' un esito che uno scenario puo' voler dimostrare, non un errore da prevenire.**
		// Se l'harness filtrasse qui le rotazioni illegali, `Spec.Facing.IllegalDeclaredRotationIsRejected`
		// non sarebbe scrivibile: verificherebbe che l'harness sa contare, non che il gioco sa rifiutare.
		if (Intent.bDeclaresFacing)
		{
			Unit->PlannedFacing = Intent.Facing;
			Unit->bDeclaresPlannedFacing = true;
		}

		if (Intent.Move.Num() == 0)
		{
			continue;
		}

		// Stessa strada del controller: i waypoint diventano un percorso composito calcolato sullo snapshot
		// AUTOREVOLE. Percorso non valido (budget, blocchi, occupanti) -> l'unita' resta ferma e l'assertion
		// lo mostra: e' il comportamento del gioco, non un caso speciale del test.
		TArray<ARTUnit*> SnapshotUnits;
		const FRTHexSnapshot Snapshot = TM->MakeCurrentSnapshot(SnapshotUnits);
		const int32 UnitId = SnapshotUnits.IndexOfByKey(Unit);
		if (UnitId == INDEX_NONE)
		{
			continue;
		}

		const FRTHexPathResult Path = URTHexSimLibrary::BuildCompositeHexPath(Snapshot, UnitId, Intent.Move);
		if (Path.Path.Num() >= 2)
		{
			Unit->PlannedWaypoints = Intent.Move;
			Unit->PlannedPath = Path.Path;
			Unit->PlannedCell = Path.Path.Last();
		}
		else
		{
			Notes.Add(FString::Printf(TEXT("turno %d: percorso rifiutato per '%s': l'unita' resta ferma"),
				TurnIndex + 1, *Intent.UnitId));
			UE_LOG(LogRT, Warning, TEXT("[RT-Test] %s: percorso rifiutato per '%s' (l'unita' resta ferma)"),
				*Scenario.ScenarioId, *Intent.UnitId);
		}
	}
}

void FRTScenarioSession::BeginTurn()
{
	ARTTurnManager* TM = TurnManager.Get();
	if (!TM || (!Scenario.bFreeRun && !Scenario.Turns.IsValidIndex(TurnIndex)))
	{
		Finish();
		return;
	}

	// In FREE-RUN non esiste un turno da leggere: gli intent non li scrive nessuno e i requisiti sono dello
	// scenario, gia' valutati in `Start()`. Il resto del corpo — azzeramento dei piani, pianificazione dei
	// bot, `LockInAndResolve` — resta identico, ed e' cio' che tiene **una sola** strada di esecuzione: se il
	// free-run avesse un ciclo suo, un verde su uno scenario a turni non direbbe piu' niente sull'altro.
	//
	// ⚠️ Locale e NON `static`: `FRTScenarioTurn` possiede tre `TArray`, e un contenitore statico di funzione
	// esegue il proprio distruttore al teardown degli statici C++ — dopo che `FMalloc` puo' essere gia' stato
	// smontato. Un valore sempre vuoto non guadagna niente dalla cache, e tre `TArray` vuoti non allocano.
	const FRTScenarioTurn EmptyTurn;
	const FRTScenarioTurn& Turn = Scenario.bFreeRun ? EmptyTurn : Scenario.Turns[TurnIndex];

	// PRIMA passata: un nome che il vocabolario non conosce e' un refuso di chi ha scritto lo scenario, non
	// un'attesa del gioco — `Error`, che ha precedenza su tutto (vedi `ErroredBy`).
	//
	// ⚠️ Va PRIMA della disponibilita', e non e' un dettaglio d'ordine: nello stesso `requires` un refuso puo'
	// stare accanto a una capability legittimamente assente, e chiedendo prima la disponibilita' il refuso si
	// nasconderebbe dietro il `Blocked` dell'altra senza che nulla lo dica. E' esattamente il caso che ha
	// tenuto invisibile `Facing` in `RT_Showcase_Relay_v01` turno 4: `["DecisionBoundary", "Facing"]`.
	for (const FString& Required : Turn.Requires)
	{
		if (!IsCapabilityKnown(Required))
		{
			ErroredBy = FString::Printf(
				TEXT("turno %d: la capability '%s' non esiste. Non e' un'attesa: e' un nome che nessun elenco ")
				TEXT("dichiara, quindi lo scenario e' scritto male. I nomi validi stanno in ")
				TEXT("`RTScenarioSession.cpp`, `AvailableCapabilities()` e `KnownUnavailableCapabilities()`."),
				TurnIndex + 1, *Required);
			Finish();
			return;
		}
	}

	// Il turno chiede qualcosa che il gioco non sa ancora fare? Ci si ferma QUI, dichiarando cosa manca.
	// Non si gioca "quel che si puo'" del turno: un turno a meta' produrrebbe uno stato che non corrisponde
	// ne' al gioco di oggi ne' a quello di domani, e ogni assertion successiva mentirebbe.
	for (const FString& Required : Turn.Requires)
	{
		if (!IsCapabilityAvailable(Required))
		{
			BlockedBy = FString::Printf(TEXT("turno %d: manca la capability '%s'"), TurnIndex + 1, *Required);
			Finish();
			return;
		}
	}

	// La coda delle risposte di QUESTO turno. Si ripopola a ogni turno e non si accumula: una decisione del
	// turno 1 rimasta in coda risponderebbe a una finestra del turno 2, e lo scenario direbbe una cosa che
	// non ha scritto. Sta dopo i due controlli sulle capability perche' un turno che non si gioca non ha
	// finestre da servire.
	PendingDecisions = Turn.Decisions;
	PendingConsumed.Init(false, PendingDecisions.Num());
	AppliedDecisionDescs.Reset();
	// Lo snapshot si ricattura al primo boundary di QUESTO turno: fra un turno e l'altro le unita' muoiono e
	// il resolver rifa' il proprio array, quindi tenerlo sarebbe peggio che ricostruirlo.
	RuntimeUnitsForTurn.Reset();

	// Tutte ferme per default: un'unita' senza intent nel turno NON eredita il piano del turno prima.
	for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
	{
		if (ARTUnit* U = Pair.Value.Get())
		{
			U->PlannedCell = U->Cell;
			U->PlannedPath.Reset();
			U->PlannedWaypoints.Reset();
			// Anche il piano d'ATTACCO: senza, un'unita' che ha attaccato al turno 1 continuerebbe a farlo
			// nei turni successivi senza che lo scenario glielo chieda.
			U->PlannedAbilityIndex = INDEX_NONE;
			U->PlannedAttackTarget = nullptr;
			U->bAttackTargetsCell = false;
			// E il BORDO dichiarato: senza, un pannello eretto al turno 1 lascerebbe il lato scritto nel piano
			// e un'azione di struttura del turno 3 lo troverebbe gia' pronto senza averlo chiesto.
			U->bHasPlannedCoverEdge = false;
			// Lo SCATTO non si azzera qui, ed e' deliberato: `RTTurnManager` lo consuma a ogni risoluzione
			// («consumato per questo turno, valido o no»), quindi un reset in questo punto sarebbe una seconda
			// copia della stessa regola — quella che smette di essere aggiornata quando la prima cambia.
			//
			// Verificato per mutazione: azzerarlo qui non fa cadere alcun test, perche' non c'e' niente da
			// azzerare. E' il modo in cui questa riga, scritta d'istinto insieme alle due sopra, e' stata tolta.
			// E la reazione armata: il turn manager la consuma da solo, ma solo se il trigger scatta. Senza
			// questo azzeramento una reazione mai scattata resterebbe armata per tutto lo scenario, e un
			// turno successivo la vedrebbe partire senza che nessun intent l'abbia chiesta.
			U->ClearReactionPlan();
		}
	}

	// Gli intent del turno diventano piani sulle unita': e' la parte lunga, e sta in una funzione propria.
	// In free-run non ce ne sono — `TurnIndex` non indicizza niente e ogni unita' e' guidata dal bot, che
	// pianifica poche righe piu' sotto dallo stesso ingresso di sempre.
	if (!Scenario.bFreeRun)
	{
		ApplyScenarioIntents(*TM);
	}

	// Uno scenario scritto male non si gioca: fermarsi qui evita di produrre uno stato che nessuna assertion
	// puo' interpretare, e soprattutto evita di riportarlo come se fosse un verdetto sul gioco.
	if (!ErroredBy.IsEmpty())
	{
		Finish();
		return;
	}

	// Le unita' `bot` decidono ORA, con gli intent scriptati gia' scritti: e' lo stesso ordine di una partita
	// vera, dove il giocatore blocca il piano e poi il turn manager pianifica per i suoi.
	//
	// `PlanBots` azzera il piano delle SOLE unita' che guida, quindi non tocca ne' sovrascrive gli intent del
	// file. Chiamarlo prima li avrebbe invece esposti al rischio opposto — un intent scriptato che sovrascrive
	// una decisione del bot — ed e' la ragione per cui l'ordine e' questo e non l'altro.
	if (bHasBotUnits)
	{
		TM->PlanBotsForTest();
	}

	TM->LockInAndResolve();
	State = EState::Resolving;
	ResolveTicks = 0;
}

void FRTScenarioSession::Step(float DeltaSeconds, bool bPumpTurnManager)
{
	ARTTurnManager* TM = TurnManager.Get();
	if (State == EState::Finished || !TM)
	{
		return;
	}

	switch (State)
	{
	case EState::PauseBeforeTurn:
	{
		// La partita puo' essersi decisa prima della fine dello scenario: i turni restanti non esistono.
		if (TM->GetPhase() == ERTMatchPhase::MatchEnded)
		{
			Finish();
			return;
		}
		PauseElapsed += DeltaSeconds;
		if (PauseElapsed >= TurnPauseSeconds)
		{
			BeginTurn();
		}
		break;
	}

	case EState::Resolving:
	{
		// In gioco e' il mondo a ticcare il turn manager: pomparlo anche qui lo farebbe correre al doppio
		// della velocita', e il playback che si vuole GUARDARE passerebbe in meta' del tempo.
		if (bPumpTurnManager)
		{
			TM->Tick(DeltaSeconds);
		}

		if (!TM->IsResolving())
		{
			// Il TurnLog di QUESTO turno, prima che il prossimo `LockInAndResolve` lo azzeri. E' l'unico
			// istante in cui e' completo e ancora esistente: leggerlo a scenario finito darebbe l'ultimo turno
			// soltanto, e un'assertion su tre turni sarebbe verde per il motivo sbagliato.
			ScenarioLog.Append(TM->GetTurnLog());

			// La stessa traccia, serializzata e tenuta SEPARATA per turno: e' cio' che il corpus golden
			// confronta (CP 12.6, #178). `ScenarioLog` accumula tutto di seguito e perde i confini, che sono
			// esattamente l'informazione con cui una divergenza dice «turno 3».
			//
			// Senza `FormatId`: il turn manager non lo espone, e una traccia che dichiara un formato non e'
			// confrontabile con una che non lo fa (`GoldenCorpusRejectsFormatMismatch`). Meglio nessun formato
			// da entrambe le parti che un formato inventato da una sola.
			FRTTurnTrace Trace;
			Trace.Bytes = URTTurnLogLibrary::SerializeTurnLog(TM->GetTurnLog(), ERTLogTopology::Hex);
			Result.TurnTraces.Add(Trace);

			// `HoldRejected` significa «il manager ha rifiutato la risposta»: sul gioco ha lo stesso effetto
			// di un `HOLD`, e nel referto deve avere il significato opposto. Si legge dal TurnLog invece di
			// duplicare `IsResponseAllowed` qui — la legalita' resta decisa in UN posto solo.
			//
			// ⚠️ Il filtro su `Category` NON e' opzionale: `FRTTurnLogEntry::Outcome` e' un `uint8` il cui
			// significato lo decide la categoria (`ERTMoveOutcome` se `Move`, e cosi' via), e il file lo
			// dichiara. Senza il filtro, una voce di movimento con lo stesso valore numerico sarebbe letta
			// come un rifiuto. Il piano ometteva il filtro e lo sospettava soltanto.
			//
			// Il messaggio nomina le decisioni con gli id di SCENARIO: il token e' `FIRE:<indice>`, e
			// riportarlo manderebbe a rileggere il turno per capire quale risposta sia stata rifiutata.
			//
			// ⚠️ E solo se a rispondere e' stata QUESTA sessione: con `test-override` o con un'unita' del bot
			// il rifiuto non riguarda nessuna decisione dello scenario, e il referto accuserebbe «una risposta
			// scriptata» che nessuno ha scritto — seguita da un elenco di decisioni applicate vuoto, cioe'
			// niente da andare a cercare.
			const bool bHaRispostoQuestaSessione =
				DecisionSource == TEXT("scenario") && AppliedDecisionDescs.Num() > 0;
			for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
			{
				if (!bHaRispostoQuestaSessione) { break; }
				if (Entry.Category != ERTLogCategory::ReactionDecision) { continue; }
				if (static_cast<ERTReactionDecisionOutcome>(Entry.Outcome)
					!= ERTReactionDecisionOutcome::Rejected)
				{
					continue;
				}
				const FString Motivo = FString::Printf(
					TEXT("turno %d: una risposta scriptata e' stata rifiutata dal resolver — il bersaglio non ")
					TEXT("era fra quelli offerti dalla finestra. Decisioni applicate in questo turno: %s"),
					TurnIndex + 1, *FString::Join(AppliedDecisionDescs, TEXT("; ")));
				if (ErroredBy.IsEmpty()) { ErroredBy = Motivo; }
				Notes.Add(Motivo);
				break;
			}

			// Il residuo si valuta a fine turno, non a fine scenario: una decisione dichiarata al T2 e mai
			// consumata e' un difetto del T2, e attribuirla al T8 manderebbe a cercare nel posto sbagliato.
			//
			// ⚠️ Passa da `ErroredBy` e NON da `Result.Outcome`: `Finish()` ricalcola l'esito con una catena
			// `if/else` a partire da `ErroredBy`, quindi un `Result.Outcome = Error` scritto qui verrebbe
			// riportato a `Pass`. Il primo errore vince — gli altri restano nelle note.
			for (int32 Index = 0; Index < PendingDecisions.Num(); ++Index)
			{
				if (PendingConsumed[Index]) { continue; }
				++Result.ScriptedDecisionsUnused;
				const FString Motivo = FString::Printf(
					TEXT("turno %d: decisione dichiarata per '%s' (%s) e mai consumata — nessuna finestra si e' ")
					TEXT("aperta per quell'unita'"),
					TurnIndex + 1, *PendingDecisions[Index].Unit, *PendingDecisions[Index].Respond);
				if (ErroredBy.IsEmpty()) { ErroredBy = Motivo; }
				Notes.Add(Motivo);
			}

			++TurnIndex;
			Result.TurnsPlayed = TurnIndex;
			// Chi decide che non c'e' un altro turno: il FILE quando i turni sono enumerati, la PARTITA in
			// free-run. Li' l'unica cosa che ferma la sessione da sola e' il tetto: la fine partita la coglie
			// `PauseBeforeTurn` al giro successivo, dallo stesso controllo che serve anche a uno scenario a
			// turni la cui partita si decide prima dell'ultimo — un secondo controllo qui sarebbe la copia che
			// smette di essere aggiornata.
			const bool bNoMoreTurns = Scenario.bFreeRun
				? (TurnIndex >= Scenario.MaxTurns)
				: (TurnIndex >= Scenario.Turns.Num());
			if (bNoMoreTurns)
			{
				Finish();
			}
			else
			{
				State = EState::PauseBeforeTurn;
				PauseElapsed = 0.f;
			}
			break;
		}

		// Tetto di sicurezza: una risoluzione che non finisce deve FALLIRE, non girare all'infinito. Senza,
		// un test appeso somiglierebbe a un test lento, e la differenza si scoprirebbe solo aspettando.
		if (++ResolveTicks > URTScenarioRunner::MaxResolveTicks)
		{
			Result.Outcome = ERTTestOutcome::Error;
			Result.ErrorMessage = FString::Printf(
				TEXT("il turno %d non ha finito di risolvere entro %d passi"),
				TurnIndex + 1, URTScenarioRunner::MaxResolveTicks);
			State = EState::Finished;
		}
		break;
	}

	default:
		break;
	}
}

FString FRTScenarioSession::DecideScriptedResponse(const FRTReactionOpportunity& Opportunity, int32 OwnerUnitId)
{
	// ⚠️ **`OwnerUnitId` NON e' lo `StableUnitId`, ed e' la correzione che il piano chiedeva al contrario.**
	// Misurato: `Key.OwnerId = Watcher.Zone.OwnerUnitId`, e la zona nasce da
	// `MakeSuppressiveZone(Map, OwnerIdx, ...)` dove `OwnerIdx = Units.IndexOfByKey(WatchOwner)`. Anche i
	// bersagli viaggiano cosi' — `M.UnitId = TargetIdx`. Tutto il giro delle reazioni parla di INDICI
	// nell'array di risoluzione, e `RTTurnManager.cpp` lo dichiara: «l'identita' e' l'INDICE dell'unita' in
	// OutUnits ... il chiamante ritrova la propria unita' con OutUnits.IndexOfByKey».
	//
	// Usando `StableUnitId` il proprietario non si sarebbe risolto MAI, e — peggio — il token
	// `FIRE:<StableUnitId>` non sarebbe stato in `AllowedResponses`: rifiutato come risposta inventata, cioe'
	// un `HoldRejected` che nel referto somiglia a un HOLD voluto. Il difetto che il task 9 vuole impedire.
	ARTTurnManager* TM = TurnManager.Get();
	if (!TM) { return FString(); }

	// 🔴 **Una volta per turno, non a ogni finestra.** `MakeCurrentSnapshot` filtra `IsAlive()`, mentre il
	// resolver costruisce il proprio array UNA volta per l'intera risoluzione: un `FIRE` che uccide un mover
	// lo toglierebbe dallo snapshot successivo e sposterebbe di uno tutti gli indici a valle, facendo
	// mappare la finestra dopo sull'unita' sbagliata — o emettere un `FIRE:<indice errato>` che
	// `IsResponseAllowed` rifiuta come inventato. Catturarlo alla prima finestra tiene lo stesso spazio di
	// id per tutta la risoluzione, ed evita anche un `GetAllActorsOfClass` per micro-step.
	if (RuntimeUnitsForTurn.Num() == 0)
	{
		TM->CollectLivingUnits(RuntimeUnitsForTurn); // servono le unita', non lo snapshot: stesso filtro, stesso ordine
	}
	const TArray<ARTUnit*>& RuntimeUnits = RuntimeUnitsForTurn;
	if (!RuntimeUnits.IsValidIndex(OwnerUnitId)) { return FString(); }
	const ARTUnit* OwnerUnit = RuntimeUnits[OwnerUnitId];

	// Risale allo scenario id del proprietario: `UnitsById` va nel verso opposto, e una scansione su quattro
	// unita' costa meno di una seconda mappa da tenere allineata.
	FString OwnerScenarioId;
	for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
	{
		if (Pair.Value.Get() == OwnerUnit) { OwnerScenarioId = Pair.Key; break; }
	}
	if (OwnerScenarioId.IsEmpty()) { return FString(); }

	for (int32 Index = 0; Index < PendingDecisions.Num(); ++Index)
	{
		if (PendingConsumed[Index]) { continue; }
		const FRTScenarioDecision& D = PendingDecisions[Index];
		if (D.Unit != OwnerScenarioId) { continue; }

		// 🔴 **La decisione si consuma solo se si riesce davvero a tradurla.** Segnarla consumata qui sopra —
		// come faceva la prima stesura — significava che una traduzione fallita usciva con un `HOLD` per
		// timeout mentre il referto diceva `applied=1`, `unused=0`, nessuna nota, esito `PASS`: esattamente
		// il «un test smette di verificare senza dirlo» che questa feature esiste per impedire.
		const auto Consuma = [&](const FString& Token) -> FString
		{
			PendingConsumed[Index] = true;
			++Result.ScriptedDecisionsApplied;
			AppliedDecisionDescs.Add(D.Target.IsEmpty()
				? FString::Printf(TEXT("'%s' %s"), *D.Unit, *D.Respond)
				: FString::Printf(TEXT("'%s' %s -> '%s'"), *D.Unit, *D.Respond, *D.Target));
			Result.LastScriptedResponse = Token;
			return Token;
		};

		if (D.Respond.Equals(TEXT("HOLD"), ESearchCase::CaseSensitive))
		{
			// ⚠️ **Anche `HOLD` si verifica contro la finestra, dal 2026-08-19.** Sembra la risposta che non
			// puo' mai essere illegale, e con il solo Overwatch lo era davvero — `BuildOverwatchTriggers` la
			// aggiunge sempre. Il `Brace` di [D-047] ha rotto quella premessa: le sue risposte sono
			// `{Hold Ground, SIDESTEP}` e `HOLD` non c'e'. Senza questo controllo uno scenario che scrivesse
			// `respond: "HOLD"` su una finestra di `Brace` la vedeva **consumata e contata come applicata**
			// (`applied=1`, `unused=0`, nessuna nota, esito `PASS`) mentre il resolver la rifiutava con
			// `HoldRejected` — cioe' esattamente il «un test smette di verificare senza dirlo» che il ramo
			// delle risposte di profilo, dieci righe piu' giu', era stato scritto per impedire.
			// 💡 La correzione era stata applicata al ramo nuovo e non al suo gemello: un criterio applicato a
			// meta' e' come non applicarlo, perche' l'esito verde arriva comunque dalla strada scoperta.
			if (!URTReactionOpportunityLibrary::IsResponseAllowed(
				Opportunity, URTReactionOpportunityLibrary::HoldResponse()))
			{
				const FString Motivo = FString::Printf(
					TEXT("turno %d: '%s' risponde 'HOLD', che non e' fra le risposte legali della sua finestra")
					TEXT(" (una finestra di `Brace` usa 'Hold Ground')"),
					TurnIndex + 1, *D.Unit);
				if (ErroredBy.IsEmpty()) { ErroredBy = Motivo; }
				Notes.Add(Motivo);
				// ⚠️ **Consumata, benche' rifiutata**, e senza incrementare `ScriptedDecisionsApplied`: la
				// decisione E' stata abbinata alla sua finestra: cio' che e' fallito e' la risposta. Lasciandola
				// non consumata, il residuo di fine turno aggiungeva una **seconda** nota che diceva l'opposto —
				// «nessuna finestra si e' aperta per quell'unita'» — e chi legge il referto si trovava due
				// diagnosi contraddittorie dello stesso fallimento, con la seconda che manda a cercare una
				// finestra mancante che invece c'era.
				PendingConsumed[Index] = true;
				return FString();
			}
			return Consuma(URTReactionOpportunityLibrary::HoldResponse());
		}

		// ➕ **E14.7 fetta 4: le risposte di un Reaction Profile passano LETTERALI.** `Hold Ground`,
		// `SIDESTEP` e le altre non hanno un bersaglio da tradurre e non hanno un token di runtime: sono gia'
		// esattamente le stringhe che `URTCatalogLibrary::BraceAllowedResponses` mette in `AllowedResponses`,
		// quindi `IsResponseAllowed` le riconosce senza conversione. E' il contrario di `FIRE:`, che esiste
		// perche' lo scenario NON puo' scrivere un id di runtime.
		//
		// ⚠️ **Si verifica che la risposta sia legale in QUESTA finestra, e non basta che il catalogo la
		// conosca**: il loader ha gia' rifiutato i refusi, ma un profilo sbagliato — `SIDESTEP` chiesto a
		// Riktor, che non ce l'ha — supererebbe il loader e arriverebbe qui. Senza questo controllo
		// `IsResponseAllowed` la rifiuterebbe nel resolver producendo un `HoldRejected`, che nel referto
		// somiglia a un HOLD voluto: il difetto che la traduzione fallita di `FIRE` ha gia' pagato una volta,
		// dieci righe piu' su.
		if (URTCatalogLibrary::IsKnownReactionProfileResponse(D.Respond))
		{
			if (!URTReactionOpportunityLibrary::IsResponseAllowed(Opportunity, D.Respond))
			{
				const FString Motivo = FString::Printf(
					TEXT("turno %d: '%s' risponde '%s', che non e' fra le risposte legali della sua finestra"),
					TurnIndex + 1, *D.Unit, *D.Respond);
				if (ErroredBy.IsEmpty()) { ErroredBy = Motivo; }
				Notes.Add(Motivo);
				// Consumata benche' rifiutata, per la stessa ragione del gemello sopra: la decisione ha trovato
				// la propria finestra, e una seconda nota sul residuo direbbe il contrario.
				PendingConsumed[Index] = true;
				return FString();
			}
			return Consuma(D.Respond);
		}
		// `FIRE`: il token porta l'id di RUNTIME, che e' esattamente cio' che lo scenario non poteva scrivere.
		// Stesso spazio di id del proprietario — l'indice nell'array di risoluzione, non lo `StableUnitId`.
		const TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(D.Target);
		ARTUnit* TargetUnit = Found ? Found->Get() : nullptr;
		const int32 TargetRuntimeId = TargetUnit ? RuntimeUnits.IndexOfByKey(TargetUnit) : INDEX_NONE;
		if (TargetRuntimeId == INDEX_NONE)
		{
			// Difesa, non politica: il loader e `Validate` lo hanno gia' rifiutato. Ma se ci si arriva —
			// bersaglio caduto, o fuori dallo snapshot — il turno deve DIRLO invece di scivolare in un
			// timeout muto. La decisione resta non consumata, quindi il residuo la conta.
			const FString Motivo = FString::Printf(
				TEXT("turno %d: il bersaglio '%s' della decisione di '%s' non e' risolvibile a runtime"),
				TurnIndex + 1, *D.Target, *D.Unit);
			if (ErroredBy.IsEmpty()) { ErroredBy = Motivo; }
			Notes.Add(Motivo);
			return FString();
		}
		return Consuma(URTReactionOpportunityLibrary::FireResponse(TargetRuntimeId));
	}

	if (PendingDecisions.Num() > 0)
	{
		// Il turno dichiara decisioni e questa finestra non ne ha trovata nessuna: non e' il caso «turno non
		// scriptato», e' una finestra SCOPERTA. Se restasse un timeout silenzioso, due decisioni scritte e
		// una applicata sarebbero verdi — che e' il modo in cui un test smette di verificare senza dirlo.
		const FString Motivo = FString::Printf(
			TEXT("turno %d: finestra aperta per '%s' senza una decisione che la nomini"),
			TurnIndex + 1, *OwnerScenarioId);
		if (ErroredBy.IsEmpty()) { ErroredBy = Motivo; }
		Notes.Add(Motivo);
	}

	// Nessuna decisione combacia: «non ho risposto». E' il comportamento di sempre — `DecisionOnTimeout` —
	// e tiene intatti i turni che non scriptano nulla.
	return FString();
}

void FRTScenarioSession::TearDown()
{
	for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
	{
		if (ARTUnit* Unit = Pair.Value.Get())
		{
			Unit->Destroy();
		}
	}
	UnitsById.Reset();

	// Il turn manager per ULTIMO: distruggerlo prima lascerebbe le unita' senza l'attore che le conosce, e un
	// suo callback in coda troverebbe un roster mezzo vuoto.
	if (ARTTurnManager* TM = TurnManager.Get())
	{
		// Sbinda PRIMA di distruggere: un delegate che sopravvive a uno scenario risponderebbe al successivo
		// con la coda del precedente, e il secondo sarebbe verde o rosso per il turno di un altro. Conta
		// davvero perche' `SetUp` RIUSA un turn manager gia' presente invece di spawnarne sempre uno nuovo.
		//
		// ⚠️ `TearDown()` non e' l'unica strada, ed e' il motivo per cui esiste anche il distruttore: il
		// percorso normale (`RunSingle` con `bTearDownAfter=false`) non passa mai di qui.
		UnbindOwnDecider();
		TM->Destroy();
	}
	TurnManager.Reset();
	PendingDecisions.Reset();
	PendingConsumed.Reset();
}

void FRTScenarioSession::Finish()
{
	// Piani AZZERATI: se restassero appesi, qualunque cosa risolvesse un turno dopo lo scenario li
	// ri-eseguirebbe. E' successo davvero, e in PIE sembrava lo scenario stesso.
	for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
	{
		if (ARTUnit* U = Pair.Value.Get())
		{
			U->PlannedCell = U->Cell;
			U->PlannedPath.Reset();
			U->PlannedWaypoints.Reset();
			// Anche il piano d'ATTACCO: senza, un'unita' che ha attaccato al turno 1 continuerebbe a farlo
			// nei turni successivi senza che lo scenario glielo chieda.
			U->PlannedAbilityIndex = INDEX_NONE;
			U->PlannedAttackTarget = nullptr;
			// E la reazione armata: il turn manager la consuma da solo, ma solo se il trigger scatta. Senza
			// questo azzeramento una reazione mai scattata resterebbe armata per tutto lo scenario, e un
			// turno successivo la vedrebbe partire senza che nessun intent l'abbia chiesta.
			U->ClearReactionPlan();
		}
	}

	// Digest dello stato finale. Copre unita', STATI, TERRENI MODIFICATI, STRUTTURE e PROGRESSO OBIETTIVI:
	// il DoD di CP 12.1 li chiedeva tutti, e fino al 2026-08-10 ne copriva solo il primo — due finali che
	// differivano per una cella in fiamme o una copertura eretta davano lo stesso hash (issue #81).
	//
	// Il calcolo vive in `URTMatchStateHashLibrary` e non piu' qui: una funzione che prende DATI si verifica
	// con un test diretto (`Simulation.ChecksumCoversEnvironment`), un blocco dentro `Finish()` no.
	{
		// La costruzione del digest e' UNA SOLA, condivisa con la partita ([D-084]): questo blocco la
		// scriveva a mano, e due costruzioni che divergono anche di un campo danno hash diversi per lo
		// stesso stato — senza che nessuno se ne accorga finche' non prova a confrontare un corpus con una
		// partita vera. Da qui in poi l'harness e' un chiamante come gli altri.
		TArray<ARTUnit*> UnitsForDigest;
		UnitsForDigest.Reserve(UnitsById.Num());
		for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
		{
			if (ARTUnit* Unit = Pair.Value.Get())
			{
				UnitsForDigest.Add(Unit);
			}
		}
		const TArray<FRTUnitStateDigest> UnitStates =
			URTMatchStateHashLibrary::BuildUnitDigests(UnitsForDigest);

		// Punteggi indicizzati per TeamId: la v0.1 e' 2v2, e il giudice della fine partita li tiene qui.
		TArray<int32> TeamScores;
		if (const ARTTurnManager* TM = TurnManager.Get())
		{
			TeamScores.Add(TM->GetTeamScore(0));
			TeamScores.Add(TM->GetTeamScore(1));
		}

		Result.StateHash = URTMatchStateHashLibrary::HashMatchState(Map, UnitStates, TeamScores);
	}

	// ⚠️ Scenario BLOCCATO: le assertion di fine scenario NON si valutano (`#582`).
	//
	// Misurerebbero lo stato di una partita che non e' stata giocata, e la precedenza degli esiti
	// (`FAIL > BLOCKED`) trasformerebbe l'attesa di una capability in un difetto del gioco. Il caso e' reale e
	// riproducibile: uno scenario il cui **primo** turno chiede una capability assente completa zero turni,
	// quindi `TurnsCompleted >= 1` cade e il report dice «FAIL (difetto del GIOCO)» per un file che sta
	// semplicemente aspettando. Gli scenari `BLOCKED` gia' in repo lo evitano per costruzione — hanno il
	// `requires` sul secondo turno, quindi il primo gira — ma e' una salvezza accidentale, non una regola.
	//
	// ⚠️ **Solo se NESSUN turno e' stato giocato**, e il confine e' stato stretto qui dopo che una prima
	// versione — «bloccato ⇒ salta le assertion» — ha fatto cadere `ShowcaseRelayV01RunsTurnOne`, che gioca
	// il turno 1 e si blocca al secondo: le sue assertion misurano uno stato che la partita ha davvero
	// raggiunto, e saltarle avrebbe tolto verifica invece di aggiungerne.
	//
	// E' la stessa distinzione del commento sulla precedenza: cio' che ha girato conta, cio' che il blocco ha
	// impedito no. Con zero turni giocati non c'e' nessuno stato da misurare — solo l'assenza di partita.
	const bool bBlocked = !BlockedBy.IsEmpty() && Result.TurnsPlayed == 0;

	// FREE-RUN: «la partita e' arrivata alla fine» e' un'ASSERTION, non un esito speciale (CP 47.4).
	//
	// ⚠️ Ed e' la scelta che fa cadere il tetto dalla parte giusta. Un tetto raggiunto che producesse `Pass`
	// renderebbe verde esattamente lo stallo di `#1088` — dodici round di soli spostamenti, zero combattimento,
	// pareggio allo scadere; un tetto che producesse `Error` direbbe «scenario scritto male» di un difetto del
	// GIOCO. Come assertion cade in `Fail` per la catena di precedenza che c'e' gia', e porta con se' expected
	// e actual come tutte le altre.
	//
	// Sta PRIMA delle `expect` dichiarate perche' e' la loro premessa: se la partita non e' finita, cio' che
	// segue misura uno stato intermedio, e leggerlo per primo dice subito quale delle due cose e' successa.
	//
	// ⚠️ **E vale per gli `Error` quanto per i `Blocked`**, che `bBlocked` da solo non copre: uno scenario con
	// un refuso nel `requires` non gioca nemmeno un turno, e un `MatchReachedEnd` rosso li' accuserebbe il
	// GIOCO di non aver finito una partita che nessuno ha giocato. E' la stessa regola che il commento qui
	// sopra dichiara per le `expect` — cio' che ha girato conta, cio' che non e' partito no.
	const bool bNonHaGiocato = !ErroredBy.IsEmpty() && Result.TurnsPlayed == 0;
	if (Scenario.bFreeRun && !bBlocked && !bNonHaGiocato)
	{
		const ARTTurnManager* TM = TurnManager.Get();
		const bool bEnded = TM && TM->GetPhase() == ERTMatchPhase::MatchEnded;

		FRTAssertionResult A;
		// `Kind` resta al default: questa assertion non e' dichiarabile da un file — la genera il free-run —
		// e nessun tipo nuovo la renderebbe piu' vera. Stesso precedente di `SameTurnLogAcrossVariants`.
		A.Description = TEXT("MatchReachedEnd");
		A.Expected = FString::Printf(TEXT("fine partita entro %d turni"), Scenario.MaxTurns);
		A.Turn = Result.TurnsPlayed;
		A.bPassed = bEnded;

		if (bEnded)
		{
			const FRTMatchResult Match = TM->GetMatchResult();
			A.Actual = FString::Printf(TEXT("%s %s, al turno %d"),
				*URTTurnRules::DescribeOutcome(Match.Outcome),
				*URTTurnRules::DescribeEndReason(Match.Reason),
				Result.TurnsPlayed);
		}
		else
		{
			A.Actual = FString::Printf(TEXT("ancora in corso dopo %d turni — il tetto non e' una fine partita"),
				Result.TurnsPlayed);
		}
		Result.Assertions.Add(A);
	}

	for (const FRTTestExpectation& Exp : Scenario.Expect)
	{
		if (bBlocked)
		{
			break;
		}

		FRTAssertionResult A;
		A.Kind = Exp.Kind;
		A.Turn = Result.TurnsPlayed;

		switch (Exp.Kind)
		{
		case ERTAssertionKind::UnitAtCell:
		{
			A.Description = FString::Printf(TEXT("UnitAtCell(%s)"), *Exp.UnitId);
			A.Expected = Exp.Cell.ToString();

			TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Unit = Found ? Found->Get() : nullptr;
			if (!Unit)
			{
				A.Actual = TEXT("unita' assente");
				A.bPassed = false;
			}
			else
			{
				A.Actual = Unit->Cell.ToString();
				A.bPassed = (Unit->Cell == Exp.Cell);
			}
			break;
		}
		case ERTAssertionKind::TurnsCompleted:
		{
			A.Description = TEXT("TurnsCompleted");
			A.Expected = FString::Printf(TEXT(">= %d"), Exp.Value);
			A.Actual = FString::FromInt(Result.TurnsPlayed);
			A.bPassed = (Result.TurnsPlayed >= Exp.Value);
			break;
		}
		case ERTAssertionKind::UnitHpEquals:
		{
			A.Description = FString::Printf(TEXT("UnitHpEquals(%s)"), *Exp.UnitId);
			A.Expected = FString::FromInt(Exp.Value);

			TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Unit = Found ? Found->Get() : nullptr;
			if (!Unit)
			{
				// Un'unita' ABBATTUTA puo' essere stata distrutta: distinguerlo da «non esiste» conta, perche'
				// sono due difetti diversi — uno di gioco, uno di scenario.
				A.Actual = TEXT("unita' assente (abbattuta o mai creata)");
				A.bPassed = false;
			}
			else
			{
				// Lo SCUDO si dichiara separatamente: qui si guardano gli HP, e un danno assorbito dallo scudo
				// deve risultare come «HP invariati», non come «nessun danno».
				A.Actual = FString::Printf(TEXT("%d (scudo %d)"), Unit->Health, Unit->Shield);
				A.bPassed = (Unit->Health == Exp.Value);
			}
			break;
		}
		case ERTAssertionKind::UnitAlive:
		{
			const bool bWantAlive = (Exp.Value != 0);
			A.Description = FString::Printf(TEXT("UnitAlive(%s)"), *Exp.UnitId);
			A.Expected = bWantAlive ? TEXT("viva") : TEXT("abbattuta");

			TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Unit = Found ? Found->Get() : nullptr;
			// Un'unita' rimossa dal mondo conta come abbattuta: e' il modo in cui il gioco toglie di mezzo chi
			// arriva a zero HP, e chiedere «e' viva?» a un puntatore nullo deve avere una risposta, non un crash.
			const bool bIsAlive = (Unit != nullptr && Unit->IsAlive());
			A.Actual = bIsAlive ? TEXT("viva") : TEXT("abbattuta");
			A.bPassed = (bIsAlive == bWantAlive);
			break;
		}
		case ERTAssertionKind::UnitFacing:
		{
			static const TCHAR* DirectionNames[6] = { TEXT("E"), TEXT("NE"), TEXT("NW"), TEXT("W"), TEXT("SW"), TEXT("SE") };
			const int32 WantIndex = FMath::Clamp(Exp.Value, 0, 5);
			A.Description = FString::Printf(TEXT("UnitFacing(%s)"), *Exp.UnitId);
			A.Expected = DirectionNames[WantIndex];

			TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Unit = Found ? Found->Get() : nullptr;
			if (!Unit)
			{
				A.Actual = TEXT("unita' assente");
				A.bPassed = false;
			}
			else
			{
				// Il facing LOGICO, non lo yaw dell'attore: e' il valore che le regole leggono, e l'unico che
				// abbia senso confrontare quando il playback puo' essere ancora a meta' interpolazione.
				const int32 ActualIndex = FMath::Clamp(static_cast<int32>(Unit->Facing), 0, 5);
				A.Actual = DirectionNames[ActualIndex];
				A.bPassed = (ActualIndex == WantIndex);
			}
			break;
		}
		// I due capi di un REDIRECT (#1060). Stessa voce, due campi: `OriginalTargetUnitId` dice da chi il colpo
		// e' partito, `UnitId` chi l'ha incassato. Entrambi portano uno `StableUnitId`, che e' anche cio' che
		// `ARTUnit` espone — quindi il confronto non passa da nessuna tabella di conversione.
		case ERTAssertionKind::OriginalTargetEquals:
		case ERTAssertionKind::EffectiveTargetEquals:
		{
			const bool bWantOriginal = (Exp.Kind == ERTAssertionKind::OriginalTargetEquals);
			A.Description = FString::Printf(TEXT("%s(%s)"),
				bWantOriginal ? TEXT("OriginalTargetEquals") : TEXT("EffectiveTargetEquals"), *Exp.UnitId);
			A.Expected = Exp.UnitId;

			TWeakObjectPtr<ARTUnit>* Found = UnitsById.Find(Exp.UnitId);
			const ARTUnit* Want = Found ? Found->Get() : nullptr;
			if (!Want)
			{
				// Un'unita' abbattuta puo' essere stata distrutta: e' lo stesso trattamento di `UnitHpEquals`,
				// e serve a non far sembrare un difetto di scenario cio' che e' un esito di gioco.
				A.Actual = TEXT("unita' attesa assente (abbattuta o mai creata)");
				A.bPassed = false;
				break;
			}

			// La PRIMA voce che dichiara un redirect. `OriginalTargetUnitId` valorizzato E' il marcatore: non
			// serve filtrare per categoria, perche' nessun'altra voce lo scrive — e filtrare per
			// `Reaction`/`Activated` legherebbe l'assertion all'unico produttore di oggi (l'interposizione),
			// mentre la domanda che pone — «da chi a chi e' passato il colpo?» — vale per qualunque redirect
			// che un giorno venisse aggiunto.
			const FRTTurnLogEntry* Redirect = nullptr;
			for (const FRTTurnLogEntry& Entry : ScenarioLog)
			{
				if (Entry.OriginalTargetUnitId != INDEX_NONE) { Redirect = &Entry; break; }
			}
			if (Redirect == nullptr)
			{
				// «Nessuno si e' interposto» e «si e' interposto per l'unita' sbagliata» sono due difetti
				// diversi: confonderli renderebbe il messaggio inutile proprio quando serve. Stesso trattamento
				// che `LogEventAmount` riserva all'evento assente.
				A.Actual = TEXT("nessun redirect nel TurnLog");
				A.bPassed = false;
				break;
			}

			const int32 ActualId = bWantOriginal ? Redirect->OriginalTargetUnitId : Redirect->UnitId;
			A.bPassed = (ActualId == Want->StableUnitId);

			// Il numero da solo non dice a chi appartiene: chi legge il referto avrebbe uno `StableUnitId` e
			// nessun modo di risalire all'unita' senza rileggere lo scenario.
			// ⚠️ **Fuori dal ramo del fallimento, e non e' cosmesi**: `Expected` porta un id di scenario
			// (`"V1"`), quindi lasciando il numero grezzo su `Actual` un'assertion PASSATA si leggerebbe
			// «Expected V1 / Actual 3» — due domini diversi ai due lati dello stesso confronto. Ogni altro
			// `Kind` tiene i due lati omogenei (cella contro cella, direzione contro direzione). Trovato da una
			// code review, che l'ha visto sul percorso verde: quello che nessuno rilegge.
			A.Actual = FString::FromInt(ActualId);
			for (const TPair<FString, TWeakObjectPtr<ARTUnit>>& Pair : UnitsById)
			{
				const ARTUnit* Other = Pair.Value.Get();
				if (Other && Other->StableUnitId == ActualId)
				{
					A.Actual = FString::Printf(TEXT("%s (id %d)"), *Pair.Key, ActualId);
					break;
				}
			}
			break;
		}
		case ERTAssertionKind::LogEventCount:
		{
			const FString EventName = URTScenarioLoader::DescribeLogEvent(Exp.LogCategory, Exp.LogOutcome);
			A.Description = FString::Printf(TEXT("LogEventCount(%s)"), *EventName);
			A.Expected = FString::FromInt(Exp.Value);

			int32 Found = 0;
			for (const FRTTurnLogEntry& Entry : ScenarioLog)
			{
				if (Entry.Category == Exp.LogCategory && Entry.Outcome == Exp.LogOutcome) { ++Found; }
			}
			A.Actual = FString::FromInt(Found);
			A.bPassed = (Found == Exp.Value);
			break;
		}
		case ERTAssertionKind::LogEventAmount:
		{
			const FString EventName = URTScenarioLoader::DescribeLogEvent(Exp.LogCategory, Exp.LogOutcome);
			A.Description = FString::Printf(TEXT("LogEventAmount(%s)"), *EventName);
			A.Expected = FString::FromInt(Exp.Value);

			// La PRIMA occorrenza: sommarle mescolerebbe finestre diverse in un numero solo, e il residuo di
			// una decisione non e' la somma dei residui.
			const int32 At = IndexOfScenarioLogEvent(ScenarioLog, Exp.LogCategory, Exp.LogOutcome);
			if (At == INDEX_NONE)
			{
				// Assente non e' «vale zero»: dirlo cosi' manderebbe a cercare un valore sbagliato dove il
				// problema e' che l'evento non e' mai stato prodotto. Stesso trattamento di `LogEventOrder`.
				A.Actual = FString::Printf(TEXT("%s assente"), *EventName);
				A.bPassed = false;
			}
			else
			{
				A.Actual = FString::FromInt(ScenarioLog[At].Amount);
				A.bPassed = (ScenarioLog[At].Amount == Exp.Value);
			}
			break;
		}
		case ERTAssertionKind::LogEventOrder:
		{
			const FString FirstName = URTScenarioLoader::DescribeLogEvent(Exp.LogCategory, Exp.LogOutcome);
			const FString ThenName = URTScenarioLoader::DescribeLogEvent(Exp.ThenCategory, Exp.ThenOutcome);
			A.Description = FString::Printf(TEXT("LogEventOrder(%s prima di %s)"), *FirstName, *ThenName);
			A.Expected = FString::Printf(TEXT("%s prima di %s"), *FirstName, *ThenName);

			const int32 FirstAt = IndexOfScenarioLogEvent(ScenarioLog, Exp.LogCategory, Exp.LogOutcome);
			const int32 ThenAt = IndexOfScenarioLogEvent(ScenarioLog, Exp.ThenCategory, Exp.ThenOutcome);

			// Un evento ASSENTE non e' «fuori ordine»: e' un altro difetto, e dirlo cosi' evita di mandare a
			// cercare un problema di sequenza dove il problema e' che l'evento non e' mai stato prodotto.
			if (FirstAt == INDEX_NONE || ThenAt == INDEX_NONE)
			{
				A.Actual = FString::Printf(TEXT("%s%s%s"),
					FirstAt == INDEX_NONE ? *FString::Printf(TEXT("%s assente"), *FirstName) : TEXT(""),
					(FirstAt == INDEX_NONE && ThenAt == INDEX_NONE) ? TEXT(", ") : TEXT(""),
					ThenAt == INDEX_NONE ? *FString::Printf(TEXT("%s assente"), *ThenName) : TEXT(""));
				A.bPassed = false;
			}
			else
			{
				A.Actual = FString::Printf(TEXT("posizioni %d e %d su %d voci"), FirstAt, ThenAt, ScenarioLog.Num());
				A.bPassed = (FirstAt < ThenAt);
			}
			break;
		}
		default:
			A.Description = TEXT("assertion non implementata");
			A.bPassed = false;
			break;
		}

		Result.Assertions.Add(A);
	}

	Result.Notes = Notes;
	// La provenienza si SCRIVE invece di dedurla. Deciso al bind in `SetUp`, copiato qui accanto alle note
	// perche' `Finish()` e' l'unico punto che ogni strada attraversa. Il membro resta perche' `TearDown` lo
	// rilegge per sbindare solo il PROPRIO decisore.
	Result.DecisionSource = DecisionSource;

	// Precedenza: ERROR > FAIL > BLOCKED > PASS.
	//
	// L'ERROR viene per primo perche' e' l'unico che parla di CHI HA SBAGLIATO invece che di cosa e'
	// successo: se lo scenario e' scritto male, ogni assertion che segue misura uno stato che non doveva
	// esistere, e chiamarla FAIL manderebbe a cercare una regressione inesistente.
	//
	// Poi un FAIL vero batte il BLOCKED: un'assertion caduta PRIMA del punto di blocco riguarda codice che
	// esiste ed e' rotto — nasconderla dietro "non e' ancora pronto" sarebbe il modo piu' comodo di perdere
	// una regressione.
	if (!ErroredBy.IsEmpty())
	{
		Result.Outcome = ERTTestOutcome::Error;
		Result.ErrorMessage = ErroredBy;
	}
	else if (Result.FailedCount() > 0)
	{
		Result.Outcome = ERTTestOutcome::Fail;
	}
	else if (!BlockedBy.IsEmpty())
	{
		Result.Outcome = ERTTestOutcome::Blocked;
		Result.BlockedReason = BlockedBy;
	}
	else
	{
		Result.Outcome = ERTTestOutcome::Pass;
	}
	State = EState::Finished;

	UE_LOG(LogRT, Log, TEXT("[RT-Test] %s: %s (%d/%d assertion, %d turni)"),
		*Result.ScenarioId, *Result.OutcomeString(),
		Result.PassedCount(), Result.Assertions.Num(), Result.TurnsPlayed);
}
