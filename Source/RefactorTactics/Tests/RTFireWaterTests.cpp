#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Combat/RTCombatLibrary.h" // BurningCleanupDamage: il danno del Cleanup si chiede alla libreria (#2460)
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLogLibrary.h" // DescribeEntry: la voce nuova deve LEGGERSI, non solo esistere
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Tests/RTAbilityFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 8.4 — fuoco e acqua si annullano, e la mappa cambia stato **nel TurnLog**.
 *
 * Il pezzo nuovo e' il TERRENO DINAMICO: fino a CP 8.3 la mappa era immutabile in partita. La superficie
 * corrente vive nella mappa (che tutti leggono gia'), la superficie originale e la durata nel `TurnManager`.
 *
 * Prefissi `Fw*` negli helper: unity build, namespace anonimi fusi con gli altri file di test.
 */
namespace
{
	UWorld* MakeFwWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyFwWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnFwMap(UWorld* World, const TMap<FRTCellId, ERTHexSurface>& Surfaces, int32 Radius = 4)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			FRTHexCellData Cell(Id);
			if (const ERTHexSurface* Surface = Surfaces.Find(Id))
			{
				Cell.Surface = *Surface;
				Cell.MoveCost = URTTerrainLibrary::FindTerrainDef(*Surface).MoveCost;
			}
			M->AddOrUpdateCell(Cell);
		}
		M->SortCells();

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnFwUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell;
		return U;
	}

	/** Mette un'azione ambientale del catalogo nello slot 3 (lo scatto) e la pianifica sul bersaglio. */
	void PlanFwEnvironmentAction(ARTUnit* Caster, const TCHAR* ActionId, ARTUnit* Target)
	{
		RTAbilityFixtures::AddCoreAbilityInSlot(Caster, ActionId, 3);
		Caster->PlannedAbilityIndex = 3;
		Caster->PlannedAttackTarget = Target;
	}

	void RunFwTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}

	ERTHexSurface FwSurfaceAt(const ARTHexMapActor* MapActor, const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = MapActor && MapActor->MapAsset ? MapActor->MapAsset->FindCell(Cell) : nullptr;
		return Data ? Data->Surface : ERTHexSurface::Floor;
	}

	/** Voci di categoria `Status` con quell'esito e quel tag (#1077). */
	int32 CountFwStatusEntries(const ARTTurnManager* TM, ERTStatusOutcome Outcome, FGameplayTag Tag)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Status
				&& E.Outcome == static_cast<uint8>(Outcome)
				&& E.ActionId == Tag.GetTagName())
			{
				++N;
			}
		}
		return N;
	}

	int32 CountFwEnvironmentEntries(const ARTTurnManager* TM, ERTEnvironmentOutcome Outcome)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Environment && E.Outcome == static_cast<uint8>(Outcome)) { ++N; }
		}
		return N;
	}
}

// ⚠️ `ClientContext` oltre a `EditorContext`, e non per simmetria con i vicini: e' uno dei **dieci test
// vincolanti** del catalogo (`roadmap-v0.1.md` §6), e CP 12.3 (`#83`) chiede che la suite giri anche in un
// pacchetto. Con il solo `EditorContext` il controller lo filtra fuori dal target `Game` — misurato:
// `Automation List` in un pacchetto Development registrava **zero** test su 875, tutti dichiarati solo Editor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTWaterExtinguishesFireTest,
	"RefactorTactics.Environment.WaterExtinguishesFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FRTWaterExtinguishesFireTest::RunTest(const FString&)
{
	// **Nome vincolante** del catalogo §15. L'acqua che arriva su una cella in fiamme la spegne: la cella
	// diventa acqua, e il TurnLog lo dice con un esito PROPRIO — «si allaga» e «si spegne» non sono lo stesso
	// evento per chi legge il replay.
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::Fire); // la cella brucia gia'
	ARTHexMapActor* MapActor = SpawnFwMap(World, Surfaces);

	ARTUnit* Caster = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnFwUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}

	TestTrue(TEXT("si parte da una cella in fiamme"),
		FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::Fire);

	PlanFwEnvironmentAction(Caster, TEXT("Action.CreateWater"), Target);
	RunFwTurn(TM);

	TestTrue(TEXT("la cella e' diventata acqua"),
		FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::ShallowWater);
	TestEqual(TEXT("e il TurnLog lo registra come «spento», non come un allagamento qualunque"),
		CountFwEnvironmentEntries(TM, ERTEnvironmentOutcome::SurfaceExtinguished), 1);

	DestroyFwWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTFireDoesNotIgniteWaterOrMetalTest,
	"RefactorTactics.Environment.Fire.DoesNotIgniteWaterOrMetal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTFireDoesNotIgniteWaterOrMetalTest::RunTest(const FString&)
{
	// Il catalogo terreni §2: il fuoco «non incendia automaticamente acqua o metallo». E' una proprieta' delle
	// SUPERFICI, non un elenco di eccezioni nel resolver — cosi' una superficie nuova dichiara da se' se brucia.
	TestFalse(TEXT("l'acqua non brucia"),
		URTTerrainLibrary::FindTerrainDef(ERTHexSurface::ShallowWater).bIsFlammable);
	TestFalse(TEXT("il metallo nemmeno"),
		URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Conductive).bIsFlammable);
	TestTrue(TEXT("il terreno neutro si'"),
		URTTerrainLibrary::FindTerrainDef(ERTHexSurface::Floor).bIsFlammable);

	// **Limite dichiarato del v0.1**: il catalogo elenca come combustibili vegetazione, olio e gas, che NON
	// esistono fra le otto superfici. Il fuoco quindi non ha di che propagarsi da solo: la propagazione e'
	// vuota **per costruzione del catalogo**, non per codice mancante. Il giorno in cui una superficie
	// combustibile verra' aggiunta, questo conteggio cambiera' e il test lo dira'.
	int32 Flammable = 0;
	for (const FRTTerrainDef& Def : URTTerrainLibrary::GetTerrainCatalog())
	{
		if (Def.bIsFlammable) { ++Flammable; }
	}
	TestEqual(TEXT("due sole superfici combustibili nel v0.1 (Floor, Rough)"), Flammable, 2);

	// In partita: incendiare una cella d'acqua non la incendia, e il rifiuto e' REGISTRATO — un'azione che
	// non fa nulla in silenzio e' peggio di una che fallisce a voce alta.
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::ShallowWater);
	ARTHexMapActor* MapActor = SpawnFwMap(World, Surfaces);

	ARTUnit* Caster = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnFwUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}

	PlanFwEnvironmentAction(Caster, TEXT("Action.Ignite"), Target);
	RunFwTurn(TM);

	TestTrue(TEXT("l'acqua resta acqua"),
		FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::ShallowWater);
	TestEqual(TEXT("e il rifiuto compare nel TurnLog"),
		CountFwEnvironmentEntries(TM, ERTEnvironmentOutcome::SurfaceRejected), 1);

	DestroyFwWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnvironmentChangesInTurnLogTest,
	"RefactorTactics.Environment.ChangesAppearInTurnLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnvironmentChangesInTurnLogTest::RunTest(const FString&)
{
	// Ogni modifica ambientale e' osservabile: accensione, durata e ritorno alla superficie originale. Senza,
	// un'unita' che a T+2 incassa 10 danni «senza motivo» resta inspiegabile nel replay.
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTHexMapActor* MapActor = SpawnFwMap(World, {}); // tutto pavimento: combustibile
	ARTUnit* Caster = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Target = SpawnFwUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Caster"), Caster) || !TestNotNull(TEXT("Target"), Target) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}

	PlanFwEnvironmentAction(Caster, TEXT("Action.Ignite"), Target);
	RunFwTurn(TM);

	TestTrue(TEXT("la cella ha preso fuoco"), FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::Fire);
	TestEqual(TEXT("il cambio e' nel TurnLog"),
		CountFwEnvironmentEntries(TM, ERTEnvironmentOutcome::SurfaceChanged), 1);

	// La voce porta la cella e la CAUSA: un cambiamento anonimo non spiega niente.
	bool bHasCauseAndCell = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Environment
			&& E.ActionId == FName(TEXT("Action.Ignite"))
			&& E.TgtCell == FRTCellId(1, 0)
			&& E.Amount == 2) // i turni di durata
		{
			bHasCauseAndCell = true;
		}
	}
	TestTrue(TEXT("con cella, causa e durata"), bHasCauseAndCell);

	// Due turni dopo la cella torna com'era, e anche questo si registra.
	RunFwTurn(TM);
	TestTrue(TEXT("dopo un turno brucia ancora"), FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::Fire);
	RunFwTurn(TM);
	TestTrue(TEXT("scaduta la durata, la cella torna pavimento"),
		FwSurfaceAt(MapActor, FRTCellId(1, 0)) == ERTHexSurface::Floor);
	TestEqual(TEXT("il ripristino e' registrato"),
		CountFwEnvironmentEntries(TM, ERTEnvironmentOutcome::SurfaceRestored), 1);

	DestroyFwWorld(World);
	return true;
}



