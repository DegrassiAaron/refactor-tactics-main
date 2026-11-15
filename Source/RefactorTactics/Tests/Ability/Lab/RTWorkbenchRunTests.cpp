// La VARIANTE attraverso il PERCORSO CANONICO: `Intent -> Planning -> Snapshot -> Resolver -> TurnLog`.
//
// Il contratto della variante (`#1988`) sapeva scrivere numeri su una lista di abilita' che nessuno
// eseguiva. Qui la lista e' quella di una partita vera, e la domanda e' la sola che conta per uno strumento
// di design: **il risultato cambia?**
//
// Lo scenario e' quello di `Scenarios/Combat/BasicAttack.json`, costruito in memoria con gli stessi numeri:
// Gadget colpisce Riktor a distanza 2 con `Hero.Gadget.ArcPulse` (220 danni). Dal CP 53.3c di E53 il colpo passa
// dalla formula di [D-589] con i numeri del catalogo: Gadget sta alle SPALLE di Riktor (che guarda a est), quindi
// entra il suo bonus alle spalle (60, [D-599]) e la DEF 45 di Riktor ([D-595] (3)): `floor((220 + 60) × 100/145)
// = 193`, meno lo scudo BASE di [D-224] (50) → Riktor finisce a 1057. `SaluteDiB1` lo calcola dal catalogo.
//
// ⚠️ E' l'unico scenario del corpus che asserisce un DANNO invece di una posizione, cioe' esattamente cio'
// che una variante numerica deve poter muovere. Per questo si riusa invece di inventarne uno.

#include "Misc/AutomationTest.h"

#include "Ability/Catalog/RTActionReadout.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Ability/Lab/RTWorkbenchVariant.h"
#include "Combat/RTCombatLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ScenarioHarness/RTScenarioRunner.h"
#include "ScenarioHarness/Authoring/RTTestScenario.h"
#include "ScenarioHarness/Report/RTTestResult.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTWorkbenchRun
{
	UWorld* MakeWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	/**
	 * La salute di Riktor dopo UN `ArcPulse` da `Danno`, dalla formula e dal catalogo: alle spalle (bonus di Gadget),
	 * contro la DEF di Riktor, meno lo scudo base. Il test chiede «il risultato cambia?», non «quanto vale la
	 * formula»: calcolarlo qui tiene la domanda ferma quando cambiano i numeri del catalogo.
	 */
	int32 SaluteDiB1(int32 Danno)
	{
		const URTHeroData* Gadget = URTHeroCatalogLibrary::MakeGadget();
		const URTHeroData* Riktor = URTHeroCatalogLibrary::MakeRiktor();
		return Riktor->MaxHealth - (URTCombatLibrary::MitigateDirectDamage(Danno, Gadget->RearBonus, 100, Riktor->Defense)
			- URTCombatLibrary::BaseShield);
	}

	void DestroyWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** `Combat.BasicAttack` in memoria: Gadget colpisce Riktor, e si asserisce la SALUTE attesa. */
	FRTTestScenario BasicAttack(int32 SaluteAttesaDiB1)
	{
		FRTTestScenario S;
		S.ScenarioId = TEXT("Internal.WorkbenchVariantRun");
		S.MapRadius = 4;

		FRTScenarioUnit Gadget;
		Gadget.Id = TEXT("A1");
		Gadget.HeroId = FName(TEXT("Hero.Gadget"));
		Gadget.TeamId = 0;
		Gadget.Cell = FRTCellId(-1, 0);
		S.Units.Add(Gadget);

		FRTScenarioUnit Riktor;
		Riktor.Id = TEXT("B1");
		Riktor.HeroId = FName(TEXT("Hero.Riktor"));
		Riktor.TeamId = 1;
		Riktor.Cell = FRTCellId(1, 0);
		S.Units.Add(Riktor);

		FRTScenarioTurn Turn;
		FRTScenarioIntent Colpo;
		Colpo.UnitId = TEXT("A1");
		Colpo.Ability = FName(TEXT("Hero.Gadget.ArcPulse"));
		Colpo.Target = TEXT("B1");
		Turn.Intents.Add(Colpo);
		S.Turns.Add(Turn);

		FRTTestExpectation Salute;
		Salute.Kind = ERTAssertionKind::UnitHpEquals;
		Salute.UnitId = TEXT("B1");
		Salute.Value = SaluteAttesaDiB1;
		S.Expect.Add(Salute);

		return S;
	}

	/** Una variante che porta il danno di ArcPulse al valore dato. */
	FRTWorkbenchVariant DannoArcPulse(const TCHAR* Id, int32 Danno)
	{
		FRTWorkbenchVariant V;
		V.VariantId = FName(Id);
		FRTAbilityParameterOverride Ov;
		Ov.ActionId = FName(TEXT("Hero.Gadget.ArcPulse"));
		Ov.ParameterKey = RTActionParameterKeys::Damage();
		Ov.Value = Danno;
		V.Overrides.Add(Ov);
		return V;
	}
}

