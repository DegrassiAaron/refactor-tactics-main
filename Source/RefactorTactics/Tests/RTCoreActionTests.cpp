#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Core/RTGameplayTags.h"
#include "Turn/RTActionEffectLibrary.h"
#include "Turn/RTActionEvent.h"
#include "Turn/RTActionQueue.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Definizione dal catalogo delle azioni generiche. Nome distinto per file (unity build). */
	FRTActionDef CoreActionDef(const TCHAR* Id)
	{
		return URTCatalogLibrary::FindCoreAction(FName(Id));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoreActionsMatchDocumentTest,
	"RefactorTactics.Actions.CoreActionsMatchCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoreActionsMatchDocumentTest::RunTest(const FString&)
{
	// Le sei azioni fondamentali con fase, priorita' e fallback della tabella §1 del catalogo. Il documento e
	// il codice non possono divergere in silenzio: se una riga cambia di la', questo test diventa rosso.
	struct FExpected
	{
		const TCHAR* Id;
		ERTMatchPhase Macro;
		int32 Priority;
		ERTActionFallback Fallback;
		ERTActionSlot Slot;
	};
	const FExpected Expected[] = {
		{ TEXT("Action.Wait"),        ERTMatchPhase::Move,  100, ERTActionFallback::Stop,   ERTActionSlot::None },
		{ TEXT("Action.Move"),        ERTMatchPhase::Move,   50, ERTActionFallback::Stop,   ERTActionSlot::Movement },
		{ TEXT("Action.BasicAttack"), ERTMatchPhase::Blast,  50, ERTActionFallback::Cancel, ERTActionSlot::Main },
		{ TEXT("Action.Guard"),       ERTMatchPhase::Prep,   40, ERTActionFallback::Cancel, ERTActionSlot::Main },
		// `Action.Activate` non e' piu' qui (#199): [D-025] la dichiara assorbita da `Interact`, e il
		// catalogo non spedisce piu' due azioni per una cosa sola. Il suo Stable ID vive in
		// `Actions.RetiredStableIdRedirectsToHeir`, che e' dove la migrazione e' verificata.
		{ TEXT("Action.Interact"),    ERTMatchPhase::Blast,  80, ERTActionFallback::Cancel, ERTActionSlot::Main },
	};

	for (const FExpected& E : Expected)
	{
		const FRTActionDef Def = CoreActionDef(E.Id);
		if (!TestTrue(FString::Printf(TEXT("%s e' nel catalogo"), E.Id), Def.ActionId == FName(E.Id)))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: macro-fase"), E.Id),
			URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == E.Macro);
		TestEqual(FString::Printf(TEXT("%s: priorita'"), E.Id), Def.Priority, E.Priority);
		TestTrue(FString::Printf(TEXT("%s: fallback"), E.Id), Def.Fallback == E.Fallback);
		TestTrue(FString::Printf(TEXT("%s: slot"), E.Id), Def.Slot == E.Slot);
	}

	// Il Move risolve DOPO il Blast: e' la divergenza dichiarata dall'ADR-0003 §3, non un dettaglio.
	TestTrue(TEXT("il movimento normale segue gli attacchi"),
		static_cast<uint8>(ERTMatchPhase::Move) > static_cast<uint8>(ERTMatchPhase::Blast));

	// `Interact` agisce solo su cio' che e' ADIACENTE. (`Activate` non esiste piu': #199.)
	TestEqual(TEXT("Interact: solo adiacente"), CoreActionDef(TEXT("Action.Interact")).RangeCells, 1);

	// L'intero catalogo generico passa dal validator, nuove voci comprese.
	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions(URTCatalogLibrary::GetCoreActionCatalog());
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("le azioni generiche restano valide"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWaitAllowsFacingTest,
	"RefactorTactics.Actions.Wait.AllowsFacingAndReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWaitAllowsFacingTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. `Wait` non e' «non fare niente»: e' non spendere NULLA. Non occupa il
	// movimento ne' l'azione principale, quindi cio' che non costa slot — facing, reazione preparata, stance
	// gia' attiva, contesa di un obiettivo — resta possibile.
	const FRTActionDef Wait = CoreActionDef(TEXT("Action.Wait"));
	if (!TestTrue(TEXT("Action.Wait e' nel catalogo"), Wait.ActionId == FName(TEXT("Action.Wait"))))
	{
		return false;
	}

	TestTrue(TEXT("non occupa alcuno slot del turno"), Wait.Slot == ERTActionSlot::None);
	TestEqual(TEXT("non produce effetti"), Wait.Effects.Num(), 0);
	TestEqual(TEXT("risolve per ultima, cosi' non anticipa nulla"), Wait.Priority, 100);
	TestFalse(TEXT("non e' interrompibile: non c'e' nulla da interrompere"), Wait.bCanBeInterrupted);

	// Confronto che rende esplicita la differenza: l'azione principale e il movimento SI spendono.
	TestTrue(TEXT("l'attacco base occupa l'azione principale"),
		CoreActionDef(TEXT("Action.BasicAttack")).Slot == ERTActionSlot::Main);
	TestTrue(TEXT("il movimento occupa lo slot movimento"),
		CoreActionDef(TEXT("Action.Move")).Slot == ERTActionSlot::Movement);

	// Limite dichiarato: facing e reazioni non esistono ancora (E5 e la presentazione). Qui si fissa la
	// proprieta' che li rendera' possibili — che `Wait` non consumi lo slot che servirebbe loro.
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBasicAttackBandTest,
	"RefactorTactics.Actions.BasicAttack.DamageByRangeBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBasicAttackBandTest::RunTest(const FString&)
{
	// Tabella delle fasce (catalogo §1): piu' lontano si colpisce, meno si fa male.
	TestEqual(TEXT("corpo a corpo (r1): 28"), URTCatalogLibrary::BasicAttackDamageForRange(1), 28);
	TestEqual(TEXT("corto raggio (r3): 25"), URTCatalogLibrary::BasicAttackDamageForRange(3), 25);
	TestEqual(TEXT("medio raggio (r4): 22"), URTCatalogLibrary::BasicAttackDamageForRange(4), 22);
	TestEqual(TEXT("lungo raggio (r6): 20"), URTCatalogLibrary::BasicAttackDamageForRange(6), 20);

	// La curva non e' mai crescente: nessuna portata paga meno di una piu' corta.
	for (int32 Range = 1; Range < 10; ++Range)
	{
		TestTrue(FString::Printf(TEXT("r%d non fa piu' male di r%d"), Range + 1, Range),
			URTCatalogLibrary::BasicAttackDamageForRange(Range + 1)
			<= URTCatalogLibrary::BasicAttackDamageForRange(Range));
	}

	// L'azione costruita per un'arma ha UN solo ID e i due numeri della sua fascia.
	const FRTActionDef Melee = URTCatalogLibrary::MakeBasicAttack(1);
	TestTrue(TEXT("stesso ActionId per tutti gli eroi"), Melee.ActionId == FName(TEXT("Action.BasicAttack")));
	TestEqual(TEXT("portata dell'arma"), Melee.RangeCells, 1);
	TestEqual(TEXT("un solo effetto: il danno"), Melee.Effects.Num(), 1);
	TestTrue(TEXT("ed e' il danno della fascia"), Melee.Effects.Num() == 1
		&& Melee.Effects[0].Effect == ERTActionEffect::Damage && Melee.Effects[0].Amount == 28);

	const FRTActionDef Ranged = URTCatalogLibrary::MakeBasicAttack(6);
	TestTrue(TEXT("a lungo raggio il danno scende"), Ranged.Effects.Num() == 1 && Ranged.Effects[0].Amount == 20);

	// Portata degenere: si ricade sul corpo a corpo invece di produrre un'arma a portata zero.
	TestEqual(TEXT("portata 0 -> corpo a corpo"), URTCatalogLibrary::MakeBasicAttack(0).RangeCells, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardFirstHitOnlyTest,
	"RefactorTactics.Actions.Guard.FirstHitOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardFirstHitOnlyTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. La guardia vale sul PRIMO danno diretto: il secondo colpo dello stesso turno
	// arriva intero. Passa dalla stessa funzione di `Status.Exposed` (CP 4.2), che applica i delta validi una
	// volta sola per bersaglio — quindi la regola resta ordine-indipendente.
	TArray<int32> Delta;
	Delta.Init(0, 2);
	Delta[1] = -URTCombatLibrary::GuardFirstHitReduction;

	const TArray<FRTAttack> Attacks = { FRTAttack(1, 30), FRTAttack(1, 30) };
	const TArray<FRTAttack> Guarded = URTCombatResolver::ApplyFirstHitDelta(Attacks, Delta);

	if (!TestEqual(TEXT("due colpi restano due"), Guarded.Num(), 2)) { return false; }
	TestEqual(TEXT("il primo colpo e' ridotto di 15"), Guarded[0].Power, 15);
	TestEqual(TEXT("il secondo arriva intero: la guardia vale una volta"), Guarded[1].Power, 30);

	// Un colpo piu' debole della riduzione viene annullato, non trasformato in cura.
	const TArray<FRTAttack> Small = URTCombatResolver::ApplyFirstHitDelta({ FRTAttack(1, 10) }, Delta);
	TestEqual(TEXT("un colpo da 10 viene azzerato"), Small[0].Power, 0);

	// Guardia ED esposizione insieme: i due delta si cumulano (+5 -15 = -10), esito prevedibile di aver fatto
	// entrambe le cose nello stesso turno.
	TArray<int32> Both;
	Both.Init(0, 2);
	Both[1] = URTCombatLibrary::ExposedFirstHitBonus - URTCombatLibrary::GuardFirstHitReduction;
	const TArray<FRTAttack> Mixed = URTCombatResolver::ApplyFirstHitDelta({ FRTAttack(1, 30) }, Both);
	TestEqual(TEXT("esposto e in guardia: 30 + 5 - 15"), Mixed[0].Power, 20);

	// L'azione che lo produce e' dati: Guard dichiara lo stato, non un numero nell'orchestratore.
	const FRTActionDef Guard = CoreActionDef(TEXT("Action.Guard"));
	TestTrue(TEXT("Guard si prepara nel Prep"),
		URTCatalogLibrary::MapResolutionPhase(Guard.ResolutionPhase) == ERTMatchPhase::Prep);
	TestEqual(TEXT("Guard dichiara un solo effetto"), Guard.Effects.Num(), 1);
	TestTrue(TEXT("ed e' Status.Guarded per un turno (scade nel Cleanup)"),
		Guard.Effects.Num() == 1 && Guard.Effects[0].Effect == ERTActionEffect::Status
		&& Guard.Effects[0].StatusTag == TAG_Status_Guarded && Guard.Effects[0].StatusDuration == 1);
	TestFalse(TEXT("la guardia non e' interrompibile"), Guard.bCanBeInterrupted);
	return true;
}

namespace
{
	/**
	 * Un'azione del catalogo e' INERTE quando non dichiara effetti e, risolta, non produce eventi.
	 *
	 * Passa dal codice reale — `FindCoreAction` per il dato, `ProduceEvents` per il comportamento —
	 * invece di rileggere il catalogo a mano: un test che replica la definizione invece di chiamarla
	 * resta verde anche quando il codice cambia, ed e' il difetto peggiore che possa avere.
	 */
	void TestActionIsInert(FAutomationTestBase& T, const TCHAR* Id)
	{
		const FRTActionDef Def = URTCatalogLibrary::FindCoreAction(FName(Id));
		if (!T.TestTrue(FString::Printf(TEXT("%s e' nel catalogo"), Id), Def.ActionId == FName(Id)))
		{
			return;
		}
		T.TestEqual(FString::Printf(TEXT("%s non dichiara effetti"), Id), Def.Effects.Num(), 0);

		FRTActionInstance Instance;
		Instance.Def = Def;
		Instance.SourceUnitId = 1;
		Instance.TargetUnitId = 2;
		const TArray<FRTActionEvent> Events = URTActionEffectLibrary::ProduceEvents(Instance);
		T.TestEqual(FString::Printf(TEXT("%s risolta non produce eventi"), Id), Events.Num(), 0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInteractIsInertTest,
	"RefactorTactics.Actions.Interact.IsInertUntilImplemented",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInteractIsInertTest::RunTest(const FString&)
{
	// `Action.Interact` esiste a catalogo — fase Blast, priorita' 80, portata 1 — ma NON FA NIENTE:
	// nessun effetto dichiarato, nessun ramo del resolver che la esegua. Vedi la issue #273.
	//
	// Questo test fissa l'inerzia invece di ricordarsela. E' scritto per CADERE: il giorno in cui
	// E10.1 le da' un effetto, diventa rosso e obbliga a sostituirlo con il test del comportamento
	// vero. Senza, un effetto aggiunto a meta' passerebbe inosservato — che e' esattamente come
	// `Marked`, il bonus Wet di Flux e le reazioni d'eroe con `Effects` vuoto sono rimasti inerti
	// per settimane.
	//
	// Il catalogo continua a dichiarare fase, priorita' e slot perche' quelle SONO decise (D-025):
	// e' il comportamento a non esserci.
	TestActionIsInert(*this, TEXT("Action.Interact"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRetiredStableIdTest,
	"RefactorTactics.Actions.RetiredStableIdRedirectsToHeir",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRetiredStableIdTest::RunTest(const FString&)
{
	// #199 — la migrazione di `Action.Activate`, decisa da [D-014] e confermata da [D-025].
	//
	// Sostituisce `Actions.Activate.IsInertUntilImplemented`: quel test verificava che l'azione non facesse
	// nulla, e ora l'azione non c'e'. Le due meta' della migrazione vanno verificate INSIEME, perche' una
	// sola sarebbe un difetto — sparire senza redirect rompe le tracce, restare col redirect e' la doppia
	// verita' che l'issue vieta.

	// 1. NON e' piu' nel catalogo spedito: una cosa sola, una azione sola.
	const TArray<FRTActionDef> Core = URTCatalogLibrary::GetCoreActionCatalog();
	bool bStillShipped = false;
	for (const FRTActionDef& Def : Core)
	{
		if (Def.ActionId == FName(TEXT("Action.Activate"))) { bStillShipped = true; break; }
	}
	TestFalse(TEXT("Action.Activate non e' piu' nel catalogo generico"), bStillShipped);

	// 2. Ma l'ID resta INTERPRETABILE: una traccia scritta prima della migrazione deve restare leggibile.
	TestEqual(TEXT("l'ID ritirato si risolve nell'erede"),
		URTCatalogLibrary::ResolveLegacyActionId(TEXT("Action.Activate")), FName(TEXT("Action.Interact")));

	// 3. E il redirect passa dall'ingresso vero del catalogo, non solo dalla tabella: chi cerca l'ID morto
	//    ottiene la definizione dell'erede, con l'identita' dell'erede — non un'azione muta.
	const FRTActionDef Redirected = URTCatalogLibrary::FindCoreAction(TEXT("Action.Activate"));
	TestEqual(TEXT("FindCoreAction risponde con Interact"), Redirected.ActionId, FName(TEXT("Action.Interact")));
	TestEqual(TEXT("...e con la sua portata di adiacenza"), Redirected.RangeCells, 1);

	// 4. Un ID che non e' ritirato passa invariato: la tabella traduce le voci che ha, non tutto cio' che
	//    riceve. Senza questo caso, un redirect troppo largo passerebbe inosservato.
	TestEqual(TEXT("un ID vivo non viene tradotto"),
		URTCatalogLibrary::ResolveLegacyActionId(TEXT("Action.Move")), FName(TEXT("Action.Move")));
	TestEqual(TEXT("un ID inesistente resta se stesso"),
		URTCatalogLibrary::ResolveLegacyActionId(TEXT("Action.NonEsiste")), FName(TEXT("Action.NonEsiste")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSprintIsAMoveProfileTest,
	"RefactorTactics.Actions.SprintIsAMoveProfileResolvedPreBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSprintIsAMoveProfileTest::RunTest(const FString&)
{
	// #199, seconda voce — `Action.Sprint`. Il DoD ammetteva due vie: migrare la fase, **oppure** scrivere
	// perche' resta dov'e'. E' stata scelta la seconda, e questo test e' la forma eseguibile di quella
	// motivazione: senza, «documentato» sarebbe una frase in un file che nessuno ricontrolla.
	//
	// COSA DICE IL CANONE. [D-015]: «`Sneak · Normal · Sprint` sono profili della famiglia `Move`» e
	// «**`Sprint` non e' un Dash**». [D-028]: `Sprint` e' «solo movimento», cioe' slot Movimento.
	//
	// COSA SIGNIFICA ESSERE UN PROFILO DI MOVE, in termini verificabili: lo STILE (percorso a budget, con
	// pathfinding, non una linea retta) e lo SLOT (movimento, non principale). Entrambi sono veri qui sotto.
	const FRTActionDef Sprint = URTCatalogLibrary::FindCoreAction(TEXT("Action.Sprint"));
	if (!TestTrue(TEXT("Action.Sprint e' nel catalogo"), Sprint.ActionId == FName(TEXT("Action.Sprint"))))
	{
		return false;
	}
	const FRTActionDef Move = URTCatalogLibrary::FindCoreAction(TEXT("Action.Move"));

	TestTrue(TEXT("D-015: Sprint ha lo stile del Move (budget), non quello di un Dash (lineare)"),
		Sprint.MovementStyle == Move.MovementStyle && Sprint.MovementStyle == ERTMovementStyle::Budget);
	TestTrue(TEXT("D-028: Sprint spende lo slot MOVIMENTO, non il principale"),
		Sprint.Slot == ERTActionSlot::Movement && Sprint.Slot == Move.Slot);

	// LA DIVERGENZA, DICHIARATA. La fase resta `FastMovement`, cioe' PRIMA del Blast, mentre il Move normale
	// e' «l'ultima fase volontaria». Non e' una svista: D-015 mette nella stessa frase «Sprint non e' un
	// Dash» e «`Dash/Charge/Leap/Blink/Reposition` restano mobilita' speciali pre-Blast» — cioe' distingue
	// la FAMIGLIA (Move) dal MOMENTO (rapido), e Sprint e' l'unico caso in cui le due non coincidono.
	//
	// Migrare la fase e' una decisione di GIOCO, non un allineamento: cambierebbe chi incassa il Blast di
	// questo turno, quando `Status.Exposed` si applica, e la misura del bot (#149). D-028 avverte che senza
	// costo di slot lo Sprint rischia gia' di essere «un Move migliore»: spostarlo dopo il Blast toglierebbe
	// l'ultimo prezzo che paga, cioe' l'esposizione.
	//
	// Questo assert e' scritto per CADERE il giorno in cui quella decisione viene presa: chi migra la fase
	// trova qui la riga da cambiare e il perche' era com'era.
	TestTrue(TEXT("#199: Sprint resta pre-Blast (fase rapida) — divergenza DICHIARATA, non una svista"),
		Sprint.ResolutionPhase == ERTResolutionPhase::FastMovement);
	TestTrue(TEXT("...mentre il Move normale resta l'ultima fase volontaria"),
		Move.ResolutionPhase == ERTResolutionPhase::NormalMovement);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
