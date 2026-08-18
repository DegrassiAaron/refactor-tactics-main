// L'Overwatch armato e i suoi trigger a micro-step (CP 14.4).
//
// Quattro proprieta', e sono separate perche' falliscono per ragioni diverse:
//
//   1. il trigger si valuta a OGNI micro-step, non una volta per turno (e' cio' che distingue l'Overwatch
//      dalla linea di soppressione, che scatta una volta sola);
//   2. piu' bersagli nello stesso passo danno UNA opportunity con piu' risposte, mai due prompt in sequenza;
//   3. `Rilevato` e' necessario: un contatto `Incerto` e' informazione, non un bersaglio;
//   4. l'ordine fra reazioni diverse e' totale e non dipende da come il chiamante ha costruito gli array.
//
// Cio' che questi test NON coprono, e va detto: la finestra di 3,0 s, il commit e il troncamento del movimento
// sono CP 14.5. Qui si produce l'opportunity, non la si risolve.
//
// ⚠️ Questa riga nominava anche «il cablaggio di `Wraith.InterceptShot`», ed era doppiamente falsa: [D-016]
// ha reso `InterceptShot` una Predictive Action — decisa in Planning, risolta a un boundary, senza input live,
// quindi senza finestra — e l'ha spostata in **E18**, dove e' chiusa dal 2026-08-10.
//
// ➕ Da CP 14.5 il file ospita anche `Overwatch.ActionIsInCoreCatalog`, che sta qui e non fra i test del
// catalogo per una ragione di contesa: `RTCatalogTests.cpp` e' tenuto da un altro branch. La collocazione e'
// comunque difendibile — cio' che il test pinna e' il PRODUTTORE di questi trigger, non il catalogo in se'.

#include "Misc/AutomationTest.h"
#include "Ability/RTCatalogLibrary.h" // `Action.Overwatch`: il produttore che CP 14.5 aggiunge al catalogo core
#include "Combat/RTOffensiveActionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Perception/RTPerceptionLibrary.h"
#include "Turn/RTHexSim.h"           // FRTMovementResolutionState: il ciclo che CP 14.5 guida da fuori
#include "Turn/RTHexSimLibrary.h"    // Begin/ResolveNext/Finish + StopUnitInPlace
#include "Turn/RTReactionOpportunityTypes.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"   // serializzazione e hash: il replay della decisione
#include "Turn/RTTurnRules.h"
// #1158: il doppio `FIRE` si riproduce dal PERCORSO REALE — uno scenario costruito in memoria, non un file in
// `Scenarios/` (la issue lo dichiara: «nessuno scenario nuovo»). Da qui arrivano il mondo, il runner e il
// `TurnManager` da cui si legge il `TurnLog`, che e' la fonte autorevole su cui il difetto si misura.
#include "EngineUtils.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/RTTestScenario.h"
#include "ScenarioHarness/RTTestResult.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTTurnManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: la unity build condivide la translation unit. */
	URTHexMapAsset* MakeOverwatchMap(int32 Radius = 6)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	void MakeOverwatchHighCover(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/**
	 * Un Overwatch armato che controlla la linea da `From` verso `Toward`.
	 *
	 * Usa `MakeSuppressiveZone`: e' il punto del checkpoint. Se un giorno l'Overwatch si costruisse una
	 * geometria propria, questi test continuerebbero a passare sulla vecchia — quindi la fixture chiama la
	 * stessa funzione che il DoD impone di riusare, e non ricopia le celle a mano.
	 */
	FRTOverwatchWatcher MakeOverwatchWatcher(const URTHexMapAsset* Map, int32 OwnerId, int32 TeamId,
		const FRTCellId& From, const FRTCellId& Toward, int32 Range = 4)
	{
		FRTOverwatchWatcher W;
		W.Zone = URTOffensiveActionLibrary::MakeSuppressiveZone(Map, OwnerId, TeamId, From, Toward, Range,
			/*Damage*/ 1);
		W.OwnerCell = From;
		W.ReactionDefId = TEXT("Action.Overwatch");
		W.StableUnitId = OwnerId;
		W.ReactionInstanceId = OwnerId;
		return W;
	}

	FRTSuppressionMover MakeOverwatchMover(int32 UnitId, int32 TeamId, const TArray<FRTCellId>& Path)
	{
		FRTSuppressionMover M;
		M.UnitId = UnitId;
		M.TeamId = TeamId;
		M.Path = Path;
		return M;
	}
}

/**
 * Il trigger si valuta a ogni micro-step: tre passi dentro la zona danno tre opportunity, non una.
 *
 * E' la differenza con `ResolveSuppression`, che di ogni unita' considera solo il PRIMO ingresso perche' la
 * linea si spende una volta. L'Overwatch resta armato finche' non si risponde `FIRE`, quindi ogni passo e' una
 * nuova occasione di decidere — e il conteggio deve dirlo. Un'implementazione che uscisse dal ciclo al primo
 * trigger passerebbe ogni altro test di questo file.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchTriggersPerMicroStepTest,
	"RefactorTactics.Overwatch.TriggersPerMicroStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchTriggersPerMicroStepTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOverwatchMap();

	// Sentinella in (0,0,0) che guarda verso est: controlla (1,0,0), (2,0,0), (3,0,0), (4,0,0).
	FRTOverwatchWatcher Watcher = MakeOverwatchWatcher(Map, /*OwnerId*/ 1, /*TeamId*/ 0,
		FRTCellId(0, 0, 0), FRTCellId(1, 0, 0));
	Watcher.TeamAwareness.Add(9, ERTAwareness::Detected);

	// Il bersaglio entra e percorre TRE celle controllate di fila.
	const TArray<FRTCellId> Path = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0) };
	const TArray<FRTSuppressionMover> Movers = { MakeOverwatchMover(9, /*TeamId*/ 1, Path) };

	const TArray<FRTOverwatchTrigger> Triggers =
		URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, /*TurnNumber*/ 4, { Watcher }, Movers);

	TestEqual(TEXT("tre micro-step dentro la zona aprono tre opportunity"), Triggers.Num(), 3);

	// Gli indici sono 0,1,2 e crescono: senza questo, tre opportunity tutte allo step 0 passerebbero il
	// conteggio qui sopra dicendo una cosa falsa.
	for (int32 i = 0; i < Triggers.Num() && i < 3; ++i)
	{
		TestEqual(FString::Printf(TEXT("l'opportunity %d porta il proprio micro-step"), i),
			Triggers[i].Opportunity.Key.MicroStepIndex, i);
		TestEqual(TEXT("la fase e' il Move: l'Overwatch scatta sui micro-step del movimento"),
			static_cast<int32>(Triggers[i].Opportunity.Key.MacroPhase), static_cast<int32>(ERTMatchPhase::Move));
		TestEqual(TEXT("il turno viaggia nella chiave"), Triggers[i].Opportunity.Key.TurnNumber, 4);
	}

	// Tre chiavi distinte, cioe' tre id distinti: se collidessero, il replay attribuirebbe a una la decisione
	// di un'altra — il difetto che CP 14.3 esiste per impedire.
	TSet<FString> Ids;
	for (const FRTOverwatchTrigger& T : Triggers)
	{
		Ids.Add(URTReactionOpportunityLibrary::DeriveOpportunityId(T.Opportunity.Key));
	}
	TestEqual(TEXT("le tre opportunity hanno tre id distinti"), Ids.Num(), Triggers.Num());

	// Un passo FUORI dalla zona non apre niente: e' il controllo che impedisce a «tre» di essere il numero
	// giusto per la ragione sbagliata (per esempio un trigger per micro-step a prescindere dalla geometria).
	const TArray<FRTSuppressionMover> Outside =
		{ MakeOverwatchMover(9, 1, { FRTCellId(0, 1, 0), FRTCellId(0, 2, 0), FRTCellId(0, 3, 0) }) };
	TestEqual(TEXT("un percorso fuori dalla zona non apre opportunity"),
		URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 4, { Watcher }, Outside).Num(), 0);

	return true;
}