/**
 * **Uno stato a termine nasce e SCADE, e il TurnLog registra entrambi i momenti** (#1077).
 *
 * Prima di questa issue nessuno dei tre momenti della vita di uno stato aveva una voce: il replay vedeva
 * un'unita' cominciare a bruciare senza sapere perche', e smettere senza sapere se fosse uscita dal fuoco o
 * se fosse scaduta la durata. Le due cause hanno esiti distinti, ed e' meta' del valore dell'issue.
 *
 * ⚠️ **L'unita' ESCE dal fuoco prima che il `Burning` finisca**, e non e' un dettaglio dell'allestimento: e'
 * l'avvertenza di #1067 — `Fire` concede `Burning` con durata **2**, non col legame alla cella, quindi lo
 * stato SOPRAVVIVE alla cella che l'ha dato. Se l'unita' restasse sul fuoco, il terreno riapplicherebbe lo
 * stato ogni turno e la scadenza non arriverebbe mai: il test misurerebbe la propria pazienza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusBirthAndExpiryInTurnLogTest,
	"RefactorTactics.Environment.Status.BirthAndExpiryAppearInTurnLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusBirthAndExpiryInTurnLogTest::RunTest(const FString&)
{
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::Fire);
	SpawnFwMap(World, Surfaces);

	ARTUnit* Unit = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	// ⚠️ **Serve un avversario, anche se non fa niente**: con una squadra sola la partita finisce per
	// eliminazione al primo Cleanup, e dal secondo turno `LockInAndResolve` non risolve piu' nulla. Il
	// sintomo e' un piano che resta non consumato, non un errore — misurato prima di scrivere questa riga.
	ARTUnit* Inerte = SpawnFwUnit(World, 1, FRTCellId(-3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("avversario"), Inerte) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}

	// T1: entra nel fuoco.
	Unit->PlannedCell = FRTCellId(1, 0);
	RunFwTurn(TM);
	if (!TestTrue(TEXT("l'unita' e' entrata nel fuoco"), Unit->Cell == FRTCellId(1, 0)))
	{
		DestroyFwWorld(World);
		return false;
	}
	TestTrue(TEXT("e sta bruciando"), Unit->HasStatus(TAG_Status_Burning));
	TestEqual(TEXT("la NASCITA e' nel TurnLog, e dice che viene dal terreno"),
		CountFwStatusEntries(TM, ERTStatusOutcome::AppliedByTerrain, TAG_Status_Burning), 1);

	// E la voce porta la durata: senza, un replay non sa quanto durera'.
	bool bConDurata = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Status
			&& E.Outcome == static_cast<uint8>(ERTStatusOutcome::AppliedByTerrain)
			&& E.ActionId == TAG_Status_Burning.GetTag().GetTagName()
			&& E.Amount > 0
			&& E.TgtCell == FRTCellId(1, 0))
		{
			bConDurata = true;
		}
	}
	TestTrue(TEXT("con la cella e una durata positiva"), bConDurata);

	// T2 e oltre: esce dal fuoco e aspetta che il conteggio finisca.
	Unit->PlannedCell = FRTCellId(0, 0);
	RunFwTurn(TM);
	TestTrue(TEXT("e' uscita dal fuoco"), Unit->Cell == FRTCellId(0, 0));

	int32 Giri = 0;
	while (Unit->HasStatus(TAG_Status_Burning) && Giri < 6)
	{
		Unit->PlannedCell = Unit->Cell;
		RunFwTurn(TM);
		++Giri;
	}
	TestFalse(TEXT("il Burning e' finito da solo"), Unit->HasStatus(TAG_Status_Burning));
	TestEqual(TEXT("e la SCADENZA e' una voce distinta"),
		CountFwStatusEntries(TM, ERTStatusOutcome::Expired, TAG_Status_Burning), 1);
	TestEqual(TEXT("che non e' una revoca: nessuno ha lasciato una cella che lo sosteneva"),
		CountFwStatusEntries(TM, ERTStatusOutcome::Revoked, TAG_Status_Burning), 0);

	DestroyFwWorld(World);
	return true;
}

/**
 * **Uno stato LEGATO ALLA CELLA nasce e viene REVOCATO uscendo, e le due voci lo dicono** (#1077).
 *
 * E' l'altra meta' della distinzione: qui la causa della fine e' una **mossa del giocatore**, non il tempo.
 * Un replay che confondesse revoca e scadenza non saprebbe dire se qualcuno ha fatto qualcosa.
 *
 * ⚠️ **`Amount` vale zero, ed e' voluto**: `ApplyStatus` riceve `ARTUnit::PersistentWhileOnCell`, che vale
 * `-1` e **non e' una durata**. Scriverlo nel log darebbe a chi rilegge «meno un turno» da interpretare, ed
 * e' per questo che la forma di vita sta nell'ESITO — `AppliedWhileOnCell` — e non in un numero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusRevocationInTurnLogTest,
	"RefactorTactics.Environment.Status.RevocationAppearsInTurnLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusRevocationInTurnLogTest::RunTest(const FString&)
{
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::ShallowWater);
	SpawnFwMap(World, Surfaces);

	ARTUnit* Unit = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	// Come sopra: senza un avversario la partita finisce al primo Cleanup e il secondo turno non risolve.
	ARTUnit* Inerte = SpawnFwUnit(World, 1, FRTCellId(-3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("avversario"), Inerte) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}

	Unit->PlannedCell = FRTCellId(1, 0);
	RunFwTurn(TM);
	if (!TestTrue(TEXT("l'unita' e' entrata in acqua"), Unit->Cell == FRTCellId(1, 0)))
	{
		DestroyFwWorld(World);
		return false;
	}
	TestTrue(TEXT("ed e' bagnata"), Unit->HasStatus(TAG_Status_Wet));
	TestEqual(TEXT("la nascita dice che lo stato e' LEGATO ALLA CELLA"),
		CountFwStatusEntries(TM, ERTStatusOutcome::AppliedWhileOnCell, TAG_Status_Wet), 1);

	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Status
			&& E.Outcome == static_cast<uint8>(ERTStatusOutcome::AppliedWhileOnCell))
		{
			TestEqual(TEXT("e non porta una durata, perche' qui una durata non esiste"), E.Amount, 0);
		}
	}

	// Esce: la cella non lo sostiene piu'.
	Unit->PlannedCell = FRTCellId(0, 0);
	RunFwTurn(TM);
	TestTrue(TEXT("e' uscita dall'acqua"), Unit->Cell == FRTCellId(0, 0));
	TestFalse(TEXT("non e' piu' bagnata"), Unit->HasStatus(TAG_Status_Wet));
	TestEqual(TEXT("la REVOCA e' registrata"),
		CountFwStatusEntries(TM, ERTStatusOutcome::Revoked, TAG_Status_Wet), 1);
	TestEqual(TEXT("e non e' una scadenza: il conteggio non c'entra"),
		CountFwStatusEntries(TM, ERTStatusOutcome::Expired, TAG_Status_Wet), 0);

	DestroyFwWorld(World);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStatusExtinguishedInTurnLogTest,
	"RefactorTactics.Environment.Status.ExtinguishedAppearsInTurnLog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStatusExtinguishedInTurnLogTest::RunTest(const FString&)
{
	// 🔴 **Il caso con cui `#1314` si apriva**: un'unita' cammina dal fuoco nell'acqua bassa, smette di
	// bruciare, e il TurnLog era **muto** — esattamente come prima di `#1077`. Il replay non poteva dire
	// *perche'* avesse smesso.
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::Fire);
	Surfaces.Add(FRTCellId(2, 0), ERTHexSurface::ShallowWater);
	SpawnFwMap(World, Surfaces);

	ARTUnit* Unit = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	// Senza un avversario la partita finisce al primo Cleanup e il secondo turno non risolve.
	ARTUnit* Inerte = SpawnFwUnit(World, 1, FRTCellId(-3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("avversario"), Inerte) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}

	// (1) Nel fuoco: prende `Burning`.
	Unit->PlannedCell = FRTCellId(1, 0);
	RunFwTurn(TM);
	if (!TestTrue(TEXT("l'unita' e' entrata nel fuoco"), Unit->Cell == FRTCellId(1, 0))
		|| !TestTrue(TEXT("e sta bruciando"), Unit->HasStatus(TAG_Status_Burning)))
	{
		DestroyFwWorld(World);
		return false;
	}

	// (2) Nell'acqua: smette di bruciare, e il log deve dire PERCHE'.
	Unit->PlannedCell = FRTCellId(2, 0);
	RunFwTurn(TM);
	TestTrue(TEXT("e' entrata in acqua"), Unit->Cell == FRTCellId(2, 0));
	TestFalse(TEXT("e non brucia piu'"), Unit->HasStatus(TAG_Status_Burning));

	TestEqual(TEXT("lo SPEGNIMENTO e' registrato"),
		CountFwStatusEntries(TM, ERTStatusOutcome::Extinguished, TAG_Status_Burning), 1);

	// ⚠️ **E non e' una scadenza ne' una revoca.** Senza questi due controlli, un esito qualunque sul tag
	// giusto passerebbe il primo assert: la domanda di `#1314` non e' «c'e' una voce» ma «la voce dice
	// *cosa* ha tolto lo stato».
	TestEqual(TEXT("non e' una scadenza: il conteggio non c'entra"),
		CountFwStatusEntries(TM, ERTStatusOutcome::Expired, TAG_Status_Burning), 0);
	TestEqual(TEXT("e non e' una revoca: nessuna cella lo sosteneva"),
		CountFwStatusEntries(TM, ERTStatusOutcome::Revoked, TAG_Status_Burning), 0);

	// (3) E si LEGGE: senza il ramo in `DescribeEntry` la voce ricadrebbe nel default e il replay
	// mostrerebbe «esito di stato non tradotto». E' gia' successo con `AppliedWhileOnCell`, che compariva
	// come «eliminata».
	bool bTradotta = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Status
			&& E.Outcome == static_cast<uint8>(ERTStatusOutcome::Extinguished))
		{
			const FString Testo = URTTurnLogLibrary::DescribeEntry(E);
			TestFalse(TEXT("la voce non ricade nel ramo 'non tradotto'"), Testo.Contains(TEXT("non tradotto")));
			TestTrue(TEXT("e dice che e' stata l'acqua"), Testo.Contains(TEXT("spento dall'acqua")));
			bTradotta = true;
		}
	}
	TestTrue(TEXT("la voce di spegnimento esiste ed e' stata letta"), bTradotta);

	DestroyFwWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Il danno ambientale nel PLAYBACK (`#2460`).
//
// I test qui sopra sorvegliano il canale della TRACCIA: il TurnLog registra il danno da terreno dal
// 2026-08-16 (`#625`, `#1067`). Questi sorvegliano il canale della PRESENTAZIONE, che fino a `#2460` non lo
// riceveva mai: `ERTResolvedEventType::HazardDamage` era dichiarato e non lo emetteva nessuno — fuori dai
// test, l'unica occorrenza del valore in tutto `Source/` era la riga della tabella che lo dichiara. Chi
// guardava la risoluzione vedeva la barra scendere senza che nulla lo spiegasse.
//
// ## Perche' stanno in QUESTO file e non in `RTTerrainTests`
//
// `RTTerrainTests` misura il catalogo e le librerie pure: non ha un `UWorld` ne' un turno. Il difetto che
// questi test sorvegliano vive nel **cablaggio** — un evento emesso o non emesso durante una risoluzione
// vera — ed e' precisamente cio' che un test puro non puo' vedere. Qui l'harness c'e' gia', e i gemelli sul
// canale della traccia sono le venti righe qui sopra: se un giorno i due canali divergessero, i due gruppi
// cadrebbero in modo diverso e si vedrebbe quale ha sbagliato.
//
// ## Perche' l'emissione sta in `AppendLogEntry`
//
// I siti che applicano danno ambientale sono **due** e stanno in punti diversi: `ApplyTerrainOnEnterEffects`
// copre l'ingresso in cella (Dash, Move, spostamento forzato, ambiente), il `Status.Burning` del Cleanup
// vive in `LockInAndResolve` fuori da quella funzione. Un'emissione per sito sarebbe una copertura da
// mantenere a mano; dal punto unico che entrambi attraversano e' **per costruzione** — la stessa scelta, e
// la stessa ragione, di `StatusChanged` (`#2245`).
//
// ⚠️ Ne segue il rischio proprio che `AttackDoesNotEmitHazardDamage` sorveglia: quel punto lo attraversa
// **ogni** voce di log, non solo quelle di danno ambientale.
//
// ⚠️ Prefissi `Fw*` negli helper, come il resto del file: unity build, namespace anonimi fusi.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Gli eventi `HazardDamage` che il turno appena risolto ha messo sulla timeline di playback. */
	TArray<FRTResolvedEvent> FwHazardEvents(const ARTTurnManager* TM)
	{
		return TM ? TM->ResolvedHazardEventsForTest() : TArray<FRTResolvedEvent>();
	}

	/** Le voci di TurnLog del danno da terreno: la gemella di ogni evento, sull'altro canale. */
	int32 CountFwTerrainDamageEntries(const ARTTurnManager* TM)
	{
		int32 N = 0;
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (URTTurnLogLibrary::IsEnvironmentalDamage(E)) { ++N; }
		}
		return N;
	}
}

