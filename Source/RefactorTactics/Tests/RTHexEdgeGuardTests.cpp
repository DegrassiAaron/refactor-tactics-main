#include "Misc/AutomationTest.h"

#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLedgeLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexMapCustomVersion.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FRTHexCellData Cell(int32 X, int32 Y, int32 Layer = 0)
	{
		return FRTHexCellData(FRTCellId(X, Y, Layer));
	}

	/** Una colonna isolata: una sola cella, quindi tutti e sei i bordi sono aperti. */
	URTHexMapAsset* LoneCell(int32 Layer = 1)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		Map->AddOrUpdateCell(Cell(0, 0, Layer));
		return Map;
	}

	/** Aggiunge un parapetto passando dall'API di mappa: `Cells.Add` diretto non aggiorna la cache. */
	void AddGuard(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		FRTHexCellData Data = *Map->FindCell(Id);
		Data.Guards.Add(FRTHexEdgeGuard(Edge));
		Map->AddOrUpdateCell(Data);
	}
}

// =====================================================================================================
// Il dato: round-trip, hash, validazione
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEdgeGuardRoundTripTest,
	"RefactorTactics.Map.EdgeGuard.RoundTripsThroughAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEdgeGuardRoundTripTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = LoneCell();
	AddGuard(Map, FRTCellId(0, 0, 1), ERTHexDirection::NE);

	const FRTHexCellData* Read = Map->FindCell(FRTCellId(0, 0, 1));
	TestNotNull(TEXT("la cella si rilegge"), Read);
	TestTrue(TEXT("il parapetto e' su NE"), Read->HasGuardOn(ERTHexDirection::NE));
	TestFalse(TEXT("e non su un altro bordo"), Read->HasGuardOn(ERTHexDirection::SW));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEdgeGuardEntersHashTest,
	"RefactorTactics.Map.EdgeGuard.EntersComputeHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEdgeGuardEntersHashTest::RunTest(const FString&)
{
	// 🔑 Il test che giustifica la scelta di far entrare il guard nell'hash: due mappe che differiscono
	// SOLO per un parapetto risolvono diversamente una spinta, quindi non possono avere lo stesso digest.
	URTHexMapAsset* Senza = LoneCell();
	URTHexMapAsset* Con = LoneCell();
	AddGuard(Con, FRTCellId(0, 0, 1), ERTHexDirection::E);

	TestNotEqual(TEXT("un parapetto cambia l'hash della mappa"), Senza->ComputeHash(), Con->ComputeHash());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEdgeGuardHashOrderTest,
	"RefactorTactics.Map.EdgeGuard.HashIsOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEdgeGuardHashOrderTest::RunTest(const FString&)
{
	// L'ordine di dichiarazione nell'asset non e' informazione: due autori che scrivono gli stessi due
	// parapetti in ordine diverso devono produrre lo stesso digest.
	URTHexMapAsset* A = LoneCell();
	AddGuard(A, FRTCellId(0, 0, 1), ERTHexDirection::E);
	AddGuard(A, FRTCellId(0, 0, 1), ERTHexDirection::SW);

	URTHexMapAsset* B = LoneCell();
	AddGuard(B, FRTCellId(0, 0, 1), ERTHexDirection::SW);
	AddGuard(B, FRTCellId(0, 0, 1), ERTHexDirection::E);

	TestEqual(TEXT("l'ordine di dichiarazione non cambia l'hash"), A->ComputeHash(), B->ComputeHash());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEdgeGuardDuplicateTest,
	"RefactorTactics.Map.EdgeGuard.DuplicateOnSameEdgeIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEdgeGuardDuplicateTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = LoneCell();
	AddGuard(Map, FRTCellId(0, 0, 1), ERTHexDirection::E);
	AddGuard(Map, FRTCellId(0, 0, 1), ERTHexDirection::E);

	const TArray<FString> Errors = Map->ValidateMap();

	const bool bSegnalato = Errors.ContainsByPredicate([](const FString& E)
	{
		return E.Contains(TEXT("parapetti sovrapposti"));
	});
	TestTrue(TEXT("il doppione sullo stesso bordo e' un errore"), bSegnalato);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEdgeGuardInertTest,
	"RefactorTactics.Map.EdgeGuard.OnConnectedEdgeIsInert",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEdgeGuardInertTest::RunTest(const FString&)
{
	// Warning e non errore: da un bordo connesso non si cade comunque, e una mappa puo' crescere intorno a
	// un parapetto autorato senza diventare invalida.
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	Map->AddOrUpdateCell(Cell(0, 0, 1));
	Map->AddOrUpdateCell(Cell(1, 0, 1)); // il vicino a E esiste
	AddGuard(Map, FRTCellId(0, 0, 1), ERTHexDirection::E);

	const TArray<FString> Errors = Map->ValidateMap();

	const bool bWarning = Errors.ContainsByPredicate([](const FString& E)
	{
		return E.Contains(TEXT("parapetto inerte"));
	});
	const bool bErrore = Errors.ContainsByPredicate([](const FString& E)
	{
		return E.Contains(TEXT("Error:")) && E.Contains(TEXT("parapetto"));
	});
	TestTrue(TEXT("il parapetto inerte e' segnalato"), bWarning);
	TestFalse(TEXT("ma NON come errore"), bErrore);

	// 🔴 **L'ordine di inserimento non deve contare**, ed e' il difetto che questo test ha trovato: la
	// prima stesura interrogava `Seen`, popolato DENTRO il ciclo di validazione, quindi il vicino
	// esisteva solo se era stato scritto prima. Qui la cella col parapetto e' inserita per SECONDA.
	URTHexMapAsset* Invertita = NewObject<URTHexMapAsset>();
	Invertita->AddOrUpdateCell(Cell(1, 0, 1));
	Invertita->AddOrUpdateCell(Cell(0, 0, 1));
	AddGuard(Invertita, FRTCellId(0, 0, 1), ERTHexDirection::E);

	const TArray<FString> AltriErrori = Invertita->ValidateMap();
	const bool bWarningInvertito = AltriErrori.ContainsByPredicate([](const FString& E)
	{
		return E.Contains(TEXT("parapetto inerte"));
	});
	TestTrue(TEXT("segnalato anche con l'ordine di inserimento invertito"), bWarningInvertito);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEdgeGuardFormatVersionTest,
	"RefactorTactics.Map.FormatVersion.MigratesFrom15",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEdgeGuardFormatVersionTest::RunTest(const FString&)
{
	// Il passo v15 -> v16 e' DICHIARATIVO: l'array nasce vuoto e nessun dato esistente cambia significato.
	// Il test che conta e' che i due numeri non divergano — lo `static_assert` lo impone al compilatore, e
	// questo lo rende leggibile anche a chi guarda il registro dei test.
	TestEqual(TEXT("custom version e format version coincidono"),
		static_cast<int32>(FRTHexMapCustomVersion::LatestVersion),
		URTHexMapAsset::CurrentFormatVersion);
	TestEqual(TEXT("la versione corrente e' 16"), URTHexMapAsset::CurrentFormatVersion, 16);

	URTHexMapAsset* Vecchia = LoneCell();
	TestEqual(TEXT("una mappa senza parapetti nasce con l'array vuoto"),
		Vecchia->FindCell(FRTCellId(0, 0, 1))->Guards.Num(), 0);
	return true;
}

// =====================================================================================================
// Le query: bordo aperto e cella di atterraggio
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpenEdgeDerivedTest,
	"RefactorTactics.Map.OpenEdge.DerivedFromMissingNeighbour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpenEdgeDerivedTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	Map->AddOrUpdateCell(Cell(0, 0, 1));
	Map->AddOrUpdateCell(Cell(1, 0, 1)); // vicino a E

	const FRTCellId Origine(0, 0, 1);
	TestFalse(TEXT("verso il vicino esistente il bordo NON e' aperto"),
		URTHexLedgeLibrary::IsEdgeOpen(Map, Origine, ERTHexDirection::E));
	TestTrue(TEXT("verso il vuoto il bordo e' aperto"),
		URTHexLedgeLibrary::IsEdgeOpen(Map, Origine, ERTHexDirection::W));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpenEdgeGuardSuppressesTest,
	"RefactorTactics.Map.OpenEdge.GuardSuppressesOpenness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpenEdgeGuardSuppressesTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = LoneCell();
	const FRTCellId Origine(0, 0, 1);

	TestTrue(TEXT("senza parapetto il bordo e' aperto"),
		URTHexLedgeLibrary::IsEdgeOpen(Map, Origine, ERTHexDirection::NE));

	AddGuard(Map, FRTCellId(0, 0, 1), ERTHexDirection::NE);

	TestFalse(TEXT("il parapetto lo chiude"),
		URTHexLedgeLibrary::IsEdgeOpen(Map, Origine, ERTHexDirection::NE));
	TestTrue(TEXT("e non tocca gli altri bordi"),
		URTHexLedgeLibrary::IsEdgeOpen(Map, Origine, ERTHexDirection::SW));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLandingSkipsMissingLayersTest,
	"RefactorTactics.Map.LandingCell.SkipsMissingLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLandingSkipsMissingLayersTest::RunTest(const FString&)
{
	// 🔴 Il test che protegge dal difetto piu' probabile: `Layer - 1`. La colonna salta il piano 2, e chi
	// cade dal 3 deve arrivare all'1 — non finire nel vuoto perche' il piano immediatamente sotto non c'e'.
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	Map->AddOrUpdateCell(Cell(0, 0, 3));
	Map->AddOrUpdateCell(Cell(0, 0, 1));

	FRTCellId Landing;
	TestTrue(TEXT("l'atterraggio esiste"),
		URTHexLedgeLibrary::FindLandingCell(Map, FRTCellId(0, 0, 3), Landing));
	TestEqual(TEXT("si scavalca il piano mancante"), Landing, FRTCellId(0, 0, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLandingIgnoresHeightTest,
	"RefactorTactics.Map.LandingCell.IgnoresHeightOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLandingIgnoresHeightTest::RunTest(const FString&)
{
	// 🔴 `Height` e' presentazione — *«per il rendering; la logica usa Layer + archi»*. Una query di
	// gameplay che ordinasse per quota farebbe decidere il rendering su dove finisce un'unita'.
	//
	// Qui il layer 2 porta una quota d'autore che lo colloca, in RENDERING, sotto il layer 1. Se la query
	// ordinasse per quota, l'atterraggio da 3 diventerebbe il layer 1; ordinando per Layer resta il 2.
	// Le due mappe devono dare lo STESSO esito: e' la prova che la presentazione non decide.
	URTHexMapAsset* Piatta = NewObject<URTHexMapAsset>();
	Piatta->AddOrUpdateCell(Cell(0, 0, 3));
	Piatta->AddOrUpdateCell(Cell(0, 0, 2));
	Piatta->AddOrUpdateCell(Cell(0, 0, 1));

	URTHexMapAsset* Sfalsata = NewObject<URTHexMapAsset>();
	Sfalsata->AddOrUpdateCell(Cell(0, 0, 3));
	FRTHexCellData Media = Cell(0, 0, 2);
	Media.Height = -100000; // quota di rendering che la porterebbe sotto ogni altra
	Sfalsata->AddOrUpdateCell(Media);
	Sfalsata->AddOrUpdateCell(Cell(0, 0, 1));

	FRTCellId A, B;
	URTHexLedgeLibrary::FindLandingCell(Piatta, FRTCellId(0, 0, 3), A);
	URTHexLedgeLibrary::FindLandingCell(Sfalsata, FRTCellId(0, 0, 3), B);

	TestEqual(TEXT("l'atterraggio non dipende da Height"), A, B);
	TestEqual(TEXT("ed e' il layer immediatamente inferiore per LAYER"), A, FRTCellId(0, 0, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLandingDeterministicTest,
	"RefactorTactics.Map.LandingCell.IsDeterministicAndInteger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLandingDeterministicTest::RunTest(const FString&)
{
	// L'ordine dell'array non e' informazione: due mappe con le stesse celle inserite in ordine diverso
	// devono produrre lo stesso atterraggio.
	URTHexMapAsset* A = NewObject<URTHexMapAsset>();
	A->AddOrUpdateCell(Cell(0, 0, 5));
	A->AddOrUpdateCell(Cell(0, 0, 2));
	A->AddOrUpdateCell(Cell(0, 0, 4));

	URTHexMapAsset* B = NewObject<URTHexMapAsset>();
	B->AddOrUpdateCell(Cell(0, 0, 4));
	B->AddOrUpdateCell(Cell(0, 0, 5));
	B->AddOrUpdateCell(Cell(0, 0, 2));

	FRTCellId LA, LB;
	TestTrue(TEXT("A atterra"), URTHexLedgeLibrary::FindLandingCell(A, FRTCellId(0, 0, 5), LA));
	TestTrue(TEXT("B atterra"), URTHexLedgeLibrary::FindLandingCell(B, FRTCellId(0, 0, 5), LB));
	TestEqual(TEXT("stesso esito a prescindere dall'ordine"), LA, LB);
	TestEqual(TEXT("ed e' la piu' alta sotto"), LA, FRTCellId(0, 0, 4));

	// Una colonna diversa non entra nel calcolo.
	URTHexMapAsset* Altra = NewObject<URTHexMapAsset>();
	Altra->AddOrUpdateCell(Cell(0, 0, 5));
	Altra->AddOrUpdateCell(Cell(9, 9, 1));
	FRTCellId Nessuna;
	TestFalse(TEXT("sotto non c'e' niente nella stessa colonna"),
		URTHexLedgeLibrary::FindLandingCell(Altra, FRTCellId(0, 0, 5), Nessuna));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLedgeFailClosedTest,
	"RefactorTactics.Map.Ledge.FailsClosedWithoutMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLedgeFailClosedTest::RunTest(const FString&)
{
	// Senza mappa autorevole non si dichiara aperto un bordo: la direzione sicura e' «non si cade».
	FRTCellId Landing;
	TestFalse(TEXT("nessun bordo aperto senza mappa"),
		URTHexLedgeLibrary::IsEdgeOpen(nullptr, FRTCellId(0, 0, 1), ERTHexDirection::E));
	TestFalse(TEXT("nessun atterraggio senza mappa"),
		URTHexLedgeLibrary::FindLandingCell(nullptr, FRTCellId(0, 0, 1), Landing));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
