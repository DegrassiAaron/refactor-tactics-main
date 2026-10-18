#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Combat/RTOffensiveActionLibrary.h"
#include "Turn/RTMovementActionLibrary.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTTurnLogLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio N, tutte le celle percorribili. Nome distinto per file (unity build). */
	URTHexMapAsset* MakeMoveActionMap(int32 Radius = 6)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return M;
	}

	/** Trasforma una cella in muro: blocca il passo e la vista. */
	void MakeWall(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.bBlocksMovement = true;
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Terreno accidentato: si percorre a piedi, e nega la mobilita' rapida (`bBlocksDashCharge`). */
	void MakeRough(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.Surface = ERTHexSurface::Rough;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Una copertura sul bordo `Edge` di `Cell`: alta ferma il passo, bassa no. Nome distinto per file (unity build). */
	void PutMoveActionCover(URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge, ERTHexCoverType Type)
	{
		verify(URTHexCoverLibrary::AddCover(Map, Cell, Edge, Type, FRTHexCover::DefaultIntegrity(Type)));
	}

	/** Una porta sul bordo `Edge` di `Cell`, nello stato dato. */
	void PutMoveActionDoor(URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge, ERTHexDoorState State)
	{
		FRTHexCellData Data = *Map->FindCell(Cell);
		Data.Doors.Add(FRTHexDoor(Edge, State));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** La direzione da `From` al vicino `To`: serve a posare un bordo senza scrivere a mano la tabella delle sei. */
	ERTHexDirection MoveActionEdgeToward(const FRTCellId& From, const FRTCellId& To)
	{
		ERTHexDirection Dir = ERTHexDirection::E;
		verify(URTHexLibrary::DirectionBetween(From, To, Dir));
		return Dir;
	}

	/** Le celle che il resolver attraversa, partenza esclusa: la retta di `HexLine` senza la prima cella. */
	TArray<FRTCellId> MoveActionHexLineTail(const FRTCellId& From, const FRTCellId& To)
	{
		TArray<FRTCellId> Line = URTHexLibrary::HexLine(From, To);
		Line.RemoveAt(0);
		return Line;
	}
}

// ⚠️ `ClientContext` oltre a `EditorContext`, e non per simmetria con i vicini: e' uno dei **dieci test
// vincolanti** del catalogo (`roadmap-v0.1.md` §6), e CP 12.3 (`#83`) chiede che la suite giri anche in un
// pacchetto. Con il solo `EditorContext` il controller lo filtra fuori dal target `Game` — misurato:
// `Automation List` in un pacchetto Development registrava **zero** test su 875, tutti dichiarati solo Editor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashBlockedArcTest,
	"RefactorTactics.Actions.Dash.BlockedArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FRTDashBlockedArcTest::RunTest(const FString&)
{
	// Nome vincolante del catalogo v0.1 §15: lo scatto e' interrotto da un muro sulla traiettoria. E' la
	// differenza fra una mobilita' LINEARE e il movimento a budget: con il pathfinding il muro si aggirerebbe,
	// e non ci sarebbe alcun arco bloccato da osservare.
	URTHexMapAsset* Map = MakeMoveActionMap();
	MakeWall(Map, FRTCellId(2, 0));

	const TMap<FRTCellId, int32> Empty;
	const TSet<int32> NoHostiles;

	const FRTLinearMoveResult Blocked = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), /*MaxCells*/ 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);

	TestTrue(TEXT("si ferma PRIMA del muro"), Blocked.Final == FRTCellId(1, 0));
	TestTrue(TEXT("e il motivo e' il terreno, non un'unita'"), Blocked.Stop == ERTLinearStop::BlockedByTerrain);
	TestEqual(TEXT("ha attraversato una sola cella"), Blocked.Entered.Num(), 1);

	// Senza muro, lo stesso scatto arriva a destinazione.
	URTHexMapAsset* Clear = MakeMoveActionMap();
	const FRTLinearMoveResult Free = URTMovementActionLibrary::ResolveLinearMove(
		Clear, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("via libera: arriva"), Free.Final == FRTCellId(3, 0) && Free.Stop == ERTLinearStop::Completed);

	// Un'unita' sulla traiettoria ferma lo scatto tanto quanto un muro, ma con un motivo diverso.
	TMap<FRTCellId, int32> Occupied;
	Occupied.Add(FRTCellId(2, 0), 7);
	const FRTLinearMoveResult Crowded = URTMovementActionLibrary::ResolveLinearMove(
		Clear, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearDash, Occupied, NoHostiles);
	TestTrue(TEXT("si ferma davanti all'unita'"), Crowded.Final == FRTCellId(1, 0));
	TestTrue(TEXT("col motivo giusto"), Crowded.Stop == ERTLinearStop::BlockedByUnit);

	// Lo scatto NON e' un percorso, e da [D-523]/[D-524] non e' nemmeno vincolato alle sei direzioni: una
	// destinazione fuori asse entro la portata si raggiunge lungo la RETTA di `HexLine`, e un muro su quella
	// retta la ferma come su un asse. ⌫ *Fino a [D-524] questa riga asseriva il rifiuto* (`NotAligned`).
	const FRTLinearMoveResult Oblique = URTMovementActionLibrary::ResolveLinearMove(
		Clear, FRTCellId(0, 0), FRTCellId(2, 1), 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("destinazione fuori asse entro la portata: arriva"),
		Oblique.Final == FRTCellId(2, 1) && Oblique.Stop == ERTLinearStop::Completed);
	TestTrue(TEXT("lungo la retta di HexLine, non lungo un percorso a scelta"),
		Oblique.Entered == MoveActionHexLineTail(FRTCellId(0, 0), FRTCellId(2, 1)));

	// Oltre la portata dichiarata non si va, nemmeno in linea retta.
	const FRTLinearMoveResult TooFar = URTMovementActionLibrary::ResolveLinearMove(
		Clear, FRTCellId(0, 0), FRTCellId(5, 0), 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("fuori portata: non parte"), TooFar.Final == FRTCellId(0, 0));
	return true;
}

