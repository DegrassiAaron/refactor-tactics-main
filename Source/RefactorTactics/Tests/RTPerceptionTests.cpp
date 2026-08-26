#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTPerceptionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 13.1 — cosa una squadra VEDE. Funzione pura e headless: nessun Actor, nessun `UWorld`.
 *
 * Il modello ha due canali che convivono ([ADR-0005](../../../docs/decisions/adr-0005-orientamento.md) §4b):
 * vista piena fino a `VisionRange` **nell'arco frontale**, e consapevolezza a 360° **entro 2 celle**. La LOS
 * serve a entrambi — si vede attraverso l'aria, non attraverso i muri.
 *
 * Riferimenti: `docs/gameplay/brief-conoscenza-parziale.md`, D-043 (la conoscenza e' di SQUADRA, non una
 * relazione fra unita').
 */

namespace
{
	/** Arena piena di raggio N sul layer 0. Nome distinto per file: la unity build condivide la TU. */
	URTHexMapAsset* MakePerceptionMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return M;
	}

	void SetPerceptionSurface(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexSurface Surface)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Surface = Surface;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	void SetPerceptionSightBlocker(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	FRTPerceiver Watcher(const FRTCellId& Cell, ERTHexDirection Facing, int32 VisionRange)
	{
		FRTPerceiver P;
		P.Cell = Cell;
		P.Facing = Facing;
		P.VisionRange = VisionRange;
		return P;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisionVisibleCellsRespectsSightTest,
	"RefactorTactics.Vision.VisibleCellsRespectsSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisionVisibleCellsRespectsSightTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakePerceptionMap(8);
	const FRTCellId Eye(0, 0, 0);

	// Vista 3 guardando a est: (3,0) dentro, (4,0) fuori. La portata e' un CONFINE, non un suggerimento.
	const TArray<FRTCellId> Seen = URTPerceptionLibrary::VisibleCells(Map, Watcher(Eye, ERTHexDirection::E, 3));
	TestTrue(TEXT("a distanza 3 nell'arco: visibile"), Seen.Contains(FRTCellId(3, 0, 0)));
	TestFalse(TEXT("a distanza 4: fuori portata"), Seen.Contains(FRTCellId(4, 0, 0)));

	// La propria cella e' sempre nota: un'unita' sa dove si trova.
	TestTrue(TEXT("la propria cella e' sempre visibile"), Seen.Contains(Eye));

	// Vista maggiore vede di piu', e l'insieme piccolo e' CONTENUTO nel grande: la portata non sposta il cono.
	const TArray<FRTCellId> Farther = URTPerceptionLibrary::VisibleCells(Map, Watcher(Eye, ERTHexDirection::E, 5));
	TestTrue(TEXT("vista 5 arriva a distanza 5"), Farther.Contains(FRTCellId(5, 0, 0)));
	for (const FRTCellId& Near : Seen)
	{
		TestTrue(TEXT("cio' che si vede con 3 si vede anche con 5"), Farther.Contains(Near));
	}

	// La LOS serve: un muro sulla linea toglie cio' che sta oltre, non cio' che sta prima.
	URTHexMapAsset* Walled = MakePerceptionMap(8);
	SetPerceptionSightBlocker(Walled, FRTCellId(2, 0, 0));
	const TArray<FRTCellId> Blocked = URTPerceptionLibrary::VisibleCells(Walled, Watcher(Eye, ERTHexDirection::E, 5));
	TestTrue(TEXT("la cella prima del muro resta visibile"), Blocked.Contains(FRTCellId(1, 0, 0)));
	TestFalse(TEXT("dietro il muro non si vede"), Blocked.Contains(FRTCellId(4, 0, 0)));

	// Vista 0: NON si spegne tutto. La consapevolezza ravvicinata e' un canale INDIPENDENTE dalla vista —
	// ADR-0005 §4b dice «entro 2 celle si percepisce comunque» — quindi restano la propria cella e le 18
	// intorno (6 a distanza 1, 12 a distanza 2). Un'unita' accecata sente ancora chi le sta addosso.
	//
	// La distinzione conta: se `VisionRange` spegnesse anche il canale ravvicinato, azzerare la vista
	// diventerebbe l'effetto piu' forte del gioco — piu' di uno stordimento — e nessuno lo ha deciso.
	const TArray<FRTCellId> Blind = URTPerceptionLibrary::VisibleCells(Map, Watcher(Eye, ERTHexDirection::E, 0));
	TestEqual(TEXT("vista 0: resta la consapevolezza ravvicinata"), Blind.Num(), 19);
	TestTrue(TEXT("la propria cella c'e'"), Blind.Contains(Eye));
	TestTrue(TEXT("e i vicini pure"), Blind.Contains(FRTCellId(1, 0, 0)));
	TestFalse(TEXT("ma non si vede a 3 celle"), Blind.Contains(FRTCellId(3, 0, 0)));

	// Senza mappa invece si chiude tutto: non si puo' verificare ne' LOS ne' terreno, e restituire contatti
	// sarebbe la piu' pericolosa delle due risposte possibili.
	const TArray<FRTCellId> NoMap = URTPerceptionLibrary::VisibleCells(nullptr, Watcher(Eye, ERTHexDirection::E, 5));
	TestEqual(TEXT("senza mappa: solo la propria cella"), NoMap.Num(), 1);

	// Ordine STABILE: due chiamate identiche danno la stessa sequenza, non lo stesso insieme in ordine diverso.
	const TArray<FRTCellId> Again = URTPerceptionLibrary::VisibleCells(Map, Watcher(Eye, ERTHexDirection::E, 3));
	TestEqual(TEXT("stesso numero di celle"), Again.Num(), Seen.Num());
	for (int32 i = 0; i < Seen.Num() && i < Again.Num(); ++i)
	{
		TestTrue(TEXT("stesso ordine"), Seen[i] == Again[i]);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisionConeUsesHexConePrimitiveTest,
	"RefactorTactics.Vision.ConeUsesHexConePrimitive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisionConeUsesHexConePrimitiveTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakePerceptionMap(8);
	const FRTCellId Eye(0, 0, 0);

	// L'arco frontale e' il CONO di 120 gradi, la stessa primitiva della difesa direzionale (CP 16.2). Oltre
	// le 2 celle della consapevolezza ravvicinata, cio' che sta fuori dal cono non si vede.
	const TArray<FRTCellId> Seen = URTPerceptionLibrary::VisibleCells(Map, Watcher(Eye, ERTHexDirection::E, 6));

	TestTrue(TEXT("davanti, lontano: visibile"), Seen.Contains(FRTCellId(5, 0, 0)));
	TestFalse(TEXT("dietro, lontano: non visibile"), Seen.Contains(FRTCellId(-5, 0, 0)));

	// Non e' la sola direzione esatta: i due settori adiacenti sono dentro l'arco.
	TestTrue(TEXT("il settore NE e' dentro l'arco"), Seen.Contains(FRTCellId(4, -4, 0)));
	TestTrue(TEXT("e il settore SE pure"), Seen.Contains(FRTCellId(0, 4, 0)));

	// La LARGHEZZA dell'arco, verificata con numeri attesi e non richiamando `HexCone` — che e' cio' che
	// l'implementazione usa, quindi confrontarli direbbe solo «la funzione chiama la funzione».
	//
	// Il ventaglio di 120 gradi copre `2N+1` celle alla distanza N: 3 a distanza 1, 5 a 2, 7 a 3. Se qualcuno
	// stringesse l'arco a 60 gradi o lo allargasse a 180, questi conteggi cadrebbero; un test scritto con
	// `HexCone` a destra e a sinistra dell'uguale resterebbe verde.
	auto CountAtDistance = [&Seen, &Eye](int32 Distance)
	{
		int32 Count = 0;
		for (const FRTCellId& Cell : Seen)
		{
			if (URTHexLibrary::HexDistance(Eye, Cell) == Distance) { ++Count; }
		}
		return Count;
	};

	// A distanza 1 e 2 conta la consapevolezza ravvicinata, che e' a 360 gradi: 6 e 12 celle, non 3 e 5.
	TestEqual(TEXT("a distanza 1 si percepisce tutto intorno"), CountAtDistance(1), 6);
	TestEqual(TEXT("a distanza 2 pure"), CountAtDistance(2), 12);

	// Da distanza 3 in poi resta solo l'arco frontale: 2N+1.
	TestEqual(TEXT("a distanza 3 solo l'arco: 7 celle"), CountAtDistance(3), 7);
	TestEqual(TEXT("a distanza 4: 9 celle"), CountAtDistance(4), 9);
	TestEqual(TEXT("a distanza 5: 11 celle"), CountAtDistance(5), 11);

	// E l'arco e' centrato sul facing: la cella esattamente davanti c'e', quella esattamente dietro no.
	TestTrue(TEXT("davanti a distanza 4"), Seen.Contains(FRTCellId(4, 0, 0)));
	TestFalse(TEXT("dietro a distanza 4"), Seen.Contains(FRTCellId(-4, 0, 0)));

	// Girarsi cambia cio' che si vede: e' la prova che il facing entra davvero nel calcolo.
	const TArray<FRTCellId> Westward = URTPerceptionLibrary::VisibleCells(Map, Watcher(Eye, ERTHexDirection::W, 6));
	TestTrue(TEXT("girata a ovest vede dietro di prima"), Westward.Contains(FRTCellId(-5, 0, 0)));
	TestFalse(TEXT("e non vede piu' davanti"), Westward.Contains(FRTCellId(5, 0, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisionAwarenessWithinTwoCellsIgnoresFacingTest,
	"RefactorTactics.Vision.AwarenessWithinTwoCellsIgnoresFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisionAwarenessWithinTwoCellsIgnoresFacingTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakePerceptionMap(6);
	const FRTCellId Eye(0, 0, 0);
	const TArray<FRTCellId> Seen = URTPerceptionLibrary::VisibleCells(Map, Watcher(Eye, ERTHexDirection::E, 5));

	// Entro 2 celle si percepisce in OGNI direzione: senza, un'unita' resta cieca alle spalle per un turno
	// intero, e cio' che non si puo' nemmeno percepire smette di essere una scelta.
	TestEqual(TEXT("il raggio ravvicinato e' 2"), URTPerceptionLibrary::CloseAwarenessRange, 2);
	TestTrue(TEXT("alle spalle a 1 cella: percepito"), Seen.Contains(FRTCellId(-1, 0, 0)));
	TestTrue(TEXT("alle spalle a 2 celle: percepito"), Seen.Contains(FRTCellId(-2, 0, 0)));
	TestFalse(TEXT("alle spalle a 3 celle: nulla"), Seen.Contains(FRTCellId(-3, 0, 0)));

	// Vale per ogni direzione, non solo per l'opposta.
	for (uint8 Raw = 0; Raw < 6; ++Raw)
	{
		const FRTCellId Adjacent = URTHexLibrary::Neighbor(Eye, static_cast<ERTHexDirection>(Raw));
		TestTrue(TEXT("ogni vicino e' percepito, ovunque guardi"), Seen.Contains(Adjacent));
	}

	// Ma la LOS serve anche da vicino: un muro adiacente nasconde cio' che gli sta dietro, a 2 celle.
	URTHexMapAsset* Walled = MakePerceptionMap(6);
	SetPerceptionSightBlocker(Walled, FRTCellId(-1, 0, 0));
	const TArray<FRTCellId> Near = URTPerceptionLibrary::VisibleCells(Walled, Watcher(Eye, ERTHexDirection::E, 5));
	TestFalse(TEXT("dietro un muro adiacente non si percepisce"), Near.Contains(FRTCellId(-2, 0, 0)));

	// La consapevolezza ravvicinata non estende la vista: con vista 1, la cella a 2 e' comunque percepita
	// (e' il canale ravvicinato), ma quella a 4 no.
	const TArray<FRTCellId> Myope = URTPerceptionLibrary::VisibleCells(Map, Watcher(Eye, ERTHexDirection::E, 1));
	TestTrue(TEXT("il canale ravvicinato vale anche con vista 1"), Myope.Contains(FRTCellId(-2, 0, 0)));
	TestFalse(TEXT("ma non porta a 4 celle"), Myope.Contains(FRTCellId(4, 0, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisionTeamKnowledgeIsUnionTest,
	"RefactorTactics.Vision.TeamKnowledgeIsUnion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisionTeamKnowledgeIsUnionTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakePerceptionMap(8);

	// Due compagni che guardano da parti opposte: la squadra sa l'unione, non l'intersezione.
	const FRTPerceiver East = Watcher(FRTCellId(0, 0, 0), ERTHexDirection::E, 4);
	const FRTPerceiver West = Watcher(FRTCellId(0, 3, 0), ERTHexDirection::W, 4);

	const TArray<FRTCellId> Team = URTPerceptionLibrary::TeamVisibleCells(Map, { East, West });
	const TArray<FRTCellId> OnlyEast = URTPerceptionLibrary::VisibleCells(Map, East);
	const TArray<FRTCellId> OnlyWest = URTPerceptionLibrary::VisibleCells(Map, West);

	for (const FRTCellId& Cell : OnlyEast)
	{
		TestTrue(TEXT("cio' che vede il primo lo sa la squadra"), Team.Contains(Cell));
	}
	for (const FRTCellId& Cell : OnlyWest)
	{
		TestTrue(TEXT("cio' che vede il secondo lo sa la squadra"), Team.Contains(Cell));
	}
	TestTrue(TEXT("l'unione e' piu' grande di ciascun contributo"),
		Team.Num() > OnlyEast.Num() && Team.Num() > OnlyWest.Num());

	// Nessun duplicato: e' un insieme, e un doppione falserebbe qualunque conteggio a valle.
	TSet<FRTCellId> Unique(Team);
	TestEqual(TEXT("nessuna cella ripetuta"), Unique.Num(), Team.Num());

	// Squadra vuota -> nessuna conoscenza. Non l'intera mappa: il fail-closed vale anche qui.
	TestEqual(TEXT("nessun osservatore, nessuna conoscenza"),
		URTPerceptionLibrary::TeamVisibleCells(Map, {}).Num(), 0);

	// La conoscenza e' di SQUADRA (D-043): quella dell'altra squadra e' un insieme SEPARATO, e il fatto che
	// A veda B non mette A dentro la conoscenza di B.
	const FRTPerceiver Enemy = Watcher(FRTCellId(3, 0, 0), ERTHexDirection::E, 4);
	const TArray<FRTCellId> Theirs = URTPerceptionLibrary::TeamVisibleCells(Map, { Enemy });
	TestTrue(TEXT("la nostra squadra vede la cella del nemico"), Team.Contains(Enemy.Cell));
	TestFalse(TEXT("ma il nemico, girato dall'altra parte, non vede la nostra"), Theirs.Contains(East.Cell));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisionSmokeCapsContactAtTwoTest,
	"RefactorTactics.Vision.SmokeCapsContactAtTwo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisionSmokeCapsContactAtTwoTest::RunTest(const FString&)
{
	// Il cap del fumo e' gia' una regola del gioco, e vive in `URTTerrainLibrary::EffectiveTargetingRange`.
	// Qui si verifica che la percezione la RIUSI invece di ricalcolarla: due verita' sullo stesso numero
	// divergerebbero appena una delle due cambia — il progetto ha gia' rifiutato una volta di farne un
	// secondo gate (`Status.Obscured.AppliedBySmokeWithoutChangingGate`, decisione D4 di CP 8.1).
	URTHexMapAsset* Map = MakePerceptionMap(8);
	const FRTCellId Eye(0, 0, 0);

	// Senza fumo: vista 5 arriva a 5.
	const TArray<FRTCellId> Clear = URTPerceptionLibrary::VisibleCells(Map, Watcher(Eye, ERTHexDirection::E, 5));
	TestTrue(TEXT("senza fumo si vede a 5"), Clear.Contains(FRTCellId(5, 0, 0)));

	// Fumo sulla linea: il contatto si ferma a 2, dentro o attraverso.
	URTHexMapAsset* Smoky = MakePerceptionMap(8);
	SetPerceptionSurface(Smoky, FRTCellId(1, 0, 0), ERTHexSurface::Smoke);
	const TArray<FRTCellId> Capped = URTPerceptionLibrary::VisibleCells(Smoky, Watcher(Eye, ERTHexDirection::E, 5));

	TestTrue(TEXT("attraverso il fumo si vede fino a 2"), Capped.Contains(FRTCellId(2, 0, 0)));
	TestFalse(TEXT("oltre le 2 celle, no"), Capped.Contains(FRTCellId(3, 0, 0)));
	TestFalse(TEXT("e nemmeno a 5"), Capped.Contains(FRTCellId(5, 0, 0)));

	// Il cap vale sulla LINEA del contatto, non su tutta la vista: una direzione senza fumo resta piena.
	TestTrue(TEXT("dove non c'e' fumo la vista resta intera"), Capped.Contains(FRTCellId(4, -4, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVisionPermutationInvariantTest,
	"RefactorTactics.Vision.PermutationInvariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVisionPermutationInvariantTest::RunTest(const FString&)
{
	// Invariante #3 applicata alla conoscenza: l'ordine in cui il resolver visita i membri della squadra non
	// deve cambiare cio' che la squadra sa, ne' l'ordine in cui lo elenca.
	URTHexMapAsset* Map = MakePerceptionMap(8);
	const FRTPerceiver A = Watcher(FRTCellId(0, 0, 0), ERTHexDirection::E, 4);
	const FRTPerceiver B = Watcher(FRTCellId(0, 3, 0), ERTHexDirection::W, 5);
	const FRTPerceiver C = Watcher(FRTCellId(-2, 1, 0), ERTHexDirection::NE, 3);

	const TArray<FRTCellId> Forward = URTPerceptionLibrary::TeamVisibleCells(Map, { A, B, C });
	const TArray<FRTCellId> Reversed = URTPerceptionLibrary::TeamVisibleCells(Map, { C, B, A });
	const TArray<FRTCellId> Shuffled = URTPerceptionLibrary::TeamVisibleCells(Map, { B, A, C });

	TestEqual(TEXT("stesso numero di celle, ordine invertito"), Reversed.Num(), Forward.Num());
	TestEqual(TEXT("stesso numero di celle, ordine mischiato"), Shuffled.Num(), Forward.Num());
	for (int32 i = 0; i < Forward.Num(); ++i)
	{
		TestTrue(TEXT("stessa sequenza con l'ordine invertito"),
			Reversed.IsValidIndex(i) && Reversed[i] == Forward[i]);
		TestTrue(TEXT("stessa sequenza con l'ordine mischiato"),
			Shuffled.IsValidIndex(i) && Shuffled[i] == Forward[i]);
	}

	// L'ordine e' quello canonico del progetto (`StableLess`: layer, poi q, poi r), non l'ordine di scoperta.
	for (int32 i = 1; i < Forward.Num(); ++i)
	{
		TestTrue(TEXT("celle ordinate con StableLess"), URTHexLibrary::StableLess(Forward[i - 1], Forward[i]));
	}
	return true;
}

/**
 * La vista **attraversa le quote**, e questo test esiste perche' per un po' non lo faceva.
 *
 * `VisibleCells` costruiva i candidati con `URTHexLibrary::HexArea`, che cabla `Center.Layer`: l'area era
 * piatta sulla quota dell'osservatore, e tutto cio' che stava su un altro piano risultava invisibile. Su una
 * mappa esagonale MULTILIVELLO — che e' il substrato della v0.1, non un caso limite — significava che nessuna
 * squadra vedeva mai il ponte sopra la testa ne' il fondo sotto.
 *
 * Il difetto e' rimasto invisibile finche' nessuno **consumava** la vista: CP 13.2 l'ha reso fatale, perche'
 * da li' un bersaglio non visto non e' bersagliabile e ogni ingaggio fra piani diversi diventava illegale.
 * E' il difetto ricorrente di questo repository visto dall'altro lato: non un dato che nessuno legge, ma un
 * dato sbagliato che nessuno leggeva **ancora**.
 *
 * La regola non e' nuova e non concede niente alla verticalita': `HasLineOfSight` e `HexDistance` il layer
 * non lo guardano gia' oggi: la linea di tiro si valuta sulla proiezione esagonale. Erano la vista e il tiro
 * a dire due cose diverse sullo stesso spazio. High Ground resta senza bonus numerico alla vista (canone
 * v0.1) — qui non si aggiunge un vantaggio, si toglie una cecita' che nessuna decisione aveva stabilito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPerceptionSeesAcrossLayersTest,
	"RefactorTactics.Vision.SeesAcrossLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPerceptionSeesAcrossLayersTest::RunTest(const FString&)
{
	// Due piani sovrapposti, entrambi pieni: e' la forma di una mappa con un ponte o un ballatoio.
	URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	for (int32 Layer = 0; Layer <= 1; ++Layer)
	{
		for (const FRTCellId& Flat : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 5))
		{
			Map->AddOrUpdateCell(FRTHexCellData(FRTCellId(Flat.X, Flat.Y, Layer)));
		}
	}
	Map->SortCells();

	const FRTPerceiver Observer = Watcher(FRTCellId(0, 0, 0), ERTHexDirection::E, /*VisionRange*/ 5);
	const TArray<FRTCellId> Seen = URTPerceptionLibrary::VisibleCells(Map, Observer);

	// Premessa: sul PROPRIO piano si vede, e questo funzionava anche prima. Senza la premessa un fallimento
	// qui sotto non distinguerebbe «non vede di sopra» da «non vede affatto».
	if (!TestTrue(TEXT("premessa: vede sul proprio piano"), Seen.Contains(FRTCellId(3, 0, 0))))
	{
		return false;
	}

	// Ravvicinata (entro 2 celle, ogni direzione) e arco frontale, entrambe sul piano DI SOPRA.
	TestTrue(TEXT("vede la cella adiacente di sopra"), Seen.Contains(FRTCellId(1, 0, 1)));
	TestTrue(TEXT("vede nel proprio arco frontale di sopra"), Seen.Contains(FRTCellId(3, 0, 1)));

	// E la quota non regala nulla: cio' che e' fuori dall'arco resta invisibile su OGNI piano. Senza questo
	// controllo il test passerebbe anche con una vista che ha smesso di filtrare.
	TestFalse(TEXT("dietro le spalle non si vede, nemmeno di sopra"), Seen.Contains(FRTCellId(-4, 0, 1)));
	TestFalse(TEXT("dietro le spalle non si vede, nemmeno sul proprio piano"), Seen.Contains(FRTCellId(-4, 0, 0)));

	// Una cella che la mappa NON contiene non compare, anche se cadrebbe nell'arco: i candidati sono le celle
	// che esistono, non un'area generata. E' la differenza che il difetto aveva reso invisibile.
	TestFalse(TEXT("una quota inesistente non si inventa"), Seen.Contains(FRTCellId(2, 0, 7)));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