/**
 * **Senza variante il risultato e' quello di sempre.**
 *
 * E' la meta' che si dimentica: un ingresso nuovo non deve cambiare cio' che facevano tutti gli altri. Il
 * default di `RunSingle` e' la baseline, e questo test lo pinna — senza, la variante potrebbe alterare in
 * silenzio ogni run del corpus.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkbenchNoVariantEqualsBaselineTest,
	"RefactorTactics.Workbench.NoVariantEqualsBaseline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkbenchNoVariantEqualsBaselineTest::RunTest(const FString&)
{
	UWorld* World = RTWorkbenchRun::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	// 1057 e' il numero del corpus (`Combat.BasicAttack`): 1200 di Riktor, 220 di ArcPulse alle spalle, DEF 45, scudo 50.
	TestEqual(TEXT("premessa: la baseline vale 1057"), RTWorkbenchRun::SaluteDiB1(220), 1057);
	const FRTTestResult Esito = URTScenarioRunner::RunSingle(World,
		RTWorkbenchRun::BasicAttack(RTWorkbenchRun::SaluteDiB1(220)), /*bTearDownAfter=*/ true);

	TestEqual(TEXT("senza variante lo scenario passa con la salute canonica"),
		Esito.Outcome, ERTTestOutcome::Pass);

	RTWorkbenchRun::DestroyWorld(World);
	return true;
}

/**
 * **Con la variante il risultato cambia, e passa dal Resolver.**
 *
 * Il Workbench non applica danno: propone un numero, e il colpo lo risolve il gioco. Se questo test passa
 * mentre `NoVariantEqualsBaseline` passa a 1057, l'override ha attraversato tutta la catena — kit, intent,
 * planning, snapshot, resolver — perche' non c'e' nessun'altra strada per cui quel numero possa arrivare
 * alla salute di B1.
 *
 * 320 anziche' 220, con lo stesso bonus alle spalle e la stessa DEF: `1200 - (floor(380 × 100/145) - 50) = 988`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkbenchVariantChangesTheOutcomeTest,
	"RefactorTactics.Workbench.VariantChangesTheOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkbenchVariantChangesTheOutcomeTest::RunTest(const FString&)
{
	UWorld* World = RTWorkbenchRun::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	TestEqual(TEXT("premessa: con 320 la salute attesa e' 988"), RTWorkbenchRun::SaluteDiB1(320), 988);
	const FRTTestResult Esito = URTScenarioRunner::RunSingle(World,
		RTWorkbenchRun::BasicAttack(RTWorkbenchRun::SaluteDiB1(320)), /*bTearDownAfter=*/ true,
		RTWorkbenchRun::DannoArcPulse(TEXT("Test.ArcPulse32"), 320));

	TestEqual(TEXT("con danno 320 la salute attesa e' 988, e lo scenario passa"),
		Esito.Outcome, ERTTestOutcome::Pass);

	RTWorkbenchRun::DestroyWorld(World);
	return true;
}

