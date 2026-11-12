#include "Misc/AutomationTest.h"
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/Resolution/RTFacingLibrary.h" // ERTRelativeDirection: il lato che la voce direzionale porta in `Amount`
#include "Turn/Log/RTTurnLogLibrary.h" // IsSubjectTheSufferer: la tassonomia del soggetto sta in un posto solo
#include "Turn/Log/RTTurnLog.h"
#include "EngineUtils.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPointerInteraction.h"
#include "Tests/Shared/RTUnitPlanTestAccess.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * CP 18.2 — `Ivrin.InterceptShot` come **Predictive Action**, verificata sul PERCORSO REALE.
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
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->SetPlannedCell(Cell);
		// [D-224] Lo scudo base sta a 0 in questo file: qui si misura una RIDUZIONE di danno, e sommarci
		// una costante di bilanciamento renderebbe l'asserto illeggibile ("15" diventerebbe "15 piu' 5") e
		// legherebbe questi test al valore del base. Chi vuole lo scudo se lo da' esplicitamente.
		U->Shield = 0;
		return U;
	}

	/**
	 * Arma `Ivrin.InterceptShot` sulla cella dichiarata. L'abilita' entra nello slot 3 e non in coda:
	 * `AbilityCooldowns` e' dimensionato sul conteggio originale e non si allarga da solo.
	 */
	bool ArmPredIntercept(ARTUnit* Shooter, const FRTCellId& LockedCell, int32 SlotIndex = 3)
	{
		URTHeroData* Ivrin = URTHeroCatalogLibrary::MakeIvrin();
		if (!Shooter || !Ivrin || !Shooter->Abilities.IsValidIndex(SlotIndex)) { return false; }

		Shooter->Abilities[SlotIndex] = Ivrin->Actions[1]; // InterceptShot
		Shooter->SetPlannedMainAction(SlotIndex);
		Shooter->DeclareAttackOnCell(LockedCell);   // mira a una CELLA, non a un'unita': e' il punto
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

	constexpr int32 PredInterceptDamage = 160; // dal catalogo eroi, non un numero di questo file

	/** L'indice di `Hero.Ivrin.InterceptShot` nel kit dell'unita', o `INDEX_NONE`: cercato per id, non scritto. */
	int32 PredInterceptIndex(const ARTUnit* Unit)
	{
		for (int32 I = 0; Unit && I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* A = Unit->GetAbility(I);
			if (A && A->Def.ActionId == TEXT("Hero.Ivrin.InterceptShot")) { return I; }
		}
		return INDEX_NONE;
	}

	/** Il piano dopo un click, in una riga: e' la misura di `#4088`, e la si legge anche quando il test e' verde. */
	FString PredPlanAfterClick(const ARTUnit* Unit)
	{
		return FString::Printf(TEXT("PlannedAbilityIndex=%d bAttackTargetsCell=%d PlannedAttackCell=%s PlannedAttackTarget=%s waypoint=%d"),
			Unit->PlannedAbilityIndex, Unit->bAttackTargetsCell ? 1 : 0, *Unit->PlannedAttackCell.ToString(),
			Unit->PlannedAttackTarget ? *Unit->PlannedAttackTarget->GetName() : TEXT("nessuno"),
			Unit->PlannedWaypoints.Num());
	}
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
	Runner->SetPlannedCell(FRTCellId(1, 0));
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
		TestEqual(TEXT("e l'identita' dell'azione"), Entry->ActionId, FName(TEXT("Hero.Ivrin.InterceptShot")));
	}

	DestroyPredWorld(World);
	return true;
}

