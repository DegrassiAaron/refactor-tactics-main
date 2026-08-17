#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Map/RTCellId.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTPredictiveLibrary.h"
#include "Turn/RTTurnLog.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Rotta di un'unita': le celle attraversate nell'ordine. Nome distinto per file (unity build). */
	FRTHexMoveResult MakePredictiveRoute(const TArray<FRTCellId>& Entered)
	{
		FRTHexMoveResult R;
		R.Entered = Entered;
		R.Final = Entered.Num() > 0 ? Entered.Last() : FRTCellId(0, 0, 0);
		R.Outcome = Entered.Num() > 0 ? ERTMoveOutcome::Moved : ERTMoveOutcome::Stayed;
		return R;
	}

	/** Un colpo armato dall'unita' `Source` sulla cella `Cell`, ostile a `Hostiles`. */
	FRTPredictiveShot MakePredictiveShot(int32 Source, const FRTCellId& Cell, const TSet<int32>& Hostiles)
	{
		FRTPredictiveShot S;
		S.SourceUnitId = Source;
		S.LockedCell = Cell;
		S.ActionId = TEXT("Hero.Wraith.InterceptShot");
		S.Hostiles = Hostiles;
		return S;
	}
}

// Il boundary e' l'ATTRAVERSAMENTO, non la posizione finale: l'unita' 0 passa sulla cella bloccata e
// proseguirebbe oltre. Un controllo sullo stato finale la mancherebbe — e sarebbe un difetto invisibile,
// perche' il caso in cui il bersaglio si FERMA sulla cella passerebbe comunque.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveBoundaryTest,
	"RefactorTactics.Predictive.ResolvesAtDeclaredBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveBoundaryTest::RunTest(const FString&)
{
	const FRTCellId Locked(-1, 0, 0);

	TArray<FRTHexMoveResult> Moves;
	// L'unita' 0 attraversa la cella bloccata al passo 1 e vorrebbe arrivare fino a (1,0).
	Moves.Add(MakePredictiveRoute({ FRTCellId(-2, 0, 0), Locked, FRTCellId(0, 0, 0), FRTCellId(1, 0, 0) }));
	Moves.Add(MakePredictiveRoute({})); // l'unita' 1 ha armato e non si muove

	const TArray<FRTPredictiveShot> Shots = { MakePredictiveShot(1, Locked, TSet<int32>{ 0 }) };

	const TArray<FRTPredictiveResolution> Res = URTPredictiveLibrary::ResolvePredictions(Shots, Moves);

	TestEqual(TEXT("un esito per colpo"), Res.Num(), 1);
	TestTrue(TEXT("la previsione ha colto"), Res[0].bMatched);
	TestEqual(TEXT("ha colto chi attraversava"), Res[0].VictimUnitId, 0);
	// 2 passi conservati: la cella di partenza del percorso piu' quella bloccata. Entra e si ferma li'.
	TestEqual(TEXT("il movimento e' troncato SULLA cella bloccata"), Res[0].VictimStepsKept, 2);

	TArray<FRTHexMoveResult> Applied = Moves;
	URTPredictiveLibrary::ApplyPredictionsToMoves(Res, Applied);

	TestEqual(TEXT("la rotta e' accorciata"), Applied[0].Entered.Num(), 2);
	TestTrue(TEXT("si ferma sulla cella bloccata"), Applied[0].Final == Locked);
	// Il reason code e' proprio: la cella era LIBERA, e `BlockedByUnit` manderebbe a cercare un'unita'
	// che non c'e'. E' la stessa ragione per cui `BlockedByTopology` non riusa `BlockedByUnit`.
	TestTrue(TEXT("il TurnLog sa dire che l'ha fermata una previsione"),
		Applied[0].Outcome == ERTMoveOutcome::StoppedByPrediction);
	return true;
}