/**
 * **Anti-vacuita' del test sopra**: la stessa variante contro l'aspettativa BASELINE deve **fallire**.
 *
 * Senza questo, `VariantChangesTheOutcome` sarebbe verde anche se la variante non arrivasse mai al resolver
 * e l'aspettativa con la variante (988) fosse sbagliata per un'altra ragione. Qui si chiede il contrario: con la variante
 * attiva, il numero di prima non regge piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkbenchVariantBreaksTheBaselineExpectationTest,
	"RefactorTactics.Workbench.VariantBreaksTheBaselineExpectation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkbenchVariantBreaksTheBaselineExpectationTest::RunTest(const FString&)
{
	UWorld* World = RTWorkbenchRun::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	const FRTTestResult Esito = URTScenarioRunner::RunSingle(World,
		RTWorkbenchRun::BasicAttack(RTWorkbenchRun::SaluteDiB1(220)), /*bTearDownAfter=*/ true,
		RTWorkbenchRun::DannoArcPulse(TEXT("Test.ArcPulse32"), 320));

	// `Fail` e non `Error`: lo scenario e' valido ed e' girato, l'assertion non regge. La distinzione e'
	// quella che il formato tiene fra un difetto del GIOCO e un difetto del TEST.
	TestEqual(TEXT("con la variante attiva l'aspettativa baseline NON regge piu'"),
		Esito.Outcome, ERTTestOutcome::Fail);

	RTWorkbenchRun::DestroyWorld(World);
	return true;
}

/**
 * **Un'azione che nessuna unita' porta ferma la run PRIMA del primo turno.**
 *
 * `Error` e non `Fail`: e' un difetto dello STRUMENTO — la variante chiede di sovrascrivere qualcosa che in
 * questo scenario non c'e' — e farlo passare per un difetto del gioco manderebbe a cercare nel posto
 * sbagliato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkbenchUnknownActionFailsBeforeTheFirstTurnTest,
	"RefactorTactics.Workbench.UnknownActionFailsBeforeTheFirstTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkbenchUnknownActionFailsBeforeTheFirstTurnTest::RunTest(const FString&)
{
	UWorld* World = RTWorkbenchRun::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	FRTWorkbenchVariant Assurda;
	Assurda.VariantId = TEXT("Test.AzioneCheNessunoPorta");
	FRTAbilityParameterOverride Ov;
	Ov.ActionId = FName(TEXT("Hero.Ivrin.PulseShot")); // Ivrin non e' in questo scenario
	Ov.ParameterKey = RTActionParameterKeys::Damage();
	Ov.Value = 99;
	Assurda.Overrides.Add(Ov);

	const FRTTestResult Esito = URTScenarioRunner::RunSingle(World,
		RTWorkbenchRun::BasicAttack(RTWorkbenchRun::SaluteDiB1(220)), /*bTearDownAfter=*/ true, Assurda);

	TestEqual(TEXT("la run si ferma con Error, non con Fail"), Esito.Outcome, ERTTestOutcome::Error);

	RTWorkbenchRun::DestroyWorld(World);
	return true;
}

/**
 * **La stessa variante da' lo stesso risultato**, due volte di fila.
 *
 * Il determinismo non e' un di piu' per uno strumento di design: un confronto baseline/variante in cui la
 * variante da' due numeri diversi non dice niente sul numero che il designer ha cambiato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkbenchSameVariantSameResultTest,
	"RefactorTactics.Workbench.SameVariantSameResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkbenchSameVariantSameResultTest::RunTest(const FString&)
{
	UWorld* World = RTWorkbenchRun::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	const FRTTestResult Prima = URTScenarioRunner::RunSingle(World,
		RTWorkbenchRun::BasicAttack(RTWorkbenchRun::SaluteDiB1(320)), /*bTearDownAfter=*/ true,
		RTWorkbenchRun::DannoArcPulse(TEXT("Test.Deterministica"), 320));

	const FRTTestResult Seconda = URTScenarioRunner::RunSingle(World,
		RTWorkbenchRun::BasicAttack(RTWorkbenchRun::SaluteDiB1(320)), /*bTearDownAfter=*/ true,
		RTWorkbenchRun::DannoArcPulse(TEXT("Test.Deterministica"), 320));

	TestEqual(TEXT("la prima run passa"), Prima.Outcome, ERTTestOutcome::Pass);
	TestEqual(TEXT("la seconda pure"), Seconda.Outcome, ERTTestOutcome::Pass);
	TestEqual(TEXT("e i due esiti coincidono"), Prima.Outcome, Seconda.Outcome);

	RTWorkbenchRun::DestroyWorld(World);
	return true;
}

