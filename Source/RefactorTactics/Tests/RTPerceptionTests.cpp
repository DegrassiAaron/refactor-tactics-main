#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
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
 *
 * ➕ `D-511`, #3743: fra due piani la vista segue ora una linea vera, da testa a testa, che si ferma sui
 * pavimenti. La fixture di prima — due piani PIENI — con quella regola non mostra piu' niente del piano di
 * sopra, ed e' giusto cosi': chi guarda aveva un soffitto. Qui la piattaforma comincia DUE celle piu' in la',
 * quindi chi guarda sta allo scoperto, e i candidati di un altro piano si vedono ancora. High Ground resta
 * senza bonus numerico alla vista (`D-018`): la quota non regala niente, toglie cio' che un pavimento copre.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPerceptionSeesAcrossLayersTest,
	"RefactorTactics.Vision.SeesAcrossLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPerceptionSeesAcrossLayersTest::RunTest(const FString&)
{
	// Un piano terra pieno, e una piattaforma al piano 1 lungo la riga di chi guarda: da x=2 a x=4. Piu' una
	// cella isolata alle sue spalle, per chiedere che la quota non regali nulla.
	URTHexMapAsset* Map = MakePerceptionMap(5);
	for (const FRTCellId& Id : { FRTCellId(2, 0, 1), FRTCellId(3, 0, 1), FRTCellId(4, 0, 1), FRTCellId(-4, 0, 1) })
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
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

	// La cella di bordo della piattaforma, a due celle: e quella dietro, a tre. Entrambe sul piano DI SOPRA.
	TestTrue(TEXT("vede la cella di bordo della piattaforma"), Seen.Contains(FRTCellId(2, 0, 1)));
	TestTrue(TEXT("vede la cella dietro il bordo, da tre celle"), Seen.Contains(FRTCellId(3, 0, 1)));

	// Due celle dietro il bordo servono cinque celle di distanza: da quattro il bordo la copre (`D-511`).
	TestFalse(TEXT("due celle dietro il bordo, da quattro celle: il bordo la copre"), Seen.Contains(FRTCellId(4, 0, 1)));

	// E la quota non regala nulla: cio' che e' fuori dall'arco resta invisibile su OGNI piano, anche quando la
	// linea e' libera. Senza la premessa sulla linea il test passerebbe anche con un pavimento in mezzo.
	if (!TestTrue(TEXT("premessa: verso la cella alle spalle la linea fra piani e' libera"),
		URTPerceptionLibrary::HasSightAcrossLayers(Map, Observer.Cell, FRTCellId(-4, 0, 1))))
	{
		return false;
	}
	TestFalse(TEXT("dietro le spalle non si vede, nemmeno di sopra"), Seen.Contains(FRTCellId(-4, 0, 1)));
	TestFalse(TEXT("dietro le spalle non si vede, nemmeno sul proprio piano"), Seen.Contains(FRTCellId(-4, 0, 0)));

	// Una cella che la mappa NON contiene non compare, anche se cadrebbe nell'arco: i candidati sono le celle
	// che esistono, non un'area generata. E' la differenza che il difetto aveva reso invisibile.
	TestFalse(TEXT("una quota inesistente non si inventa"), Seen.Contains(FRTCellId(2, 0, 7)));
	return true;
}

