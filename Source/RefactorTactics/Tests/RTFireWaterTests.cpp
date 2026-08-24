#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

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
		URTActionData* Action = NewObject<URTActionData>(Caster);
		Action->Def = URTCatalogLibrary::FindCoreAction(FName(ActionId));
		Action->RangeCells = Action->Def.RangeCells;
		Action->CooldownTurns = Action->Def.CooldownTurns;
		Action->Power = URTCatalogLibrary::FirstDamage(Action->Def);
		Caster->Abilities[3] = Action;
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

#endif // WITH_DEV_AUTOMATION_TESTS
