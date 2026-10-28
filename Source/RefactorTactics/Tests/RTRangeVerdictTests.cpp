// La portata a tre stati di una skill (`#3750`, fetta F5; spec dell'ispettore §4 e P7): libero, bloccato, senza verdetto.
// Fetta B: il calcolo puro.
//
// Ogni divieto ha il suo gemello. Un test che mostra solo la forma giusta passerebbe anche con una portata che dipende da
// cio' che la squadra non vede: il difetto che D-552 esiste per impedire non cambia la forma, cambia da cosa dipende.
// L'INSIEME delle celle e' sotto lo stesso vincolo dei verdetti (il Fumo cappa la portata), e ha il suo gemello a parte.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTRangeVerdict.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTTeamKnowledge.h"
#include "Turn/RTMatchSetupLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	constexpr int32 RvRadius = 8;
	constexpr int32 RvRange = 5;

	URTHexMapAsset* RvArena()
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), RvRadius);
	}

	FRTCellId RvFrom()
	{
		return FRTCellId(0, 0, 0);
	}

	/** Visibili quelle scelte; esplorata ogni visibile (come dopo il refresh della conoscenza), piu' quelle che il test aggiunge. */
	FRTTeamKnowledge RvKnowledge(const URTHexMapAsset* Map, TFunctionRef<bool(const FRTCellId&)> IsVisible)
	{
		FRTTeamKnowledge Knowledge;
		Knowledge.TeamId = 0;
		for (const FRTHexCellData& Cell : Map->Cells)
		{
			if (IsVisible(Cell.Id))
			{
				Knowledge.VisibleCells.Add(Cell.Id);
				Knowledge.ExploredCells.Add(Cell.Id);
			}
		}
		return Knowledge;
	}

	FRTTeamKnowledge RvAllVisible(const URTHexMapAsset* Map)
	{
		return RvKnowledge(Map, [](const FRTCellId&) { return true; });
	}

	/** Tutto in vista tranne le celle indicate. */
	FRTTeamKnowledge RvAllVisibleBut(const URTHexMapAsset* Map, const TArray<FRTCellId>& Hidden)
	{
		return RvKnowledge(Map, [&Hidden](const FRTCellId& Cell) { return !Hidden.Contains(Cell); });
	}

	void RvEdit(URTHexMapAsset* Map, const FRTCellId& Id, TFunctionRef<void(FRTHexCellData&)> Edit)
	{
		for (FRTHexCellData& Cell : Map->Cells)
		{
			if (Cell.Id == Id) { Edit(Cell); }
		}
	}

	void RvWall(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		RvEdit(Map, Id, [](FRTHexCellData& Cell) { Cell.bBlocksLineOfSight = true; });
	}

	void RvSmoke(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		RvEdit(Map, Id, [](FRTHexCellData& Cell) { Cell.Surface = ERTHexSurface::Smoke; });
	}

	URTHexMapAsset* RvTwin(const URTHexMapAsset* Map)
	{
		return DuplicateObject<URTHexMapAsset>(Map, GetTransientPackage());
	}

	TArray<FRTRangeCellVerdict> RvClassify(const URTHexMapAsset* Map, const FRTTeamKnowledge& Knowledge,
		ERTLineOfSightPolicy Policy = ERTLineOfSightPolicy::Required, int32 Range = RvRange, int32 MinRange = 0)
	{
		return URTRangeVerdictLibrary::ClassifyFor(GetTransientPackage(), Map, Knowledge, RvFrom(), Range, Policy, MinRange);
	}

	/** Una stringa per lista: i confronti fra mondi gemelli sono su TUTTA l'uscita, celle e verdetti nell'ordine in cui escono. */
	FString RvSignature(const TArray<FRTRangeCellVerdict>& List)
	{
		FString Out;
		for (const FRTRangeCellVerdict& Entry : List)
		{
			Out += FString::Printf(TEXT("%d,%d,%d:%d;"), static_cast<int32>(Entry.Cell.X), static_cast<int32>(Entry.Cell.Y),
				static_cast<int32>(Entry.Cell.Layer), static_cast<int32>(Entry.Verdict));
		}
		return Out;
	}

	const FRTRangeCellVerdict* RvFind(const TArray<FRTRangeCellVerdict>& List, const FRTCellId& Cell)
	{
		return List.FindByPredicate([&Cell](const FRTRangeCellVerdict& Entry) { return Entry.Cell == Cell; });
	}

	bool RvIs(const TArray<FRTRangeCellVerdict>& List, const FRTCellId& Cell, ERTRangeVerdict Verdict)
	{
		const FRTRangeCellVerdict* Entry = RvFind(List, Cell);
		return Entry != nullptr && Entry->Verdict == Verdict;
	}

	TArray<FRTCellId> RvCells(const TArray<FRTRangeCellVerdict>& List)
	{
		TArray<FRTCellId> Cells;
		for (const FRTRangeCellVerdict& Entry : List) { Cells.Add(Entry.Cell); }
		return Cells;
	}
}