/**
 * **Sotto un pavimento non si vede niente di cio' che c'e' sopra, e da sopra niente di cio' che c'e' sotto**
 * (`D-511`, #3743). Vale per le CELLE, non solo per chi le occupa: e' la scelta d'autore «Anche le celle».
 *
 * Due piani pieni, uno sull'altro: e' la fixture con cui fino a #3743 questo file asseriva il contrario.
 *
 * 🔑 **Il controllo che da' senso al rosso e' il buco.** Tolta UNA cella dal soffitto — quella in cui la linea
 * raggiunge la quota — la cella oltre il buco si vede: non e' il piano di sopra a essere invisibile, e' il
 * pavimento a coprirlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPerceptionFloorHidesLayerTest,
	"RefactorTactics.Vision.FloorHidesTheOtherLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPerceptionFloorHidesLayerTest::RunTest(const FString&)
{
	auto TwoFloors = [](const FRTCellId* Hole)
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (int32 Layer = 0; Layer <= 1; ++Layer)
		{
			for (const FRTCellId& Flat : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 5))
			{
				const FRTCellId Id(Flat.X, Flat.Y, Layer);
				if (Hole == nullptr || !(Id == *Hole))
				{
					Map->AddOrUpdateCell(FRTHexCellData(Id));
				}
			}
		}
		Map->SortCells();
		return Map;
	};
	auto CountOnLayer = [](const TArray<FRTCellId>& Cells, int32 Layer)
	{
		int32 Count = 0;
		for (const FRTCellId& Cell : Cells)
		{
			Count += (Cell.Layer == Layer) ? 1 : 0;
		}
		return Count;
	};

	const URTHexMapAsset* Full = TwoFloors(nullptr);
	const FRTPerceiver Sotto = Watcher(FRTCellId(0, 0, 0), ERTHexDirection::E, /*VisionRange*/ 5);
	const FRTPerceiver Sopra = Watcher(FRTCellId(0, 0, 1), ERTHexDirection::E, /*VisionRange*/ 5);

	const TArray<FRTCellId> DaSotto = URTPerceptionLibrary::VisibleCells(Full, Sotto);
	const TArray<FRTCellId> DaSopra = URTPerceptionLibrary::VisibleCells(Full, Sopra);
	if (!TestTrue(TEXT("premessa: da sotto si vede il proprio piano"), CountOnLayer(DaSotto, 0) > 10)
		|| !TestTrue(TEXT("premessa: da sopra si vede il proprio piano"), CountOnLayer(DaSopra, 1) > 10))
	{
		return false;
	}
	TestEqual(TEXT("da sotto un soffitto pieno, nessuna cella del piano di sopra"), CountOnLayer(DaSotto, 1), 0);
	TestEqual(TEXT("da sopra un pavimento pieno, nessuna cella del piano di sotto"), CountOnLayer(DaSopra, 0), 0);

	// Il buco: la linea da (0,0,0) a (2,0,1) raggiunge la quota del piano 1 sopra la cella (1,0).
	const FRTCellId Hole(1, 0, 1);
	const URTHexMapAsset* Holed = TwoFloors(&Hole);
	const TArray<FRTCellId> DalBuco = URTPerceptionLibrary::VisibleCells(Holed, Sotto);
	TestTrue(TEXT("dal buco si vede la cella oltre"), DalBuco.Contains(FRTCellId(2, 0, 1)));
	TestFalse(TEXT("e il buco non e' una cella: non compare"), DalBuco.Contains(Hole));
	TestFalse(TEXT("la cella sopra la testa resta coperta"), DalBuco.Contains(FRTCellId(0, 0, 1)));
	return true;
}

