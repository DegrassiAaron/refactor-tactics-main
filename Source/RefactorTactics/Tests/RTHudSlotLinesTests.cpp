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
#include "Ability/RTMovementProfileLibrary.h" // gli id dei profili, mai scritti a mano

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

/**
 * `#1410` `AC-1`: l'ANDATURA si legge nella riga del movimento, e **solo quando devia dal cammino**.
 *
 * La regola di [D-425] resa in testo, con la sua asimmetria: `Move` e `Still` NON si nominano perche' sono
 * il caso normale; `Sprint` e `Sneak` aggettivano il percorso; `Withdraw` lo SOSTITUISCE, perche' il
 * ripiegamento non e' un cammino svolto in un certo modo.
 *
 * Gli id non si scrivono a mano: vengono dalla libreria, o il test si romperebbe in silenzio il giorno in
 * cui un profilo cambia nome — e resterebbe verde asserendo su una stringa che non esiste piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDMovementProfileReadsOnTheLineTest,
	"RefactorTactics.HUD.MovementProfileReadsOnTheMovementLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDMovementProfileReadsOnTheLineTest::RunTest(const FString&)
{
	using Lib = URTMovementProfileLibrary;

	auto RigaMovimento = [](FName Profilo)
	{
		FRTUnitSlotsView Slots;
		Slots.Movement = MakeSlotLineFixture(true);
		Slots.MovementProfileId = Profilo;
		return ARTHUD::ComposeSlotLines(Slots)[0].Text;
	};

	// --- il cammino non si nomina, ed e' la meta' che conta -------------------------------------------
	const FString Cammino = RigaMovimento(Lib::ProfileMove);
	TestTrue(TEXT("il Move resta «percorso»"), Cammino.Contains(TEXT("percorso")));
	TestFalse(TEXT("e non guadagna nessun aggettivo"), Cammino.Contains(TEXT(",")));
	TestEqual(TEXT("un piano senza profilo dichiarato legge come il Move"),
		RigaMovimento(NAME_None), Cammino);
	TestEqual(TEXT("e il fermo pure: non e' un'andatura diversa"),
		RigaMovimento(Lib::ProfileStill), Cammino);

	// --- le due andature che deviano ------------------------------------------------------------------
	const FString Corsa = RigaMovimento(Lib::ProfileSprint);
	TestTrue(TEXT("lo Sprint si legge «di corsa»"), Corsa.Contains(TEXT("di corsa")));
	TestTrue(TEXT("e resta un percorso"), Corsa.Contains(TEXT("percorso")));

	const FString Furtivo = RigaMovimento(Lib::ProfileSneak);
	TestTrue(TEXT("lo Sneak si legge «furtivo»"), Furtivo.Contains(TEXT("furtivo")));
	TestTrue(TEXT("e resta un percorso"), Furtivo.Contains(TEXT("percorso")));

	// ⛔ **Il ripiegamento SOSTITUISCE il percorso, non lo aggettiva.** Senza questa riga negativa la
	// funzione potrebbe concatenare come le altre due e il test resterebbe verde: «percorso, ripiegamento»
	// direbbe che l'`Overwatch` impone un modo di camminare invece di un'altra cosa.
	const FString Ripiego = RigaMovimento(Lib::ProfileWithdraw);
	TestTrue(TEXT("il Withdraw si legge «ripiegamento»"), Ripiego.Contains(TEXT("ripiegamento")));
	TestFalse(TEXT("e NON e' un percorso"), Ripiego.Contains(TEXT("percorso")));

	// --- le tre righe restano tre, e le altre due non cambiano ----------------------------------------
	FRTUnitSlotsView Piena;
	Piena.Movement = MakeSlotLineFixture(true);
	Piena.Main = MakeSlotLineFixture(true);
	Piena.Reaction = MakeSlotLineFixture(true);
	Piena.MovementProfileId = Lib::ProfileSprint;
	const TArray<FRTSlotLine> Tre = ARTHUD::ComposeSlotLines(Piena);
	if (!TestEqual(TEXT("tre slot, tre righe: l'andatura non ne aggiunge una quarta"), Tre.Num(), 3))
	{
		return false;
	}
	TestFalse(TEXT("la principale non prende l'andatura del movimento"),
		Tre[1].Text.Contains(TEXT("di corsa")));
	TestFalse(TEXT("ne' la reazione"), Tre[2].Text.Contains(TEXT("di corsa")));

	// --- il NOME dell'azione vince sull'andatura -------------------------------------------------------
	// Una mobilita' che occupa lo slot si chiama col proprio nome: l'andatura parla del movimento SENZA
	// nome, che e' il caso di cui D-425 si occupa. Senza questa riga, un'andatura che scavalcasse il nome
	// non si vedrebbe da nessun altro assert.
	FRTUnitSlotsView ConNome;
	ConNome.Movement = MakeSlotLineFixture(true, TEXT("Scatto"));
	ConNome.MovementProfileId = Lib::ProfileSprint;
	const FString RigaConNome = ARTHUD::ComposeSlotLines(ConNome)[0].Text;
	TestTrue(TEXT("l'azione che occupa lo slot si chiama col proprio nome"),
		RigaConNome.Contains(TEXT("Scatto")));
	TestFalse(TEXT("e l'andatura non lo scavalca"), RigaConNome.Contains(TEXT("di corsa")));

	// --- un id sconosciuto non finisce a schermo -------------------------------------------------------
	// ⛔ Un `FName` grezzo davanti al giocatore sarebbe un difetto di catalogo trasformato in testo. Ricade
	// sul cammino, e il posto dove quel difetto deve farsi vedere e' il test del catalogo.
	const FString Ignoto = RigaMovimento(FName(TEXT("MovementProfile.NonEsiste")));
	TestEqual(TEXT("un profilo sconosciuto legge come il cammino"), Ignoto, Cammino);
	TestFalse(TEXT("e non stampa il proprio id"), Ignoto.Contains(TEXT("NonEsiste")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