// Su un campo aperto e tutto in vista ogni cella in portata e' libera, e l'insieme e' quello dell'azione armata.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRangeVerdictOpenFieldTest,
	"RefactorTactics.RangeVerdict.AnOpenSeenFieldIsAllFreeAndMatchesTheArmedRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRangeVerdictOpenFieldTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = RvArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }

	const TArray<FRTRangeCellVerdict> Out = RvClassify(Map, RvAllVisible(Map));
	const TArray<FRTCellId> Armed = URTCombatLibrary::TargetableRangeCells(Map, RvFrom(), RvRange, ERTLineOfSightPolicy::Required);

	TestTrue(TEXT("premessa: la portata non e' vuota"), Armed.Num() > 1);
	TestTrue(TEXT("l'insieme e' quello di TargetableRangeCells, cella per cella"), RvCells(Out) == Armed);
	int32 NotFree = 0;
	for (const FRTRangeCellVerdict& Entry : Out) { NotFree += Entry.Verdict == ERTRangeVerdict::Free ? 0 : 1; }
	TestEqual(TEXT("senza ostacoli e tutto in vista, nessuna cella e' bloccata o senza verdetto"), NotFree, 0);
	return true;
}

// Un muro in vista blocca le celle dietro di esso e solo quelle.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRangeVerdictVisibleWallTest,
	"RefactorTactics.RangeVerdict.AVisibleWallBlocksTheCellsBehindItAndOnlyThose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRangeVerdictVisibleWallTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = RvArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	RvWall(Map, FRTCellId(2, 0, 0));

	const TArray<FRTRangeCellVerdict> Out = RvClassify(Map, RvAllVisible(Map));

	TestTrue(TEXT("dietro il muro, a 3 celle: bloccata"), RvIs(Out, FRTCellId(3, 0, 0), ERTRangeVerdict::Blocked));
	TestTrue(TEXT("dietro il muro, a 5 celle: bloccata"), RvIs(Out, FRTCellId(5, 0, 0), ERTRangeVerdict::Blocked));
	TestTrue(TEXT("davanti al muro: libera"), RvIs(Out, FRTCellId(1, 0, 0), ERTRangeVerdict::Free));
	TestTrue(TEXT("dalla parte opposta: libera"), RvIs(Out, FRTCellId(-3, 0, 0), ERTRangeVerdict::Free));
	TestTrue(TEXT("su un altro asse: libera"), RvIs(Out, FRTCellId(0, 3, 0), ERTRangeVerdict::Free));
	TestNotNull(TEXT("una cella bloccata resta NELL'INSIEME: e' in portata, solo senza tiro"), RvFind(Out, FRTCellId(4, 0, 0)));
	return true;
}