// Il terreno che nega lo scatto NON e' un muro, e il TurnLog deve poterlo dire. Fino al 2026-09-08 le due
// cose condividevano `ERTLinearStop::BlockedByTerrain` — il commento del resolver lo dichiarava («muro,
// bordo mappa, o terreno che nega lo scatto») — e a valle nessuno poteva distinguerle.
//
// ⚠️ Misurato sulla seduta `U46`: la carica di `R_ROU` rifiutata dal rough usciva nel log come
// `Branth: resta (...)`, identica alle unita' che non avevano pianificato nulla. Questo test pinna la
// distinzione alla sorgente; che la riga poi ARRIVI al log lo verifica la partita, non l'unita' di misura.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashDeniedByTerrainTest,
	"RefactorTactics.Actions.Dash.DeniedByTerrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FRTDashDeniedByTerrainTest::RunTest(const FString&)
{
	const TMap<FRTCellId, int32> Empty;
	const TSet<int32> NoHostiles;

	// (1) Il rough sulla traiettoria ferma lo scatto CON UN MOTIVO PROPRIO.
	URTHexMapAsset* Rough = MakeMoveActionMap();
	MakeRough(Rough, FRTCellId(2, 0));
	const FRTLinearMoveResult Denied = URTMovementActionLibrary::ResolveLinearMove(
		Rough, FRTCellId(0, 0), FRTCellId(3, 0), /*MaxCells*/ 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);

	TestTrue(TEXT("si ferma prima del rough"), Denied.Final == FRTCellId(1, 0));
	TestTrue(TEXT("e il motivo e' il TERRENO CHE NEGA LO SCATTO, non un muro"),
		Denied.Stop == ERTLinearStop::TerrainDeniesDash);

	// (2) CONTROLLO DIFFERENZIALE: il muro, sulla stessa cella, produce l'ALTRO motivo. Senza questa meta'
	// il test resterebbe verde anche se `StepBlockReason` restituisse sempre `TerrainDeniesDash`.
	URTHexMapAsset* Wall = MakeMoveActionMap();
	MakeWall(Wall, FRTCellId(2, 0));
	const FRTLinearMoveResult Blocked = URTMovementActionLibrary::ResolveLinearMove(
		Wall, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("il muro resta `BlockedByTerrain`"), Blocked.Stop == ERTLinearStop::BlockedByTerrain);
	TestTrue(TEXT("i due motivi sono DIVERSI"), Denied.Stop != Blocked.Stop);

	// (3) La differenza che i due motivi esistono per raccontare: dal rough ci si passa A PIEDI. Se questo
	// cadesse, distinguerlo dal muro non insegnerebbe piu' niente — sarebbe un muro con un altro nome.
	const FRTHexCellData* RoughCell = Rough->FindCell(FRTCellId(2, 0));
	TestTrue(TEXT("la cella rough esiste e NON blocca il movimento"),
		RoughCell != nullptr && !RoughCell->bBlocksMovement);

	// (4) Il SALTO scavalca le celle intermedie: il rough non lo riguarda, e atterrarci resta permesso.
	const FRTLinearMoveResult Leap = URTMovementActionLibrary::ResolveLinearMove(
		Rough, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearLeap, Empty, NoHostiles);
	TestTrue(TEXT("il salto passa sopra il rough"),
		Leap.Final == FRTCellId(3, 0) && Leap.Stop == ERTLinearStop::Completed);

	// (5) Il narratore NON deve cadere nel default. Un motivo nuovo che l'enum porta ma la traduzione non
	// conosce produce «non eseguibile» — cioe' esattamente la riga muta che questo lavoro rimuove, spostata
	// di un livello. Si asserisce il TESTO, perche' e' il testo che la voce `PIE-V01-LOG` legge.
	const FString Detto = URTTurnLogLibrary::DescribeInvalidReason(ERTActionInvalidReason::TerrainDeniesDash);
	TestNotEqual(TEXT("il motivo e' tradotto, non generico"), Detto, FString(TEXT("non eseguibile")));
	TestTrue(TEXT("e nomina il terreno"), Detto.Contains(TEXT("terreno")));
	TestTrue(TEXT("e dice che a piedi si passa"), Detto.Contains(TEXT("corsa")));

	// 🔑 **Si ITERA l'enum, non si elencano a mano i motivi che ricordo.** Una lista scritta a mano protegge
	// solo cio' che l'autore aveva in mente il giorno in cui l'ha scritta, e il prossimo valore aggiunto in
	// coda nasce muto senza che nulla diventi rosso — che e' la stessa forma del difetto chiuso da questo
	// lavoro un livello piu' su.
	//
	// ⚠️ **E il giro completo trova TRE buchi che precedono questa PR**: `DoorLocked`, `DoorDestroyed` e
	// `InsufficientMovementPoints` non hanno un `case` in `DescribeInvalidReason` e rendono tutti e tre
	// «non eseguibile» — proprio la riga muta che il ramo `Fallback` esiste per non produrre. Sono
	// dichiarati qui come attesi finche' chi possiede porte e movimento non li traduce: il test **non li
	// assolve**, li nomina.
	//
	// 🔑 **Il terzo l'ha trovato questa iterazione, non una lettura.** La stesura a lista chiusa ne
	// enumerava due, ed erano i due che una code review aveva individuato leggendo lo `switch`: il giro
	// sull'enum ne ha misurato uno in piu'. E' la differenza fra proteggere cio' che qualcuno ricorda e
	// misurare cio' che c'e'.
	const UEnum* Motivi = StaticEnum<ERTActionInvalidReason>();
	if (TestNotNull(TEXT("l'enum dei motivi e' riflesso"), Motivi))
	{
		const TSet<FString> BuchiNoti = {
			TEXT("DoorLocked"), TEXT("DoorDestroyed"), TEXT("InsufficientMovementPoints") };
		TArray<FString> Muti;
		for (int32 I = 0; I < Motivi->NumEnums() - 1; ++I) // -1: salta il `_MAX` sintetico
		{
			const FString Nome = Motivi->GetNameStringByIndex(I);
			if (Nome == TEXT("None")) { continue; } // `None` = azione eseguibile: non ha una frase
			const auto Valore = static_cast<ERTActionInvalidReason>(Motivi->GetValueByIndex(I));
			if (URTTurnLogLibrary::DescribeInvalidReason(Valore) == TEXT("non eseguibile"))
			{
				Muti.Add(Nome);
			}
		}
		Muti.Sort();
		TArray<FString> Attesi = BuchiNoti.Array();
		Attesi.Sort();
		TestEqual(TEXT("gli unici motivi senza traduzione sono i due delle porte, gia' muti prima di questa PR"),
			FString::Join(Muti, TEXT(", ")), FString::Join(Attesi, TEXT(", ")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChargeStopsOnImpactTest,
	"RefactorTactics.Actions.Charge.StopsOnImpact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChargeStopsOnImpactTest::RunTest(const FString&)
{
	// La carica si arresta ADDOSSO al primo nemico incontrato e lo segnala: il colpo (20 danni + spinta 1) lo
	// applica poi il Blast, perche' l'impatto e' controllo — codice 20/30 del catalogo.
	URTHexMapAsset* Map = MakeMoveActionMap();

	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(FRTCellId(2, 0), /*UnitId*/ 4);
	TSet<int32> Hostiles;
	Hostiles.Add(4);

	const FRTLinearMoveResult Impact = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), /*MaxCells*/ 3, ERTMovementStyle::LinearCharge, Occupancy, Hostiles);

	TestTrue(TEXT("si ferma davanti al nemico, non lo attraversa"), Impact.Final == FRTCellId(1, 0));
	TestTrue(TEXT("l'esito e' l'impatto, non un blocco qualsiasi"), Impact.Stop == ERTLinearStop::Impact);
	TestEqual(TEXT("e dice CHI ha urtato"), Impact.ImpactUnitId, 4);

	// Un ALLEATO sulla traiettoria non e' un bersaglio: la carica si ferma, ma non colpisce nessuno.
	TSet<int32> NoHostiles;
	const FRTLinearMoveResult Friend = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearCharge, Occupancy, NoHostiles);
	TestTrue(TEXT("davanti a un alleato ci si ferma senza impatto"), Friend.Stop == ERTLinearStop::BlockedByUnit);
	TestEqual(TEXT("nessuno da colpire"), Friend.ImpactUnitId, static_cast<int32>(INDEX_NONE));

	// Senza nessuno sulla strada, la carica e' uno scatto come gli altri.
	const TMap<FRTCellId, int32> Empty;
	const FRTLinearMoveResult Missed = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearCharge, Empty, Hostiles);
	TestTrue(TEXT("a vuoto: arriva e non colpisce"),
		Missed.Final == FRTCellId(3, 0) && Missed.ImpactUnitId == INDEX_NONE);

	// Un muro interrompe la carica prima del bersaglio: niente impatto attraverso la parete.
	URTHexMapAsset* Walled = MakeMoveActionMap();
	MakeWall(Walled, FRTCellId(1, 0));
	const FRTLinearMoveResult Wall = URTMovementActionLibrary::ResolveLinearMove(
		Walled, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearCharge, Occupancy, Hostiles);
	TestTrue(TEXT("il muro ferma la carica"), Wall.Stop == ERTLinearStop::BlockedByTerrain);
	TestEqual(TEXT("e non c'e' impatto"), Wall.ImpactUnitId, static_cast<int32>(INDEX_NONE));
	return true;
}

/**
 * [D-516] (`#3736`) — **una CARICA col bersaglio in linea ma oltre la portata corre per tutta la portata e si
 * ferma**, e solo la carica: per ogni altro stile un bersaglio oltre la portata resta `NotAligned`.
 *
 * 🔑 L'oracolo e' la COPPIA carica / scatto sulla stessa geometria: un test che guardasse solo la carica
 * passerebbe anche se la regola valesse per tutti gli stili.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTChargeBeyondRangeRunsToRangeTest,
	"RefactorTactics.Actions.Charge.BeyondRangeRunsToRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTChargeBeyondRangeRunsToRangeTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMoveActionMap();
	const TMap<FRTCellId, int32> Nessuno;
	TSet<int32> Hostiles;
	Hostiles.Add(4);

	// Bersaglio a 5 celle, portata 3.
	const FRTLinearMoveResult Corta = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(5, 0), /*MaxCells*/ 3, ERTMovementStyle::LinearCharge, Nessuno, Hostiles);
	TestTrue(TEXT("la carica corre fino all'ultima cella della portata"), Corta.Final == FRTCellId(3, 0));
	TestTrue(TEXT("e l'esito dice che il bersaglio era oltre"), Corta.Stop == ERTLinearStop::OutOfRange);
	TestEqual(TEXT("tre celle percorse"), Corta.Entered.Num(), 3);
	TestEqual(TEXT("nessun impatto"), Corta.ImpactUnitId, static_cast<int32>(INDEX_NONE));
	TestFalse(TEXT("e non e' un arrivo: IsLinearReachable resta falso"), URTMovementActionLibrary::IsLinearReachable(
		Map, FRTCellId(0, 0), FRTCellId(5, 0), 3, ERTMovementStyle::LinearCharge, Nessuno, Hostiles));

	// Controllo: lo SCATTO sulla stessa geometria non parte, come prima.
	const FRTLinearMoveResult Scatto = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(5, 0), 3, ERTMovementStyle::LinearDash, Nessuno, Hostiles);
	TestTrue(TEXT("lo scatto oltre la portata resta NotAligned"), Scatto.Stop == ERTLinearStop::NotAligned);
	TestTrue(TEXT("e non si muove"), Scatto.Final == FRTCellId(0, 0) && Scatto.Entered.Num() == 0);

	// Un nemico sulla linea ENTRO la portata e' un impatto, come per un bersaglio in portata.
	TMap<FRTCellId, int32> Davanti;
	Davanti.Add(FRTCellId(2, 0), 4);
	const FRTLinearMoveResult Urto = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(5, 0), 3, ERTMovementStyle::LinearCharge, Davanti, Hostiles);
	TestTrue(TEXT("un nemico entro la portata ferma la carica con un impatto"), Urto.Stop == ERTLinearStop::Impact);
	TestEqual(TEXT("e dice chi"), Urto.ImpactUnitId, 4);

	// Uno OLTRE la portata non la riguarda: la corsa finisce prima.
	TMap<FRTCellId, int32> Oltre;
	Oltre.Add(FRTCellId(4, 0), 4);
	const FRTLinearMoveResult Lontano = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(5, 0), 3, ERTMovementStyle::LinearCharge, Oltre, Hostiles);
	TestTrue(TEXT("un nemico oltre la portata non e' un impatto"),
		Lontano.Stop == ERTLinearStop::OutOfRange && Lontano.ImpactUnitId == INDEX_NONE);

	// Un muro entro la portata la ferma come sempre.
	URTHexMapAsset* Walled = MakeMoveActionMap();
	MakeWall(Walled, FRTCellId(2, 0));
	const FRTLinearMoveResult Muro = URTMovementActionLibrary::ResolveLinearMove(
		Walled, FRTCellId(0, 0), FRTCellId(5, 0), 3, ERTMovementStyle::LinearCharge, Nessuno, Hostiles);
	TestTrue(TEXT("un muro entro la portata ferma la carica davanti"),
		Muro.Stop == ERTLinearStop::BlockedByTerrain && Muro.Final == FRTCellId(1, 0));
	return true;
}

