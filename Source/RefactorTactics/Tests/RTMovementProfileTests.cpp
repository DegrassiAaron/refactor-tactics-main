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
	TestEqual(TEXT("lo Sprint raddoppia il budget (D-412)"), Sprint.MoveBudgetPercent, 200);
	TestEqual(TEXT("e i passi seguono lo stesso moltiplicatore"), Sprint.StepBudgetPercent, 200);

	// `Withdraw` e' il criterio esplicito di `#653`: «oggi non e' esprimibile». Il moltiplicatore e' [D-412].
	TestTrue(TEXT("il Withdraw e' un profilo"), Withdraw.IsValid());
	TestEqual(TEXT("il Withdraw ripiega a un quarto del budget (D-412)"), Withdraw.MoveBudgetPercent, 25);

	// ⏱️ *Fino al 2026-09-13 queste tre righe asserivano ASSOLUTI — Sprint `8`, Withdraw `2` — e [D-412] le
	// ha rese percentuali. Il numero assoluto non e' sparito: e' diventato una CONSEGUENZA del budget
	// dell'eroe, ed e' il caso che il test qui sotto esibisce invece di lasciarlo dedurre.*
	// 🔴 **E il numero si muove per quasi tutto il roster.** Il budget non e' il `MoveRange = 4` del default
	// di classe: e' `GetEffectiveMoveRange()`, che parte da `Hero->MovePoints` (`RTUnit.cpp:1600`), e gli
	// eroi spediti dichiarano **5 · 5 · 4 · 6**. ∴ lo Sprint passa da `8` fisso a **10 · 10 · 8 · 12**, e
	// solo per uno dei quattro resta dov'era. Le righe qui sotto coprono i due estremi.
	TestEqual(TEXT("l'eroe da 4 scatta di 8, come l'assoluto di prima"), Sprint.ResolveMoveBudget(4), 8);
	TestEqual(TEXT("quello da 6 scatta di 12, che l'assoluto non gli dava"), Sprint.ResolveMoveBudget(6), 12);

	// ⛔ **Il troncamento e' la regola, non un residuo dell'aritmetica intera**: [D-412] prescrive di
	// arrotondare **per difetto**. Senza questa riga la migrazione passerebbe anche se qualcuno arrotondasse
	// per eccesso «per conservare il 2 di prima».
	TestEqual(TEXT("un eroe da 5 ripiega di 1, non di 2"), Withdraw.ResolveMoveBudget(5), 1);
	TestEqual(TEXT("e uno da 8 di 2, che e' il caso in cui il vecchio assoluto era giusto"),
		Withdraw.ResolveMoveBudget(8), 2);

	// 🔴 **Sotto `4` il quarto e' ZERO, e il caso si raggiunge in partita.** Non e' un difetto del
	// moltiplicatore: e' esattamente il caso per cui [D-412] prescrive il **primo passo garantito**, che
	// vive nel pathfinding e **non e' implementato**.
	//
	// ⛔ **E non basta dire «nessun eroe spedito e' in quel caso»**, come una prima stesura di questa nota
	// sosteneva guardando il `MoveRange = 4` del default di classe. Il budget passa da
	// `GetEffectiveMoveRange()`, che applica anche lo `StandUp` di [D-319]: l'eroe che dichiara
	// `MovePoints = 4` scende a **3** quando si rialza, e con l'`Overwatch` armato — che [D-070] gli riserva
	// al solo `Withdraw` — **non ripiega affatto**. La riga asserisce lo zero; a correggerlo sara' il primo
	// passo garantito.
	TestEqual(TEXT("budget 3: il quarto e' zero, e serve il primo passo garantito che non c'e' ancora"),
		Withdraw.ResolveMoveBudget(3), 0);

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

	TestEqual(TEXT("il Move vale il 100% dei passi dell'unita'"),
		Move.StepBudgetPercent, FRTMovementProfile::NeutralPercent);
	TestEqual(TEXT("e il 100% dell'asperita'"), Move.MoveBudgetPercent, FRTMovementProfile::NeutralPercent);

	// Un eroe da 7 resta da 7, non diventa da 5.
	TestEqual(TEXT("un eroe da 7 conserva 7"), Move.ResolveMoveBudget(7), 7);
	TestEqual(TEXT("un eroe da 3 conserva 3"), Move.ResolveMoveBudget(3), 3);
	TestEqual(TEXT("i passi seguono lo stesso moltiplicatore"), Move.ResolveStepBudget(7), 7);

	// Lo Sprint invece SCALA l'unita' invece di ignorarla, ed e' cio' che [D-412] ha cambiato.
	//
	// ⏱️ *Questa riga diceva «lo Sprint vale 8 anche per un eroe da 3», e asseriva l'assoluto: era il
	// comportamento di prima, e conteneva il difetto che il moltiplicatore corregge — un eroe da 9 aveva uno
	// Sprint piu' CORTO del proprio Move.*
	const FRTMovementProfile Sprint = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileSprint);
	TestEqual(TEXT("lo Sprint di un eroe da 3 vale 6, non l'assoluto 8"), Sprint.ResolveMoveBudget(3), 6);
	// 🔑 **L'invariante che l'assoluto NON garantiva, ed e' la ragione del moltiplicatore**: lo scatto e'
	// STRETTAMENTE piu' lungo del passo per ogni eroe che si muove. Con `8` cablato un eroe da 9 lo smentiva.
	//
	// ⚠️ **`>` e non `>=`, e la differenza non e' pedanteria**: con `>=` l'asserzione sarebbe vera anche se
	// qualcuno riportasse lo Sprint a `100`, cioe' se il profilo smettesse di essere uno scatto. Il caso
	// `Range == 0` e' escluso apposta — li' entrambi valgono zero, e non c'e' scatto da confrontare.
	for (const int32 Range : { 1, 3, 4, 5, 7, 9, 12 })
	{
		TestTrue(*FString::Printf(TEXT("con budget %d lo Sprint e' piu' lungo del Move"), Range),
			Sprint.ResolveMoveBudget(Range) > Move.ResolveMoveBudget(Range));
	}
	// ⛔ E il pavimento vale per entrambi: un budget nullo non produce uno scatto negativo.
	TestEqual(TEXT("budget zero: nessuno dei due va sotto zero"), Sprint.ResolveMoveBudget(0), 0);
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
 * ✅ `Sneak` ha i suoi numeri, e da quel momento si sceglie ([D-412], che chiude `AE-5`).
 *
 * ⏱️ *Questo test si chiamava `SneakIsDeclaredButNotPlannable` e asseriva l'opposto — `bPlannable` falso —
 * ed era giusto: il catalogo markdown gli assegnava «—» al posto di un budget, e dargliene uno l'avrebbe
 * inventato. `AE-5` ha risposto alle tre domande che poneva (costo, portata, rumore), e la lacuna che il
 * test dichiarava non c'e' piu'.*
 *
 * 🔑 **Il test non e' stato cancellato ma ROVESCIATO**, e la ragione e' che serve ancora: `bPlannable`
 * distingue *«non ha numeri»* dalle **altre due** ragioni per cui un profilo non si offre — `Still`, che e'
 * derivato, e `Withdraw`, che e' riservato. Un `bPlannable` senza nessun `false` non proverebbe piu' che
 * quella distinzione esiste, ed e' cio' che `#1410` `AC-4` chiede di non confondere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileSneakIsPlannableWithItsNumbers,
	"RefactorTactics.MovementProfile.SneakIsPlannableWithItsNumbers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileSneakIsPlannableWithItsNumbers::RunTest(const FString&)
{
	const FRTMovementProfile Sneak = URTMovementProfileLibrary::FindProfile(
		URTMovementProfileLibrary::ProfileSneak);

	TestTrue(TEXT("lo Sneak esiste come tipo"), Sneak.IsValid());
	TestTrue(TEXT("e ora si puo' pianificare: AE-5 e' chiusa da D-412"), Sneak.bPlannable);
	TestEqual(TEXT("meta' del budget (D-412)"), Sneak.MoveBudgetPercent, 50);
	TestEqual(TEXT("un eroe da 4 sguscia di 2"), Sneak.ResolveMoveBudget(4), 2);

	// ⛔ **Ogni profilo del catalogo e' ora pianificabile, e il campo resta comunque necessario**: le
	// esclusioni dall'offerta sono TRE e solo una di esse e' «non ha numeri». Senza questa asserzione
	// nessuno noterebbe che il campo ha smesso di distinguere qualcosa.
	for (const FRTMovementProfile& Profile : URTMovementProfileLibrary::GetCoreMovementProfileCatalog())
	{
		TestTrue(*FString::Printf(TEXT("%s e' pianificabile"), *Profile.Id.ToString()), Profile.bPlannable);
	}

	// 🔑 **I numeri arrivano fino al piano, e non restano un dato di catalogo.** Un piano che nomina il
	// profilo `Sneak` lo dichiara, col budget dimezzato: e' cio' che [D-412] rende vero, ed e' verificabile
	// **senza** che un'azione del catalogo lo nomini — `ProfileForPlan` legge `MovementProfileId` dalla voce
	// di piano, chiunque l'abbia messa.
	//
	// ⛔ **Nessuna `Action.Sneak` esiste, ed e' voluto.** Una voce del catalogo core rende obbligatoria la
	// propria chiave icona, e `DA_IconCatalog` non ha `UI.Icon.Action.Sneak`: l'azione entra con [#1410],
	// che porta il selettore e il glifo. Qui si prova la META' che [D-412] possiede — il profilo ha i suoi
	// numeri — e non quella che non gli appartiene.
	//
	// ⛔ **E non si passa da nessun elenco di profili offribili**: `OfferableProfiles()` e' uscita con
	// [D-425] insieme al selettore che la consumava. Cio' che qui si prova e' la META' che [D-412]
	// possiede — il profilo ha i suoi numeri — non che qualcuno lo scelga.
	const TArray<FRTPlannedAction> SneakPlan = {
		PlanEntryWithProfile(TEXT("Action.Sneak"), URTMovementProfileLibrary::ProfileSneak),
	};
	TestEqual(TEXT("un piano che nomina lo Sneak dichiara quel profilo"),
		URTMovementProfileLibrary::ProfileForPlan(SneakPlan).Id,
		URTMovementProfileLibrary::ProfileSneak);
	TestEqual(TEXT("col budget dimezzato che il profilo porta"),
		URTMovementProfileLibrary::ProfileForPlan(SneakPlan).ResolveMoveBudget(4), 2);
	return true;
}

