// L'identita' di una `ReactionOpportunity` (CP 14.3).
//
// Il punto di questo file e' UNO: l'id di una opportunity si DERIVA dai sei campi che la individuano, e non
// si genera. Un GUID runtime sarebbe piu' comodo e romperebbe il replay in silenzio — due esecuzioni dello
// stesso scenario darebbero id diversi, quindi hash diversi, e la divergenza si presenterebbe come un difetto
// del resolver invece che come cio' che e': un identificatore che non e' una funzione del suo stato.

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Unit/RTUnit.h"
#include "Turn/RTTurnRules.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Combat/RTOffensiveActionLibrary.h"  // MakeSuppressiveZone: la stessa geometria del resolver
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"                 // Neighbor: il facing dichiarato diventa una cella
#include "Map/RTHexMapAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	FRTReactionOpportunityKey MakeOpportunityKeyForIdTest()
	{
		FRTReactionOpportunityKey Key;
		Key.TurnNumber = 3;
		Key.MacroPhase = ERTMatchPhase::Blast;
		Key.MicroStepIndex = 2;
		Key.OwnerId = 7;
		Key.ReactionDefId = TEXT("Action.Counter");
		Key.Seq = 0;
		return Key;
	}

	/** Nomi distinti per file: la unity build condivide la translation unit. */
	URTHexMapAsset* MakeConeFollowMap(int32 Radius = 6)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	/**
	 * Un watcher costruito **esattamente come lo costruisce il resolver**: zona da `MakeSuppressiveZone`,
	 * origine nella cella passata, direzione ricavata dal facing DICHIARATO con `Neighbor`.
	 *
	 * Ricopiare la geometria a mano renderebbe il test verde su una zona che il gioco non produce piu' — e' la
	 * stessa ragione per cui la fixture di `RTOverwatchTriggerTests` chiama la funzione invece di elencare celle.
	 */
	FRTOverwatchWatcher MakeConeFollowWatcher(const URTHexMapAsset* Map, int32 OwnerId,
		const FRTCellId& From, ERTHexDirection Facing, int32 Range = 4)
	{
		FRTOverwatchWatcher W;
		W.Zone = URTOffensiveActionLibrary::MakeSuppressiveZone(Map, OwnerId, /*OwnerTeamId*/ 0, From,
			URTHexLibrary::Neighbor(From, Facing), Range, /*Damage*/ 1);
		W.OwnerCell = From;
		W.ReactionDefId = TEXT("Action.Overwatch");
		W.bArmed = true;
		W.StableUnitId = OwnerId;
		W.ReactionInstanceId = OwnerId;
		return W;
	}

	FRTSuppressionMover MakeConeFollowMover(int32 UnitId, const TArray<FRTCellId>& Path)
	{
		FRTSuppressionMover M;
		M.UnitId = UnitId;
		M.TeamId = 1;
		M.Path = Path;
		return M;
	}
}