/**
 * 🔴 **Entrare nel fuoco durante il MOVIMENTO produce un evento di playback, e l'evento dice tutto.**
 *
 * ⚠️ **Anti-vacuita': si asserisce ogni CAMPO, non l'esistenza.** Un'emissione che scrivesse la fase
 * sbagliata, o il soggetto in `Source` invece che in `Target`, passerebbe un test che conta soltanto — ed e'
 * esattamente il difetto che `#2460` chiude, nato da un canale che *sembrava* coperto.
 *
 * 🔑 **E si asserisce la COERENZA con la voce gemella**, non solo il valore: i due canali raccontano lo
 * stesso fatto, e il giorno in cui divergessero questo test lo direbbe invece di lasciarlo scoprire a
 * schermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardPlaybackOnMoveTest,
	"RefactorTactics.Environment.Hazard.PlaybackOnMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardPlaybackOnMoveTest::RunTest(const FString&)
{
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::Fire);
	SpawnFwMap(World, Surfaces);

	ARTUnit* Unit = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	// Senza un avversario la partita finisce al primo Cleanup: stessa ragione dei test qui sopra.
	ARTUnit* Inerte = SpawnFwUnit(World, 1, FRTCellId(-3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("avversario"), Inerte) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}
	// [D-224] Lo scudo base assorbirebbe parte dei 10 danni e renderebbe l'asserto sull'`Amount` illeggibile.
	// ⚠️ Non cambia cio' che si misura: `Amount` porta il danno NOMINALE, quindi varrebbe 10 comunque — ma
	// senza scudo la premessa «il danno e' arrivato» resta verificabile a occhio.
	Unit->Shield = 0;

	Unit->PlannedCell = FRTCellId(1, 0);
	RunFwTurn(TM);

	// Anti-vacuita': se l'unita' non fosse entrata, «zero eventi» e «un evento» direbbero la stessa cosa.
	if (!TestTrue(TEXT("premessa: l'unita' e' entrata nel fuoco"), Unit->Cell == FRTCellId(1, 0)))
	{
		DestroyFwWorld(World);
		return false;
	}

	// 🔴 **DUE eventi, e la prima stesura di questo test ne attendeva uno.** Entrare nel fuoco e' un fatto
	// solo per chi guarda la mappa, ma sono **due danni distinti nello stesso turno**: la fiamma della cella
	// (`Terrain.Fire`, 10, nella fase del movimento) e lo `Status.Burning` che quella cella ha appena
	// concesso, che il Cleanup dello stesso turno fa gia' scattare (8). Misurato, non previsto — la suite ha
	// riportato `2` dove il test chiedeva `1`.
	//
	// 🔑 **E il canale deve poterli distinguere**, che e' il motivo per cui questo test adesso asserisce
	// entrambi invece di contarne uno: a schermo sono due colpi con due cause, e una presentazione che li
	// fondesse racconterebbe 18 danni in un lampo solo.
	const TArray<FRTResolvedEvent> Eventi = FwHazardEvents(TM);
	if (!TestEqual(TEXT("entrare nel fuoco produce DUE danni ambientali: la fiamma e lo status che ne nasce"),
		Eventi.Num(), 2))
	{
		DestroyFwWorld(World);
		return false;
	}

	const FRTResolvedEvent* Fiamma = Eventi.FindByPredicate(
		[](const FRTResolvedEvent& E) { return E.Phase == ERTMatchPhase::Move; });
	const FRTResolvedEvent* Brucia = Eventi.FindByPredicate(
		[](const FRTResolvedEvent& E) { return E.Phase == ERTMatchPhase::Cleanup; });
	if (!TestNotNull(TEXT("la fiamma della cella porta la fase del movimento"), Fiamma)
		|| !TestNotNull(TEXT("e il tick di Burning porta quella del Cleanup"), Brucia))
	{
		DestroyFwWorld(World);
		return false;
	}

	// 🔴 Il soggetto sta in `Target` e NON in `Source`, ed e' l'inverso di `StatusChanged`: la convenzione
	// della struct e' «`Source` = chi ha agito», e in un danno da terreno non c'e' un attaccante.
	TestEqual(TEXT("il soggetto e' chi SUBISCE"), Fiamma->TargetStableUnitId, Unit->StableUnitId);
	// ⚠️ `0` = «nessuno» ([D-063]), non «l'unita' numero zero». Chi consuma deve leggerlo cosi'.
	TestEqual(TEXT("nessun attaccante dichiarato"), Fiamma->SourceStableUnitId, 0);
	TestEqual(TEXT("l'Amount della fiamma e' il danno NOMINALE del catalogo"), Fiamma->Amount, 10);
	TestEqual(TEXT("la cella e' quella che ha colpito"), Fiamma->Origin, FRTCellId(1, 0));

	// ⚠️ **I due importi sono diversi, ed e' cio' che rende i due eventi non intercambiabili.** Se un giorno
	// coincidessero, un'emissione che sbagliasse causa passerebbe questo test senza che nulla lo dica.
	TestEqual(TEXT("il tick di Burning porta il SUO danno, non quello dell'ingresso"),
		Brucia->Amount, URTCombatLibrary::BurningCleanupDamage);
	TestNotEqual(TEXT("e i due importi restano distinguibili"), Fiamma->Amount, Brucia->Amount);

	// 🔑 La coerenza fra i due canali: due voci di traccia, due eventi di playback, sugli stessi due fatti.
	TestEqual(TEXT("le voci gemelle nel TurnLog sono due, come gli eventi"),
		CountFwTerrainDamageEntries(TM), 2);

	DestroyFwWorld(World);
	return true;
}

/**
 * 🔴 **Lo stesso ingresso durante il DASH porta la fase `Dash`, non `Move`.**
 *
 * E' l'unico test che prova che la fase non e' **cablata**: `ApplyTerrainOnEnterEffects` la riceve per
 * parametro proprio perche' i suoi chiamanti stanno in fasi diverse, e un'emissione che scrivesse una
 * costante passerebbe il test qui sopra senza che nulla lo dica.
 *
 * ⚠️ **La trappola che questa fase documenta**: durante il Cleanup il membro `Phase` del TurnManager vale
 * `Planning`, e il codice di quella funzione ci e' gia' caduto una volta. Emettendo da `AppendLogEntry` la
 * fase arriva da `Entry.Phase`, che il produttore ha gia' riempito con `InPhase`: il membro non e'
 * raggiungibile per sbaglio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardPlaybackOnDashTest,
	"RefactorTactics.Environment.Hazard.PlaybackOnDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardPlaybackOnDashTest::RunTest(const FString&)
{
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::Fire);
	SpawnFwMap(World, Surfaces);

	ARTUnit* Unit = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Inerte = SpawnFwUnit(World, 1, FRTCellId(-3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("avversario"), Inerte) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}
	Unit->Shield = 0;

	// Uno scatto lineare dal catalogo nello slot dello scatto, e la cella oltre il fuoco: la linea passa
	// per (1,0). `Fire` non dichiara `bBlocksDashCharge` — solo `Rough` lo fa — quindi lo scatto la attraversa.
	const int32 Scatto = RTAbilityFixtures::AddCoreAbilityInSlot(Unit, TEXT("Action.Dodge"), 3);
	// ⚠️ `TestTrue` su un confronto e non `TestNotEqual`: `INDEX_NONE` e' un enum senza nome, e gli overload
	// `float`/`double` di quella funzione lo rendono una chiamata ambigua (`error C2668`).
	if (!TestTrue(TEXT("premessa: lo scatto e' nel kit"), Scatto != INDEX_NONE))
	{
		DestroyFwWorld(World);
		return false;
	}
	Unit->PlannedDashAbility = Scatto;
	Unit->PlannedDashCell = FRTCellId(2, 0);
	Unit->PlannedCell = FRTCellId(2, 0); // niente Move volontario oltre lo scatto

	RunFwTurn(TM);

	if (!TestTrue(TEXT("premessa: lo scatto ha attraversato il fuoco"), Unit->Cell == FRTCellId(2, 0)))
	{
		DestroyFwWorld(World);
		return false;
	}

	// ⚠️ Due eventi anche qui, e per la ragione misurata in `PlaybackOnMove`: la fiamma nella fase dello
	// scatto, piu' il tick del `Burning` che quella cella ha appena concesso, nel Cleanup dello stesso turno.
	const TArray<FRTResolvedEvent> Eventi = FwHazardEvents(TM);
	if (!TestEqual(TEXT("la fiamma piu' il tick dello status che ne nasce"), Eventi.Num(), 2))
	{
		DestroyFwWorld(World);
		return false;
	}

	// 🔴 Il cuore del test: l'evento della FIAMMA porta la fase dello scatto.
	const FRTResolvedEvent* Fiamma = Eventi.FindByPredicate(
		[](const FRTResolvedEvent& E) { return E.Phase == ERTMatchPhase::Dash; });
	if (!TestNotNull(TEXT("un evento porta la fase Dash"), Fiamma))
	{
		DestroyFwWorld(World);
		return false;
	}
	// ⚠️ **E nessuno porta `Move`**: e' la costante plausibile che un'emissione cablata scriverebbe, ed e'
	// l'unica cosa che questo test distingue dal suo gemello sul movimento.
	TestNull(TEXT("e NESSUNO porta Move, che sarebbe la costante plausibile"),
		Eventi.FindByPredicate([](const FRTResolvedEvent& E) { return E.Phase == ERTMatchPhase::Move; }));
	TestEqual(TEXT("il soggetto resta chi subisce"), Fiamma->TargetStableUnitId, Unit->StableUnitId);
	TestEqual(TEXT("e l'Amount resta il nominale"), Fiamma->Amount, 10);

	DestroyFwWorld(World);
	return true;
}

/**
 * 🔴 **Attraversare DUE celle pericolose produce DUE eventi, non uno.**
 *
 * Separa un'emissione per **cella** da una per **unita'**: sono entrambe plausibili guardando il caso a una
 * cella sola, e producono un numero diverso solo qui. Il danno e' per cella — `ApplyTerrainOnEnterEffects`
 * itera `Entered` — e la presentazione deve poter mostrare due colpi invece di uno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardPlaybackTwoCellsTwoEventsTest,
	"RefactorTactics.Environment.Hazard.TwoCellsTwoEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardPlaybackTwoCellsTwoEventsTest::RunTest(const FString&)
{
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::Fire);
	Surfaces.Add(FRTCellId(2, 0), ERTHexSurface::Fire);
	SpawnFwMap(World, Surfaces);

	ARTUnit* Unit = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Inerte = SpawnFwUnit(World, 1, FRTCellId(-3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("avversario"), Inerte) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}
	// 20 danni in arrivo su due celle: senza margine l'unita' morirebbe a meta' strada e il test misurerebbe
	// un'altra cosa. La salute si alza QUI e non nel catalogo, che resta il bilanciamento vero.
	Unit->Shield = 0;
	Unit->MaxHealth = 100;
	Unit->Health = 100;

	Unit->PlannedCell = FRTCellId(2, 0);
	RunFwTurn(TM);

	if (!TestTrue(TEXT("premessa: l'unita' ha attraversato entrambe le celle ed e' viva"),
		Unit->Cell == FRTCellId(2, 0) && Unit->IsAlive()))
	{
		DestroyFwWorld(World);
		return false;
	}

	// 🔴 **Il cuore: DUE eventi di TERRENO, uno per cella.** Si contano quelli della fase del movimento e
	// non il totale: il turno ne porta un terzo, il tick del `Burning` nel Cleanup, che le due celle hanno
	// concesso — un fatto solo, per uno status solo, e non e' cio' che questo test misura.
	//
	// ⚠️ La prima stesura contava il totale e attendeva `2`: la suite ha riportato `3`. Contare il totale
	// avrebbe legato questo test al numero di status che il fuoco concede, che e' un'altra domanda.
	const TArray<FRTResolvedEvent> Eventi = FwHazardEvents(TM);
	TArray<FRTResolvedEvent> DaTerreno = Eventi.FilterByPredicate(
		[](const FRTResolvedEvent& E) { return E.Phase == ERTMatchPhase::Move; });
	if (!TestEqual(TEXT("due celle pericolose attraversate producono DUE eventi di terreno"),
		DaTerreno.Num(), 2))
	{
		DestroyFwWorld(World);
		return false;
	}
	// ⚠️ E sono due celle DIVERSE: due eventi sulla stessa cella sarebbero un doppio invio, non due colpi.
	TestNotEqual(TEXT("e i due eventi nominano celle diverse"), DaTerreno[0].Origin, DaTerreno[1].Origin);
	TestEqual(TEXT("entrambi portano il danno nominale"), DaTerreno[0].Amount, 10);
	TestEqual(TEXT("entrambi portano il danno nominale"), DaTerreno[1].Amount, 10);

	// La coerenza con l'altro canale regge anche sul numero, contando TUTTI i danni ambientali del turno.
	TestEqual(TEXT("e il TurnLog ne registra quanti la timeline"),
		CountFwTerrainDamageEntries(TM), Eventi.Num());

	DestroyFwWorld(World);
	return true;
}

/**
 * 🔴 **Un colpo normale non emette `HazardDamage`, ed e' il rischio proprio del punto unico.**
 *
 * L'emissione sta in `AppendLogEntry`, che **ogni** voce di log attraversa: un discriminante sbagliato
 * riempirebbe la timeline di `HazardDamage` per ogni attacco, e i test qui sopra **passerebbero lo stesso**
 * — troverebbero i loro eventi in mezzo al rumore. E' il gemello esatto di
 * `RefactorTactics.Status.PlaybackIgnoresNonStatusEntries`, che sorveglia lo stesso punto per l'altro tipo.
 *
 * ⚠️ **Anti-vacuita': si verifica prima che il colpo sia davvero avvenuto.** Senza, «zero eventi» sarebbe
 * vero anche per un turno in cui non e' successo niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardPlaybackIgnoresPlainAttackTest,
	"RefactorTactics.Environment.Hazard.AttackDoesNotEmitHazardDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardPlaybackIgnoresPlainAttackTest::RunTest(const FString&)
{
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// Nessun terreno pericoloso: su un'arena pulita l'unico danno del turno e' il colpo.
	SpawnFwMap(World, TMap<FRTCellId, ERTHexSurface>());

	ARTUnit* Attaccante = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Bersaglio = SpawnFwUnit(World, 1, FRTCellId(1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("attaccante"), Attaccante) || !TestNotNull(TEXT("bersaglio"), Bersaglio)
		|| !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}
	Bersaglio->Shield = 0; // lo scudo assorbirebbe il colpo e la premessa non sarebbe piu' verificabile

	const int32 HpPrima = Bersaglio->Health;
	Attaccante->PlannedAbilityIndex = 0; // l'attacco base
	Attaccante->PlannedAttackTarget = Bersaglio;
	Bersaglio->PlannedCell = Bersaglio->Cell;

	RunFwTurn(TM);

	// Anti-vacuita': il colpo dev'essere arrivato, o «zero eventi» non prova niente.
	if (!TestTrue(TEXT("premessa: il colpo e' andato a segno"), Bersaglio->Health < HpPrima))
	{
		DestroyFwWorld(World);
		return false;
	}

	// 🔴 Il cuore: un danno inflitto da un'unita' non e' danno ambientale, e non deve arrivare qui.
	TestEqual(TEXT("un colpo normale non emette nessun HazardDamage"), FwHazardEvents(TM).Num(), 0);

	DestroyFwWorld(World);
	return true;
}

/**
 * 🔴 **Il `Status.Burning` del Cleanup produce il proprio evento, e con la fase giusta.**
 *
 * E' il sito che `ApplyTerrainOnEnterEffects` **non** copre: vive in `LockInAndResolve`, fuori da quella
 * funzione. Se l'emissione fosse stata scritta accanto alla voce come la issue proponeva, questo caso
 * sarebbe rimasto scoperto — ed e' il motivo per cui il punto unico e' stato preferito.
 *
 * ⚠️ **La fase e' `Cleanup`, e li' il membro `Phase` del TurnManager vale `Planning`**: e' la trappola
 * documentata in `ApplyTerrainOnEnterEffects`, e questo test la sorveglia sul canale nuovo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardPlaybackBurningInCleanupTest,
	"RefactorTactics.Environment.Hazard.BurningInCleanupEmitsEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardPlaybackBurningInCleanupTest::RunTest(const FString&)
{
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	// Nessun fuoco sulla mappa: il `Burning` si concede a mano, cosi' l'unico danno ambientale del turno e'
	// quello del Cleanup e il conteggio non va sceverato.
	SpawnFwMap(World, TMap<FRTCellId, ERTHexSurface>());

	ARTUnit* Unit = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Inerte = SpawnFwUnit(World, 1, FRTCellId(-3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("avversario"), Inerte) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}
	Unit->Shield = 0;
	Unit->ApplyStatus(TAG_Status_Burning, /*Turns=*/ 2);
	if (!TestTrue(TEXT("premessa: l'unita' brucia"), Unit->HasStatus(TAG_Status_Burning)))
	{
		DestroyFwWorld(World);
		return false;
	}

	RunFwTurn(TM);

	const TArray<FRTResolvedEvent> Eventi = FwHazardEvents(TM);
	if (!TestEqual(TEXT("il tick di Burning produce UN evento"), Eventi.Num(), 1))
	{
		DestroyFwWorld(World);
		return false;
	}
	// 🔴 Il cuore: la fase e' quella vera del Cleanup, non il `Planning` del membro.
	TestEqual(TEXT("la fase e' Cleanup"), Eventi[0].Phase, ERTMatchPhase::Cleanup);
	TestNotEqual(TEXT("e NON e' Planning, che e' cio' che il membro Phase direbbe"),
		Eventi[0].Phase, ERTMatchPhase::Planning);
	TestEqual(TEXT("il soggetto e' chi brucia"), Eventi[0].TargetStableUnitId, Unit->StableUnitId);
	// ⚠️ 8, non 10: il Cleanup e l'ingresso hanno danni diversi, e l'evento porta quello della sua causa.
	TestEqual(TEXT("l'Amount e' il danno del Burning, non quello dell'ingresso"),
		Eventi[0].Amount, URTCombatLibrary::BurningCleanupDamage);

	DestroyFwWorld(World);
	return true;
}