/**
 * **Anche il colpo al boundary porta la voce direzionale** (`#2128`, [D-126]).
 *
 * 🔴 **Questa e' la QUARTA famiglia, e per un giorno e' stata l'unica scoperta.** `#2128` ha esteso
 * `ERTFacingOutcome::HitCameFromSide` a contrattacchi e Overwatch dichiarando — in ADR-0005, nel codice e in
 * `spec-turnlog.md` — che «un'assenza significa una cosa sola». Non era vero: `ResolvePredictiveBoundary`
 * risolveva colpi `ERTDamageSource::Direct` senza emettere niente. L'ha trovato una **code review dopo il
 * merge**, non un test — perche' nessun oracolo copriva «predictive + voce direzionale». Questo test e'
 * quell'oracolo.
 *
 * ⚠️ **La cella del difensore e' quella BLOCCATA, non `Victim->Cell`**: `ResolvePredictiveBoundary` gira
 * prima di `PlaceOnCell`, quindi l'Actor porta ancora la cella di partenza del turno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveTracesIncomingSideTest,
	"RefactorTactics.Predictive.InterceptTracesTheSideItCameFrom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveTracesIncomingSideTest::RunTest(const FString&)
{
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	const FRTCellId Locked(0, 0);

	ARTUnit* Runner = SpawnPredUnit(World, /*Team*/ 0, FRTCellId(-1, 0));
	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 1, FRTCellId(0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TestNotNull(TEXT("Runner"), Runner) || !TestNotNull(TEXT("Shooter"), Shooter)
		|| !TestNotNull(TEXT("TM"), TM) || !TestTrue(TEXT("intercetto armato"), ArmPredIntercept(Shooter, Locked)))
	{
		DestroyPredWorld(World);
		return false;
	}

	// Il Runner guarda a `E` — il default — e non ha azioni con bersaglio che lo rigirino.
	if (!TestTrue(TEXT("premessa: il Runner guarda a E"), Runner->Facing == ERTHexDirection::E))
	{
		DestroyPredWorld(World);
		return false;
	}

	Runner->SetPlannedCell(FRTCellId(1, 0));
	RunPredTurn(TM);

	// PREMESSA: senza il colpo non c'e' nessuna voce da cercare, e il test resterebbe verde sul nulla.
	const FRTTurnLogEntry* Pred = FindPredEntry(TM);
	if (!TestNotNull(TEXT("premessa: la previsione ha colpito"), Pred)
		|| !TestEqual(TEXT("premessa: come TriggerMatched"), Pred->Outcome,
			static_cast<uint8>(ERTPredictiveOutcome::TriggerMatched)))
	{
		DestroyPredWorld(World);
		return false;
	}

	const FRTTurnLogEntry* FromSide = nullptr;
	int32 Direzionali = 0;
	for (const FRTTurnLogEntry& E : TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Facing
			&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::HitCameFromSide))
		{
			++Direzionali;
			FromSide = &E;
		}
	}

	TestEqual(TEXT("un colpo risolto, una voce direzionale"), Direzionali, 1);

	if (TestNotNull(TEXT("il boundary predittivo ha lasciato la voce"), FromSide))
	{
		// Lo Shooter e' in (0,1); rispetto alla cella bloccata (0,0) cade nello spicchio `SE` (5). Il Runner
		// guarda a `E` (0), quindi l'indice relativo e' (5 - 0 + 6) % 6 = 5.
		//
		// ⚠️ **5 e non 0, deliberatamente**: `Front` vale zero, che e' anche il valore di un `Amount` mai
		// scritto — un'assertion su `Front` passerebbe anche senza produttore.
		TestEqual(TEXT("il colpo e' arrivato da FrontRight"),
			FromSide->Amount, static_cast<int32>(ERTRelativeDirection::FrontRight));
		// 🔴 La privacy: la cella dello sparatore non compare, e i due campi portano il difensore.
		TestTrue(TEXT("SrcCell e TgtCell portano entrambi il difensore"), FromSide->SrcCell == FromSide->TgtCell);
		// E la cella e' quella BLOCCATA, cioe' dove il colpo e' avvenuto — non quella di partenza del Runner.
		TestTrue(TEXT("ed e' la cella bloccata, non quella di partenza"), FromSide->SrcCell == Locked);
		// Il soggetto e' chi ha subito, non chi ha sparato.
		TestEqual(TEXT("il soggetto della voce e' il Runner"), FromSide->UnitId, Runner->StableUnitId);
		TestTrue(TEXT("e la tassonomia lo sa"), URTTurnLogLibrary::IsSubjectTheSufferer(*FromSide));
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
	Runner->SetPlannedCell(Detour);
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

	Sitter->SetPlannedCell(Locked); // resta fermo
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

	Runner->SetPlannedCell(FRTCellId(1, 0));
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

// ======================================================================================================
// `#4088` — la previsione la arma il GIOCATORE, dal controller vero
// ======================================================================================================
//
// I test sopra scrivono il piano a mano (`ArmPredIntercept`): provano il turno, non chi gli da' la cella. Questi due
// partono dal controller — l'azione armata dalla barra, il click — e arrivano al TurnLog attraverso il lock-in.

/**
 * **Il giocatore arma `InterceptShot` cliccando la cella** (`#4088`): il puntatore chiede una cella, il click la scrive
 * nel piano, e al lock-in la previsione colpisce chi ci entra.
 *
 * 🔴 **Misurato il 2026-11-12 sul codice di `08d83547e`, prima della correzione**: il puntatore chiedeva un'unita', e il
 * click sulla cella vuota diventava un waypoint del tiratore (`PlannedAbilityIndex=-1 bAttackTargetsCell=0 waypoint=1`).
 * Nessuna previsione al lock-in, e chi attraversava la cella incassava 0. La riga `dopo il click` del log lo dice.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictivePlayerLocksTheCellTest,
	"RefactorTactics.Predictive.PlayerLocksTheCellItClicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictivePlayerLocksTheCellTest::RunTest(const FString&)
{
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	const FRTCellId Locked(0, 0);

	// La scena di `InterceptCellHit`, con le squadre scambiate: il tiratore e' del giocatore.
	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 0, FRTCellId(0, 1)); // adiacente alla cella da bloccare
	ARTUnit* Runner = SpawnPredUnit(World, /*Team*/ 1, FRTCellId(-1, 0));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	const int32 Intercetto = PredInterceptIndex(Shooter);
	if (!TestNotNull(TEXT("Runner"), Runner) || !TestNotNull(TEXT("Shooter"), Shooter) || !TestNotNull(TEXT("TM"), TM)
		|| !TestNotNull(TEXT("PC"), PC)
		|| !TestNotEqual(TEXT("premessa: InterceptShot e' nel kit di Ivrin"), Intercetto, (int32)INDEX_NONE))
	{
		DestroyPredWorld(World);
		return false;
	}

	PC->SelectActorForTest(Shooter);
	PC->SelectAbilityForCurrentForTest(Intercetto);
	TestEqual(TEXT("armata, InterceptShot chiede una CELLA"), PC->GetPointerTargetKind(), ERTPointerTargetKind::Cell);

	PC->HandleClickOnCellForTest(Locked);
	AddInfo(FString::Printf(TEXT("dopo il click sulla cella %s: %s"), *Locked.ToString(), *PredPlanAfterClick(Shooter)));
	TestEqual(TEXT("il click pianifica InterceptShot"), Shooter->PlannedAbilityIndex, Intercetto);
	TestTrue(TEXT("con il bersaglio su una cella"), Shooter->bAttackTargetsCell);
	TestTrue(TEXT("quella cliccata"), Shooter->PlannedAttackCell == Locked);
	TestEqual(TEXT("e non come una destinazione del tiratore"), Shooter->PlannedWaypoints.Num(), 0);

	// Il Runner ATTRAVERSA la cella bloccata, come in `InterceptCellHit`.
	Runner->SetPlannedCell(FRTCellId(1, 0));
	const int32 Before = Runner->Health;

	RunPredTurn(TM);

	const FRTTurnLogEntry* Entry = FindPredEntry(TM);
	if (TestNotNull(TEXT("al lock-in la previsione si arma: il TurnLog la registra"), Entry))
	{
		TestEqual(TEXT("come TriggerMatched"), Entry->Outcome, static_cast<uint8>(ERTPredictiveOutcome::TriggerMatched));
		TestTrue(TEXT("sulla cella cliccata"), Entry->TgtCell == Locked);
	}
	TestEqual(TEXT("chi e' entrato incassa i danni del catalogo"), Before - Runner->Health, PredInterceptDamage);

	DestroyPredWorld(World);
	return true;
}

