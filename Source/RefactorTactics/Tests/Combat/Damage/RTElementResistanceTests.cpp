#include "Misc/AutomationTest.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapAsset.h"
#include "Tests/Shared/RTWorldFixtures.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "UI/HUD/RTHudViewModel.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * #4189, fetta 1 di #4188 - il colpo ELEMENTALE si mitiga con la RES dell'elemento, non con la DEF ([D-634] (4) e (6)).
 *
 * `RTActionElementTests.cpp` prova il tragitto dell'elemento dal `Def` al colpo; qui si prova che cosa il colpo ne fa:
 * `MitigatedHitPower` sceglie la difesa con `FRTHexCombatUnit::DefenseAgainst` (la DEF per `NAME_None`, altrimenti la
 * voce dell'elemento in `ElementResistance` o `ResistanceDefault`), con la formula di sempre, `MitigateDirectDamage`, e
 * un solo arrotondamento ([D-595] (1)).
 *
 * Il bersaglio di prova e' quello del piano: DEF 45, RES 35 (Riktor nel catalogo) e una RES di 10 per `Element.Fire`,
 * che nessun eroe di oggi dichiara e che qui si inietta. Contro di lui un colpo da 240 vale:
 *   - senza elemento   floor(240 x 100/145) = 165 (la DEF);
 *   - `Element.Electric` floor(240 x 100/135) = 177 (la RES di default: l'elemento non ha una voce sua);
 *   - `Element.Fire`     floor(240 x 100/110) = 218 (la voce dell'elemento).
 *
 * Accanto alla raccolta si provano gli altri punti che rileggono la difesa del colpo: `ApplyExposedToFirstHit` (il
 * `Mod` 120 si ricalcola con la stessa difesa), `RedirectHitTo` (la RES e' dell'intercettore), il breakdown di
 * `ToAttacks` (la voce `Defense` dice quale statistica ha letto), e i due punti che copiano la RES da un `ARTUnit` a un
 * `FRTHexCombatUnit`: lo snapshot del Blast (con un turno vero) e le unita' dell'anteprima dell'HUD.
 */

namespace
{
	/** Nomi distinti per file: la unity build condivide la translation unit. */
	FName ResElectric() { return FName(TEXT("Element.Electric")); }
	FName ResFire() { return FName(TEXT("Element.Fire")); }

	constexpr int32 ResTargetDefense = 45;
	constexpr int32 ResTargetResistance = 35;
	constexpr int32 ResTargetFireResistance = 10;

	URTHexMapAsset* MakeResMap(int32 Radius)
	{
		return URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
	}

	FRTHexCombatUnit ResUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		U.Facing = Facing;
		return U;
	}

	/** Il bersaglio di prova: DEF 45, RES 35, RES 10 contro il fuoco. */
	FRTHexCombatUnit ResTarget(int32 UnitId, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing)
	{
		FRTHexCombatUnit U = ResUnit(UnitId, TeamId, Cell, Facing);
		U.Defense = ResTargetDefense;
		U.ResistanceDefault = ResTargetResistance;
		U.ElementResistance.Add(ResFire(), ResTargetFireResistance);
		return U;
	}

	/** Un colpo singolo a portata 5, mirato sulla cella del bersaglio ([D-415]). */
	FRTHexAttackIntent ResIntent(const FRTCellId& AimCell, int32 AttackerId, int32 TargetId, int32 Power, const FName& ElementId)
	{
		FRTHexAttackIntent I;
		I.AttackerId = AttackerId;
		I.TargetId = TargetId;
		I.TargetCell = AimCell;
		I.Shape = ERTAbilityShape::Single;
		I.RangeCells = 5;
		I.Power = Power;
		I.bCountsAsAttack = true;
		I.ElementId = ElementId;
		return I;
	}

	const FRTHexAttackHit* ResHitOn(const TArray<FRTHexAttackHit>& Hits, int32 TargetId)
	{
		for (const FRTHexAttackHit& Hit : Hits)
		{
			if (Hit.TargetId == TargetId) { return &Hit; }
		}
		return nullptr;
	}

	TArray<int32> ResIdentityOrder(const FRTHexBlastPlan& Plan)
	{
		TArray<int32> Order;
		for (int32 h = 0; h < Plan.Hits.Num(); ++h) { Order.Add(h); }
		return Order;
	}

	ARTUnit* ResSpawnUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World || !Hero) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // il piano lo scriviamo noi
		U->ConfigureFromHeroData(Hero); // DEF e RES dal catalogo
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->SetPlannedCell(Cell); // fermo: qui conta il Blast, non il movimento
		return U;
	}

	/** Il colpo che `CollectHexAttacks` ha raccolto in un turno vero, e gli HP che il bersaglio ha perso. */
	struct FResTurnOutcome
	{
		int32 Hits = 0;
		int32 HitPower = -1;
		int32 DefenseApplied = -1;
		FName HitElement;
		int32 HpLost = -1;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementResistanceMitigatesTheElementalHitTest,
	"RefactorTactics.Damage.Element.ResistanceMitigatesTheElementalHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementResistanceMitigatesTheElementalHitTest::RunTest(const FString&)
{
	// La scelta da sola: DEF per il colpo diretto, la voce dell'elemento se c'e', altrimenti la RES di default.
	{
		const FRTHexCombatUnit T = ResTarget(1, 1, FRTCellId(0, 0, 0), ERTHexDirection::W);
		TestEqual(TEXT("DefenseAgainst(None) = la DEF"), T.DefenseAgainst(NAME_None), ResTargetDefense);
		TestEqual(TEXT("DefenseAgainst(Electric) = la RES di default"), T.DefenseAgainst(ResElectric()), ResTargetResistance);
		TestEqual(TEXT("DefenseAgainst(Fire) = la RES del fuoco"), T.DefenseAgainst(ResFire()), ResTargetFireResistance);
	}

	// Il colpo: attaccante a ovest, bersaglio che lo guarda (frontale: nessun bonus alle spalle), arena piena (nessuna
	// copertura). Un piano per elemento, stesso colpo da 240.
	struct FCase { const TCHAR* Label; FName ElementId; int32 Defense; int32 Expected; };
	const TArray<FCase> Cases = {
		{ TEXT("senza elemento: floor(240 x 100/145)"), NAME_None, ResTargetDefense, 165 },
		{ TEXT("Element.Electric: floor(240 x 100/135)"), ResElectric(), ResTargetResistance, 177 },
		{ TEXT("Element.Fire: floor(240 x 100/110)"), ResFire(), ResTargetFireResistance, 218 },
	};
	const FRTCellId Bersaglio(0, 0, 0);
	for (const FCase& C : Cases)
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(ResUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E));
		Units.Add(ResTarget(1, 1, Bersaglio, ERTHexDirection::W));
		TArray<FRTHexAttackIntent> Intents;
		Intents.Add(ResIntent(Bersaglio, 0, 1, 240, C.ElementId));

		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeResMap(3));
		const FRTHexAttackHit* Hit = ResHitOn(Plan.Hits, 1);
		if (!TestNotNull(*FString::Printf(TEXT("%s: il colpo c'e'"), C.Label), Hit)) { continue; }
		TestEqual(*FString::Printf(TEXT("%s = %d"), C.Label, C.Expected), Hit->Power, C.Expected);
		TestEqual(*FString::Printf(TEXT("%s: il mitigato e' lo stesso (nessuna copertura)"), C.Label), Hit->MitigatedPower, C.Expected);
		TestEqual(*FString::Printf(TEXT("%s: la difesa letta e' %d"), C.Label, C.Defense), Hit->DefenseApplied, C.Defense);
		TestEqual(*FString::Printf(TEXT("%s: e' la formula di sempre, con quella difesa"), C.Label), Hit->Power,
			URTCombatLibrary::MitigateDirectDamage(240, 0, 100, C.Defense));
		TestEqual(*FString::Printf(TEXT("%s: frontale, nessun bonus"), C.Label), Hit->RearBonusApplied, 0);
		TestEqual(*FString::Printf(TEXT("%s: il colpo porta il suo elemento"), C.Label), Hit->ElementId, C.ElementId);
	}
	return true;
}