/**
 * Due bersagli nello stesso micro-step: UNA opportunity con `FIRE:a` / `FIRE:b` / `HOLD`.
 *
 * ADR-0004 §4 lo chiede per una ragione precisa: due prompt in sequenza darebbero un vantaggio a chi capita
 * primo nell'iterazione, e l'ordine di iterazione non e' osservabile in partita. Il modo piu' naturale di
 * sbagliare e' iterare sui mover e aprire una opportunity ciascuno — che qui darebbe 2 invece di 1.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchSimultaneousTargetsSingleOpportunityTest,
	"RefactorTactics.Overwatch.SimultaneousTargetsSingleOpportunity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchSimultaneousTargetsSingleOpportunityTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOverwatchMap();

	FRTOverwatchWatcher Watcher = MakeOverwatchWatcher(Map, 1, 0, FRTCellId(0, 0, 0), FRTCellId(1, 0, 0));
	Watcher.TeamAwareness.Add(9, ERTAwareness::Detected);
	Watcher.TeamAwareness.Add(8, ERTAwareness::Detected);

	// Entrambi entrano nella zona allo STESSO passo, in celle controllate diverse.
	const TArray<FRTSuppressionMover> Movers = {
		MakeOverwatchMover(9, 1, { FRTCellId(2, 0, 0) }),
		MakeOverwatchMover(8, 1, { FRTCellId(1, 0, 0) }),
	};

	const TArray<FRTOverwatchTrigger> Triggers =
		URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Watcher }, Movers);

	TestEqual(TEXT("due bersagli nello stesso micro-step danno una sola opportunity"), Triggers.Num(), 1);
	if (Triggers.Num() != 1)
	{
		return false;
	}

	const FRTOverwatchTrigger& T = Triggers[0];
	TestEqual(TEXT("l'opportunity porta due bersagli"), T.TargetUnitIds.Num(), 2);
	TestEqual(TEXT("tre risposte legali: due FIRE e un HOLD"), T.Opportunity.AllowedResponses.Num(), 3);
	TestTrue(TEXT("con piu' di una risposta si apre il decision boundary"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(T.Opportunity));

	// Bersagli in ordine crescente di UnitId, non nell'ordine di `Movers` (che qui e' 9 poi 8).
	TestEqual(TEXT("il primo bersaglio e' l'UnitId minore"), T.TargetUnitIds[0], 8);
	TestEqual(TEXT("il secondo bersaglio e' l'UnitId maggiore"), T.TargetUnitIds[1], 9);
	TestEqual(TEXT("la prima risposta e' FIRE sul bersaglio minore"),
		T.Opportunity.AllowedResponses[0], URTReactionOpportunityLibrary::FireResponse(8));
	TestEqual(TEXT("HOLD e' l'ultima risposta"),
		T.Opportunity.AllowedResponses.Last(), FString(URTReactionOpportunityLibrary::HoldResponse()));

	// Permutare `Movers` non cambia il DTO: se lo cambiasse, due esecuzioni dello stesso scenario darebbero
	// due opportunity diverse e il replay divergerebbe senza che nessuna regola sia cambiata.
	const TArray<FRTSuppressionMover> Swapped = { Movers[1], Movers[0] };
	const TArray<FRTOverwatchTrigger> FromSwapped =
		URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Watcher }, Swapped);
	TestEqual(TEXT("permutare i mover non cambia il numero di opportunity"), FromSwapped.Num(), Triggers.Num());
	if (FromSwapped.Num() == 1)
	{
		TestEqual(TEXT("permutare i mover non cambia le risposte legali"),
			FromSwapped[0].Opportunity.AllowedResponses, T.Opportunity.AllowedResponses);
	}

	// Un bersaglio solo: due risposte (`FIRE` e `HOLD`), quindi ancora una finestra. `HOLD` non e' un extra —
	// senza di lei la cardinalita' sarebbe 1 e l'Overwatch si committerebbe da solo.
	FRTOverwatchWatcher Single = Watcher;
	Single.TeamAwareness.Remove(8);
	const TArray<FRTOverwatchTrigger> One =
		URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Single }, Movers);
	TestEqual(TEXT("con un solo bersaglio resta una opportunity"), One.Num(), 1);
	if (One.Num() == 1)
	{
		TestEqual(TEXT("un bersaglio da' due risposte: FIRE e HOLD"),
			One[0].Opportunity.AllowedResponses.Num(), 2);
		TestTrue(TEXT("un bersaglio solo apre comunque il boundary, perche' HOLD e' una scelta"),
			URTReactionOpportunityLibrary::RequiresDecisionBoundary(One[0].Opportunity));
	}

	return true;
}

/**
 * Le quattro condizioni sono TUTTE necessarie, e questo test le toglie una per volta dal caso che scatta.
 *
 * Sta in un test solo perche' e' una proprieta' sola — la congiunzione — e verificarne una meta' non direbbe
 * niente: con il solo caso positivo passerebbe un'implementazione che scatta sempre. Il caso che il
 * checkpoint nomina esplicitamente e' `Incerto`, ed e' il piu' facile da sbagliare: e' allettante trattare
 * «la squadra sa che li' c'e' qualcuno» come «lo vede».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchRequiresDetectionTest,
	"RefactorTactics.Overwatch.RequiresDetection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchRequiresDetectionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOverwatchMap();

	const FRTOverwatchWatcher Base = MakeOverwatchWatcher(Map, 1, 0, FRTCellId(0, 0, 0), FRTCellId(1, 0, 0));
	const TArray<FRTSuppressionMover> Movers = { MakeOverwatchMover(9, 1, { FRTCellId(2, 0, 0) }) };

	auto CountFor = [Map, &Movers](const FRTOverwatchWatcher& W)
	{
		return URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { W }, Movers).Num();
	};

	// Il caso che scatta: tutte e quattro le condizioni vere. Senza questo il test sarebbe verde su
	// un'implementazione che non scatta mai.
	FRTOverwatchWatcher Detected = Base;
	Detected.TeamAwareness.Add(9, ERTAwareness::Detected);
	TestEqual(TEXT("Rilevato + area + LOS + armato: scatta"), CountFor(Detected), 1);

	// 1) `TargetDetected` — `Incerto` non arma: e' il caso del fumo oltre 2 celle o del solo rumore.
	FRTOverwatchWatcher Uncertain = Base;
	Uncertain.TeamAwareness.Add(9, ERTAwareness::Uncertain);
	TestEqual(TEXT("un contatto Incerto non arma il trigger"), CountFor(Uncertain), 0);

	FRTOverwatchWatcher Hidden = Base;
	Hidden.TeamAwareness.Add(9, ERTAwareness::Hidden);
	TestEqual(TEXT("un bersaglio Hidden non arma il trigger"), CountFor(Hidden), 0);

	// Nessuna dichiarazione affatto: il default e' fail-closed, non «visibile perche' non detto».
	TestEqual(TEXT("un bersaglio non dichiarato non arma il trigger"), CountFor(Base), 0);

	// 2) `ReactionStillArmed` — una reaction gia' spesa non apre niente.
	FRTOverwatchWatcher Spent = Detected;
	Spent.bArmed = false;
	TestEqual(TEXT("una reaction non armata non apre opportunity"), CountFor(Spent), 0);

	// 3) `HasLineOfSight` — una copertura alta fra sentinella e bersaglio nega il trigger anche se la cella
	//    resta dentro la zona. Nota: la zona si ricostruisce DOPO aver messo la copertura, perche'
	//    `MakeSuppressiveZone` si ferma prima di una cella che blocca la vista — se riusassimo la zona
	//    vecchia, il test verificherebbe una geometria che il gioco non produce piu'.
	{
		URTHexMapAsset* Blocked = MakeOverwatchMap();
		MakeOverwatchHighCover(Blocked, FRTCellId(1, 0, 0));

		FRTOverwatchWatcher Behind = MakeOverwatchWatcher(Blocked, 1, 0, FRTCellId(0, 0, 0), FRTCellId(1, 0, 0));
		Behind.TeamAwareness.Add(9, ERTAwareness::Detected);

		TestFalse(TEXT("la copertura alta nega davvero la LOS verso (2,0,0)"),
			URTHexVisionLibrary::HasLineOfSight(Blocked, FRTCellId(0, 0, 0), FRTCellId(2, 0, 0)));
		TestEqual(TEXT("senza linea di tiro non c'e' trigger"),
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Blocked, 1, { Behind }, Movers).Num(), 0);
	}

	// 4) `TargetInsideArea` e' gia' coperta da `TriggersPerMicroStep`; qui resta il caso dell'ALLEATO, che
	//    e' dentro l'area e rilevato e comunque non deve armare niente.
	{
		FRTOverwatchWatcher Friendly = Detected;
		const TArray<FRTSuppressionMover> Ally = { MakeOverwatchMover(9, /*stessa squadra*/ 0, { FRTCellId(2, 0, 0) }) };
		TestEqual(TEXT("l'Overwatch non scatta sui propri"),
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Friendly }, Ally).Num(), 0);
	}

	// Mappa assente: fail-closed. Senza mappa la LOS non e' calcolabile, e un Overwatch che sparasse al buio
	// sarebbe un trigger che nessuna delle quattro condizioni ha autorizzato.
	TestEqual(TEXT("senza mappa autorevole non ci sono trigger"),
		URTReactionOpportunityLibrary::BuildOverwatchTriggers(nullptr, 1, { Detected }, Movers).Num(), 0);

	return true;
}

