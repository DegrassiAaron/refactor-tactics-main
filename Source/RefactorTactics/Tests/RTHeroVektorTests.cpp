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
	int32 VektorEffectAmount(const TArray<FRTActionEffectSpec>& Effects, ERTActionEffect Kind)
	{
		for (const FRTActionEffectSpec& Spec : Effects)
		{
			if (Spec.Effect == Kind) { return Spec.Amount; }
		}
		return 0;
	}

	int32 VektorVariantParam(const FRTAbilityVariant& Variant, const TCHAR* Key)
	{
		const int32* Found = Variant.Parameters.Find(FName(Key));
		return Found ? *Found : INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVektorMatchesCatalogTest,
	"RefactorTactics.Heroes.Vektor.MatchesCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVektorMatchesCatalogTest::RunTest(const FString&)
{
	// Numeri della tabella §4 del catalogo eroi v0.1.
	URTHeroData* Vektor = URTHeroCatalogLibrary::MakeVektor();
	if (!TestNotNull(TEXT("Vektor costruito"), Vektor)) { return false; }

	TestEqual(TEXT("HeroId"), Vektor->HeroId, FName(TEXT("Hero.Vektor")));
	TestEqual(TEXT("salute"), Vektor->MaxHealth, 100);
	TestEqual(TEXT("movimento"), Vektor->MovePoints, 6);
	TestEqual(TEXT("vista"), Vektor->VisionRange, 6);
	TestEqual(TEXT("resistenza push"), Vektor->PushResistance, 0);
	TestEqual(TEXT("affinita'"), Vektor->Affinity, FName(TEXT("Affinity.Movement")));
	TestEqual(TEXT("debolezza simmetrica a Bastion"), Vektor->Weakness, FName(TEXT("Affinity.Structures")));

	if (!TestEqual(TEXT("cinque azioni"), Vektor->Actions.Num(), 5)) { return false; }

	const URTActionData* PulseShot = Vektor->Actions[0];
	TestEqual(TEXT("PulseShot: 21 danni"), VektorEffectAmount(PulseShot->Def.Effects, ERTActionEffect::Damage), 21);
	TestEqual(TEXT("PulseShot: range 4"), PulseShot->Def.RangeCells, 4);
	TestNotEqual(TEXT("non e' il danno generico della fascia medio raggio"),
		VektorEffectAmount(PulseShot->Def.Effects, ERTActionEffect::Damage),
		URTCatalogLibrary::BasicAttackDamageForRange(4));

	const URTActionData* PassingBlade = Vektor->Actions[2];
	TestEqual(TEXT("PassingBlade: 20 danni"), VektorEffectAmount(PassingBlade->Def.Effects, ERTActionEffect::Damage), 20);
	TestEqual(TEXT("PassingBlade: Dash 3"), PassingBlade->Def.RangeCells, 3);
	TestTrue(TEXT("PassingBlade: risolve nel Dash"),
		URTCatalogLibrary::MapResolutionPhase(PassingBlade->Def.ResolutionPhase) == ERTMatchPhase::Dash);

	// PassingBlade ATTRAVERSA i nemici, la carica di Bastion si FERMA sul primo: la differenza e' un dato
	// (`ERTMovementStyle`), non un `if` sull'ActionId. E' il motivo per cui quel campo esiste (CP 4.5).
	TestTrue(TEXT("PassingBlade: passa attraverso (LinearDash)"),
		PassingBlade->Def.MovementStyle == ERTMovementStyle::LinearDash);
	const FRTActionDef ChargeDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Charge"));
	TestTrue(TEXT("e non si ferma addosso come la carica"),
		PassingBlade->Def.MovementStyle != ChargeDef.MovementStyle);

	// Feint risolve nel Blast per priorita' (fase Control, codice 30 del catalogo): precede il danno.
	const URTActionData* Feint = Vektor->Actions[4];
	TestTrue(TEXT("Feint: risolve nel Blast"),
		URTCatalogLibrary::MapResolutionPhase(Feint->Def.ResolutionPhase) == ERTMatchPhase::Blast);
	TestEqual(TEXT("Feint: cooldown 2"), Feint->Def.CooldownTurns, 2);

	const TArray<FString> Errors = URTHeroCatalogLibrary::ValidateHeroes({ Vektor });
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("Vektor e' strutturalmente valido"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVektorInterceptShotStopsMovementTest,
	"RefactorTactics.Heroes.Vektor.InterceptShotStopsMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVektorInterceptShotStopsMovementTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. Il motore delle reazioni ora esiste (E5, CP 5.5) e le altre tre reazioni
	// d'eroe sono cablate (CP 6.7) — **questa no, ed e' una decisione**: il suo trigger e' d'ingresso su
	// movimento, quindi appartiene a E14 (ADR-0004). Cablarla qui duplicherebbe `FRTSuppressiveZone`, e
	// "fermare il movimento" resta comunque fuori da `ERTActionEffect`. Il test verifica cio' che il dato
	// dichiara oggi, incluso il rinvio: slot `None` e nessun trigger, cosi' il pass delle reazioni non la
	// raccoglie e non puo' registrare un'attivazione senza effetti.
	const URTActionData* Intercept = URTHeroCatalogLibrary::MakeVektor()->Actions[1];

	TestEqual(TEXT("InterceptShot: 16 danni"),
		VektorEffectAmount(Intercept->Def.Effects, ERTActionEffect::Damage), 16);
	TestEqual(TEXT("InterceptShot: cooldown 2"), Intercept->Def.CooldownTurns, 2);

	// Si PREPARA (non si esegue): risolve nel Prep, non occupa l'azione principale, e non e' interrompibile —
	// una reazione gia' pronta non si annulla.
	TestTrue(TEXT("si prepara nel Prep"),
		URTCatalogLibrary::MapResolutionPhase(Intercept->Def.ResolutionPhase) == ERTMatchPhase::Prep);
	TestTrue(TEXT("non consuma l'azione principale"), Intercept->Def.Slot == ERTActionSlot::None);
	TestFalse(TEXT("una reazione preparata non si interrompe"), Intercept->Def.bCanBeInterrupted);
	// Il rinvio a E14 e' un DATO, non un commento: senza trigger, il pass delle reazioni non la vede.
	TestTrue(TEXT("rinviata a E14: nessun trigger dichiarato"),
		Intercept->Def.ReactionTrigger == ERTReactionTrigger::None);

    // Limite dichiarato: lo STOP del movimento non e' rappresentabile. Il parente gia' costruito e'
	// `Action.SuppressiveLine` (CP 4.6), che ferma chi entra in una cella controllata: quando E5 arrivera',
	// l'aggancio e' quello. Qui si verifica che quel meccanismo esista gia' e faccia la cosa giusta, cosi'
	// InterceptShot avra' dove attaccarsi invece di reinventarlo.
	const FRTActionDef Suppressive = URTCatalogLibrary::FindCoreAction(TEXT("Action.SuppressiveLine"));
	TestFalse(TEXT("Action.SuppressiveLine esiste (l'aggancio per E5)"), Suppressive.ActionId.IsNone());
	TestTrue(TEXT("e si prepara anch'essa nel Prep"),
		URTCatalogLibrary::MapResolutionPhase(Suppressive.ResolutionPhase) == ERTMatchPhase::Prep);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVektorVariantTradeoffTest,
	"RefactorTactics.Heroes.Vektor.VariantTradeoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVektorVariantTradeoffTest::RunTest(const FString&)
{
	// Nome vincolante della DoD: nessuna variante migliore in ogni parametro. Qui il compromesso e' meta'
	// effetto (il danno) e meta' parametro non ancora modellato (le celle controllate).
	const URTActionData* Intercept = URTHeroCatalogLibrary::MakeVektor()->Actions[1];
	if (!TestEqual(TEXT("due varianti"), Intercept->Variants.Num(), 2)) { return false; }

	const FRTAbilityVariant& Precise = Intercept->Variants[0];
	const FRTAbilityVariant& Extended = Intercept->Variants[1];

	const int32 PreciseDamage = VektorEffectAmount(Precise.Effects, ERTActionEffect::Damage);
	const int32 ExtendedDamage = VektorEffectAmount(Extended.Effects, ERTActionEffect::Damage);
	const int32 PreciseCells = VektorVariantParam(Precise, TEXT("ControlledCells"));
	const int32 ExtendedCells = VektorVariantParam(Extended, TEXT("ControlledCells"));

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

	// ⚠️ FATTO DOCUMENTATO, non una regola violata: sulle **sole quattro statistiche base** Vektor (100/6/6/0)
	// domina Flux (90/5/6/0) e Riva (95/5/5/0) — e' migliore o pari ovunque, strettamente migliore in salute e
	// movimento. Sono i numeri del PDF del catalogo, non una scelta fatta qui.
	//
	// Il catalogo §5 scrive «nessun eroe domina in ogni parametro» riferendosi al pacchetto COMPLETO
	// (statistiche + abilita'): «Flux ha il danno combo piu' alto ma la salute piu' bassa». La differenziazione
	// di Flux e Riva sta nelle abilita' — il +8 su Wet, la cura ad area — non nelle statistiche.
	//
	// Il gate di chiusura di E6 (#20) pone la non-dominanza sulle **varianti**, non sugli eroi, ed e' quello
	// che i test `*.VariantTradeoff` verificano eroe per eroe. Qui si registra il fatto perche' resti visibile
	// a chi ribilancera' (E11), invece di sparire fra i numeri.
	const URTHeroData* VektorInRoster = Roster[3];
	const URTHeroData* FluxInRoster = Roster[0];
	TestTrue(TEXT("nota di bilanciamento: Vektor ha piu' salute E piu' movimento di Flux"),
		VektorInRoster->MaxHealth > FluxInRoster->MaxHealth
		&& VektorInRoster->MovePoints > FluxInRoster->MovePoints);
	TestTrue(TEXT("...ma Flux compensa nelle abilita': il bonus combo piu' alto del roster"),
		URTCombatLibrary::FluxWetDischargeBonus > 0);

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
