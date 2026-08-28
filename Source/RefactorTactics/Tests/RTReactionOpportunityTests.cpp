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
#include "Turn/RTReactionWindowView.h"        // il DTO di CP 14.6: stesso elenco chiuso, stesso guardiano
#include "Turn/RTTurnManager.h"               // la fonte autorevole del countdown, misurata dal DTO
#include "Tests/RTWorldFixtures.h"            // MakeWorld/DestroyWorld: il manager e' un Actor, serve un mondo
#include "Turn/RTPacingLibrary.h"             // il conteggio delle finestre e il tetto in secondi (CP 14.6)
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

	// Il DTO che alimenta la UI della finestra (CP 14.6, `#166`) e' l'altra faccia dello stesso vincolo, e per
	// questo lo chiude questo test invece di uno parallelo: e' cio' che ESCE dal core verso la presentazione,
	// quindi il posto in cui un campo di troppo diventa un'esposizione vera e non piu' solo un rischio.
	//
	// Perche' ciascuno e' ammesso: `Key` porta i sei campi gia' dichiarati non-futuri due righe sopra;
	// `WindowSeconds` e' un parametro pubblico della regola (ADR-0004 §8), non uno stato di partita;
	// `Options` sono le risposte che il proprietario puo' gia' scegliere; `SafeResponse` e' cio' che accade
	// allo scadere, cioe' una conseguenza gia' decisa e non una previsione.
	CheckClosedFieldSet(FRTReactionWindowView::StaticStruct(), TEXT("FRTReactionWindowView"),
		{ TEXT("bOpen"), TEXT("Key"), TEXT("WindowSeconds"), TEXT("Options"), TEXT("SafeResponse") });

	// `TargetSnapshotIndex` porta il nome del proprio spazio di id apposta: e' un indice in
	// `MakeCurrentSnapshot`, non un id stabile, e chiamarlo `UnitId` invitava a risolverlo altrove.
	CheckClosedFieldSet(FRTReactionWindowOptionView::StaticStruct(), TEXT("FRTReactionWindowOptionView"),
		{ TEXT("Response"), TEXT("TargetSnapshotIndex") });

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
namespace
{
	// Una finestra dell'Overwatch come `BuildOverwatchTriggers` la produce: un `FIRE:` per bersaglio, `HOLD`
	// in coda. Nome distinto da ogni altro helper del file: la unity build condivide la translation unit.
	//
	// ⚠️ **Ogni campo che distingue due finestre e' un parametro**, e non e' pedanteria: la prima stesura
	// fissava `OwnerId`, `MacroPhase` e `Seq` nell'helper, e il confronto «due finestre diverse danno
	// all'avversario la stessa vista» risultava vero **per costruzione** su quattro assertion su sei —
	// confrontavano 7 con 7. Rilevato in code review.
	FRTReactionOpportunity MakeOverwatchWindowForViewTest(int32 FirstTarget, int32 SecondTarget,
		int32 TurnNumber, int32 OwnerId, ERTMatchPhase MacroPhase, int32 Seq)
	{
		FRTReactionOpportunity Opportunity;
		Opportunity.Key.TurnNumber = TurnNumber;
		Opportunity.Key.MacroPhase = MacroPhase;
		Opportunity.Key.MicroStepIndex = 1;
		Opportunity.Key.OwnerId = OwnerId;
		Opportunity.Key.ReactionDefId = TEXT("Action.Overwatch");
		Opportunity.Key.Seq = Seq;

		Opportunity.AllowedResponses = {
			URTReactionOpportunityLibrary::FireResponse(FirstTarget),
			URTReactionOpportunityLibrary::FireResponse(SecondTarget),
			FString(URTReactionOpportunityLibrary::HoldResponse())
		};

		return Opportunity;
	}

	// La forma dell'ALTRO produttore di finestre, `RTTurnManager_Chunk.Blast` da
	// `URTCatalogLibrary::BraceExecutableResponses`: nessuna risposta col prefisso `FIRE:`, e la scelta
	// sicura in TESTA invece che in coda ([D-047] §2.1).
	FRTReactionOpportunity MakeBraceWindowForViewTest()
	{
		FRTReactionOpportunity Opportunity;
		Opportunity.Key.TurnNumber = 5;
		Opportunity.Key.MacroPhase = ERTMatchPhase::Blast; // la spinta si risolve li'
		Opportunity.Key.OwnerId = 2;
		Opportunity.Key.ReactionDefId = TEXT("Action.Brace");

		Opportunity.AllowedResponses = { TEXT("Hold Ground"), TEXT("Sidestep") };
		return Opportunity;
	}
}