/**
 * 🔴 **Un turno il cui unico fatto e' un danno da terreno si conclude SENZA beat di playback.**
 *
 * ⚠️ **E' un effetto che l'aggiunta del produttore introduce, e che nessun altro test vedrebbe.** Il ramo
 * finale di `LockInAndResolve` e' `if (bEnablePlayback && ResolvedTimeline.Num() > 0)`: prima di `#2460` un
 * turno cosi' andava a `ConcludeTurn()` **diretto**, adesso passa da `BeginPlayback()`. Misurato innocuo —
 * `PlaybackPhases` si costruisce solo da `MoveAnims` / `PlaybackAttacks` / le spinte / il Prep, e
 * `HazardDamage` non alimenta nessuno dei quattro, quindi la funzione conclude e ritorna.
 *
 * 🔑 **Ma innocuo per una coincidenza, ed e' la coincidenza che questo test pinna.** Il giorno in cui
 * `#2455` dara' una durata a questo evento, un turno di solo fuoco comincerebbe ad aspettare senza che
 * nulla lo dica. `IsResolving()` e' il segnale: e' `bIsResolving`, che `BeginPlayback` accende **dopo** la
 * sua uscita anticipata.
 *
 * ⚠️ **Niente `RunFwTurn` qui**: quella funzione tichetta finche' la risoluzione non finisce, e
 * nasconderebbe proprio la differenza che si vuole misurare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardOnlyTurnConcludesImmediatelyTest,
	"RefactorTactics.Environment.Hazard.OnlyHazardTurnConcludesImmediately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardOnlyTurnConcludesImmediatelyTest::RunTest(const FString&)
{
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	SpawnFwMap(World, TMap<FRTCellId, ERTHexSurface>());

	ARTUnit* Unit = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Inerte = SpawnFwUnit(World, 1, FRTCellId(-3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("avversario"), Inerte) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}
	// Il playback ACCESO: con `bEnablePlayback = false` il ramo non verrebbe nemmeno valutato, e il test
	// misurerebbe la scorciatoia invece della cosa.
	TM->bEnablePlayback = true;
	Unit->Shield = 0;
	Unit->ApplyStatus(TAG_Status_Burning, /*Turns=*/ 2);

	// Nessuno si muove e nessuno colpisce: l'unico fatto del turno e' il tick di `Burning`.
	Unit->PlannedCell = Unit->Cell;
	Unit->PlannedAbilityIndex = INDEX_NONE;
	Inerte->PlannedCell = Inerte->Cell;
	Inerte->PlannedAbilityIndex = INDEX_NONE;

	TM->LockInAndResolve();

	// Anti-vacuita': l'evento dev'esserci davvero, o «conclude subito» sarebbe vero per il motivo banale
	// che la timeline e' vuota — cioe' il comportamento di PRIMA della modifica.
	if (!TestTrue(TEXT("premessa: il turno ha prodotto almeno un HazardDamage"),
		FwHazardEvents(TM).Num() > 0))
	{
		DestroyFwWorld(World);
		return false;
	}

	// 🔴 Il cuore: la timeline non e' vuota, eppure non c'e' niente da mostrare.
	TestFalse(TEXT("un turno di solo danno ambientale non apre un playback"), TM->IsResolving());

	DestroyFwWorld(World);
	return true;
}

