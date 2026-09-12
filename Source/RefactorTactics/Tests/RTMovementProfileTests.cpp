#include "Misc/AutomationTest.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTMovementProfileLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTPlanValidationLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Voce di piano che dichiara un profilo, come fa il catalogo core per `Move` e `Sprint`. */
	FRTPlannedAction PlanEntryWithProfile(const FName& ActionId, const FName& ProfileId)
	{
		FRTPlannedAction Entry;
		Entry.Def.ActionId = ActionId;
		Entry.Def.Slot = ERTActionSlot::Movement;
		Entry.Def.MovementProfileId = ProfileId;
		return Entry;
	}

	/** Il def di un'azione del catalogo core, per `ActionId`. */
	bool FindCoreAction(const FName& ActionId, FRTActionDef& Out)
	{
		for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
		{
			if (Def.ActionId == ActionId)
			{
				Out = Def;
				return true;
			}
		}
		return false;
	}
}

/**
 * Il fatto che `#653` doveva rovesciare: i profili ESISTONO come entita'.
 *
 * Prima di questo checkpoint `grep -rn MovementProfile Source/` dava zero, e i budget di §2.1 vivevano solo
 * nel catalogo markdown. Questo test non verifica un comportamento — verifica che ci sia un posto dove
 * scriverlo, che e' cio' che [#641] e [#606] non trovavano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileCatalogDeclaresTheProfiles,
	"RefactorTactics.MovementProfile.CatalogDeclaresTheProfiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileCatalogDeclaresTheProfiles::RunTest(const FString&)
{
	const FRTMovementProfile Sprint = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileSprint);
	const FRTMovementProfile Withdraw = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileWithdraw);

	TestTrue(TEXT("lo Sprint e' un profilo"), Sprint.IsValid());
	TestEqual(TEXT("lo Sprint porta gli 8 punti del catalogo par. 2.1"), Sprint.MoveBudget, 8);
	TestEqual(TEXT("e gli 8 passi, finche' ogni cella costa 1"), Sprint.StepBudget, 8);

	// `Withdraw` e' il criterio esplicito di `#653`: «oggi non e' esprimibile». Il numero e' [D-070].
	TestTrue(TEXT("il Withdraw e' un profilo"), Withdraw.IsValid());
	TestEqual(TEXT("il Withdraw porta i 2 punti di D-070"), Withdraw.MoveBudget, 2);

	// Un profilo che nessuno ha dichiarato non si inventa.
	TestFalse(TEXT("un Id sconosciuto non produce un profilo"),
		URTMovementProfileLibrary::FindProfile(TEXT("MovementProfile.Teleport")).IsValid());
	return true;
}

/**
 * 🔑 **La clausola per cui `#653` non cambia niente**, e il test che la tiene vera.
 *
 * `Move` non cabla il `5` del catalogo markdown: eredita dall'unita'. Se qualcuno gli scrivesse un numero,
 * ogni eroe con `MoveRange` diverso da quello si troverebbe il budget cambiato senza che nessuna decisione
 * lo avesse deciso — ed e' il modo in cui questo checkpoint potrebbe rompere il gioco in silenzio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileMoveInheritsUnitBudget,
	"RefactorTactics.MovementProfile.MoveInheritsUnitBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileMoveInheritsUnitBudget::RunTest(const FString&)
{
	const FRTMovementProfile Move = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileMove);

	TestEqual(TEXT("il Move dichiara di ereditare i passi"),
		Move.StepBudget, FRTMovementProfile::InheritFromUnit);
	TestEqual(TEXT("e l'asperita'"), Move.MoveBudget, FRTMovementProfile::InheritFromUnit);

	// Un eroe da 7 resta da 7, non diventa da 5.
	TestEqual(TEXT("un eroe da 7 conserva 7"), Move.ResolveMoveBudget(7), 7);
	TestEqual(TEXT("un eroe da 3 conserva 3"), Move.ResolveMoveBudget(3), 3);
	TestEqual(TEXT("i passi seguono la stessa eredita'"), Move.ResolveStepBudget(7), 7);

	// Lo Sprint invece IGNORA l'unita': e' il primo profilo che sposta davvero un numero.
	const FRTMovementProfile Sprint = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileSprint);
	TestEqual(TEXT("lo Sprint vale 8 anche per un eroe da 3"), Sprint.ResolveMoveBudget(3), 8);
	return true;
}

/**
 * Chi non pianifica movimento conserva il budget che ha sempre avuto.
 *
 * ⚠️ E' il caso che questo checkpoint poteva rompere senza accorgersene: nello snapshot `MoveBudget` e' una
 * CAPACITA', non una dichiarazione di spesa, e da essa dipende cio' che vede chi sta ancora pianificando.
 * Un `Still` da zero avrebbe reso immobili le unita' senza piano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileStillKeepsUnitCapacity,
	"RefactorTactics.MovementProfile.StillKeepsUnitCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileStillKeepsUnitCapacity::RunTest(const FString&)
{
	const FRTMovementProfile Still = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileStill);

	TestTrue(TEXT("il fermo e' un profilo"), Still.IsValid());
	TestEqual(TEXT("chi non muove conserva la capacita' dell'unita'"), Still.ResolveMoveBudget(5), 5);
	TestEqual(TEXT("il fermo concede la stabilita' piu' alta (spec par. 3.2)"), Still.Stability, 3);
	return true;
}

/**
 * ⛔ `Sneak` e' dichiarato SENZA numeri (`AE-5`), e sta nel catalogo proprio per questo.
 *
 * Ometterlo avrebbe nascosto la lacuna; dargli un budget l'avrebbe inventata. Il tipo lo prevede, il dato
 * no, e `bPlannable` e' il campo che lo dice invece di lasciarlo intuire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileSneakIsDeclaredButNotPlannable,
	"RefactorTactics.MovementProfile.SneakIsDeclaredButNotPlannable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileSneakIsDeclaredButNotPlannable::RunTest(const FString&)
{
	const FRTMovementProfile Sneak = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileSneak);

	TestTrue(TEXT("lo Sneak esiste come tipo"), Sneak.IsValid());
	TestFalse(TEXT("ma non si puo' pianificare: non ha numeri (AE-5)"), Sneak.bPlannable);

	// Ogni altro profilo del catalogo e' invece scegliibile: senza questo confronto il test sopra passerebbe
	// anche se nessun profilo lo fosse.
	for (const FRTMovementProfile& Profile : URTMovementProfileLibrary::GetCoreMovementProfileCatalog())
	{
		if (Profile.Id != URTMovementProfileLibrary::ProfileSneak)
		{
			TestTrue(*FString::Printf(TEXT("%s e' pianificabile"), *Profile.Id.ToString()),
				Profile.bPlannable);
		}
	}
	return true;
}

/**
 * Il criterio centrale della DoD: **il piano dichiara quale profilo l'unita' ha scelto**.
 *
 * 🔑 Lo dichiara RICAVANDOLO, non con un campo parallelo: e' la ragione per cui non puo' esistere un piano
 * che dica `Sprint` come azione e `Move` come profilo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfilePlanDeclaresTheProfile,
	"RefactorTactics.MovementProfile.PlanDeclaresTheProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfilePlanDeclaresTheProfile::RunTest(const FString&)
{
	const TArray<FRTPlannedAction> SprintPlan = {
		PlanEntryWithProfile(TEXT("Action.Sprint"), URTMovementProfileLibrary::ProfileSprint),
	};
	TestEqual(TEXT("un piano che scatta dichiara lo Sprint"),
		URTMovementProfileLibrary::ProfileForPlan(SprintPlan).Id,
		URTMovementProfileLibrary::ProfileSprint);

	const TArray<FRTPlannedAction> MovePlan = {
		PlanEntryWithProfile(TEXT("Action.Move"), URTMovementProfileLibrary::ProfileMove),
	};
	TestEqual(TEXT("un piano che cammina dichiara il Move"),
		URTMovementProfileLibrary::ProfileForPlan(MovePlan).Id,
		URTMovementProfileLibrary::ProfileMove);

	// Nessun movimento pianificato: il fermo, che e' un profilo e non un'assenza.
	const TArray<FRTPlannedAction> EmptyPlan;
	TestEqual(TEXT("un piano vuoto dichiara il fermo"),
		URTMovementProfileLibrary::ProfileForPlan(EmptyPlan).Id,
		URTMovementProfileLibrary::ProfileStill);

	// Un'azione che non e' un movimento non porta un profilo e non ne impone uno.
	FRTPlannedAction Attack;
	Attack.Def.ActionId = TEXT("Action.BasicAttack");
	Attack.Def.Slot = ERTActionSlot::Main;
	const TArray<FRTPlannedAction> AttackOnly = { Attack };
	TestEqual(TEXT("un piano di solo attacco dichiara il fermo"),
		URTMovementProfileLibrary::ProfileForPlan(AttackOnly).Id,
		URTMovementProfileLibrary::ProfileStill);
	return true;
}

/**
 * Le azioni di movimento del catalogo core NOMINANO il proprio profilo.
 *
 * Senza questa riga il profilo esisterebbe ma non lo sceglierebbe nessuno: `ProfileForPlan` leggerebbe
 * `NAME_None` su ogni piano reale e risponderebbe sempre `Still`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileCoreActionsNameTheirProfile,
	"RefactorTactics.MovementProfile.CoreActionsNameTheirProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileCoreActionsNameTheirProfile::RunTest(const FString&)
{
	FRTActionDef Move;
	FRTActionDef Sprint;
	FRTActionDef Attack;

	TestTrue(TEXT("Action.Move e' nel catalogo core"), FindCoreAction(TEXT("Action.Move"), Move));
	TestTrue(TEXT("Action.Sprint e' nel catalogo core"), FindCoreAction(TEXT("Action.Sprint"), Sprint));
	TestTrue(TEXT("Action.BasicAttack e' nel catalogo core"),
		FindCoreAction(TEXT("Action.BasicAttack"), Attack));

	TestEqual(TEXT("il Move nomina il profilo Move"),
		Move.MovementProfileId, URTMovementProfileLibrary::ProfileMove);
	TestEqual(TEXT("lo Sprint nomina il profilo Sprint"),
		Sprint.MovementProfileId, URTMovementProfileLibrary::ProfileSprint);

	// ⚠️ Un'azione che non muove NON nomina un profilo: se lo facesse, `ProfileForPlan` leggerebbe il
	// profilo di un attacco come se fosse il modo in cui l'unita' si sposta.
	TestTrue(TEXT("un attacco non nomina nessun profilo"), Attack.MovementProfileId.IsNone());
	return true;
}

/**
 * La scala di `Stability` e' ORDINATA, ed e' la meta' del dato che [#606] verra' a cercare
 * (`legale <=> Profilo.Stability >= Azione.MinStability`).
 *
 * ⚠️ Il test verifica l'ORDINE, non i quattro valori: la spec li dichiara «la parte da playtestare». Un
 * ordine sbagliato invece renderebbe il modello a soglia privo di senso — lo Sprint concederebbe piu'
 * stabilita' di chi sta fermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileStabilityIsOrdered,
	"RefactorTactics.MovementProfile.StabilityIsOrdered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileStabilityIsOrdered::RunTest(const FString&)
{
	const int32 Still = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileStill).Stability;
	const int32 Sneak = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileSneak).Stability;
	const int32 Move = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileMove).Stability;
	const int32 Sprint = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileSprint).Stability;

	TestTrue(TEXT("fermo concede piu' stabilita' dello Sneak"), Still > Sneak);
	TestTrue(TEXT("lo Sneak piu' del Move"), Sneak > Move);
	TestTrue(TEXT("il Move piu' dello Sprint"), Move > Sprint);
	TestEqual(TEXT("e lo Sprint non ne concede (D-116: hai speso il turno a coprire distanza)"), Sprint, 0);
	return true;
}

/**
 * I due budget di [D-117] sono due CAMPI distinti sullo snapshot, e oggi portano lo stesso valore.
 *
 * ⚠️ **L'uguaglianza e' il comportamento corrente, non un invariante**: ogni cella costa `1`, quindi «quanto
 * lontano arrivi» e «quanta asperita' assorbi» coincidono. A separarli e' [#666]. Questo test cadra' quando
 * quella funzione di costo arrivera', ed e' esattamente cio' che deve fare — dichiarare che il valore era
 * uno solo finche' nessuno li ha separati.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileSnapshotCarriesBothBudgets,
	"RefactorTactics.MovementProfile.SnapshotCarriesBothBudgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileSnapshotCarriesBothBudgets::RunTest(const FString&)
{
	// Il costruttore che ogni chiamante usa oggi: un budget solo, e le due misure che ne nascono uguali.
	const FRTHexSimUnit Unit(0, FRTCellId(0, 0), /*MoveBudget*/ 5, /*bAlive*/ true);

	TestEqual(TEXT("l'asperita' e' quella dichiarata"), Unit.MoveBudget, 5);
	TestEqual(TEXT("i passi la seguono, finche' ogni cella costa 1"), Unit.StepBudget, 5);

	// ⛔ Un default a zero avrebbe reso immobile ogni unita' costruita da chi non conosce il campo nuovo,
	// nel momento in cui [#666] gli dara' un lettore.
	const FRTHexSimUnit Defaulted(1, FRTCellId(1, 0), /*MoveBudget*/ 3, /*bAlive*/ true);
	TestNotEqual(TEXT("il default non e' zero passi"), Defaulted.StepBudget, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