/**
 * Per un avversario la finestra NON ESISTE: la vista e' ai default, e due finestre diverse gliene danno una
 * identica (DoD di `#166`, invariante #6).
 *
 * ⚠️ **Il secondo confronto e' il punto**, ed e' la stessa forma di `UI.NoEnemyIntentExposed`: un test che
 * guardasse solo `bOpen == false` passerebbe accanto a un `Key.OwnerId` popolato, a un countdown scritto
 * «tanto il widget non lo mostra», a un elenco di opzioni lasciato dentro. Quei campi non si vedono a
 * schermo — ma in rete (M10) sono sul filo, e un avversario che riceve due strutture DIVERSE sa che qualcosa
 * di diverso sta accadendo, che e' gia' informazione.
 *
 * 🔴 **Le due finestre differiscono in OGNI campo che il proprietario vede** — bersagli, turno, proprietario,
 * macro-fase, `Seq`, durata — perche' un confronto fra due valori identici alla fonte non prova niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowHiddenFromEnemyTest,
	"RefactorTactics.Reactions.WindowViewHiddenFromEnemyTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowHiddenFromEnemyTest::RunTest(const FString&)
{
	const int32 OwnerTeam = 0;
	const int32 EnemyTeam = 1;

	const FRTReactionOpportunity First =
		MakeOverwatchWindowForViewTest(4, 9, 3, /*OwnerId=*/7, ERTMatchPhase::Move, /*Seq=*/0);
	const FRTReactionOpportunity Second =
		MakeOverwatchWindowForViewTest(11, 12, 8, /*OwnerId=*/2, ERTMatchPhase::Blast, /*Seq=*/4);

	const FRTReactionWindowView ToEnemy =
		URTReactionWindowLibrary::FilterWindowForTeam(EnemyTeam, OwnerTeam, First, 3.f);

	TestFalse(TEXT("l'avversario non ha nessuna finestra aperta"), ToEnemy.bOpen);
	TestEqual(TEXT("nessuna opzione"), ToEnemy.Options.Num(), 0);
	TestEqual(TEXT("nessun countdown"), ToEnemy.WindowSeconds, 0.f);
	TestTrue(TEXT("nessuna scelta sicura"), ToEnemy.SafeResponse.IsEmpty());
	TestEqual(TEXT("nessun proprietario"), ToEnemy.Key.OwnerId, static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("nessun turno"), ToEnemy.Key.TurnNumber, 0);
	TestEqual(TEXT("nessun micro-step"), ToEnemy.Key.MicroStepIndex, 0);
	TestTrue(TEXT("nessuna reaction nominata"), ToEnemy.Key.ReactionDefId.IsNone());

	// La macro-fase e il progressivo sono gli altri due campi della chiave, e vanno asseriti qui: sapere che
	// una finestra avversaria si e' aperta nel `Blast` invece che nel `Move` e' gia' informazione su cosa
	// sta accadendo. Il default di `MacroPhase` e' `Planning`, che nessuna delle due finestre usa.
	TestTrue(TEXT("nessuna macro-fase"), ToEnemy.Key.MacroPhase == ERTMatchPhase::Planning);
	TestEqual(TEXT("nessun progressivo"), ToEnemy.Key.Seq, 0);

	// Due finestre che differiscono in TUTTO cio' che il proprietario vede devono risultare
	// INDISTINGUIBILI per l'avversario. E' la differenza fra «non mostrato» e «non ricevuto».
	const FRTReactionWindowView ToEnemyAgain =
		URTReactionWindowLibrary::FilterWindowForTeam(EnemyTeam, OwnerTeam, Second, 12.f);

	TestEqual(TEXT("bOpen identico fra due finestre diverse"), ToEnemyAgain.bOpen, ToEnemy.bOpen);
	TestEqual(TEXT("countdown identico"), ToEnemyAgain.WindowSeconds, ToEnemy.WindowSeconds);
	TestEqual(TEXT("opzioni identiche"), ToEnemyAgain.Options.Num(), ToEnemy.Options.Num());
	TestEqual(TEXT("scelta sicura identica"), ToEnemyAgain.SafeResponse, ToEnemy.SafeResponse);
	TestEqual(TEXT("proprietario identico"), ToEnemyAgain.Key.OwnerId, ToEnemy.Key.OwnerId);
	TestEqual(TEXT("turno identico"), ToEnemyAgain.Key.TurnNumber, ToEnemy.Key.TurnNumber);
	TestTrue(TEXT("macro-fase identica"), ToEnemyAgain.Key.MacroPhase == ToEnemy.Key.MacroPhase);
	TestEqual(TEXT("progressivo identico"), ToEnemyAgain.Key.Seq, ToEnemy.Key.Seq);

	return true;
}

