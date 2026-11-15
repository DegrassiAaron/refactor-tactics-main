#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioPlayback.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La traduzione fra **la traccia** e **le viste d'authoring** (`#1625`).
 *
 * 🔴 **È l'unico posto in cui i due spazi di id si incontrano**, e per questo i test qui usano una mappa in
 * cui l'`int32` NON segue l'ordine di dichiarazione delle viste. Una traduzione che ignorasse la mappa e
 * associasse per indice passerebbe su una corrispondenza «ordinata» e fallirebbe qui — che è precisamente
 * ciò che si vuole, perché in partita quella corrispondenza **non è** ordinata: `EnsureMatchRoster` assegna
 * gli id dopo aver ordinato il roster per `TeamId` e `Cell`.
 */
namespace
{
	FRTScenarioUnitView Vista(const TCHAR* Id, int32 Q, int32 R,
		ERTHexDirection Facing = ERTHexDirection::E, int32 TeamId = 0)
	{
		FRTScenarioUnitView V;
		V.Id = Id;
		V.HeroId = FName(TEXT("Hero.Gadget"));
		V.TeamId = TeamId;
		V.Cell = FRTCellId(Q, R);
		V.Facing = Facing;
		return V;
	}

	FRTTracedUnitState Stato(int32 UnitId, int32 Q, int32 R,
		ERTHexDirection Facing = ERTHexDirection::E, bool bAlive = true)
	{
		FRTTracedUnitState S;
		S.UnitId = UnitId;
		S.Cell = FRTCellId(Q, R);
		S.Facing = Facing;
		S.bAlive = bAlive;
		return S;
	}

	const FRTScenarioUnitView* Trova(const TArray<FRTScenarioUnitView>& Viste, const TCHAR* Id)
	{
		for (const FRTScenarioUnitView& V : Viste) { if (V.Id == Id) { return &V; } }
		return nullptr;
	}
}

/**
 * **Le viste si spostano dove la traccia dice, e la mappa decide QUALE si sposta.**
 *
 * ⛔ **ANTI-VACUITÀ: la mappa è deliberatamente «al contrario».** `alfa` è dichiarata per prima ma ha
 * `StableUnitId` **2**; `zulu` è dichiarata seconda e ha **1**. Una traduzione per indice scambierebbe le
 * due destinazioni — e il campo mostrerebbe due unità mosse, entrambe nel posto sbagliato, **senza un
 * errore**. È il difetto che si vede solo guardando, ed è il motivo per cui questo test esiste.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPlaybackTranslatesTest,
	"RefactorTactics.Scenario.Playback.ViewsMoveWhereTheTraceSays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPlaybackTranslatesTest::RunTest(const FString&)
{
	const TArray<FRTScenarioUnitView> Viste = { Vista(TEXT("alfa"), 0, 0), Vista(TEXT("zulu"), 5, 0) };

	// 🔴 Al contrario di proposito: alfa -> 2, zulu -> 1.
	TMap<int32, FString> Mappa;
	Mappa.Add(1, TEXT("zulu"));
	Mappa.Add(2, TEXT("alfa"));

	const TArray<FRTTracedUnitState> Stati = {
		Stato(1, /*zulu ->*/ 7, 0, ERTHexDirection::W),
		Stato(2, /*alfa ->*/ 3, 0, ERTHexDirection::NE),
	};

	const TArray<FRTScenarioUnitView> Out = RTScenarioPlayback::ViewsAtTracedStates(Viste, Stati, Mappa);
	if (!TestEqual(TEXT("due viste in uscita"), Out.Num(), 2)) { return false; }

	if (const FRTScenarioUnitView* A = Trova(Out, TEXT("alfa")))
	{
		TestEqual(TEXT("alfa va dove dice lo stato con il SUO id (2), non quello del suo indice"),
			A->Cell, FRTCellId(3, 0));
		TestEqual(TEXT("e ne prende il facing"), A->Facing, ERTHexDirection::NE);
		// ⚠️ I campi d'authoring non si perdono nella conversione.
		TestEqual(TEXT("l'eroe resta quello dello scenario"), A->HeroId, FName(TEXT("Hero.Gadget")));
	}
	else { AddError(TEXT("alfa manca dall'uscita")); }

	if (const FRTScenarioUnitView* Z = Trova(Out, TEXT("zulu")))
	{
		TestEqual(TEXT("e zulu dove dice l'id 1"), Z->Cell, FRTCellId(7, 0));
		TestEqual(TEXT("col suo facing"), Z->Facing, ERTHexDirection::W);
	}
	else { AddError(TEXT("zulu manca dall'uscita")); }

	// La premessa dell'anti-vacuità, esplicita: se la mappa fosse in ordine, il test non distinguerebbe
	// una traduzione per id da una per indice.
	TestNotEqual(TEXT("la mappa NON segue l'ordine di dichiarazione: il test può distinguere"),
		*Mappa.Find(1), FString(TEXT("alfa")));

	return true;
}

