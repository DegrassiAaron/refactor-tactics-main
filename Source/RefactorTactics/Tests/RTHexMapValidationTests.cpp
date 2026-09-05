// Le regole di TOPOLOGIA di `URTHexMapAsset::ValidateMapDetailed` — `E23`, [D-289], `#1832`.
//
// Quattro regole, e ognuna descrive un dato che si CONTRADDICE, non uno scritto male: una cella dove
// nessuno puo' stare e che non si dichiara impraticabile, un riparo che nessuno puo' usare, un blocco cotto
// da una geometria che non c'e' piu', due muri con la stessa identita' di sorgente.
//
// 🔑 **Ogni test ha la sua controprova.** Un validator che segnala sempre passerebbe meta' di questi test,
// ed e' il difetto piu' facile da scrivere in questo dominio: per ogni caso invalido c'e' il caso valido
// piu' vicino, che deve restare in silenzio.
//
// ⚠️ **Le regole 3 e 6 della issue non sono qui.** La 3 e' irrappresentabile — la forma a due maschere di
// `#1828` non sa esprimere la configurazione che vieterebbe, quindi un suo test non potrebbe fallire. La 6
// non ha il proprio antecedente: nessun campo dice quante unita' il livello si aspetti in una cella. Le
// ragioni stanno accanto alle regole, in `RTHexMapAsset.h`.

#include "Misc/AutomationTest.h"