// ⚠️ Cio' che questo test protegge e' il CONTROLLO DI LINEA, non la mappa neutra: ogni muro nascosto sta sulla linea della
// cella asserita, quindi il controllo risponde «senza verdetto» prima che la mappa conti. La mappa neutra e' osservabile
// solo dall'INSIEME (il gemello del Fumo): su una linea interamente in vista coincide con quella vera.
//
// 🔴 MONDI GEMELLI SUL VERDETTO. Due mappe che differiscono solo in un muro che la squadra non vede adesso danno la stessa
// uscita, e la cella dietro il muro non ha verdetto. Due varianti del "non vede": mai vista e ESPLORATA ma non piu' in vista
// (D-225, D-227: contiene cio' che c'e' adesso, che la squadra non puo' leggere).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRangeVerdictHiddenWallTwinsTest,
	"RefactorTactics.RangeVerdict.TheVerdictIsTheSameWhenOnlyAHiddenWallDiffers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRangeVerdictHiddenWallTwinsTest::RunTest(const FString&)
{
	URTHexMapAsset* WorldA = RvArena();
	if (!TestNotNull(TEXT("l'arena esiste"), WorldA)) { return false; }
	URTHexMapAsset* WorldB = RvTwin(WorldA);
	URTHexMapAsset* WorldC = RvTwin(WorldA);
	if (!TestTrue(TEXT("le gemelle esistono"), WorldB && WorldC)) { return false; }

	const FRTCellId NeverSeen(2, 0, 0);
	const FRTCellId ExploredOnly(-2, 0, 0);
	// B differisce da A solo in due muri non in vista: uno mai visto, uno esplorato e non piu' in vista.
	RvWall(WorldB, NeverSeen);
	RvWall(WorldB, ExploredOnly);
	// C differisce da A in un muro IN VISTA.
	RvWall(WorldC, FRTCellId(0, 2, 0));

	FRTTeamKnowledge Knowledge = RvAllVisibleBut(WorldA, { NeverSeen, ExploredOnly });
	Knowledge.ExploredCells.Add(ExploredOnly);

	const TArray<FRTRangeCellVerdict> InA = RvClassify(WorldA, Knowledge);
	const TArray<FRTRangeCellVerdict> InB = RvClassify(WorldB, Knowledge);
	const TArray<FRTRangeCellVerdict> InC = RvClassify(WorldC, Knowledge);

	TestEqual(TEXT("un muro mai visto e uno esplorato e non piu' in vista non cambiano l'uscita"), RvSignature(InA), RvSignature(InB));
	TestTrue(TEXT("la cella dietro il muro mai visto non ha verdetto"), RvIs(InB, FRTCellId(3, 0, 0), ERTRangeVerdict::Unverdicted));
	TestTrue(TEXT("e neppure quella dietro il muro esplorato"), RvIs(InB, FRTCellId(-3, 0, 0), ERTRangeVerdict::Unverdicted));
	TestTrue(TEXT("lo stesso in A, dove il muro non c'e': la linea attraversa comunque una cella non in vista"),
		RvIs(InA, FRTCellId(3, 0, 0), ERTRangeVerdict::Unverdicted) && RvIs(InA, FRTCellId(-3, 0, 0), ERTRangeVerdict::Unverdicted));

	// Controllo positivo 1: le due mappe SI DISTINGUONO se le si guardasse per intero.
	const FRTTeamKnowledge Everything = RvAllVisible(WorldA);
	TestTrue(TEXT("premessa: guardando tutto, A lascia libera la cella a 3"), RvIs(RvClassify(WorldA, Everything), FRTCellId(3, 0, 0), ERTRangeVerdict::Free));
	TestTrue(TEXT("premessa: guardando tutto, B la blocca"), RvIs(RvClassify(WorldB, Everything), FRTCellId(3, 0, 0), ERTRangeVerdict::Blocked));
	TestTrue(TEXT("premessa: lo stesso sul lato esplorato"), RvIs(RvClassify(WorldA, Everything), FRTCellId(-3, 0, 0), ERTRangeVerdict::Free)
		&& RvIs(RvClassify(WorldB, Everything), FRTCellId(-3, 0, 0), ERTRangeVerdict::Blocked));

	// Controllo positivo 2: un muro IN VISTA cambia l'uscita.
	TestTrue(TEXT("un muro in vista cambia il verdetto dietro di esso"),
		RvIs(InA, FRTCellId(0, 3, 0), ERTRangeVerdict::Free) && RvIs(InC, FRTCellId(0, 3, 0), ERTRangeVerdict::Blocked));
	return true;
}