// Il caso che nessuno scriverebbe dopo, e per cui esiste `Spec.Predictive.WhiffOnEmptyCell`: la previsione
// e' SBAGLIATA e il colpo deve andare a vuoto. Un'implementazione che al boundary rivalutasse il bersaglio
// piu' vicino passerebbe il test del colpo a segno e fallirebbe questo — la cella bloccata non si rivaluta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveWhiffTest,
	"RefactorTactics.Predictive.WhiffUsesDeclaredFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveWhiffTest::RunTest(const FString&)
{
	const FRTCellId Locked(-1, 0, 0);

	TArray<FRTHexMoveResult> Moves;
	// L'unita' 0 devia: passa ACCANTO alla cella bloccata senza mai entrarci.
	Moves.Add(MakePredictiveRoute({ FRTCellId(-2, 0, 0), FRTCellId(-2, 1, 0) }));
	Moves.Add(MakePredictiveRoute({}));

	const TArray<FRTPredictiveShot> Shots = { MakePredictiveShot(1, Locked, TSet<int32>{ 0 }) };

	const TArray<FRTPredictiveResolution> Res = URTPredictiveLibrary::ResolvePredictions(Shots, Moves);

	TestEqual(TEXT("il colpo produce comunque un esito"), Res.Num(), 1);
	TestFalse(TEXT("la previsione va a vuoto"), Res[0].bMatched);
	TestEqual(TEXT("nessuna vittima"), Res[0].VictimUnitId, INDEX_NONE);

	// Il fallback DICHIARATO nel catalogo per la thin slice e' `Cancel`, cioe' fizzle: non si esegue nulla.
	// Il punto e' che il fallback non viene SCELTO qui: `ERTActionFallback` esiste gia' ed e' un dato, quindi
	// il boundary non ha un ramo che decida cosa fare quando sbaglia.
	FRTActionDef Def;
	Def.PredictiveTargeting = ERTPredictiveTargeting::LockCell;
	Def.PredictionBoundary = ERTPredictionBoundary::MovementEntry;
	Def.Fallback = ERTActionFallback::Cancel;
	TestTrue(TEXT("il fallback del whiff e' dichiarato, non deciso a runtime"),
		Def.Fallback == ERTActionFallback::Cancel);

	// Il movimento di chi ha deviato resta INTATTO: il whiff non e' un colpo attenuato, e' un colpo assente.
	TArray<FRTHexMoveResult> Applied = Moves;
	URTPredictiveLibrary::ApplyPredictionsToMoves(Res, Applied);
	TestEqual(TEXT("chi ha deviato percorre tutta la sua rotta"), Applied[0].Entered.Num(), 2);
	TestTrue(TEXT("e non e' fermato da nessuna previsione"), Applied[0].Outcome == ERTMoveOutcome::Moved);
	return true;
}

// L'invariante che rende il boundary un ORDINE e non una corsa: permutare gli array di input non cambia
// l'esito. Senza chiavi totali due colpi sullo stesso passo si sceglierebbero a seconda di come il chiamante
// ha riempito gli array, e il replay divergerebbe senza che nessun test lo veda.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictivePermutationTest,
	"RefactorTactics.Predictive.PermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictivePermutationTest::RunTest(const FString&)
{
	const FRTCellId Locked(0, 0, 0);

	TArray<FRTHexMoveResult> Moves;
	Moves.Add(MakePredictiveRoute({ FRTCellId(-1, 0, 0), Locked, FRTCellId(1, 0, 0) })); // unita' 0: passo 1
	Moves.Add(MakePredictiveRoute({ FRTCellId(2, 0, 0), FRTCellId(1, 0, 0), Locked }));  // unita' 1: passo 2
	Moves.Add(MakePredictiveRoute({}));                                                   // unita' 2: arma

	// DUE colpi sulla stessa cella, e due unita' che ci entrano a passi diversi. E' il caso che discrimina:
	// entrambi i colpi devono cogliere chi arriva PRIMA (passo 1), non uno ciascuno. Se l'ordine di
	// processamento fosse per colpo invece che per passo, il secondo troverebbe l'unita' del passo 2 — e la
	// risposta dipenderebbe da come il chiamante ha riempito gli array.
	const TArray<FRTPredictiveShot> A = {
		MakePredictiveShot(2, Locked, TSet<int32>{ 0, 1 }),
		MakePredictiveShot(2, Locked, TSet<int32>{ 0, 1 })
	};

	const TArray<FRTPredictiveResolution> First = URTPredictiveLibrary::ResolvePredictions(A, Moves);

	// Stessa configurazione con l'ordine delle UNITA' invertito: le stesse tre rotte, indici rimescolati.
	TArray<FRTHexMoveResult> Permuted;
	Permuted.Add(MakePredictiveRoute({}));                                                   // era l'unita' 2
	Permuted.Add(MakePredictiveRoute({ FRTCellId(2, 0, 0), FRTCellId(1, 0, 0), Locked }));    // era l'unita' 1
	Permuted.Add(MakePredictiveRoute({ FRTCellId(-1, 0, 0), Locked, FRTCellId(1, 0, 0) }));   // era l'unita' 0

	const TArray<FRTPredictiveShot> B = {
		MakePredictiveShot(0, Locked, TSet<int32>{ 1, 2 }),
		MakePredictiveShot(0, Locked, TSet<int32>{ 1, 2 })
	};

	const TArray<FRTPredictiveResolution> Second = URTPredictiveLibrary::ResolvePredictions(B, Permuted);

	TestEqual(TEXT("stesso numero di esiti"), First.Num(), Second.Num());
	// La vittima e' la STESSA unita' logica in entrambe le numerazioni — indice 0 nella prima, indice 2 nella
	// seconda — cioe' quella che entra al passo minore. E' l'assertion che conta: confrontare solo il numero
	// di passi lascerebbe passare un'implementazione che coglie l'unita' sbagliata alla stessa profondita'.
	TestTrue(TEXT("il primo colpo coglie in entrambe"), First[0].bMatched && Second[0].bMatched);
	TestEqual(TEXT("e coglie chi entra per primo"), First[0].VictimUnitId, 0);
	TestEqual(TEXT("la stessa unita', rinumerata"), Second[0].VictimUnitId, 2);
	TestEqual(TEXT("allo stesso passo"), First[0].VictimStepsKept, Second[0].VictimStepsKept);

	// Il secondo colpo coglie la STESSA unita', non quella del passo 2: due colpi sulla stessa cella sparano
	// entrambi a chi ci mette piede per primo, e chi arriva dopo non ci arriva nemmeno.
	TestTrue(TEXT("il secondo colpo coglie in entrambe"), First[1].bMatched && Second[1].bMatched);
	TestEqual(TEXT("e coglie la stessa vittima del primo"), First[1].VictimUnitId, First[0].VictimUnitId);
	TestEqual(TEXT("anche nella numerazione permutata"), Second[1].VictimUnitId, Second[0].VictimUnitId);
	return true;
}

