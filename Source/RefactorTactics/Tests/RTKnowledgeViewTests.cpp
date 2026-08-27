#include "Misc/AutomationTest.h"
#include "Map/RTCellId.h"
#include "Perception/RTKnowledgeView.h"
#include "Perception/RTTeamKnowledge.h"
#include "UI/RTHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 13.5 — la porta fra lo stato autorevole e la presentazione.
 *
 * Il test che conta e' `ViewIsIndependentOfHiddenState`: e' il gemello umano di
 * `HexBotPlay.HiddenEnemyFairness`, ed e' il debito che D-143 assegna al primo consumatore che introduca un
 * overlay di conoscenza. Questa fase e' quel consumatore.
 */

namespace
{
	/** Nome distinto per file: la unity build condivide la translation unit. */
	FRTKnowledgeSubject KvSubject(int32 StableId, int32 TeamId, const FRTCellId& Cell)
	{
		FRTKnowledgeSubject S;
		S.StableUnitId = StableId;
		S.TeamId = TeamId;
		S.Cell = Cell;
		S.HeroId = FName(*FString::Printf(TEXT("Hero%d"), StableId));
		S.HeroDisplayName = FText::FromString(FString::Printf(TEXT("Eroe %d"), StableId));
		S.bAlive = true;
		return S;
	}