/**
 * **Chi la traccia non nomina resta dov'è; chi è caduto esce; chi non è traducibile non si muove.**
 *
 * 🔴 L'ultima è la parte fail-closed, e non è teorica: prima del primo `Run` la mappa è **vuota**, e una
 * traduzione che «facesse del suo meglio» sposterebbe i marcatori su posizioni prese da una corsa che non
 * è mai avvenuta. Muovere quello sbagliato è peggio che non muoverne nessuno, perché il primo si vede e
 * sembra vero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPlaybackEdgesTest,
	"RefactorTactics.Scenario.Playback.UntracedStaysAndFallenLeaves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPlaybackEdgesTest::RunTest(const FString&)
{
	const TArray<FRTScenarioUnitView> Viste = {
		Vista(TEXT("mossa"), 0, 0),
		Vista(TEXT("ferma"), 4, 0),
		Vista(TEXT("caduta"), 8, 0),
	};

	TMap<int32, FString> Mappa;
	Mappa.Add(1, TEXT("mossa"));
	Mappa.Add(3, TEXT("caduta"));
	// ⚠️ `ferma` NON è nella mappa e non è negli stati: è il caso «la traccia non la nomina».

	const TArray<FRTTracedUnitState> Stati = {
		Stato(1, 2, 0),
		Stato(3, 8, 0, ERTHexDirection::E, /*bAlive=*/ false),
	};

	const TArray<FRTScenarioUnitView> Out = RTScenarioPlayback::ViewsAtTracedStates(Viste, Stati, Mappa);

	TestEqual(TEXT("due viste: la caduta è uscita"), Out.Num(), 2);
	TestNull(TEXT("chi è stato abbattuto non si disegna"), Trova(Out, TEXT("caduta")));

	if (const FRTScenarioUnitView* M = Trova(Out, TEXT("mossa")))
	{
		TestEqual(TEXT("chi si è mosso è dove dice la traccia"), M->Cell, FRTCellId(2, 0));
	}
	if (const FRTScenarioUnitView* F = Trova(Out, TEXT("ferma")))
	{
		TestEqual(TEXT("chi la traccia non nomina resta alla posa di partenza"), F->Cell, FRTCellId(4, 0));
	}
	else { AddError(TEXT("l'unità non tracciata è sparita: la posa di partenza era già la risposta")); }

	// --- FAIL-CLOSED: mappa vuota => nessuno si muove -------------------------------------------------
	const TArray<FRTScenarioUnitView> SenzaMappa =
		RTScenarioPlayback::ViewsAtTracedStates(Viste, Stati, TMap<int32, FString>());
	TestEqual(TEXT("senza traduzione restano tutte e tre"), SenzaMappa.Num(), 3);
	if (const FRTScenarioUnitView* M = Trova(SenzaMappa, TEXT("mossa")))
	{
		TestEqual(TEXT("e nessuna si sposta: si preferisce fermo a sbagliato"), M->Cell, FRTCellId(0, 0));
	}

	return true;
}

/**
 * **Il verso opposto: dalle viste allo stato iniziale che la ricostruzione richiede.**
 *
 * ⚠️ La traccia dichiara i **cambiamenti**, non le posizioni di partenza: senza questo passo
 * `UnitsAtPosition` non avrebbe niente da muovere. E chi non ha un `StableUnitId` resta fuori — non è una
 * perdita, è che non comparirà mai nella traccia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioPlaybackInitialTest,
	"RefactorTactics.Scenario.Playback.InitialStatesCarryTheAuthoredPose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioPlaybackInitialTest::RunTest(const FString&)
{
	const TArray<FRTScenarioUnitView> Viste = {
		Vista(TEXT("alfa"), 1, 2, ERTHexDirection::SW),
		Vista(TEXT("senzaid"), 9, 9),
	};

	TMap<int32, FString> Mappa;
	Mappa.Add(5, TEXT("alfa"));

	const TArray<FRTTracedUnitState> Stati = RTScenarioPlayback::InitialStatesFromViews(Viste, Mappa);

	if (!TestEqual(TEXT("solo l'unità con un id entra"), Stati.Num(), 1)) { return false; }
	TestEqual(TEXT("e porta il SUO StableUnitId, non un indice"), Stati[0].UnitId, 5);
	TestEqual(TEXT("con la cella d'authoring"), Stati[0].Cell, FRTCellId(1, 2));
	TestEqual(TEXT("e il facing d'authoring"), Stati[0].Facing, ERTHexDirection::SW);
	TestTrue(TEXT("viva: la traccia dirà chi cade"), Stati[0].bAlive);

	// ⛔ Il giro completo: viste -> stati iniziali -> viste, senza traccia, deve tornare al punto di
	// partenza. È l'identità che rende sicuro chiamarli in sequenza, e senza di essa un difetto di
	// traduzione si vedrebbe solo dopo il primo movimento.
	const TArray<FRTScenarioUnitView> Ritorno =
		RTScenarioPlayback::ViewsAtTracedStates(Viste, Stati, Mappa);
	if (const FRTScenarioUnitView* A = Trova(Ritorno, TEXT("alfa")))
	{
		TestEqual(TEXT("andata e ritorno non spostano nulla"), A->Cell, FRTCellId(1, 2));
		TestEqual(TEXT("né riorientano"), A->Facing, ERTHexDirection::SW);
	}
	else { AddError(TEXT("alfa persa nel giro di andata e ritorno")); }

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