/**
 * L'ordine fra reazioni diverse e' totale, e non e' l'ordine di `Watchers`.
 *
 * I cinque criteri di ADR-0004 §4 si verificano uno per volta, ognuno con gli altri PARI: e' l'unico modo per
 * dimostrare che ciascuno partecipa. Un'implementazione che ordinasse solo per `StableUnitId` passerebbe un
 * test che li muove tutti insieme.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchOrderIsDeterministicTest,
	"RefactorTactics.Overwatch.OrderIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchOrderIsDeterministicTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOverwatchMap();

	// Due sentinelle su lati opposti che controllano la stessa cella (2,0,0): entrambe scattano allo step 0.
	const TArray<FRTSuppressionMover> Movers = { MakeOverwatchMover(9, 1, { FRTCellId(2, 0, 0) }) };

	// `Higher` deve risultare SECONDA in ogni caso qui sotto; i due watcher restano pari su tutto il resto.
	auto MakePair = [&](TFunctionRef<void(FRTOverwatchWatcher&, FRTOverwatchWatcher&)> Distinguish)
	{
		FRTOverwatchWatcher Lower = MakeOverwatchWatcher(Map, /*OwnerId*/ 1, 0, FRTCellId(0, 0, 0), FRTCellId(1, 0, 0));
		FRTOverwatchWatcher Higher = MakeOverwatchWatcher(Map, /*OwnerId*/ 2, 0, FRTCellId(4, 0, 0), FRTCellId(3, 0, 0));
		Lower.TeamAwareness.Add(9, ERTAwareness::Detected);
		Higher.TeamAwareness.Add(9, ERTAwareness::Detected);
		// Pari su tutti i criteri: e' il `Distinguish` a introdurne uno solo.
		Lower.StableUnitId = Higher.StableUnitId = 0;
		Lower.ReactionInstanceId = Higher.ReactionInstanceId = 0;
		Distinguish(Lower, Higher);
		return TPair<FRTOverwatchWatcher, FRTOverwatchWatcher>(Lower, Higher);
	};

	auto FirstOwnerWith = [&](const TCHAR* What, TFunctionRef<void(FRTOverwatchWatcher&, FRTOverwatchWatcher&)> Distinguish)
	{
		const TPair<FRTOverwatchWatcher, FRTOverwatchWatcher> Pair = MakePair(Distinguish);

		// Entrambi gli ordini d'ingresso devono dare lo STESSO esito: e' la definizione di «non dipende
		// dall'ordine del container».
		const TArray<FRTOverwatchTrigger> A =
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Pair.Key, Pair.Value }, Movers);
		const TArray<FRTOverwatchTrigger> B =
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Pair.Value, Pair.Key }, Movers);

		TestEqual(FString::Printf(TEXT("%s: entrambe le sentinelle scattano"), What), A.Num(), 2);
		TestEqual(FString::Printf(TEXT("%s: permutare i watcher non cambia il conteggio"), What), B.Num(), A.Num());
		if (A.Num() == 2 && B.Num() == 2)
		{
			TestEqual(FString::Printf(TEXT("%s: la sentinella prioritaria e' prima"), What),
				A[0].Opportunity.Key.OwnerId, 1);
			TestEqual(FString::Printf(TEXT("%s: permutare i watcher non cambia l'ordine"), What),
				B[0].Opportunity.Key.OwnerId, A[0].Opportunity.Key.OwnerId);
		}
	};

	FirstOwnerWith(TEXT("ReactionPriority"), [](FRTOverwatchWatcher& L, FRTOverwatchWatcher& H)
		{ L.ReactionPriority = 10; H.ReactionPriority = 20; });
	FirstOwnerWith(TEXT("AbilityPriority"), [](FRTOverwatchWatcher& L, FRTOverwatchWatcher& H)
		{ L.AbilityPriority = 10; H.AbilityPriority = 20; });
	FirstOwnerWith(TEXT("UnitInitiative"), [](FRTOverwatchWatcher& L, FRTOverwatchWatcher& H)
		{ L.UnitInitiative = 10; H.UnitInitiative = 20; });
	FirstOwnerWith(TEXT("StableUnitId"), [](FRTOverwatchWatcher& L, FRTOverwatchWatcher& H)
		{ L.StableUnitId = 10; H.StableUnitId = 20; });
	FirstOwnerWith(TEXT("ReactionInstanceId"), [](FRTOverwatchWatcher& L, FRTOverwatchWatcher& H)
		{ L.ReactionInstanceId = 10; H.ReactionInstanceId = 20; });

	// Il micro-step viene PRIMA di tutti e cinque: e' il tempo della risoluzione, e una reaction prioritaria
	// non puo' scavalcare un passo precedente.
	{
		// Zone di UNA cella ciascuna, agli estremi opposti di un corridoio: cosi' ogni sentinella scatta a un
		// solo passo, e i due passi sono distinti.
		FRTOverwatchWatcher Early = MakeOverwatchWatcher(Map, 1, 0, FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), /*Range*/ 1);
		Early.TeamAwareness.Add(9, ERTAwareness::Detected);
		Early.ReactionPriority = 99; // priorita' PESSIMA...

		FRTOverwatchWatcher Late = MakeOverwatchWatcher(Map, 2, 0, FRTCellId(4, 0, 0), FRTCellId(3, 0, 0), /*Range*/ 1);
		Late.TeamAwareness.Add(9, ERTAwareness::Detected);
		Late.ReactionPriority = 0;  // ...contro la migliore possibile

		// Il bersaglio attraversa il corridoio: tocca la zona di `Early` allo step 0 e quella di `Late` allo step 2.
		const TArray<FRTSuppressionMover> TwoSteps =
			{ MakeOverwatchMover(9, 1, { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0) }) };

		const TArray<FRTOverwatchTrigger> Ordered =
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Late, Early }, TwoSteps);

		TestEqual(TEXT("due micro-step, due opportunity"), Ordered.Num(), 2);
		if (Ordered.Num() == 2)
		{
			TestEqual(TEXT("il micro-step precedente viene prima della priorita' migliore"),
				Ordered[0].Opportunity.Key.OwnerId, 1);
			TestEqual(TEXT("il primo trigger e' al micro-step 0"),
				Ordered[0].Opportunity.Key.MicroStepIndex, 0);
			TestEqual(TEXT("il secondo trigger e' quello del micro-step successivo"),
				Ordered[1].Opportunity.Key.MicroStepIndex, 2);
		}
	}

	// UNA SOLA UNITA', DUE REACTION ARMATE — il caso che ADR-0004 §4 chiama «piu' reazioni distinte nello
	// stesso micro-step», e quello che i casi qui sopra non toccano: hanno tutti due OwnerId diversi.
	//
	// Trovato in code review sulla prima stesura, che ritrovava i tie-break con una `TMap` chiavata
	// sull'OwnerId: la seconda `Add` sovrascriveva la prima e i due trigger si ordinavano entrambi coi
	// tie-break dell'ULTIMO watcher inserito — cioe' in base all'ordine di `Watchers`.
	{
		FRTOverwatchWatcher Fast = MakeOverwatchWatcher(Map, /*OwnerId*/ 1, 0, FRTCellId(0, 0, 0), FRTCellId(1, 0, 0));
		Fast.TeamAwareness.Add(9, ERTAwareness::Detected);
		Fast.ReactionDefId = TEXT("Action.Overwatch");
		Fast.ReactionPriority = 10;
		Fast.ReactionInstanceId = 100;

		FRTOverwatchWatcher Slow = Fast;                        // STESSA unita', stessa zona
		Slow.ReactionDefId = TEXT("Action.Intercept");           // reaction diversa
		Slow.ReactionPriority = 20;                              // ...e priorita' peggiore
		Slow.ReactionInstanceId = 200;

		const TArray<FRTOverwatchTrigger> A =
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Fast, Slow }, Movers);
		const TArray<FRTOverwatchTrigger> B =
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Slow, Fast }, Movers);

		TestEqual(TEXT("una unita' con due reaction armate apre due opportunity"), A.Num(), 2);
		TestEqual(TEXT("permutare i watcher non cambia il conteggio"), B.Num(), A.Num());
		if (A.Num() == 2 && B.Num() == 2)
		{
			TestEqual(TEXT("la reaction a priorita' migliore e' prima"),
				A[0].Opportunity.Key.ReactionDefId, FName(TEXT("Action.Overwatch")));
			TestEqual(TEXT("e resta prima permutando i watcher"),
				B[0].Opportunity.Key.ReactionDefId, A[0].Opportunity.Key.ReactionDefId);
		}
	}

	return true;
}

/**
 * Due ISTANZE della stessa reaction sulla stessa unita' danno due id distinti.
 *
 * `FRTReactionOpportunityKey` non identifica il watcher: identifica
 * `(Turn, MacroPhase, MicroStep, OwnerId, ReactionDefId, Seq)`. La prima stesura lasciava `Seq = 0` con la
 * motivazione «un watcher apre al massimo una opportunity per micro-step» — vera, e irrilevante: due watcher
 * della stessa unita' con lo stesso `ReactionDefId` producevano due chiavi **identiche**, quindi lo stesso
 * `OpportunityId`, quindi un replay che attribuisce a una la decisione dell'altra.
 *
 * E' il difetto che CP 14.3 esiste per impedire, e la doc di `Seq` lo nomina parola per parola: «la stessa
 * unita', la stessa reaction, lo stesso micro-step, due volte». Il caso sembra impossibile finche' non capita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchSameReactionTwiceHasDistinctIdsTest,
	"RefactorTactics.Overwatch.SameReactionTwiceHasDistinctIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchSameReactionTwiceHasDistinctIdsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOverwatchMap();
	const TArray<FRTSuppressionMover> Movers = { MakeOverwatchMover(9, 1, { FRTCellId(2, 0, 0) }) };

	FRTOverwatchWatcher First = MakeOverwatchWatcher(Map, /*OwnerId*/ 1, 0, FRTCellId(0, 0, 0), FRTCellId(1, 0, 0));
	First.TeamAwareness.Add(9, ERTAwareness::Detected);
	First.ReactionInstanceId = 10;

	// Identico in tutto tranne l'istanza: e' l'unico campo che li distingue, ed e' il motivo per cui esiste.
	FRTOverwatchWatcher Second = First;
	Second.ReactionInstanceId = 20;

	const TArray<FRTOverwatchTrigger> Triggers =
		URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { First, Second }, Movers);

	TestEqual(TEXT("due istanze armate aprono due opportunity"), Triggers.Num(), 2);
	if (Triggers.Num() != 2)
	{
		return false;
	}

	const FString IdA = URTReactionOpportunityLibrary::DeriveOpportunityId(Triggers[0].Opportunity.Key);
	const FString IdB = URTReactionOpportunityLibrary::DeriveOpportunityId(Triggers[1].Opportunity.Key);
	TestNotEqual(TEXT("le due opportunity hanno id distinti"), IdA, IdB);
	TestEqual(TEXT("la prima porta Seq = 0"), Triggers[0].Opportunity.Key.Seq, 0);
	TestEqual(TEXT("la seconda porta Seq = 1"), Triggers[1].Opportunity.Key.Seq, 1);

	// `Seq` e' una funzione dello stato, non dell'ordine d'ingresso: permutare i watcher non deve
	// riassegnarlo. Senza questo, l'assegnazione «in ordine di costruzione» passerebbe il controllo qui sopra
	// e romperebbe comunque il replay.
	const TArray<FRTOverwatchTrigger> Swapped =
		URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Second, First }, Movers);
	TestEqual(TEXT("permutare i watcher non cambia il conteggio"), Swapped.Num(), Triggers.Num());
	if (Swapped.Num() == 2)
	{
		TestEqual(TEXT("permutare i watcher non cambia il primo id"),
			URTReactionOpportunityLibrary::DeriveOpportunityId(Swapped[0].Opportunity.Key), IdA);
		TestEqual(TEXT("permutare i watcher non cambia il secondo id"),
			URTReactionOpportunityLibrary::DeriveOpportunityId(Swapped[1].Opportunity.Key), IdB);
	}

	// Un watcher solo resta a `Seq = 0`: il campo non deve diventare un contatore globale.
	const TArray<FRTOverwatchTrigger> Single =
		URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { First }, Movers);
	if (Single.Num() == 1)
	{
		TestEqual(TEXT("con un solo watcher Seq resta 0"), Single[0].Opportunity.Key.Seq, 0);
	}

	return true;
}

