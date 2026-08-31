#include "Misc/AutomationTest.h"

#include "RTLauncherWorkspace.h"
#include "ScenarioHarness/RTScenarioDraft.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * `Start Session` e il registro delle superfici (#1682, slice `L6`).
 *
 * ⛔ **Cosa questi test NON coprono.** Che il pulsante compaia, che i pulsanti delle superfici attivino il
 * mode o invochino il tab, e che il messaggio di rifiuto si legga accanto all'elenco sono Slate su un
 * editor vivo: nessun automation test li vede, e sono voce di seduta (`editor-sessions.yaml`). Qui c'e'
 * cio' che decide *se* la sessione parte, *perche'* non parte, e *quali* superfici il workspace ha il
 * diritto di dichiarare — che e' dove la slice puo' sbagliare in silenzio.
 */

/**
 * L'AC principale: uno scenario che la validazione canonica respinge non avvia la sessione, e lo dice.
 *
 * ⚠️ **L'oracolo e' triplo, e serve che lo sia.** Solo `!bAllowed` lo soddisfa anche un'implementazione che
 * rifiuta tutto; solo la causa lo soddisfa una che non dice niente all'utente. La frase non vuota e' la
 * meta' dell'AC che si perde piu' facilmente, perche' non si vede in un rifiuto che funziona.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherInvalidScenarioCannotStartSessionTest,
	"RefactorTactics.DevSandboxLauncher.InvalidScenarioCannotStartSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherInvalidScenarioCannotStartSessionTest::RunTest(const FString&)
{
	const FRTLauncherStartDecision Decision =
		FRTLauncherWorkspace::DecideStart(TEXT("Combat.BasicAttack"), ERTScenarioAuthoringResult::Invalid, FString());

	TestFalse(TEXT("uno scenario invalido non avvia la sessione"), Decision.bAllowed);
	TestTrue(TEXT("la causa e' `Invalid`, non un rifiuto generico"), Decision.Refusal == ERTLauncherStartRefusal::Invalid);
	TestFalse(TEXT("il motivo e' visibile: un rifiuto muto soddisfa meta' dell'AC"), Decision.Reason.IsEmpty());

	// La frase della facade prevale su quella di ripiego: nomina il campo, e la nostra no.
	const FRTLauncherStartDecision WithFacadeReason =
		FRTLauncherWorkspace::DecideStart(TEXT("Combat.BasicAttack"), ERTScenarioAuthoringResult::Invalid, TEXT("units[0].cell fuori dalla mappa"));
	TestEqual(TEXT("quando la facade spiega, e' la sua frase a comparire"), WithFacadeReason.Reason, FString(TEXT("units[0].cell fuori dalla mappa")));

	return true;
}

/**
 * Le cause restano distinte, e la selezione mancante viene PRIMA dell'esito.
 *
 * ⚠️ Senza l'ultima asserzione un'implementazione che legge `OpenResult` per prima classificherebbe
 * «nessuno scenario scelto» come «scenario invalido», mandando a correggere un file che nessuno ha aperto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherStartRefusalNamesItsCauseTest,
	"RefactorTactics.DevSandboxLauncher.StartRefusalNamesItsCause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherStartRefusalNamesItsCauseTest::RunTest(const FString&)
{
	const FRTLauncherStartDecision NoSelection =
		FRTLauncherWorkspace::DecideStart(FString(), ERTScenarioAuthoringResult::Success, FString());
	TestFalse(TEXT("senza selezione non si parte"), NoSelection.bAllowed);
	TestTrue(TEXT("e la causa e' la selezione, non lo scenario"), NoSelection.Refusal == ERTLauncherStartRefusal::NoSelection);
	TestFalse(TEXT("con una frase propria"), NoSelection.Reason.IsEmpty());

	const FRTLauncherStartDecision NotFound =
		FRTLauncherWorkspace::DecideStart(TEXT("Nessuno.Scenario"), ERTScenarioAuthoringResult::NotFound, FString());
	TestTrue(TEXT("un id assente e' `NotFound`, non `Invalid`"), NotFound.Refusal == ERTLauncherStartRefusal::NotFound);
	TestTrue(TEXT("e la frase nomina l'id che non si trova"), NotFound.Reason.Contains(TEXT("Nessuno.Scenario")));

	const FRTLauncherStartDecision ToolBroken =
		FRTLauncherWorkspace::DecideStart(TEXT("Combat.BasicAttack"), ERTScenarioAuthoringResult::RunFailed, FString());
	TestTrue(TEXT("un guasto dello strumento non si spaccia per scenario invalido"), ToolBroken.Refusal == ERTLauncherStartRefusal::ToolFailure);

	// La selezione mancante prevale su un esito che direbbe di si': e' l'ordine dei due controlli.
	TestTrue(TEXT("nessuna selezione batte un `Success` che nessuno ha prodotto"),
		FRTLauncherWorkspace::DecideStart(FString(), ERTScenarioAuthoringResult::Success, FString()).Refusal == ERTLauncherStartRefusal::NoSelection);

	return true;
}

/**
 * Solo `Success` avvia la sessione — e la prova si fa sull'enum REALE, non su un elenco scritto qui.
 *
 * ⚠️ **E' il test che protegge il guardrail «nessun silent fallback», e lo protegge nel tempo.** Un valore
 * aggiunto domani a `ERTScenarioAuthoringResult` entra da solo in questo ciclo: se la mappatura lo
 * lasciasse cadere in un ramo permissivo, il test diventa rosso senza che nessuno debba ricordarsi di
 * aggiornarlo. Un elenco letterale di sei casi, invece, invecchia in silenzio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherOnlySuccessStartsTheSessionTest,
	"RefactorTactics.DevSandboxLauncher.OnlySuccessStartsTheSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherOnlySuccessStartsTheSessionTest::RunTest(const FString&)
{
	const UEnum* Enum = StaticEnum<ERTScenarioAuthoringResult>();
	if (!TestNotNull(TEXT("l'enum della facade e' riflesso: senza, questo test non misura niente"), Enum))
	{
		return false;
	}

	int32 Examined = 0;
	for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
	{
		// L'ultima voce di un `UENUM` e' il sentinella `_MAX` generato: non e' un esito che la facade
		// restituisce, e classificarlo direbbe qualcosa su un valore che non esiste.
		if (Enum->HasMetaData(TEXT("Hidden"), Index) || Enum->GetNameStringByIndex(Index).EndsWith(TEXT("_MAX")))
		{
			continue;
		}

		const auto Value = static_cast<ERTScenarioAuthoringResult>(Enum->GetValueByIndex(Index));
		const FRTLauncherStartDecision Decision = FRTLauncherWorkspace::DecideStart(TEXT("Combat.BasicAttack"), Value, FString());
		++Examined;

		if (Value == ERTScenarioAuthoringResult::Success)
		{
			TestTrue(TEXT("`Success` avvia"), Decision.bAllowed);
			continue;
		}

		TestFalse(*FString::Printf(TEXT("`%s` non avvia"), *Enum->GetNameStringByIndex(Index)), Decision.bAllowed);
		TestFalse(*FString::Printf(TEXT("`%s` dice perche'"), *Enum->GetNameStringByIndex(Index)), Decision.Reason.IsEmpty());
	}

	// Senza questa riga il ciclo potrebbe non aver esaminato niente e il test sarebbe verde su zero casi.
	TestTrue(TEXT("almeno due esiti esaminati: un ciclo vuoto passerebbe senza misurare"), Examined >= 2);

	return true;
}

/**
 * Il registro dichiara solo cio' che si raggiunge, e nomina la issue di cio' che manca.
 *
 * ⚠️ **Il test itera il registro invece di elencare cinque voci**, ed e' precisamente l'AC di #1682:
 * *«aggiungere una superficie non richiede di modificare nessun criterio»*. Una voce nuova entra qui da
 * sola, con le stesse due condizioni.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherSurfaceRegistryDeclaresOnlyWhatItReachesTest,
	"RefactorTactics.DevSandboxLauncher.SurfaceRegistryDeclaresOnlyWhatItReaches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherSurfaceRegistryDeclaresOnlyWhatItReachesTest::RunTest(const FString&)
{
	const TArray<FRTLauncherSurface>& Surfaces = FRTLauncherWorkspace::Surfaces();

	if (!TestTrue(TEXT("il registro non e' vuoto"), Surfaces.Num() > 0))
	{
		return false;
	}

	int32 DeclaredCount = 0;
	int32 PendingCount = 0;

	for (const FRTLauncherSurface& Surface : Surfaces)
	{
		TestFalse(TEXT("ogni superficie ha una chiave"), Surface.Key.IsNone());

		if (Surface.bDeclared)
		{
			++DeclaredCount;
			TestFalse(*FString::Printf(TEXT("'%s' e' dichiarata: ha un bersaglio da attivare"), *Surface.Key.ToString()),
				Surface.ActivationTarget.IsNone());
			TestEqual(*FString::Printf(TEXT("'%s' e' dichiarata: non ha una issue pendente"), *Surface.Key.ToString()),
				Surface.PendingIssue, 0);
			TestTrue(*FString::Printf(TEXT("'%s' non ha etichetta di attesa"), *Surface.Key.ToString()),
				FRTLauncherWorkspace::PendingLabel(Surface).IsEmpty());
		}
		else
		{
			++PendingCount;
			TestTrue(*FString::Printf(TEXT("'%s' e' pendente: nomina la issue che la porta"), *Surface.Key.ToString()),
				Surface.PendingIssue > 0);
			TestTrue(*FString::Printf(TEXT("'%s' e' pendente: non promette un bersaglio"), *Surface.Key.ToString()),
				Surface.ActivationTarget.IsNone());
			TestTrue(*FString::Printf(TEXT("'%s' dice il numero della issue"), *Surface.Key.ToString()),
				FRTLauncherWorkspace::PendingLabel(Surface).Contains(FString::FromInt(Surface.PendingIssue)));
		}
	}

	// Entrambi i rami vanno esercitati: con zero pendenti la seconda meta' del test sarebbe vacua, e con
	// zero dichiarate il workspace non porterebbe da nessuna parte.
	TestTrue(TEXT("almeno una superficie dichiarata"), DeclaredCount > 0);
	TestTrue(TEXT("almeno una pendente: e' cio' che rende il registro onesto invece che ottimista"), PendingCount > 0);

	return true;
}

/**
 * Le chiavi sono uniche, e `Find` risolve ogni voce del registro.
 *
 * ⚠️ Due voci con la stessa chiave non sono un doppione innocuo: `Find` ne restituirebbe una sola, e il
 * pannello attiverebbe sempre quella — l'altra resterebbe elencata e inerte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLauncherSurfaceKeysAreUniqueTest,
	"RefactorTactics.DevSandboxLauncher.SurfaceKeysAreUnique",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLauncherSurfaceKeysAreUniqueTest::RunTest(const FString&)
{
	const TArray<FRTLauncherSurface>& Surfaces = FRTLauncherWorkspace::Surfaces();

	TSet<FName> Seen;
	for (const FRTLauncherSurface& Surface : Surfaces)
	{
		bool bAlready = false;
		Seen.Add(Surface.Key, &bAlready);
		TestFalse(*FString::Printf(TEXT("la chiave '%s' compare una volta sola"), *Surface.Key.ToString()), bAlready);

		const FRTLauncherSurface* Found = FRTLauncherWorkspace::Find(Surface.Key);
		if (TestNotNull(*FString::Printf(TEXT("'%s' si ritrova per chiave"), *Surface.Key.ToString()), Found))
		{
			TestTrue(TEXT("e la voce ritrovata e' la stessa"), Found->bDeclared == Surface.bDeclared);
		}
	}

	TestNull(TEXT("una chiave che non esiste non risolve, invece di risolvere alla prima"),
		FRTLauncherWorkspace::Find(TEXT("SuperficieCheNonEsiste")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