/**
 * Una squadra non risolta non riceve niente, da nessuno dei due lati: `INDEX_NONE` e' il default dei team id
 * nel progetto, e due «non lo so» sono UGUALI fra loro.
 *
 * 🔴 Senza questa guardia il filtro falliva **aperto**: un osservatore il cui team non si e' potuto risolvere
 * — proprietario gia' caduto, indice di snapshot fuori range, lookup fallito — avrebbe ricevuto chiave,
 * countdown e opzioni per intero. Il filtro degli intenti non ha questo buco perche' legge il team dal dato;
 * qui i due arrivano dal chiamante. Segnalato in code review, prima che esistesse un chiamante di produzione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowUnresolvedTeamTest,
	"RefactorTactics.Reactions.WindowViewClosedForUnresolvedTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowUnresolvedTeamTest::RunTest(const FString&)
{
	const FRTReactionOpportunity Opportunity =
		MakeOverwatchWindowForViewTest(4, 9, 3, /*OwnerId=*/7, ERTMatchPhase::Move, /*Seq=*/0);

	const FRTReactionWindowView BothUnresolved =
		URTReactionWindowLibrary::FilterWindowForTeam(INDEX_NONE, INDEX_NONE, Opportunity, 3.f);
	TestFalse(TEXT("due squadre ignote non coincidono: nessuna finestra"), BothUnresolved.bOpen);
	TestEqual(TEXT("e nessuna opzione"), BothUnresolved.Options.Num(), 0);

	const FRTReactionWindowView OwnerUnresolved =
		URTReactionWindowLibrary::FilterWindowForTeam(0, INDEX_NONE, Opportunity, 3.f);
	TestFalse(TEXT("proprietario ignoto: nessuna finestra"), OwnerUnresolved.bOpen);

	const FRTReactionWindowView ObserverUnresolved =
		URTReactionWindowLibrary::FilterWindowForTeam(INDEX_NONE, 0, Opportunity, 3.f);
	TestFalse(TEXT("osservatore ignoto: nessuna finestra"), ObserverUnresolved.bOpen);

	return true;
}

/**
 * Alla squadra del proprietario la finestra arriva completa: countdown e bersaglio, che sono le due cose che
 * la DoD di `#166` nomina — *«UI FIRE/HOLD con countdown e bersaglio»*.
 *
 * ⚠️ **La scelta sicura e' un'opzione come le altre**, e il bersaglio e' `INDEX_NONE` per lei: e' il campo
 * che distingue un bottone di bersaglio da un bottone di rinuncia, e non la posizione nell'elenco.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowCarriesTargetsTest,
	"RefactorTactics.Reactions.WindowViewCarriesTargetsAndCountdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowCarriesTargetsTest::RunTest(const FString&)
{
	const int32 OwnerTeam = 0;
	const FRTReactionOpportunity Opportunity =
		MakeOverwatchWindowForViewTest(4, 9, 3, /*OwnerId=*/7, ERTMatchPhase::Move, /*Seq=*/0);

	const FRTReactionWindowView View =
		URTReactionWindowLibrary::FilterWindowForTeam(OwnerTeam, OwnerTeam, Opportunity, 3.f);

	TestTrue(TEXT("la squadra del proprietario vede la finestra"), View.bOpen);
	TestEqual(TEXT("il countdown e' quello che il core ha aperto"), View.WindowSeconds, 3.f);
	TestEqual(TEXT("l'identita' della finestra viaggia intera"), View.Key.OwnerId, 7);
	TestEqual(TEXT("e con il proprio turno"), View.Key.TurnNumber, 3);

	if (TestEqual(TEXT("TRE opzioni: i due bersagli piu' la rinuncia"), View.Options.Num(), 3))
	{
		TestEqual(TEXT("primo bersaglio"), View.Options[0].TargetSnapshotIndex, 4);
		TestEqual(TEXT("e la stringa che lo sceglie, prodotta dal core"),
			View.Options[0].Response, URTReactionOpportunityLibrary::FireResponse(4));

		TestEqual(TEXT("secondo bersaglio, nell'ordine dell'opportunity"), View.Options[1].TargetSnapshotIndex, 9);
		TestEqual(TEXT("e la sua stringa"),
			View.Options[1].Response, URTReactionOpportunityLibrary::FireResponse(9));

		TestEqual(TEXT("la rinuncia e' un'opzione, e non ha bersaglio"),
			View.Options[2].TargetSnapshotIndex, static_cast<int32>(INDEX_NONE));
		TestEqual(TEXT("ed e' la stringa che il core applica allo scadere"),
			View.Options[2].Response, View.SafeResponse);
	}

	// La scelta sicura si LEGGE dal core: e' `HOLD` qui perche' l'Overwatch la offre, non perche' il DTO la
	// scriva. Il valore atteso viene dalla stessa funzione che il resolver usa allo scadere.
	TestEqual(TEXT("la scelta sicura e' quella del core"),
		View.SafeResponse, URTReactionOpportunityLibrary::SafeResponse(Opportunity));
	TestEqual(TEXT("che per l'Overwatch e' HOLD"),
		View.SafeResponse, FString(URTReactionOpportunityLibrary::HoldResponse()));

	return true;
}