/**
 * [D-516] — **«c'e' un percorso lineare fino a lui?»**: la retta e' libera dal terreno a qualunque distanza, e le
 * unita' non contano. E' la domanda che la pianificazione fa per una carica oltre la portata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLineOpenToTargetTest,
	"RefactorTactics.Actions.Charge.LineOpenToTargetIgnoresRangeAndUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLineOpenToTargetTest::RunTest(const FString&)
{
	URTHexMapAsset* Aperta = MakeMoveActionMap();
	TestTrue(TEXT("una retta libera di 5 celle e' aperta, senza portata da rispettare"),
		URTMovementActionLibrary::IsLineOpenToTarget(Aperta, FRTCellId(0, 0), FRTCellId(5, 0)));
	// ⌫ *Fino a [D-524] una cella fuori asse «non aveva una retta».* Ora ce l'ha: ogni cella dello stesso piano sta
	// su una retta fra i centri, e a chiuderla sono le celle e i lati. Restano senza retta la cella stessa e un altro piano.
	TestTrue(TEXT("una cella fuori asse ha una retta, ed e' aperta"),
		URTMovementActionLibrary::IsLineOpenToTarget(Aperta, FRTCellId(0, 0), FRTCellId(2, 1)));
	TestFalse(TEXT("la cella stessa non ha una retta da percorrere"),
		URTMovementActionLibrary::IsLineOpenToTarget(Aperta, FRTCellId(0, 0), FRTCellId(0, 0)));
	TestFalse(TEXT("un altro piano non e' mai in linea"),
		URTMovementActionLibrary::IsLineOpenToTarget(Aperta, FRTCellId(0, 0, 0), FRTCellId(2, 1, 1)));

	// Un bordo con copertura alta CHE LA RETTA ATTRAVERSA la chiude, quanto un muro. La retta (0,0)->(2,1) e'
	// (0,0), (1,0), (1,1), (2,1): il bordo (1,0)-(1,1) e' suo, il bordo (1,0)-(2,0) no.
	URTHexMapAsset* BordoFuoriRetta = MakeMoveActionMap();
	PutMoveActionCover(BordoFuoriRetta, FRTCellId(1, 0), MoveActionEdgeToward(FRTCellId(1, 0), FRTCellId(2, 0)),
		ERTHexCoverType::High);
	TestTrue(TEXT("controllo: un bordo alto che la retta non attraversa la lascia aperta"),
		URTMovementActionLibrary::IsLineOpenToTarget(BordoFuoriRetta, FRTCellId(0, 0), FRTCellId(2, 1)));
	URTHexMapAsset* BordoSullaRetta = MakeMoveActionMap();
	PutMoveActionCover(BordoSullaRetta, FRTCellId(1, 0), MoveActionEdgeToward(FRTCellId(1, 0), FRTCellId(1, 1)),
		ERTHexCoverType::High);
	TestFalse(TEXT("un bordo alto fra (1,0) e (1,1), che la retta attraversa, la chiude"),
		URTMovementActionLibrary::IsLineOpenToTarget(BordoSullaRetta, FRTCellId(0, 0), FRTCellId(2, 1)));

	URTHexMapAsset* ConMuro = MakeMoveActionMap();
	MakeWall(ConMuro, FRTCellId(4, 0));
	TestFalse(TEXT("un muro sulla retta la chiude, anche oltre una portata qualsiasi"),
		URTMovementActionLibrary::IsLineOpenToTarget(ConMuro, FRTCellId(0, 0), FRTCellId(5, 0)));

	URTHexMapAsset* Accidentata = MakeMoveActionMap();
	MakeRough(Accidentata, FRTCellId(5, 0));
	TestFalse(TEXT("il terreno che nega la corsa la chiude, cella del bersaglio compresa"),
		URTMovementActionLibrary::IsLineOpenToTarget(Accidentata, FRTCellId(0, 0), FRTCellId(5, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLeapIgnoresIntermediateTest,
	"RefactorTactics.Actions.Leap.IgnoresIntermediateCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLeapIgnoresIntermediateTest::RunTest(const FString&)
{
	// Il salto scavalca cio' che sta in mezzo — unita' e coperture basse — ma la cella d'atterraggio la
	// SUBISCE: dev'essere percorribile e libera.
	URTHexMapAsset* Map = MakeMoveActionMap();

	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(FRTCellId(1, 0), 2);
	Occupancy.Add(FRTCellId(2, 0), 3);
	const TSet<int32> NoHostiles;

	const FRTLinearMoveResult Over = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), /*MaxCells*/ 3, ERTMovementStyle::LinearLeap, Occupancy, NoHostiles);

	TestTrue(TEXT("atterra oltre le due unita'"), Over.Final == FRTCellId(3, 0));
	TestTrue(TEXT("l'esito e' completato"), Over.Stop == ERTLinearStop::Completed);
	TestEqual(TEXT("nessuna cella intermedia attraversata: solo l'atterraggio"), Over.Entered.Num(), 1);

	// Confronto che rende esplicita la differenza: uno scatto normale, sulla stessa mappa, non passa.
	const FRTLinearMoveResult Dash = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearDash, Occupancy, NoHostiles);
	TestTrue(TEXT("lo scatto invece si ferma subito"), Dash.Final == FRTCellId(0, 0));

	// Atterraggio occupato: il salto non parte (non si atterra addosso a qualcuno).
	TMap<FRTCellId, int32> Landing = Occupancy;
	Landing.Add(FRTCellId(3, 0), 5);
	const FRTLinearMoveResult Busy = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearLeap, Landing, NoHostiles);
	TestTrue(TEXT("atterraggio occupato: si resta"), Busy.Final == FRTCellId(0, 0));
	TestTrue(TEXT("col motivo giusto"), Busy.Stop == ERTLinearStop::BlockedByUnit);

	// Atterraggio dentro un muro: idem, e il motivo e' il terreno.
	URTHexMapAsset* Walled = MakeMoveActionMap();
	MakeWall(Walled, FRTCellId(3, 0));
	const FRTLinearMoveResult IntoWall = URTMovementActionLibrary::ResolveLinearMove(
		Walled, FRTCellId(0, 0), FRTCellId(3, 0), 3, ERTMovementStyle::LinearLeap, Occupancy, NoHostiles);
	TestTrue(TEXT("non si atterra dentro un muro"), IntoWall.Stop == ERTLinearStop::BlockedByTerrain);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSprintNoReactionTest,
	"RefactorTactics.Actions.Sprint.NoReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSprintNoReactionTest::RunTest(const FString&)
{
	// Chi corre a perdifiato non para: `Sprint` e' l'unica mobilita' della v0.1 che nega la reazione, e lo
	// dichiara nei DATI. A farla valere sara' E5, che introduce lo slot reazione; qui si fissa la regola.
	const FRTActionDef Sprint = URTCatalogLibrary::FindCoreAction(TEXT("Action.Sprint"));
	if (!TestTrue(TEXT("Action.Sprint e' nel catalogo"), Sprint.ActionId == FName(TEXT("Action.Sprint"))))
	{
		return false;
	}
	TestFalse(TEXT("lo Sprint non consente reazioni"), Sprint.bAllowsReaction);

	// Le altre mobilita' invece la consentono: la differenza e' il punto: se tutte la negassero, il dato non
	// distinguerebbe nulla.
	const TCHAR* Others[] = { TEXT("Action.Dodge"), TEXT("Action.Charge"), TEXT("Action.Leap"),
		TEXT("Action.Reposition"), TEXT("Action.Move"), TEXT("Action.Wait") };
	for (const TCHAR* Id : Others)
	{
		const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(Id));
		TestTrue(FString::Printf(TEXT("%s consente la reazione"), Id), Def.bAllowsReaction);
	}

	// E dopo D-028 negare la reazione e' rimasto il suo prezzo PRINCIPALE, non uno in piu': lo Sprint occupa
	// il solo slot movimento come `Move`, quindi cio' che lo distingue da 3 MP in piu' e' esattamente questa
	// riga piu' `Status.Exposed`. Se al playtest non basta, e' un `Move` migliore — l'upgrade puro che D-015
	// vieta (voce `BAL-1` del backlog).
	TestTrue(TEXT("Sprint spende il solo movimento"), Sprint.Slot == ERTActionSlot::Movement);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementCatalogTest,
	"RefactorTactics.Actions.MovementActionsDeclareStyleAndPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementCatalogTest::RunTest(const FString&)
{
	// Ogni mobilita' dichiara IN DATI la macro-fase (ADR-0003 §3) e il modo in cui si sposta. `Dodge`,
	// `Charge`, `Leap` e `Reposition` risolvono nel Dash; `Move`, `Sprint` e `Withdraw` nel Move, dopo il
	// Blast — ed e' la divisione che [D-015] chiama strutturale, non un elenco da contare.
	struct FExpected { const TCHAR* Id; ERTMatchPhase Macro; ERTMovementStyle Style; int32 Range; int32 Cooldown; };
	const FExpected Expected[] = {
		// ⚠️ **`Move` e non `Dash` dal 2026-09-12** ([D-116] voce 1, `#641`): lo Sprint e' un profilo della
		// famiglia `Move` e risolve dopo il Blast. Lo stile era gia' `Budget` — i due terzi della
		// migrazione che [D-068] aveva misurato come gia' avvenuti.
		// ⚠️ `Range` **0** per le azioni a budget: il loro numero e' del profilo ([D-427]), non dell'azione.
		// Che valga per OGNI azione a quello stile — e non per quelle elencate qui — lo asserisce
		// `RefactorTactics.Catalog.BudgetActionsDeclareNoRange`, che cicla sul catalogo.
		{ TEXT("Action.Sprint"),     ERTMatchPhase::Move, ERTMovementStyle::Budget,       0, 0 },
		{ TEXT("Action.Dodge"),       ERTMatchPhase::Dash, ERTMovementStyle::LinearDash,   3, 1 },
		{ TEXT("Action.Charge"),     ERTMatchPhase::Dash, ERTMovementStyle::LinearCharge, 3, 2 },
		{ TEXT("Action.Leap"),       ERTMatchPhase::Dash, ERTMovementStyle::LinearLeap,   3, 2 },
		{ TEXT("Action.Reposition"), ERTMatchPhase::Dash, ERTMovementStyle::LinearDash,   2, 1 },
		{ TEXT("Action.Move"),       ERTMatchPhase::Move, ERTMovementStyle::Budget,       0, 0 },
		// 🔴 **`Withdraw` mancava da questa tabella**, e con essa la sua macro-fase, il suo stile e il suo
		// fallback: la tabella dichiara di coprire «ogni mobilita'» e ne saltava una. Trovato dalla code
		// review di #3204.
		{ TEXT("Action.Withdraw"),   ERTMatchPhase::Move, ERTMovementStyle::Budget,       0, 0 },
	};

	for (const FExpected& E : Expected)
	{
		const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(E.Id));
		if (!TestTrue(FString::Printf(TEXT("%s e' nel catalogo"), E.Id), Def.ActionId == FName(E.Id))) { continue; }

		TestTrue(FString::Printf(TEXT("%s: macro-fase dichiarata"), E.Id),
			URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == E.Macro);
		TestTrue(FString::Printf(TEXT("%s: stile di movimento"), E.Id), Def.MovementStyle == E.Style);
		TestEqual(FString::Printf(TEXT("%s: portata"), E.Id), Def.RangeCells, E.Range);
		TestEqual(FString::Printf(TEXT("%s: cooldown"), E.Id), Def.CooldownTurns, E.Cooldown);
		TestTrue(FString::Printf(TEXT("%s: fallback Stop (ci si ferma, non si annulla)"), E.Id),
			Def.Fallback == ERTActionFallback::Stop);
	}

	// La carica porta con se' i suoi effetti: 20 danni e una spinta di 1, dichiarati come dati.
	const FRTActionDef Charge = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	TestEqual(TEXT("la carica dichiara due effetti"), Charge.Effects.Num(), 2);
	if (Charge.Effects.Num() == 2)
	{
		TestTrue(TEXT("20 danni"), Charge.Effects[0].Effect == ERTActionEffect::Damage && Charge.Effects[0].Amount == 20);
		TestTrue(TEXT("e una spinta di 1"), Charge.Effects[1].Effect == ERTActionEffect::Push && Charge.Effects[1].Amount == 1);
	}

	// `Reposition` non applica `Exposed`: e' la differenza con lo Sprint, oltre alla distanza.
	TestEqual(TEXT("Reposition non applica stati"),
		URTCatalogLibrary::FindCoreAction(TEXT("Action.Reposition")).Effects.Num(), 0);

	// Solo le mobilita' lineari passano da ResolveLinearMove.
	TestTrue(TEXT("il budget non e' lineare"), !URTMovementActionLibrary::IsLinear(ERTMovementStyle::Budget));
	TestTrue(TEXT("lo scatto lo e'"), URTMovementActionLibrary::IsLinear(ERTMovementStyle::LinearDash));
	return true;
}

