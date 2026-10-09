#include "Misc/AutomationTest.h"

#include "RTHexEditorClick.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * LA VALIDAZIONE ASPETTA UN TICK DI QUIETE (#1864, casella 8).
 *
 * 🔴 **Il difetto che questa guardia esiste per non introdurre, misurato leggendo il costo.**
 * `URTHexMapAsset::ValidateMap()` chiama in coda `ValidateMapDetailed`, che per OGNI cella esegue
 * `ComputeMask`, `HasLegalPlacement` ed `EnumerateCoverOptions` — cioe' il lavoro geometrico della
 * cottura dell'intera mappa. `URTHexEditorMode::ModeTick` gira una volta per fotogramma, e la guardia su
 * `Revision` che il readout usa da `#1186` non basta: durante un trascinamento del **pennello** la mappa
 * cambia a *ogni* fotogramma, quindi quella guardia lascia passare tutto.
 *
 * ⚠️ **Non e' un test sul tempo, ed e' il punto**: la guardia conta i tick, non i millisecondi. Un
 * debounce temporale renderebbe il numero di validazioni dipendente dal frame rate — cioe' dalla
 * macchina — e un test non potrebbe pinnarlo senza misurare tempi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexValidationGateWaitsForQuietTest,
	"RefactorTactics.Editor.Validation.ItWaitsForOneQuietTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexValidationGateWaitsForQuietTest::RunTest(const FString&)
{
	int32 LastSeen = INDEX_NONE;
	bool bPending = false;

	// Primo giro su una mappa mai vista: la revisione «cambia» (da INDEX_NONE a 7) e si aspetta.
	TestFalse(TEXT("al primo avvistamento non si valida subito"),
		RTHexEditor::ShouldRevalidate(7, LastSeen, bPending));

	// Il fotogramma dopo la mappa e' ferma: ora si valida, e UNA volta sola.
	TestTrue(TEXT("dopo un tick di quiete si valida"),
		RTHexEditor::ShouldRevalidate(7, LastSeen, bPending));
	TestFalse(TEXT("e non si ripete finche' la mappa non cambia"),
		RTHexEditor::ShouldRevalidate(7, LastSeen, bPending));
	TestFalse(TEXT("ne' al terzo tick fermo"),
		RTHexEditor::ShouldRevalidate(7, LastSeen, bPending));

	// 🔑 IL CASO CHE CONTA: una pennellata muove la revisione a ogni fotogramma. Zero validazioni
	// mentre si trascina, e UNA al rilascio — altrimenti `ValidateMap` girerebbe su tutta la mappa
	// sessanta volte al secondo.
	int32 Validazioni = 0;
	for (int32 Rev = 8; Rev <= 27; ++Rev) // venti fotogrammi di trascinamento
	{
		if (RTHexEditor::ShouldRevalidate(Rev, LastSeen, bPending)) { ++Validazioni; }
	}
	TestEqual(TEXT("durante il trascinamento non si valida MAI"), Validazioni, 0);

	// Il dito si alza: la revisione non si muove piu'.
	TestTrue(TEXT("al primo fotogramma di quiete si valida"),
		RTHexEditor::ShouldRevalidate(27, LastSeen, bPending));

	int32 Dopo = 0;
	for (int32 I = 0; I < 30; ++I)
	{
		if (RTHexEditor::ShouldRevalidate(27, LastSeen, bPending)) { ++Dopo; }
	}
	TestEqual(TEXT("e poi non si valida piu', per trenta fotogrammi fermi"), Dopo, 0);

	return true;
}

/**
 * UN CAMBIAMENTO DURANTE L'ATTESA non perde la validazione: la rimanda.
 *
 * ⚠️ **E' il caso che distingue «rimanda» da «annulla»**, e una guardia scritta con un solo flag
 * booleano lo sbaglia facilmente. Se la mappa cambia, si ferma per un tick, e poi cambia di nuovo prima
 * che il tick di quiete arrivi, la validazione deve comunque avvenire **dopo** l'ultimo cambiamento —
 * mai essere scartata perche' «ce n'era gia' una in attesa».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexValidationGateNeverLosesTheWorkTest,
	"RefactorTactics.Editor.Validation.AChangeDuringTheWaitPostponesItInsteadOfDroppingIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexValidationGateNeverLosesTheWorkTest::RunTest(const FString&)
{
	int32 LastSeen = 3;
	bool bPending = false;

	RTHexEditor::ShouldRevalidate(4, LastSeen, bPending); // cambia: in attesa
	TestTrue(TEXT("c'e' lavoro in attesa"), bPending);

	RTHexEditor::ShouldRevalidate(5, LastSeen, bPending); // cambia di nuovo: ancora in attesa
	TestTrue(TEXT("il secondo cambiamento non ha cancellato l'attesa"), bPending);

	TestTrue(TEXT("e la validazione avviene dopo l'ULTIMO cambiamento"),
		RTHexEditor::ShouldRevalidate(5, LastSeen, bPending));
	TestFalse(TEXT("una volta sola"),
		RTHexEditor::ShouldRevalidate(5, LastSeen, bPending));

	// CONTROPROVA: la revisione registrata e' quella dell'ultimo cambiamento, non del primo. Senza,
	// tornare al valore precedente sembrerebbe «nessun cambiamento».
	TestEqual(TEXT("lo stato ricorda l'ultima revisione vista"), LastSeen, 5);
	TestTrue(TEXT("e tornare indietro conta come un cambiamento"),
		!RTHexEditor::ShouldRevalidate(4, LastSeen, bPending) && bPending);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