/**
 * **Chi sta sopra la testa di un altro non lo vede, e non ne e' visto** (`D-510`, #3741).
 *
 * Decisione d'autore del 2026-10-13: «Se uno sta nella cella sopra un nemico, i 2 non si vedono (la linea di
 * visuale passa al centro perpendicolare)». La cella impilata sta a distanza ZERO sulla proiezione: fino a
 * #3741 la prendeva la consapevolezza ravvicinata, in ogni direzione. Da `D-511` e' il caso verticale della
 * linea fra piani, e questo test tiene vero che lo sia rimasto.
 *
 * 🔑 **Due controlli danno senso al rosso.** Un compagno spostato di UNA cella, allo scoperto, quella stessa
 * cella la vede: e' la colonna a nasconderla, non il piano. E la LINEA DI TIRO fra le due celle impilate resta
 * libera: la regola e' del contatto, non della linea. Messa nella linea fa orbitare i bot sull'arena generata
 * (`Bot.StalemateProbeGeneratedArenaFilterNeverStepsBack`), ed e' la ragione per cui questo test la fissa li'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPerceptionStackedColumnTest,
	"RefactorTactics.Vision.StackedColumnIsNotSeen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPerceptionStackedColumnTest::RunTest(const FString&)
{
	// Un piano terra pieno e una PILA: due celle sopra la (0,0), una per piano. Niente altro in quota, cosi'
	// chi sta accanto alla pila e' allo scoperto.
	URTHexMapAsset* Map = MakePerceptionMap(5);
	for (const FRTCellId& Id : { FRTCellId(0, 0, 1), FRTCellId(0, 0, 2) })
	{
		Map->AddOrUpdateCell(FRTHexCellData(Id));
	}
	Map->SortCells();

	const FRTPerceiver Sotto = Watcher(FRTCellId(0, 0, 0), ERTHexDirection::E, /*VisionRange*/ 5);
	const TArray<FRTCellId> DaSotto = URTPerceptionLibrary::VisibleCells(Map, Sotto);
	if (!TestTrue(TEXT("premessa: da sotto si vede la propria cella"), DaSotto.Contains(FRTCellId(0, 0, 0)))
		|| !TestTrue(TEXT("premessa: e la cella accanto sul proprio piano"), DaSotto.Contains(FRTCellId(1, 0, 0))))
	{
		return false;
	}
	TestFalse(TEXT("da sotto non si vede la cella sopra la testa"), DaSotto.Contains(FRTCellId(0, 0, 1)));
	TestFalse(TEXT("e nemmeno quella due piani piu' su"), DaSotto.Contains(FRTCellId(0, 0, 2)));

	const FRTPerceiver Sopra = Watcher(FRTCellId(0, 0, 1), ERTHexDirection::E, /*VisionRange*/ 5);
	const TArray<FRTCellId> DaSopra = URTPerceptionLibrary::VisibleCells(Map, Sopra);
	if (!TestTrue(TEXT("premessa: da sopra si vede la propria cella"), DaSopra.Contains(FRTCellId(0, 0, 1))))
	{
		return false;
	}
	TestFalse(TEXT("da sopra non si vede la cella sotto i piedi"), DaSopra.Contains(FRTCellId(0, 0, 0)));
	TestFalse(TEXT("ne' quella sopra la testa"), DaSopra.Contains(FRTCellId(0, 0, 2)));

	// La domanda di squadra, che e' quella che decide se un nemico e' rilevato.
	TArray<FRTPerceiver> Squadra;
	Squadra.Add(Sotto);
	TestTrue(TEXT("un nemico sopra la testa resta nascosto"),
		URTPerceptionLibrary::TeamAwarenessOfCell(Map, Squadra, FRTCellId(0, 0, 1)) == ERTAwareness::Hidden);

	// Un compagno accanto, allo scoperto, la vede: la conoscenza e' di squadra, e la regola toglie UNA linea,
	// non la cella.
	Squadra.Add(Watcher(FRTCellId(-1, 0, 0), ERTHexDirection::E, /*VisionRange*/ 5));
	TestTrue(TEXT("un compagno che non sta nella colonna la rileva per la squadra"),
		URTPerceptionLibrary::TeamAwarenessOfCell(Map, Squadra, FRTCellId(0, 0, 1)) == ERTAwareness::Detected);

	// ⛔ La linea di TIRO non cambia: e' la geometria su cui i bot scelgono le celle.
	TestTrue(TEXT("la linea di vista fra le due celle impilate resta libera: la regola e' del contatto"),
		URTHexVisionLibrary::HasLineOfSight(Map, FRTCellId(0, 0, 0), FRTCellId(0, 0, 1)));
	return true;
}