// 🔴 L'INSIEME E' SOTTO LO STESSO VINCOLO DEI VERDETTI. Il Fumo cappa la portata efficace (a 2 celle): un Fumo su una cella
// che la squadra non vede restringerebbe l'insieme, e chi guarda lo leggerebbe dalla sua forma. Due mappe che differiscono solo
// in un Fumo non in vista (mai visto, o esplorato e non piu' in vista) danno la stessa uscita; un Fumo IN VISTA la cambia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRangeVerdictHiddenSmokeTwinsTest,
	"RefactorTactics.RangeVerdict.TheSetIsTheSameWhenOnlyAHiddenSmokeDiffers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRangeVerdictHiddenSmokeTwinsTest::RunTest(const FString&)
{
	URTHexMapAsset* WorldA = RvArena();
	if (!TestNotNull(TEXT("l'arena esiste"), WorldA)) { return false; }
	URTHexMapAsset* WorldB = RvTwin(WorldA);
	URTHexMapAsset* WorldC = RvTwin(WorldA);
	if (!TestTrue(TEXT("le gemelle esistono"), WorldB && WorldC)) { return false; }

	const FRTCellId NeverSeen(2, 0, 0);
	const FRTCellId ExploredOnly(-2, 0, 0);
	RvSmoke(WorldB, NeverSeen);
	RvSmoke(WorldB, ExploredOnly);
	RvSmoke(WorldC, FRTCellId(0, 1, 0)); // IN VISTA

	FRTTeamKnowledge Knowledge = RvAllVisibleBut(WorldA, { NeverSeen, ExploredOnly });
	Knowledge.ExploredCells.Add(ExploredOnly);

	const TArray<FRTRangeCellVerdict> InA = RvClassify(WorldA, Knowledge);
	const TArray<FRTRangeCellVerdict> InB = RvClassify(WorldB, Knowledge);
	const TArray<FRTRangeCellVerdict> InC = RvClassify(WorldC, Knowledge);

	TestTrue(TEXT("premessa: in A la cella a 3 su ciascun lato e' in portata"),
		RvFind(InA, FRTCellId(3, 0, 0)) != nullptr && RvFind(InA, FRTCellId(-3, 0, 0)) != nullptr);
	TestEqual(TEXT("un Fumo mai visto e uno esplorato e non piu' in vista non cambiano la forma ne' i verdetti"),
		RvSignature(InA), RvSignature(InB));

	// Controllo positivo 1: il Fumo restringe davvero l'insieme dell'azione armata, se lo si guarda per intero.
	const FRTTeamKnowledge Everything = RvAllVisible(WorldA);
	const TArray<FRTRangeCellVerdict> SeenInA = RvClassify(WorldA, Everything);
	const TArray<FRTRangeCellVerdict> SeenInB = RvClassify(WorldB, Everything);
	TestTrue(TEXT("premessa: guardando tutto, in A la cella a 3 c'e'"), RvFind(SeenInA, FRTCellId(3, 0, 0)) != nullptr);
	TestTrue(TEXT("premessa: guardando tutto, in B il Fumo la toglie dall'insieme"), RvFind(SeenInB, FRTCellId(3, 0, 0)) == nullptr);
	TestTrue(TEXT("premessa: e anche sul lato esplorato"), RvFind(SeenInB, FRTCellId(-3, 0, 0)) == nullptr);

	// Controllo positivo 2: un Fumo IN VISTA restringe l'insieme anche nell'uscita a tre stati.
	TestTrue(TEXT("un Fumo in vista toglie dall'insieme la cella oltre il cappello"), RvFind(InC, FRTCellId(0, 3, 0)) == nullptr);
	TestTrue(TEXT("e la stessa cella in A c'e'"), RvFind(InA, FRTCellId(0, 3, 0)) != nullptr);
	return true;
}