	/** Conoscenza della squadra 0 al turno 5, con i contatti passati esplicitamente. */
	FRTTeamKnowledge KvKnowledge(const TArray<FRTLastKnownContact>& Contacts)
	{
		FRTTeamKnowledge K;
		K.TeamId = 0;
		K.TurnNumber = 5;
		K.Contacts = Contacts;
		return K;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeViewOmitsHiddenTest,
	"RefactorTactics.Knowledge.ViewOmitsHidden",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeViewOmitsHiddenTest::RunTest(const FString&)
{
	// Squadra 0: un alleato (id 1) e due avversari — uno visto ora (id 2), uno ignoto (id 3).
	const FRTCellId AllyCell(0, 0, 0);
	const FRTCellId SeenCell(3, 0, 0);
	const FRTCellId HiddenCell(7, 0, 0);

	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> Subjects = {
		KvSubject(1, 0, AllyCell),
		KvSubject(2, 1, SeenCell),
		KvSubject(3, 1, HiddenCell)
	};

	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, /*ObserverTeamId*/ 0);

	TestEqual(TEXT("l'alleato e il nemico visto: due voci, non tre"), View.Entries.Num(), 2);
	TestNotNull(TEXT("l'alleato c'e'"), URTKnowledgeViewLibrary::FindEntry(View, 1));
	TestNotNull(TEXT("il nemico visto c'e'"), URTKnowledgeViewLibrary::FindEntry(View, 2));

	// 🔴 Il cuore: NESSUNA voce, non una voce con un flag.
	TestNull(TEXT("l'ignoto non ha una voce"), URTKnowledgeViewLibrary::FindEntry(View, 3));

	// E la sua cella non compare da nessuna parte nella vista.
	for (const FRTKnowledgeEntry& E : View.Entries)
	{
		TestFalse(TEXT("la cella dell'ignoto non trapela"), E.Cell == HiddenCell);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeViewIsIndependentOfHiddenStateTest,
	"RefactorTactics.Knowledge.ViewIsIndependentOfHiddenState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeViewIsIndependentOfHiddenStateTest::RunTest(const FString&)
{
	// Lo stesso osservatore, due stati autoritativi DIVERSI: il nemico ignoto sta in A oppure in B.
	// Se la vista differisce, l'informazione e' passata.
	const FRTCellId SeenCell(3, 0, 0);
	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> WorldA = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),
		KvSubject(2, 1, SeenCell),
		KvSubject(3, 1, FRTCellId(7, 0, 0))
	};
	const TArray<FRTKnowledgeSubject> WorldB = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),
		KvSubject(2, 1, SeenCell),
		KvSubject(3, 1, FRTCellId(-7, 2, 0)) // stesso ignoto, dall'altra parte della mappa
	};

	const FRTKnowledgeView A = URTKnowledgeViewLibrary::ViewForTeam(K, WorldA, 0);
	const FRTKnowledgeView B = URTKnowledgeViewLibrary::ViewForTeam(K, WorldB, 0);

	TestEqual(TEXT("stesso numero di voci"), A.Entries.Num(), B.Entries.Num());
	for (int32 i = 0; i < A.Entries.Num() && i < B.Entries.Num(); ++i)
	{
		TestEqual(TEXT("stessa identita'"), A.Entries[i].StableUnitId, B.Entries[i].StableUnitId);
		TestTrue(TEXT("stessa cella"), A.Entries[i].Cell == B.Entries[i].Cell);
		TestTrue(TEXT("stessa visibilita'"), A.Entries[i].Visibility == B.Entries[i].Visibility);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeLastContactCarriesIdentityNotConditionTest,
	"RefactorTactics.Knowledge.LastContactCarriesIdentityNotCondition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeLastContactCarriesIdentityNotConditionTest::RunTest(const FString&)
{
	// Il nemico 2 e' stato visto in (3,0), poi si e' spostato in (6,0) senza essere visto.
	const FRTCellId Remembered(3, 0, 0);
	const FRTCellId Actual(6, 0, 0);

	const FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, Remembered, 5) });
	// `VisibleCells` VUOTA: nessuno lo vede ora. Resta solo il ricordo.

	const TArray<FRTKnowledgeSubject> Subjects = { KvSubject(2, 1, Actual) };
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, 0);

	const FRTKnowledgeEntry* E = URTKnowledgeViewLibrary::FindEntry(View, 2);
	if (!TestNotNull(TEXT("il ricordo produce una voce"), E))
	{
		return false;
	}
	TestTrue(TEXT("e' un ricordo, non un contatto vivo"), E->Visibility == ERTKnowledgeVisibility::Remembered);
	TestTrue(TEXT("porta la cella del CONTATTO"), E->Cell == Remembered);
	TestFalse(TEXT("e NON quella attuale"), E->Cell == Actual);
	TestFalse(TEXT("l'identita' c'e'"), E->HeroDisplayName.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTKnowledgeHudDrawsOnlyKnownUnitsTest,
	"RefactorTactics.Knowledge.HudDrawsOnlyKnownUnits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTKnowledgeHudDrawsOnlyKnownUnitsTest::RunTest(const FString&)
{
	const FRTCellId SeenCell(3, 0, 0);
	FRTTeamKnowledge K = KvKnowledge({ FRTLastKnownContact(2, SeenCell, 5) });
	K.VisibleCells.Add(SeenCell);

	const TArray<FRTKnowledgeSubject> Subjects = {
		KvSubject(1, 0, FRTCellId(0, 0, 0)),  // alleato
		KvSubject(2, 1, SeenCell),            // nemico visto
		KvSubject(3, 1, FRTCellId(7, 0, 0))   // nemico ignoto
	};
	const FRTKnowledgeView View = URTKnowledgeViewLibrary::ViewForTeam(K, Subjects, 0);

	TestTrue(TEXT("l'alleato si disegna"), ARTHUD::ShouldDrawUnitOverlay(View, 1, /*bIsOwnTeam*/ true));
	TestTrue(TEXT("il nemico visto si disegna"), ARTHUD::ShouldDrawUnitOverlay(View, 2, false));
	TestFalse(TEXT("il nemico ignoto NON si disegna"), ARTHUD::ShouldDrawUnitOverlay(View, 3, false));

	// Anti-vacuita': un'unita' della propria squadra si disegna anche se, per un difetto della porta, non
	// avesse una voce. Il proprio schieramento non si nasconde mai a se stessi.
	TestTrue(TEXT("la propria squadra non si nasconde mai"),
		ARTHUD::ShouldDrawUnitOverlay(View, 99, /*bIsOwnTeam*/ true));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
