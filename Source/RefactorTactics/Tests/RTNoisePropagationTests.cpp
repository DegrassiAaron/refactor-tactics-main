#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexCoverLibrary.h" // FRTHexCover, BlocksTraversal: l'occlusione e' un ARCO
#include "Map/RTHexMapAsset.h"
#include "Perception/RTAcousticPropagationLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 13.3 — il rumore come SECONDO CANALE di informazione (#158).
 *
 * Non un debuff e non un secondo raggio di rilevamento: un evento reale della simulazione che si propaga sul
 * **grafo tattico** e che alcune squadre sentono e altre no, secondo una soglia per eroe (D-041).
 *
 * Le due grandezze vivono sulla stessa scala `0-10` — intensita' (`Wait 0 · Sprint 5 · Dash 6 · esplosione
 * 10`) e `HearingThreshold` — ed e' cio' che le rende confrontabili. Soglia **bassa** = orecchio fine.
 */

namespace
{
	/** Arena piatta piena di raggio N, tutta `Floor`. Nome distinto per file: la unity build condivide la TU. */
	URTHexMapAsset* MakeNoiseMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	void SetNoiseSurface(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexSurface Surface)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Surface = Surface;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	void SetNoiseWall(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.bBlocksMovement = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Uno Sprint: l'intensita' `5` del catalogo, che e' il caso di verifica di D-041. */
	FRTNoiseEvent Sprint(const FRTCellId& Origin)
	{
		FRTNoiseEvent E;
		E.SourceUnitId = 1;
		E.OriginCell = Origin;
		E.NoiseType = ERTNoiseType::Movement;
		E.Intensity = 5;
		E.TurnIndex = 1;
		E.MicroStepIndex = 0;
		return E;
	}

	int32 HeardAt(const URTHexMapAsset* Map, const FRTNoiseEvent& Event, const FRTCellId& Where)
	{
		return URTAcousticPropagationLibrary::NoiseAtCell(
			URTAcousticPropagationLibrary::Propagate(Map, Event), Where);
	}
}

/**
 * La propagazione e' **deterministica** e non dipende dall'ordine di scoperta.
 *
 * Un flood fill su TMap e' il posto classico dove il non-determinismo entra senza farsi notare: l'ordine di
 * iterazione dipende dall'hash, la frontiera si sceglie «il primo che capita», e due esecuzioni identiche
 * producono due array diversi che nessuno confronta mai. Qui si confrontano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoiseDeterministicTest,
	"RefactorTactics.Noise.PropagationIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoiseDeterministicTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeNoiseMap(6);
	const FRTNoiseEvent Event = Sprint(FRTCellId(0, 0));

	const TArray<FRTNoiseReception> First = URTAcousticPropagationLibrary::Propagate(Map, Event);
	if (!TestTrue(TEXT("premessa: il rumore raggiunge piu' di una cella"), First.Num() > 1)) { return false; }

	for (int32 Rep = 0; Rep < 5; ++Rep)
	{
		const TArray<FRTNoiseReception> Again = URTAcousticPropagationLibrary::Propagate(Map, Event);
		if (!TestEqual(TEXT("stesso numero di celle a ogni esecuzione"), Again.Num(), First.Num())) { return false; }
		for (int32 i = 0; i < First.Num(); ++i)
		{
			TestTrue(TEXT("stessa cella nella stessa posizione"), Again[i].Cell == First[i].Cell);
			TestEqual(TEXT("stessa intensita' residua"), Again[i].ReceivedNoise, First[i].ReceivedNoise);
		}
	}

	// L'ordine e' quello canonico del progetto, non quello di scoperta.
	for (int32 i = 1; i < First.Num(); ++i)
	{
		TestTrue(TEXT("celle ordinate con StableLess"),
			URTHexLibrary::StableLess(First[i - 1].Cell, First[i].Cell));
	}
	return true;
}

/**
 * L'attenuazione **per superficie**, con i numeri concreti — non «attenua di qualcosa».
 *
 * Il caso e' quello che D-041 usa come verifica di scala, e serve a due cose insieme: pinna l'eccezione
 * **D-042** (acqua bassa `+2`, non il `+3` che la formula del workbook darebbe) e dimostra che la soglia
 * separa davvero il roster invece di essere una statistica decorativa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoiseAttenuationBySurfaceTest,
	"RefactorTactics.Noise.AttenuationBySurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoiseAttenuationBySurfaceTest::RunTest(const FString&)
{
	// I delta del catalogo, voce per voce. E' la tabella del workbook (`08_Terreni` -> `Noise_Mod`) con
	// `NoiseDelta = Noise_Mod - 1`, e i due valori decisi a mano perche' il workbook non li ha.
	TestEqual(TEXT("Floor: e' il riferimento della scala"),
		URTAcousticPropagationLibrary::SurfaceNoiseDelta(ERTHexSurface::Floor), 0);
	TestEqual(TEXT("HighGround: come il terreno libero"),
		URTAcousticPropagationLibrary::SurfaceNoiseDelta(ERTHexSurface::HighGround), 0);
	TestEqual(TEXT("Smoke: acceca, non ammutolisce"),
		URTAcousticPropagationLibrary::SurfaceNoiseDelta(ERTHexSurface::Smoke), 0);
	TestEqual(TEXT("Ice: scricchiola (+1), e le due fonti concordano"),
		URTAcousticPropagationLibrary::SurfaceNoiseDelta(ERTHexSurface::Ice), 1);
	TestEqual(TEXT("Fire: crepita (+4)"),
		URTAcousticPropagationLibrary::SurfaceNoiseDelta(ERTHexSurface::Fire), 4);
	// ⚠️ Le due voci senza riga nel workbook, decise il 2026-08-11. Il test le pinna perche' un valore
	// deciso a voce e non verificato e' indistinguibile da un default dimenticato.
	TestEqual(TEXT("Rough: accidentato (+1), deciso — nessuna riga nel workbook"),
		URTAcousticPropagationLibrary::SurfaceNoiseDelta(ERTHexSurface::Rough), 1);
	TestEqual(TEXT("Conductive: 0, deciso — ha gia' un owner, ed e' elettrico"),
		URTAcousticPropagationLibrary::SurfaceNoiseDelta(ERTHexSurface::Conductive), 0);
	// ⚠️ **D-042**, il solo punto dove formula e decisione divergono: `Noise_Mod 4` darebbe +3.
	TestEqual(TEXT("ShallowWater: +2 per D-042, NON il +3 della formula"),
		URTAcousticPropagationLibrary::SurfaceNoiseDelta(ERTHexSurface::ShallowWater), 2);

	// La verifica di scala di D-041, misurata sul percorso vero invece che sulla tabella.
	URTHexMapAsset* Map = MakeNoiseMap(6);
	const FRTCellId Origin(0, 0);
	const FRTCellId TwoAway(2, 0);

	// Su terreno libero uno Sprint (5) attenuato di 2 celle arriva a 3: lo sentono Riva e Bastion
	// (soglia 3), non Flux e Vektor (soglia 5). E' l'esempio che D-041 usa per giustificare i valori.
	const int32 OnFloor = HeardAt(Map, Sprint(Origin), TwoAway);
	TestEqual(TEXT("Sprint su terreno libero, a due celle: 3"), OnFloor, 3);

	const URTHeroData* Riva = URTHeroCatalogLibrary::MakeRiva();
	const URTHeroData* Flux = URTHeroCatalogLibrary::MakeFlux();
	if (!TestNotNull(TEXT("Riva costruita"), Riva) || !TestNotNull(TEXT("Flux costruito"), Flux)) { return false; }
	TestTrue(TEXT("Riva (orecchio fine, 3) lo sente"),
		URTAcousticPropagationLibrary::IsAudible(OnFloor, Riva->HearingThreshold));
	TestFalse(TEXT("Flux (5) non lo sente"),
		URTAcousticPropagationLibrary::IsAudible(OnFloor, Flux->HearingThreshold));

	// Lo STESSO Sprint, dall'acqua bassa: +2 alla sorgente. Arriva a 5, e adesso lo sente anche Flux.
	// E' la differenza che D-042 descrive a parole — «sprintarci fa 7 su 10, come un Dash su terreno libero».
	SetNoiseSurface(Map, Origin, ERTHexSurface::ShallowWater);
	const int32 FromWater = HeardAt(Map, Sprint(Origin), TwoAway);
	TestEqual(TEXT("Sprint dall'acqua bassa, a due celle: 5"), FromWater, 5);
	TestTrue(TEXT("adesso lo sente anche Flux"),
		URTAcousticPropagationLibrary::IsAudible(FromWater, Flux->HearingThreshold));
	return true;
}

/**
 * La soglia decide, e decide **per eroe**.
 *
 * Il gemello di controllo non e' un lusso: senza un eroe che NON sente, il test passerebbe anche con una
 * soglia ignorata e tutti che sentono tutto — cioe' col canale acustico ridotto a un secondo `VisionRange`,
 * che e' esattamente cio' che D-041 esiste per evitare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoiseThresholdDecidesTest,
	"RefactorTactics.Noise.ThresholdDecidesDetection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoiseThresholdDecidesTest::RunTest(const FString&)
{
	// I quattro valori di D-041, dal catalogo. Pinnati: sono una decisione, e una decisione che nessun test
	// guarda e' una riga di documentazione.
	const URTHeroData* Flux = URTHeroCatalogLibrary::MakeFlux();
	const URTHeroData* Riva = URTHeroCatalogLibrary::MakeRiva();
	const URTHeroData* Bastion = URTHeroCatalogLibrary::MakeBastion();
	const URTHeroData* Vektor = URTHeroCatalogLibrary::MakeVektor();
	if (!TestNotNull(TEXT("roster costruito"), Flux) || !TestNotNull(TEXT("roster costruito"), Riva)
		|| !TestNotNull(TEXT("roster costruito"), Bastion) || !TestNotNull(TEXT("roster costruito"), Vektor))
	{
		return false;
	}
	TestEqual(TEXT("Flux 5"), Flux->HearingThreshold, 5);
	TestEqual(TEXT("Riva 3"), Riva->HearingThreshold, 3);
	TestEqual(TEXT("Bastion 3"), Bastion->HearingThreshold, 3);
	TestEqual(TEXT("Vektor 5"), Vektor->HearingThreshold, 5);

	// L'udito COMPENSA la vista: chi vede lontano sente meno. E' la proprieta' che rende l'udito una seconda
	// via all'informazione e non un raddoppio della prima.
	TestTrue(TEXT("Flux vede piu' di Riva"), Flux->VisionRange > Riva->VisionRange);
	TestTrue(TEXT("...e in cambio sente meno"), Flux->HearingThreshold > Riva->HearingThreshold);

	TestTrue(TEXT("soglia raggiunta esattamente: si sente"),
		URTAcousticPropagationLibrary::IsAudible(3, 3));
	TestFalse(TEXT("un punto sotto: non si sente"),
		URTAcousticPropagationLibrary::IsAudible(2, 3));
	// Il silenzio non si sente nemmeno con l'orecchio perfetto: `Action.Wait` vale 0.
	TestFalse(TEXT("rumore nullo non e' udibile nemmeno a soglia 0"),
		URTAcousticPropagationLibrary::IsAudible(0, 0));
	return true;
}

/**
 * Il rumore usa il **grafo**, non un raggio euclideo (D14 del brief — sigla locale).
 *
 * E' la voce di DoD che vieta `SphereOverlap`, e la si prova nel solo modo che significhi qualcosa: una cella
 * vicina in linea d'aria ma **lontana sul grafo** deve ricevere di meno, o niente. Una sfera le darebbe lo
 * stesso valore della sua gemella in campo aperto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoiseUsesGraphTest,
	"RefactorTactics.Noise.UsesGraphNotEuclideanRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoiseUsesGraphTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeNoiseMap(6);

	// Un muro pieno che separa l'origine dalla cella dietro: il suono deve girarci intorno. Si chiudono tutte
	// e tre le celle della «parete» perche' su esagoni una sola lascerebbe due passaggi diagonali aperti, e il
	// test misurerebbe la deviazione invece dell'ostacolo.
	const FRTCellId Origin(0, 0);
	const FRTCellId Behind(2, 0);
	SetNoiseWall(Map, FRTCellId(1, 0));
	SetNoiseWall(Map, FRTCellId(1, -1));
	SetNoiseWall(Map, FRTCellId(2, -1));

	const int32 Blocked = HeardAt(Map, Sprint(Origin), Behind);

	// Il controllo: la stessa distanza esagonale, in campo aperto. Se il rumore fosse una sfera, i due numeri
	// sarebbero identici — e' questa uguaglianza che il test deve poter smentire.
	URTHexMapAsset* Open = MakeNoiseMap(6);
	const int32 Clear = HeardAt(Open, Sprint(Origin), Behind);
	TestEqual(TEXT("premessa: in campo aperto, a due celle, arriva 3"), Clear, 3);
	TestTrue(TEXT("premessa: la distanza esagonale e' la stessa"),
		URTHexLibrary::HexDistance(Origin, Behind) == 2);

	TestTrue(TEXT("dietro il muro arriva MENO che in campo aperto"), Blocked < Clear);
	TestEqual(TEXT("il giro lungo costa una cella in piu': 2"), Blocked, 2);
	return true;
}

/**
 * L'occlusione: un arco che il movimento non attraversa, il suono lo attraversa **pagando**.
 *
 * E' la differenza fra questo test e `UsesGraphNotEuclideanRadius`: li' l'ostacolo e' volume pieno e il suono
 * gira intorno; qui e' un varco chiuso — copertura alta, porta — e il suono passa attenuato. Senza la
 * distinzione, uno dei due test sarebbe una copia dell'altro.
 *
 * ⚠️ `BlockedEdgeAttenuation = 2` viene dal documento sorgente e **non ha ancora una voce nel Decision Log**.
 * E' pinnato qui apposta: il giorno che venga consolidato con un altro numero, questo test lo dice.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoiseOcclusionTest,
	"RefactorTactics.Noise.OcclusionAttenuates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoiseOcclusionTest::RunTest(const FString&)
{
	const FRTCellId Origin(0, 0);
	const FRTCellId Beyond(1, 0);

	// Controllo: senza barriera, la cella adiacente riceve l'intensita' meno un passo.
	URTHexMapAsset* Open = MakeNoiseMap(4);
	const int32 Clear = HeardAt(Open, Sprint(Origin), Beyond);
	TestEqual(TEXT("premessa: adiacente in campo aperto, uno Sprint arriva a 4"), Clear, 4);

	// Stessa geometria, con una copertura ALTA sul bordo fra le due celle: il passo non e' percorribile, ma
	// entrambe le celle restano occupabili — e il suono le attraversa pagando l'occlusione.
	URTHexMapAsset* Walled = MakeNoiseMap(4);
	FRTHexCellData Data = *Walled->FindCell(Origin);
	Data.Covers.Add(FRTHexCover(ERTHexDirection::E, ERTHexCoverType::High,
		FRTHexCover::DefaultIntegrity(ERTHexCoverType::High)));
	Walled->AddOrUpdateCell(Data);
	Walled->SortCells();
	if (!TestTrue(TEXT("premessa: il bordo blocca davvero il passo"),
			URTHexCoverLibrary::BlocksTraversal(Walled, Origin, Beyond)))
	{
		return false;
	}

	const int32 Occluded = HeardAt(Walled, Sprint(Origin), Beyond);
	TestTrue(TEXT("attraverso il varco chiuso arriva MENO"), Occluded < Clear);
	TestEqual(TEXT("e arriva esattamente meno di BlockedEdgeAttenuation"),
		Occluded, Clear - URTAcousticPropagationLibrary::BlockedEdgeAttenuation);
	// Passa comunque: un muro attenua, non interrompe. Se questo diventasse 0, l'occlusione sarebbe
	// diventata un blocco e i due test tornerebbero a misurare la stessa cosa.
	TestTrue(TEXT("ma passa: attenua, non interrompe"), Occluded > 0);
	return true;
}

/**
 * Invarianza per permutazione: la propagazione non dipende da come la mappa e' stata costruita.
 *
 * L'ordine delle celle in `URTHexMapAsset::Cells` e' un dettaglio di authoring — chi disegna la mappa le
 * aggiunge nell'ordine in cui ci clicca sopra. Se il rumore ne dipendesse, due mappe identiche darebbero
 * partite diverse, e il replay smetterebbe di essere un replay.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoisePermutationInvariantTest,
	"RefactorTactics.Noise.PermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoisePermutationInvariantTest::RunTest(const FString&)
{
	const FRTCellId Origin(0, 0);
	TArray<FRTCellId> Ids = URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 5);

	// Due mappe con le STESSE celle e le stesse superfici, inserite in ordine opposto.
	URTHexMapAsset* Forward = NewObject<URTHexMapAsset>();
	for (const FRTCellId& Id : Ids) { Forward->AddOrUpdateCell(FRTHexCellData(Id)); }
	Forward->SortCells();

	URTHexMapAsset* Reversed = NewObject<URTHexMapAsset>();
	for (int32 i = Ids.Num() - 1; i >= 0; --i) { Reversed->AddOrUpdateCell(FRTHexCellData(Ids[i])); }
	Reversed->SortCells();

	// Un OSTACOLO in mezzo, cosi' il grafo e' asimmetrico e la permutazione ha qualcosa su cui sbagliare.
	//
	// ⚠️ Qui non serve una superficie rumorosa e sarebbe un errore usarla: `NoiseDelta` agisce alla SORGENTE,
	// quindi del ghiaccio a meta' strada la propagazione non si accorge — il test resterebbe verde
	// verificando molto meno di quel che dichiara. Un muro invece cambia i percorsi, che e' esattamente
	// dove l'ordine di scoperta puo' influire.
	SetNoiseWall(Forward, FRTCellId(1, 0));
	SetNoiseWall(Reversed, FRTCellId(1, 0));
	SetNoiseWall(Forward, FRTCellId(1, -1));
	SetNoiseWall(Reversed, FRTCellId(1, -1));

	const TArray<FRTNoiseReception> A = URTAcousticPropagationLibrary::Propagate(Forward, Sprint(Origin));
	const TArray<FRTNoiseReception> B = URTAcousticPropagationLibrary::Propagate(Reversed, Sprint(Origin));

	if (!TestEqual(TEXT("stesso numero di celle raggiunte"), B.Num(), A.Num())) { return false; }
	if (!TestTrue(TEXT("premessa: il rumore raggiunge piu' di una cella"), A.Num() > 1)) { return false; }
	for (int32 i = 0; i < A.Num(); ++i)
	{
		TestTrue(TEXT("stessa cella nella stessa posizione"), B[i].Cell == A[i].Cell);
		TestEqual(TEXT("stessa intensita' residua"), B[i].ReceivedNoise, A[i].ReceivedNoise);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