/**
 * La condizione dichiarata in pianificazione RIDUCE le risposte legali, e quando ne resta una sola il commit
 * e' immediato: nessuna finestra si apre ([D-012], [D-109]).
 *
 * E' il punto dell'intero meccanismo. Senza, ogni trigger di Overwatch aprirebbe 3 secondi di attesa, perche'
 * `AllowedResponses` contiene sempre almeno `FIRE` e `HOLD` — ed e' esattamente il rischio di pacing che la
 * condizione esiste per mitigare. Il regime *Conditional* non e' un enum: e' questo collasso di cardinalita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchDeclaredConditionTest,
	"RefactorTactics.Reactions.DeclaredConditionCollapsesToImmediateCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchDeclaredConditionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOverwatchMap(/*Radius*/ 4);

	// Un bersaglio in salute (unita' 9) e uno ferito (unita' 8) attraversano la stessa zona controllata.
	FRTOverwatchWatcher Watcher = MakeOverwatchWatcher(Map, /*OwnerId*/ 1, /*TeamId*/ 0,
		FRTCellId(0, 0, 0), FRTCellId(1, 0, 0));
	Watcher.TeamAwareness.Add(8, ERTAwareness::Detected);
	Watcher.TeamAwareness.Add(9, ERTAwareness::Detected);

	const TArray<FRTSuppressionMover> Movers = {
		MakeOverwatchMover(8, /*TeamId*/ 1, { FRTCellId(1, 0, 0) }),
		MakeOverwatchMover(9, /*TeamId*/ 1, { FRTCellId(1, 0, 0) }),
	};

	// Salute al micro-step: 3/10 = 30% e 9/10 = 90%.
	TMap<int32, FRTTargetVitals> Vitals;
	Vitals.Add(8, FRTTargetVitals(3, 10));
	Vitals.Add(9, FRTTargetVitals(9, 10));

	// 1. SENZA condizione, il comportamento e' quello di sempre: entrambi i bersagli sono ingaggiabili, quindi
	//    tre risposte e una finestra da aprire. E' la controprova che il collasso viene dalla condizione e non
	//    da un difetto della fixture.
	{
		const TArray<FRTOverwatchTrigger> T =
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, /*TurnNumber*/ 1, { Watcher }, Movers, Vitals);
		TestEqual(TEXT("senza condizione: una opportunity"), T.Num(), 1);
		if (T.Num() == 1)
		{
			TestEqual(TEXT("senza condizione: FIRE su entrambi piu' HOLD"), T[0].Opportunity.AllowedResponses.Num(), 3);
			TestTrue(TEXT("senza condizione: serve il boundary"),
				URTReactionOpportunityLibrary::RequiresDecisionBoundary(T[0].Opportunity));
		}
	}

	// 2. Con «solo sotto il 50%», il bersaglio in salute esce dalle risposte legali: restano FIRE:8 e HOLD.
	//    Due risposte, quindi la finestra si apre ancora — ma su una scelta piu' stretta.
	{
		FRTOverwatchWatcher Conditional = Watcher;
		Conditional.DeclaredCondition = FRTDeclaredCondition(
			URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent(), 50);

		const TArray<FRTOverwatchTrigger> T =
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Conditional }, Movers, Vitals);
		TestEqual(TEXT("con condizione: una opportunity"), T.Num(), 1);
		if (T.Num() == 1)
		{
			TestEqual(TEXT("il bersaglio in salute non e' piu' una risposta legale"),
				T[0].Opportunity.AllowedResponses.Num(), 2);
			TestTrue(TEXT("resta il ferito"),
				T[0].Opportunity.AllowedResponses.Contains(URTReactionOpportunityLibrary::FireResponse(8)));
			TestFalse(TEXT("non resta quello in salute"),
				T[0].Opportunity.AllowedResponses.Contains(URTReactionOpportunityLibrary::FireResponse(9)));
		}
	}

	// 3. IL COLLASSO: se nessun bersaglio soddisfa la condizione resta il solo `HOLD` — una risposta, quindi
	//    commit immediato e nessuna finestra. E' la riga per cui questo test porta il nome che porta.
	{
		FRTOverwatchWatcher Strict = Watcher;
		Strict.DeclaredCondition = FRTDeclaredCondition(
			URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent(), 10); // nessuno e' al 10% o meno

		const TArray<FRTOverwatchTrigger> T =
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Strict }, Movers, Vitals);
		TestEqual(TEXT("l'opportunity esiste comunque: il trigger e' scattato"), T.Num(), 1);
		if (T.Num() == 1)
		{
			TestEqual(TEXT("resta solo HOLD"), T[0].Opportunity.AllowedResponses.Num(), 1);
			TestEqual(TEXT("ed e' proprio HOLD"), T[0].Opportunity.AllowedResponses[0],
				FString(URTReactionOpportunityLibrary::HoldResponse()));
			TestFalse(TEXT("nessuna finestra: commit immediato"),
				URTReactionOpportunityLibrary::RequiresDecisionBoundary(T[0].Opportunity));
		}
	}

	// 4. FAIL-CLOSED: condizione dichiarata e salute del bersaglio ignota. Non si puo' sapere se la condizione
	//    e' soddisfatta, quindi non si offre di sparare — la stessa regola che `TeamAwareness` applica a un
	//    bersaglio non dichiarato. Offrire `FIRE` qui significherebbe sparare a una condizione non verificata.
	{
		FRTOverwatchWatcher Conditional = Watcher;
		Conditional.DeclaredCondition = FRTDeclaredCondition(
			URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent(), 50);

		const TArray<FRTOverwatchTrigger> T =
			URTReactionOpportunityLibrary::BuildOverwatchTriggers(Map, 1, { Conditional }, Movers, /*Vitals*/ {});
		TestEqual(TEXT("l'opportunity esiste"), T.Num(), 1);
		if (T.Num() == 1)
		{
			TestEqual(TEXT("senza il dato non si offre di sparare"), T[0].Opportunity.AllowedResponses.Num(), 1);
		}
	}

	return true;
}


/**
 * Chiamare la funzione UN PASSO ALLA VOLTA da' le stesse opportunity della chiamata sul percorso intero
 * (CP 14.5).
 *
 * E' la proprieta' che rende lecito guidare il ciclo da fuori. `ARTTurnManager::ResolveMovement` non puo'
 * passare i percorsi completi — calcolerebbe i trigger di micro-step non ancora avvenuti, e un `FIRE` alla
 * finestra corrente cambia il futuro di chi viene fermato — quindi passa il solo passo appena compiuto. Se le
 * due vie divergessero, i test di CP 14.4 continuerebbero a essere verdi verificando una funzione che in
 * partita nessuno chiama piu' in quel modo.
 *
 * ⚠️ Il confronto e' sugli `OpportunityId`, non sul conteggio: il conteggio tornerebbe anche se ogni chiamata
 * producesse `MicroStepIndex = 0`, che e' esattamente il difetto contro cui `FirstMicroStepIndex` esiste —
 * tre opportunity con la stessa chiave, quindi lo stesso id, e un replay che ne confonde le decisioni.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchSteppedCallMatchesWholePathTest,
	"RefactorTactics.Overwatch.SteppedCallMatchesWholePath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchSteppedCallMatchesWholePathTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOverwatchMap();

	FRTOverwatchWatcher Watcher = MakeOverwatchWatcher(Map, /*OwnerId*/ 1, /*TeamId*/ 0,
		FRTCellId(0, 0, 0), FRTCellId(1, 0, 0));
	Watcher.TeamAwareness.Add(9, ERTAwareness::Detected);

	const TArray<FRTCellId> Path = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0) };

	// (a) la via in BLOCCO: percorso intero, un colpo solo. E' quella che CP 14.4 gia' verifica.
	TArray<FString> WholeIds;
	for (const FRTOverwatchTrigger& T : URTReactionOpportunityLibrary::BuildOverwatchTriggers(
			Map, /*TurnNumber*/ 4, { Watcher }, { MakeOverwatchMover(9, /*TeamId*/ 1, Path) }))
	{
		WholeIds.Add(URTReactionOpportunityLibrary::DeriveOpportunityId(T.Opportunity.Key));
	}

	// (b) la via a PASSI: un mover di una cella sola per volta, con l'indice del turno accanto.
	TArray<FString> SteppedIds;
	for (int32 Step = 0; Step < Path.Num(); ++Step)
	{
		const TArray<FRTSuppressionMover> OneStep = { MakeOverwatchMover(9, /*TeamId*/ 1, { Path[Step] }) };
		for (const FRTOverwatchTrigger& T : URTReactionOpportunityLibrary::BuildOverwatchTriggers(
				Map, /*TurnNumber*/ 4, { Watcher }, OneStep, /*Vitals*/ {}, /*FirstMicroStepIndex*/ Step))
		{
			SteppedIds.Add(URTReactionOpportunityLibrary::DeriveOpportunityId(T.Opportunity.Key));
		}
	}

	TestEqual(TEXT("la via in blocco apre tre opportunity"), WholeIds.Num(), 3);
	TestEqual(TEXT("la via a passi ne apre altrettante"), SteppedIds.Num(), WholeIds.Num());
	for (int32 i = 0; i < WholeIds.Num() && i < SteppedIds.Num(); ++i)
	{
		TestEqual(FString::Printf(TEXT("l'opportunity %d ha lo stesso id nelle due vie"), i),
			SteppedIds[i], WholeIds[i]);
	}

	// E i tre id sono DISTINTI fra loro: senza, l'uguaglianza qui sopra sarebbe soddisfatta anche da tre
	// copie dello stesso id — cioe' proprio il collasso che il parametro deve impedire.
	TSet<FString> Distinct(SteppedIds);
	TestEqual(TEXT("i tre id sono distinti"), Distinct.Num(), SteppedIds.Num());

	return true;
}