/**
 * Review Focus 1 del piano: un'azione di oggi, senza elemento, contro lo stesso bersaglio da' lo stesso danno di prima.
 * Gadget tira `Hero.Gadget.ArcPulse` (220) a Riktor (DEF 45, RES 35 nel catalogo, piu' la RES 10 contro il fuoco
 * iniettata), di fronte e senza copertura, in un turno vero: il colpo vale floor(220 x 100/145) = 151, e gli HP persi
 * 151 meno lo scudo base.
 *
 * Il controllo e' la stessa azione con l'elemento scritto sul suo `Def`: `Element.Electric` vale floor(220 x 100/135) =
 * 162, `Element.Fire` floor(220 x 100/110) = 200. E' cio' che prova lo snapshot del Blast (`RTTurnManager_Blast.cpp`):
 * se la RES non arrivasse da `ARTUnit` al `FRTHexCombatUnit`, il colpo elementale passerebbe intero (220).
 * ⚠️ Si legge il colpo RACCOLTO (`bRecordBlastPlanForTest`), non gli HP, per i due elementali: lo scudo base sul solo
 * `Direct` (fetta 1, passo dello scudo) cambia gli HP di un colpo elementale, non il colpo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementTodaysActionKeepsTheDefenseInATurnTest,
	"RefactorTactics.Damage.Element.TodaysActionKeepsTheDefenseInATurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementTodaysActionKeepsTheDefenseInATurnTest::RunTest(const FString&)
{
	const auto Turno = [this](bool bOverrideElement, const FName& ElementId, FResTurnOutcome& Out) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = MakeResMap(6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		ARTUnit* Gadget = ResSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeGadget(), FRTCellId(-1, 0, 0));
		ARTUnit* Riktor = ResSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
		if (!TestNotNull(TEXT("mappa"), MapActor) || !TestNotNull(TEXT("turn manager"), TM)
			|| !TestNotNull(TEXT("Gadget"), Gadget) || !TestNotNull(TEXT("Riktor"), Riktor)
			|| !TestNotNull(TEXT("l'attacco base di Gadget"), Gadget->GetAbility(0)))
		{
			return false;
		}

		// Premesse sul catalogo: se si muovono, i numeri qui sotto sono da riscrivere, non da ignorare.
		URTActionData* ArcPulse = Gadget->GetAbility(0);
		if (!TestEqual(TEXT("premessa: l'azione 0 di Gadget e' ArcPulse"), ArcPulse->Def.ActionId, FName(TEXT("Hero.Gadget.ArcPulse")))
			|| !TestEqual(TEXT("premessa: ArcPulse vale 220"), URTCatalogLibrary::FirstDamage(ArcPulse->Def), 220)
			|| !TestEqual(TEXT("premessa: ArcPulse e' senza elemento nel catalogo"), ArcPulse->Def.ElementId, FName(NAME_None))
			|| !TestEqual(TEXT("premessa: Riktor ha DEF 45"), Riktor->Defense, ResTargetDefense)
			|| !TestEqual(TEXT("premessa: Riktor ha RES 35"), Riktor->ResistanceDefault, ResTargetResistance)
			|| !TestEqual(TEXT("premessa: Riktor non dichiara RES per elemento"), Riktor->ElementResistance.Num(), 0))
		{
			return false;
		}

		Riktor->ElementResistance.Add(ResFire(), ResTargetFireResistance); // INIETTATA: nessun eroe di oggi la dichiara
		if (bOverrideElement)
		{
			ArcPulse->Def.ElementId = ElementId;
		}
		Riktor->Facing = ERTHexDirection::W; // guarda Gadget: frontale, nessun bonus alle spalle
		Riktor->ClearPlannedMainAction();
		Gadget->Facing = ERTHexDirection::E;
		Gadget->SetPlannedMainAction(0);
		Gadget->DeclareAttackOnUnit(Riktor);

		TM->bRecordBlastPlanForTest = true;
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}

		const TArray<FRTHexAttackHit>& Hits = TM->RecordedBlastHitsForTest();
		Out.Hits = Hits.Num();
		if (Hits.Num() == 1)
		{
			Out.HitPower = Hits[0].Power;
			Out.DefenseApplied = Hits[0].DefenseApplied;
			Out.HitElement = Hits[0].ElementId;
		}
		Out.HpLost = Riktor->MaxHealth - Riktor->Health;
		return true;
	};

	// L'azione di oggi, com'e' nel catalogo.
	FResTurnOutcome Oggi;
	if (!Turno(/*bOverrideElement*/ false, NAME_None, Oggi)) { return false; }
	if (!TestEqual(TEXT("premessa: un colpo"), Oggi.Hits, 1)) { return false; }
	TestEqual(TEXT("ArcPulse contro Riktor: floor(220 x 100/145) = 151, come prima"), Oggi.HitPower, 151);
	TestEqual(TEXT("e' la formula con la DEF"), Oggi.HitPower, URTCombatLibrary::MitigateDirectDamage(220, 0, 100, ResTargetDefense));
	TestEqual(TEXT("la difesa letta e' la DEF"), Oggi.DefenseApplied, ResTargetDefense);
	TestEqual(TEXT("il colpo e' senza elemento"), Oggi.HitElement, FName(NAME_None));
	TestEqual(TEXT("HP persi: 151 meno lo scudo base"), Oggi.HpLost, 151 - URTCombatLibrary::BaseShield);

	// Il controllo: la stessa azione con un elemento legge la RES, e la RES e' arrivata nello snapshot.
	FResTurnOutcome Elettrico, Fuoco;
	if (!Turno(/*bOverrideElement*/ true, ResElectric(), Elettrico) || !Turno(/*bOverrideElement*/ true, ResFire(), Fuoco))
	{
		return false;
	}
	if (TestEqual(TEXT("controllo Electric: un colpo"), Elettrico.Hits, 1))
	{
		TestEqual(TEXT("Electric: floor(220 x 100/135) = 162"), Elettrico.HitPower, 162);
		TestEqual(TEXT("Electric: la difesa letta e' la RES di default"), Elettrico.DefenseApplied, ResTargetResistance);
	}
	if (TestEqual(TEXT("controllo Fire: un colpo"), Fuoco.Hits, 1))
	{
		TestEqual(TEXT("Fire: floor(220 x 100/110) = 200"), Fuoco.HitPower, 200);
		TestEqual(TEXT("Fire: la difesa letta e' la RES del fuoco"), Fuoco.DefenseApplied, ResTargetFireResistance);
	}
	return true;
}