/**
 * La tavola `S1` della seduta `U88`, come **conoscenza di squadra**: e' la scena davanti a cui l'autore ha
 * scritto «mi sembra che aevik non potrebbe vedere nessuno dei due nemici sopra» (`D-511`, #3743).
 *
 * Sulla piattaforma di `TestArena`: chi sta a terra SOTTO non vede nessuna delle sue celle; un compagno allo
 * scoperto ne vede il bordo e non la cella dietro; un terzo, dall'altro lato, vede quella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPerceptionPlatformTeamKnowledgeTest,
	"RefactorTactics.Vision.PlatformIsSeenFromTheOpenNotFromUnderneath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPerceptionPlatformTeamKnowledgeTest::RunTest(const FString&)
{
	const URTHexMapAsset* Map = URTMatchSetupLibrary::MakeTestArena(GetTransientPackage());
	if (!TestNotNull(TEXT("premessa: TestArena esiste"), Map))
	{
		return false;
	}
	const TArray<FRTCellId> Platform = { FRTCellId(2, -1, 1), FRTCellId(2, 0, 1), FRTCellId(3, -1, 1), FRTCellId(3, 0, 1) };
	const FRTCellId Bordo(2, 0, 1);
	const FRTCellId Dietro(3, 0, 1);

	// Sotto la piattaforma, rivolto verso di essa: la vista piu' lunga del roster non cambia niente.
	const FRTPerceiver Sotto = Watcher(FRTCellId(2, 0, 0), ERTHexDirection::E, /*VisionRange*/ 7);
	const TArray<FRTCellId> DaSotto = URTPerceptionLibrary::VisibleCells(Map, Sotto);
	if (!TestTrue(TEXT("premessa: da sotto si vede a terra"), DaSotto.Contains(FRTCellId(3, 0, 0))))
	{
		return false;
	}
	for (const FRTCellId& Cell : Platform)
	{
		TestFalse(*FString::Printf(TEXT("da sotto la piattaforma non si vede la sua cella (%d,%d)"), Cell.X, Cell.Y),
			DaSotto.Contains(Cell));
	}

	TArray<FRTPerceiver> Squadra;
	Squadra.Add(Sotto);
	TestTrue(TEXT("chi sta sul bordo e' nascosto a chi sta sotto"),
		URTPerceptionLibrary::TeamAwarenessOfCell(Map, Squadra, Bordo) == ERTAwareness::Hidden);
	TestTrue(TEXT("chi sta dietro e' nascosto a chi sta sotto"),
		URTPerceptionLibrary::TeamAwarenessOfCell(Map, Squadra, Dietro) == ERTAwareness::Hidden);

	// Un compagno allo scoperto, accanto alla piattaforma.
	Squadra.Add(Watcher(FRTCellId(1, 0, 0), ERTHexDirection::E, /*VisionRange*/ 7));
	TestTrue(TEXT("dallo scoperto si rileva chi sta sul bordo"),
		URTPerceptionLibrary::TeamAwarenessOfCell(Map, Squadra, Bordo) == ERTAwareness::Detected);
	TestTrue(TEXT("ma non chi sta una cella piu' indietro: il bordo lo copre"),
		URTPerceptionLibrary::TeamAwarenessOfCell(Map, Squadra, Dietro) == ERTAwareness::Hidden);

	// Un terzo dall'altro lato, per cui la cella «dietro» e' quella di bordo.
	Squadra.Add(Watcher(FRTCellId(4, 0, 0), ERTHexDirection::W, /*VisionRange*/ 7));
	TestTrue(TEXT("dall'altro lato si rileva anche l'altro"),
		URTPerceptionLibrary::TeamAwarenessOfCell(Map, Squadra, Dietro) == ERTAwareness::Detected);

	// E al contrario, con la stessa linea: chi sta sul bordo vede la cella a terra li' accanto, non quella sotto
	// i propri piedi ne' quelle sotto la piattaforma.
	const FRTPerceiver SulBordo = Watcher(Bordo, ERTHexDirection::W, /*VisionRange*/ 7);
	const TArray<FRTCellId> DalBordo = URTPerceptionLibrary::VisibleCells(Map, SulBordo);
	TestTrue(TEXT("dal bordo si vede la cella a terra accanto"), DalBordo.Contains(FRTCellId(1, 0, 0)));
	TestFalse(TEXT("dal bordo non si vede la cella sotto i piedi"), DalBordo.Contains(FRTCellId(2, 0, 0)));
	TestFalse(TEXT("ne' quella sotto la cella dietro"), DalBordo.Contains(FRTCellId(3, 0, 0)));

	// ➕ **La regola non toglie soltanto.** Da terra, tre celle dietro il muro di `q = 0`, chi sta sul bordo della
	// piattaforma si VEDE: la linea da testa a testa passa sopra il muro, che ferma solo il tratto basso. La linea
	// di tiro, che sta sul piano di chi guarda, quel muro lo trova: e' la premessa, ed e' cio' che la regola di
	// ieri rispondeva. Chi sta a terra dietro lo stesso muro resta nascosto, e da piu' vicino il muro copre ancora.
	const FRTPerceiver OltreIlMuro = Watcher(FRTCellId(-3, 0, 0), ERTHexDirection::E, /*VisionRange*/ 7);
	const FRTCellId ATerraDietroIlMuro(1, 0, 0);
	if (!TestFalse(TEXT("premessa: la linea di TIRO da oltre il muro al bordo della piattaforma e' ferma"),
			URTHexVisionLibrary::HasLineOfSight(Map, OltreIlMuro.Cell, Bordo))
		|| !TestFalse(TEXT("premessa: e lo e' anche verso la cella a terra dietro il muro"),
			URTHexVisionLibrary::HasLineOfSight(Map, OltreIlMuro.Cell, ATerraDietroIlMuro)))
	{
		return false;
	}
	const TArray<FRTCellId> DaOltre = URTPerceptionLibrary::VisibleCells(Map, OltreIlMuro);
	TestTrue(TEXT("da tre celle dietro il muro si vede chi sta in alto sul bordo: la linea passa sopra il muro"),
		DaOltre.Contains(Bordo));
	TestFalse(TEXT("e non chi sta a terra dietro il muro"), DaOltre.Contains(ATerraDietroIlMuro));

	const FRTPerceiver PiuVicino = Watcher(FRTCellId(-2, 0, 0), ERTHexDirection::E, /*VisionRange*/ 7);
	TestFalse(TEXT("da due celle dietro il muro la linea lo attraversa in basso: il bordo resta coperto"),
		URTPerceptionLibrary::VisibleCells(Map, PiuVicino).Contains(Bordo));
	return true;
}