/**
 * Guardia contro il buco di #142: `ERTMovementStyle` ha `None` come default, quindi un'azione di mobilita'
 * che si dimentica di dichiararlo NON e' un errore di compilazione — e' uno scatto che, in silenzio, torna a
 * usare il pathfinding del movimento normale (aggira gli ostacoli, attraversa `Rough`, sale di layer).
 *
 * La regola vale per ENTRAMBI i cataloghi rimasti — quello generico (`Action.*`) e quello degli EROI. Dal
 * 2026-08-10 non ce n'e' un terzo: il catalogo dei due archetipi legacy e' stato rimosso, e con esso l'unico
 * percorso di configurazione che nessuna partita esercitava. `URTHeroData` e' cio' che il GameMode schiera
 * davvero — se la guardia guardasse altrove sarebbe verde mentre il buco e' aperto proprio dove il gioco passa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFastMovementDeclaresStyleTest,
	"RefactorTactics.Actions.EveryFastMovementDeclaresStyle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFastMovementDeclaresStyleTest::RunTest(const FString&)
{
	TArray<FRTActionDef> All = URTCatalogLibrary::GetCoreActionCatalog();
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }
		for (const URTActionData* Action : Hero->Actions)
		{
			if (Action) { All.Add(Action->Def); }
		}
	}

	int32 FastMovers = 0;
	for (const FRTActionDef& Def : All)
	{
		if (URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) != ERTMatchPhase::Dash)
		{
			continue;
		}
		++FastMovers;
		TestTrue(FString::Printf(TEXT("%s risolve nel Dash: deve dichiarare COME si sposta"), *Def.ActionId.ToString()),
			Def.MovementStyle != ERTMovementStyle::None);
	}

	// Se il filtro non trovasse nulla, il test passerebbe senza provare niente.
	TestTrue(TEXT("il catalogo contiene mobilita' rapide da verificare"), FastMovers > 0);

	// Le mobilita' rapide sono LINEARI: e' il comportamento che CP 4.5 ha introdotto e che il default
	// `None` stava annullando. Le due asserzioni stavano su `Ranger.Dash` e `Guardian.Charge`, cioe' sul
	// catalogo dei due archetipi legacy, rimosso il 2026-08-10: verificavano lo stile su azioni che nessuna
	// partita spediva. Ora stanno sul catalogo generico e sul roster, che sono cio' che il gioco usa.
	const FRTActionDef CoreDodge = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dodge"));
	TestTrue(TEXT("Action.Dodge e' uno scatto lineare"), CoreDodge.MovementStyle == ERTMovementStyle::LinearDash);

	const FRTActionDef CoreCharge = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	TestTrue(TEXT("Action.Charge e' una carica"), CoreCharge.MovementStyle == ERTMovementStyle::LinearCharge);

	// Una carica si ferma ADDOSSO al nemico: senza effetti dichiarati l'impatto sarebbe un contatto da zero
	// danni, cioe' una carica che non carica.
	TestEqual(TEXT("Action.Charge dichiara due effetti"), CoreCharge.Effects.Num(), 2);
	if (CoreCharge.Effects.Num() == 2)
	{
		TestTrue(TEXT("20 danni d'impatto"),
			CoreCharge.Effects[0].Effect == ERTActionEffect::Damage && CoreCharge.Effects[0].Amount == 20);
		TestTrue(TEXT("e una spinta di 1"),
			CoreCharge.Effects[1].Effect == ERTActionEffect::Push && CoreCharge.Effects[1].Amount == 1);
	}

	// L'eroe che quella carica la porta in partita: `Branth.Ram` riusa `Action.Charge` per intero, ed e'
	// il motivo per cui lo stile e' un dato e non un `if` sull'ActionId.
	const URTHeroData* Branth = nullptr;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (Hero && Hero->HeroId == TEXT("Hero.Branth")) { Branth = Hero; break; }
	}
	if (TestNotNull(TEXT("Branth e' nel roster"), Branth))
	{
		const URTActionData* Ram = nullptr;
		for (const URTActionData* A : Branth->Actions)
		{
			if (A && A->Def.ActionId == TEXT("Hero.Branth.Ram")) { Ram = A; break; }
		}
		if (TestNotNull(TEXT("Hero.Branth.Ram e' nel kit"), Ram))
		{
			TestTrue(TEXT("Hero.Branth.Ram e' una carica"), Ram->Def.MovementStyle == ERTMovementStyle::LinearCharge);
		}
	}
	return true;
}

// =====================================================================================================
// MOB-1 — la lama che PASSA ATTRAVERSO.
//
// `LinearDash` si ferma su cio' che incontra, `LinearCharge` si ferma addosso, `LinearLeap` scavalca senza
// toccare. Manca la quarta: attraversare colpendo. `Ivrin.PassingBlade` la descriveva gia' («colpisce per
// 20 le unita' ATTRAVERSATE») ma usava `LinearDash`, quindi non attraversava nessuno e quei 20 danni non
// avevano un momento in cui applicarsi.
//
// Stile NUOVO invece di cambiare `LinearDash`: quello e' condiviso con `Action.Dodge` e `Action.Reposition`,
// dove fermarsi davanti a un nemico e' il comportamento giusto. Cambiarlo li' avrebbe fatto passare ogni
// scatto attraverso le linee avversarie.
// =====================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLinearPassTest,
	"RefactorTactics.Movement.PassGoesThroughUnitsAndRecordsThem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLinearPassTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMoveActionMap();

	// Due unita' sulla traiettoria fra (0,0) e (4,0): 7 e 9, in celle diverse.
	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(FRTCellId(1, 0, 0), 7);
	Occupancy.Add(FRTCellId(3, 0, 0), 9);
	const TSet<int32> Hostiles = { 7, 9 };

	const FRTLinearMoveResult Pass = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0, 0), FRTCellId(4, 0, 0), /*MaxCells*/ 4,
		ERTMovementStyle::LinearPass, Occupancy, Hostiles);

	TestTrue(TEXT("la lama arriva a destinazione"), Pass.Final == FRTCellId(4, 0, 0));
	TestEqual(TEXT("e lo dichiara come arrivo, non come blocco"),
		static_cast<int32>(Pass.Stop), static_cast<int32>(ERTLinearStop::Completed));
	TestEqual(TEXT("ha attraversato ENTRAMBE le unita'"), Pass.PassedThroughUnitIds.Num(), 2);
	TestTrue(TEXT("e sa quali"),
		Pass.PassedThroughUnitIds.Contains(7) && Pass.PassedThroughUnitIds.Contains(9));

	// Il confronto che rende il test discriminante: lo STESSO percorso con lo stile vecchio si ferma subito.
	const FRTLinearMoveResult Dash = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0, 0), FRTCellId(4, 0, 0), /*MaxCells*/ 4,
		ERTMovementStyle::LinearDash, Occupancy, Hostiles);
	TestTrue(TEXT("lo scatto normale invece si ferma davanti alla prima"), Dash.Final == FRTCellId(0, 0, 0));
	TestEqual(TEXT("e non attraversa nessuno"), Dash.PassedThroughUnitIds.Num(), 0);

	// La cella d'ARRIVO resta un vincolo: si passa ATTRAVERSO, non ci si ferma dentro qualcuno.
	TMap<FRTCellId, int32> Occupied;
	Occupied.Add(FRTCellId(4, 0, 0), 5);
	const FRTLinearMoveResult OntoUnit = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0, 0), FRTCellId(4, 0, 0), /*MaxCells*/ 4,
		ERTMovementStyle::LinearPass, Occupied, TSet<int32>{ 5 });
	TestFalse(TEXT("non si atterra sopra un'unita'"), OntoUnit.Final == FRTCellId(4, 0, 0));

	// Gli stessi identici parametri della partita (Probe.PassingBlade): se questo passa e lo scenario no, il
	// difetto non e' qui.
	TMap<FRTCellId, int32> Mid;
	Mid.Add(FRTCellId(0, 0, 0), 1);
	const FRTLinearMoveResult AsInMatch = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(-2, 0, 0), FRTCellId(1, 0, 0), /*MaxCells*/ 3,
		ERTMovementStyle::LinearPass, Mid, TSet<int32>{ 1 });
	TestTrue(TEXT("coi parametri della partita la lama arriva"), AsInMatch.Final == FRTCellId(1, 0, 0));
	TestEqual(TEXT("e registra chi ha attraversato"), AsInMatch.PassedThroughUnitIds.Num(), 1);

	// E un muro ferma anche la lama: attraversare le UNITA' non vuol dire attraversare la roccia.
	MakeWall(Map, FRTCellId(2, 0, 0));
	const FRTLinearMoveResult Walled = URTMovementActionLibrary::ResolveLinearMove(
		Map, FRTCellId(0, 0, 0), FRTCellId(4, 0, 0), /*MaxCells*/ 4,
		ERTMovementStyle::LinearPass, Occupancy, Hostiles);
	TestEqual(TEXT("il muro ferma la lama"),
		static_cast<int32>(Walled.Stop), static_cast<int32>(ERTLinearStop::BlockedByTerrain));
	TestEqual(TEXT("e conta solo chi ha attraversato PRIMA del muro"), Walled.PassedThroughUnitIds.Num(), 1);

	return true;
}