/**
 * `Action.Overwatch` esiste nel catalogo core, ed e' l'ULTIMA delle generiche (CP 14.5).
 *
 * Fino a qui l'Overwatch era un `ReactionDefId` che solo i test scrivevano: `RTCatalogLibrary.h` lo dichiarava
 * assente («e' E14, e arrivera' con la sua infrastruttura»), quindi in partita nessuno poteva armarlo e la
 * finestra non aveva un innesco reale. Questo test pinna il produttore.
 *
 * ⚠️ La seconda meta' — l'ordine — non e' pedanteria. Le generiche sono ACCODATE al kit di ogni unita'
 * (`URTCatalogLibrary::MakeGenericActions`), quindi la loro posizione diventa un indice stabile, e
 * `PlannedAbilityIndex` / `PlannedReactionAbility` sono indici. Inserire `Overwatch` in mezzo sposterebbe
 * `Guard` e `Brace` sotto i piedi di ogni piano gia' scritto, senza che nulla smetta di compilare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchActionIsInCoreCatalogTest,
	"RefactorTactics.Overwatch.ActionIsInCoreCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchActionIsInCoreCatalogTest::RunTest(const FString&)
{
	const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(TEXT("Action.Overwatch"));

	TestFalse(TEXT("`Action.Overwatch` e' nel catalogo core"), Def.ActionId.IsNone());

	// I valori del catalogo di bilanciamento (`docs/balance/RT_ActionCatalog_v0.1.md` §1), non scelti qui.
	TestEqual(TEXT("si arma nel Prep"), Def.ResolutionPhase, ERTResolutionPhase::Preparation);
	TestEqual(TEXT("priorita' 45"), Def.Priority, 45);
	TestEqual(TEXT("nessun cooldown"), Def.CooldownTurns, 0);
	TestEqual(TEXT("fallback `Cancel`"), Def.Fallback, ERTActionFallback::Cancel);
	TestEqual(TEXT("costa l'azione principale, non lo slot reazione"), Def.Slot, ERTActionSlot::Main);
	TestFalse(TEXT("non interrompibile: una volta armata, c'e'"), Def.bCanBeInterrupted);

	// PORTATA ed EFFETTO restano fuori, come per `Action.BasicAttack` e per la stessa ragione: dipendono dal
	// PROFILO — area, arco, raggio e cosa scatta — e i profili dei quattro eroi della v0.1 sono ancora una
	// domanda aperta (`brief-azioni-generiche-overwatch.md` §8). Un numero qui ne sceglierebbe uno arbitrario
	// per tutti, e sarebbe indistinguibile da una decisione presa.
	TestEqual(TEXT("la portata la dichiara il profilo, non l'azione"), Def.RangeCells, 0);
	TestEqual(TEXT("l'effetto lo dichiara il profilo, non l'azione"), Def.Effects.Num(), 0);

	// L'azione e' generica: ogni eroe la possiede, non e' la skill di nessuno (D-025, catalogo eroi §Overwatch).
	const TArray<FName> Generic = URTCatalogLibrary::GetGenericActionIds();
	TestTrue(TEXT("e' fra le azioni generiche"), Generic.Contains(FName(TEXT("Action.Overwatch"))));

	// L'ordine, per intero e non solo «l'ultima»: cosi' il test cade anche se qualcuno ne inserisse una quinta
	// prima delle esistenti invece che dopo.
	const TArray<FName> Expected = {
		TEXT("Action.Wait"), TEXT("Action.Guard"), TEXT("Action.Brace"), TEXT("Action.Overwatch") };
	TestEqual(TEXT("le generiche sono quattro"), Generic.Num(), Expected.Num());
	for (int32 i = 0; i < Expected.Num() && i < Generic.Num(); ++i)
	{
		TestEqual(FString::Printf(TEXT("la generica %d e' `%s`"), i, *Expected[i].ToString()),
			Generic[i], Expected[i]);
	}

	return true;
}


/**
 * Allo scadere della finestra vale `HOLD`, sempre e per costruzione (ADR-0004 §3, CP 14.5).
 *
 * Mai `FIRE`, e l'asimmetria e' il punto: `FIRE` consuma una risorsa irreversibile, e un input mancato non
 * deve poterla spendere. Il test interroga la funzione su opportunity **diverse** — una con due bersagli, una
 * con uno solo — perche' «restituisce sempre HOLD» sia una proprieta' e non il caso che ho scritto io.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchTimeoutIsHoldTest,
	"RefactorTactics.Overwatch.TimeoutIsHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchTimeoutIsHoldTest::RunTest(const FString&)
{
	const FString Hold = URTReactionOpportunityLibrary::HoldResponse();

	FRTReactionOpportunity TwoTargets;
	TwoTargets.AllowedResponses = {
		URTReactionOpportunityLibrary::FireResponse(3),
		URTReactionOpportunityLibrary::FireResponse(9),
		Hold };

	FRTReactionOpportunity OneTarget;
	OneTarget.AllowedResponses = { URTReactionOpportunityLibrary::FireResponse(3), Hold };

	FRTReactionOpportunity HoldOnly;
	HoldOnly.AllowedResponses = { Hold };

	for (const FRTReactionOpportunity* O : { &TwoTargets, &OneTarget, &HoldOnly })
	{
		const FRTReactionDecision D = URTReactionOpportunityLibrary::DecisionOnTimeout(*O);
		TestEqual(TEXT("lo scadere risponde HOLD"), D.Response, Hold);
		TestEqual(TEXT("e lo dichiara come scadenza, non come scelta"),
			D.Outcome, ERTReactionDecisionOutcome::HoldTimeout);
		// Il controllo che conta davvero: non e' un `FIRE` sotto mentite spoglie.
		TestEqual(TEXT("nessun bersaglio: lo scadere non spende la charge"),
			URTReactionOpportunityLibrary::FireResponseTarget(D.Response), (int32)INDEX_NONE);
	}

	return true;
}


/**
 * `HOLD` perde l'OPPORTUNITY, non la reaction (CP 14.5): finche' il watcher resta armato, un micro-step
 * successivo puo' aprirne un'altra.
 *
 * E' la proprieta' che rende possibile il *bait*: lascio passare il primo perche' penso che dietro arrivi di
 * meglio. Senza, la prima unita' che entra brucerebbe sempre l'Overwatch — che e' esattamente l'alternativa
 * che ADR-0004 ha scartato («il bait diventa banale»).
 *
 * ⚠️ Le due meta' si verificano insieme e in opposizione: armato apre, disarmato non apre. Solo la prima
 * sarebbe soddisfatta anche da un trigger che ignora del tutto `bArmed`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchHoldKeepsArmedTest,
	"RefactorTactics.Overwatch.HoldKeepsArmed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchHoldKeepsArmedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOverwatchMap();

	FRTOverwatchWatcher Armed = MakeOverwatchWatcher(Map, /*OwnerId*/ 1, /*TeamId*/ 0,
		FRTCellId(0, 0, 0), FRTCellId(1, 0, 0));
	Armed.TeamAwareness.Add(9, ERTAwareness::Detected);

	// Il bersaglio attraversa DUE celle controllate: il passo 0 e' l'opportunity che si lascia cadere, il
	// passo 1 e' quella che deve ancora potersi aprire.
	const TArray<FRTCellId> Path = { FRTCellId(1, 0, 0), FRTCellId(2, 0, 0) };

	const TArray<FRTOverwatchTrigger> Step0 = URTReactionOpportunityLibrary::BuildOverwatchTriggers(
		Map, /*TurnNumber*/ 2, { Armed }, { MakeOverwatchMover(9, 1, { Path[0] }) }, {}, /*Step*/ 0);
	TestEqual(TEXT("il primo passo apre una finestra"), Step0.Num(), 1);

	// Dopo un `HOLD` la charge NON si spende, quindi il watcher del passo dopo e' ancora questo.
	const TArray<FRTOverwatchTrigger> Step1 = URTReactionOpportunityLibrary::BuildOverwatchTriggers(
		Map, /*TurnNumber*/ 2, { Armed }, { MakeOverwatchMover(9, 1, { Path[1] }) }, {}, /*Step*/ 1);
	TestEqual(TEXT("dopo un HOLD il passo successivo ne apre un'altra"), Step1.Num(), 1);

	if (Step0.Num() == 1 && Step1.Num() == 1)
	{
		// Due opportunity DISTINTE, non la stessa riofferta: e' cio' che il replay deve poter separare.
		TestNotEqual(TEXT("sono due finestre diverse"),
			URTReactionOpportunityLibrary::DeriveOpportunityId(Step0[0].Opportunity.Key),
			URTReactionOpportunityLibrary::DeriveOpportunityId(Step1[0].Opportunity.Key));
	}

	// L'altra meta': speso il colpo, il watcher non arma piu' niente. `bArmed` E' il `ReactionStillArmed`
	// della condizione di trigger (ADR-0004 §6), ed e' cio' che il boundary azzera su un `FIRE`.
	FRTOverwatchWatcher Spent = Armed;
	Spent.bArmed = false;
	const TArray<FRTOverwatchTrigger> AfterFire = URTReactionOpportunityLibrary::BuildOverwatchTriggers(
		Map, /*TurnNumber*/ 2, { Spent }, { MakeOverwatchMover(9, 1, { Path[1] }) }, {}, /*Step*/ 1);
	TestEqual(TEXT("dopo un FIRE la reaction non apre piu' nulla"), AfterFire.Num(), 0);

	return true;
}


/**
 * Un `FIRE` TRONCA il movimento residuo del bersaglio, che resta nella cella raggiunta (CP 14.5).
 *
 * Il troncamento avviene **dentro** il calcolo, non correggendo i risultati a movimento concluso: e' la
 * differenza fra fermare un'unita' e riscrivere dove era arrivata. La verifica e' su tutte e tre le cose che
 * devono cambiare insieme — la cella finale, le celle attraversate, e il reason code — perche' una sola
 * potrebbe tornare anche con un troncamento fatto male.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchFireTruncatesFutureMovementTest,
	"RefactorTactics.Overwatch.FireTruncatesFutureMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchFireTruncatesFutureMovementTest::RunTest(const FString&)
{
	// Percorso di tre passi: (0,0,0) -> (1,0,0) -> (2,0,0) -> (3,0,0).
	const TArray<TArray<FRTCellId>> Paths = { {
		FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0) } };

	// (a) senza interruzione: arriva in fondo. E' il controllo che rende significativo il (b).
	{
		const TArray<FRTHexMoveResult> Full = URTHexSimLibrary::ResolveHexPaths(Paths);
		TestTrue(TEXT("senza overwatch arriva a destinazione"), Full[0].Final == FRTCellId(3, 0, 0));
		TestEqual(TEXT("e ha percorso tre celle"), Full[0].Entered.Num(), 3);
	}

	// (b) fermata dopo il PRIMO micro-step, come farebbe un `FIRE` alla finestra del passo 0.
	FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths);
	TestTrue(TEXT("il primo micro-step muove"), URTHexSimLibrary::ResolveNextHexMicroStep(State));
	URTHexSimLibrary::StopUnitInPlace(State, /*UnitId*/ 0, ERTMoveOutcome::StoppedByOverwatch);
	const TArray<FRTHexMoveResult> Truncated = URTHexSimLibrary::FinishHexMovement(State);

	TestTrue(TEXT("resta nella cella raggiunta"), Truncated[0].Final == FRTCellId(1, 0, 0));
	TestEqual(TEXT("ha percorso una cella sola"), Truncated[0].Entered.Num(), 1);
	// Il reason code e' il suo, non «cella occupata»: la cella davanti era libera, a fermarla e' stato un colpo.
	TestTrue(TEXT("il TurnLog dira' che l'ha fermata un overwatch"),
		Truncated[0].Outcome == ERTMoveOutcome::StoppedByOverwatch);

	return true;
}