/**
 * **Il click su un'unita' mira la SUA cella** (`#4088`, spec del puntatore §4.1 conseguenza 2: in `Targeting`/`Cell`
 * un'unita' sopra la cella non la ruba). La previsione si arma, e manca chi c'era gia': colpisce chi ENTRA
 * (`Predictive.CrossingIsNotPresence`).
 *
 * 🔴 **Misurato il 2026-11-12 sul codice di `08d83547e`, prima della correzione**: il click sul nemico pianificava
 * InterceptShot con un bersaglio-UNITA' (`bAttackTargetsCell=0`, `PlannedAttackTarget` il nemico), e al lock-in il turno
 * non armava nessuna previsione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictivePlayerClicksAUnitTest,
	"RefactorTactics.Predictive.PlayerClickOnAUnitLocksItsCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictivePlayerClicksAUnitTest::RunTest(const FString&)
{
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 0, FRTCellId(0, 1));
	ARTUnit* Enemy = SpawnPredUnit(World, /*Team*/ 1, FRTCellId(0, 0)); // accanto, e fermo
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	const int32 Intercetto = PredInterceptIndex(Shooter);
	if (!TestNotNull(TEXT("Enemy"), Enemy) || !TestNotNull(TEXT("Shooter"), Shooter) || !TestNotNull(TEXT("TM"), TM)
		|| !TestNotNull(TEXT("PC"), PC)
		|| !TestNotEqual(TEXT("premessa: InterceptShot e' nel kit di Ivrin"), Intercetto, (int32)INDEX_NONE))
	{
		DestroyPredWorld(World);
		return false;
	}

	PC->SelectActorForTest(Shooter);
	PC->SelectAbilityForCurrentForTest(Intercetto);

	// La matrice del click vero, non `HandleClickOnUnitForTest`: e' cio' che il raycast su una mesh raggiunge.
	TestTrue(TEXT("il click sul nemico e' consumato"), PC->DispatchUnitClickForTest(Enemy, Shooter));
	AddInfo(FString::Printf(TEXT("dopo il click sul nemico in %s: %s"), *Enemy->Cell.ToString(), *PredPlanAfterClick(Shooter)));
	TestEqual(TEXT("il click pianifica InterceptShot"), Shooter->PlannedAbilityIndex, Intercetto);
	TestTrue(TEXT("con il bersaglio su una cella"), Shooter->bAttackTargetsCell);
	TestTrue(TEXT("quella del nemico cliccato"), Shooter->PlannedAttackCell == Enemy->Cell);
	TestNull(TEXT("e non lui"), Shooter->PlannedAttackTarget.Get());

	const FRTCellId Bloccata = Enemy->Cell;
	RunPredTurn(TM);

	const FRTTurnLogEntry* Entry = FindPredEntry(TM);
	if (TestNotNull(TEXT("al lock-in la previsione si arma: il TurnLog la registra"), Entry))
	{
		TestTrue(TEXT("sulla cella del nemico cliccato"), Entry->TgtCell == Bloccata);
		TestEqual(TEXT("e chi c'era gia' non e' chi entra: PredictionWhiffed"), Entry->Outcome,
			static_cast<uint8>(ERTPredictiveOutcome::PredictionWhiffed));
	}

	DestroyPredWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