/**
 * Il criterio centrale della DoD: **il piano dichiara quale profilo l'unita' ha scelto**.
 *
 * 🔑 Lo dichiara RICAVANDOLO, non con un campo parallelo: e' la ragione per cui non puo' esistere un piano
 * che dica `Sprint` come azione e `Move` come profilo.
 */
/**
 * 🔑 **Una sola CORSA fra i cinque profili, e il criterio e' un dato** ([D-406]).
 *
 * [D-319] dice «chi ha perso l'equilibrio non corre», e fino a [#641] il soggetto di quella frase e'
 * implicito: il criterio e' `ERTMovementStyle::Budget` **dentro il ciclo del Dash**, dove lo `Sprint` e'
 * l'unica mobilita' a budget che puo' stare. ⛔ Lo stile da solo **non** distingue — `Action.Move` dichiara
 * lo stesso `Budget`, e `Actions.SprintIsAMoveProfileResolvedAfterBlast` lo asserisce — quindi con la
 * migrazione il recinto sparisce e il criterio portato com'e' rifiuterebbe anche il Move normale.
 *
 * ⚠️ **Questo test cade se qualcuno dichiara una seconda corsa senza deciderlo**, ed e' il punto: `Withdraw`
 * in particolare deve restare `false` perche' [D-070] lo IMPONE a chi arma l'`Overwatch` — un criterio che
 * lo rifiutasse lascerebbe uno sbilanciato con lo slot movimento riservato a un profilo vietato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileOnlySprintIsARun,
	"RefactorTactics.MovementProfile.OnlySprintIsARun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileOnlySprintIsARun::RunTest(const FString&)
{
	const TArray<FRTMovementProfile> Catalogo = URTMovementProfileLibrary::GetCoreMovementProfileCatalog();
	if (!TestTrue(TEXT("il catalogo dei profili non e' vuoto"), Catalogo.Num() > 0))
	{
		return false;
	}

	// Si enumerano le CORSE invece di contarle: un totale direbbe «una» senza dire quale, e il giorno in cui
	// cambiasse il messaggio non aiuterebbe chi legge il rosso.
	TArray<FName> Corse;
	for (const FRTMovementProfile& Profilo : Catalogo)
	{
		if (Profilo.bIsRun)
		{
			Corse.Add(Profilo.Id);
		}
	}

	TestTrue(TEXT("lo Sprint e' una corsa"), Corse.Contains(URTMovementProfileLibrary::ProfileSprint));
	TestFalse(TEXT("il Move normale non lo e': camminare non e' correre"),
		Corse.Contains(URTMovementProfileLibrary::ProfileMove));
	TestFalse(TEXT("ne' il fermo"), Corse.Contains(URTMovementProfileLibrary::ProfileStill));
	TestFalse(TEXT("ne' lo Sneak, che e' l'opposto di una corsa"),
		Corse.Contains(URTMovementProfileLibrary::ProfileSneak));
	// D-070: il ripiegamento e' IMPOSTO dall'Overwatch, non scelto. Se fosse una corsa, uno sbilanciato si
	// troverebbe lo slot movimento riservato a un profilo che il criterio gli vieta.
	TestFalse(TEXT("ne' il Withdraw, che l'Overwatch impone invece di offrire"),
		Corse.Contains(URTMovementProfileLibrary::ProfileWithdraw));

	return true;
}

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


/**
 * `AC-4` di `#1410`: **un solo profilo si dichiara**, e le altre quattro esclusioni hanno quattro ragioni
 * diverse che non vanno confuse.
 *
 * ⏱️ *Questo test si chiamava `OfferableExcludesForThreeReasons` e interrogava `OfferableProfiles()`, cioe'
 * l'elenco che il selettore mostrava.* [D-425] ha smontato il selettore: `Move` e `Sprint` si LEGGONO dalla
 * distanza, quindi la domanda non e' piu' *«quali compaiono nell'elenco»* ma **«quali si possono
 * dichiarare»**, e le ragioni passano da tre a quattro perche' i due profili derivati sono due.
 *
 * ⛔ **Non basta asserire il predicato: il test chiude anche il CANALE.** `IsDeclarableProfile` potrebbe
 * dire il vero mentre `CeilingProfile` onora comunque un `MovementProfile.Sprint` scritto nel campo del
 * piano — ed e' precisamente il canale che l'`AC-4` nomina. Le ultime due asserzioni lo verificano dove la
 * dichiarazione ha effetto, non dove e' descritta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileOnlySneakIsDeclarable,
	"RefactorTactics.MovementProfile.OnlySneakIsDeclarable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileOnlySneakIsDeclarable::RunTest(const FString&)
{
	// `Sneak` si dichiara: e' la rinuncia a meta' distanza in cambio del silenzio ([D-425] punto (8)).
	TestTrue(TEXT("Sneak si dichiara"),
		URTMovementProfileLibrary::IsDeclarableProfile(URTMovementProfileLibrary::ProfileSneak));

	// `Move` e `Sprint` sono LETTI dalla distanza: dichiararli sarebbe il selettore che D-425 smonta.
	TestFalse(TEXT("Move non si dichiara: lo legge la distanza"),
		URTMovementProfileLibrary::IsDeclarableProfile(URTMovementProfileLibrary::ProfileMove));
	TestFalse(TEXT("Sprint non si dichiara: lo legge la distanza"),
		URTMovementProfileLibrary::IsDeclarableProfile(URTMovementProfileLibrary::ProfileSprint));

	// `Still` e' derivato dall'assenza di piano; `Withdraw` lo impone l'Overwatch ([D-070]).
	TestFalse(TEXT("Still non si dichiara: e' l'assenza di piano"),
		URTMovementProfileLibrary::IsDeclarableProfile(URTMovementProfileLibrary::ProfileStill));
	TestFalse(TEXT("Withdraw non si dichiara: lo impone l'Overwatch"),
		URTMovementProfileLibrary::IsDeclarableProfile(URTMovementProfileLibrary::ProfileWithdraw));

	// ⛔ **E il campo del piano non e' una scorciatoia.** Un eroe da 4 che ha pianificato UN passo cammina,
	// e ci cammina anche se qualcuno gli ha scritto `Sprint` nella dichiarazione: la banda viene dalla
	// distanza, e il canale e' chiuso dove produce effetti.
	TestEqual(TEXT("dichiarare Sprint non fa sprintare chi ha pianificato un passo"),
		URTMovementProfileLibrary::ProfileForPlannedSteps(/*Passi*/ 1, /*Base*/ 4,
			URTMovementProfileLibrary::ProfileSprint, NAME_None).Id,
		URTMovementProfileLibrary::ProfileMove);

	// ⛔ E nemmeno abbassa un tetto: dichiarare `Withdraw` non da' il ripiegamento a chi non ha armato
	// niente. Senza questa riga il predicato sopra resterebbe vero e il canale aperto lo stesso.
	TestEqual(TEXT("dichiarare Withdraw non riserva niente: il tetto resta quello nudo"),
		URTMovementProfileLibrary::CeilingProfile(
			URTMovementProfileLibrary::ProfileWithdraw, NAME_None).Id,
		URTMovementProfileLibrary::ProfileSprint);
	return true;
}