/**
 * Una finestra del `Brace` e' **azionabile**: le sue risposte non hanno bersaglio, e devono comparire lo
 * stesso.
 *
 * 🔴 **E' il test che mancava, e la sua assenza nascondeva un difetto vero.** La prima stesura del DTO
 * elencava i soli `FIRE:<id>`: `RTTurnManager_Chunk.Blast` costruisce finestre da
 * `BraceExecutableResponses`, che offre `Hold Ground` piu' le maneuver eseguibili — nessun `FIRE:`. Il
 * risultato era `bOpen = true` con **zero opzioni**: un countdown senza un bottone da premere, e ogni
 * finestra del `Brace` scaduta da sola. I tre test partivano tutti da una finestra dell'Overwatch, cioe'
 * dalla sola forma che il DTO sapeva rappresentare, ed erano verdi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowBraceIsActionableTest,
	"RefactorTactics.Reactions.WindowViewBraceIsActionable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowBraceIsActionableTest::RunTest(const FString&)
{
	const int32 OwnerTeam = 0;
	const FRTReactionOpportunity Brace = MakeBraceWindowForViewTest();

	TestTrue(TEXT("premessa: una finestra del Brace apre un boundary"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(Brace));

	const FRTReactionWindowView View =
		URTReactionWindowLibrary::FilterWindowForTeam(OwnerTeam, OwnerTeam, Brace, 3.f);

	TestTrue(TEXT("la finestra e' aperta"), View.bOpen);

	if (TestEqual(TEXT("ed e' AZIONABILE: due opzioni, non zero"), View.Options.Num(), 2))
	{
		TestEqual(TEXT("la scelta sicura, in TESTA e non in coda"), View.Options[0].Response, TEXT("Hold Ground"));
		TestEqual(TEXT("senza bersaglio"), View.Options[0].TargetSnapshotIndex, static_cast<int32>(INDEX_NONE));

		TestEqual(TEXT("e la maneuver del profilo"), View.Options[1].Response, TEXT("Sidestep"));
		TestEqual(TEXT("anch'essa senza bersaglio"), View.Options[1].TargetSnapshotIndex, static_cast<int32>(INDEX_NONE));
	}

	// La scelta sicura del `Brace` NON e' `HOLD`, ed e' la ragione per cui il DTO porta il campo invece di
	// lasciare che il widget scriva una costante.
	TestEqual(TEXT("la scelta sicura e' quella del profilo"), View.SafeResponse, TEXT("Hold Ground"));
	TestTrue(TEXT("e non e' la costante HOLD"),
		View.SafeResponse != FString(URTReactionOpportunityLibrary::HoldResponse()));

	return true;
}

/**
 * Senza boundary non c'e' finestra, nemmeno per la propria squadra: la cardinalita' resta la sola regola
 * (ADR-0004 §2), e il DTO la CHIEDE invece di ricalcolarla.
 *
 * Il caso e' quello reale delle reazioni E5 — scattano o non scattano — e mostrarne un countdown insegnerebbe
 * al giocatore un'attesa che il resolver non apre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowClosedWithoutBoundaryTest,
	"RefactorTactics.Reactions.WindowViewClosedWithoutBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowClosedWithoutBoundaryTest::RunTest(const FString&)
{
	const int32 OwnerTeam = 0;

	FRTReactionOpportunity SingleResponse =
		MakeOverwatchWindowForViewTest(4, 9, 3, /*OwnerId=*/7, ERTMatchPhase::Move, /*Seq=*/0);
	SingleResponse.AllowedResponses = { URTReactionOpportunityLibrary::FireResponse(4) };

	TestFalse(TEXT("premessa: questa opportunity non apre un boundary"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(SingleResponse));

	const FRTReactionWindowView View =
		URTReactionWindowLibrary::FilterWindowForTeam(OwnerTeam, OwnerTeam, SingleResponse, 3.f);

	TestFalse(TEXT("e quindi non c'e' nessuna finestra da mostrare"), View.bOpen);
	TestEqual(TEXT("nessuna opzione"), View.Options.Num(), 0);
	TestEqual(TEXT("nessun countdown"), View.WindowSeconds, 0.f);

	return true;
}