/**
 * **L'override raggiunge OGNI istanza che porta l'azione, non la prima.**
 *
 * 🔴 E' il limite che questa slice ha scoperto in `#1988`: `Apply` si fermava alla prima corrispondenza.
 * Con un solo kit — l'unico caso che i test del contratto esercitavano — era corretto; con piu' unita' no,
 * perche' `ConfigureFromHeroData` da' a ciascuna istanze **proprie**. Il difetto sarebbe stato visibile
 * solo nel 2v2, e come una differenza attribuita al numero invece che a quale figura ha agito.
 *
 * Il test sta al livello della libreria e non della run perche' li' e' discriminante: due kit costruiti
 * separatamente sono esattamente due unita' che portano la stessa abilita'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWorkbenchVariantReachesEveryInstanceTest,
	"RefactorTactics.Workbench.VariantReachesEveryInstance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWorkbenchVariantReachesEveryInstanceTest::RunTest(const FString&)
{
	// Due kit distinti: e' cio' che due unita' dello stesso eroe hanno in partita.
	const URTHeroData* PrimoEroe = URTHeroCatalogLibrary::MakeGadget();
	const URTHeroData* SecondoEroe = URTHeroCatalogLibrary::MakeGadget();
	if (!TestNotNull(TEXT("il primo Gadget esiste"), PrimoEroe)) { return false; }
	if (!TestNotNull(TEXT("il secondo Gadget esiste"), SecondoEroe)) { return false; }

	TArray<URTActionData*> Unione;
	URTActionData* PrimoArcPulse = nullptr;
	URTActionData* SecondoArcPulse = nullptr;
	for (const TObjectPtr<URTActionData>& V : PrimoEroe->Actions)
	{
		if (URTActionData* A = V)
		{
			Unione.Add(A);
			if (A->Def.ActionId == FName(TEXT("Hero.Gadget.ArcPulse"))) { PrimoArcPulse = A; }
		}
	}
	for (const TObjectPtr<URTActionData>& V : SecondoEroe->Actions)
	{
		if (URTActionData* A = V)
		{
			Unione.Add(A);
			if (A->Def.ActionId == FName(TEXT("Hero.Gadget.ArcPulse"))) { SecondoArcPulse = A; }
		}
	}

	if (!TestNotNull(TEXT("ArcPulse del primo kit"), PrimoArcPulse)) { return false; }
	if (!TestNotNull(TEXT("ArcPulse del secondo kit"), SecondoArcPulse)) { return false; }
	// Anti-vacuita': se fossero la STESSA istanza, il test non distinguerebbe «tutte» da «la prima».
	if (!TestNotEqual(TEXT("i due kit hanno istanze DISTINTE, o il test non discrimina"),
		static_cast<const void*>(PrimoArcPulse), static_cast<const void*>(SecondoArcPulse)))
	{
		return false;
	}

	FRTWorkbenchVariant V = RTWorkbenchRun::DannoArcPulse(TEXT("Test.DueIstanze"), 320);
	FRTWorkbenchVariant Ripristino;
	if (!TestEqual(TEXT("la variante si applica"),
		URTWorkbenchVariantLibrary::Apply(V, Unione, Ripristino), ERTVariantApplyResult::Ok))
	{
		return false;
	}

	TestEqual(TEXT("la PRIMA istanza porta il valore nuovo"), PrimoArcPulse->Power, 320);
	TestEqual(TEXT("e anche la SECONDA, che prima restava indietro"), SecondoArcPulse->Power, 320);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
