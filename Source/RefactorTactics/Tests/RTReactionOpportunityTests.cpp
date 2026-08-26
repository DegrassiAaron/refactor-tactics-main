// Proprieta' pure di una `ReactionOpportunity`: identita' (CP 14.3) e geometria della zona armata (CP 14.6).
//
// 🔴 L'intestazione diceva «Il punto di questo file e' UNO», e ha smesso di essere vera quando CP 14.6 ci ha
// aggiunto un test di geometria — messo qui perche' la sua casa naturale, `RTOverwatchTriggerTests.cpp`,
// appartiene a un'altra track (`D-139`). Corretta invece di lasciata: chi cerca la geometria dell'Overwatch
// non guarderebbe in un file che si dichiara sugli id.
//
// Il primo punto resta: l'id di una opportunity si DERIVA dai sei campi che la individuano, e non
// si genera. Un GUID runtime sarebbe piu' comodo e romperebbe il replay in silenzio — due esecuzioni dello
// stesso scenario darebbero id diversi, quindi hash diversi, e la divergenza si presenterebbe come un difetto
// del resolver invece che come cio' che e': un identificatore che non e' una funzione del suo stato.

#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
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

	/**
	 * Nomi distinti per file: la unity build condivide la translation unit.
	 *
	 * ⚠️ **Duplicato di `MakeOverwatchMap` in `RTOverwatchTriggerTests.cpp`**, e non per scelta: quel file e'
	 * di un'altra track. Debito dichiarato — la soluzione giusta e' un header di fixture condiviso, non una
	 * terza copia, e va fatta quando i due file tornano alla stessa persona.
	 */
	URTHexMapAsset* MakeZoneFollowMap()
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius*/ 6);
		return M;
	}

	/**
	 * Un watcher costruito **esattamente come lo costruisce il resolver**: zona da `MakeSuppressiveZone`,
	 * origine nella cella passata, direzione ricavata dal facing DICHIARATO con `Neighbor`.
	 *
	 * Ricopiare la geometria a mano renderebbe il test verde su una zona che il gioco non produce piu'.
	 *
	 * ⚠️ I tre identificatori sono **distinti**, e non e' pedanteria: nel resolver `Zone.OwnerUnitId` e'
	 * l'indice in `Units`, `StableUnitId` e' l'id stabile dell'unita', e `ReactionInstanceId` e' l'indice in
	 * `ArmedOverwatches` — tre cose diverse. Passarli uguali renderebbe la fixture cieca proprio alla classe di
	 * difetto che `RTTurnManager.cpp` documenta accanto al ciclo: due Overwatch della stessa unita' che
	 * ricadono sullo stesso indice.
	 */
	FRTOverwatchWatcher MakeZoneFollowWatcher(const URTHexMapAsset* Map, const FRTCellId& From,
		ERTHexDirection Facing, int32 OwnerIdx, int32 StableId, int32 InstanceId)
	{
		FRTOverwatchWatcher W;
		W.Zone = URTOffensiveActionLibrary::MakeSuppressiveZone(Map, OwnerIdx, /*OwnerTeamId*/ 0, From,
			URTHexLibrary::Neighbor(From, Facing), /*RangeCells*/ 4, /*Damage*/ 1);
		W.OwnerCell = From;
		W.ReactionDefId = TEXT("Action.Overwatch");
		W.StableUnitId = StableId;
		W.ReactionInstanceId = InstanceId;
		W.TeamAwareness.Add(9, ERTAwareness::Detected);
		return W;
	}

	FRTSuppressionMover MakeZoneFollowMover(int32 UnitId, const TArray<FRTCellId>& Path)
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
 * CP 14.6 (**D-169**) — **la zona di un Overwatch armato e' funzione di *(cella corrente, facing dichiarato)*.**
 * Un watcher spinto **rilocalizza**: la linea controllata si sposta con lui e resta puntata dove aveva
 * dichiarato.
 *
 * ## Perche' esiste
 *
 * Il DoD di `#166` chiedeva che il movimento forzato **invalidasse** l'overwatch armato. Il codice fa il
 * contrario e lo motiva: `ResolveReactionBoundary` ricostruisce il watcher a **ogni micro-step** dalla cella
 * in `State.Pos`, *«un watcher costruito una volta nel Prep avrebbe la LOS di tre celle fa»*. `D-169`
 * conferma quel comportamento come regola — e una regola senza test e' prosa.
 *
 * ## Le due variabili, separate
 *
 * Le asserzioni variano **una cosa per volta**: prima la cella a facing fisso, poi il facing a cella fissa.
 * 🔴 La prima stesura di questo test variava solo la cella, e la code review ha osservato che
 * un'implementazione che ignorasse `Armed.Facing` mirando sempre a est le sarebbe passata sotto — mentre
 * `D-169` cita questo test come cio' che pinna *anche* il facing.
 *
 * ## ⚠️ Cosa NON copre
 *
 * È la meta' **pura**: data la coppia, la zona ne e' funzione. Che sia `ResolveReactionBoundary` a passare
 * `State.Pos[OwnerIdx]` e `Armed.Facing` vive in `RTTurnManager` e nel file di test che lo esercita, **di
 * altre track**. Finche' manca, questo pin non prova che il resolver usi la cella corrente — prova che, se la
 * usa, la zona la segue.
 *
 * 🔴 **E non pinna «la reaction resta armata»**, che pure e' parte di `D-169`: `bArmed` lo scrive il
 * chiamante e `BuildOverwatchTriggers` riceve i watcher per `const&`, quindi qualunque asserzione su quel
 * campo rileggerebbe cio' che il test stesso ha appena scritto. La prima stesura ne conteneva una, ed era
 * tautologica — esattamente il difetto che il commit dichiarava di evitare. Rimossa: quella meta' si verifica
 * solo dove `bArmed` viene deciso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArmedZoneFollowsCurrentCellTest,
	"RefactorTactics.Reactions.ArmedZoneFollowsCurrentCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArmedZoneFollowsCurrentCellTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeZoneFollowMap();
	if (!TestNotNull(TEXT("mappa di prova"), Map)) { return false; }

	const FRTCellId Origin(0, 0, 0);
	const FRTCellId Pushed(0, 2, 0);   // due celle piu' in la', come dopo una spinta

	// I tre corridoi: est da (0,0), est da (0,2), e nord-est da (0,0). Tre celle ciascuno, tutte dentro il
	// raggio 6 della mappa — la piu' lontana e' (3,-3), a distanza 3.
	const TArray<FRTCellId> EastFromOrigin = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0) };
	const TArray<FRTCellId> EastFromPushed = { FRTCellId(1, 2, 0), FRTCellId(2, 2, 0), FRTCellId(3, 2, 0) };
	const TArray<FRTCellId> NorthEastFromOrigin = { FRTCellId(1, -1, 0), FRTCellId(2, -2, 0), FRTCellId(3, -3, 0) };

	auto TriggerCount = [Map](const FRTOverwatchWatcher& W, const TArray<FRTCellId>& Path)
	{
		return URTReactionOpportunityLibrary::BuildOverwatchTriggers(
			Map, /*TurnNumber*/ 4, { W }, { MakeZoneFollowMover(9, Path) }).Num();
	};

	// --- 1. LA CELLA cambia, il facing no --------------------------------------------------------------
	const FRTOverwatchWatcher AtOrigin =
		MakeZoneFollowWatcher(Map, Origin, ERTHexDirection::E, /*OwnerIdx*/ 0, /*StableId*/ 41, /*InstanceId*/ 0);
	const FRTOverwatchWatcher AtPushed =
		MakeZoneFollowWatcher(Map, Pushed, ERTHexDirection::E, /*OwnerIdx*/ 1, /*StableId*/ 41, /*InstanceId*/ 0);

	// Il conteggio e' **esatto**: tre celle controllate percorse in tre micro-step danno tre opportunity, come
	// pinna `Overwatch.TriggersPerMicroStep` sullo stesso percorso. Un `> 0` lascerebbe verde una regressione
	// che si ferma alla prima — ed e' proprio il difetto che quel test fratello esiste per prendere.
	TestEqual(TEXT("premessa: dalla cella iniziale la zona copre il proprio corridoio, per intero"),
		TriggerCount(AtOrigin, EastFromOrigin), 3);

	TestEqual(TEXT("spinto, copre per intero il corridoio davanti alla NUOVA cella"),
		TriggerCount(AtPushed, EastFromPushed), 3);

	// L'asserzione che distingue «la zona segue» da «la zona si aggiunge».
	TestEqual(TEXT("e non copre piu' quello davanti alla cella di partenza"),
		TriggerCount(AtPushed, EastFromOrigin), 0);

	TestEqual(TEXT("simmetrica: il watcher fermo non copre il corridoio dell'altro"),
		TriggerCount(AtOrigin, EastFromPushed), 0);

	// --- 2. IL FACING cambia, la cella no --------------------------------------------------------------
	const FRTOverwatchWatcher AtOriginFacingNE =
		MakeZoneFollowWatcher(Map, Origin, ERTHexDirection::NE, /*OwnerIdx*/ 0, /*StableId*/ 41, /*InstanceId*/ 1);

	TestEqual(TEXT("stessa cella, facing NE: copre il corridoio nord-est per intero"),
		TriggerCount(AtOriginFacingNE, NorthEastFromOrigin), 3);

	// Senza questa, un'implementazione che ignorasse il facing e mirasse sempre a est passerebbe tutto il
	// resto del test: e' la meta' della regola che `D-169` chiama «col facing DICHIARATO all'armamento».
	TestEqual(TEXT("e NON copre piu' quello a est, che pure parte dalla stessa cella"),
		TriggerCount(AtOriginFacingNE, EastFromOrigin), 0);

	TestEqual(TEXT("simmetrica: il watcher a est non copre il corridoio nord-est"),
		TriggerCount(AtOrigin, NorthEastFromOrigin), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
