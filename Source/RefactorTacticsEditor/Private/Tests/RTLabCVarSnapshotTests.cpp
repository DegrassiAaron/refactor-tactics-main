// La fotografia delle CVar del lanciatore PIE del Lab (#3541).
//
// Provano su CVar di prova — nessun `GEditor`, nessun PIE — le due cose che il lanciatore deve garantire: il
// valore torna com'era, e la priorita' (`SetBy`) anche — da `Code` in su; sotto `Code` torna `Code`, il
// limite dichiarato nell'header, e i casi `constructor` e `commandline` lo asseriscono. Il caso piu'
// importante e' `Constructor`: e' lo stato di `rt.Test.Scenario` in un Editor appena aperto, e un test che
// imposta la variabile PRIMA di fotografarla non lo incontra mai (e' cosi' che `SetWithCurrentPriority`
// sembrava funzionare: su `Constructor` risolve a `SETBY_ERROR` e viene rifiutato).
//
// Ogni blocco ha la SUA CVar: una priorita' alzata (`Console`, `Commandline`) non si abbassa, e un blocco
// non deve ereditare la storia del precedente.

#include "Misc/AutomationTest.h"

#include "RTLabCVarSnapshot.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS

// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
namespace RTLabCVarSnapshotTestsInternal
{
	/** `SETBY_ERROR` (`ConsoleManager.cpp:804`): cio' che `SetWithCurrentPriority` risolve su una CVar mai impostata. */
	constexpr uint32 SetByErrore = 0x01000000;

	/**
	 * Una CVar di prova che vive quanto lo scope. `bKeepState = false`: con il default `true` la storia delle
	 * priorita' resta nel manager, e un secondo run nello stesso processo troverebbe la voce stantia
	 * (`ConsoleManager.cpp:3375-3385`).
	 */
	struct FProva
	{
		explicit FProva(const TCHAR* InNome)
			: Nome(InNome)
			, Var(IConsoleManager::Get().RegisterConsoleVariable(InNome, TEXT("x"), TEXT("solo test"), ECVF_Default))
		{
		}
		~FProva()
		{
			if (Var)
			{
				IConsoleManager::Get().UnregisterConsoleObject(Var, /*bKeepState=*/ false);
			}
		}
		FProva(const FProva&) = delete;
		FProva& operator=(const FProva&) = delete;

		uint32 SetBy() const { return static_cast<uint32>(Var->GetFlags() & ECVF_SetByMask); }

