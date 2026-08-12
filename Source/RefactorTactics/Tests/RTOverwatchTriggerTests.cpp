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
// Cio' che questi test NON coprono, e va detto: la finestra di 3,0 s, il commit, il troncamento del movimento
// e il cablaggio di `Vektor.InterceptShot` sono CP 14.5. Qui si produce l'opportunity, non la si risolve.

#include "Misc/AutomationTest.h"
#include "Combat/RTOffensiveActionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Perception/RTPerceptionLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Turn/RTTurnRules.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
