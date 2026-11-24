#include "Misc/AutomationTest.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapAsset.h"
#include "Tests/Shared/RTAbilityFixtures.h"
#include "Tests/Shared/RTDamagePathFixtures.h"
#include "Tests/Shared/RTWorldFixtures.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * #4189, fetta 1 di #4188 — l'ELEMENTO dell'azione, dal catalogo al colpo ([D-634] (4)).
 *
 * Un'azione dichiara il proprio elemento (`FRTActionDef::ElementId`, `NAME_None` = `Direct`), il validatore del catalogo
 * rifiuta un identificatore fuori da `URTHeroCatalogLibrary::ElementIds()`, e l'intento e il colpo del Blast
 * lo portano. Questi test fissano solo il TRAGITTO del dato, che e' la premessa della regola; che cosa il colpo ne fa
 * (la RES dell'elemento al posto della DEF) lo provano i test di `RTElementResistanceTests.cpp`.
 *
 * ⚠️ **Perche' un turno vero, e non soltanto `CollectHexAttacks`**: l'intento nasce in due punti del `TurnManager`
 * (`CollectAttackIntents` e l'impatto di una carica), e ogni campo dell'intento che viene dal `Def` e' stato
 * dimenticato almeno una volta in uno dei due (lo raccontano i commenti accanto a `bFriendlyFire`, `bCountsAsAttack`,
 * `LineOfSightPolicy` e `MinRangeCells`). Il piano puro prova il secondo tratto, `intento -> colpo`; il primo si vede
 * solo da un turno, con `ARTTurnManager::bRecordBlastPlanForTest`.
 */

namespace
{
	/** `Element.Electric`: l'elemento di prova. Una funzione e non una costante globale, per non costruire un `FName` all'avvio. */
	FName ElemElectric() { return FName(TEXT("Element.Electric")); }

	/** Una riga del catalogo valida in ogni altro campo: cambia solo l'elemento. */
	FRTActionDef ElemValidDef(const TCHAR* ActionId, const FName& ElementId)
	{
		FRTActionDef Def;
		Def.ActionId = FName(ActionId);
		Def.ResolutionPhase = ERTResolutionPhase::Attack;
		Def.Priority = 50;
		Def.ActionPointCost = URTCatalogLibrary::ActionPointCostBlast;
		Def.bCountsAsAttack = true;
		Def.ElementId = ElementId;
		return Def;
	}

	FRTHexCombatUnit ElemUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		return U;
	}

	/** Un colpo singolo a portata 5, mirato sulla cella del bersaglio ([D-415]). */
	FRTHexAttackIntent ElemIntent(int32 AttackerId, int32 TargetId, const FRTCellId& AimCell, const FName& ElementId)
	{
		FRTHexAttackIntent I;
		I.AttackerId = AttackerId;
		I.TargetId = TargetId;
		I.TargetCell = AimCell;
		I.Shape = ERTAbilityShape::Single;
		I.RangeCells = 5;
		I.Power = 100;
		I.bCountsAsAttack = true;
		I.ElementId = ElementId;
		return I;
	}

	const FRTHexAttackHit* ElemHitOfIntent(const TArray<FRTHexAttackHit>& Hits, int32 IntentIndex)
	{
		for (const FRTHexAttackHit& Hit : Hits)
		{
			if (Hit.IntentIndex == IntentIndex) { return &Hit; }
		}
		return nullptr;
	}

	ARTUnit* ElemSpawnUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World || !Hero) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // il piano lo scriviamo noi
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->SetPlannedCell(Cell); // fermo: qui conta il Blast, non il movimento
		return U;
	}

	void ElemRunTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Gli elementi dell'intento e del colpo che una riga produce in un turno vero. */
	struct FElemTurnOutcome
	{
		int32 Intents = 0;
		int32 Hits = 0;
		FName IntentElement;
		FName HitElement;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementValidatorAcceptsOnlyKnownElementsTest,
	"RefactorTactics.Damage.Element.ValidatorAcceptsOnlyKnownElements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementValidatorAcceptsOnlyKnownElementsTest::RunTest(const FString&)
{
	// Senza elemento e' il colpo `Direct`, cioe' ogni riga del catalogo di oggi: deve restare valida.
	TestEqual(TEXT("una riga senza elemento e' valida"),
		URTCatalogLibrary::ValidateActions({ ElemValidDef(TEXT("Test.Direct"), NAME_None) }).Num(), 0);
	TestEqual(TEXT("una riga Element.Electric e' valida"),
		URTCatalogLibrary::ValidateActions({ ElemValidDef(TEXT("Test.Electric"), ElemElectric()) }).Num(), 0);

	// Tutti quelli della lista, non solo quello di prova: il validatore legge la lista, non un elenco suo.
	for (const FName& Id : URTHeroCatalogLibrary::ElementIds())
	{
		TestEqual(FString::Printf(TEXT("%s e' accettato"), *Id.ToString()),
			URTCatalogLibrary::ValidateActions({ ElemValidDef(TEXT("Test.Element"), Id) }).Num(), 0);
	}

	// Fuori lista: un elemento che non esiste e il nome senza prefisso, il refuso piu' probabile.
	for (const TCHAR* Unknown : { TEXT("Element.Lightning"), TEXT("Electric") })
	{
		const TArray<FString> Errori = URTCatalogLibrary::ValidateActions({ ElemValidDef(TEXT("Test.Unknown"), FName(Unknown)) });
		if (!TestEqual(FString::Printf(TEXT("%s: un errore"), Unknown), Errori.Num(), 1)) { continue; }
		TestTrue(FString::Printf(TEXT("%s: l'errore nomina l'azione"), Unknown), Errori[0].Contains(TEXT("Test.Unknown")));
		TestTrue(FString::Printf(TEXT("%s: e l'elemento"), Unknown), Errori[0].Contains(Unknown));
	}
	return true;
}

/**
 * L'elemento lo onora solo un colpo che nasce da un intento del Blast (#4189), e il validatore lo rifiuta su ogni altra riga.
 *
 * Fuori dal Blast il danno lo applica chi non legge `ElementId`: `ApplyDamage(Direct)` al boundary della predittiva e nel
 * Move dell'Overwatch, `FRTCounterAttack` per una reazione, la scarica `Environmental` nel Cleanup; il Prep non lo applica
 * affatto. Un elemento su quelle righe sarebbe un dato che il colpo ignora in silenzio, e la fetta 2 ne mette su una
 * predittiva e su due reazioni di Gadget: deve trovarlo rifiutato finche' quei percorsi non lo portano.
 *
 * Le righe sono quelle di `RTDamagePathFixtures`: vere, con l'elemento come sola differenza dal catalogo. Il controllo e'
 * doppio: ogni riga senza elemento e' valida, e le righe che passano dal Blast restano valide con l'elemento — fra loro due
 * sintetiche che un predicato piu' largo (`PredictiveTargeting`, `ReactionTrigger`) rifiuterebbe per sbaglio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementRefusedWhereTheDamageSkipsTheBlastTest,
	"RefactorTactics.Damage.Element.RefusedWhereTheDamageSkipsTheBlast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementRefusedWhereTheDamageSkipsTheBlastTest::RunTest(const FString&)
{
	const FRTActionDef Counter = URTCatalogLibrary::FindCoreAction(TEXT("Action.Counter"));
	TestTrue(TEXT("premessa: Action.Counter e' una reazione"), Counter.Slot == ERTActionSlot::Reaction);
	TestTrue(TEXT("premessa: e per fase sta nel Blast, quindi la fase da sola non basta"),
		URTCatalogLibrary::MapResolutionPhase(Counter.ResolutionPhase) == ERTMatchPhase::Blast);

	for (RTDamagePathFixtures::FRow& Riga : RTDamagePathFixtures::OffTheBlast())
	{
		if (!TestFalse(FString::Printf(TEXT("%s: premessa, la riga esiste"), *Riga.Name), Riga.Def.ActionId.IsNone()))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s: premessa, senza elemento e' valida"), *Riga.Name),
			URTCatalogLibrary::ValidateActions({ Riga.Def }).Num(), 0);

		Riga.Def.ElementId = ElemElectric();
		const TArray<FString> Errori = URTCatalogLibrary::ValidateActions({ Riga.Def });
		if (!TestEqual(FString::Printf(TEXT("%s: con un elemento, un errore"), *Riga.Name), Errori.Num(), 1)) { continue; }
		TestTrue(FString::Printf(TEXT("%s: l'errore nomina la riga"), *Riga.Name),
			Errori[0].Contains(Riga.Def.ActionId.ToString()));
		TestTrue(FString::Printf(TEXT("%s: e l'elemento"), *Riga.Name), Errori[0].Contains(TEXT("Element.Electric")));
		TestTrue(FString::Printf(TEXT("%s: e la issue"), *Riga.Name), Errori[0].Contains(TEXT("#4189")));
	}

	for (RTDamagePathFixtures::FRow& Riga : RTDamagePathFixtures::ThroughTheBlast())
	{
		if (!TestFalse(FString::Printf(TEXT("%s: premessa, la riga esiste"), *Riga.Name), Riga.Def.ActionId.IsNone()))
		{
			continue;
		}
		Riga.Def.ElementId = ElemElectric();
		const TArray<FString> Errori = URTCatalogLibrary::ValidateActions({ Riga.Def });
		for (const FString& Errore : Errori) { AddInfo(FString::Printf(TEXT("%s: %s"), *Riga.Name, *Errore)); }
		TestEqual(FString::Printf(TEXT("%s: con un elemento resta valida"), *Riga.Name), Errori.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementHitCarriesTheIntentElementTest,
	"RefactorTactics.Damage.Element.HitCarriesTheIntentElement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementHitCarriesTheIntentElementTest::RunTest(const FString&)
{
	// Due attaccanti sullo stesso bersaglio: uno elettrico, uno senza elemento. Ogni colpo porta quello del SUO intento.
	TArray<FRTHexCombatUnit> Units;
	Units.Add(ElemUnit(0, 0, FRTCellId(0, 0, 0)));  // elettrico
	Units.Add(ElemUnit(1, 1, FRTCellId(2, 0, 0)));  // il bersaglio
	Units.Add(ElemUnit(2, 0, FRTCellId(0, 2, 0)));  // diretto
	Units.Add(ElemUnit(3, 1, FRTCellId(2, -1, 0))); // chi intercetta, sotto

	TArray<FRTHexAttackIntent> Intents;
	Intents.Add(ElemIntent(0, 1, Units[1].Cell, ElemElectric()));
	Intents.Add(ElemIntent(2, 1, Units[1].Cell, NAME_None));

	URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 4);
	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, Intents, Map);

	const FRTHexAttackHit* Elettrico = ElemHitOfIntent(Plan.Hits, 0);
	const FRTHexAttackHit* Diretto = ElemHitOfIntent(Plan.Hits, 1);
	if (!TestNotNull(TEXT("premessa: l'intento elettrico colpisce"), Elettrico)
		|| !TestNotNull(TEXT("premessa: l'intento diretto colpisce"), Diretto))
	{
		return false;
	}
	TestEqual(TEXT("il colpo dell'intento elettrico e' elettrico"), Elettrico->ElementId, ElemElectric());
	TestEqual(TEXT("il colpo dell'intento senza elemento resta senza"), Diretto->ElementId, FName(NAME_None));

	// Un colpo intercettato cambia bersaglio, non elemento: l'elemento e' dell'azione.
	const FRTHexAttackHit Rediretto = URTHexCombatLibrary::RedirectHitTo(3, *Elettrico, Units, Intents, Map);
	TestEqual(TEXT("premessa: il colpo ha cambiato bersaglio"), Rediretto.TargetId, 3);
	TestEqual(TEXT("e resta elettrico"), Rediretto.ElementId, ElemElectric());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementTravelsFromActionToHitInATurnTest,
	"RefactorTactics.Damage.Element.TravelsFromActionToHitInATurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementTravelsFromActionToHitInATurnTest::RunTest(const FString&)
{
	// L'attacco base di Ivrin contro un'altra Ivrin, adiacente e frontale: l'azione di prova e' quell'attacco, con
	// l'elemento scritto sul suo `Def`. Lo stesso turno senza elemento e' il controllo: il dato viene dal `Def`.
	const auto Turno = [this](const FName& ElementId, FElemTurnOutcome& Out) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		ARTUnit* Attaccante = ElemSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(-1, 0, 0));
		ARTUnit* Bersaglio = ElemSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(0, 0, 0));
		if (!TestNotNull(TEXT("mappa"), MapActor) || !TestNotNull(TEXT("turn manager"), TM)
			|| !TestNotNull(TEXT("attaccante"), Attaccante) || !TestNotNull(TEXT("bersaglio"), Bersaglio)
			|| !TestNotNull(TEXT("l'attacco base"), Attaccante->GetAbility(0)))
		{
			return false;
		}

		Attaccante->GetAbility(0)->Def.ElementId = ElementId;
		Bersaglio->Facing = ERTHexDirection::W;
		Bersaglio->ClearPlannedMainAction();
		Attaccante->SetPlannedMainAction(0);
		Attaccante->DeclareAttackOnUnit(Bersaglio);

		TM->bRecordBlastPlanForTest = true;
		ElemRunTurn(TM);

		const TArray<FRTHexAttackHit>& Hits = TM->RecordedBlastHitsForTest();
		const TArray<FRTHexAttackIntent>& Intents = TM->RecordedBlastIntentsForTest();
		Out.Intents = Intents.Num();
		Out.Hits = Hits.Num();
		if (Hits.Num() == 1 && Intents.IsValidIndex(Hits[0].IntentIndex))
		{
			Out.IntentElement = Intents[Hits[0].IntentIndex].ElementId;
			Out.HitElement = Hits[0].ElementId;
		}
		return true;
	};

	FElemTurnOutcome ConElemento, SenzaElemento;
	if (!Turno(ElemElectric(), ConElemento) || !Turno(NAME_None, SenzaElemento)) { return false; }

	if (!TestEqual(TEXT("premessa: un intento"), ConElemento.Intents, 1)
		|| !TestEqual(TEXT("premessa: un colpo"), ConElemento.Hits, 1))
	{
		return false;
	}
	TestEqual(TEXT("l'intento porta l'elemento dell'azione"), ConElemento.IntentElement, ElemElectric());
	TestEqual(TEXT("e il colpo pure"), ConElemento.HitElement, ElemElectric());

	TestEqual(TEXT("controllo: senza elemento, un colpo"), SenzaElemento.Hits, 1);
	TestEqual(TEXT("e il suo intento resta senza elemento"), SenzaElemento.IntentElement, FName(NAME_None));
	TestEqual(TEXT("e il colpo pure"), SenzaElemento.HitElement, FName(NAME_None));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDamageElementChargeImpactCarriesTheChargeElementTest,
	"RefactorTactics.Damage.Element.ChargeImpactCarriesTheChargeElement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDamageElementChargeImpactCarriesTheChargeElementTest::RunTest(const FString&)
{
	// Il secondo punto in cui l'intento nasce: l'impatto di una carica non passa dal ciclo dei piani
	// (`AppendChargeImpactIntents`). Riktor carica dritto e incontra Ivrin a (2,0), come `Actions.Charge.ImpactResolvesInBlast`.
	const auto Carica = [this](const FName& ElementId, FElemTurnOutcome& Out) -> bool
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6); }
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
		ARTUnit* Caricatore = ElemSpawnUnit(World, 0, URTHeroCatalogLibrary::MakeRiktor(), FRTCellId(0, 0, 0));
		ARTUnit* Vittima = ElemSpawnUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(2, 0, 0));
		if (!TestNotNull(TEXT("mappa"), MapActor) || !TestNotNull(TEXT("turn manager"), TM)
			|| !TestNotNull(TEXT("caricatore"), Caricatore) || !TestNotNull(TEXT("vittima"), Vittima))
		{
			return false;
		}

		const int32 Idx = RTAbilityFixtures::AddCoreAbility(Caricatore, TEXT("Action.Charge"));
		if (!TestNotNull(TEXT("la carica"), Caricatore->GetAbility(Idx))) { return false; }
		Caricatore->GetAbility(Idx)->Def.ElementId = ElementId;
		Vittima->ClearPlannedMainAction();
		Caricatore->SetPlannedDash(Idx, FRTCellId(3, 0, 0)); // dritto: incontra la vittima a (2,0) per strada

		TM->bRecordBlastPlanForTest = true;
		ElemRunTurn(TM);

		const TArray<FRTHexAttackHit>& Hits = TM->RecordedBlastHitsForTest();
		const TArray<FRTHexAttackIntent>& Intents = TM->RecordedBlastIntentsForTest();
		Out.Intents = Intents.Num();
		Out.Hits = Hits.Num();
		if (Hits.Num() == 1 && Intents.IsValidIndex(Hits[0].IntentIndex))
		{
			Out.IntentElement = Intents[Hits[0].IntentIndex].ElementId;
			Out.HitElement = Hits[0].ElementId;
		}
		return true;
	};

	FElemTurnOutcome ConElemento, SenzaElemento;
	if (!Carica(ElemElectric(), ConElemento) || !Carica(NAME_None, SenzaElemento)) { return false; }

	if (!TestEqual(TEXT("premessa: un intento, l'impatto"), ConElemento.Intents, 1)
		|| !TestEqual(TEXT("premessa: un colpo"), ConElemento.Hits, 1))
	{
		return false;
	}
	TestEqual(TEXT("l'impatto porta l'elemento della carica"), ConElemento.IntentElement, ElemElectric());
	TestEqual(TEXT("e il colpo pure"), ConElemento.HitElement, ElemElectric());

	TestEqual(TEXT("controllo: senza elemento, un colpo"), SenzaElemento.Hits, 1);
	TestEqual(TEXT("e l'impatto resta senza elemento"), SenzaElemento.IntentElement, FName(NAME_None));
	TestEqual(TEXT("e il colpo pure"), SenzaElemento.HitElement, FName(NAME_None));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