// =====================================================================================================
// [D-523] / [D-524] (#3793) — «in linea» e' la RETTA LIBERA fra i centri, non una delle sei direzioni.
//
// La retta e' la sequenza di celle di `URTHexLibrary::HexLine` con il suo spareggio intero; la portata e' la
// distanza esagonale; la fermano le celle E i lati (`URTHexCoverLibrary::BlocksTraversal`) per `Dash`, `Charge`
// e `Pass`, mentre il `Leap` guarda solo l'atterraggio. Gli attacchi lineari restano sulle sei direzioni.
//
// 🔑 **L'oracolo e' DOPPIO, e non per scrupolo.** Ogni test confronta il risultato con `HexLine` (cosi' una retta
// riscritta a parte cade) E con celle scritte a mano ricavate da una copia indipendente dell'algoritmo (cosi' un
// `HexLine` cambiato sotto i piedi cade anche lui, invece di trascinarsi dietro il test).
// =====================================================================================================

/**
 * La scena di `PIE-V01-CHARGECLICK` (4): Branth in `(-4,3)`, Muiren in `(-2,4)`, scarto `(+2,+1)`, distanza 3.
 * La retta passa per `(-3,3)` e `(-3,4)`: ESATTAMENTE le celle che l'autore ha indicato davanti alla tavola `U92-V5`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveLineOffAxisTest,
	"RefactorTactics.Actions.Dash.OffAxisFollowsTheHexLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveLineOffAxisTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMoveActionMap();
	const TMap<FRTCellId, int32> Empty;
	const TSet<int32> NoHostiles;

	const FRTCellId Branth(-4, 3), Muiren(-2, 4);
	const TArray<FRTCellId> Attese = { FRTCellId(-3, 3), FRTCellId(-3, 4), FRTCellId(-2, 4) };

	int32 Distanza = 0;
	TestFalse(TEXT("premessa: lo scarto (+2,+1) NON e' su una delle sei direzioni"),
		URTMovementActionLibrary::IsStraightLine(Branth, Muiren, Distanza));
	TestEqual(TEXT("premessa: la distanza e' 3, quanto la portata di Hero.Branth.Ram"),
		URTHexLibrary::HexDistance(Branth, Muiren), 3);
	TestTrue(TEXT("premessa: la retta di HexLine e' quella dell'autore"),
		MoveActionHexLineTail(Branth, Muiren) == Attese);

	// Lo scatto arriva, lungo la retta.
	const FRTLinearMoveResult Dash = URTMovementActionLibrary::ResolveLinearMove(
		Map, Branth, Muiren, /*MaxCells*/ 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("lo scatto arriva su Muiren"), Dash.Final == Muiren && Dash.Stop == ERTLinearStop::Completed);
	TestTrue(TEXT("lungo (-3,3) e (-3,4)"), Dash.Entered == Attese);

	// La carica: si ferma ADIACENTE al nemico e lo colpisce, e l'adiacente e' l'ultimo passo della retta.
	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(Muiren, 4);
	const TSet<int32> Hostiles = { 4 };
	const FRTLinearMoveResult Charge = URTMovementActionLibrary::ResolveLinearMove(
		Map, Branth, Muiren, 3, ERTMovementStyle::LinearCharge, Occupancy, Hostiles);
	TestTrue(TEXT("la carica urta Muiren"), Charge.Stop == ERTLinearStop::Impact && Charge.ImpactUnitId == 4);
	TestTrue(TEXT("fermandosi in (-3,4), davanti a lui"), Charge.Final == FRTCellId(-3, 4));
	TestTrue(TEXT("dopo (-3,3) e (-3,4)"), Charge.Entered == TArray<FRTCellId>{ FRTCellId(-3, 3), FRTCellId(-3, 4) });
	TestFalse(TEXT("non e' un caso di oltre-portata"), Charge.bTargetBeyondRange);

	// Il salto atterra su qualunque cella libera entro la portata, e non attraversa niente.
	const FRTLinearMoveResult Leap = URTMovementActionLibrary::ResolveLinearMove(
		Map, Branth, Muiren, 3, ERTMovementStyle::LinearLeap, Empty, NoHostiles);
	TestTrue(TEXT("il salto atterra su Muiren"), Leap.Final == Muiren && Leap.Stop == ERTLinearStop::Completed);
	TestTrue(TEXT("senza attraversare le celle in mezzo"), Leap.Entered == TArray<FRTCellId>{ Muiren });

	// La lama attraversa chi sta sulla retta: stesse celle, stessi attraversati.
	TMap<FRTCellId, int32> Mezzo;
	Mezzo.Add(FRTCellId(-3, 3), 5);
	const FRTLinearMoveResult Pass = URTMovementActionLibrary::ResolveLinearMove(
		Map, Branth, Muiren, 3, ERTMovementStyle::LinearPass, Mezzo, TSet<int32>{ 5 });
	TestTrue(TEXT("la lama arriva"), Pass.Final == Muiren && Pass.Stop == ERTLinearStop::Completed);
	TestTrue(TEXT("e attraversa chi stava in (-3,3)"), Pass.PassedThroughUnitIds == TArray<int32>{ 5 });

	// La portata e' la distanza esagonale: a 2 la stessa cella e' fuori, e lo scatto non parte.
	const FRTLinearMoveResult Corta = URTMovementActionLibrary::ResolveLinearMove(
		Map, Branth, Muiren, 2, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("con portata 2 la cella a distanza 3 non si raggiunge"),
		Corta.Final == Branth && Corta.Entered.Num() == 0);
	return true;
}