// Se nulla di cio' che non si vede ha un effetto, l'insieme coincide con quello dell'azione armata, cella per cella.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRangeVerdictMatchesArmedRangeTest,
	"RefactorTactics.RangeVerdict.TheSetIsTheArmedRangeWhenNothingHiddenHasAnEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRangeVerdictMatchesArmedRangeTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = RvArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	// Effetti IN VISTA (un muro e un Fumo), nessuno nel buio; il buio e' a est e a sud.
	RvWall(Map, FRTCellId(-2, 0, 0));
	RvSmoke(Map, FRTCellId(0, -1, 0));
	const FRTTeamKnowledge Knowledge = RvKnowledge(Map, [](const FRTCellId& Cell) { return Cell.X < 3 && Cell.Y < 3; });

	const TArray<FRTRangeCellVerdict> Out = RvClassify(Map, Knowledge);
	const TArray<FRTCellId> Armed = URTCombatLibrary::TargetableRangeCells(Map, RvFrom(), RvRange, ERTLineOfSightPolicy::Required);

	TestTrue(TEXT("premessa: l'insieme non e' vuoto"), Armed.Num() > 1);
	// Il Fumo IN VISTA toglie davvero qualcosa: senza, la portata libera ha piu' celle e include la cella oltre la nube.
	const TArray<FRTCellId> Unrestricted = URTCombatLibrary::TargetableRangeCells(RvArena(), RvFrom(), RvRange, ERTLineOfSightPolicy::Required);
	TestTrue(TEXT("premessa: il Fumo in vista restringe l'insieme armato"), Armed.Num() < Unrestricted.Num());
	TestTrue(TEXT("premessa: la cella oltre la nube manca dall'insieme armato, e c'e' nella portata libera"),
		!Armed.Contains(FRTCellId(0, -3, 0)) && Unrestricted.Contains(FRTCellId(0, -3, 0)));
	TestTrue(TEXT("stesso insieme dell'azione armata, nello stesso ordine"), RvCells(Out) == Armed);
	bool bSomeUnverdicted = false;
	bool bSomeVerdict = false;
	for (const FRTRangeCellVerdict& Entry : Out)
	{
		(Entry.Verdict == ERTRangeVerdict::Unverdicted ? bSomeUnverdicted : bSomeVerdict) = true;
	}
	TestTrue(TEXT("premessa: il buio produce celle senza verdetto, e il resto ha un verdetto"), bSomeUnverdicted && bSomeVerdict);
	return true;
}

// LA LETTURA PRUDENTE. Un tiro bloccato da un muro in vista resta senza verdetto se, oltre il muro, la linea attraversa una
// cella non in vista: il terzo stato copre ogni linea che attraversa celle non in vista (spec dell'ispettore, P7).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRangeVerdictStrictReadingTest,
	"RefactorTactics.RangeVerdict.AWallDoesNotGiveAVerdictOverACellOutOfSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRangeVerdictStrictReadingTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = RvArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	RvWall(Map, FRTCellId(2, 0, 0));

	const FRTTeamKnowledge Partial = RvAllVisibleBut(Map, { FRTCellId(4, 0, 0) });
	const TArray<FRTRangeCellVerdict> Out = RvClassify(Map, Partial);

	TestTrue(TEXT("linea tutta in vista, muro in mezzo: bloccata"), RvIs(Out, FRTCellId(3, 0, 0), ERTRangeVerdict::Blocked));
	TestTrue(TEXT("il bersaglio non in vista: senza verdetto"), RvIs(Out, FRTCellId(4, 0, 0), ERTRangeVerdict::Unverdicted));
	TestTrue(TEXT("oltre la cella non in vista, anche se il muro in vista basterebbe a bloccare: senza verdetto"),
		RvIs(Out, FRTCellId(5, 0, 0), ERTRangeVerdict::Unverdicted));

	// Controllo positivo: con la cella in vista, la stessa cella e' bloccata.
	TestTrue(TEXT("tutto in vista: bloccata"), RvIs(RvClassify(Map, RvAllVisible(Map)), FRTCellId(5, 0, 0), ERTRangeVerdict::Blocked));
	return true;
}