/**
 * 🔴 **La fixture `VisionSplit` produce davvero due regioni visibili DISGIUNTE**, e questo test esiste
 * perche' altrimenti sarebbe una promessa scritta in un commento.
 *
 * E' il caso che [#1715](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1715) deve giudicare
 * e che nessuna arena esistente produce: sulle arene di raggio **4** una squadra vede quasi tutto, quindi la
 * sua vista e' una macchia sola e un contorno coinciderebbe col bordo della mappa.
 *
 * 🔑 **L'oracolo e' il numero di COMPONENTI CONNESSE**, non un conteggio di celle: e' la proprieta'
 * che rende il caso interessante, ed e' invariante a mappa, roster e `VisionRange` finche' la geometria
 * regge. La connessione si calcola su `URTHexLibrary::Neighbors`, che **non attraversa i layer** — quindi
 * un layer diverso sarebbe una componente a se' per costruzione del grafo — ed e' la ragione per cui
 * questo test conta il SOLO suolo, anche ora che `VisionSplit` non ha piu' celle fuori dal layer 0.
 *
 * ⚠️ Gli osservatori non sono inventati: si chiedono a `PickStartCells`, la **stessa** funzione che
 * allestisce la partita vera. Se un giorno cambiasse l'ordine delle celle di partenza, i due compagni
 * potrebbero nascere nella stessa camera e questo test diventerebbe rosso — il che e' esattamente cio' che
 * si vuole, perche' la fixture avrebbe smesso di servire al proprio scopo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPerceptionVisionSplitYieldsDisconnectedRegionsTest,
	"RefactorTactics.Perception.VisionSplitYieldsDisconnectedRegions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPerceptionVisionSplitYieldsDisconnectedRegionsTest::RunTest(const FString&)
{
	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeVisionSplitArena(GetTransientPackage());
	if (!TestNotNull(TEXT("arena"), Arena)) { return false; }

	// Le celle di partenza VERE: team 0 prende le prime due percorribili in ordine `(q, r)`.
	const TArray<FRTCellId> Start = URTMatchSetupLibrary::PickStartCells(Arena, /*NumPerTeam=*/ 2, /*Layer=*/ 0);
	if (!TestEqual(TEXT("quattro celle di partenza (2 per squadra)"), Start.Num(), 4)) { return false; }

	AddInfo(FString::Printf(TEXT("team 0 parte da (%d,%d,L%d) e (%d,%d,L%d)"),
		Start[0].X, Start[0].Y, Start[0].Layer, Start[1].X, Start[1].Y, Start[1].Layer));

	// ⚠️ `VisionRange` 5: il PIU' CORTO del roster (Phase, Branth). Se le regioni restano disgiunte col
	// piu' corto non e' una prova per gli altri; e' il contrario — con una vista piu' lunga si toccherebbero
	// piu' facilmente, quindi il caso difficile per questo test e' il **7** di Aevik, provato sotto.
	auto Osservatori = [&](int32 Range)
	{
		TArray<FRTPerceiver> Team;
		for (int32 I = 0; I < 2; ++I)
		{
			FRTPerceiver P;
			P.Cell = Start[I];
			P.VisionRange = Range;
			Team.Add(P);
		}
		return Team;
	};

	// Componenti connesse dell'insieme visibile, per BFS sui vicini dello STESSO layer.
	auto Componenti = [](const TArray<FRTCellId>& Celle)
	{
		TSet<FRTCellId> Rimaste(Celle);
		int32 N = 0;
		while (Rimaste.Num() > 0)
		{
			++N;
			TArray<FRTCellId> Coda;
			Coda.Add(*Rimaste.CreateConstIterator());
			Rimaste.Remove(Coda[0]);
			while (Coda.Num() > 0)
			{
				const FRTCellId Corrente = Coda.Pop();
				for (const FRTCellId& Vicino : URTHexLibrary::Neighbors(Corrente))
				{
					if (Rimaste.Remove(Vicino) > 0) { Coda.Add(Vicino); }
				}
			}
		}
		return N;
	};

	for (const int32 Range : { 5, 7 })
	{
		const TArray<FRTCellId> Viste = URTPerceptionLibrary::TeamVisibleCells(Arena, Osservatori(Range));
		// 🔴 **Si contano le componenti del SOLO layer 0, e la ragione e' una mutazione fallita.**
		// Contandole su tutti i layer il test passava anche con il muro reso trasparente: la seconda
		// componente era la **piattaforma del layer 1** — separata per costruzione del grafo, visto che
		// `Neighbors` non attraversa i layer — e non la camera sud. Il test misurava un artefatto della
		// topologia invece della divisione che la fixture esiste per produrre.
		TArray<FRTCellId> Suolo;
		for (const FRTCellId& C : Viste) { if (C.Layer == 0) { Suolo.Add(C); } }
		const int32 NumComponenti = Componenti(Suolo);
		AddInfo(FString::Printf(
			TEXT("VisionRange %d: %d celle viste (%d sul suolo) in %d componenti di suolo"),
			Range, Viste.Num(), Suolo.Num(), NumComponenti));

		TestTrue(*FString::Printf(TEXT("VisionRange %d: la squadra vede qualcosa"), Range), Viste.Num() > 0);

		// 🔑 L'asserzione che la fixture esiste per soddisfare.
		TestTrue(*FString::Printf(
				TEXT("VisionRange %d: la vista di squadra e' in almeno DUE regioni di SUOLO disgiunte (ne ha %d)"),
				Range, NumComponenti),
			NumComponenti >= 2);

		// E resta terreno mai visto: se la squadra vedesse tutto, non ci sarebbe un confine da giudicare.
		TestTrue(*FString::Printf(
				TEXT("VisionRange %d: resta terreno non visto (%d viste su %d celle)"),
				Range, Viste.Num(), Arena->NumCells()),
			Viste.Num() < Arena->NumCells());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