/**
 * Il PAREGGIO: `(0,0) -> (1,1)` cade esattamente fra due celle, e `HexLine` ne sceglie una con il suo spareggio
 * intero. La scelta decide anche la VALIDITA': con un ostacolo su `(1,0)` la carica e' rifiutata anche se `(0,1)`
 * e' libera, con l'ostacolo su `(0,1)` arriva da `(1,0)` e l'impatto parte da un'altra cella d'arrivo.
 *
 * ⚠️ **Il lato e' fisso nelle coordinate, non rispetto a chi si muove**, quindi non e' simmetrico per rotazione:
 * lo dice il test sul ritorno `(1,1) -> (0,0)`, che ripercorre la stessa retta all'indietro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveLineTieBreakTest,
	"RefactorTactics.Actions.Dash.TieBreakDecidesValidityInBothDirections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveLineTieBreakTest::RunTest(const FString&)
{
	const FRTCellId Origine(0, 0), Pareggio(1, 1);
	const FRTCellId ViaEst(1, 0), ViaSudEst(0, 1);
	const TMap<FRTCellId, int32> Empty;
	const TSet<int32> NoHostiles;

	// Premessa: la scelta e' quella di HexLine, e il test e' discriminante solo se le DUE vie sono celle diverse.
	const TArray<FRTCellId> Retta = URTHexLibrary::HexLine(Origine, Pareggio);
	if (!TestEqual(TEXT("premessa: tre celle, la partenza e due passi"), Retta.Num(), 3)) { return false; }
	TestTrue(TEXT("premessa: HexLine passa da (1,0), non da (0,1)"), Retta[1] == ViaEst);
	TestFalse(TEXT("premessa: l'altra via esiste ed e' una cella diversa"), Retta[1] == ViaSudEst);

	// (1) Libera: arriva da (1,0).
	URTHexMapAsset* Libera = MakeMoveActionMap();
	const FRTLinearMoveResult Arriva = URTMovementActionLibrary::ResolveLinearMove(
		Libera, Origine, Pareggio, 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("a mappa libera arriva su (1,1)"), Arriva.Final == Pareggio && Arriva.Stop == ERTLinearStop::Completed);
	TestTrue(TEXT("passando da (1,0)"), Arriva.Entered == TArray<FRTCellId>{ ViaEst, Pareggio });

	// (2) Ostacolo su (1,0): rifiutato, benche' (0,1) sia libera. ⛔ E' il verso che un'implementazione
	// «sceglie la via libera» rovescerebbe, ed e' la differenza fra la retta e un percorso.
	URTHexMapAsset* MuroEst = MakeMoveActionMap();
	MakeWall(MuroEst, ViaEst);
	const FRTLinearMoveResult Rifiutata = URTMovementActionLibrary::ResolveLinearMove(
		MuroEst, Origine, Pareggio, 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("con un muro su (1,0) lo scatto non parte"), Rifiutata.Final == Origine && Rifiutata.Entered.Num() == 0);
	TestTrue(TEXT("perche' la retta passa da li'"), Rifiutata.Stop == ERTLinearStop::BlockedByTerrain);
	TestFalse(TEXT("e la retta non e' aperta"), URTMovementActionLibrary::IsLineOpenToTarget(MuroEst, Origine, Pareggio));

	// (3) Ostacolo su (0,1): arriva comunque, da (1,0).
	URTHexMapAsset* MuroSudEst = MakeMoveActionMap();
	MakeWall(MuroSudEst, ViaSudEst);
	const FRTLinearMoveResult Indifferente = URTMovementActionLibrary::ResolveLinearMove(
		MuroSudEst, Origine, Pareggio, 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("con un muro su (0,1) lo scatto arriva"), Indifferente.Final == Pareggio);
	TestTrue(TEXT("da (1,0)"), Indifferente.Entered == TArray<FRTCellId>{ ViaEst, Pareggio });
	TestTrue(TEXT("e la retta e' aperta"), URTMovementActionLibrary::IsLineOpenToTarget(MuroSudEst, Origine, Pareggio));

	// (4) La carica su un nemico in (1,1): senza ostacoli urta da (1,0); con un muro su (0,1) urta da (1,0); con un muro
	// su (1,0) e' rifiutata e non urta nessuno.
	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(Pareggio, 4);
	const TSet<int32> Hostiles = { 4 };
	const FRTLinearMoveResult UrtoLibero = URTMovementActionLibrary::ResolveLinearMove(
		Libera, Origine, Pareggio, 3, ERTMovementStyle::LinearCharge, Occupancy, Hostiles);
	TestTrue(TEXT("la carica urta partendo da (1,0)"),
		UrtoLibero.Stop == ERTLinearStop::Impact && UrtoLibero.Final == ViaEst);
	const FRTLinearMoveResult UrtoConMuroSudEst = URTMovementActionLibrary::ResolveLinearMove(
		MuroSudEst, Origine, Pareggio, 3, ERTMovementStyle::LinearCharge, Occupancy, Hostiles);
	TestTrue(TEXT("un muro su (0,1) non cambia da dove urta"),
		UrtoConMuroSudEst.Stop == ERTLinearStop::Impact && UrtoConMuroSudEst.Final == ViaEst);
	const FRTLinearMoveResult UrtoConMuroEst = URTMovementActionLibrary::ResolveLinearMove(
		MuroEst, Origine, Pareggio, 3, ERTMovementStyle::LinearCharge, Occupancy, Hostiles);
	TestTrue(TEXT("un muro su (1,0) la rifiuta"),
		UrtoConMuroEst.Stop == ERTLinearStop::BlockedByTerrain && UrtoConMuroEst.ImpactUnitId == INDEX_NONE);

	// (5) Andata e ritorno coincidono: (1,1) -> (0,0) ripercorre (1,0), non (0,1).
	const FRTLinearMoveResult Ritorno = URTMovementActionLibrary::ResolveLinearMove(
		Libera, Pareggio, Origine, 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("il ritorno passa da (1,0)"), Ritorno.Entered == TArray<FRTCellId>{ ViaEst, Origine });

	// (6) La scelta e' DETERMINISTICA: la stessa domanda ripetuta risponde uguale.
	for (int32 Giro = 0; Giro < 8; ++Giro)
	{
		const FRTLinearMoveResult Ancora = URTMovementActionLibrary::ResolveLinearMove(
			Libera, Origine, Pareggio, 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
		TestTrue(TEXT("la scelta del pareggio non cambia fra due chiamate"), Ancora.Entered == Arriva.Entered);
	}
	return true;
}

/**
 * Chi la segue: tutti e quattro gli stili, e ogni cella entro la portata e' raggiungibile a mappa libera.
 *
 * Le celle a distanza <= R passano da 6R (le sei direzioni) a 3R(R+1): a R = 3, da 18 a 36. Lo dice [D-524] (1)
 * come cambio di bilanciamento DICHIARATO; il test lo fissa perche' non torni indietro in silenzio.
 *
 * E la direzione opposta: le sei direzioni sono un sottoinsieme ESATTO della retta, quindi ogni piano che era
 * valido resta valido e fa lo stesso percorso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveLineEveryCellTest,
	"RefactorTactics.Actions.Dash.EveryCellInRangeIsReachableOnAFreeMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveLineEveryCellTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMoveActionMap(/*Radius=*/ 6);
	const TMap<FRTCellId, int32> Empty;
	const TSet<int32> NoHostiles;
	const FRTCellId Origine(0, 0);
	constexpr int32 Portata = 3;

	const ERTMovementStyle Stili[] = { ERTMovementStyle::LinearDash, ERTMovementStyle::LinearCharge,
		ERTMovementStyle::LinearLeap, ERTMovementStyle::LinearPass };
	for (const ERTMovementStyle Stile : Stili)
	{
		int32 Raggiungibili = 0;
		int32 SulleSeiDirezioni = 0;
		for (const FRTCellId& C : URTHexLibrary::HexArea(Origine, Portata))
		{
			if (C == Origine) { continue; }
			const FRTLinearMoveResult R = URTMovementActionLibrary::ResolveLinearMove(
				Map, Origine, C, Portata, Stile, Empty, NoHostiles);
			if (!(R.Final == C && R.Stop == ERTLinearStop::Completed)) { continue; }
			++Raggiungibili;

			int32 Distanza = 0;
			if (URTMovementActionLibrary::IsStraightLine(Origine, C, Distanza))
			{
				++SulleSeiDirezioni;
				// Il percorso su un asse e' quello di sempre: Origine + k * passo.
				TArray<FRTCellId> Passo;
				const FIntPoint Delta(C.X - Origine.X, C.Y - Origine.Y);
				for (int32 K = 1; K <= Distanza; ++K)
				{
					Passo.Add(FRTCellId(Delta.X / Distanza * K, Delta.Y / Distanza * K, 0));
				}
				if (Stile != ERTMovementStyle::LinearLeap)
				{
					TestTrue(TEXT("su un asse il percorso e' lo stesso di prima"), R.Entered == Passo);
				}
			}
		}
		TestEqual(*FString::Printf(TEXT("stile %d: a portata 3 le celle raggiungibili sono 3R(R+1) = 36"),
			static_cast<int32>(Stile)), Raggiungibili, 36);
		TestEqual(*FString::Printf(TEXT("stile %d: di cui sulle sei direzioni, 6R(R+1)/2 = 18"),
			static_cast<int32>(Stile)), SulleSeiDirezioni, 18);
	}
	return true;
}