// Chi tira da una cella che la squadra non vede non ha portata, e neppure se la cella e' esplorata.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRangeVerdictUnseenSourceTest,
	"RefactorTactics.RangeVerdict.ASourceOnAnUnseenCellHasNoRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRangeVerdictUnseenSourceTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = RvArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }

	FRTTeamKnowledge Blind = RvAllVisibleBut(Map, { RvFrom() });
	// ESPLORATA ma non in vista: dice dove sta l'unita' quanto una cella mai vista (D-225, D-227).
	Blind.ExploredCells.Add(RvFrom());
	TestEqual(TEXT("cella di partenza non in vista: nessuna cella in portata"), RvClassify(Map, Blind).Num(), 0);

	TestTrue(TEXT("controllo positivo: in vista, la portata c'e'"), RvClassify(Map, RvAllVisible(Map)).Num() > 1);

	// Nessuna mappa, nessuna conoscenza: nessuna portata, e nessun crash.
	TestEqual(TEXT("senza mappa: vuota"), RvClassify(nullptr, RvAllVisible(Map)).Num(), 0);
	TestEqual(TEXT("senza celle in vista: vuota"), RvClassify(Map, FRTTeamKnowledge()).Num(), 0);
	TestEqual(TEXT("Classify senza mappa neutra: vuota"),
		URTRangeVerdictLibrary::Classify(nullptr, RvAllVisible(Map).VisibleCells, RvFrom(), RvRange, ERTLineOfSightPolicy::Required).Num(), 0);
	return true;
}

// Portata massima e minima sono quelle dell'azione, e passano dal calcolo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRangeVerdictRangeBoundsTest,
	"RefactorTactics.RangeVerdict.TheActionRangeBoundsAreTheCallers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRangeVerdictRangeBoundsTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = RvArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	const FRTTeamKnowledge Seen = RvAllVisible(Map);

	const TArray<FRTRangeCellVerdict> Short = RvClassify(Map, Seen, ERTLineOfSightPolicy::Required, /*Range=*/ 2);
	TestTrue(TEXT("portata 2: la cella a 2 c'e'"), RvFind(Short, FRTCellId(2, 0, 0)) != nullptr);
	TestTrue(TEXT("portata 2: la cella a 3 non c'e'"), RvFind(Short, FRTCellId(3, 0, 0)) == nullptr);

	const TArray<FRTRangeCellVerdict> Ring = RvClassify(Map, Seen, ERTLineOfSightPolicy::Required, /*Range=*/ 4, /*MinRange=*/ 2);
	TestTrue(TEXT("portata minima 2: la cella a 1 non c'e'"), RvFind(Ring, FRTCellId(1, 0, 0)) == nullptr);
	TestTrue(TEXT("portata minima 2: la cella di partenza non c'e'"), RvFind(Ring, RvFrom()) == nullptr);
	TestTrue(TEXT("portata minima 2: la cella a 2 c'e'"), RvFind(Ring, FRTCellId(2, 0, 0)) != nullptr);
	TestTrue(TEXT("portata 4: la cella a 5 non c'e'"), RvFind(Ring, FRTCellId(5, 0, 0)) == nullptr);
	return true;
}

// La licenza di linea dell'azione e' quella passata: un tiro che non chiede la linea non e' bloccato da un muro in vista.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRangeVerdictPolicyTest,
	"RefactorTactics.RangeVerdict.TheLineOfSightPolicyIsTheActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRangeVerdictPolicyTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = RvArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Map)) { return false; }
	RvWall(Map, FRTCellId(2, 0, 0));
	const FRTTeamKnowledge Seen = RvAllVisible(Map);

	TestTrue(TEXT("controllo positivo: con la linea richiesta il muro blocca"),
		RvIs(RvClassify(Map, Seen, ERTLineOfSightPolicy::Required), FRTCellId(3, 0, 0), ERTRangeVerdict::Blocked));
	TestTrue(TEXT("senza il requisito della linea il muro non blocca"),
		RvIs(RvClassify(Map, Seen, ERTLineOfSightPolicy::NotRequired), FRTCellId(3, 0, 0), ERTRangeVerdict::Free));
	return true;
}

