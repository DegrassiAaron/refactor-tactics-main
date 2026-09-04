#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTMapVisuals.h"
#include "Map/RTStructuralBodyLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mappa vuota in memoria: il derivatore e' puro, non serve un mondo. */
	URTHexMapAsset* MakeBodyMap()
	{
		return NewObject<URTHexMapAsset>(GetTransientPackage());
	}

	void PutCell(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexBodyFill Fill, int32 Height = 0)
	{
		FRTHexCellData Cell(Id);
		Cell.BodyFill = Fill;
		Cell.Height = Height;
		Map->AddOrUpdateCell(Cell);
		Map->SortCells();
	}

	const FRTStructuralBody* Find(const TArray<FRTStructuralBody>& Bodies, const FRTCellId& Id)
	{
		return Bodies.FindByPredicate([&Id](const FRTStructuralBody& B) { return B.Cell == Id; });
	}
}

/**
 * 🔑 **La frazione dichiarata dall'autore diventa un corpo, e sopra il vuoto non viene troncata.**
 *
 * ➕ Il controllo positivo e' l'altezza, non la presenza: un derivatore che producesse un corpo di spessore
 * arbitrario passerebbe un test che si limita a contare gli elementi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBodyFullOverVoidTest,
	"RefactorTactics.StructuralBody.FullFillOverVoidIsNotTruncated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBodyFullOverVoidTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeBodyMap();
	if (!TestNotNull(TEXT("mappa"), Map)) { return false; }

	// Una sola superficie, a Layer 1, con il vuoto sotto: nessuna cella nella sua colonna piu' in basso.
	PutCell(Map, FRTCellId(0, 0, 1), ERTHexBodyFill::Full);

	const TArray<FRTStructuralBody> Bodies = URTStructuralBodyLibrary::DeriveBodies(Map);
	if (!TestEqual(TEXT("un corpo, uno solo"), Bodies.Num(), 1)) { return false; }

	const FRTStructuralBody& B = Bodies[0];
	AddInfo(FString::Printf(TEXT("Full sopra il vuoto: top %.1f · bottom %.1f · alto %.1f (LayerHeight %.1f)"),
		B.TopZ, B.BottomZ, B.Height(), Map->LayerHeight));

	// La faccia superiore sta sotto il TILE, non al centro della cella.
	TestTrue(TEXT("il corpo comincia sotto il tile"),
		FMath::IsNearlyEqual(B.TopZ, Map->LayerHeight - RTCellTopZ, 0.01f));
	// `Full` = un volume-cella intero, e sopra il vuoto niente lo accorcia.
	TestTrue(TEXT("alto esattamente un volume-cella"), FMath::IsNearlyEqual(B.Height(), Map->LayerHeight, 0.01f));
	TestFalse(TEXT("e non e' stato troncato: sotto non c'era niente"), B.bTruncated);
	return true;
}

/**
 * 🔴 **Due superfici impilate: quella sopra si ferma sulla FACCIA SUPERIORE di quella sotto, non la
 * attraversa** — l'AC che #1865 chiama «confine inferiore corretto».
 *
 * 🔑 **E `bTruncated` e' asserito insieme all'altezza**, perche' i due fatti sono separabili: un corpo alto
 * quanto serve ma che non dichiara di essere stato accorciato lascerebbe un log incapace di dire *perche'*.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBodyStopsOnCellBelowTest,
	"RefactorTactics.StructuralBody.BodyStopsOnTheFaceOfTheCellBelow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBodyStopsOnCellBelowTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeBodyMap();
	if (!TestNotNull(TEXT("mappa"), Map)) { return false; }

	PutCell(Map, FRTCellId(0, 0, 1), ERTHexBodyFill::None); // la superficie sotto: non genera corpo
	PutCell(Map, FRTCellId(0, 0, 2), ERTHexBodyFill::Full); // quella sopra ne chiede uno intero

	const TArray<FRTStructuralBody> Bodies = URTStructuralBodyLibrary::DeriveBodies(Map);
	if (!TestEqual(TEXT("solo la superficie con BodyFill genera un corpo"), Bodies.Num(), 1)) { return false; }

	const FRTStructuralBody* B = Find(Bodies, FRTCellId(0, 0, 2));
	if (!TestNotNull(TEXT("il corpo e' quello di Layer 2"), B)) { return false; }

	const float FacciaDiSotto = Map->LayerHeight + RTCellTopZ; // centro di Layer 1 + mezzo tile
	AddInfo(FString::Printf(TEXT("L2 su L1: bottom %.1f, faccia superiore di L1 %.1f, troncato=%d"),
		B->BottomZ, FacciaDiSotto, B->bTruncated ? 1 : 0));

	TestTrue(TEXT("la base poggia sulla faccia superiore della cella sotto"),
		FMath::IsNearlyEqual(B->BottomZ, FacciaDiSotto, 0.01f));
	TestTrue(TEXT("e lo dichiara"), B->bTruncated);
	// La conseguenza che conta: non attraversa. Senza questa riga, un troncamento a una quota SBAGLIATA ma
	// dichiarata passerebbe.
	TestTrue(TEXT("non entra nella cella sottostante"), B->BottomZ >= FacciaDiSotto - 0.01f);
	return true;
}

/**
 * 🔑 **Il ponte e il tunnel sono lo STESSO caso visto da due lati**, e il dato d'autore e' cio' che li
 * distingue da una collina: `Third` lascia lo spazio, `Full` no.
 *
 * ⚠️ Il test asserisce che sotto il corpo **resti spazio libero misurabile**, non che il corpo esista: un
 * derivatore che riempisse tutto produrrebbe comunque un corpo, e la differenza sta solo nel quanto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBodyBridgeKeepsPassageTest,
	"RefactorTactics.StructuralBody.PartialFillLeavesThePassage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBodyBridgeKeepsPassageTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeBodyMap();
	if (!TestNotNull(TEXT("mappa"), Map)) { return false; }

	// Tunnel: pavimento a Layer 0, e sopra una campata a Layer 1 che chiede solo un terzo.
	PutCell(Map, FRTCellId(0, 0, 0), ERTHexBodyFill::None);
	PutCell(Map, FRTCellId(0, 0, 1), ERTHexBodyFill::Third);

	const TArray<FRTStructuralBody> Bodies = URTStructuralBodyLibrary::DeriveBodies(Map);
	const FRTStructuralBody* B = Find(Bodies, FRTCellId(0, 0, 1));
	if (!TestNotNull(TEXT("la campata genera un corpo"), B)) { return false; }

	const float TettoDelPavimento = RTCellTopZ;
	const float Passaggio = B->BottomZ - TettoDelPavimento;
	AddInfo(FString::Printf(TEXT("campata a 1/3: bottom %.1f, tetto del pavimento %.1f, passaggio libero %.1f"),
		B->BottomZ, TettoDelPavimento, Passaggio));

	TestFalse(TEXT("a un terzo non serve troncare: lo spazio bastava"), B->bTruncated);
	TestTrue(TEXT("alto un terzo del volume-cella"),
		FMath::IsNearlyEqual(B->Height(), Map->LayerHeight / 3.f, 0.01f));
	// 🔑 La proprieta' che rende il caso interessante: sotto ci si passa ancora.
	TestTrue(FString::Printf(TEXT("resta un passaggio libero sotto la campata (misurato %.1f)"), Passaggio),
		Passaggio > Map->LayerHeight / 2.f);

	// ⚠️ CONTROLLO NEGATIVO, e senza di lui il test non proverebbe che a deciderlo e' l'AUTORE: la stessa
	// geometria con `Full` chiude il passaggio. Se il derivatore ignorasse `BodyFill`, le due misure
	// coinciderebbero e la riga qui sopra sarebbe vera per caso.
	PutCell(Map, FRTCellId(0, 0, 1), ERTHexBodyFill::Full);
	const TArray<FRTStructuralBody> Chiusi = URTStructuralBodyLibrary::DeriveBodies(Map);
	const FRTStructuralBody* C = Find(Chiusi, FRTCellId(0, 0, 1));
	if (!TestNotNull(TEXT("con Full il corpo esiste ancora"), C)) { return false; }
	TestTrue(TEXT("ma con Full il passaggio si chiude"),
		FMath::IsNearlyEqual(C->BottomZ, TettoDelPavimento, 0.01f));
	TestTrue(TEXT("e il troncamento lo dichiara"), C->bTruncated);
	return true;
}

/**
 * ⛔ **Il corpo non e' dato di gioco**: nessuna cella nasce, e `ComputeHash` non si muove — il primo
 * non-goal di #1865.
 *
 * 🔑 **L'hash si confronta PRIMA e DOPO aver cambiato `BodyFill`**, non contro una costante: pinnare un
 * numero direbbe soltanto che l'hash e' stabile, non che questo campo ne sia fuori.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBodyIsNotGameStateTest,
	"RefactorTactics.StructuralBody.FillChangesNeitherCellsNorHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBodyIsNotGameStateTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeBodyMap();
	if (!TestNotNull(TEXT("mappa"), Map)) { return false; }

	PutCell(Map, FRTCellId(0, 0, 0), ERTHexBodyFill::None);
	PutCell(Map, FRTCellId(0, 0, 1), ERTHexBodyFill::None);

	const int32 CelleP = Map->NumCells();
	const uint32 HashP = Map->ComputeHash();
	TestEqual(TEXT("senza riempimento nessun corpo"), URTStructuralBodyLibrary::DeriveBodies(Map).Num(), 0);

	PutCell(Map, FRTCellId(0, 0, 1), ERTHexBodyFill::Full);
	const TArray<FRTStructuralBody> Bodies = URTStructuralBodyLibrary::DeriveBodies(Map);

	// ➕ Controllo positivo: il campo fa DAVVERO qualcosa, altrimenti le due righe sotto sarebbero vacue.
	TestEqual(TEXT("ora il corpo c'e'"), Bodies.Num(), 1);
	TestEqual(TEXT("ma nessuna cella e' nata"), Map->NumCells(), CelleP);
	TestEqual(TEXT("e l'hash non si e' mosso: il corpo e' presentazione"), Map->ComputeHash(), HashP);
	return true;
}

/**
 * 🔑 **Stesso dato, stessa lista** — l'AC di determinismo. Le celle si inseriscono in ordine INVERSO e
 * l'esito deve coincidere elemento per elemento: l'ordine canonico e' quello di `StableLess`, non quello
 * di arrivo nell'array.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBodyDerivationIsDeterministicTest,
	"RefactorTactics.StructuralBody.DerivationIsInvariantToInsertionOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBodyDerivationIsDeterministicTest::RunTest(const FString&)
{
	const TArray<FRTCellId> Ids = {
		FRTCellId(0, 0, 1), FRTCellId(1, 0, 1), FRTCellId(-1, 2, 1), FRTCellId(2, -1, 1)
	};

	URTHexMapAsset* A = MakeBodyMap();
	URTHexMapAsset* B = MakeBodyMap();
	if (!TestNotNull(TEXT("mappa A"), A) || !TestNotNull(TEXT("mappa B"), B)) { return false; }

	for (int32 I = 0; I < Ids.Num(); ++I) { PutCell(A, Ids[I], ERTHexBodyFill::TwoThirds); }
	for (int32 I = Ids.Num() - 1; I >= 0; --I) { PutCell(B, Ids[I], ERTHexBodyFill::TwoThirds); }

	const TArray<FRTStructuralBody> BodiesA = URTStructuralBodyLibrary::DeriveBodies(A);
	const TArray<FRTStructuralBody> BodiesB = URTStructuralBodyLibrary::DeriveBodies(B);

	// ➕ Controllo positivo: quattro corpi, non zero. Due liste vuote sarebbero "identiche" senza provare nulla.
	if (!TestEqual(TEXT("quattro corpi per lato"), BodiesA.Num(), Ids.Num())) { return false; }
	if (!TestEqual(TEXT("stesso numero di corpi"), BodiesB.Num(), BodiesA.Num())) { return false; }

	for (int32 I = 0; I < BodiesA.Num(); ++I)
	{
		TestTrue(FString::Printf(TEXT("corpo %d: stessa cella"), I), BodiesA[I].Cell == BodiesB[I].Cell);
		TestTrue(FString::Printf(TEXT("corpo %d: stessa base"), I),
			FMath::IsNearlyEqual(BodiesA[I].BottomZ, BodiesB[I].BottomZ, 0.01f));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