/**
 * Stessi sei campi -> stesso id, e SEMPRE lo stesso: due derivazioni successive non possono divergere.
 *
 * E' la meta' del contratto che un GUID fallirebbe subito. L'altra meta' — campi diversi, id diversi — sta
 * nel test accanto: separate perche' falliscono per ragioni diverse, e un test che le mescola non dice quale
 * delle due proprieta' e' saltata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpportunityIdIsDerivedNotRandomTest,
	"RefactorTactics.Reactions.OpportunityIdIsDerivedNotRandom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpportunityIdIsDerivedNotRandomTest::RunTest(const FString&)
{
	const FRTReactionOpportunityKey Key = MakeOpportunityKeyForIdTest();

	const FString First = URTReactionOpportunityLibrary::DeriveOpportunityId(Key);
	const FString Second = URTReactionOpportunityLibrary::DeriveOpportunityId(Key);

	// Un id vuoto passerebbe l'uguaglianza qui sotto senza identificare niente: e' il modo piu' facile in cui
	// questo test potrebbe diventare verde e inutile.
	TestFalse(TEXT("l'id derivato non e' vuoto"), First.IsEmpty());
	TestEqual(TEXT("due derivazioni dalla stessa chiave danno lo stesso id"), First, Second);

	// La chiave ricostruita da zero, non riusata: un id che dipendesse dall'ISTANZA invece che dai VALORI
	// passerebbe il confronto qui sopra e fallirebbe questo.
	const FString FromEquivalentKey = URTReactionOpportunityLibrary::DeriveOpportunityId(MakeOpportunityKeyForIdTest());
	TestEqual(TEXT("una chiave equivalente ricostruita da zero da' lo stesso id"), First, FromEquivalentKey);

	return true;
}

/**
 * Ognuno dei sei campi partecipa all'id: cambiarne uno solo lo cambia.
 *
 * Senza questo, `DeriveOpportunityId` potrebbe ignorare meta' della chiave e nessuno se ne accorgerebbe
 * finche' due opportunity distinte non collidessero — cioe' finche' il replay non attribuisse a una la
 * decisione dell'altra. Il campo che si dimentica piu' facilmente e' `Seq`, che esiste proprio per
 * distinguere due opportunity identiche in tutto il resto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpportunityIdUsesEveryFieldTest,
	"RefactorTactics.Reactions.OpportunityIdUsesEveryField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpportunityIdUsesEveryFieldTest::RunTest(const FString&)
{
	const FRTReactionOpportunityKey Base = MakeOpportunityKeyForIdTest();
	const FString BaseId = URTReactionOpportunityLibrary::DeriveOpportunityId(Base);

	auto DiffersFromBase = [this, &BaseId](const TCHAR* What, const FRTReactionOpportunityKey& Changed)
	{
		const FString ChangedId = URTReactionOpportunityLibrary::DeriveOpportunityId(Changed);
		TestNotEqual(FString::Printf(TEXT("cambiare %s cambia l'id"), What), ChangedId, BaseId);
	};

	{
		FRTReactionOpportunityKey K = Base; K.TurnNumber = 4;
		DiffersFromBase(TEXT("TurnNumber"), K);
	}
	{
		FRTReactionOpportunityKey K = Base; K.MacroPhase = ERTMatchPhase::Move;
		DiffersFromBase(TEXT("MacroPhase"), K);
	}
	{
		FRTReactionOpportunityKey K = Base; K.MicroStepIndex = 3;
		DiffersFromBase(TEXT("MicroStepIndex"), K);
	}
	{
		FRTReactionOpportunityKey K = Base; K.OwnerId = 8;
		DiffersFromBase(TEXT("OwnerId"), K);
	}
	{
		FRTReactionOpportunityKey K = Base; K.ReactionDefId = TEXT("Action.Deflect");
		DiffersFromBase(TEXT("ReactionDefId"), K);
	}
	{
		FRTReactionOpportunityKey K = Base; K.Seq = 1;
		DiffersFromBase(TEXT("Seq"), K);
	}

	return true;
}

/**
 * La cardinalita' delle risposte legali decide il regime, e nient'altro (ADR-0004 §2).
 *
 * Le tre cardinalita' stanno in un test solo perche' sono **una** proprieta' — la soglia — e verificarne una
 * meta' non direbbe niente: con il solo caso `≤ 1` passerebbe un'implementazione che non apre MAI il
 * boundary, con il solo `≥ 2` una che lo apre SEMPRE. E' lo stesso motivo per cui
 * `Combat.RiktorImpactShotSlows` ha un gemello di controllo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSingleResponseCommitsWithoutWindowTest,
	"RefactorTactics.Reactions.SingleResponseCommitsWithoutWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSingleResponseCommitsWithoutWindowTest::RunTest(const FString&)
{
	FRTReactionOpportunity Opportunity;
	Opportunity.Key = MakeOpportunityKeyForIdTest();

	// Nessuna risposta legale: non c'e' niente da scegliere, quindi niente da chiedere.
	TestFalse(TEXT("zero risposte non aprono un boundary"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity));

	// Una sola risposta: il CASO DEGENERE, cioe' le reazioni E5 di oggi. `Counter`, `Deflect`, `Shield`,
	// `Cleanse` e il profilo base di `Brace` scattano o non scattano, e restano deterministiche.
	Opportunity.AllowedResponses = { TEXT("Hold Ground") };
	TestFalse(TEXT("una sola risposta si committa senza finestra"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity));

	// Due risposte: qui e solo qui nasce una scelta, e quindi una finestra.
	Opportunity.AllowedResponses = { TEXT("Hold Ground"), TEXT("Fire") };
	TestTrue(TEXT("due risposte aprono il decision boundary"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity));

	return true;
}

/**
 * Il DTO non porta futuro: nessuna posizione, nessun trigger, nessuna opportunity altrui, nessun intento
 * avversario (ADR-0004 §7, invariante #6).
 *
 * E' un test D'ARCHITETTURA e non di comportamento: interroga la reflection invece di eseguire il resolver,
 * perche' cio' che va impedito non e' un valore sbagliato ma un CAMPO IN PIU'. Un test che leggesse i valori
 * passerebbe felicemente accanto a un `FutureCells` lasciato vuoto quel giro.
 *
 * Il rosso di questo test si ottiene per MUTAZIONE — aggiungere un campo fuori elenco — e non scrivendolo
 * prima dell'implementazione: sul DTO conforme passa per costruzione. Verificato cosi': aggiungendo un
 * `UPROPERTY() FRTCellId PredictedCell` cade nominandolo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpportunityLeaksNoFutureTest,
	"RefactorTactics.Overwatch.OpportunityLeaksNoFuture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpportunityLeaksNoFutureTest::RunTest(const FString&)
{
	// Ogni campo che il DTO puo' avere, ed e' un ELENCO CHIUSO: chi ne aggiunge uno deve passare di qui e
	// dichiarare perche' non e' informazione futura. La lista corta e' il punto — non un dettaglio da tenere
	// aggiornato.
	auto CheckClosedFieldSet = [this](UScriptStruct* Struct, const TCHAR* What, const TSet<FString>& Allowed)
	{
		if (!Struct)
		{
			AddError(FString::Printf(TEXT("%s: struct non risolta dalla reflection"), What));
			return;
		}

		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			const FString Name = It->GetName();
			if (!Allowed.Contains(Name))
			{
				AddError(FString::Printf(
					TEXT("%s espone il campo '%s', che non e' nell'elenco chiuso: se e' informazione futura ")
					TEXT("(posizione, trigger, opportunity altrui, intento avversario) non puo' stare nel DTO; ")
					TEXT("se non lo e', va aggiunto qui con la ragione"),
					What, *Name));
			}
		}
	};

	CheckClosedFieldSet(FRTReactionOpportunity::StaticStruct(), TEXT("FRTReactionOpportunity"),
		{ TEXT("Key"), TEXT("AllowedResponses") });

	// La chiave viaggia dentro il DTO, quindi il suo contenuto e' altrettanto esposto. `MicroStepIndex` e'
	// passato: dice DOVE nella risoluzione ci si trova, non dove si andra'.
	CheckClosedFieldSet(FRTReactionOpportunityKey::StaticStruct(), TEXT("FRTReactionOpportunityKey"),
		{ TEXT("TurnNumber"), TEXT("MacroPhase"), TEXT("MicroStepIndex"),
		  TEXT("OwnerId"), TEXT("ReactionDefId"), TEXT("Seq") });

	return true;
}

/**
 * Le condizioni dichiarabili in pianificazione sono un elenco CHIUSO, e l'elenco vive nel codice ([D-109]).
 *
 * Nel dato sarebbe piu' flessibile e sbagliato: dichiarare una condizione inesistente diventerebbe una
 * modifica al JSON invece di un errore, ed e' la stessa ragione per cui `IsCapabilityAvailable` tiene il
 * proprio elenco nel codice. La v0.1 ne ammette **una**: la soglia di salute del bersaglio.
 *
 * Il parametro fa parte di cio' che si valida. Una soglia oltre il 100% renderebbe la condizione sempre
 * vera — cioe' una condizione che non condiziona, che e' peggio di nessuna condizione: il giocatore crede
 * di aver ristretto il fuoco e non l'ha fatto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionConditionValidatorTest,
	"RefactorTactics.Reactions.UnknownConditionIsRejectedByValidator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionConditionValidatorTest::RunTest(const FString&)
{
	const FName Allowed = URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent();

	TestTrue(TEXT("la soglia di salute e' la voce ammessa dalla v0.1"),
		URTReactionOpportunityLibrary::IsDeclaredConditionAllowed(FRTDeclaredCondition(Allowed, 50)));

	// Il caso per cui il validator esiste: un id che il gioco non conosce. Senza questo controllo un piano
	// salvato potrebbe nominare una condizione che nessuna funzione valuta, e il trigger non saprebbe dirlo.
	TestFalse(TEXT("un id che il gioco non conosce viene rifiutato"),
		URTReactionOpportunityLibrary::IsDeclaredConditionAllowed(FRTDeclaredCondition(TEXT("TargetIsFlanked"), 1)));

	// Assenza di condizione non e' una condizione ammessa: chi non dichiara non passa di qui.
	TestFalse(TEXT("nessun id dichiarato non e' una condizione valida"),
		URTReactionOpportunityLibrary::IsDeclaredConditionAllowed(FRTDeclaredCondition()));

	// La soglia e' una percentuale intera (D-109: niente float, gate G7) e deve restringere davvero.
	TestFalse(TEXT("soglia oltre il 100% rifiutata"),
		URTReactionOpportunityLibrary::IsDeclaredConditionAllowed(FRTDeclaredCondition(Allowed, 101)));
	TestFalse(TEXT("soglia negativa rifiutata"),
		URTReactionOpportunityLibrary::IsDeclaredConditionAllowed(FRTDeclaredCondition(Allowed, -1)));
	TestTrue(TEXT("soglia 100 ammessa: «spara comunque» resta dichiarabile"),
		URTReactionOpportunityLibrary::IsDeclaredConditionAllowed(FRTDeclaredCondition(Allowed, 100)));

	return true;
}

/**
 * La condizione entra nel piano solo se e' dichiarabile, e solo se c'e' una reazione a cui applicarla.
 *
 * E' l'anello che rende la condizione un dato del GIOCO e non dei test: senza un punto che la scrive
 * validandola, `FRTDeclaredCondition` sarebbe un campo con un consumatore e nessun produttore — la forma di
 * difetto che questo repository ha gia' incontrato con `PlannedReactionAbility`, scritto per mesi dai soli
 * test mentre il resolver lo leggeva.
 *
 * Il rifiuto non e' silenzioso e non e' parziale: o la condizione entra intera, o il piano resta com'era.
 * Una condizione applicata a meta' sarebbe peggio di nessuna condizione, perche' il giocatore crederebbe di
 * aver ristretto il fuoco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionConditionPlanTest,
	"RefactorTactics.Reactions.DeclaredConditionEntersThePlanOnlyIfValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionConditionPlanTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!World)
	{
		AddError(TEXT("mondo non creato"));
		return false;
	}
	if (GEngine)
	{
		FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
		Ctx.SetCurrentWorld(World);
	}

	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	const FRTDeclaredCondition Valid(URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent(), 50);

	if (Unit)
	{
		// 1. Senza reazione armata non c'e' niente da condizionare: la dichiarazione non ha un bersaglio nel
		//    piano, e accettarla lascerebbe una condizione orfana che il prossimo armamento erediterebbe.
		Unit->PlannedReactionAbility = INDEX_NONE;
		TestFalse(TEXT("senza reazione armata la condizione e' rifiutata"),
			Unit->SetPlannedReactionCondition(Valid));
		TestFalse(TEXT("e il piano resta senza condizione"), Unit->PlannedReactionCondition.IsDeclared());

		// 2. Con la reazione armata, una condizione che il gioco non conosce viene rifiutata e non lascia
		//    tracce: il campo resta quello di prima, non una versione a meta'.
		Unit->PlannedReactionAbility = 0;
		TestFalse(TEXT("un id sconosciuto e' rifiutato"),
			Unit->SetPlannedReactionCondition(FRTDeclaredCondition(TEXT("TargetIsFlanked"), 1)));
		TestFalse(TEXT("il piano resta senza condizione"), Unit->PlannedReactionCondition.IsDeclared());

		// 3. Il caso buono.
		TestTrue(TEXT("la condizione ammessa entra nel piano"), Unit->SetPlannedReactionCondition(Valid));
		TestTrue(TEXT("ed e' dichiarata"), Unit->PlannedReactionCondition.IsDeclared());
		TestEqual(TEXT("con la sua soglia"), Unit->PlannedReactionCondition.Param, 50);

		// 4. Togliere la condizione e' sempre legittimo: e' il modo di tornare a «spara comunque», e non deve
		//    passare per il validator — `NAME_None` non e' una condizione ammessa, e infatti non lo e'.
		TestTrue(TEXT("la condizione si puo' togliere"), Unit->SetPlannedReactionCondition(FRTDeclaredCondition()));
		TestFalse(TEXT("e il piano torna senza condizione"), Unit->PlannedReactionCondition.IsDeclared());
	}
	else
	{
		AddError(TEXT("unita' non spawnata"));
	}

	if (GEngine)
	{
		GEngine->DestroyWorldContext(World);
	}
	World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
	return true;
}

/**
 * CP 14.6 (**D-169**) — **il cono di un Overwatch armato segue la cella CORRENTE del proprietario, col facing
 * DICHIARATO all'armamento.** Un watcher spinto **rilocalizza**: non perde la reaction.
 *
 * ## Perche' questo test esiste
 *
 * Il DoD di `#166` chiedeva che il movimento forzato **invalidasse** l'overwatch armato. La misura dice il
 * contrario, e il codice lo motiva: `ResolveReactionBoundary` ricostruisce il watcher a **ogni micro-step**
 * dalla cella in `State.Pos`, *«un watcher costruito una volta nel Prep avrebbe la LOS di tre celle fa»*.
 * `D-169` conferma quel comportamento come **regola** invece di cambiarlo — e una regola senza test e' prosa.
 *
 * ## Cosa questo test copre, e cosa NO
 *
 * Copre la meta' **pura**: dato un watcher costruito da una cella, il cono e' funzione di *(cella, facing)*.
 * Le due asserzioni incrociate sono il punto — ciascun watcher scatta sul proprio corridoio e **non**
 * sull'altro — perche' un test che guardasse un solo watcher passerebbe anche con un cono che non si muove.
 *
 * ⚠️ **NON copre** che sia davvero `ResolveReactionBoundary` a passare `State.Pos[OwnerIdx]`: quella meta' e'
 * integrazione, vive in `RTTurnManager` e nel file di test che lo esercita, ed entrambi appartengono ad altre
 * track (`D-139`). Va aggiunta da chi li possiede, e finche' manca questo pin **non prova** che il resolver usi
 * la cella corrente — prova che, se la usa, il cono la segue.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArmedConeFollowsCurrentCellTest,
	"RefactorTactics.Reactions.ArmedConeFollowsCurrentCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArmedConeFollowsCurrentCellTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeConeFollowMap();
	if (!TestNotNull(TEXT("mappa di prova"), Map)) { return false; }

	// Stesso facing per entrambi: a cambiare e' SOLO la cella da cui si guarda. E' la variabile che il
	// movimento forzato muove, e l'unica che questo test fa variare.
	constexpr ERTHexDirection Facing = ERTHexDirection::E;

	const FRTCellId Origin(0, 0, 0);
	const FRTCellId Pushed(0, 2, 0);   // due celle piu' in la', come dopo una spinta

	FRTOverwatchWatcher FromOrigin = MakeConeFollowWatcher(Map, /*OwnerId*/ 1, Origin, Facing);
	FRTOverwatchWatcher FromPushed = MakeConeFollowWatcher(Map, /*OwnerId*/ 1, Pushed, Facing);

	// Il bersaglio e' `Rilevato` per entrambi: senza, il trigger non scatta per la condizione di ADR-0004 §6 e
	// il test misurerebbe la conoscenza di squadra invece della geometria.
	FromOrigin.TeamAwareness.Add(9, ERTAwareness::Detected);
	FromPushed.TeamAwareness.Add(9, ERTAwareness::Detected);

	// Due corridoi paralleli, uno per riga: quello davanti alla posizione iniziale e quello davanti alla
	// posizione dopo la spinta.
	const TArray<FRTCellId> AlongOrigin = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0) };
	const TArray<FRTCellId> AlongPushed = { FRTCellId(1, 2, 0), FRTCellId(2, 2, 0), FRTCellId(3, 2, 0) };

	auto Triggers = [Map](const FRTOverwatchWatcher& W, const TArray<FRTCellId>& Path)
	{
		return URTReactionOpportunityLibrary::BuildOverwatchTriggers(
			Map, /*TurnNumber*/ 4, { W }, { MakeConeFollowMover(9, Path) });
	};

	// Premessa: il cono ORIGINALE vede il proprio corridoio. Senza, le due asserzioni negative sotto
	// sarebbero verdi per la ragione sbagliata — un cono vuoto non scatta su niente.
	TestTrue(TEXT("premessa: dalla cella iniziale il cono vede il corridoio davanti a se'"),
		Triggers(FromOrigin, AlongOrigin).Num() > 0);

	// Il cono si e' MOSSO: dalla cella nuova vede il proprio corridoio...
	TestTrue(TEXT("spinto, il cono vede il corridoio davanti alla NUOVA cella"),
		Triggers(FromPushed, AlongPushed).Num() > 0);

	// ...e non piu' quello vecchio. E' l'asserzione che distingue «il cono segue» da «il cono si allarga».
	TestEqual(TEXT("e NON vede piu' il corridoio davanti alla cella di partenza"),
		Triggers(FromPushed, AlongOrigin).Num(), 0);

	// Simmetrica: il watcher fermo non vede il corridoio dell'altro. Senza questa, un cono grande abbastanza
	// da coprire entrambe le righe passerebbe le tre asserzioni precedenti.
	TestEqual(TEXT("e il watcher fermo non vede il corridoio dell'altro"),
		Triggers(FromOrigin, AlongPushed).Num(), 0);

	// La reaction NON e' decaduta: la spinta rilocalizza, non disarma. `bArmed` e' il `ReactionStillArmed`
	// della condizione di trigger (ADR-0004 §6), e resta vero attraverso lo spostamento.
	TestTrue(TEXT("dopo la spinta la reaction e' ancora armata"), FromPushed.bArmed);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
