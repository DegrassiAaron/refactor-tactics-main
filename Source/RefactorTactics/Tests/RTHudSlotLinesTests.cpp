// La terna movimento / principale / reazione, come il Canvas la scrive.
//
// `URTHudViewModel::BuildUnitSlots` decide COSA occupa uno slot, ed e' gia' testato altrove
// (`HudViewModel.SlotsDeriveFromThePlan`). Qui si verifica il passo che mancava: la composizione delle tre
// righe che `ARTHUD` disegna.
//
// Il difetto che questi test chiudono e' che la terna **non era disegnata da nessuno**. Il view model la
// calcolava, i widget UMG che avrebbero dovuto consumarla non esistono (`Content/RT/UI` non e' nel
// progetto), e il Canvas mostrava solo una riga d'intento — che salta le unita' senza piano, cioe' proprio
// quelle per cui la domanda «cosa mi resta da scegliere?» ha una risposta utile.
//
// `ComposeSlotLines` e' statica e pura come `ComputePlannedHitMarks`: prende la vista e restituisce testo,
// senza toccare selezione ne' stato del HUD.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "UI/RTHudViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	FRTPlannedSlotView MakeSlotLineFixture(bool bOccupied, const TCHAR* DisplayName = nullptr)
	{
		FRTPlannedSlotView Slot;
		Slot.bOccupied = bOccupied;
		if (DisplayName)
		{
			Slot.ActionId = FName(DisplayName);
			Slot.DisplayName = FText::FromString(DisplayName);
		}
		return Slot;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDFreeSlotsShownTest,
	"RefactorTactics.HUD.FreeSlotsAreShownNotOmitted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDFreeSlotsShownTest::RunTest(const FString&)
{
	// Un'unita' senza piano: e' il caso che la riga d'intento del Canvas SALTA, e quello in cui sapere
	// quali slot restano liberi vale di piu'. Le tre righe devono esserci comunque.
	const FRTUnitSlotsView Empty;
	const TArray<FRTSlotLine> Lines = ARTHUD::ComposeSlotLines(Empty);

	if (!TestEqual(TEXT("tre slot, tre righe"), Lines.Num(), 3)) { return false; }

	for (const FRTSlotLine& Line : Lines)
	{
		TestFalse(TEXT("slot libero, riga non marcata occupata"), Line.bOccupied);
		TestFalse(TEXT("una riga vuota non direbbe che lo slot e' libero"), Line.Text.IsEmpty());
	}

	// Ogni riga nomina il PROPRIO slot: tre righe identiche non direbbero quale e' libero.
	TestTrue(TEXT("la prima riga e' il movimento"), Lines[0].Text.Contains(TEXT("Movimento")));
	TestTrue(TEXT("la seconda e' l'azione principale"), Lines[1].Text.Contains(TEXT("Principale")));
	TestTrue(TEXT("la terza e' la reazione"), Lines[2].Text.Contains(TEXT("Reazione")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDSprintTwoSlotsTest,
	"RefactorTactics.HUD.SprintShowsTheSameNameOnBothSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDSprintTwoSlotsTest::RunTest(const FString&)
{
	// Un'azione che dichiara `MovementAndMain` e' un'azione sola su DUE slot. `BuildUnitSlots` riempie
	// entrambi con la stessa azione; la composizione non deve «pulire» il doppione, perche' mostrare il
	// movimento libero a chi ha speso il turno per una mobilita' che costa entrambi e' il difetto per cui la
	// terna esiste.
	//
	// ⚠️ Il nome del test dice `Sprint` perche' `Action.Sprint` era l'unica azione a dichiararlo: da [D-028]
	// occupa il solo movimento, e oggi nessuna azione dei cataloghi usa quella forma. Il caso resta valido -
	// la fixture e' sintetica, non legge il catalogo - e il nome si corregge quando si tocca il test.
	FRTUnitSlotsView Slots;
	Slots.Movement = MakeSlotLineFixture(true, TEXT("Scatto"));
	Slots.Main     = MakeSlotLineFixture(true, TEXT("Scatto"));

	const TArray<FRTSlotLine> Lines = ARTHUD::ComposeSlotLines(Slots);
	if (!TestEqual(TEXT("tre slot, tre righe"), Lines.Num(), 3)) { return false; }

	TestTrue(TEXT("il movimento e' occupato"), Lines[0].bOccupied);
	TestTrue(TEXT("e lo dice per nome"), Lines[0].Text.Contains(TEXT("Scatto")));
	TestTrue(TEXT("la principale e' occupata"), Lines[1].bOccupied);
	TestTrue(TEXT("e porta lo stesso nome: una sola azione, due slot"),
		Lines[1].Text.Contains(TEXT("Scatto")));

	// La reazione resta indipendente (CP 5.1): correre non impedisce di tenere una reazione pronta.
	TestFalse(TEXT("la reazione resta libera"), Lines[2].bOccupied);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDPathHasNoNameTest,
	"RefactorTactics.HUD.OccupiedMovementWithoutAnActionSaysPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDPathHasNoNameTest::RunTest(const FString&)
{
	// Un percorso tracciato a waypoint occupa il movimento e NON e' un'azione scelta: resta senza `ActionId`
	// e senza `DisplayName`. E' il caso piu' comune del gioco, e una riga che si fermasse a «Movimento:»
	// sembrerebbe un dato mancante invece di un percorso.
	FRTUnitSlotsView Slots;
	Slots.Movement = MakeSlotLineFixture(true);

	const TArray<FRTSlotLine> Lines = ARTHUD::ComposeSlotLines(Slots);
	if (!TestEqual(TEXT("tre slot, tre righe"), Lines.Num(), 3)) { return false; }

	TestTrue(TEXT("il movimento risulta occupato"), Lines[0].bOccupied);
	TestTrue(TEXT("e la riga dice cosa lo occupa"), Lines[0].Text.Contains(TEXT("percorso")));

	// ⚠️ Il ripiego del movimento e' «percorso» perche' li' e' vero. Sugli altri due slot un'occupazione
	// senza nome non e' un percorso, e chiamarla cosi' sarebbe una riga che mente.
	FRTUnitSlotsView Nameless;
	Nameless.Main = MakeSlotLineFixture(true);
	const TArray<FRTSlotLine> MainLines = ARTHUD::ComposeSlotLines(Nameless);
	TestTrue(TEXT("la principale occupata senza nome non si chiama percorso"),
		!MainLines[1].Text.Contains(TEXT("percorso")));
	TestTrue(TEXT("ma dice comunque di essere occupata"), MainLines[1].bOccupied);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