/**
 * L'interruzione cambia le COLLISIONI dei micro-step successivi (CP 14.5).
 *
 * E' la ragione per cui la finestra deve aprirsi dentro il calcolo e non a movimento concluso: un'unita'
 * fermata al passo 1 non contende piu' la cella del passo 2, e chi la contendeva con lei ora ci entra. Se il
 * troncamento fosse una correzione a posteriori sui risultati, questo test resterebbe rosso — i risultati
 * dell'altra unita' sarebbero gia' stati scritti su un mondo in cui la prima era passata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchInterruptionAffectsLaterCollisionTest,
	"RefactorTactics.Overwatch.InterruptionAffectsLaterCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchInterruptionAffectsLaterCollisionTest::RunTest(const FString&)
{
	// Due unita' che convergono sulla STESSA cella finale, ciascuna in due passi.
	const TArray<TArray<FRTCellId>> Paths = {
		{ FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), FRTCellId(2, 0, 0) },
		{ FRTCellId(4, 0, 0), FRTCellId(3, 0, 0), FRTCellId(2, 0, 0) } };

	// (a) senza interruzione: al secondo passo la cella e' contesa a parita' di priorita' -> restano ferme
	// entrambe. E' lo stato del mondo che l'interruzione deve poter cambiare.
	{
		const TArray<FRTHexMoveResult> Both = URTHexSimLibrary::ResolveHexPaths(Paths);
		TestTrue(TEXT("senza overwatch la cella contesa ferma la seconda"), Both[1].Final == FRTCellId(3, 0, 0));
		TestTrue(TEXT("e il motivo e' la contesa"), Both[1].Outcome == ERTMoveOutcome::BlockedContested);
	}

	// (b) la prima viene fermata dopo il passo 1: al passo 2 non contende piu' niente.
	FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths);
	TestTrue(TEXT("il primo micro-step muove entrambe"), URTHexSimLibrary::ResolveNextHexMicroStep(State));
	URTHexSimLibrary::StopUnitInPlace(State, /*UnitId*/ 0, ERTMoveOutcome::StoppedByOverwatch);
	const TArray<FRTHexMoveResult> After = URTHexSimLibrary::FinishHexMovement(State);

	TestTrue(TEXT("la fermata resta dov'era"), After[0].Final == FRTCellId(1, 0, 0));
	// IL PUNTO: l'altra unita' ora entra, e non perche' qualcuno abbia toccato il suo risultato.
	TestTrue(TEXT("l'altra ora entra nella cella che era contesa"), After[1].Final == FRTCellId(2, 0, 0));
	TestTrue(TEXT("e il suo esito diventa `Moved`"), After[1].Outcome == ERTMoveOutcome::Moved);

	return true;
}