// 🔴 UNA LINEA OBLIQUA SI GIUDICA SULLE SUE CELLE, E SU NESSUN'ALTRA. Tutti gli altri test mirano lungo un asse, dove
// qualunque costruzione della linea darebbe le stesse celle. Qui il bersaglio e' obliquo, e si prova in due versi: una cella
// nascosta SULLA linea toglie il verdetto (e ne' un muro ne' un Fumo li' sotto cambiano l'uscita), una cella nascosta
// ACCANTO alla linea no.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRangeVerdictObliqueLineTest,
	"RefactorTactics.RangeVerdict.AnObliqueLineIsJudgedOnItsOwnCellsAndNoOthers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRangeVerdictObliqueLineTest::RunTest(const FString&)
{
	URTHexMapAsset* Flat = RvArena();
	if (!TestNotNull(TEXT("l'arena esiste"), Flat)) { return false; }

	const FRTCellId Target(3, 1, 0);
	const TArray<FRTCellId> Line = URTHexLibrary::HexLine(RvFrom(), Target);
	if (!TestTrue(TEXT("premessa: la linea obliqua ha celle interne"), Line.Num() >= 4)) { return false; }
	const FRTCellId OnLine = Line[2];

	// Una cella accanto alla linea che non ne fa parte (la prima nell'ordine stabile).
	TArray<FRTCellId> Around = URTHexLibrary::HexArea(OnLine, 1);
	Around.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
	FRTCellId OffLine = OnLine;
	bool bFoundOff = false;
	for (const FRTCellId& Cell : Around)
	{
		if (!Line.Contains(Cell) && Flat->ContainsCell(Cell)) { OffLine = Cell; bFoundOff = true; break; }
	}
	if (!TestTrue(TEXT("premessa: esiste una cella accanto alla linea"), bFoundOff)) { return false; }

	URTHexMapAsset* WallOn = RvTwin(Flat);
	RvWall(WallOn, OnLine);
	URTHexMapAsset* SmokeOn = RvTwin(Flat);
	RvSmoke(SmokeOn, OnLine);
	URTHexMapAsset* WallOff = RvTwin(Flat);
	RvWall(WallOff, OffLine);

	const FRTTeamKnowledge HideOn = RvAllVisibleBut(Flat, { OnLine });
	const FRTTeamKnowledge HideOff = RvAllVisibleBut(Flat, { OffLine });
	const FRTTeamKnowledge Everything = RvAllVisible(Flat);

	// Nascosta SULLA linea: nessun verdetto, e un muro o un Fumo li' sotto non cambiano nulla.
	const TArray<FRTRangeCellVerdict> FlatOn = RvClassify(Flat, HideOn);
	TestTrue(TEXT("cella nascosta sulla linea: il bersaglio non ha verdetto"), RvIs(FlatOn, Target, ERTRangeVerdict::Unverdicted));
	TestEqual(TEXT("un muro sulla cella nascosta non cambia l'uscita"), RvSignature(RvClassify(WallOn, HideOn)), RvSignature(FlatOn));
	TestEqual(TEXT("un Fumo sulla cella nascosta non cambia l'uscita"), RvSignature(RvClassify(SmokeOn, HideOn)), RvSignature(FlatOn));

	// Controlli positivi: guardando tutto, il muro blocca la linea obliqua e il Fumo toglie il bersaglio dall'insieme.
	TestTrue(TEXT("premessa: guardando tutto, senza muro il bersaglio e' libero"), RvIs(RvClassify(Flat, Everything), Target, ERTRangeVerdict::Free));
	TestTrue(TEXT("premessa: guardando tutto, con il muro sulla linea e' bloccato"), RvIs(RvClassify(WallOn, Everything), Target, ERTRangeVerdict::Blocked));
	TestTrue(TEXT("premessa: guardando tutto, il Fumo sulla linea toglie il bersaglio dall'insieme"),
		RvFind(RvClassify(Flat, Everything), Target) != nullptr && RvFind(RvClassify(SmokeOn, Everything), Target) == nullptr);

	// Nascosta ACCANTO alla linea: il verdetto resta, anche con un muro nella cella nascosta.
	TestTrue(TEXT("cella nascosta accanto alla linea: il bersaglio resta libero"), RvIs(RvClassify(Flat, HideOff), Target, ERTRangeVerdict::Free));
	TestTrue(TEXT("anche con un muro sulla cella nascosta accanto"), RvIs(RvClassify(WallOff, HideOff), Target, ERTRangeVerdict::Free));
	TestTrue(TEXT("premessa: guardando tutto, il muro accanto alla linea non la blocca"),
		RvIs(RvClassify(WallOff, Everything), Target, ERTRangeVerdict::Free));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