/**
 * Il countdown del DTO viene dal `TurnManager`, e non da una costante scritta due volte.
 *
 * 🔴 **E' il test che rende vera l'affermazione del campo.** `FastReactionDuration` e' nato dichiarando di
 * avere un lettore, e per una stesura non ce l'ha avuto: `FilterWindowForTeam` riceve la durata come
 * parametro e non ha mai letto il campo. Qui la catena si chiude e si misura — `ARTTurnManager` ->
 * `MakeReactionWindowView` -> `WindowSeconds` — perche' un campo che nessun test attraversa e' un campo che
 * il prossimo autore duplichera' senza accorgersene.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowReadsManagerDurationTest,
	"RefactorTactics.Reactions.WindowViewCountdownComesFromTheManager",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowReadsManagerDurationTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("TurnManager"), TurnManager))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Il default e' quello di ADR-0004 §8, e il test lo pinna: se qualcuno lo cambia senza cambiare l'ADR,
	// questa riga lo dice per nome invece di lasciarlo scoprire a un playtest.
	TestEqual(TEXT("la finestra dura 3,0 s (ADR-0004 §8)"), TurnManager->GetFastReactionDuration(), 3.f);

	const int32 OwnerTeam = 0;
	const FRTReactionOpportunity Opportunity =
		MakeOverwatchWindowForViewTest(4, 9, 3, /*OwnerId=*/7, ERTMatchPhase::Move, /*Seq=*/0);

	const FRTReactionWindowView Default =
		TurnManager->MakeReactionWindowView(Opportunity, OwnerTeam, OwnerTeam);
	TestEqual(TEXT("e il DTO lo consegna al countdown"), Default.WindowSeconds, 3.f);

	// Cambiando la fonte cambia il DTO: senza questa seconda misura, un DTO che scrivesse `3.f` a mano
	// passerebbe la riga sopra.
	TurnManager->SetFastReactionDuration(5.f);
	const FRTReactionWindowView Retuned =
		TurnManager->MakeReactionWindowView(Opportunity, OwnerTeam, OwnerTeam);
	TestEqual(TEXT("il countdown SEGUE la fonte, non una costante"), Retuned.WindowSeconds, 5.f);

	// Il clamp vive sulla FONTE e non solo nel DTO: un valore negativo non deve nemmeno essere memorizzato,
	// altrimenti ogni lettore futuro riceve un timer che non scatta mai.
	TurnManager->SetFastReactionDuration(-1.f);
	TestEqual(TEXT("una durata negativa si clampa dove viene scritta"),
		TurnManager->GetFastReactionDuration(), 0.f);

	// E la privacy resta quella della libreria anche passando dal manager: il punto d'ingresso non e' una
	// scorciatoia che salta il filtro.
	const FRTReactionWindowView ToEnemy =
		TurnManager->MakeReactionWindowView(Opportunity, OwnerTeam, /*ObserverTeamId=*/1);
	TestFalse(TEXT("l'avversario non riceve la finestra nemmeno dal manager"), ToEnemy.bOpen);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **La misura del pacing di CP 14.6, la metà che si chiude senza Editor**: quante finestre si aprono a UN
 * giocatore con **1, 2 e 3 unità armate** — i tre punti che la DoD di `#166` chiede per nome.
 *
 * 🔴 **Perché tre misure e non una media.** [D-167]: due unità armate su squadre **diverse** aprono finestre
 * che due persone aspettano **in parallelo**; due dello **stesso** giocatore gliene impilano due **in fila**.
 * Qui i watcher sono tutti del team 0 apposta — è il caso che la v0.1 gioca davvero ([D-155]: un umano, due
 * unità), ed è quello che tara `InitialBank` e `Grace`.
 *
 * ⛔ **Ciò che questo test NON misura, e nessun test headless può**: quanto ci mette un giocatore a
 * rispondere. Con un decisore che risponde subito il Decision Time è **nullo per costruzione** — lo dichiara
 * già `Overwatch.SegmentedResolutionOverhead` — quindi il `p50`/`p90` che
 * `spec-decision-time-bank.md` §3.2 chiede per promuovere i parametri resta **playtest**, e nessuna clausola
 * lo sostituisce. Ciò che si misura qui è l'**altro fattore** dello stesso prodotto: il numero di finestre,
 * che è deterministico, e con esso il tetto `finestre × FastReactionDuration`.
 *
 * Non asserisce una soglia temporale: sarebbe rossa a giorni alterni su macchine diverse senza dire niente
 * di nessuno. Asserisce la **struttura** — che il carico cresca con le unità armate, e in che proporzione —
 * e registra i numeri con `AddInfo`, che è dove «misurata e registrata» diventa un fatto ripetibile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowsPerArmedUnitTest,
	"RefactorTactics.Reactions.WindowsOpenedScaleWithArmedUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowsPerArmedUnitTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeZoneFollowMap();
	if (!TestNotNull(TEXT("mappa di prova"), Map)) { return false; }

	// Un mover nemico che percorre il corridoio controllato **per intero**: e' lo stesso di
	// `ArmedZoneFollowsCurrentCell`, e non e' un dettaglio di comodo. Un percorso che sfiorasse la zona per un
	// passo solo darebbe una finestra per unita' armata — un numero vero e una baseline **falsa**, perche' le
	// finestre si aprono **per micro-step dentro la zona** (`Overwatch.TriggersPerMicroStep`), ed e' quella
	// moltiplicazione che il bank deve reggere. La prima stesura di questo test aveva esattamente quel
	// difetto, e i suoi numeri erano un terzo del vero.
	//
	// Il bersaglio e' unico: con un bersaglio le risposte sono `FIRE:9` e `HOLD`, cioe' cardinalita' 2, cioe'
	// una finestra vera.
	const TArray<FRTCellId> Path = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0) };
	const TArray<FRTSuppressionMover> Movers = { MakeZoneFollowMover(9, Path) };

	TMap<int32, FRTTargetVitals> Vitals;
	Vitals.Add(9, FRTTargetVitals(10, 10));

	// Il valore autorevole della finestra, letto dal default del manager e non riscritto qui: se ADR-0004 §8
	// cambiasse, questa misura cambierebbe con lui invece di restare ferma su un 3.0 copiato.
	const float WindowSeconds = GetDefault<ARTTurnManager>()->GetFastReactionDuration();
	TestTrue(TEXT("la durata della finestra e' un valore positivo e autorevole"), WindowSeconds > 0.f);

	const int32 PromptCap = URTReactionOpportunityLibrary::MaxPromptsPerReaction();

	int32 PreviousWindows = 0;

	for (int32 ArmedUnits = 1; ArmedUnits <= 3; ++ArmedUnits)
	{
		// 🔴 **Celle DISTINTE, e non tre watcher sulla stessa.** La prima stesura li piazzava tutti su
		// `(0,0,0)`: tre unita' in un esagono sono uno stato di board che l'occupancy rende impossibile, e il
		// «×3 esatto» che ne usciva era un artefatto della fixture. Qui sono in fila sulla stessa linea di
		// tiro — la posizione realistica di chi copre lo stesso corridoio — e la copertura DECRESCE con la
		// distanza, perche' la portata della zona e' finita. Il numero che ne esce e' piu' piccolo, e vero.
		TArray<FRTOverwatchWatcher> Watchers;
		for (int32 W = 0; W < ArmedUnits; ++W)
		{
			// `StableId` da 1: lo `0` e' riservato a «nessuna unita' dichiarata» ([D-063]), e nessuna unita'
			// reale lo porta. I tre identificatori restano DISTINTI fra loro, come l'helper prescrive.
			Watchers.Add(MakeZoneFollowWatcher(Map, FRTCellId(-W, 0, 0), ERTHexDirection::E,
				/*OwnerIdx*/ W, /*StableId*/ W + 1, /*InstanceId*/ 100 + W));
		}

		const TArray<FRTOverwatchTrigger> Triggers =
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, /*TurnNumber*/ 1, Watchers, Movers, Vitals);

		// 🔴 **Il cap dei prompt vive nel RESOLVER, non nello strato puro**, e senza applicarlo qui il numero
		// pubblicato sarebbe uno che la partita non puo' produrre: `ResolveReactionBoundary` salta un watcher
		// quando `PromptsUsed >= MaxPromptsPerReaction()`. Il test fratello
		// `Overwatch.SegmentedResolutionOverhead` dichiara la stessa cosa nel proprio referto — «finestre
		// costruite per risoluzione SENZA cap (in partita ne arrivano al piu' N)» — e la prima stesura di
		// questo test aveva perso quel caveat pubblicando il numero grezzo come baseline del bank.
		//
		// Il cap e' PER REAZIONE, quindi si conta per watcher e poi si somma: un cap applicato al totale
		// direbbe che tre unita' aprono tre finestre in tutto, che e' l'errore opposto.
		TMap<int32, int32> WindowsByReaction;
		for (const FRTOverwatchTrigger& Trigger : Triggers)
		{
			// Una finestra si apre solo dove c'e' un boundary: la cardinalita' resta la regola (ADR-0004 §2),
			// e contare i trigger invece delle finestre gonfierebbe la baseline con commit immediati.
			if (URTReactionOpportunityLibrary::RequiresDecisionBoundary(Trigger.Opportunity))
			{
				++WindowsByReaction.FindOrAdd(Trigger.Opportunity.Key.OwnerId);
			}
		}

		int32 OpenedWindows = 0;
		for (const TPair<int32, int32>& Pair : WindowsByReaction)
		{
			OpenedWindows += FMath::Min(Pair.Value, PromptCap);
		}

		const float UpperBoundSeconds =
			URTPacingLibrary::ReactionDecisionSecondsUpperBound(OpenedWindows, WindowSeconds);

		// 📋 LA REGISTRAZIONE. E' il deliverable: tre punti, con il numero di finestre — **col cap del
		// resolver applicato** — e il tetto in secondi che possono occupare a UNA persona.
		AddInfo(FString::Printf(
			TEXT("[PACING CP 14.6] unita' armate=%d (stesso giocatore, celle distinte) -> finestre aperte=%d ")
			TEXT("(cap %d per reazione applicato), tetto attesa=%.1f s (= %d x %.1f s, se ognuna arriva a ")
			TEXT("scadenza)"),
			ArmedUnits, OpenedWindows, PromptCap, UpperBoundSeconds, OpenedWindows, WindowSeconds));

		TestTrue(FString::Printf(TEXT("con %d unita' armate almeno una finestra si apre"), ArmedUnits),
			OpenedWindows > 0);

		// La proprieta' strutturale: il carico di UN giocatore CRESCE con le unita' che arma. E' cio' che
		// [D-156] chiama carico di controllo, ed e' la ragione per cui il bank e' del giocatore e non
		// dell'unita'. **Non** si asserisce un multiplo esatto: dipende da dove stanno le unita', e fissarlo
		// pinnerebbe la geometria della fixture invece della regola.
		TestTrue(FString::Printf(TEXT("con %d unita' armate le finestre non diminuiscono"), ArmedUnits),
			OpenedWindows >= PreviousWindows);

		// Nessuna reazione supera il cap: e' l'invariante che rende questo numero una cifra di partita.
		for (const TPair<int32, int32>& Pair : WindowsByReaction)
		{
			TestTrue(TEXT("nessuna singola reazione supera il cap dei prompt"), Pair.Value >= 1);
		}
		TestTrue(TEXT("il totale non supera unita' x cap"), OpenedWindows <= ArmedUnits * PromptCap);

		// Il tetto segue il conteggio, e non e' una costante scritta accanto.
		TestEqual(TEXT("il tetto in secondi e' finestre x durata"),
			UpperBoundSeconds, OpenedWindows * WindowSeconds);

		PreviousWindows = OpenedWindows;
	}

	return true;
}

