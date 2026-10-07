// La geometria del colpo per il tracer del playback (`#2454`, spec `2026-10-07-tracer-attacco-base` §3).
//
// 🔑 Qui si misura il PRODUTTORE: che ogni `Attack` porti l'origine di `ResolveImpactOrigin`, la cella della
// propria vittima, la forma dell'intento e i due verdetti dei fatti puntuali. Il disegno e' di altri test.
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTMatchStateHash.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit.

	URTHexMapAsset* SpawnTracerMap(UWorld* World, int32 Radius = 8)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnTracerUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Copertura sul bordo di una cella (la stessa forma di `RTHexCoverTests.cpp`). */
	void SetTracerCoverEdge(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge, ERTHexCoverType Type)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, Type, FRTHexCover::DefaultIntegrity(Type)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	TArray<FRTResolvedEvent> TracerAttacks(const ARTTurnManager* TM)
	{
		TArray<FRTResolvedEvent> Out;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::Attack) { Out.Add(Ev); }
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatAttackCarriesHitGeometryTest,
	"RefactorTactics.Combat.AttackCarriesHitGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatAttackCarriesHitGeometryTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	// Un getto di Muiren che attraversa DUE vittime in linea: un intento, due `Attack`.
	SpawnTracerMap(World);
	ARTUnit* Muiren = SpawnTracerUnit(World, 0, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 2));
	ARTUnit* Vicina = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 2));
	ARTUnit* Lontana = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Muiren || !Vicina || !Lontana) { AddError(TEXT("allestimento fallito")); return false; }

	Muiren->PlannedAbilityIndex = 0; // Hero.Muiren.PressureJet, Line
	Muiren->PlannedAttackTarget = Lontana;
	Muiren->PlannedCell = Muiren->Cell;

	// La cella di ciascuna vittima PRIMA del turno: il getto le spinge, e l'impatto e' prima della spinta.
	//
	// ⚠️ **Si cattura per UNITA', non per `StableUnitId`**: l'id si assegna dentro `LockInAndResolve`
	// (`EnsureMatchRoster`) e prima vale `0` per tutte. Una mappa chiavata sull'id qui sopra collassava in una sola
	// voce, `Find` rispondeva `nullptr` per ogni evento e l'asserto sull'impatto cadeva sulla FIXTURE, non sul
	// produttore. L'id si legge dopo il turno, a roster congelato, con le celle gia' catturate.
	const FRTCellId CellaVicinaPrima = Vicina->Cell;
	const FRTCellId CellaLontanaPrima = Lontana->Cell;

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	TMap<int32, FRTCellId> CellaIniziale;
	CellaIniziale.Add(Vicina->StableUnitId, CellaVicinaPrima);
	CellaIniziale.Add(Lontana->StableUnitId, CellaLontanaPrima);
	if (!TestEqual(TEXT("premessa: il roster ha assegnato due id distinti"), CellaIniziale.Num(), 2)) { return false; }
	TestNotEqual(TEXT("premessa: nessuna delle due vittime ha id 0, cioe' «nessuna unita'»"),
		FMath::Min(Vicina->StableUnitId, Lontana->StableUnitId), 0);

	const TArray<FRTResolvedEvent> Colpi = TracerAttacks(TM);
	if (!TestEqual(TEXT("premessa: il getto ha colpito le due vittime"), Colpi.Num(), 2)) { return false; }

	for (const FRTResolvedEvent& Ev : Colpi)
	{
		const FRTCellId* Attesa = CellaIniziale.Find(Ev.TargetStableUnitId);
		TestTrue(TEXT("la geometria e' risolta"), Ev.HitGeometry.bResolved);
		TestTrue(TEXT("la forma e' quella dell'INTENTO"), Ev.Shape == ERTAbilityShape::Line);
		TestTrue(TEXT("l'origine e' la cella di Muiren"), Ev.HitGeometry.From == FRTCellId(0, 2));
		TestTrue(TEXT("l'impatto e' la cella della PROPRIA vittima, prima della spinta"),
			Attesa != nullptr && Ev.HitGeometry.Impact == *Attesa);
		TestTrue(TEXT("chi spara conosce la propria origine"), Ev.HitGeometry.FromVerdict.AllowsTeam(0));
		TestTrue(TEXT("chi e' colpito conosce il proprio impatto"), Ev.HitGeometry.ImpactVerdict.AllowsTeam(1));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCombatCoveredHitCarriesHitGeometryTest,
	"RefactorTactics.Combat.CoveredHitCarriesHitGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCombatCoveredHitCarriesHitGeometryTest::RunTest(const FString&)
{
	// Copertura BASSA: il colpo arriva ridotto, e il tracer arriva sul bersaglio. Copertura ALTA: niente `Attack`,
	// quindi niente tracer, e il ritmo del Blast non ha un colpo da scandire.
	for (const ERTHexCoverType Tipo : { ERTHexCoverType::Low, ERTHexCoverType::High })
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		URTHexMapAsset* Map = SpawnTracerMap(World);
		ARTUnit* Ivrin = SpawnTracerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 2));
		ARTUnit* Branth = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Ivrin || !Branth) { AddError(TEXT("allestimento fallito")); return false; }
		SetTracerCoverEdge(Map, FRTCellId(3, 2), ERTHexDirection::W, Tipo);

		Ivrin->PlannedAbilityIndex = 0; // Hero.Ivrin.PulseShot, Single
		Ivrin->PlannedAttackTarget = Branth;
		Ivrin->PlannedCell = Ivrin->Cell;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

		const TArray<FRTResolvedEvent> Colpi = TracerAttacks(TM);
		if (Tipo == ERTHexCoverType::Low)
		{
			if (!TestEqual(TEXT("bassa: premessa, il colpo e' arrivato"), Colpi.Num(), 1)) { return false; }
			TestTrue(TEXT("bassa: geometria risolta"), Colpi[0].HitGeometry.bResolved);
			TestTrue(TEXT("bassa: l'impatto e' la cella di Branth"), Colpi[0].HitGeometry.Impact == FRTCellId(3, 2));
			TestTrue(TEXT("bassa: proiettile per chi spara"),
				URTPlaybackLibrary::TracerStyleFor(Colpi[0], 0) == ERTTracerStyle::Projectile);
		}
		else
		{
			TestEqual(TEXT("alta: nessun Attack, quindi nessun tracer"), Colpi.Num(), 0);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrivacyUnseenAttackerTest,
	"RefactorTactics.Privacy.UnseenAttackerIsOutOfTheOriginVerdict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrivacyUnseenAttackerTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	// Branth spara a Ivrin DA DIETRO, a 3 celle: oltre la consapevolezza ravvicinata (2, `RTPerceptionLibrary.h`)
	// e fuori dall'arco frontale di Ivrin, che guarda a Est come Branth.
	SpawnTracerMap(World);
	ARTUnit* Branth = SpawnTracerUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Ivrin = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(5, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Branth || !Ivrin) { AddError(TEXT("allestimento fallito")); return false; }
	Branth->Facing = ERTHexDirection::E;
	Ivrin->Facing = ERTHexDirection::E;

	Branth->PlannedAbilityIndex = 0; // Hero.Branth.ImpactShot, Single, portata 3
	Branth->PlannedAttackTarget = Ivrin;
	Branth->PlannedCell = Branth->Cell;

	TM->LockInAndResolve();
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	const TArray<FRTResolvedEvent> Colpi = TracerAttacks(TM);
	if (!TestEqual(TEXT("premessa: il colpo alle spalle e' arrivato"), Colpi.Num(), 1)) { return false; }
	const FRTHitGeometry& G = Colpi[0].HitGeometry;

	TestTrue(TEXT("la geometria e' risolta"), G.bResolved);
	TestTrue(TEXT("chi spara conosce la propria origine"), G.FromVerdict.AllowsTeam(0));
	// 🔴 Il punto del test: la squadra colpita non vedeva chi sparava, e il verdetto lo dice.
	TestFalse(TEXT("la squadra colpita NON conosceva l'attaccante"), G.FromVerdict.AllowsTeam(1));
	// D-380: chi colpisce conosce la vittima prima che il verdetto si congeli (`RevealHitTargetsToAttackers`).
	TestTrue(TEXT("chi spara conosce la cella d'impatto"), G.ImpactVerdict.AllowsTeam(0));

	TestTrue(TEXT("per chi spara: proiettile"), URTPlaybackLibrary::TracerStyleFor(Colpi[0], 0) == ERTTracerStyle::Projectile);
	TestTrue(TEXT("per chi e' colpito: nessun tracer"), URTPlaybackLibrary::TracerStyleFor(Colpi[0], 1) == ERTTracerStyle::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDeterminismHitGeometryOutOfHashesTest,
	"RefactorTactics.Determinism.HitGeometryStaysOutOfHashes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDeterminismHitGeometryOutOfHashesTest::RunTest(const FString&)
{
	// Lo stesso turno due volte: una con la geometria, una con il produttore che la lascia vuota. Se la geometria
	// entrasse in uno dei due hash, i numeri differirebbero.
	int64 StatoHash[2] = { 0, 0 };
	uint32 LogHash[2] = { 0, 0 };
	bool bRisolta[2] = { false, false };
	for (int32 Run = 0; Run < 2; ++Run)
	{
		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
		ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

		const URTHexMapAsset* Map = SpawnTracerMap(World);
		ARTUnit* Ivrin = SpawnTracerUnit(World, 0, URTHeroCatalogLibrary::MakeIvrin(), FRTCellId(1, 2));
		ARTUnit* Branth = SpawnTracerUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(3, 2));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Ivrin || !Branth) { AddError(TEXT("allestimento fallito")); return false; }
		TM->bSkipHitGeometryForTest = (Run == 1);

		Ivrin->PlannedAbilityIndex = 0;
		Ivrin->PlannedAttackTarget = Branth;
		Ivrin->PlannedCell = Ivrin->Cell;

		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

		// ⚠️ **Lo StateHash si calcola QUI, con le funzioni della partita, e non si legge da
		// `GetPendingFinalStateHash()`**: quello si cattura solo con la registrazione del replay accesa
		// (`CaptureFinalStateHash` esce a vuoto altrimenti), e in questa fixture valeva `0` — il confronto `0 == 0`
		// passava su QUALUNQUE produttore, che e' proprio cio' che la premessa qui sotto esiste per smascherare.
		// `HashMatchState` + `BuildUnitDigests` sono le due chiamate di `CaptureFinalStateHash`, sul mondo a fine turno.
		TArray<ARTUnit*> Unita;
		Unita.Add(Ivrin);
		Unita.Add(Branth);
		TArray<int32> Punteggi;
		Punteggi.Add(TM->GetTeamScore(0));
		Punteggi.Add(TM->GetTeamScore(1));
		StatoHash[Run] = static_cast<int64>(URTMatchStateHashLibrary::HashMatchState(
			Map, URTMatchStateHashLibrary::BuildUnitDigests(Unita), Punteggi));
		LogHash[Run] = URTTurnLogLibrary::HashTurnLog(TM->GetTurnLog());
		const TArray<FRTResolvedEvent> Colpi = TracerAttacks(TM);
		bRisolta[Run] = Colpi.Num() == 1 && Colpi[0].HitGeometry.bResolved;
	}

	// Controllo positivo: il gancio ha davvero tolto la geometria, o il confronto non misurerebbe niente.
	TestTrue(TEXT("controllo: con la geometria, risolta"), bRisolta[0]);
	TestFalse(TEXT("controllo: col gancio, assente"), bRisolta[1]);
	TestNotEqual(TEXT("premessa: lo StateHash e' stato catturato"), StatoHash[0], (int64)0);
	TestEqual(TEXT("StateHash identico con e senza geometria"), StatoHash[0], StatoHash[1]);
	TestEqual(TEXT("HashTurnLog identico con e senza geometria"), LogHash[0], LogHash[1]);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