/**
 * Il BORDO: copertura alta o porta chiusa o bloccata fermano `Dash`, `Charge` e `Pass` — e non il `Leap`, che guarda
 * solo l'atterraggio. Chiude un `IMPLEMENTATION DRIFT` rispetto a `RT_TestMatrix_v0.1.md` riga 3 anche per le sei
 * direzioni, e il motivo dello stop e' PROPRIO (`BlockedByEdge`), distinto da `BlockedByTerrain`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveLineEdgeTest,
	"RefactorTactics.Actions.Dash.AnEdgeStopsDashChargeAndPassButNotTheLeap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveLineEdgeTest::RunTest(const FString&)
{
	const FRTCellId Origine(0, 0), Primo(1, 0), Secondo(2, 0), Terzo(3, 0);
	const ERTHexDirection Est = MoveActionEdgeToward(Primo, Secondo);
	const TMap<FRTCellId, int32> Empty;
	const TSet<int32> NoHostiles;
	const ERTMovementStyle Fermati[] = { ERTMovementStyle::LinearDash, ERTMovementStyle::LinearCharge,
		ERTMovementStyle::LinearPass };

	// (1) Copertura ALTA sul bordo (1,0)-(2,0): ferma i tre stili dopo un passo, con il motivo del bordo.
	URTHexMapAsset* Alta = MakeMoveActionMap();
	PutMoveActionCover(Alta, Primo, Est, ERTHexCoverType::High);
	for (const ERTMovementStyle Stile : Fermati)
	{
		const FRTLinearMoveResult R = URTMovementActionLibrary::ResolveLinearMove(
			Alta, Origine, Terzo, 3, Stile, Empty, NoHostiles);
		TestTrue(*FString::Printf(TEXT("stile %d: si ferma prima del bordo"), static_cast<int32>(Stile)),
			R.Final == Primo && R.Entered.Num() == 1);
		TestTrue(*FString::Printf(TEXT("stile %d: col motivo del BORDO, non della cella"), static_cast<int32>(Stile)),
			R.Stop == ERTLinearStop::BlockedByEdge);
	}
	const FRTLinearMoveResult Salto = URTMovementActionLibrary::ResolveLinearMove(
		Alta, Origine, Terzo, 3, ERTMovementStyle::LinearLeap, Empty, NoHostiles);
	TestTrue(TEXT("il salto scavalca il bordo: guarda solo l'atterraggio"),
		Salto.Final == Terzo && Salto.Stop == ERTLinearStop::Completed);
	TestFalse(TEXT("la retta non e' aperta"), URTMovementActionLibrary::IsLineOpenToTarget(Alta, Origine, Terzo));

	// (2) Il bordo sul PRIMO passo: lo scatto non muove di una cella, e dice perche'.
	URTHexMapAsset* PrimoPasso = MakeMoveActionMap();
	PutMoveActionCover(PrimoPasso, Origine, MoveActionEdgeToward(Origine, Primo), ERTHexCoverType::High);
	const FRTLinearMoveResult Fermo = URTMovementActionLibrary::ResolveLinearMove(
		PrimoPasso, Origine, Terzo, 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("sul primo bordo non si muove"), Fermo.Final == Origine && Fermo.Entered.Num() == 0);
	TestTrue(TEXT("e il motivo e' il bordo"), Fermo.Stop == ERTLinearStop::BlockedByEdge);

	// (3) Il bordo dichiarato dalla sola faccia LONTANA ferma uguale: la barriera e' fisica.
	URTHexMapAsset* FacciaLontana = MakeMoveActionMap();
	PutMoveActionCover(FacciaLontana, Secondo, MoveActionEdgeToward(Secondo, Primo), ERTHexCoverType::High);
	TestTrue(TEXT("la faccia di (2,0) verso (1,0) ferma come quella di (1,0)"),
		URTMovementActionLibrary::ResolveLinearMove(FacciaLontana, Origine, Terzo, 3, ERTMovementStyle::LinearDash,
			Empty, NoHostiles).Stop == ERTLinearStop::BlockedByEdge);

	// (4) La copertura BASSA ripara chi ci sta dietro ma non chiude il passaggio.
	URTHexMapAsset* Bassa = MakeMoveActionMap();
	PutMoveActionCover(Bassa, Primo, Est, ERTHexCoverType::Low);
	TestTrue(TEXT("la copertura bassa non ferma lo scatto"),
		URTMovementActionLibrary::ResolveLinearMove(Bassa, Origine, Terzo, 3, ERTMovementStyle::LinearDash,
			Empty, NoHostiles).Final == Terzo);

	// (5) Porta: chiusa e bloccata fermano, aperta no.
	const TPair<ERTHexDoorState, bool> Porte[] = {
		{ ERTHexDoorState::Closed, true }, { ERTHexDoorState::Locked, true }, { ERTHexDoorState::Open, false } };
	for (const TPair<ERTHexDoorState, bool>& Porta : Porte)
	{
		URTHexMapAsset* ConPorta = MakeMoveActionMap();
		PutMoveActionDoor(ConPorta, Primo, Est, Porta.Key);
		const FRTLinearMoveResult R = URTMovementActionLibrary::ResolveLinearMove(
			ConPorta, Origine, Terzo, 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
		if (Porta.Value)
		{
			TestTrue(TEXT("una porta chiusa o bloccata ferma lo scatto sul bordo"),
				R.Final == Primo && R.Stop == ERTLinearStop::BlockedByEdge);
		}
		else
		{
			TestTrue(TEXT("una porta aperta lascia passare"), R.Final == Terzo && R.Stop == ERTLinearStop::Completed);
		}
	}

	// (6) Sulla retta FUORI ASSE: (0,0) -> (1,1) passa da (1,0), e il bordo (1,0)-(1,1) e' il secondo passo.
	URTHexMapAsset* FuoriAsse = MakeMoveActionMap();
	PutMoveActionCover(FuoriAsse, Primo, MoveActionEdgeToward(Primo, FRTCellId(1, 1)), ERTHexCoverType::High);
	const FRTLinearMoveResult Obliqua = URTMovementActionLibrary::ResolveLinearMove(
		FuoriAsse, Origine, FRTCellId(1, 1), 3, ERTMovementStyle::LinearDash, Empty, NoHostiles);
	TestTrue(TEXT("fuori asse il bordo sulla retta ferma dopo un passo"),
		Obliqua.Final == Primo && Obliqua.Stop == ERTLinearStop::BlockedByEdge);

	// (7) La carica verso un nemico DIETRO il bordo non lo urta: il bordo la ferma prima.
	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(Terzo, 4);
	const FRTLinearMoveResult Carica = URTMovementActionLibrary::ResolveLinearMove(
		Alta, Origine, Terzo, 3, ERTMovementStyle::LinearCharge, Occupancy, TSet<int32>{ 4 });
	TestTrue(TEXT("la carica si ferma sul bordo senza urtare nessuno"),
		Carica.Stop == ERTLinearStop::BlockedByEdge && Carica.ImpactUnitId == INDEX_NONE);

	// (8) Il bordo sull'ULTIMO passo, verso l'atterraggio, non riguarda il salto.
	URTHexMapAsset* SuAtterraggio = MakeMoveActionMap();
	PutMoveActionCover(SuAtterraggio, Secondo, MoveActionEdgeToward(Secondo, Terzo), ERTHexCoverType::High);
	TestTrue(TEXT("il salto atterra anche dietro un bordo alto che il passo non passerebbe"),
		URTMovementActionLibrary::ResolveLinearMove(SuAtterraggio, Origine, Terzo, 3, ERTMovementStyle::LinearLeap,
			Empty, NoHostiles).Final == Terzo);
	return true;
}

/**
 * [D-516] (3)-(4) con la retta nuova: «la `HexLine` si costruisce fino al bersaglio e poi si tronca alla portata».
 *
 * 🔑 Il bersaglio `(3,1)` e' scelto perche' DISCRIMINA: la retta fino a lui passa da `(1,0)`, `(2,0)`, `(2,1)`, mentre
 * la retta fino alla SUA terza cella `(2,1)` passa da `(1,0)`, `(1,1)`, `(2,1)`. Troncare dopo aver costruito la retta
 * al bersaglio o costruirla fino al limite della portata danno percorsi diversi, e solo la prima e' la decisione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveLineTruncationTest,
	"RefactorTactics.Actions.Charge.BeyondRangeTruncatesTheLineToTheTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveLineTruncationTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMoveActionMap();
	const TMap<FRTCellId, int32> Empty;
	const TSet<int32> Hostiles = { 4 };
	const FRTCellId Origine(0, 0), Bersaglio(3, 1);

	const TArray<FRTCellId> FinoAlBersaglio = URTHexLibrary::HexLine(Origine, Bersaglio);
	const TArray<FRTCellId> FinoAlLimite = URTHexLibrary::HexLine(Origine, FinoAlBersaglio[3]);
	if (!TestEqual(TEXT("premessa: il bersaglio e' a distanza 4, oltre la portata 3"),
		URTHexLibrary::HexDistance(Origine, Bersaglio), 4)) { return false; }
	if (!TestFalse(TEXT("premessa: le due costruzioni danno percorsi DIVERSI, o il test non discrimina"),
		FinoAlLimite[2] == FinoAlBersaglio[2])) { return false; }

	const FRTLinearMoveResult R = URTMovementActionLibrary::ResolveLinearMove(
		Map, Origine, Bersaglio, /*MaxCells*/ 3, ERTMovementStyle::LinearCharge, Empty, Hostiles);
	TestTrue(TEXT("la carica corre fino alla portata e si ferma"),
		R.Stop == ERTLinearStop::OutOfRange && R.bTargetBeyondRange);
	TestTrue(TEXT("lungo il prefisso della retta fino al BERSAGLIO: (1,0), (2,0), (2,1)"),
		R.Entered == TArray<FRTCellId>{ FRTCellId(1, 0), FRTCellId(2, 0), FRTCellId(2, 1) });
	const TArray<FRTCellId> Coda = MoveActionHexLineTail(Origine, Bersaglio);
	TArray<FRTCellId> Prefisso;
	for (int32 I = 0; I < 3; ++I) { Prefisso.Add(Coda[I]); }
	TestTrue(TEXT("e' proprio il prefisso di HexLine, partenza esclusa"), R.Entered == Prefisso);
	TestTrue(TEXT("fermandosi in (2,1)"), R.Final == FRTCellId(2, 1));

	// Lo scatto, sulla stessa geometria, non parte: oltre la portata resta com'era.
	TestTrue(TEXT("lo scatto oltre la portata non parte"),
		URTMovementActionLibrary::ResolveLinearMove(Map, Origine, Bersaglio, 3, ERTMovementStyle::LinearDash,
			Empty, Hostiles).Entered.Num() == 0);
	return true;
}

