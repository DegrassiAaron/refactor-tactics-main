// Lo sguardo (`#3747`, F2): il soggetto pubblico e il compositore delle righe.

#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "UI/RTGlance.h"
#include "UI/RTGlossary.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */

	/** I campi di `Type` con un tipo non ammesso: oggetti, o struct fuori dall'elenco. */
	TArray<FString> GlanceForbiddenFields(const UStruct* Type, const TSet<FName>& AllowedStructs, int32& Examined)
	{
		TArray<FString> Out;
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			++Examined;
			const FProperty* Element = *It;
			if (const FArrayProperty* Array = CastField<FArrayProperty>(Element)) { Element = Array->Inner; }
			if (CastField<FObjectPropertyBase>(Element) || CastField<FSoftObjectProperty>(Element))
			{
				Out.Add(FString::Printf(TEXT("%s (oggetto)"), *It->GetName()));
			}
			else if (const FStructProperty* AsStruct = CastField<FStructProperty>(Element))
			{
				if (!AllowedStructs.Contains(AsStruct->Struct->GetFName()))
				{
					Out.Add(FString::Printf(TEXT("%s (%s)"), *It->GetName(), *AsStruct->Struct->GetName()));
				}
			}
		}
		return Out;
	}

	FRTStatusBadgeView GlanceBadge(const TCHAR* Tag)
	{
		FRTStatusBadgeView Badge;
		Badge.Tag = FName(Tag);
		Badge.RemainingTurns = 1;
		return Badge;
	}

	FRTGlanceUnitSubject GlanceUnit(const TArray<const TCHAR*>& Tags, bool bFacing)
	{
		FRTGlanceUnitSubject Subject;
		Subject.DisplayName = FText::FromString(TEXT("Aevik"));
		Subject.Facing = ERTHexDirection::NE;
		Subject.bShowFacing = bFacing;
		for (const TCHAR* Tag : Tags) { Subject.Statuses.Add(GlanceBadge(Tag)); }
		return Subject;
	}

	const TArray<const TCHAR*>& GlanceAllStatuses()
	{
		static const TArray<const TCHAR*> All = {
			TEXT("Status.Root"), TEXT("Status.Slow"), TEXT("Status.Reveal"), TEXT("Status.Exposed"),
			TEXT("Status.Guarded"), TEXT("Status.Marked"), TEXT("Status.Wet"), TEXT("Status.Braced") };
		return All;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlanceViewCarriesNoRawTypesTest,
	"RefactorTactics.UI.GlanceViewCarriesNoRawTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlanceViewCarriesNoRawTypesTest::RunTest(const FString&)
{
	const TSet<FName> Allowed = { FName(TEXT("RTCellId")), FName(TEXT("RTStatusBadgeView")), FName(TEXT("RTGlanceCoverSide")) };

	int32 Examined = 0;
	const TArray<const UStruct*> Types = {
		FRTGlanceUnitSubject::StaticStruct(), FRTGlanceCellSubject::StaticStruct(),
		FRTGlanceView::StaticStruct(), FRTGlanceCoverSide::StaticStruct() };
	for (const UStruct* Type : Types)
	{
		for (const FString& Field : GlanceForbiddenFields(Type, Allowed, Examined))
		{
			AddError(FString::Printf(TEXT("`%s` porta un tipo grezzo: %s"), *Type->GetName(), *Field));
		}
	}
	TestTrue(TEXT("il controllo esamina dei campi"), Examined > 0);

	// Anti-vacuita': lo stesso controllo, su un tipo che contiene un `FGameplayTag`, deve vederlo.
	int32 Dummy = 0;
	TestTrue(TEXT("il controllo prende un FGameplayTag grezzo"),
		GlanceForbiddenFields(FGameplayTagContainer::StaticStruct(), Allowed, Dummy).Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlanceUnitLinesNameEveryStatusTest,
	"RefactorTactics.UI.GlanceUnitLinesNameEveryStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlanceUnitLinesNameEveryStatusTest::RunTest(const FString&)
{
	const FRTGlanceView View = URTGlanceLibrary::ComposeUnit(GlanceUnit({ TEXT("Status.Burning"), TEXT("Status.Wet") }, true));
	const FString All = FString::Join(View.Lines, TEXT("\n"));

	for (const TCHAR* Tag : { TEXT("Status.Burning"), TEXT("Status.Wet") })
	{
		const FString Name = RTGlossary::NameFor(FName(Tag)).ToString();
		const FString Effect = RTGlossary::EffectFor(FName(Tag)).ToString();
		TestTrue(*FString::Printf(TEXT("il nome di %s compare"), Tag), !Name.IsEmpty() && All.Contains(Name));
		TestTrue(*FString::Printf(TEXT("l'effetto di %s compare"), Tag), !Effect.IsEmpty() && All.Contains(Effect));
	}
	TestTrue(TEXT("il verso compare"), All.Contains(TEXT("NE")));
	TestFalse(TEXT("senza taglio"), View.bTruncated);
	TestTrue(TEXT("la prima riga e' il nome"), View.Lines.Num() > 0 && View.Lines[0] == TEXT("Aevik"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlanceCellLinesFollowSection4Test,
	"RefactorTactics.UI.GlanceCellLinesFollowSection4",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlanceCellLinesFollowSection4Test::RunTest(const FString&)
{
	FRTGlanceCellSubject Plain;
	Plain.Surface = ERTHexSurface::Fire;
	const FString PlainText = FString::Join(URTGlanceLibrary::ComposeCell(Plain).Lines, TEXT("\n"));
	TestTrue(TEXT("la superficie col nome del glossario"),
		PlainText.Contains(RTGlossary::NameFor(RTGlossary::KeyForSurface(ERTHexSurface::Fire)).ToString()));
	TestFalse(TEXT("costo 1: non si dice"), PlainText.Contains(TEXT("Costo")));
	TestTrue(TEXT("il piano si dice"), PlainText.Contains(TEXT("Piano 0")));
	TestFalse(TEXT("nessun segno se non ci sono"),
		PlainText.Contains(TEXT("Obiettivo")) || PlainText.Contains(TEXT("Porta")) || PlainText.Contains(TEXT("vista")));

	FRTGlanceCellSubject Full = Plain;
	Full.EntryCost = 3;
	Full.Layer = 2;
	Full.bBlocksLineOfSight = true;
	Full.bIsObjective = true;
	Full.bHasDoor = true;
	FRTGlanceCoverSide Side;
	Side.Edge = ERTHexDirection::SW;
	Side.Type = ERTHexCoverType::High;
	Full.Covers.Add(Side);
	const FString FullText = FString::Join(URTGlanceLibrary::ComposeCell(Full).Lines, TEXT("\n"));
	TestTrue(TEXT("costo diverso da 1"), FullText.Contains(TEXT("Costo d'ingresso: 3")));
	TestTrue(TEXT("piano 2"), FullText.Contains(TEXT("Piano 2")));
	TestTrue(TEXT("riparo per lato"), FullText.Contains(TEXT("SW")) &&
		FullText.Contains(RTGlossary::NameFor(RTGlossary::KeyForCover(ERTHexCoverType::High)).ToString()));
	TestTrue(TEXT("blocca la vista"), FullText.Contains(TEXT("Blocca la vista")));
	TestTrue(TEXT("obiettivo"), FullText.Contains(TEXT("Obiettivo")));
	TestTrue(TEXT("porta"), FullText.Contains(TEXT("Porta")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlanceNeverTruncatesSilentlyTest,
	"RefactorTactics.UI.GlanceNeverTruncatesSilently",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlanceNeverTruncatesSilentlyTest::RunTest(const FString&)
{
	const FRTGlanceUnitSubject Subject = GlanceUnit(GlanceAllStatuses(), true);
	const FRTGlanceView View = URTGlanceLibrary::ComposeUnit(Subject);

	TestTrue(TEXT("il taglio e' dichiarato"), View.bTruncated);
	TestTrue(TEXT("l'ultima riga dice che ce ne sono altri"),
		View.Lines.Num() > 0 && View.Lines.Last().StartsWith(TEXT("E altri ")));

	// Il primo gruppo mantiene l'ordine della targa: i primi stati del soggetto, nell'ordine dato.
	const int32 Shown = View.Lines.Num() - 3; // nome, verso, dichiarazione
	TestTrue(TEXT("almeno uno stato mostrato"), Shown >= 1);
	for (int32 i = 0; i < Shown; ++i)
	{
		const FString Name = RTGlossary::NameFor(Subject.Statuses[i].Tag).ToString();
		TestTrue(*FString::Printf(TEXT("riga %d = stato %d nell'ordine"), i + 2, i), View.Lines[i + 2].StartsWith(Name));
	}
	const int32 Hidden = Subject.Statuses.Num() - Shown;
	TestTrue(TEXT("il conteggio dei nascosti e' esatto"), View.Lines.Last().Contains(FString::FromInt(Hidden)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlanceFitsTheDeclaredLineBudgetTest,
	"RefactorTactics.UI.GlanceFitsTheDeclaredLineBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlanceFitsTheDeclaredLineBudgetTest::RunTest(const FString&)
{
	const FRTGlanceView Unit = URTGlanceLibrary::ComposeUnit(GlanceUnit(GlanceAllStatuses(), true));
	TestEqual(TEXT("il massimo dichiarato dalla spec e' quattro"), URTGlanceLibrary::MaxUnitLines, 4);
	TestTrue(TEXT("unita': entro il massimo"), Unit.Lines.Num() <= 4);
	TestEqual(TEXT("il caso peggiore riempie il massimo"), Unit.Lines.Num(), URTGlanceLibrary::MaxUnitLines);

	FRTGlanceCellSubject Cell;
	Cell.Surface = ERTHexSurface::Fire;
	Cell.EntryCost = 4;
	Cell.Layer = 1;
	Cell.bBlocksLineOfSight = true;
	Cell.bIsObjective = true;
	Cell.bHasDoor = true;
	for (int32 Edge = 0; Edge < 6; ++Edge)
	{
		FRTGlanceCoverSide Side;
		Side.Edge = static_cast<ERTHexDirection>(Edge);
		Side.Type = ERTHexCoverType::Low;
		Cell.Covers.Add(Side);
	}
	const FRTGlanceView CellView = URTGlanceLibrary::ComposeCell(Cell);
	TestTrue(TEXT("cella: entro il massimo dichiarato"), CellView.Lines.Num() <= URTGlanceLibrary::MaxCellLines);
	TestEqual(TEXT("il caso peggiore riempie il massimo"), CellView.Lines.Num(), URTGlanceLibrary::MaxCellLines);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlanceFacingIsOffInResolutionPlaybackTest,
	"RefactorTactics.UI.GlanceFacingIsOffInResolutionPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlanceFacingIsOffInResolutionPlaybackTest::RunTest(const FString&)
{
	const FRTGlanceView Off = URTGlanceLibrary::ComposeUnit(GlanceUnit({ TEXT("Status.Wet") }, false));
	TestFalse(TEXT("verso spento: nessuna riga lo dice"), FString::Join(Off.Lines, TEXT("\n")).Contains(TEXT("Guarda")));
	const FRTGlanceView On = URTGlanceLibrary::ComposeUnit(GlanceUnit({ TEXT("Status.Wet") }, true));
	TestTrue(TEXT("verso acceso: la riga c'e'"), FString::Join(On.Lines, TEXT("\n")).Contains(TEXT("Guarda")));
	return true;
}

namespace
{
	/** Un mondo di prova per il cancello: la cella C, la vista e la conoscenza della squadra osservatrice. */
	struct FGlanceWorld
	{
		FRTCellId Cell;
		FRTKnowledgeView View;
		FRTTeamKnowledge Knowledge;
		FRTGlanceUnitSubject Unit;
		FRTGlanceCellSubject CellSubject;
	};

	FGlanceWorld GlanceMakeWorld(ERTHexSurface Surface, bool bInView, bool bExplored)
	{
		FGlanceWorld W;
		W.Cell.X = 2; W.Cell.Y = 3; W.Cell.Layer = 0;
		if (bInView) { W.Knowledge.VisibleCells.Add(W.Cell); }
		if (bInView || bExplored) { W.Knowledge.ExploredCells.Add(W.Cell); }
		W.CellSubject.Cell = W.Cell;
		W.CellSubject.Surface = Surface;
		W.Unit = GlanceUnit({ TEXT("Status.Burning") }, true);
		W.Unit.DisplayName = FText::FromString(TEXT("NemicoNascosto"));
		W.Unit.Cell = W.Cell;
		return W;
	}

	FRTGlanceTarget GlanceUnitTarget(const FGlanceWorld& W, int32 Id)
	{
		return URTPointerLibrary::ResolveGlanceTarget(Id, true, W.Cell, false);
	}

	FRTKnowledgeEntry GlanceEntry(int32 Id, ERTKnowledgeVisibility Visibility)
	{
		FRTKnowledgeEntry E;
		E.StableUnitId = Id;
		E.Visibility = Visibility;
		return E;
	}

	FString GlanceText(const FRTGlanceView& V) { return FString::Join(V.Lines, TEXT("\n")); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlanceUnobservedEnemyReadsAsAnEmptyCellTest,
	"RefactorTactics.UI.GlanceUnobservedEnemyReadsAsAnEmptyCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlanceUnobservedEnemyReadsAsAnEmptyCellTest::RunTest(const FString&)
{
	for (const bool bInView : { true, false })
	{
		// Mondo A: un nemico nascosto sta sulla cella (nessuna voce nella vista). Mondo B: la cella e' vuota.
		const FGlanceWorld A = GlanceMakeWorld(ERTHexSurface::Fire, bInView, false);
		const FGlanceWorld B = GlanceMakeWorld(ERTHexSurface::Fire, bInView, false);
		const FRTGlanceView WithEnemy = URTGlanceLibrary::ResolveGlance(A.View, A.Knowledge, GlanceUnitTarget(A, 7), A.Unit, A.CellSubject);
		const FRTGlanceView Empty = URTGlanceLibrary::ResolveGlance(B.View, B.Knowledge,
			URTPointerLibrary::ResolveGlanceTarget(INDEX_NONE, true, B.Cell, false), B.Unit, B.CellSubject);

		TestEqual(*FString::Printf(TEXT("stesse righe (in vista=%d)"), bInView), GlanceText(WithEnemy), GlanceText(Empty));
		TestTrue(TEXT("stesso numero di righe"), WithEnemy.Lines.Num() == Empty.Lines.Num());
		TestFalse(TEXT("il nome nascosto non esce"), GlanceText(WithEnemy).Contains(TEXT("NemicoNascosto")));
		// Anti-vacuita': in vista le righe ci sono, fuori vista no. Senza, l'uguaglianza sarebbe banale.
		TestEqual(TEXT("in vista parla, fuori vista tace"), WithEnemy.Lines.Num() > 0, bInView);
	}

	// Controllo positivo: la stessa unita' con una voce `Live` parla, col suo nome.
	FGlanceWorld Known = GlanceMakeWorld(ERTHexSurface::Fire, true, false);
	Known.View.Entries.Add(GlanceEntry(7, ERTKnowledgeVisibility::Live));
	TestTrue(TEXT("osservata, parla"), GlanceText(URTGlanceLibrary::ResolveGlance(Known.View, Known.Knowledge,
		GlanceUnitTarget(Known, 7), Known.Unit, Known.CellSubject)).Contains(TEXT("NemicoNascosto")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlanceExploredCellReadsAsNeverSeenTest,
	"RefactorTactics.UI.GlanceExploredCellReadsAsNeverSeen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlanceExploredCellReadsAsNeverSeenTest::RunTest(const FString&)
{
	const FGlanceWorld Explored = GlanceMakeWorld(ERTHexSurface::Fire, false, true);
	const FGlanceWorld Never = GlanceMakeWorld(ERTHexSurface::Ice, false, false);
	const FRTGlanceTarget T = URTPointerLibrary::ResolveGlanceTarget(INDEX_NONE, true, Explored.Cell, false);
	const FRTGlanceView A = URTGlanceLibrary::ResolveGlance(Explored.View, Explored.Knowledge, T, Explored.Unit, Explored.CellSubject);
	const FRTGlanceView B = URTGlanceLibrary::ResolveGlance(Never.View, Never.Knowledge, T, Never.Unit, Never.CellSubject);
	TestEqual(TEXT("esplorata e mai vista: stessa uscita"), GlanceText(A), GlanceText(B));
	TestEqual(TEXT("e vuota"), A.Lines.Num(), 0);

	// Controllo positivo: in vista parla.
	const FGlanceWorld Seen = GlanceMakeWorld(ERTHexSurface::Fire, true, true);
	TestTrue(TEXT("in vista, parla"), URTGlanceLibrary::ResolveGlance(Seen.View, Seen.Knowledge, T, Seen.Unit, Seen.CellSubject).Lines.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlanceNewbornUnitSpeaksOnlyAfterTheVeilTest,
	"RefactorTactics.UI.GlanceNewbornUnitSpeaksOnlyAfterTheVeil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlanceNewbornUnitSpeaksOnlyAfterTheVeilTest::RunTest(const FString&)
{
	// L'unita' non ha voce `Live`: o non ha voce (appena nata, `bKnownToObserver` ancora vero), o ha un ricordo.
	for (const bool bRemembered : { false, true })
	{
		FGlanceWorld W = GlanceMakeWorld(ERTHexSurface::Fire, true, false);
		if (bRemembered) { W.View.Entries.Add(GlanceEntry(7, ERTKnowledgeVisibility::Remembered)); }
		const FRTGlanceView Got = URTGlanceLibrary::ResolveGlance(W.View, W.Knowledge, GlanceUnitTarget(W, 7), W.Unit, W.CellSubject);
		const FRTGlanceView CellOnly = URTGlanceLibrary::ComposeCell(W.CellSubject);
		TestEqual(*FString::Printf(TEXT("parla la cella, non l'unita' (ricordo=%d)"), bRemembered), GlanceText(Got), GlanceText(CellOnly));
		TestTrue(TEXT("e la cella ha delle righe"), CellOnly.Lines.Num() > 0);
	}
	return true;
}

#endif