/**
 * `AC-2` di `#1410`, la meta' che questa PR consegna: **il profilo dichiarato entra nel piano**.
 *
 * 🔑 Non e' un dettaglio di composizione: da qui passa tutto il resto — il budget dello snapshot
 * (`MakeSimUnit` -> `ProfileForPlan`) e il divieto di reazione di [D-116], che legge il piano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileActionForProfileIsTheInverse,
	"RefactorTactics.MovementProfile.ActionForProfileIsTheInverse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileActionForProfileIsTheInverse::RunTest(const FString&)
{
	const FRTActionDef ForMove =
		URTMovementProfileLibrary::FindActionForProfile(URTMovementProfileLibrary::ProfileMove);
	const FRTActionDef ForSprint =
		URTMovementProfileLibrary::FindActionForProfile(URTMovementProfileLibrary::ProfileSprint);

	TestEqual(TEXT("il profilo Move porta ad Action.Move"), ForMove.ActionId, FName(TEXT("Action.Move")));
	TestEqual(TEXT("il profilo Sprint porta ad Action.Sprint"), ForSprint.ActionId,
		FName(TEXT("Action.Sprint")));

	// L'inversa e' davvero tale: si torna al punto di partenza.
	TestEqual(TEXT("e l'andata e ritorno chiude"), ForSprint.MovementProfileId,
		URTMovementProfileLibrary::ProfileSprint);

	// ⛔ Un profilo che nessuna azione nomina non produce un'azione inventata.
	// ⚠️ Il soggetto e' `Still`, non piu' `Withdraw`: dal 2026-09-13 il ripiegamento ha la propria azione
	// (`#1410` `AC-5`), mentre «fermo» resta per costruzione senza — lo si ottiene non muovendosi, e
	// un'azione che lo nomini non avrebbe niente da eseguire.
	TestTrue(TEXT("un profilo senza azione non produce un Def"),
		URTMovementProfileLibrary::FindActionForProfile(URTMovementProfileLibrary::ProfileStill)
			.ActionId.IsNone());
	return true;
}

/**
 * `#641` / [D-116]: lo Sprint paga TRE prezzi, e questo test li asserisce insieme perche' le voci della
 * decisione **non sono separabili**.
 *
 * ⛔ **Il caso che nessuno vuole e' la migrazione a meta'**: fase spostata e prezzi rimasti indietro
 * produce l'upgrade puro che [D-015] vieta — 8 punti contro 5, `Exposed` inerte, nessun cooldown. Tre
 * asserzioni separate cadrebbero una alla volta lasciando credere a un difetto isolato; qui cadono
 * insieme, che e' la forma della decisione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileSprintPaysItsPrices,
	"RefactorTactics.MovementProfile.SprintPaysItsPrices",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileSprintPaysItsPrices::RunTest(const FString&)
{
	FRTActionDef Sprint;
	if (!TestTrue(TEXT("Action.Sprint e' nel catalogo"), FindCoreAction(TEXT("Action.Sprint"), Sprint)))
	{
		return false;
	}

	// (1) La fase: dopo il Blast, quindi non spara da una posizione nuova.
	TestEqual(TEXT("D-116 voce 1: Sprint risolve in NormalMovement"),
		Sprint.ResolutionPhase, ERTResolutionPhase::NormalMovement);

	// (4) Il prezzo che la migrazione gli toglierebbe, restituito: `Exposed` due turni.
	int32 ExposedTurns = 0;
	for (const FRTActionEffectSpec& Effect : Sprint.Effects)
	{
		if (Effect.Effect == ERTActionEffect::Status)
		{
			ExposedTurns = FMath::Max(ExposedTurns, Effect.StatusDuration);
		}
	}
	TestEqual(TEXT("D-116 voce 4: Exposed dura 2 turni, altrimenti e' inerte dopo il Blast"),
		ExposedTurns, 2);

	// Il terzo prezzo: chi corre non para. Il FLAG resta sul catalogo; a leggerlo dal PIANO e non piu' dal
	// solo `ResolveDash` e' `ARTTurnManager`, perche' in fase Move lo scatto non passa piu' di li'.
	TestFalse(TEXT("D-116: lo Sprint nega la reazione"), Sprint.bAllowsReaction);

	// Controprova: il Move normale non paga nessuno dei tre, altrimenti gli assert sopra non
	// distinguerebbero lo Sprint da qualunque azione.
	FRTActionDef Move;
	TestTrue(TEXT("Action.Move e' nel catalogo"), FindCoreAction(TEXT("Action.Move"), Move));
	TestTrue(TEXT("il Move normale conserva la reazione"), Move.bAllowsReaction);
	TestEqual(TEXT("e non applica effetti"), Move.Effects.Num(), 0);
	return true;
}


/**
 * `AC-5` di `#1410`: armare l'`Overwatch` RISERVA lo slot movimento al `Withdraw` ([D-070]).
 *
 * 🔑 **Il vincolo e' un DATO sull'azione, non un `if` sull'`ActionId`**: il consumatore legge
 * `ReservesMovementProfileId` e non ha bisogno di sapere quale azione lo ha imposto. Un kit che
 * dichiarasse un'altra azione «questa ti inchioda a un ripiegamento» e' coperto senza toccare il
 * controller.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileOverwatchReservesTheSlot,
	"RefactorTactics.MovementProfile.OverwatchReservesTheMovementSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileOverwatchReservesTheSlot::RunTest(const FString&)
{
	FRTActionDef Overwatch;
	if (!TestTrue(TEXT("Action.Overwatch e' nel catalogo"),
		FindCoreAction(TEXT("Action.Overwatch"), Overwatch)))
	{
		return false;
	}

	TestEqual(TEXT("l'Overwatch riserva lo slot movimento al Withdraw (D-070)"),
		Overwatch.ReservesMovementProfileId, URTMovementProfileLibrary::ProfileWithdraw);

	// ⛔ E il vincolo si LEGGE dal piano: e' cosi' che il selettore sa di dover rifiutare.
	FRTPlannedAction Planned;
	Planned.Def = Overwatch;
	const TArray<FRTPlannedAction> WithOverwatch = { Planned };
	TestEqual(TEXT("il piano che arma l'Overwatch dichiara la riserva"),
		URTMovementProfileLibrary::ReservedProfileForPlan(WithOverwatch),
		URTMovementProfileLibrary::ProfileWithdraw);

	// Controprova: un piano che non arma niente non riserva nulla, altrimenti l'asserzione sopra sarebbe
	// vera per qualunque piano e il test non distinguerebbe.
	FRTPlannedAction Attack;
	Attack.Def.ActionId = TEXT("Action.BasicAttack");
	const TArray<FRTPlannedAction> WithoutOverwatch = { Attack };
	TestTrue(TEXT("un piano senza riserve non ne dichiara"),
		URTMovementProfileLibrary::ReservedProfileForPlan(WithoutOverwatch).IsNone());

	// ⚠️ Nessuna ALTRA azione del catalogo riserva lo slot: se domani ne comparisse una per sbaglio, il
	// giocatore si troverebbe il movimento inchiodato senza che nulla lo dichiari.
	int32 Reserving = 0;
	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		if (!Def.ReservesMovementProfileId.IsNone()) { ++Reserving; }
	}
	TestEqual(TEXT("e l'Overwatch e' la sola a riservarlo"), Reserving, 1);
	return true;
}

/**
 * `#1410` `AC-5`, la meta' senza la quale la riserva sarebbe immobilita': `Withdraw` **e' pianificabile**.
 *
 * 🔴 **L'azione non esisteva fino al 2026-09-13.** Il PROFILO era arrivato con `#653` — era il suo
 * criterio, *«`Withdraw` diventa esprimibile»* — ma un profilo che nessuna azione nomina non entra in un
 * piano: `ProfileForPlan` non lo restituirebbe mai. Senza questa voce, [D-070] avrebbe inchiodato chi arma
 * l'`Overwatch` all'immobilita' invece che al ripiegamento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileWithdrawIsPlannable,
	"RefactorTactics.MovementProfile.WithdrawIsPlannable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileWithdrawIsPlannable::RunTest(const FString&)
{
	const FRTActionDef Withdraw =
		URTMovementProfileLibrary::FindActionForProfile(URTMovementProfileLibrary::ProfileWithdraw);

	TestEqual(TEXT("il profilo Withdraw ha un'azione che lo nomina"),
		Withdraw.ActionId, FName(TEXT("Action.Withdraw")));
	// ⏱️ *Asseriva «che vale i 2 punti di D-070», cioe' `RangeCells == 2`.* Dal 2026-09-18 il budget lo
	// possiede il profilo ([D-427]): `2` era una seconda sede, e per un eroe da 5 il ×0,25 di [D-412] vale
	// **1**. I due numeri non erano nemmeno d'accordo.
	TestEqual(TEXT("il budget del ripiegamento non vive sull'azione: RangeCells e' zero"), Withdraw.RangeCells, 0);
	TestEqual(TEXT("e occupa il solo slot movimento"), Withdraw.Slot, ERTActionSlot::Movement);

	// ⚠️ **Fase `Move` e non `Dash`**: il ripiegamento e' un profilo della famiglia `Move` ([D-015]), non
	// una mobilita' rapida. Se risolvesse prima del Blast sarebbe uno scatto, e D-070 non dice questo.
	TestEqual(TEXT("e risolve dopo il Blast, come gli altri profili"),
		Withdraw.ResolutionPhase, ERTResolutionPhase::NormalMovement);

	// ⛔ Resta comunque IMPOSTO e non scelto: lo si ottiene armando l'Overwatch ([D-070]), e dichiararlo
	// non fa niente. ⏱️ *Fino al 2026-09-15 la stessa cosa si asseriva su `OfferableProfiles()`, cioe'
	// sull'elenco del selettore; [D-425] lo ha smontato, e la domanda si pone dove la riserva ha effetto.*
	TestFalse(TEXT("ma non e' dichiarabile (AC-4)"),
		URTMovementProfileLibrary::IsDeclarableProfile(URTMovementProfileLibrary::ProfileWithdraw));
	TestEqual(TEXT("e la riserva arriva dal PIANO, non dalla dichiarazione"),
		URTMovementProfileLibrary::CeilingProfile(
			NAME_None, URTMovementProfileLibrary::ProfileWithdraw).Id,
		URTMovementProfileLibrary::ProfileWithdraw);
	return true;
}

/**
 * `AC-2` di `#1410`, il cuore di [D-425]: **la banda segue la distanza pianificata**.
 *
 * 🔑 **Il denominatore e' sempre il movimento BASE dell'unita'** ([D-425] punto (1)), e il test lo prova
 * misurando due eroi diversi sulla stessa distanza: quattro passi sono un cammino per chi vale `4` e una
 * corsa per chi vale `2`. Un test su un solo eroe resterebbe verde anche con una soglia cablata.
 *
 * ⛔ **Le soglie si asseriscono AI BORDI, non al centro.** `4 <= 4` e' `Move`, `5` e' `Sprint`: un test che
 * chiedesse `2` e `7` resterebbe verde con la soglia spostata di uno, che e' l'errore piu' probabile.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileBandFollowsThePlannedDistance,
	"RefactorTactics.MovementProfile.BandFollowsThePlannedDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileBandFollowsThePlannedDistance::RunTest(const FString&)
{
	using Lib = URTMovementProfileLibrary;
	auto Banda = [](int32 Passi, int32 Base)
	{
		return Lib::ProfileForPlannedSteps(Passi, Base, NAME_None, NAME_None).Id;
	};

	// Nessun passo: il fermo, che e' un profilo e non un'assenza.
	TestEqual(TEXT("zero passi: Still"), Banda(0, 4), Lib::ProfileStill);

	// Eroe da 4 — la banda `Move` arriva a `1x` INCLUSO, e il primo passo oltre e' gia' corsa.
	TestEqual(TEXT("1 passo su base 4: Move"), Banda(1, 4), Lib::ProfileMove);
	TestEqual(TEXT("4 passi su base 4: ancora Move, il bordo e' incluso"), Banda(4, 4), Lib::ProfileMove);
	TestEqual(TEXT("5 passi su base 4: Sprint, il primo passo oltre 1x"), Banda(5, 4), Lib::ProfileSprint);
	TestEqual(TEXT("8 passi su base 4: Sprint, il bordo alto"), Banda(8, 4), Lib::ProfileSprint);

	// 🔑 Stessa distanza, eroe diverso, banda diversa: e' cio' che prova che il denominatore e' l'eroe.
	TestEqual(TEXT("4 passi su base 2 sono una corsa, non un cammino"), Banda(4, 2), Lib::ProfileSprint);

	// ⛔ Oltre OGNI tetto si legge `Sprint`, non `Move`: un percorso che nessun tetto copre non deve
	// pagare il prezzo piu' basso. Non dovrebbe accadere — il troncamento lo previene — e se accade si vede.
	TestEqual(TEXT("oltre il tetto piu' alto: Sprint, non Move"), Banda(99, 4), Lib::ProfileSprint);

	// ⚠️ **Un tetto dichiarato o imposto E' il profilo, e sopra di esso non si legge nessuna banda.** Un
	// passo solo resta `Sneak` per chi lo ha dichiarato e `Withdraw` per chi ha armato: le due bande basse
	// si sovrappongono, e la distanza da sola non le separerebbe mai.
	TestEqual(TEXT("un passo, Sneak dichiarato: Sneak"),
		Lib::ProfileForPlannedSteps(1, 4, Lib::ProfileSneak, NAME_None).Id, Lib::ProfileSneak);
	TestEqual(TEXT("un passo, Withdraw imposto: Withdraw"),
		Lib::ProfileForPlannedSteps(1, 4, NAME_None, Lib::ProfileWithdraw).Id, Lib::ProfileWithdraw);
	TestEqual(TEXT("e la riserva vince sulla dichiarazione (D-070)"),
		Lib::ProfileForPlannedSteps(1, 4, Lib::ProfileSneak, Lib::ProfileWithdraw).Id,
		Lib::ProfileWithdraw);

	// [D-319]: chi ha perso l'equilibrio non corre, e ora «non corre» significa «non arriva cosi' lontano».
	// Il piano da 6 passi resta, ma la banda che ne risulta e' un cammino — e il prezzo dello Sprint non
	// scatta per una corsa che il troncamento gli ha gia' tolto.
	TestEqual(TEXT("sbilanciato: 6 passi su base 4 si leggono Move, non Sprint"),
		Lib::ProfileForPlannedSteps(6, 4, NAME_None, NAME_None, /*bRunDenied*/ true).Id,
		Lib::ProfileMove);
	TestEqual(TEXT("ma sgusciare non e' correre: lo Sneak dichiarato sopravvive"),
		Lib::ProfileForPlannedSteps(2, 4, Lib::ProfileSneak, NAME_None, /*bRunDenied*/ true).Id,
		Lib::ProfileSneak);
	return true;
}