#include "Map/RTGeometryGrammar.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverPlacementLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexOccupancyLibrary.h"
#include "Tests/RTAuthoredArenaForTest.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr float ValidationHexSize = 100.0f;
	const FRTCellId ValidationOrigin{ 0, 0, 0 };

	/** Una mappa con una sola cella all'origine e la dimensione dell'esagono fissata. */
	URTHexMapAsset* MakeValidationMap()
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		Map->HexSize = ValidationHexSize;
		FRTHexCellData Cell;
		Cell.Id = ValidationOrigin;
		Map->AddOrUpdateCell(Cell);
		return Map;
	}

	/** Il muro continuo: diametro sull'asse, passante per il centro. Stessa forma dei test di posa. */
	// ⚠️ **Il prefisso non e' stile: e' cio' che tiene in piedi l'unity build** (`#2271`). Questa funzione e
	// quella di `RTHexCoverPlacementTests.cpp` erano entrambe `Diameter` in un namespace ANONIMO, che protegge
	// dal linker ma non dal compilatore: quando il raggruppamento le mette nella stessa unita' il build muore
	// con `C2084 ha gia' un corpo`. Il difetto era latente e si e' visto aggiungendo righe a un TERZO file di
	// test, che ha cambiato il raggruppamento — cioe' colpisce chi passa di qui, non chi lo ha introdotto.
	FRTGeometrySegment ValidationDiameter(ERTTacticalAxis Axis)
	{
		FRTGeometrySegment S;
		S.Axis = Axis;
		S.Offset = 0;
		S.AlongStart = -RT_GeometryQuanta;
		S.AlongEnd = RT_GeometryQuanta;
		S.WallType = ERTHexCoverType::High;
		return S;
	}

	/** La maschera d'occupancy della cella, dalla stessa catena che usano cottura e validator. */
	FRTOccupancyMask MaskOfCell(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		TArray<FRTOccupancyPolyline> Geometry;
		for (const FRTHexInteriorWall& Wall : Map->InteriorWalls)
		{
			if (Wall.Cell == Cell)
			{
				Geometry.Add(URTGeometryGrammarLibrary::ToPolyline(Wall.Segment, Map->HexSize));
			}
		}
		return URTHexOccupancyLibrary::ComputeMask(Geometry, Map->HexSize);
	}

	/** Quante segnalazioni di questo reason code produce la mappa. */
	int32 CountReason(const URTHexMapAsset* Map, ERTMapValidationReason Reason)
	{
		TArray<FRTMapValidationIssue> Issues;
		Map->ValidateMapDetailed(Issues);
		int32 Count = 0;
		for (const FRTMapValidationIssue& Issue : Issues)
		{
			if (Issue.Reason == Reason)
			{
				++Count;
			}
		}
		return Count;
	}

	/** Chiude la cella riempiendo tutti i dodici settori: tre diametri a 0, 60 e 120 gradi. */
	void FillCellWithWalls(URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		for (const ERTTacticalAxis Axis : { ERTTacticalAxis::Deg0, ERTTacticalAxis::Deg60,
											ERTTacticalAxis::Deg120 })
		{
			Map->InteriorWalls.Add(FRTHexInteriorWall(Cell, ValidationDiameter(Axis)));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------
// REGOLA 1 — posa impossibile
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapNoLegalPlacementTest,
	"RefactorTactics.HexMapValidation.NoLegalPlacementIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapNoLegalPlacementTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeValidationMap();
	FillCellWithWalls(Map, ValidationOrigin);

	// LA PREMESSA, misurata e non assunta: se i tre diametri non saturassero la cella il test non
	// proverebbe niente, e passerebbe per la ragione sbagliata.
	const FRTOccupancyMask Mask = MaskOfCell(Map, ValidationOrigin);
	if (!TestFalse(TEXT("premessa: la cella non ha alcuna posa legale"),
			URTHexCoverPlacementLibrary::HasLegalPlacement(Mask, FRTFootprintProfile())))
	{
		AddError(FString::Printf(TEXT("maschera ottenuta: 0x%03X"), Mask.Sectors));
		return false;
	}

	TestEqual(TEXT("una cella senza posa e non marcata e' segnalata"),
		CountReason(Map, ERTMapValidationReason::NoLegalPlacement), 1);

	// CONTROPROVA — la stessa cella, dichiarata impraticabile dall'autore: il dato non si contraddice piu',
	// e il silenzio e' l'esito giusto. Senza questa meta' il test passerebbe con un validator che segnala
	// ogni cella satura.
	FRTHexCellData Blocked;
	Blocked.Id = ValidationOrigin;
	Blocked.bBlocksMovement = true;
	Map->AddOrUpdateCell(Blocked);

	TestEqual(TEXT("marcata impraticabile, la stessa cella non e' piu' segnalata"),
		CountReason(Map, ERTMapValidationReason::NoLegalPlacement), 0);

	// E una cella VUOTA non e' mai segnalata: e' il caso della quasi totalita' delle celle di una mappa, ed
	// e' cio' che rende attendibile il test di non-regressione sugli asset versionati.
	URTHexMapAsset* Empty = MakeValidationMap();
	TestEqual(TEXT("una cella senza muri non produce segnalazioni"),
		CountReason(Empty, ERTMapValidationReason::NoLegalPlacement), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// REGOLA 2 — copertura irraggiungibile
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapUnreachableCoverTest,
	"RefactorTactics.HexMapValidation.UnreachableCoverIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapUnreachableCoverTest::RunTest(const FString&)
{
	// Una copertura su un bordo di una cella SATURA: non esiste alcuna regione di posa, quindi nessuna
	// opzione, quindi nessuna unita' potra' mai usarla.
	URTHexMapAsset* Map = MakeValidationMap();
	FillCellWithWalls(Map, ValidationOrigin);

	FRTHexCellData WithCover;
	WithCover.Id = ValidationOrigin;
	WithCover.bBlocksMovement = true; // marcata, cosi' la regola 1 tace e resta un solo difetto in campo
	FRTHexCover Cover;
	Cover.Edge = ERTHexDirection::E;
	Cover.Type = ERTHexCoverType::Low;
	Cover.Integrity = 10;
	WithCover.Covers.Add(Cover);
	Map->AddOrUpdateCell(WithCover);

	TestEqual(TEXT("una copertura che nessuna posa raggiunge e' segnalata"),
		CountReason(Map, ERTMapValidationReason::UnreachableCover), 1);

	// CONTROPROVA — la stessa copertura su una cella LIBERA: la regione unica tocca ogni bordo, l'opzione
	// esiste, e il validator tace.
	URTHexMapAsset* Open = MakeValidationMap();
	FRTHexCellData OpenCell;
	OpenCell.Id = ValidationOrigin;
	OpenCell.Covers.Add(Cover);
	Open->AddOrUpdateCell(OpenCell);

	TestEqual(TEXT("la stessa copertura su una cella libera non e' segnalata"),
		CountReason(Open, ERTMapValidationReason::UnreachableCover), 0);

	// E una copertura SENZA TIPO non entra in questa regola: e' gia' segnalata da quella storica, e
	// contarla due volte farebbe smettere il numero di segnalazioni di dire quanti difetti ci sono.
	URTHexMapAsset* Typeless = MakeValidationMap();
	FillCellWithWalls(Typeless, ValidationOrigin);
	FRTHexCellData NoType;
	NoType.Id = ValidationOrigin;
	NoType.bBlocksMovement = true;
	FRTHexCover Blank;
	Blank.Edge = ERTHexDirection::E;
	Blank.Type = ERTHexCoverType::None;
	Blank.Integrity = 10;
	NoType.Covers.Add(Blank);
	Typeless->AddOrUpdateCell(NoType);

	TestEqual(TEXT("una copertura senza tipo non e' contata due volte"),
		CountReason(Typeless, ERTMapValidationReason::UnreachableCover), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// REGOLA 4 — blocco derivato stantio
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapStaleGeneratedBlockTest,
	"RefactorTactics.HexMapValidation.StaleGeneratedBlockIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapStaleGeneratedBlockTest::RunTest(const FString&)
{
	// Cella senza geometria — quindi con una posa legale — ma marcata impraticabile dalla COTTURA.
	// E' il rebake mancante: la geometria che l'aveva chiusa non c'e' piu'.
	URTHexMapAsset* Map = MakeValidationMap();
	FRTHexCellData Stale;
	Stale.Id = ValidationOrigin;
	Stale.bBlocksMovement = true;
	Stale.bMovementBlockGenerated = true;
	Map->AddOrUpdateCell(Stale);

	TestEqual(TEXT("un blocco derivato senza geometria che lo giustifichi e' segnalato"),
		CountReason(Map, ERTMapValidationReason::StaleGeneratedBlock), 1);

	// 🔑 **LA CONTROPROVA CHE CONTA: lo stesso stato, ma dipinto a mano.**
	// «L'autore vince» e' gia' la regola di `DeriveStandability`, e questo validator non la contraddice:
	// una cella che l'autore vuole impraticabile non e' un errore, qualunque cosa dica la geometria.
	// Senza questa meta', la regola segnalerebbe ogni cella che un designer ha chiuso di proposito.
	URTHexMapAsset* Authored = MakeValidationMap();
	FRTHexCellData ByHand;
	ByHand.Id = ValidationOrigin;
	ByHand.bBlocksMovement = true;
	ByHand.bMovementBlockGenerated = false;
	Authored->AddOrUpdateCell(ByHand);

	TestEqual(TEXT("un blocco DIPINTO A MANO non e' mai segnalato"),
		CountReason(Authored, ERTMapValidationReason::StaleGeneratedBlock), 0);

	// E un blocco derivato che la geometria giustifica ancora resta in silenzio.
	URTHexMapAsset* Justified = MakeValidationMap();
	FillCellWithWalls(Justified, ValidationOrigin);
	FRTHexCellData Good;
	Good.Id = ValidationOrigin;
	Good.bBlocksMovement = true;
	Good.bMovementBlockGenerated = true;
	Justified->AddOrUpdateCell(Good);

	TestEqual(TEXT("un blocco derivato ancora giustificato non e' segnalato"),
		CountReason(Justified, ERTMapValidationReason::StaleGeneratedBlock), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// REGOLA 5 — identita' di sorgente duplicata
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapDuplicateCoverSourceTest,
	"RefactorTactics.HexMapValidation.DuplicateCoverSourceIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapDuplicateCoverSourceTest::RunTest(const FString&)
{
	// 🔑 **Il caso che questa regola aggiunge e' STRETTO, e va detto**: due muri identici sono gia'
	// segnalati dalla regola storica «muro interno duplicato di», che confronta i segmenti con
	// `operator==`. Quello che quella regola NON vede e' la coppia che differisce solo per il `Layer` del
	// segmento: `operator==` la dice diversa — il layer lo confronta — ma `FRTCoverSourceId` il layer non
	// lo porta, quindi le due opzioni di copertura risultanti sono indistinguibili.
	URTHexMapAsset* Map = MakeValidationMap();

	FRTGeometrySegment First = ValidationDiameter(ERTTacticalAxis::Deg0);
	FRTGeometrySegment Second = First;
	Second.Layer = First.Layer + 1;

	TestFalse(TEXT("premessa: per la regola storica i due segmenti sono DIVERSI"), First == Second);

	Map->InteriorWalls.Add(FRTHexInteriorWall(ValidationOrigin, First));
	Map->InteriorWalls.Add(FRTHexInteriorWall(ValidationOrigin, Second));

	TestEqual(TEXT("due muri con la stessa identita' di sorgente sono segnalati"),
		CountReason(Map, ERTMapValidationReason::DuplicateCoverSource), 1);

	// CONTROPROVA 1 — due muri davvero diversi (assi diversi) non producono nulla.
	URTHexMapAsset* Distinct = MakeValidationMap();
	Distinct->InteriorWalls.Add(FRTHexInteriorWall(ValidationOrigin, ValidationDiameter(ERTTacticalAxis::Deg0)));
	Distinct->InteriorWalls.Add(FRTHexInteriorWall(ValidationOrigin, ValidationDiameter(ERTTacticalAxis::Deg60)));
	TestEqual(TEXT("due muri su assi diversi non sono un duplicato di sorgente"),
		CountReason(Distinct, ERTMapValidationReason::DuplicateCoverSource), 0);

	// CONTROPROVA 2 — due muri IDENTICI: li segnala la regola storica, non questa. Contarli qui sarebbe
	// contare due volte lo stesso difetto.
	URTHexMapAsset* Same = MakeValidationMap();
	Same->InteriorWalls.Add(FRTHexInteriorWall(ValidationOrigin, First));
	Same->InteriorWalls.Add(FRTHexInteriorWall(ValidationOrigin, First));
	TestEqual(TEXT("il duplicato esatto NON e' contato da questa regola"),
		CountReason(Same, ERTMapValidationReason::DuplicateCoverSource), 0);
	TestTrue(TEXT("ma la regola storica lo segnala comunque"), Same->ValidateMap().Num() > 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// DETERMINISMO — l'elenco non dipende da come l'asset e' stato costruito
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapValidationDeterminismTest,
	"RefactorTactics.HexMapValidation.IssueListIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapValidationDeterminismTest::RunTest(const FString&)
{
	// Tre celle difettose. L'ordine in cui entrano nell'asset lo decide chi edita: due asset che descrivono
	// la stessa mappa con le celle scritte in ordine diverso devono produrre lo STESSO elenco.
	const TArray<FRTCellId> Ids = { FRTCellId(2, -1, 0), FRTCellId(0, 0, 0), FRTCellId(-1, 2, 0) };

	auto BuildBroken = [&Ids](bool bReversed) -> URTHexMapAsset*
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		Map->HexSize = ValidationHexSize;
		for (int32 I = 0; I < Ids.Num(); ++I)
		{
			const FRTCellId& Id = Ids[bReversed ? (Ids.Num() - 1 - I) : I];
			FRTHexCellData Cell;
			Cell.Id = Id;
			Cell.bBlocksMovement = true;
			Cell.bMovementBlockGenerated = true; // difetto: blocco derivato senza geometria
			Map->AddOrUpdateCell(Cell);
		}
		return Map;
	};

	TArray<FRTMapValidationIssue> Forward;
	TArray<FRTMapValidationIssue> Reversed;
	BuildBroken(false)->ValidateMapDetailed(Forward);
	BuildBroken(true)->ValidateMapDetailed(Reversed);

	if (!TestEqual(TEXT("le tre celle difettose sono segnalate"), Forward.Num(), 3))
	{
		return false;
	}
	TestEqual(TEXT("e l'ordine inverso ne produce lo stesso numero"), Reversed.Num(), Forward.Num());

	for (int32 I = 0; I < Forward.Num() && I < Reversed.Num(); ++I)
	{
		TestTrue(*FString::Printf(TEXT("segnalazione %d: stessa cella"), I),
			Forward[I].Cell == Reversed[I].Cell);
		TestTrue(*FString::Printf(TEXT("segnalazione %d: stesso reason"), I),
			Forward[I].Reason == Reversed[I].Reason);
	}

	// E l'ordine e' quello CANONICO, non quello di inserimento: `StableLess` decide, come per `ComputeHash`.
	for (int32 I = 1; I < Forward.Num(); ++I)
	{
		TestTrue(TEXT("le celle escono in ordine stabile"),
			URTHexLibrary::StableLess(Forward[I - 1].Cell, Forward[I].Cell));
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// NON-REGRESSIONE — le mappe versionate non guadagnano segnalazioni
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexMapVersionedMapsCleanTest,
	"RefactorTactics.HexMapValidation.VersionedMapsProduceNoNewIssues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexMapVersionedMapsCleanTest::RunTest(const FString&)
{
	// 🔴 **E' il criterio d'accettazione piu' importante della issue**: quattro regole nuove che segnalassero
	// le mappe gia' disegnate sarebbero rumore, e chi disegna imparerebbe a ignorare il pannello.
	const TCHAR* Paths[] = {
		TEXT("/Game/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.DA_HexMap_Arena"),
		TEXT("/Game/RT/Maps/Dev/L_DevSandbox/Data/DA_HexMap_Sandbox.DA_HexMap_Sandbox"),
		TEXT("/Game/RT/Maps/Dev/_Scratch/DA_HexMap_Scratch_Basin.DA_HexMap_Scratch_Basin"),
	};

	int32 Loaded = 0;
	for (const TCHAR* Path : Paths)
	{
		URTHexMapAsset* Map = LoadObject<URTHexMapAsset>(nullptr, Path);
		if (Map == nullptr)
		{
			// ⚠️ Non e' un fallimento: un asset puo' essere stato spostato o rinominato, e far diventare
			// rosso questo test per un percorso stantio direbbe il falso sul validator. Si conta quanti
			// se ne sono caricati, e si pretende che ne arrivi almeno uno.
			AddInfo(FString::Printf(TEXT("asset non caricato (percorso stantio?): %s"), Path));
			continue;
		}
		++Loaded;

		TArray<FRTMapValidationIssue> Issues;
		Map->ValidateMapDetailed(Issues);
		for (const FRTMapValidationIssue& Issue : Issues)
		{
			AddError(FString::Printf(TEXT("%s produce una segnalazione nuova (reason %d): %s"),
				Path, static_cast<int32>(Issue.Reason), *Issue.Message));
		}
		TestEqual(*FString::Printf(TEXT("%s: zero segnalazioni delle regole nuove"), Path),
			Issues.Num(), 0);
	}

	TestTrue(TEXT("almeno una mappa versionata e' stata caricata: senza, il test sarebbe vacuo"),
		Loaded > 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