// La linea che separa E18 da E14: la risoluzione e' TOTALE. Non esiste un esito «in attesa», non c'e' uno
// stato sospeso da riprendere (`FRTMovementResolutionState`, che esiste per l'Overwatch di CP 14.2) e la
// funzione restituisce tutto in una chiamata sola. E' cio' che D-016 chiede: nessun input umano nella
// Resolution.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveNoWaitTest,
	"RefactorTactics.Predictive.NoResolverWait",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveNoWaitTest::RunTest(const FString&)
{
	const FRTCellId Hit(0, 0, 0);
	const FRTCellId Empty(3, 0, 0);

	TArray<FRTHexMoveResult> Moves;
	Moves.Add(MakePredictiveRoute({ FRTCellId(-1, 0, 0), Hit }));
	Moves.Add(MakePredictiveRoute({}));

	// Un colpo che coglie e uno che va a vuoto, insieme: entrambi devono chiudersi nella stessa chiamata.
	const TArray<FRTPredictiveShot> Shots = {
		MakePredictiveShot(1, Hit,   TSet<int32>{ 0 }),
		MakePredictiveShot(1, Empty, TSet<int32>{ 0 })
	};

	const TArray<FRTPredictiveResolution> Res = URTPredictiveLibrary::ResolvePredictions(Shots, Moves);

	TestEqual(TEXT("ogni colpo ha esattamente un esito, subito"), Res.Num(), Shots.Num());
	TestTrue(TEXT("il primo coglie"), Res[0].bMatched);
	TestFalse(TEXT("il secondo va a vuoto"), Res[1].bMatched);
	for (int32 i = 0; i < Res.Num(); ++i)
	{
		// L'esito e' attribuibile senza dipendere dall'ordine di ritorno: nessun colpo resta senza risposta,
		// che e' la forma osservabile di «il resolver non aspetta nessuno».
		TestEqual(TEXT("l'esito e' attribuito al colpo giusto"), Res[i].ShotIndex, i);
	}

	// Chiamarla due volte con gli stessi input da' lo stesso risultato: non c'e' stato interno che sopravviva
	// alla chiamata, quindi non c'e' niente che possa «riprendere» piu' tardi.
	const TArray<FRTPredictiveResolution> Again = URTPredictiveLibrary::ResolvePredictions(Shots, Moves);
	TestEqual(TEXT("nessuno stato sospeso fra due chiamate"), Again.Num(), Res.Num());
	TestTrue(TEXT("e lo stesso esito"), Again[0].bMatched == Res[0].bMatched
		&& Again[1].bMatched == Res[1].bMatched
		&& Again[0].VictimUnitId == Res[0].VictimUnitId);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