/**
 * [D-425] punto (9): **il tetto nudo e' `2x`**, e la banda alta e' raggiungibile senza dichiarare niente.
 *
 * ⛔ **E' la meta' che un test sulle bande non copre.** `BandFollowsThePlannedDistance` prova che `5` passi
 * su base `4` si LEGGONO `Sprint`; questo prova che `5` passi si possono **pianificare**. Con il tetto a
 * `1x` la prima asserzione resterebbe verde e la banda `Sprint` non sarebbe raggiungibile da nessuno —
 * cioe' una banda che non e' una banda, l'alternativa che [D-425] scarta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMovementProfileNakedCeilingIsTwiceTheBase,
	"RefactorTactics.MovementProfile.NakedCeilingIsTwiceTheBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMovementProfileNakedCeilingIsTwiceTheBase::RunTest(const FString&)
{
	using Lib = URTMovementProfileLibrary;

	const FRTMovementProfile Nudo = Lib::CeilingProfile(NAME_None, NAME_None);
	TestEqual(TEXT("senza dichiarazioni il tetto e' il doppio dei passi"),
		Nudo.ResolveStepBudget(4), 8);
	TestEqual(TEXT("e il doppio dell'asperita'"), Nudo.ResolveMoveBudget(4), 8);

	// 🔑 **La riprova che chiude il cerchio**: pianificati fin la', i passi si leggono `Sprint`. Senza
	// questa riga il tetto potrebbe valere `8` mentre la banda alta resta irraggiungibile per una soglia
	// scritta altrove.
	TestEqual(TEXT("e la distanza che concede si legge Sprint"),
		Lib::ProfileForPlannedSteps(Nudo.ResolveStepBudget(4), 4, NAME_None, NAME_None).Id,
		Lib::ProfileSprint);

	// Dichiarare `Sneak` DIMEZZA: e' il prezzo di [D-425] punto (8), pagato in distanza.
	TestEqual(TEXT("dichiarare Sneak dimezza il tetto"),
		Lib::CeilingProfile(Lib::ProfileSneak, NAME_None).ResolveStepBudget(4), 2);

	// L'`Overwatch` lo porta a un quarto ([D-070] + [D-412]). ⚠️ Su base `4` fa `1`, non `2`: la divisione
	// tronca, ed e' l'«arrotondare per difetto» che il catalogo dichiara.
	TestEqual(TEXT("l'Overwatch lo porta a un quarto"),
		Lib::CeilingProfile(NAME_None, Lib::ProfileWithdraw).ResolveStepBudget(4), 1);

	// [D-319]: negare la corsa e' abbassare il tetto a `1x`, non rifiutare una dichiarazione.
	TestEqual(TEXT("sbilanciato: il tetto nudo scende a 1x"),
		Lib::CeilingProfile(NAME_None, NAME_None, /*bRunDenied*/ true).ResolveStepBudget(4), 4);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