/**
 * [D-524] (1): «gli attacchi lineari restano sulle sei direzioni». `IsStraightLine` e' letta anche da
 * `LineCells`, `ResolveLineAttack` e dalla soppressione, quindi NON si cambia sul posto: il movimento legge
 * `MovementLine`, e le due divergono ESATTAMENTE sulle celle fuori asse.
 *
 * Il test mette fianco a fianco i due lettori sulla stessa cella `(2,1)`: il movimento arriva, l'attacco non parte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveLineAttacksStayTest,
	"RefactorTactics.Actions.LineAttack.StaysOnTheSixDirectionsWhileMovementDoesNot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveLineAttacksStayTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeMoveActionMap();
	const FRTCellId Origine(0, 0), FuoriAsse(2, 1), SuAsse(3, 0);
	const TMap<FRTCellId, int32> Empty;

	int32 Distanza = 0;
	TestFalse(TEXT("IsStraightLine rifiuta ancora la cella fuori asse"),
		URTMovementActionLibrary::IsStraightLine(Origine, FuoriAsse, Distanza));
	TestTrue(TEXT("e accetta ancora quella su un asse"),
		URTMovementActionLibrary::IsStraightLine(Origine, SuAsse, Distanza) && Distanza == 3);

	// L'attacco lineare: fuori asse non parte, e non c'e' una linea di celle da colpire.
	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(FuoriAsse, 4);
	const TSet<int32> Hostiles = { 4 };
	const FRTLineAttackResult Attacco = URTOffensiveActionLibrary::ResolveLineAttack(
		Map, Origine, FuoriAsse, /*Range*/ 5, Occupancy, Hostiles);
	TestTrue(TEXT("l'attacco lineare fuori asse non parte"), Attacco.Stop == ERTLineStop::NotAligned);
	TestEqual(TEXT("e non colpisce nessuno"), Attacco.HitUnitId, static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("LineCells fuori asse e' vuota (la soppressione passa da qui)"),
		URTOffensiveActionLibrary::LineCells(Map, Origine, FuoriAsse, 5).Num(), 0);

	// Il movimento sulla STESSA cella: la carica urta.
	const FRTLinearMoveResult Carica = URTMovementActionLibrary::ResolveLinearMove(
		Map, Origine, FuoriAsse, 5, ERTMovementStyle::LinearCharge, Occupancy, Hostiles);
	TestTrue(TEXT("la carica sulla stessa cella urta il nemico"),
		Carica.Stop == ERTLinearStop::Impact && Carica.ImpactUnitId == 4);

	// E su un asse i due lettori continuano a coincidere: stessa direzione, stesso primo bersaglio.
	TMap<FRTCellId, int32> SuAsseOccupata;
	SuAsseOccupata.Add(SuAsse, 5);
	const FRTLineAttackResult AttaccoSuAsse = URTOffensiveActionLibrary::ResolveLineAttack(
		Map, Origine, SuAsse, 5, SuAsseOccupata, TSet<int32>{ 5 });
	TestTrue(TEXT("su un asse l'attacco colpisce come prima"),
		AttaccoSuAsse.Stop == ERTLineStop::Hit && AttaccoSuAsse.HitUnitId == 5);
	return true;
}

/**
 * `MovementLine` e' la funzione che il movimento legge, e coincide con `HexLine` ESATTAMENTE dove il movimento puo'
 * andare: stesso piano, cella diversa dalla partenza. Altrove risponde falso (e lascia la retta vuota).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMoveLineFunctionTest,
	"RefactorTactics.Actions.Dash.MovementLineIsHexLineOnTheSamePlane",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMoveLineFunctionTest::RunTest(const FString&)
{
	const FRTCellId Origine(0, 0);
	for (const FRTCellId& C : URTHexLibrary::HexArea(Origine, 4))
	{
		TArray<FRTCellId> Retta;
		if (C == Origine)
		{
			TestFalse(TEXT("la cella stessa non ha una retta"), URTMovementActionLibrary::MovementLine(Origine, C, Retta));
			TestEqual(TEXT("e la retta resta vuota"), Retta.Num(), 0);
			continue;
		}
		const bool bHa = URTMovementActionLibrary::MovementLine(Origine, C, Retta);
		TestTrue(TEXT("ogni altra cella dello stesso piano ha una retta"), bHa);
		TestTrue(TEXT("che e' la retta di HexLine, estremi inclusi"), Retta == URTHexLibrary::HexLine(Origine, C));
		TestEqual(TEXT("di distanza + 1 celle"), Retta.Num(), URTHexLibrary::HexDistance(Origine, C) + 1);
	}

	TArray<FRTCellId> AltroPiano;
	TestFalse(TEXT("un altro piano non e' mai in linea: il vincolo di layer di IsStraightLine resta"),
		URTMovementActionLibrary::MovementLine(FRTCellId(0, 0, 0), FRTCellId(2, 1, 1), AltroPiano));
	TestEqual(TEXT("e la retta resta vuota"), AltroPiano.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