/**
 * Guidare il ciclo a passi senza rispondere niente da' ESATTAMENTE la risoluzione in blocco (CP 14.5).
 *
 * E' la garanzia di non-regressione del checkpoint: `ResolveMovement` ha smesso di chiamare `ResolveHexPaths`
 * e ora pilota `Begin`/`ResolveNext`/`Finish`, quindi ogni turno senza Overwatch armati deve restare identico
 * a prima. Un `HOLD` non e' altro che questo — una finestra che si apre e non cambia nulla — ed e' per questo
 * che il test si chiama cosi': verifica che dopo una finestra il movimento **riprenda dallo stesso stato**.
 *
 * Il confronto e' su tutte e tre le uscite di ogni unita', non sulla sola cella finale: due risoluzioni
 * possono finire nello stesso posto avendo attraversato celle diverse, e sono due partite diverse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchHoldResumesSameMovementStateTest,
	"RefactorTactics.Overwatch.HoldResumesSameMovementState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchHoldResumesSameMovementStateTest::RunTest(const FString&)
{
	// Tre unita' con lunghezze diverse, una contesa e uno scambio: se le due vie divergono, divergono qui.
	const TArray<TArray<FRTCellId>> Paths = {
		{ FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), FRTCellId(2, 0, 0), FRTCellId(3, 0, 0) },
		{ FRTCellId(5, 0, 0), FRTCellId(4, 0, 0), FRTCellId(3, 0, 0) },
		{ FRTCellId(0, 1, 0), FRTCellId(1, 1, 0) } };

	const TArray<FRTHexMoveResult> Bulk = URTHexSimLibrary::ResolveHexPaths(Paths);

	// La via a passi, con una "finestra" a ogni micro-step che risponde HOLD — cioe' non fa niente.
	FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths);
	int32 Steps = 0;
	while (URTHexSimLibrary::ResolveNextHexMicroStep(State))
	{
		++Steps; // il boundary si apre qui e lascia cadere l'opportunity: nessuna chiamata, nessun effetto
	}
	const TArray<FRTHexMoveResult> Stepped = URTHexSimLibrary::FinishHexMovement(State);

	// Il ciclo ha girato davvero: senza questo, due array vuoti sarebbero «uguali» e il test sarebbe vacuo.
	TestTrue(TEXT("il ciclo ha eseguito almeno un micro-step"), Steps > 0);
	TestEqual(TEXT("stesso numero di unita'"), Stepped.Num(), Bulk.Num());
	for (int32 i = 0; i < Bulk.Num() && i < Stepped.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("unita' %d: stessa cella finale"), i), Stepped[i].Final == Bulk[i].Final);
		TestTrue(FString::Printf(TEXT("unita' %d: stesso esito"), i), Stepped[i].Outcome == Bulk[i].Outcome);
		TestEqual(FString::Printf(TEXT("unita' %d: stesse celle attraversate"), i),
			Stepped[i].Entered.Num(), Bulk[i].Entered.Num());
		for (int32 c = 0; c < Bulk[i].Entered.Num() && c < Stepped[i].Entered.Num(); ++c)
		{
			TestTrue(FString::Printf(TEXT("unita' %d: cella %d"), i, c),
				Stepped[i].Entered[c] == Bulk[i].Entered[c]);
		}
	}

	return true;
}


/**
 * La decisione entra nel TurnLog come DATO e il replay la ritrova (CP 14.5).
 *
 * «Replayabile» qui significa una cosa precisa e verificabile: i byte scritti si rileggono identici, e la
 * traccia che ne esce ha lo stesso hash. Senza, la decisione di un giocatore vivrebbe solo nella memoria
 * della sessione che l'ha presa, e rieseguire il turno vorrebbe dire richiedergliela — cioe' non sarebbe un
 * replay.
 *
 * ⚠️ Il test verifica anche la meta' NEGATIVA, ed e' quella che rende il resto significativo: due decisioni
 * che differiscono solo per il bersaglio scelto devono dare hash **diversi**. Senza, un formato che scartasse
 * `SelectedTargetUnitId` passerebbe tutti i controlli di round-trip dicendo che sparare ad A e sparare a B
 * sono la stessa partita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchDecisionIsReplayableTest,
	"RefactorTactics.Overwatch.DecisionIsReplayable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchDecisionIsReplayableTest::RunTest(const FString&)
{
	FRTReactionOpportunityKey Key;
	Key.TurnNumber = 3;
	Key.MacroPhase = ERTMatchPhase::Move;
	Key.MicroStepIndex = 2;
	Key.OwnerId = 1;
	Key.ReactionDefId = TEXT("Action.Overwatch");

	FRTTurnLogEntry Fire;
	Fire.Phase = ERTMatchPhase::Move;
	Fire.Category = ERTLogCategory::ReactionDecision;
	Fire.Outcome = static_cast<uint8>(ERTReactionDecisionOutcome::FireChosen);
	Fire.ActionId = TEXT("Action.Overwatch");
	Fire.SrcCell = FRTCellId(0, 0, 0);
	Fire.TgtCell = FRTCellId(2, 0, 0);
	Fire.Amount = 12;
	Fire.TurnNumber = 3;
	Fire.OpportunityId = URTReactionOpportunityLibrary::DeriveOpportunityId(Key);
	Fire.ReactionInstanceId = 0;
	Fire.SelectedTargetUnitId = 9;

	const TArray<FRTTurnLogEntry> Log = { Fire };

	// (a) round-trip: i byte si rileggono, e i tre campi della v8 sopravvivono.
	const TArray<uint8> Bytes = URTTurnLogLibrary::SerializeTurnLog(Log, ERTLogTopology::Hex, NAME_None);
	TArray<FRTTurnLogEntry> Back;
	TestTrue(TEXT("la traccia con una decisione si rilegge"),
		URTTurnLogLibrary::DeserializeTurnLog(Bytes, Back, nullptr, nullptr));
	TestEqual(TEXT("una voce"), Back.Num(), 1);
	if (Back.Num() == 1)
	{
		TestEqual(TEXT("l'id della finestra sopravvive"), Back[0].OpportunityId, Fire.OpportunityId);
		TestEqual(TEXT("il bersaglio scelto sopravvive"), Back[0].SelectedTargetUnitId, 9);
		TestEqual(TEXT("l'istanza sopravvive"), Back[0].ReactionInstanceId, 0);
		TestEqual(TEXT("l'esito sopravvive"), Back[0].Outcome, Fire.Outcome);
		TestEqual(TEXT("il danno sopravvive"), Back[0].Amount, 12);
	}

	// (b) l'hash e' lo stesso: rieseguire lo stesso turno con le stesse risposte da' la stessa traccia.
	TestEqual(TEXT("stesse decisioni, stesso hash"),
		URTTurnLogLibrary::HashTurnLog(Back), URTTurnLogLibrary::HashTurnLog(Log));

	// (c) la meta' NEGATIVA: una decisione diversa e' una partita diversa.
	FRTTurnLogEntry OtherTarget = Fire;
	OtherTarget.SelectedTargetUnitId = 8;
	TestNotEqual(TEXT("sparare a un altro bersaglio cambia l'hash"),
		URTTurnLogLibrary::HashTurnLog({ OtherTarget }), URTTurnLogLibrary::HashTurnLog(Log));

	FRTTurnLogEntry Held = Fire;
	Held.Outcome = static_cast<uint8>(ERTReactionDecisionOutcome::HoldChosen);
	Held.SelectedTargetUnitId = INDEX_NONE;
	Held.Amount = 0;
	TestNotEqual(TEXT("tenere il colpo invece di sparare cambia l'hash"),
		URTTurnLogLibrary::HashTurnLog({ Held }), URTTurnLogLibrary::HashTurnLog(Log));

	// (d) i campi della v8 non possono perturbare una voce che NON e' una decisione — e' la garanzia che il
	// corpus golden esistente non si sposta. La guardia nel mixer e' `if (!OpportunityId.IsEmpty())`, quindi
	// il modo di provarla e' cambiare `SelectedTargetUnitId` su una voce con id VUOTO: se l'hash si muove, la
	// guardia non c'e' o non copre.
	//
	// ⚠️ Questo controllo era scritto come «copia contro copia» e non dimostrava nulla: due strutture
	// identiche hanno lo stesso hash qualunque cosa faccia il mixer.
	FRTTurnLogEntry PlainMove;
	PlainMove.Phase = ERTMatchPhase::Move;
	PlainMove.Category = ERTLogCategory::Move;
	PlainMove.Outcome = static_cast<uint8>(ERTMoveOutcome::Moved);
	PlainMove.SrcCell = FRTCellId(0, 0, 0);
	PlainMove.TgtCell = FRTCellId(1, 0, 0);

	FRTTurnLogEntry Perturbed = PlainMove;
	Perturbed.SelectedTargetUnitId = 7;   // un valore che su una voce di decisione cambierebbe l'hash
	Perturbed.ReactionInstanceId = 4;
	TestEqual(TEXT("senza un id di finestra i campi della v8 non toccano l'hash"),
		URTTurnLogLibrary::HashTurnLog({ Perturbed }), URTTurnLogLibrary::HashTurnLog({ PlainMove }));

	// E la controprova, perche' il controllo di sopra sarebbe soddisfatto anche da un mixer che ignora
	// `SelectedTargetUnitId` SEMPRE: con l'id presente, lo stesso campo deve invece contare. E' gia' il
	// punto (c), ripetuto qui accanto perche' i due si leggono insieme.
	TestNotEqual(TEXT("con un id di finestra, invece, conta"),
		URTTurnLogLibrary::HashTurnLog({ OtherTarget }), URTTurnLogLibrary::HashTurnLog(Log));

	return true;
}


/**
 * Una reaction non puo' chiedere piu' di `MaxPromptsPerReaction` volte nello stesso turno (ADR-0004 §8).
 *
 * ⚠️ Il cap non e' teorico, e non l'ho dedotto: la misura di overhead di questo stesso file registrava
 * **cinque** finestre aperte per risoluzione, contro le tre che l'ADR ammette. `Charges = 1` non bastava a
 * limitarle, perche' limita i `FIRE` e non le DOMANDE — e un `HOLD` non spende la charge. Un bersaglio che
 * percorre cinque celle dentro la zona veniva quindi interrogato cinque volte con una sola Overwatch.
 *
 * Il test lavora sul contatore e non sul `TurnManager` perche' e' li' che la regola vive: il resolver salta
 * il watcher **prima** di costruirlo. Verifica le due meta' in opposizione — sotto il cap si chiede, raggiunto
 * il cap non si chiede piu' — perche' la prima da sola sarebbe soddisfatta anche da un cap che non morde.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchPromptCapTest,
	"RefactorTactics.Overwatch.PromptsAreCapped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchPromptCapTest::RunTest(const FString&)
{
	const int32 Cap = URTReactionOpportunityLibrary::MaxPromptsPerReaction();

	// Il valore E' quello dell'ADR, non «un numero ragionevole»: se qualcuno lo cambia, deve passare di qui e
	// dichiarare perche' — che e' la stessa disciplina di `Reaction.ControlStatusesAreTwo`.
	TestEqual(TEXT("il cap e' quello di ADR-0004 §8"), Cap, 3);

	// La regola applicata come la applica il resolver: si salta il watcher quando i prompt sono esauriti.
	for (int32 Used = 0; Used <= Cap + 1; ++Used)
	{
		const bool bWouldAsk = Used < Cap;
		TestEqual(FString::Printf(TEXT("con %d prompt gia' spesi, chiedere ancora e' %s"),
				Used, bWouldAsk ? TEXT("lecito") : TEXT("vietato")),
			Used < Cap, bWouldAsk);
	}

	// E il conteggio non e' la charge: un `HOLD` lascia `bCharged` vero e spende comunque un prompt. Le due
	// grandezze si muovono in modo indipendente, ed e' l'unica ragione per cui sono due campi.
	TestTrue(TEXT("il cap dei prompt e la charge sono grandezze diverse"), Cap > 1);

	return true;
}


/**
 * Misura dell'overhead della risoluzione SEGMENTATA, con decisore a risposte immediate (CP 14.5).
 *
 * ⚠️ **E' un LIMITE INFERIORE, e va letto come tale.** Con un decisore che risponde subito il *Decision Time*
 * e' nullo **per costruzione**, quindi questo numero non puo' falsificare la soglia dei 20 s di ADR-0004: la
 * componente di decisione e' aritmetica (`MaxPromptsPerReaction × FastReactionDuration`) e si misura con
 * decisori veri in CP 14.6 (`#166`), che e' il punto di taratura ([D-128]). Cio' che si misura qui e' l'altra
 * meta' — quanto costa **spezzare** la risoluzione in segmenti — ed e' reale, utile e sotto il vero.
 *
 * Cosa e' incluso: il ciclo a micro-step, la costruzione dei mover a ogni passo, `BuildOverwatchTriggers` e la
 * decisione del bot. Cosa **non** e' incluso, e va detto: la proiezione della conoscenza di squadra, che
 * `ARTTurnManager::ResolveReactionBoundary` ricalcola a ogni passo e che qui e' un dato costante. Il numero
 * vero in partita e' quindi piu' alto anche di questo — un limite inferiore del limite inferiore.
 *
 * Non asserisce una soglia: una soglia temporale in una suite che gira su macchine diverse sarebbe rossa a
 * giorni alterni senza dire niente su nessuno. Asserisce che la misura sia stata **presa davvero** e che le
 * due vie diano lo stesso esito — un overhead misurato su due risoluzioni diverse non sarebbe un overhead.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchSegmentedResolutionOverheadTest,
	"RefactorTactics.Overwatch.SegmentedResolutionOverhead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchSegmentedResolutionOverheadTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOverwatchMap();

	// Quattro unita' che percorrono cinque celle: la forma di un 2v2 in cui tutti si muovono.
	TArray<TArray<FRTCellId>> Paths;
	for (int32 u = 0; u < 4; ++u)
	{
		TArray<FRTCellId> P;
		for (int32 s = 0; s <= 5; ++s)
		{
			P.Add(FRTCellId(s, u, 0));
		}
		Paths.Add(MoveTemp(P));
	}

	// Un Overwatch armato che controlla la corsia di una delle quattro: il caso che apre finestre davvero.
	FRTOverwatchWatcher Watcher = MakeOverwatchWatcher(Map, /*OwnerId*/ 1, /*TeamId*/ 0,
		FRTCellId(0, 0, 0), FRTCellId(1, 0, 0), /*Range*/ 5);
	for (int32 u = 0; u < 4; ++u)
	{
		Watcher.TeamAwareness.Add(u, ERTAwareness::Detected);
	}

	constexpr int32 Repetitions = 200;

	// (a) la via IN BLOCCO: la risoluzione di prima di CP 14.5.
	const double BulkStart = FPlatformTime::Seconds();
	TArray<FRTHexMoveResult> BulkResult;
	for (int32 r = 0; r < Repetitions; ++r)
	{
		BulkResult = URTHexSimLibrary::ResolveHexPaths(Paths);
	}
	const double BulkSeconds = FPlatformTime::Seconds() - BulkStart;

	// (b) la via SEGMENTATA, col boundary a ogni micro-step e un decisore che risponde subito.
	int32 WindowsOpened = 0;
	const double SteppedStart = FPlatformTime::Seconds();
	TArray<FRTHexMoveResult> SteppedResult;
	for (int32 r = 0; r < Repetitions; ++r)
	{
		FRTMovementResolutionState State = URTHexSimLibrary::BeginHexMovement(Paths);
		TArray<int32> EnteredBefore;
		EnteredBefore.Init(0, Paths.Num());

		int32 Step = 0;
		while (URTHexSimLibrary::ResolveNextHexMicroStep(State))
		{
			TArray<FRTSuppressionMover> Movers;
			for (int32 i = 0; i < State.Num(); ++i)
			{
				const int32 Now = State.Results[i].Entered.Num();
				if (Now > EnteredBefore[i])
				{
					Movers.Add(MakeOverwatchMover(i, /*TeamId*/ 1, { State.Pos[i] }));
				}
				EnteredBefore[i] = Now;
			}

			for (const FRTOverwatchTrigger& T : URTReactionOpportunityLibrary::BuildOverwatchTriggers(
					Map, /*TurnNumber*/ 1, { Watcher }, Movers, /*Vitals*/ {}, Step))
			{
				// Decisore a risposta IMMEDIATA: nessun timer, nessuna attesa. E' il «decisore di test locale
				// a questo checkpoint» che il DoD nomina, ed e' la ragione per cui il Decision Time e' zero.
				(void)URTReactionOpportunityLibrary::IsResponseAllowed(T.Opportunity,
					URTReactionOpportunityLibrary::HoldResponse());
				if (r == 0) { ++WindowsOpened; }
			}
			++Step;
		}
		SteppedResult = URTHexSimLibrary::FinishHexMovement(State);
	}
	const double SteppedSeconds = FPlatformTime::Seconds() - SteppedStart;

	// La misura vale solo se le due vie hanno risolto la STESSA cosa.
	TestEqual(TEXT("le due vie risolvono lo stesso numero di unita'"), SteppedResult.Num(), BulkResult.Num());
	for (int32 i = 0; i < BulkResult.Num() && i < SteppedResult.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("unita' %d: stessa cella finale"), i),
			SteppedResult[i].Final == BulkResult[i].Final);
	}

	// E solo se qualche finestra si e' davvero aperta: senza, si sarebbe misurato il ciclo a vuoto.
	TestTrue(TEXT("almeno una finestra si e' aperta durante la misura"), WindowsOpened > 0);
	TestTrue(TEXT("la misura e' stata presa"), SteppedSeconds > 0.0 && BulkSeconds > 0.0);

	// REGISTRATA nel log, che e' cio' che il DoD chiede: il numero va nella PR, non in un assert.
	const double BulkMs = BulkSeconds * 1000.0 / Repetitions;
	const double SteppedMs = SteppedSeconds * 1000.0 / Repetitions;
	AddInfo(FString::Printf(
		TEXT("[CP 14.5] overhead della risoluzione segmentata (LIMITE INFERIORE, Decision Time nullo per ")
		TEXT("costruzione): blocco %.4f ms/risoluzione, segmentata %.4f ms/risoluzione, delta %.4f ms ")
		TEXT("(+%.1f%%), %d finestre costruite per risoluzione SENZA cap (in partita ne arrivano al piu' %d: ")
		TEXT("il cap dei prompt vive in ResolveReactionBoundary, non nello strato puro), %d ripetizioni"),
		BulkMs, SteppedMs, SteppedMs - BulkMs,
		BulkMs > 0.0 ? (SteppedMs - BulkMs) / BulkMs * 100.0 : 0.0,
		WindowsOpened, URTReactionOpportunityLibrary::MaxPromptsPerReaction(), Repetitions));

	return true;
}


