// La fotografia delle CVar del lanciatore PIE del Lab (#3541).
//
// Provano su una CVar di prova — nessun `GEditor`, nessun PIE — le due cose che il lanciatore deve
// garantire: il valore torna com'era, e la priorita' (`SetBy`) anche. La seconda e' quella che
// `Set(..., ECVF_SetByConsole)` rompe in silenzio: alza il «pavimento», e ogni `Set` successivo a priorita'
// inferiore viene ignorato.

#include "Misc/AutomationTest.h"

#include "RTLabCVarSnapshot.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabCVarSnapshotRestoresValueAndPriorityTest,
	"RefactorTactics.Lab.CVarSnapshotRestoresValueAndPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabCVarSnapshotRestoresValueAndPriorityTest::RunTest(const FString&)
{
	// Nome unico: la CVar di prova non deve poter collidere con una vera, ne' con un'altra prova.
	const TCHAR* Nome = TEXT("rt.Test.LabSnapshot.Prova");
	IConsoleVariable* Var = IConsoleManager::Get().RegisterConsoleVariable(
		Nome, TEXT("x"), TEXT("solo test"), ECVF_Default);
	if (!TestNotNull(TEXT("la CVar di prova si registra"), Var))
	{
		return false;
	}
	ON_SCOPE_EXIT { IConsoleManager::Get().UnregisterConsoleObject(Var); };

	const auto SetByDi = [Var]() { return static_cast<uint32>(Var->GetFlags() & ECVF_SetByMask); };

	// --- caso code: il valore l'ha messo il codice ---
	Var->Set(TEXT("a"), ECVF_SetByCode);
	{
		FRTLabCVarSnapshot Foto;
		FString Errore;
		if (!TestTrue(TEXT("caso code: Capture riesce"), FRTLabCVarSnapshot::Capture(Nome, Foto, Errore)))
		{
			AddError(Errore);
			return false;
		}
		TestEqual(TEXT("caso code: la fotografia tiene il valore"), Foto.ValorePrima, FString(TEXT("a")));
		TestEqual(TEXT("caso code: la fotografia tiene il SetBy"),
			static_cast<uint32>(Foto.SetByPrima), static_cast<uint32>(ECVF_SetByCode));

		TestTrue(TEXT("caso code: Apply riesce"), Foto.Apply(TEXT("b"), Errore));
		TestEqual(TEXT("caso code: Apply cambia il valore"), Var->GetString(), FString(TEXT("b")));

		TestTrue(TEXT("caso code: Restore riesce"), Foto.Restore(Errore));
		TestEqual(TEXT("caso code: Restore riporta il valore"), Var->GetString(), FString(TEXT("a")));
		TestEqual(TEXT("caso code: Restore riporta il SetBy a code"),
			SetByDi(), static_cast<uint32>(ECVF_SetByCode));
	}

	// 🔑 Il pavimento non si alza: dopo Apply + Restore, un `Set` a priorita' `Code` deve ancora PRENDERE.
	// Con `Set(..., ECVF_SetByConsole)` la variabile resterebbe a `Console` e questo `Set` verrebbe ignorato.
	Var->Set(TEXT("z"), ECVF_SetByCode);
	TestEqual(TEXT("il pavimento non si alza: un Set a priorita' code prende ancora dopo Restore"),
		Var->GetString(), FString(TEXT("z")));

	// --- caso console: il valore l'ha digitato qualcuno in console ---
	Var->Set(TEXT("c"), ECVF_SetByConsole);
	{
		FRTLabCVarSnapshot Foto;
		FString Errore;
		if (!TestTrue(TEXT("caso console: Capture riesce"), FRTLabCVarSnapshot::Capture(Nome, Foto, Errore)))
		{
			AddError(Errore);
			return false;
		}

		// Controllo positivo: scavalca davvero il valore digitato in console.
		TestTrue(TEXT("caso console: Apply riesce"), Foto.Apply(TEXT("d"), Errore));
		TestEqual(TEXT("caso console: Apply scavalca il valore digitato in console"),
			Var->GetString(), FString(TEXT("d")));

		TestTrue(TEXT("caso console: Restore riesce"), Foto.Restore(Errore));
		TestEqual(TEXT("caso console: Restore riporta il valore"), Var->GetString(), FString(TEXT("c")));
		TestEqual(TEXT("caso console: Restore riporta il SetBy a console"),
			SetByDi(), static_cast<uint32>(ECVF_SetByConsole));
	}

	// Una variabile che non esiste si rifiuta con il motivo, senza fotografia.
	{
		FRTLabCVarSnapshot Foto;
		FString Errore;
		TestFalse(TEXT("una CVar assente non si fotografa"),
			FRTLabCVarSnapshot::Capture(TEXT("rt.Test.LabSnapshot.NonEsiste"), Foto, Errore));
		TestTrue(TEXT("una CVar assente dichiara il motivo"), !Errore.IsEmpty());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