/**
 * `Status.Exposed` ricalcola il primo colpo con `Mod` 120 ([D-595] (2)) partendo dalla difesa che il colpo ha
 * registrato: per un colpo elementale e' la RES. Due bersagli identici ed esposti, uno colpito col fulmine e uno senza
 * elemento, 240 ciascuno: floor(240 x 120 x 100/(100 x 135)) = 213 il primo, floor(240 x 120 x 100/(100 x 145)) = 198
 * il secondo. Se il ricalcolo rileggesse la DEF, il colpo elettrico raccolto a 177 scenderebbe a 198 + (177 - 165).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementExposedRecomputesWithTheResistanceTest,
	"RefactorTactics.Damage.Element.ExposedRecomputesWithTheResistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementExposedRecomputesWithTheResistanceTest::RunTest(const FString&)
{
	const FRTCellId DifElettrico(0, 0, 0), DifDiretto(0, 3, 0);
	TArray<FRTHexCombatUnit> Units;
	Units.Add(ResUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E));
	Units.Add(ResTarget(1, 1, DifElettrico, ERTHexDirection::W));
	Units.Add(ResUnit(2, 0, FRTCellId(-1, 3, 0), ERTHexDirection::E));
	Units.Add(ResTarget(3, 1, DifDiretto, ERTHexDirection::W));
	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(ResIntent(DifElettrico, 0, 1, 240, ResElectric()));
	Intents.Add(ResIntent(DifDiretto, 2, 3, 240, NAME_None));

	FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeResMap(4));
	if (!TestEqual(TEXT("due colpi raccolti"), Plan.Hits.Num(), 2)) { return false; }
	const FRTHexAttackHit* E = ResHitOn(Plan.Hits, 1);
	const FRTHexAttackHit* D = ResHitOn(Plan.Hits, 3);
	if (!TestNotNull(TEXT("il colpo elettrico"), E) || !TestNotNull(TEXT("il colpo diretto"), D)) { return false; }
	TestEqual(TEXT("premessa: elettrico raccolto con la RES, 177"), E->Power, 177);
	TestEqual(TEXT("premessa: diretto raccolto con la DEF, 165"), D->Power, 165);

	const TArray<bool> Esposto = { false, true, false, true };
	const TArray<int32> Moltiplicati = URTHexCombatLibrary::ApplyExposedToFirstHit(Plan.Hits, ResIdentityOrder(Plan), Esposto);
	TestEqual(TEXT("due colpi moltiplicati"), Moltiplicati.Num(), 2);
	E = ResHitOn(Plan.Hits, 1);
	D = ResHitOn(Plan.Hits, 3);
	TestEqual(TEXT("elettrico Exposed contro RES 35: 213"), E->Power, 213);
	TestEqual(TEXT("elettrico: e' la formula con la RES"), E->Power,
		URTCombatLibrary::MitigateDirectDamage(240, 0, URTCombatLibrary::ExposedModPercent, ResTargetResistance));
	TestEqual(TEXT("elettrico: la difesa letta resta la RES"), E->DefenseApplied, ResTargetResistance);
	TestEqual(TEXT("diretto Exposed contro DEF 45: 198"), D->Power, 198);
	TestEqual(TEXT("diretto: la difesa letta resta la DEF"), D->DefenseApplied, ResTargetDefense);
	return true;
}

/**
 * `Action.Intercept` riassegna il colpo ([D-017]): la difesa e' TARGET-DEPENDENT, e per un colpo elementale e' la RES
 * dell'intercettore. Bersaglio originale senza difese a (2,0), intercettore in mezzo a (1,0) col profilo di prova; un
 * colpo di fuoco da 240 nasce intero sul primo e arriva all'intercettore a floor(240 x 100/110) = 218.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementInterceptedHitUsesTheInterceptorsResistanceTest,
	"RefactorTactics.Damage.Element.InterceptedHitUsesTheInterceptorsResistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementInterceptedHitUsesTheInterceptorsResistanceTest::RunTest(const FString&)
{
	TArray<FRTHexCombatUnit> Units;
	Units.Add(ResUnit(0, 0, FRTCellId(0, 0, 0), ERTHexDirection::E));
	Units.Add(ResUnit(1, 1, FRTCellId(2, 0, 0), ERTHexDirection::W));
	Units.Add(ResTarget(2, 1, FRTCellId(1, 0, 0), ERTHexDirection::W));
	URTHexMapAsset* Map = MakeResMap(3);

	struct FCase { const TCHAR* Label; FName ElementId; int32 Defense; int32 Expected; };
	const TArray<FCase> Cases = {
		{ TEXT("fuoco"), ResFire(), ResTargetFireResistance, 218 },
		{ TEXT("controllo senza elemento"), NAME_None, ResTargetDefense, 165 },
	};
	for (const FCase& C : Cases)
	{
		TArray<FRTHexAttackIntent> Intents;
		Intents.Add(ResIntent(FRTCellId(2, 0, 0), 0, 1, 240, C.ElementId));
		const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);
		const FRTHexAttackHit* Original = ResHitOn(Plan.Hits, 1);
		if (!TestNotNull(*FString::Printf(TEXT("%s: premessa, il colpo sul bersaglio originale esiste"), C.Label), Original)) { continue; }
		TestEqual(*FString::Printf(TEXT("%s: premessa, senza difese il colpo e' pieno"), C.Label), Original->Power, 240);

		const FRTHexAttackHit Redirected = URTHexCombatLibrary::RedirectHitTo(2, *Original, Units, Intents, Map);
		TestEqual(*FString::Printf(TEXT("%s: riassegnato all'intercettore"), C.Label), Redirected.TargetId, 2);
		TestEqual(*FString::Printf(TEXT("%s: con la difesa dell'intercettore, %d"), C.Label, C.Expected), Redirected.Power, C.Expected);
		TestEqual(*FString::Printf(TEXT("%s: la difesa letta e' la sua, %d"), C.Label, C.Defense), Redirected.DefenseApplied, C.Defense);
		TestEqual(*FString::Printf(TEXT("%s: il mitigato e' coerente"), C.Label), Redirected.MitigatedPower, C.Expected);
		TestEqual(*FString::Printf(TEXT("%s: l'elemento resta quello del colpo"), C.Label), Redirected.ElementId, C.ElementId);
	}
	return true;
}

/**
 * Il breakdown di `ToAttacks` registra la difesa che il colpo ha letto: per un colpo elementale la voce `Defense` porta
 * la RES come operando e `ResistanceSource` come provenienza; per uno diretto la DEF e `DefenseSource`, come prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementBreakdownNamesTheResistanceUsedTest,
	"RefactorTactics.Damage.Element.BreakdownNamesTheResistanceUsed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementBreakdownNamesTheResistanceUsedTest::RunTest(const FString&)
{
	TestNotEqual(TEXT("le due provenienze sono distinte"), URTCombatLibrary::ResistanceSource, URTCombatLibrary::DefenseSource);

	struct FCase { const TCHAR* Label; FName ElementId; int32 Defense; int32 Expected; FName Source; };
	const TArray<FCase> Cases = {
		{ TEXT("Element.Electric"), ResElectric(), ResTargetResistance, 177, URTCombatLibrary::ResistanceSource },
		{ TEXT("senza elemento"), NAME_None, ResTargetDefense, 165, URTCombatLibrary::DefenseSource },
	};
	const FRTCellId Bersaglio(0, 0, 0);
	for (const FCase& C : Cases)
	{
		TArray<FRTHexCombatUnit> Units;
		Units.Add(ResUnit(0, 0, FRTCellId(-1, 0, 0), ERTHexDirection::E));
		Units.Add(ResTarget(1, 1, Bersaglio, ERTHexDirection::W));
		TArray<FRTHexAttackIntent> Intents;
		Intents.Add(ResIntent(Bersaglio, 0, 1, 240, C.ElementId));

		const TArray<FRTAttack> Attacks = URTHexCombatLibrary::ToAttacks(URTHexCombatLibrary::CollectHexAttacks(Units, Intents, MakeResMap(3)));
		if (!TestEqual(*FString::Printf(TEXT("%s: un attacco"), C.Label), Attacks.Num(), 1)) { continue; }
		const TArray<FRTDamageStageEntry>& S = Attacks[0].Breakdown;
		if (!TestEqual(*FString::Printf(TEXT("%s: due stadi, catalogo e difesa"), C.Label), S.Num(), 2)) { continue; }
		TestTrue(*FString::Printf(TEXT("%s: [1] difesa"), C.Label), S[1].Stage == ERTDamageStage::Defense);
		TestEqual(*FString::Printf(TEXT("%s: [1] operando %d"), C.Label, C.Defense), S[1].Operand, C.Defense);
		TestEqual(*FString::Printf(TEXT("%s: [1] 240 -> %d"), C.Label, C.Expected), S[1].After, C.Expected);
		TestEqual(*FString::Printf(TEXT("%s: [1] provenienza"), C.Label), S[1].SourceId, C.Source);
		TestEqual(*FString::Printf(TEXT("%s: il registro finisce sul colpo"), C.Label), S[1].After, Attacks[0].Power);
	}
	return true;
}

/**
 * Le unita' dell'anteprima dell'HUD (`URTHudViewModel::MakeHexCombatUnits`) copiano DEF e bonus alle spalle dal Blast
 * per mostrare cio' che il resolver calcolera'; con la RES nel colpo copiano anche lei. Riktor dal catalogo con una RES
 * contro il fuoco iniettata: l'unita' dell'anteprima risponde come quella del Blast.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementPreviewUnitsCarryTheResistanceTest,
	"RefactorTactics.Damage.Element.PreviewUnitsCarryTheResistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementPreviewUnitsCarryTheResistanceTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTUnit* Riktor = ResSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
	if (!TestNotNull(TEXT("Riktor"), Riktor)) { return false; }
	Riktor->ElementResistance.Add(ResFire(), ResTargetFireResistance);

	const TArray<FRTHexCombatUnit> HexUnits = URTHudViewModel::MakeHexCombatUnits({ Riktor });
	if (!TestEqual(TEXT("un'unita'"), HexUnits.Num(), 1)) { return false; }
	const FRTHexCombatUnit& U = HexUnits[0];
	TestEqual(TEXT("la DEF"), U.Defense, Riktor->Defense);
	TestEqual(TEXT("la RES di default"), U.ResistanceDefault, Riktor->ResistanceDefault);
	TestEqual(TEXT("contro il fuoco, la RES del fuoco"), U.DefenseAgainst(ResFire()), Riktor->ResistanceAgainst(ResFire()));
	TestEqual(TEXT("contro il fulmine, la RES di default"), U.DefenseAgainst(ResElectric()), Riktor->ResistanceAgainst(ResElectric()));
	TestEqual(TEXT("premessa: il fuoco ha davvero una voce sua"), Riktor->ResistanceAgainst(ResFire()), ResTargetFireResistance);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