		const TCHAR* Nome;
		IConsoleVariable* Var;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLabCVarSnapshotRestoresValueAndPriorityTest,
	"RefactorTactics.Lab.CVarSnapshotRestoresValueAndPriority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLabCVarSnapshotRestoresValueAndPriorityTest::RunTest(const FString&)
{
	using namespace RTLabCVarSnapshotTestsInternal;

	// --- caso constructor: la CVar non e' MAI stata impostata (PRIMA di ogni `Set`) ---
	{
		FProva Prova(TEXT("rt.Test.LabSnapshot.Costruttore"));
		if (!TestNotNull(TEXT("caso constructor: la CVar di prova si registra"), Prova.Var)) { return false; }

		FRTLabCVarSnapshot Foto;
		FString Errore;
		if (!TestTrue(TEXT("caso constructor: Capture riesce"), FRTLabCVarSnapshot::Capture(Prova.Nome, Foto, Errore)))
		{
			AddError(Errore);
			return false;
		}
		TestEqual(TEXT("caso constructor: la fotografia tiene il SetBy Constructor"),
			static_cast<uint32>(Foto.SetByPrima), static_cast<uint32>(ECVF_SetByConstructor));

		TestTrue(TEXT("caso constructor: Apply riesce"), Foto.Apply(TEXT("b"), Errore));
		TestEqual(TEXT("caso constructor: Apply cambia il valore"), Prova.Var->GetString(), FString(TEXT("b")));
		TestNotEqual(TEXT("caso constructor: Apply non risolve a SETBY_ERROR"), Prova.SetBy(), SetByErrore);

		TestTrue(TEXT("caso constructor: Restore riesce"), Foto.Restore(Errore));
		TestEqual(TEXT("caso constructor: Restore riporta il valore iniziale"), Prova.Var->GetString(), FString(TEXT("x")));
		// Il limite dichiarato nell'header, asserito: non si torna a `Constructor`, si resta a `Code`.
		TestEqual(TEXT("caso constructor: SetBy dopo Restore e' Code"), Prova.SetBy(), static_cast<uint32>(ECVF_SetByCode));
	}

	// --- caso code: il valore l'ha messo il codice ---
	{
		FProva Prova(TEXT("rt.Test.LabSnapshot.Codice"));
		if (!TestNotNull(TEXT("caso code: la CVar di prova si registra"), Prova.Var)) { return false; }
		Prova.Var->Set(TEXT("a"), ECVF_SetByCode);

		FRTLabCVarSnapshot Foto;
		FString Errore;
		if (!TestTrue(TEXT("caso code: Capture riesce"), FRTLabCVarSnapshot::Capture(Prova.Nome, Foto, Errore)))
		{
			AddError(Errore);
			return false;
		}
		TestEqual(TEXT("caso code: la fotografia tiene il valore"), Foto.ValorePrima, FString(TEXT("a")));
		TestEqual(TEXT("caso code: la fotografia tiene il SetBy"),
			static_cast<uint32>(Foto.SetByPrima), static_cast<uint32>(ECVF_SetByCode));

		TestTrue(TEXT("caso code: Apply riesce"), Foto.Apply(TEXT("b"), Errore));
		TestEqual(TEXT("caso code: Apply cambia il valore"), Prova.Var->GetString(), FString(TEXT("b")));

		TestTrue(TEXT("caso code: Restore riesce"), Foto.Restore(Errore));
		TestEqual(TEXT("caso code: Restore riporta il valore"), Prova.Var->GetString(), FString(TEXT("a")));
		TestEqual(TEXT("caso code: Restore riporta il SetBy a code"), Prova.SetBy(), static_cast<uint32>(ECVF_SetByCode));

		// 🔑 Il pavimento non si alza: dopo Apply + Restore, un `Set` a priorita' `Code` deve ancora PRENDERE.
		// Con `Set(..., ECVF_SetByConsole)` la variabile resterebbe a `Console` e questo `Set` verrebbe ignorato.
		Prova.Var->Set(TEXT("z"), ECVF_SetByCode);
		TestEqual(TEXT("il pavimento non si alza: un Set a priorita' code prende ancora dopo Restore"),
			Prova.Var->GetString(), FString(TEXT("z")));
	}

	// --- caso console: il valore l'ha digitato qualcuno in console ---
	{
		FProva Prova(TEXT("rt.Test.LabSnapshot.Console"));
		if (!TestNotNull(TEXT("caso console: la CVar di prova si registra"), Prova.Var)) { return false; }
		Prova.Var->Set(TEXT("c"), ECVF_SetByConsole);

		FRTLabCVarSnapshot Foto;
		FString Errore;
		if (!TestTrue(TEXT("caso console: Capture riesce"), FRTLabCVarSnapshot::Capture(Prova.Nome, Foto, Errore)))
		{
			AddError(Errore);
			return false;
		}

		// Controllo positivo: scavalca davvero il valore digitato in console (priorita' uguale).
		TestTrue(TEXT("caso console: Apply riesce"), Foto.Apply(TEXT("d"), Errore));
		TestEqual(TEXT("caso console: Apply scavalca il valore digitato in console"),
			Prova.Var->GetString(), FString(TEXT("d")));

		TestTrue(TEXT("caso console: Restore riesce"), Foto.Restore(Errore));
		TestEqual(TEXT("caso console: Restore riporta il valore"), Prova.Var->GetString(), FString(TEXT("c")));
		TestEqual(TEXT("caso console: Restore riporta il SetBy a console"),
			Prova.SetBy(), static_cast<uint32>(ECVF_SetByConsole));
	}

	// --- caso commandline: `-dpcvars=`. ⚠️ `Commandline` (0x0D) sta SOTTO `Code` (0x0E): ricade nel limite
	// dichiarato dell'header — il valore torna, il `SetBy` no (diventa `Code`). Si asserisce il fatto vero. ---
	{
		FProva Prova(TEXT("rt.Test.LabSnapshot.RigaDiComando"));
		if (!TestNotNull(TEXT("caso commandline: la CVar di prova si registra"), Prova.Var)) { return false; }
		Prova.Var->Set(TEXT("k"), ECVF_SetByCommandline);

		FRTLabCVarSnapshot Foto;
		FString Errore;
		if (!TestTrue(TEXT("caso commandline: Capture riesce"), FRTLabCVarSnapshot::Capture(Prova.Nome, Foto, Errore)))
		{
			AddError(Errore);
			return false;
		}

		TestEqual(TEXT("caso commandline: la fotografia tiene il SetBy Commandline"),
			static_cast<uint32>(Foto.SetByPrima), static_cast<uint32>(ECVF_SetByCommandline));

		// Controllo positivo: scavalca il valore della riga di comando.
		TestTrue(TEXT("caso commandline: Apply riesce"), Foto.Apply(TEXT("m"), Errore));
		TestEqual(TEXT("caso commandline: Apply cambia il valore"), Prova.Var->GetString(), FString(TEXT("m")));

		TestTrue(TEXT("caso commandline: Restore riesce"), Foto.Restore(Errore));
		TestEqual(TEXT("caso commandline: Restore riporta il valore"), Prova.Var->GetString(), FString(TEXT("k")));
		TestEqual(TEXT("caso commandline: SetBy dopo Restore e' Code (limite dichiarato, sotto Code non si torna)"),
			Prova.SetBy(), static_cast<uint32>(ECVF_SetByCode));
	}

	// --- alzata durante: una riga digitata in console MENTRE il banco e' applicato ---
	{
		FProva Prova(TEXT("rt.Test.LabSnapshot.Alzata"));
		if (!TestNotNull(TEXT("alzata durante: la CVar di prova si registra"), Prova.Var)) { return false; }
		Prova.Var->Set(TEXT("a"), ECVF_SetByCode);

		FRTLabCVarSnapshot Foto;
		FString Errore;
		if (!TestTrue(TEXT("alzata durante: Capture riesce"), FRTLabCVarSnapshot::Capture(Prova.Nome, Foto, Errore)))
		{
			AddError(Errore);
			return false;
		}
		TestTrue(TEXT("alzata durante: Apply riesce"), Foto.Apply(TEXT("b"), Errore));

		// Simula la riga digitata durante il PIE: da qui `Restore` (a priorita' `Code`) non puo' piu' scrivere.
		Prova.Var->Set(TEXT("q"), ECVF_SetByConsole);

		FString ErroreRestore;
		TestFalse(TEXT("alzata durante: Restore non riesce"), Foto.Restore(ErroreRestore));
		TestEqual(TEXT("alzata durante: la variabile resta com'e' stata alzata"),
			Prova.Var->GetString(), FString(TEXT("q")));
		TestTrue(TEXT("alzata durante: l'errore nomina la priorita' superiore"),
			ErroreRestore.Contains(TEXT("priorita' superiore")));
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
