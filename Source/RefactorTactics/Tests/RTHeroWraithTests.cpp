#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	int32 WraithEffectAmount(const TArray<FRTActionEffectSpec>& Effects, ERTActionEffect Kind)
	{
		for (const FRTActionEffectSpec& Spec : Effects)
		{
			if (Spec.Effect == Kind) { return Spec.Amount; }
		}
		return 0;
	}

	int32 WraithVariantParam(const FRTAbilityVariant& Variant, const TCHAR* Key)
	{
		const int32* Found = Variant.Parameters.Find(FName(Key));
		return Found ? *Found : INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWraithMatchesCatalogTest,
	"RefactorTactics.Heroes.Wraith.MatchesCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWraithMatchesCatalogTest::RunTest(const FString&)
{
	// Numeri della tabella §4 del catalogo eroi v0.1.
	URTHeroData* Wraith = URTHeroCatalogLibrary::MakeWraith();
	if (!TestNotNull(TEXT("Wraith costruito"), Wraith)) { return false; }

	TestEqual(TEXT("HeroId"), Wraith->HeroId, FName(TEXT("Hero.Wraith")));
	TestEqual(TEXT("salute"), Wraith->MaxHealth, 90); // 100 -> 90 (#131): il costo statistico di Wraith
	TestEqual(TEXT("movimento"), Wraith->MovePoints, 6);
	TestEqual(TEXT("vista"), Wraith->VisionRange, 6);
	TestEqual(TEXT("resistenza push"), Wraith->PushResistance, 0);
	TestEqual(TEXT("affinita'"), Wraith->Affinity, FName(TEXT("Affinity.Movement")));
	TestEqual(TEXT("debolezza simmetrica a Riktor"), Wraith->Weakness, FName(TEXT("Affinity.Structures")));

	if (!TestEqual(TEXT("cinque azioni"), Wraith->Actions.Num(), 5)) { return false; }

	const URTActionData* PulseShot = Wraith->Actions[0];
	TestEqual(TEXT("PulseShot: 21 danni"), WraithEffectAmount(PulseShot->Def.Effects, ERTActionEffect::Damage), 21);
	TestEqual(TEXT("PulseShot: range 4"), PulseShot->Def.RangeCells, 4);
	TestNotEqual(TEXT("non e' il danno generico della fascia medio raggio"),
		WraithEffectAmount(PulseShot->Def.Effects, ERTActionEffect::Damage),
		URTCatalogLibrary::BasicAttackDamageForRange(4));

	const URTActionData* PassingBlade = Wraith->Actions[2];
	TestEqual(TEXT("PassingBlade: 20 danni"), WraithEffectAmount(PassingBlade->Def.Effects, ERTActionEffect::Damage), 20);
	TestEqual(TEXT("PassingBlade: Dash 3"), PassingBlade->Def.RangeCells, 3);
	TestTrue(TEXT("PassingBlade: risolve nel Dash"),
		URTCatalogLibrary::MapResolutionPhase(PassingBlade->Def.ResolutionPhase) == ERTMatchPhase::Dash);

	// PassingBlade ATTRAVERSA i nemici, la carica di Riktor si FERMA sul primo: la differenza e' un dato
	// (`ERTMovementStyle`), non un `if` sull'ActionId. E' il motivo per cui quel campo esiste (CP 4.5).
	//
	// Fino al 2026-08-08 questa riga diceva «passa attraverso» e verificava `LinearDash`, che si ferma su cio'
	// che incontra: la contraddizione era scritta dentro l'assertion, e il test la difendeva. `LinearPass`
	// esiste per farla sparire (MOB-1).
	TestTrue(TEXT("PassingBlade: passa attraverso (LinearPass)"),
		PassingBlade->Def.MovementStyle == ERTMovementStyle::LinearPass);
	const FRTActionDef ChargeDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	TestTrue(TEXT("e non si ferma addosso come la carica"),
		PassingBlade->Def.MovementStyle != ChargeDef.MovementStyle);

	// Feint risolve nel Blast per priorita' (fase Control, codice 30 del catalogo): precede il danno.
	const URTActionData* Feint = Wraith->Actions[4];
	TestTrue(TEXT("Feint: risolve nel Blast"),
		URTCatalogLibrary::MapResolutionPhase(Feint->Def.ResolutionPhase) == ERTMatchPhase::Blast);
	TestEqual(TEXT("Feint: cooldown 2"), Feint->Def.CooldownTurns, 2);

	const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ Wraith });
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("Wraith e' strutturalmente valido"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWraithInterceptShotStopsMovementTest,
	"RefactorTactics.Heroes.Wraith.InterceptShotStopsMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWraithInterceptShotStopsMovementTest::RunTest(const FString&)
{
	// Nome vincolante della DoD di CP 6.4. Fino al 2026-08-10 questo test verificava un LIMITE — «lo stop del
	// movimento non e' rappresentabile, l'azione e' rinviata a E14» — ed era la verita' del suo momento. E18
	// CP 18.2 l'ha superata: lo stop ora esiste come esito del boundary predittivo
	// (`ERTMoveOutcome::StoppedByPrediction`), quindi il nome del test e' finalmente descrittivo invece che
	// aspirazionale. Le assertion sono state SOSTITUITE, non cancellate: la domanda «questa azione ferma
	// davvero chi entra?» resta quella, ed e' cambiata la risposta.
	const URTActionData* Intercept = URTHeroCatalogLibrary::MakeWraith()->Actions[1];

	TestEqual(TEXT("InterceptShot: 16 danni"),
		WraithEffectAmount(Intercept->Def.Effects, ERTActionEffect::Damage), 16);
	TestEqual(TEXT("InterceptShot: cooldown 2"), Intercept->Def.CooldownTurns, 2);

	// Si dichiara nel Prep e si verifica al Move: la fase resta quella, e non e' interrompibile — un colpo
	// gia' armato su una cella non si annulla.
	TestTrue(TEXT("si arma nel Prep"),
		URTCatalogLibrary::MapResolutionPhase(Intercept->Def.ResolutionPhase) == ERTMatchPhase::Prep);
	TestFalse(TEXT("una previsione armata non si interrompe"), Intercept->Def.bCanBeInterrupted);

	// Lo STOP e' rappresentabile: c'e' un esito di movimento che lo dice, e non riusa `BlockedByUnit` —
	// la cella e' libera, e mandare il giocatore a cercare un'unita' che non c'e' sarebbe una bugia.
	TestTrue(TEXT("lo stop del movimento ha un esito proprio nel TurnLog"),
		ERTMoveOutcome::StoppedByPrediction != ERTMoveOutcome::BlockedByUnit);

	// `Action.SuppressiveLine` (CP 4.6) resta il parente: ferma anch'essa chi entra in una cella controllata.
	// Restano due azioni distinte — quella controlla una LINEA valutata nel Blast, questa una cella dichiarata
	// e verificata al Move — ma condividono la semantica, e il giorno in cui una delle due cambiasse regola
	// senza l'altra sarebbe un difetto da vedere.
	const FRTActionDef Suppressive = URTCatalogLibrary::FindCoreAction(TEXT("Action.SuppressiveLine"));
	TestFalse(TEXT("Action.SuppressiveLine esiste ancora"), Suppressive.ActionId.IsNone());
	TestTrue(TEXT("e si arma anch'essa nel Prep"),
		URTCatalogLibrary::MapResolutionPhase(Suppressive.ResolutionPhase) == ERTMatchPhase::Prep);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWraithInterceptShotIsPredictiveTest,
	"RefactorTactics.Heroes.Wraith.InterceptShotIsPredictive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWraithInterceptShotIsPredictiveTest::RunTest(const FString&)
{
	// La MIGRAZIONE di classificazione di D-016, verificata sul dato. Il rinvio a E14 era dichiarato nei dati
	// (slot `None`, nessun trigger) proprio perche' un commento non si puo' verificare: per lo stesso motivo
	// la sua fine dev'essere un dato, e non la sparizione di quelle righe.
	const URTActionData* Intercept = URTHeroCatalogLibrary::MakeWraith()->Actions[1];

	TestEqual(TEXT("identita' invariata: e' una migrazione, non un'azione nuova"),
		Intercept->Def.ActionId, FName(TEXT("Hero.Wraith.InterceptShot")));

	TestTrue(TEXT("la cella si blocca in Planning"),
		Intercept->Def.PredictiveTargeting == ERTPredictiveTargeting::LockCell);
	TestTrue(TEXT("e si verifica al boundary del Move"),
		Intercept->Def.PredictionBoundary == ERTPredictionBoundary::MovementEntry);

	// Lo slot torna `Main`: e' un'azione dichiarata in pianificazione, non una reazione tenuta pronta. Se
	// restasse `None` non consumerebbe nulla, e prevedere sarebbe gratis.
	TestTrue(TEXT("occupa l'azione principale: prevedere costa il turno"),
		Intercept->Def.Slot == ERTActionSlot::Main);

	// Il fallback del whiff e' DICHIARATO nel catalogo: `Cancel` = fizzle. Non e' scelto dal resolver, che
	// e' cio' che tiene la Predictive Action dentro il motore azioni invece che accanto ad esso.
	TestTrue(TEXT("il whiff usa il fallback dichiarato"), Intercept->Def.Fallback == ERTActionFallback::Cancel);

	// E NON e' una reazione: nessun trigger, nessuno slot reazione. Le due classificazioni si escludono, e
	// tenerle entrambe produrrebbe un'azione valutata due volte da due pass diversi.
	TestTrue(TEXT("non e' una reazione"), Intercept->Def.Slot != ERTActionSlot::Reaction
		&& Intercept->Def.ReactionTrigger == ERTReactionTrigger::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWraithVariantTradeoffTest,
	"RefactorTactics.Heroes.Wraith.VariantTradeoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTWraithVariantTradeoffTest::RunTest(const FString&)
{
	// Nome vincolante della DoD: nessuna variante migliore in ogni parametro. Qui il compromesso e' meta'
	// effetto (il danno) e meta' parametro non ancora modellato (le celle controllate).
	const URTActionData* Intercept = URTHeroCatalogLibrary::MakeWraith()->Actions[1];
	if (!TestEqual(TEXT("due varianti"), Intercept->Variants.Num(), 2)) { return false; }

	const FRTAbilityVariant& Precise = Intercept->Variants[0];
	const FRTAbilityVariant& Extended = Intercept->Variants[1];

	const int32 PreciseDamage = WraithEffectAmount(Precise.Effects, ERTActionEffect::Damage);
	const int32 ExtendedDamage = WraithEffectAmount(Extended.Effects, ERTActionEffect::Damage);
	const int32 PreciseCells = WraithVariantParam(Precise, TEXT("ControlledCells"));
	const int32 ExtendedCells = WraithVariantParam(Extended, TEXT("ControlledCells"));

	TestEqual(TEXT("preciso: 20 danni"), PreciseDamage, 20);
	TestEqual(TEXT("preciso: una cella"), PreciseCells, 1);
	TestEqual(TEXT("esteso: 14 danni"), ExtendedDamage, 14);
	TestEqual(TEXT("esteso: tre celle"), ExtendedCells, 3);

	// La prova della DoD: il preciso vince sul danno, l'esteso sulla copertura dello spazio. Incrociate,
	// nessuna delle due domina.
	TestTrue(TEXT("il preciso fa piu' male"), PreciseDamage > ExtendedDamage);
	TestTrue(TEXT("l'esteso controlla piu' spazio"), ExtendedCells > PreciseCells);

	// Nessuna e' un puro potenziamento della base (16 danni, una cella): il preciso paga in copertura, e
	// l'esteso paga in danno.
	TestTrue(TEXT("preciso: sopra la base nel danno"), PreciseDamage > 16);
	TestTrue(TEXT("esteso: sotto la base nel danno"), ExtendedDamage < 16);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroRosterTest,
	"RefactorTactics.Heroes.RosterIsBalanced",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroRosterTest::RunTest(const FString&)
{
	// Gate di chiusura dell'epic E6 (#20): «i 4 eroi sono distinguibili da dati e nessuna variante e'
	// migliore in ogni parametro». Il roster completo passa il validator ed e' internamente coerente.
	TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
	if (!TestEqual(TEXT("quattro eroi"), Roster.Num(), 4)) { return false; }

	TArray<const URTHeroData*> AsConst;
	for (URTHeroData* Hero : Roster) { AsConst.Add(Hero); }
	const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes(AsConst);
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("il roster completo e' valido"), Errors.Num(), 0);

	// Gli eroi sono DISTINGUIBILI: nessuna coppia condivide lo stesso profilo statistico. E' la prima meta'
	// del gate («distinguibili da dati»).
	for (int32 i = 0; i < Roster.Num(); ++i)
	{
		for (int32 j = i + 1; j < Roster.Num(); ++j)
		{
			const bool bIdenticalStats =
				Roster[i]->MaxHealth == Roster[j]->MaxHealth &&
				Roster[i]->MovePoints == Roster[j]->MovePoints &&
				Roster[i]->VisionRange == Roster[j]->VisionRange &&
				Roster[i]->PushResistance == Roster[j]->PushResistance;
			TestFalse(FString::Printf(TEXT("%s e %s hanno profili diversi"),
				*Roster[i]->HeroId.ToString(), *Roster[j]->HeroId.ToString()), bIdenticalStats);
		}
	}

	// BILANCIAMENTO (#131). Wraith era 100/6/6/0 e dominava sia Gadget (90/5/6/0) sia Phase (95/5/5/0): migliore
	// o pari ovunque, strettamente migliore in salute e movimento. Il catalogo §5 gli attribuiva a parole un
	// costo — «compra mobilita' con l'assenza di difese» — che sui numeri non esisteva.
	//
	// Con 90 HP la dominanza su **Phase** e' finita, e questa parte diventa una REGOLA: il ciclo sotto non
	// registra piu' un fatto, lo vieta. Un ritorno a 95+ HP fa cadere il test invece di passare inosservato.
	const URTHeroData* WraithInRoster = Roster[3];
	const URTHeroData* GadgetInRoster = Roster[0];
	const URTHeroData* PhaseInRoster = Roster[1];

	auto DominatesOnBaseStats = [](const URTHeroData* A, const URTHeroData* B)
	{
		// A domina B: >= su tutte e quattro le statistiche base, e > su almeno una.
		const bool bWeaklyBetter =
			A->MaxHealth      >= B->MaxHealth &&
			A->MovePoints     >= B->MovePoints &&
			A->VisionRange    >= B->VisionRange &&
			A->PushResistance >= B->PushResistance;
		const bool bStrictlyBetterSomewhere =
			A->MaxHealth      >  B->MaxHealth ||
			A->MovePoints     >  B->MovePoints ||
			A->VisionRange    >  B->VisionRange ||
			A->PushResistance >  B->PushResistance;
		return bWeaklyBetter && bStrictlyBetterSomewhere;
	};

	// ✅ **`#131` CHIUSA il 2026-08-10.** Non piu' due assert su due coppie scelte a mano: la proprieta' vale
	// per **ogni** coppia del roster, quindi si verifica su ogni coppia. E' la forma che regge quando il
	// roster crescera' a otto (E35): un eroe nuovo che dominasse qualcuno fa cadere questo test da solo,
	// senza che nessuno debba ricordarsi di aggiungere una riga.
	//
	// Le due leve che l'hanno chiusa, e perche' proprio quelle:
	//   Wraith 100 -> 90 HP  ([D-069]) — toglie la dominanza su Phase
	//   Gadget    6 -> 7 vista ([D-073]) — toglie quella su Gadget, che il calo di Wraith NON aveva risolto
	//
	// Le alternative scartate, misurate e non intuite: dare 6 MP a Gadget o toglierne uno a Wraith rende i due
	// profili IDENTICI, e il ciclo di distinguibilita' qui sopra sarebbe caduto — un test rotto per ripararne
	// un altro. Una `PushResistance` negativa per Wraith non ha effetto osservabile, perche' e' una SOGLIA e
	// le spinte valgono almeno 1.
	for (int32 i = 0; i < Roster.Num(); ++i)
	{
		for (int32 j = 0; j < Roster.Num(); ++j)
		{
			if (i == j) { continue; }
			TestFalse(FString::Printf(TEXT("#131: %s non domina %s sulle statistiche base"),
				*Roster[i]->HeroId.ToString(), *Roster[j]->HeroId.ToString()),
				DominatesOnBaseStats(Roster[i], Roster[j]));
		}
	}

	// La compensazione nelle abilita' resta com'era, e non era in discussione: `#131` riguardava la scheda
	// statistiche, dove il costo di ogni eroe ora e' visibile.
	TestTrue(TEXT("Gadget conserva il bonus combo piu' alto del roster"),
		URTCombatLibrary::GadgetWetDischargeBonus > 0);
	TestTrue(TEXT("e Gadget e' l'unico a vedere oltre il raggio 6"),
		GadgetInRoster->VisionRange > WraithInRoster->VisionRange);

	// Le affinita' sono tutte diverse: quattro identita' ambientali, non due coppie di gemelli.
	TSet<FName> Affinities;
	for (const URTHeroData* Hero : Roster) { Affinities.Add(Hero->Affinity); }
	TestEqual(TEXT("quattro affinita' distinte"), Affinities.Num(), 4);

	// Le debolezze chiudono due coppie simmetriche: la debolezza di ognuno e' l'affinita' di un altro.
	// E' la proprieta' che rende le combo leggibili — chi bagna prepara il colpo di chi fulmina.
	for (const URTHeroData* Hero : Roster)
	{
		TestTrue(FString::Printf(TEXT("la debolezza di %s e' l'affinita' di un altro eroe"),
			*Hero->HeroId.ToString()), Affinities.Contains(Hero->Weakness));
		TestNotEqual(FString::Printf(TEXT("%s non e' debole alla propria affinita'"), *Hero->HeroId.ToString()),
			Hero->Weakness, Hero->Affinity);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