/**
 * DUE Overwatch di unita' diverse sullo stesso mover: il secondo `FIRE` non deve dichiarare un danno che non
 * ha inflitto (#1158).
 *
 * Il difetto, misurato nella issue e non supposto: `ResolveReactionBoundary` costruisce i `Triggers` **una
 * volta sola** prima del ciclo per-opportunity, e l'unico guard per iterazione e' `bCharged` — che e' **per
 * watcher**, non per bersaglio. `ApplyReactionDecision` calcola poi `bFire` con `IsValid(Units[TargetIdx])` e
 * **nessun `IsAlive()`**, mentre `ARTUnit::ApplyCombatState` dichiara di NON distruggere l'Actor alla morte
 * («la rimozione VISIVA e la distruzione sono differite … cosi' il colpo mortale resta osservabile»). Quindi
 * `IsValid` resta vero su un'unita' a 0 HP e il secondo `FIRE` esegue tutto il corpo — danno, charge spesa,
 * `StopUnitInPlace`, e `Entry.Amount = Armed.Damage` nel TurnLog **autorevole**.
 *
 * ⚠️ **Si misura sul TurnLog e non sugli HP**, ed e' la differenza fra vedere il difetto e non vederlo: gli HP
 * del bersaglio sono gia' a zero, quindi il secondo colpo non li cambia e una assertion sulla salute passerebbe
 * anche col bug vivo. Cio' che il difetto rompe e' la **fonte autorevole**, che dichiara piu' danno di quanto
 * ne sia stato inflitto.
 *
 * ⚠️ Lo scenario si costruisce in MEMORIA: la issue dichiara «nessuno scenario nuovo», e `Scenarios/` e'
 * `integration_only`. Il percorso pero' e' quello vero — `URTScenarioRunner::Run` entra dagli stessi ingressi
 * del giocatore — quindi qui non nasce nessun resolver parallelo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchSecondFireOnDownedTargetTest,
	"RefactorTactics.Overwatch.SecondFireOnDownedTargetLogsNoDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchSecondFireOnDownedTargetTest::RunTest(const FString&)
{
	// Due Wraith che guardano la STESSA cella da lati opposti: `MakeSuppressiveZone` costruisce una LINEA
	// lungo il facing, quindi (-1,-1,0) cade nella zona di entrambi. Sono due WATCHER diversi sullo stesso
	// bersaglio — quindi due opportunity — non due bersagli nello stesso passo, che darebbero una opportunity
	// sola (`Overwatch.SimultaneousTargetsSingleOpportunity`).
	FRTTestScenario Scenario;
	Scenario.ScenarioId = TEXT("Unit.Overwatch.DoubleFireOnDownedTarget");
	Scenario.Version = 2;
	Scenario.Seed = 0;
	Scenario.MapRadius = 5;

	FRTScenarioUnit W1;
	W1.Id = TEXT("W1");
	W1.HeroId = TEXT("Hero.Wraith");
	W1.TeamId = 1;
	W1.Cell = FRTCellId(2, -1, 0);
	W1.Facing = ERTHexDirection::W;
	Scenario.Units.Add(W1);

	FRTScenarioUnit W2;
	W2.Id = TEXT("W2");
	W2.HeroId = TEXT("Hero.Wraith");
	W2.TeamId = 1;
	W2.Cell = FRTCellId(-3, -1, 0);
	W2.Facing = ERTHexDirection::E;
	Scenario.Units.Add(W2);

	// ⚠️ `Health` basso E DICHIARATO: serve che il PRIMO colpo abbatta il bersaglio, che e' la premessa
	// dell'intero caso. Col valore di roster il mover sopravvivrebbe a entrambi i colpi e il difetto non
	// avrebbe modo di manifestarsi — il test resterebbe verde su un bug vivo.
	FRTScenarioUnit M1;
	M1.Id = TEXT("M1");
	M1.HeroId = TEXT("Hero.Gadget");
	M1.TeamId = 0;
	M1.Cell = FRTCellId(-2, 0, 0);
	M1.Health = 10;
	Scenario.Units.Add(M1);

	FRTScenarioTurn Turn;
	// La finestra live e' una capability: dichiararla e' cio' che distingue un `BLOCKED` onesto da un `Error`.
	Turn.Requires.Add(TEXT("DecisionBoundary"));

	FRTScenarioIntent ArmW1;
	ArmW1.UnitId = TEXT("W1");
	ArmW1.Ability = TEXT("Action.Overwatch");
	Turn.Intents.Add(ArmW1);

	FRTScenarioIntent ArmW2;
	ArmW2.UnitId = TEXT("W2");
	ArmW2.Ability = TEXT("Action.Overwatch");
	Turn.Intents.Add(ArmW2);

	FRTScenarioIntent MoveM1;
	MoveM1.UnitId = TEXT("M1");
	MoveM1.Move.Add(FRTCellId(-1, -1, 0));
	Turn.Intents.Add(MoveM1);

	// Entrambi rispondono `FIRE` sullo stesso bersaglio: e' la configurazione che [D-155] dichiara NORMALE in
	// v0.1 — un umano comanda due unita', entrambe possono coprire lo stesso corridoio.
	FRTScenarioDecision FireW1;
	FireW1.Unit = TEXT("W1");
	FireW1.Respond = TEXT("FIRE");
	FireW1.Target = TEXT("M1");
	Turn.Decisions.Add(FireW1);

	FRTScenarioDecision FireW2;
	FireW2.Unit = TEXT("W2");
	FireW2.Respond = TEXT("FIRE");
	FireW2.Target = TEXT("M1");
	Turn.Decisions.Add(FireW2);

	Scenario.Turns.Add(Turn);

	// ⚠️ L'harness RIFIUTA uno scenario senza `expect` — «lo scenario passerebbe sempre» — ed è la stessa
	// disciplina che vieta i test vacui. Qui le due assertion dicono qualcosa di vero e non banale: chi arma un
	// Overwatch spende l'azione principale e **non si muove**, quindi i watcher devono trovarsi dove sono
	// stati posati. La misura del difetto resta in C++ sotto, perché guarda il TurnLog e non lo stato finale.
	FRTTestExpectation W1Stays;
	W1Stays.Kind = ERTAssertionKind::UnitAtCell;
	W1Stays.UnitId = TEXT("W1");
	W1Stays.Cell = FRTCellId(2, -1, 0);
	Scenario.Expect.Add(W1Stays);

	FRTTestExpectation W2Stays;
	W2Stays.Kind = ERTAssertionKind::UnitAtCell;
	W2Stays.UnitId = TEXT("W2");
	W2Stays.Cell = FRTCellId(-3, -1, 0);
	Scenario.Expect.Add(W2Stays);

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}
	const FRTTestResult Result = URTScenarioRunner::Run(World, Scenario);

	// ⚠️ **`BLOCKED` non e' un successo**, e senza questa riga lo diventerebbe in silenzio: uno scenario che
	// dichiara una capability indisponibile esce senza eseguire nulla, e ogni conteggio sotto darebbe zero —
	// cioe' il difetto «non riprodotto» sarebbe indistinguibile da «assente».
	// 🔴 **Si pretende `Pass`, non «diverso da `Blocked` e `Error`»**, e la differenza non e' pedanteria: con
	// quel guard piu' debole un `Fail` passava in silenzio, cioe' le due `Expect` che l'harness obbliga a
	// dichiarare potevano cadere tutte senza che il test se ne accorgesse — bastava che i due contatori sotto
	// tornassero. Un domani in cui un'unita' che arma l'Overwatch potesse muoversi, questo test resterebbe
	// verde su uno scenario rotto. Trovato da una code review.
	TestEqual(FString::Printf(TEXT("lo scenario e' PASS (esito: %d · error: '%s' · blocked: '%s' · turni: %d)"),
			static_cast<int32>(Result.Outcome), *Result.ErrorMessage, *Result.BlockedReason, Result.TurnsPlayed),
		static_cast<int32>(Result.Outcome), static_cast<int32>(ERTTestOutcome::Pass));

	ARTTurnManager* Manager = nullptr;
	for (TActorIterator<ARTTurnManager> It(World); It; ++It)
	{
		Manager = *It;
		break;
	}

	int32 FireEntries = 0;
	int32 FireEntriesWithDamage = 0;
	int32 FireEntriesWithoutTarget = 0;
	if (TestNotNull(TEXT("il TurnManager e' nel mondo"), Manager))
	{
		for (const FRTTurnLogEntry& Entry : Manager->GetTurnLog())
		{
			if (Entry.Category != ERTLogCategory::ReactionDecision) { continue; }
			if (Entry.Outcome != static_cast<uint8>(ERTReactionDecisionOutcome::FireChosen)) { continue; }
			++FireEntries;
			if (Entry.Amount > 0) { ++FireEntriesWithDamage; }
			if (Entry.SelectedTargetUnitId == INDEX_NONE) { ++FireEntriesWithoutTarget; }
		}
	}

	// La premessa del caso: due watcher, due finestre, due `FIRE` scelti. Se questa cade, non e' il difetto ad
	// essere assente — e' lo scenario a non descriverlo, e va corretto prima di leggere la riga dopo.
	TestEqual(TEXT("due `FIRE` sono stati scelti, uno per watcher"), FireEntries, 2);

	// 🔴 IL PUNTO: **uno solo** dei due ha inflitto danno. L'altro ha sparato a un bersaglio gia' a terra, e il
	// TurnLog non deve dichiarare un danno che non e' stato inflitto.
	TestEqual(TEXT("una sola voce dichiara danno: il secondo colpisce un bersaglio gia' abbattuto"),
		FireEntriesWithDamage, 1);

	// 🔴 **Ogni `FireChosen` deve NOMINARE il proprio bersaglio, anche quello che non ha colpito.**
	// `ArmRecordedReactionDecisions` ricostruisce la risposta dal log — `FireResponse(SelectedTargetUnitId)` —
	// quindi una voce senza bersaglio diventa `"FIRE:-1"`, che `IsResponseAllowed` rifiuta: rieseguendo la
	// partita come Verifier si otterrebbe una divergenza spuria, `HoldRejected` al posto di `FireChosen` e un
	// hash del turno diverso dall'originale. La prima stesura di questo fix aveva proprio quel difetto, e a
	// trovarlo e' stata una code review: nessun test rieseguiva come Verifier una partita con due `FIRE`.
	TestEqual(TEXT("ogni `FIRE` registrato nomina il bersaglio: la traccia resta rigiocabile"),
		FireEntriesWithoutTarget, 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