/**
 * 🔴 **Chi MUORE per danno ambientale lascia comunque il suo evento, e l'esito e' distinguibile.**
 *
 * ⚠️ **La distinzione non vive sulla timeline, e questo test dichiara dove vive.** `FRTResolvedEvent` non
 * porta un esito: `Amount` e' il danno **nominale**, identico che il colpo uccida o no. Chi muore per hazard
 * non emette nemmeno `Defeated` — quello lo scrive solo `ResolveCombatPasses`, da `NewlyDefeated` calcolato
 * sul Blast — e resta coperto dal catch-all di `ConcludeTurn`.
 *
 * 🔑 **Distinguibile SUI DUE CANALI insieme**: l'evento dice che il danno c'e' stato, e la voce di TurnLog
 * gemella porta `Lethal` dove quella di un danno non letale porta `Hit`. Il test asserisce entrambi sullo
 * stesso fatto — che e' anche il modo in cui si accorgerebbe se un giorno divergessero.
 *
 * ⛔ **Dare un beat proprio alla morte da hazard sarebbe presentazione**, cioe' `#2455`: fuori dallo scope
 * di `#2460`, che consegna il dato e non il disegno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHazardPlaybackLethalIsDistinguishableTest,
	"RefactorTactics.Environment.Hazard.LethalIsDistinguishable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHazardPlaybackLethalIsDistinguishableTest::RunTest(const FString&)
{
	UWorld* World = MakeFwWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	TMap<FRTCellId, ERTHexSurface> Surfaces;
	Surfaces.Add(FRTCellId(1, 0), ERTHexSurface::Fire);
	SpawnFwMap(World, Surfaces);

	ARTUnit* Unit = SpawnFwUnit(World, 0, FRTCellId(0, 0));
	ARTUnit* Inerte = SpawnFwUnit(World, 1, FRTCellId(-3, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("avversario"), Inerte) || !TestNotNull(TEXT("TM"), TM))
	{
		DestroyFwWorld(World);
		return false;
	}
	// Meno HP del danno d'ingresso: i 10 del fuoco la uccidono.
	Unit->Shield = 0;
	Unit->Health = 4;

	Unit->PlannedCell = FRTCellId(1, 0);
	RunFwTurn(TM);

	if (!TestFalse(TEXT("premessa: l'unita' e' morta nel fuoco"), Unit->IsAlive()))
	{
		DestroyFwWorld(World);
		return false;
	}

	// (1) L'evento c'e' lo stesso: morire non lo cancella.
	const TArray<FRTResolvedEvent> Eventi = FwHazardEvents(TM);
	if (!TestEqual(TEXT("anche un danno letale lascia il suo evento"), Eventi.Num(), 1))
	{
		DestroyFwWorld(World);
		return false;
	}
	TestEqual(TEXT("con il soggetto giusto"), Eventi[0].TargetStableUnitId, Unit->StableUnitId);
	// ⚠️ Il NOMINALE, non i 4 HP realmente tolti: e' la convenzione, e cambiarla scollegherebbe i due canali.
	TestEqual(TEXT("e l'Amount resta il nominale, non gli HP persi"), Eventi[0].Amount, 10);

	// (2) L'esito letale vive sull'altro canale, e ci si legge.
	bool bLetale = false;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (URTTurnLogLibrary::IsEnvironmentalDamage(E)
			&& static_cast<ERTCombatOutcome>(E.Outcome) == ERTCombatOutcome::Lethal)
		{
			bLetale = true;
		}
	}
	TestTrue(TEXT("la voce gemella nel TurnLog dichiara l'esito Lethal"), bLetale);

	// (3) ⛔ E la morte da hazard NON ha un beat di playback: dichiarato, non subito. Il conteggio si chiede
	// alla timeline INTERA — cercare un `Defeated` fra gli eventi gia' filtrati su `HazardDamage` darebbe
	// zero per costruzione, che e' un asserto vacuo travestito da verifica.
	//
	// 🔑 Se un giorno qualcuno aggiungera' quel beat (`#2455`), questa riga diventa rossa e lo obbliga a
	// rivedere la clausola della tabella di binding, che oggi dichiara esattamente il contrario.
	TestEqual(TEXT("nessun Defeated in tutto il turno: la morte da hazard resta al catch-all"),
		TM->ResolvedEventCountOfTypeForTest(ERTResolvedEventType::Defeated), 0);

	DestroyFwWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