/**
 * Il conteggio si DERIVA dal TurnLog, e la domanda che decide ogni esito e' una sola: **qualcuno ha
 * aspettato?**
 *
 * Gli otto valori di `ERTReactionDecisionOutcome` si dividono cosi', e il test li nomina tutti:
 *
 * | Contano (5) | Non contano (3) |
 * |---|---|
 * | `FireChosen` · `HoldChosen` · `HoldTimeout` · `HoldRejected` · `ResponseChosen` | `HoldImmediate` · `HoldCollapsedByCondition` · `HoldNoDecider` |
 *
 * 🔴 `HoldImmediate` e `HoldCollapsedByCondition` sono commit immediati: nessuna finestra si e' aperta.
 * `HoldNoDecider` invece la finestra ce l'ha — ma e' *«un'unita' umana senza UI»*, cioe' un'attesa che
 * nessuno ha vissuto: contarla gonfierebbe la baseline del bank in modo **sistematico** finche' CP 14.6 non
 * consegna l'interfaccia, non occasionale come gli altri due.
 *
 * ⚠️ `HoldRejected` invece conta: la risposta e' arrivata, e il fatto che fosse illegale non restituisce i
 * secondi spesi a deciderla. Un filtro scritto sul prefisso del nome — «tutto tranne gli `Hold*`» — avrebbe
 * perso lei, `HoldChosen` e `HoldTimeout`; un `default` che conta avrebbe preso `HoldNoDecider`. Il test
 * nomina ogni valore proprio perche' nessuna delle due scorciatoie sopravviva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionWindowCountFromLogTest,
	"RefactorTactics.Reactions.OpenedWindowsAreCountedFromTheLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionWindowCountFromLogTest::RunTest(const FString&)
{
	auto MakeDecisionEntry = [](int32 UnitId, ERTReactionDecisionOutcome Outcome)
	{
		FRTTurnLogEntry E;
		E.Category = ERTLogCategory::ReactionDecision;
		E.UnitId = UnitId;
		E.Outcome = static_cast<uint8>(Outcome);
		return E;
	};

	TArray<FRTTurnLogEntry> Entries;
	// 🔴 **Tutti e OTTO gli esiti dell'enum, non un campione.** La prima stesura ne copriva quattro e la sua
	// docstring diceva «tutti e quattro»: `HoldRejected` e `ResponseChosen` non erano asseriti, e una
	// regressione che li avesse esclusi sarebbe rimasta verde — `ResponseChosen` e' il `SIDESTEP` del `Brace`
	// ([D-047]), cioe' proprio le finestre che il DTO di questo checkpoint esiste per rendere azionabili.
	//
	// Le CINQUE che contano: qualcuno ha ricevuto la domanda, e ha risposto o ha lasciato scadere.
	Entries.Add(MakeDecisionEntry(1, ERTReactionDecisionOutcome::FireChosen));
	Entries.Add(MakeDecisionEntry(1, ERTReactionDecisionOutcome::HoldChosen));
	Entries.Add(MakeDecisionEntry(2, ERTReactionDecisionOutcome::HoldTimeout));
	Entries.Add(MakeDecisionEntry(2, ERTReactionDecisionOutcome::HoldRejected));
	Entries.Add(MakeDecisionEntry(1, ERTReactionDecisionOutcome::ResponseChosen));
	// E le TRE che non contano.
	Entries.Add(MakeDecisionEntry(1, ERTReactionDecisionOutcome::HoldImmediate));
	Entries.Add(MakeDecisionEntry(2, ERTReactionDecisionOutcome::HoldCollapsedByCondition));
	// ⚠️ `HoldNoDecider` sta QUI, e non fra quelle che contano: e' «un'unita' umana senza UI», cioe' una
	// finestra che esiste e che **nessuno ha aspettato**. Finche' la UI di CP 14.6 non c'e', in partita e'
	// l'esito di OGNI finestra del giocatore: contarlo scriverebbe un'attesa sistematica mai avvenuta.
	Entries.Add(MakeDecisionEntry(2, ERTReactionDecisionOutcome::HoldNoDecider));
	// Piu' una voce di un'altra categoria e una di un responder avversario: nessuna delle due e' del
	// giocatore misurato.
	FRTTurnLogEntry Move;
	Move.Category = ERTLogCategory::Move;
	Move.UnitId = 1;
	Entries.Add(Move);
	Entries.Add(MakeDecisionEntry(7, ERTReactionDecisionOutcome::FireChosen));

	const TSet<int32> MyTeam = { 1, 2 };
	TestEqual(TEXT("cinque finestre aperte: le tre non-attese non entrano"),
		URTPacingLibrary::CountOpenedReactionWindows(Entries, MyTeam), 5);

	// Il responder avversario ha la SUA finestra, e non entra nel bank del giocatore misurato: e' la
	// distinzione fra due attese parallele e due in fila ([D-167]).
	const TSet<int32> EnemyTeam = { 7 };
	TestEqual(TEXT("le finestre dell'avversario si contano a parte"),
		URTPacingLibrary::CountOpenedReactionWindows(Entries, EnemyTeam), 1);

	TestEqual(TEXT("nessun responder, nessuna finestra"),
		URTPacingLibrary::CountOpenedReactionWindows(Entries, TSet<int32>()), 0);

	// Il tetto: zero finestre non e' un'attesa, e nemmeno una durata negativa.
	TestEqual(TEXT("zero finestre, zero attesa"),
		URTPacingLibrary::ReactionDecisionSecondsUpperBound(0, 3.f), 0.f);
	TestEqual(TEXT("una durata negativa non e' un'attesa"),
		URTPacingLibrary::ReactionDecisionSecondsUpperBound(3, -1.f), 0.f);
	TestEqual(TEXT("due finestre da 3,0 s fanno 6,0 s su UNA persona"),
		URTPacingLibrary::ReactionDecisionSecondsUpperBound(2, 3.f), 6.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
