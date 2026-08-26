#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTTurnLog.h"
#include "EngineUtils.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 18.2 — `Wraith.InterceptShot` come **Predictive Action**, verificata sul PERCORSO REALE.
 *
 * Questi tre test girano attraverso `ARTTurnManager::LockInAndResolve`, non sulla libreria pura. La ragione
 * e' [D-036](../../../docs/decisions/RT_PDR_00_Decision_Log.md): un test che non attraversa il percorso reale
 * puo' restare verde per sempre mentre la regola non esiste — era successo con il bonus Wet, che passava
 * senza mai passare dal `TurnManager`. La libreria pura ha gia' i suoi quattro test (CP 18.1); questi
 * verificano che qualcuno la CHIAMI.
 *
 * Prefissi `Pred*` negli helper: unity build, i namespace anonimi si fondono fra file.
 */
namespace
{
	UWorld* MakePredWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyPredWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	void SpawnPredMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);

		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
	}

	ARTUnit* SpawnPredUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
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

	/**
	 * Arma `Wraith.InterceptShot` sulla cella dichiarata. L'abilita' entra nello slot 3 e non in coda:
	 * `AbilityCooldowns` e' dimensionato sul conteggio originale e non si allarga da solo.
	 */
	bool ArmPredIntercept(ARTUnit* Shooter, const FRTCellId& LockedCell, int32 SlotIndex = 3)
	{
		URTHeroData* Wraith = URTHeroCatalogLibrary::MakeWraith();
		if (!Shooter || !Wraith || !Shooter->Abilities.IsValidIndex(SlotIndex)) { return false; }

		Shooter->Abilities[SlotIndex] = Wraith->Actions[1]; // InterceptShot
		Shooter->PlannedAbilityIndex = SlotIndex;
		Shooter->bAttackTargetsCell = true;   // mira a una CELLA, non a un'unita': e' il punto
		Shooter->PlannedAttackCell = LockedCell;
		Shooter->PlannedAttackTarget = nullptr;
		return true;
	}

	void RunPredTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** La prima voce predittiva del TurnLog, o nullptr. */
	const FRTTurnLogEntry* FindPredEntry(const ARTTurnManager* TM)
	{
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category == ERTLogCategory::Predictive) { return &E; }
		}
		return nullptr;
	}

	constexpr int32 PredInterceptDamage = 16; // dal catalogo eroi, non un numero di questo file
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveInterceptHitTest,
	"RefactorTactics.Predictive.InterceptCellHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveInterceptHitTest::RunTest(const FString&)
{
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	const FRTCellId Locked(0, 0);

	ARTUnit* Runner = SpawnPredUnit(World, /*Team*/ 0, FRTCellId(-1, 0));
	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 1, FRTCellId(0, 1)); // adiacente alla cella bloccata
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Runner"), Runner) || !TestNotNull(TEXT("Shooter"), Shooter)
		|| !TestNotNull(TEXT("TM"), TM) || !TestTrue(TEXT("intercetto armato"), ArmPredIntercept(Shooter, Locked)))
	{
		DestroyPredWorld(World);
		return false;
	}

	// Il Runner ATTRAVERSA la cella bloccata e vorrebbe proseguire fino a (1,0).
	Runner->PlannedCell = FRTCellId(1, 0);
	const int32 Before = Runner->Health;

	RunPredTurn(TM);

	TestEqual(TEXT("chi e' entrato incassa i danni del catalogo"), Before - Runner->Health, PredInterceptDamage);
	// E si FERMA sulla cella bloccata: il troncamento e' meta' dell'azione, e senza di esso la previsione
	// azzeccata costerebbe solo qualche punto vita a chi passa comunque.
	TestTrue(TEXT("e si ferma SULLA cella bloccata, non oltre"), Runner->Cell == Locked);

	const FRTTurnLogEntry* Entry = FindPredEntry(TM);
	if (TestNotNull(TEXT("il TurnLog registra la previsione"), Entry))
	{
		TestEqual(TEXT("come TriggerMatched"), Entry->Outcome,
			static_cast<uint8>(ERTPredictiveOutcome::TriggerMatched));
		TestTrue(TEXT("con la cella su cui si e' scommesso"), Entry->TgtCell == Locked);
		TestEqual(TEXT("e l'identita' dell'azione"), Entry->ActionId, FName(TEXT("Hero.Wraith.InterceptShot")));
	}

	DestroyPredWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveInterceptMissTest,
	"RefactorTactics.Predictive.InterceptCellMiss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveInterceptMissTest::RunTest(const FString&)
{
	// Il gemello del test sopra, ed e' quello che conta: un'implementazione che al boundary cercasse il
	// bersaglio piu' vicino passerebbe `InterceptCellHit` e fallirebbe QUESTO. E' la stessa ragione per cui
	// `Spec.Predictive.WhiffOnEmptyCell` e' stato scritto prima dell'implementazione.
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	const FRTCellId Locked(0, 0);
	const FRTCellId Detour(-1, 1);

	ARTUnit* Runner = SpawnPredUnit(World, /*Team*/ 0, FRTCellId(-1, 0));
	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 1, FRTCellId(0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Runner"), Runner) || !TestNotNull(TEXT("Shooter"), Shooter)
		|| !TestNotNull(TEXT("TM"), TM) || !TestTrue(TEXT("intercetto armato"), ArmPredIntercept(Shooter, Locked)))
	{
		DestroyPredWorld(World);
		return false;
	}

	// Il Runner DEVIA: non passa mai dalla cella prevista.
	Runner->PlannedCell = Detour;
	const int32 Before = Runner->Health;

	RunPredTurn(TM);

	TestEqual(TEXT("chi ha deviato e' illeso"), Runner->Health, Before);
	TestTrue(TEXT("e arriva dove voleva: il whiff non tronca niente"), Runner->Cell == Detour);

	const FRTTurnLogEntry* Entry = FindPredEntry(TM);
	if (TestNotNull(TEXT("il whiff e' registrato, non taciuto"), Entry))
	{
		TestEqual(TEXT("come PredictionWhiffed"), Entry->Outcome,
			static_cast<uint8>(ERTPredictiveOutcome::PredictionWhiffed));
		// La cella c'e' anche quando non e' successo niente: e' l'informazione che spiega il turno.
		TestTrue(TEXT("con la cella su cui si era scommesso"), Entry->TgtCell == Locked);
		TestEqual(TEXT("e nessun danno"), Entry->Amount, 0);
	}

	DestroyPredWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveCrossingNotPresenceTest,
	"RefactorTactics.Predictive.CrossingIsNotPresence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveCrossingNotPresenceTest::RunTest(const FString&)
{
	// La distinzione che il boundary esiste per fare: il colpo prende chi **entra**, non chi **c'e' gia'**.
	// Un'unita' ferma sulla cella prevista non viene colta — non e' entrata, e la previsione riguardava un
	// movimento che non e' avvenuto. Senza questa proprieta' `InterceptShot` diventerebbe un colpo ad area
	// ritardato, cioe' un'altra azione.
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	const FRTCellId Locked(0, 0);

	ARTUnit* Sitter = SpawnPredUnit(World, /*Team*/ 0, Locked); // gia' li', e non si muove
	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 1, FRTCellId(0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Sitter"), Sitter) || !TestNotNull(TEXT("Shooter"), Shooter)
		|| !TestNotNull(TEXT("TM"), TM) || !TestTrue(TEXT("intercetto armato"), ArmPredIntercept(Shooter, Locked)))
	{
		DestroyPredWorld(World);
		return false;
	}

	Sitter->PlannedCell = Locked; // resta fermo
	const int32 Before = Sitter->Health;

	RunPredTurn(TM);

	TestEqual(TEXT("chi era gia' sulla cella NON viene colto"), Sitter->Health, Before);
	TestTrue(TEXT("e resta dov'era"), Sitter->Cell == Locked);

	const FRTTurnLogEntry* Entry = FindPredEntry(TM);
	if (TestNotNull(TEXT("la previsione ha comunque un esito"), Entry))
	{
		// Va a vuoto: la presenza non e' un'entrata, ed e' il whiff a dirlo. Un colpo che scattasse qui
		// renderebbe indistinguibili «ti ho previsto» e «eri li'».
		TestEqual(TEXT("presenza non e' entrata: whiff"), Entry->Outcome,
			static_cast<uint8>(ERTPredictiveOutcome::PredictionWhiffed));
	}

	DestroyPredWorld(World);
	return true;
}

/**
 * Il colpo deciso a un decision boundary e' un TIRO NORMALE: la copertura si applica come per il Blast.
 *
 * 🔴 Fino al 2026-08-25 non lo era, e nessun documento diceva perche' ([#888]): due percorsi — la
 * predittiva e l'Overwatch `FIRE` — chiamavano `ApplyDamage` diretto, saltando `EffectiveCoverReduction`
 * che il Blast usa. Un bersaglio dietro un muro prendeva **danno pieno** da un Overwatch e danno ridotto
 * da un attacco base della stessa arma.
 *
 * La regola scelta e' la coerenza — il brief dice che chi arma *«spara con la propria arma»*, e se l'arma
 * e' la stessa lo sono anche le regole del tiro. Il counterplay del difensore resta la **rotta**: non
 * passare di li'. Chi ci passa comunque puo' pagare meno se si e' coperto.
 *
 * ⚠️ Questo test e' il GEMELLO di `InterceptCellHit`: stessa scena, un muro in piu'. Confrontare i due
 * danni e' cio' che prova la regola — un test che guardasse solo il valore assoluto passerebbe anche se
 * la copertura fosse applicata due volte, o applicata a caso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveCoverAppliesTest,
	"RefactorTactics.Predictive.BoundaryShotRespectsCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveCoverAppliesTest::RunTest(const FString&)
{
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	const FRTCellId Locked(0, 0);

	// Una copertura bassa su OGNI bordo della cella bloccata: quale bordo attraversi il tiro dipende dalla
	// geometria assiale, e fissarne uno solo renderebbe il test dipendente da un dettaglio che non sta
	// verificando. Con tutti i bordi coperti, la riduzione c'e' comunque.
	ARTHexMapActor* MapActor = nullptr;
	for (TActorIterator<ARTHexMapActor> It(World); It; ++It) { MapActor = *It; }
	if (!TestNotNull(TEXT("map actor"), MapActor)) { DestroyPredWorld(World); return false; }

	FRTHexCellData Cell(Locked);
	for (const ERTHexDirection Edge : { ERTHexDirection::E, ERTHexDirection::NE, ERTHexDirection::NW,
		ERTHexDirection::W, ERTHexDirection::SW, ERTHexDirection::SE })
	{
		FRTHexCover Cover;
		Cover.Edge = Edge;
		Cover.Type = ERTHexCoverType::Low;
		Cell.Covers.Add(Cover);
	}
	MapActor->MapAsset->AddOrUpdateCell(Cell);
	MapActor->MapAsset->SortCells();

	ARTUnit* Runner = SpawnPredUnit(World, /*Team*/ 0, FRTCellId(-1, 0));
	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 1, FRTCellId(0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Runner"), Runner) || !TestNotNull(TEXT("Shooter"), Shooter)
		|| !TestNotNull(TEXT("TM"), TM) || !TestTrue(TEXT("intercetto armato"), ArmPredIntercept(Shooter, Locked)))
	{
		DestroyPredWorld(World);
		return false;
	}

	Runner->PlannedCell = FRTCellId(1, 0);
	const int32 Before = Runner->Health;

	// Diagnostica del SETUP, prima di girare il turno: se la riduzione fosse zero gia' qui, il test
	// starebbe misurando una mappa senza copertura invece del percorso del boundary.
	{
		FRTHexCombatUnit A; A.UnitId = 0; A.TeamId = 1; A.Cell = Shooter->Cell; A.Facing = Shooter->Facing;
		FRTHexCombatUnit T; T.UnitId = 1; T.TeamId = 0; T.Cell = Locked;       T.Facing = Runner->Facing;
		const int32 Direct = URTHexCombatLibrary::EffectiveCoverReduction(
			MapActor->MapAsset, A, T, ERTAbilityShape::Single);
		TestTrue(*FString::Printf(TEXT("setup: la copertura riduce davvero (%d), facing bersaglio %d"),
			Direct, static_cast<int32>(Runner->Facing)), Direct > 0);
	}

	RunPredTurn(TM);

	const int32 Taken = Before - Runner->Health;

	// La premessa: il colpo e' arrivato. Senza, un danno ridotto sarebbe indistinguibile da un colpo mancato.
	TestTrue(TEXT("premessa: il colpo e' arrivato"), Taken > 0);

	// E ha subito la copertura: meno del danno pieno che `InterceptCellHit` misura sulla stessa scena
	// senza muro. E' il confronto fra i due test a portare la regola.
	TestTrue(*FString::Printf(
		TEXT("il colpo di boundary subisce la copertura: %d invece dei %d pieni"), Taken, PredInterceptDamage),
		Taken < PredInterceptDamage);

	DestroyPredWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
