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
#include "Combat/RTCombatLibrary.h" // la portata minima di InterceptShot: ClassifyHexTargeting e TargetableRangeCells (#4092)
#include "Map/RTHexLibrary.h" // DirectionBetween: il bordo del muro nelle scene di #4091
#include "Map/Cover/RTHexCoverLibrary.h" // AddCover: la copertura alta che toglie la linea (#4091)
#include "Turn/Actions/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo del rifiuto (#4091)
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

/**
 * **InterceptShot non blocca la propria cella** ([D-610] (2), `#4092`): ha portata minima 1, e il click sulla cella del
 * tiratore e' rifiutato `TooClose` senza scrivere il piano. La cella accanto, lo stesso click, si blocca.
 *
 * 🔴 Prima, con il minimo a 0, quel click bloccava la propria cella: chi restava fermo mancava sempre, e chi si spostava
 * colpiva da distanza 0.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveRefusesOwnCellTest,
	"RefactorTactics.Predictive.InterceptShotRefusesTheShootersOwnCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveRefusesOwnCellTest::RunTest(const FString&)
{
	UWorld* World = MakePredWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
	SpawnPredMap(World);

	ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 0, FRTCellId(0, 1));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	const int32 Intercetto = PredInterceptIndex(Shooter);
	const URTActionData* Azione = Shooter ? Shooter->GetAbility(Intercetto) : nullptr;
	if (!TestNotNull(TEXT("Shooter"), Shooter) || !TestNotNull(TEXT("TM"), TM) || !TestNotNull(TEXT("PC"), PC)
		|| !TestNotNull(TEXT("premessa: InterceptShot e' nel kit di Ivrin"), Azione))
	{
		DestroyPredWorld(World);
		return false;
	}
	TestEqual(TEXT("il catalogo: portata minima 1"), Azione->Def.MinRangeCells, 1);

	const URTHexMapAsset* Mappa = nullptr;
	for (TActorIterator<ARTHexMapActor> It(World); It; ++It) { Mappa = It->MapAsset; }
	TestEqual(TEXT("la propria cella e' troppo vicina"),
		URTCombatLibrary::ClassifyHexTargeting(Mappa, Shooter->Cell, Shooter->Cell, Azione->RangeCells,
			Azione->Def.LineOfSightPolicy, Azione->Def.MinRangeCells),
		ERTHexTargetReason::TooClose);
	TestFalse(TEXT("e la portata a schermo non la contiene"),
		URTCombatLibrary::TargetableRangeCells(Mappa, Shooter->Cell, Azione->RangeCells, Azione->Def.LineOfSightPolicy,
			Azione->Def.MinRangeCells).Contains(Shooter->Cell));

	PC->SelectActorForTest(Shooter);
	PC->SelectAbilityForCurrentForTest(Intercetto);
	PC->HandleClickOnCellForTest(Shooter->Cell);
	AddInfo(FString::Printf(TEXT("dopo il click sulla propria cella: %s"), *PredPlanAfterClick(Shooter)));
	TestFalse(TEXT("il click sulla propria cella non blocca niente"), Shooter->bAttackTargetsCell);
	TestEqual(TEXT("e non pianifica InterceptShot"), Shooter->PlannedAbilityIndex, (int32)INDEX_NONE);

	// Il controllo: la cella accanto, con lo stesso gesto, si blocca. Senza, un click che non fa mai niente resterebbe verde.
	const FRTCellId Accanto(0, 0);
	PC->HandleClickOnCellForTest(Accanto);
	TestTrue(TEXT("la cella accanto si blocca"), Shooter->bAttackTargetsCell && Shooter->PlannedAttackCell == Accanto);

	DestroyPredWorld(World);
	return true;
}

// =========================================================================================================
// [D-610] (1) e (5)(b), `#4091` — portata e linea si rigiudicano: all'armamento e al boundary
// =========================================================================================================

namespace
{
	/** Cio' che un turno con una predittiva lascia, letto per le due sedi di `#4091`. */
	struct FPredRejudge
	{
		bool bValido = false;
		int32 VociPredittive = 0;
		uint8 EsitoPredittivo = MAX_uint8;
		int32 Rifiuti = 0; // voci `Fallback` dell'azione predittiva
		FRTTurnLogEntry Rifiuto;
		int32 Attivazioni = 0; // `AbilityActivated` dell'azione predittiva
		int32 Ricarica = 0;
		int32 DannoAChiEntra = 0;
		FRTCellId DoveFinisceChiEntra;
		FRTCellId DoveFinisceChiArma;
	};

	URTHexMapAsset* PredMapAsset(UWorld* World)
	{
		for (TActorIterator<ARTHexMapActor> It(World); It; ++It) { return It->MapAsset; }
		return nullptr;
	}

	/** Una copertura ALTA sul bordo fra due celle adiacenti: ferma la linea (`BlocksTraversal`) e non l'occupazione. */
	bool PredWall(UWorld* World, const FRTCellId& From, const FRTCellId& To)
	{
		ERTHexDirection Bordo;
		URTHexMapAsset* Map = PredMapAsset(World);
		return Map && URTHexLibrary::DirectionBetween(From, To, Bordo)
			&& URTHexCoverLibrary::AddCover(Map, From, Bordo, ERTHexCoverType::High, /*Integrity*/ 100);
	}

	void ReadPredRejudge(const ARTTurnManager* TM, const ARTUnit* Shooter, int32 SlotIndex, FPredRejudge& Out)
	{
		const FName Id(TEXT("Hero.Ivrin.InterceptShot"));
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.ActionId != Id) { continue; }
			if (E.Category == ERTLogCategory::Predictive) { ++Out.VociPredittive; Out.EsitoPredittivo = E.Outcome; }
			if (E.Category == ERTLogCategory::Fallback) { ++Out.Rifiuti; Out.Rifiuto = E; }
		}
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == ERTResolvedEventType::AbilityActivated && Ev.ActionId == Id) { ++Out.Attivazioni; }
		}
		Out.Ricarica = Shooter->GetAbilityCooldown(SlotIndex);
		Out.DoveFinisceChiArma = Shooter->Cell;
	}
}

/**
 * **All'armamento la cella si rigiudica con la domanda del click** ([D-610] (5)(b), `#4091`): `CLIENT PROPOSES → SERVER
 * VALIDATES`. Il piano e' scritto A MANO, come lo scrivono il bot e lo Scenario Harness: il click non lo avrebbe accettato,
 * e prima di `#4091` il turno lo armava lo stesso — la portata e la linea non le guardava nessuno.
 *
 * 🔑 **Quattro scene dalla stessa cella, e l'unica variabile e' la cella bloccata** (e un muro): a distanza 2 (fuori
 * portata), la propria (sotto il minimo di [D-610] (2)), quella accanto dietro una copertura alta (senza linea), e quella
 * accanto libera — il **controllo positivo**: senza, «nessuna previsione» lo darebbe anche un turno che non arma niente.
 *
 * ⚠️ **Chi paga e' la tabella del Blast** (`PagaSeParteSenzaColpo`): la linea bloccata paga (`R4`), il fuori portata e il
 * sotto-minimo no (`R9`). La ricarica a fine turno lo misura, e il controllo positivo dice che la misura vede un pagamento.
 *
 * Chi entra attraversa la cella bloccata da ovest a est: se il colpo partisse, lo fermerebbe li' con i danni del catalogo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveArmingRejudgesTest,
	"RefactorTactics.Predictive.ArmingRejudgesTheLockedCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveArmingRejudgesTest::RunTest(const FString&)
{
	const FRTCellId DaDove(0, 1);
	auto Gioca = [this, &DaDove](const FRTCellId& Bloccata, bool bMuro, bool bChiEntra) -> FPredRejudge
	{
		FPredRejudge Out;
		UWorld* World = MakePredWorld();
		if (!World) { return Out; }
		SpawnPredMap(World);
		const FRTCellId Partenza(Bloccata.X - 1, Bloccata.Y);
		ARTUnit* Runner = bChiEntra ? SpawnPredUnit(World, /*Team*/ 0, Partenza) : nullptr;
		ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 1, DaDove);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if ((bChiEntra && !Runner) || !Shooter || !TM || !ArmPredIntercept(Shooter, Bloccata)
			|| (bMuro && !PredWall(World, DaDove, Bloccata)))
		{
			DestroyPredWorld(World);
			return Out;
		}
		if (bMuro)
		{
			// Premessa: il muro c'e' davvero per la domanda del click. Senza, la scena «senza linea» misurerebbe un'altra cosa.
			TestEqual(TEXT("premessa: il muro toglie la linea"), URTCombatLibrary::ClassifyHexTargeting(PredMapAsset(World),
				DaDove, Bloccata, /*Range*/ 1, ERTLineOfSightPolicy::Required, /*MinRange*/ 1), ERTHexTargetReason::NoLineOfSight);
		}
		const int32 Prima = Runner ? Runner->Health : 0;
		if (Runner) { Runner->SetPlannedCell(FRTCellId(Bloccata.X + 1, Bloccata.Y)); }

		RunPredTurn(TM);

		ReadPredRejudge(TM, Shooter, /*SlotIndex*/ 3, Out);
		Out.DannoAChiEntra = Runner ? Prima - Runner->Health : 0;
		Out.DoveFinisceChiEntra = Runner ? Runner->Cell : FRTCellId();
		Out.bValido = true;
		DestroyPredWorld(World);
		return Out;
	};

	// ── Il controllo: la cella accanto, libera. Si arma, si attiva, paga, e chi entra viene fermato. ───────
	const FPredRejudge Armata = Gioca(FRTCellId(0, 0), /*bMuro*/ false, /*bChiEntra*/ true);
	if (!TestTrue(TEXT("banco del controllo"), Armata.bValido)) { return false; }
	TestEqual(TEXT("controllo: nessun rifiuto"), Armata.Rifiuti, 0);
	TestEqual(TEXT("controllo: la previsione va a segno"), Armata.EsitoPredittivo,
		static_cast<uint8>(ERTPredictiveOutcome::TriggerMatched));
	TestTrue(FString::Printf(TEXT("controllo: chi entra incassa il colpo (danno %d)"), Armata.DannoAChiEntra),
		Armata.DannoAChiEntra > 0);
	TestEqual(TEXT("controllo: una attivazione"), Armata.Attivazioni, 1);
	TestTrue(FString::Printf(TEXT("controllo: e la ricarica parte (%d): la misura vede un pagamento"), Armata.Ricarica),
		Armata.Ricarica > 0);

	struct FCaso
	{
		const TCHAR* Nome;
		FRTCellId Bloccata;
		bool bMuro;
		bool bChiEntra;
		ERTActionInvalidReason Motivo;
		bool bPaga;
	};
	const FCaso Casi[] = {
		// Distanza 2 con portata 1. Gratis: il click la rifiuta da questa stessa origine (`R9`).
		{ TEXT("fuori portata"), FRTCellId(0, -1), false, true, ERTActionInvalidReason::OutOfRange, false },
		// Distanza 0 con minimo 1 ([D-610] (2)). Gratis come il fuori portata. Nessuno entra: la cella e' occupata da chi arma.
		{ TEXT("la propria cella"), DaDove, false, false, ERTActionInvalidReason::TooClose, false },
		// Accanto, dietro una copertura alta. PAGA: la linea bloccata e' `R4`, la stessa riga del Blast.
		{ TEXT("senza linea"), FRTCellId(0, 0), true, true, ERTActionInvalidReason::NoLineOfSight, true },
	};
	for (const FCaso& Caso : Casi)
	{
		const FPredRejudge R = Gioca(Caso.Bloccata, Caso.bMuro, Caso.bChiEntra);
		if (!TestTrue(FString::Printf(TEXT("banco: %s"), Caso.Nome), R.bValido)) { continue; }
		TestEqual(FString::Printf(TEXT("%s: nessuna voce predittiva — la previsione non si e' armata"), Caso.Nome),
			R.VociPredittive, 0);
		if (TestEqual(FString::Printf(TEXT("%s: il turno lo DICHIARA, una voce di rifiuto"), Caso.Nome), R.Rifiuti, 1))
		{
			TestEqual(FString::Printf(TEXT("%s: Fallback/Cancelled"), Caso.Nome), R.Rifiuto.Outcome,
				static_cast<uint8>(ERTFallbackOutcome::Cancelled));
			TestTrue(FString::Printf(TEXT("%s: nel Prep, dove si arma"), Caso.Nome), R.Rifiuto.Phase == ERTMatchPhase::Prep);
			TestEqual(FString::Printf(TEXT("%s: col suo motivo"), Caso.Nome), R.Rifiuto.Amount, static_cast<int32>(Caso.Motivo));
			TestTrue(FString::Printf(TEXT("%s: da dove sta chi arma"), Caso.Nome), R.Rifiuto.SrcCell == DaDove);
			TestTrue(FString::Printf(TEXT("%s: verso la cella dichiarata"), Caso.Nome), R.Rifiuto.TgtCell == Caso.Bloccata);
			TestTrue(FString::Printf(TEXT("%s: con la priorita' dell'azione ([D-196])"), Caso.Nome), R.Rifiuto.Priority > 0);
		}
		TestEqual(FString::Printf(TEXT("%s: nessuna attivazione — non c'e' stato un gesto"), Caso.Nome), R.Attivazioni, 0);
		if (Caso.bPaga)
		{
			TestTrue(FString::Printf(TEXT("%s: paga, come nel Blast (ricarica %d)"), Caso.Nome, R.Ricarica), R.Ricarica > 0);
		}
		else
		{
			TestEqual(FString::Printf(TEXT("%s: gratis, come nel Blast"), Caso.Nome), R.Ricarica, 0);
		}
		if (Caso.bChiEntra)
		{
			TestEqual(FString::Printf(TEXT("%s: chi entra non viene colpito"), Caso.Nome), R.DannoAChiEntra, 0);
			TestTrue(FString::Printf(TEXT("%s: e arriva dove voleva — nessuno lo ha fermato"), Caso.Nome),
				R.DoveFinisceChiEntra == FRTCellId(Caso.Bloccata.X + 1, Caso.Bloccata.Y));
		}
	}
	return true;
}

/**
 * **Al boundary il colpo parte solo se la cella bloccata e' ancora a portata e in linea da dove il tiratore e'** ([D-610]
 * (1), `#4091`). Il tiratore arma dalla cella accanto e poi SCATTA nello stesso turno con `Hero.Ivrin.PassingBlade`:
 * `ResolveDash` riscrive `Unit->Cell` prima del Move, e il boundary legge `Shooter->Cell` ([D-610] (5)(a)).
 *
 * 🔑 **Quattro scatti dalla stessa cella, e l'unica variabile e' dove finiscono**: accanto alla cella bloccata (il
 * **controllo**: il colpo parte, chi entra viene fermato — e' la combo scatto + intercetto che (1) conserva), lontano
 * (fuori portata), sopra la cella bloccata (sotto il minimo di [D-610] (2)), e accanto ma dietro una copertura alta (senza
 * linea). Prima di `#4091` i tre casi colpivano tutti: il controllo era l'unico esito.
 *
 * ⚠️ **L'armamento vale in tutte e quattro**: la cella e' accanto alla partenza, e il controllo di (5)(b) la accetta. Lo
 * dice l'attivazione, che c'e' sempre; il rifiuto, quando c'e', e' del Move e non del Prep.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPredictiveDashRejudgedTest,
	"RefactorTactics.Predictive.ShooterWhoDashesIsRejudgedAtTheBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPredictiveDashRejudgedTest::RunTest(const FString&)
{
	const FRTCellId Bloccata(0, 0);
	const FRTCellId Partenza(0, 1);
	auto Gioca = [this, &Bloccata, &Partenza](const FRTCellId& Arrivo, bool bMuro, bool bChiEntra) -> FPredRejudge
	{
		FPredRejudge Out;
		UWorld* World = MakePredWorld();
		if (!World) { return Out; }
		SpawnPredMap(World);
		ARTUnit* Runner = bChiEntra ? SpawnPredUnit(World, /*Team*/ 0, FRTCellId(-1, 0)) : nullptr;
		ARTUnit* Shooter = SpawnPredUnit(World, /*Team*/ 1, Partenza);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if ((bChiEntra && !Runner) || !Shooter || !TM || !ArmPredIntercept(Shooter, Bloccata)
			|| (bMuro && !PredWall(World, Arrivo, Bloccata)))
		{
			DestroyPredWorld(World);
			return Out;
		}
		// Lo scatto del kit, cercato per id: `Hero.Ivrin.PassingBlade`, portata 3, slot del movimento (non toglie il `Main`).
		int32 Lama = INDEX_NONE;
		for (int32 I = 0; I < Shooter->NumAbilities(); ++I)
		{
			const URTActionData* A = Shooter->GetAbility(I);
			if (A && A->Def.ActionId == TEXT("Hero.Ivrin.PassingBlade")) { Lama = I; }
		}
		if (!TestTrue(TEXT("premessa: Ivrin ha PassingBlade"), Lama != INDEX_NONE))
		{
			DestroyPredWorld(World);
			return Out;
		}
		Shooter->SetPlannedDash(Lama, Arrivo);
		Shooter->SetPlannedCell(Arrivo); // nel Move resta dove lo scatto l'ha lasciato
		const int32 Prima = Runner ? Runner->Health : 0;
		if (Runner) { Runner->SetPlannedCell(FRTCellId(1, 0)); } // attraversa la cella bloccata da ovest a est

		RunPredTurn(TM);

		ReadPredRejudge(TM, Shooter, /*SlotIndex*/ 3, Out);
		Out.DannoAChiEntra = Runner ? Prima - Runner->Health : 0;
		Out.DoveFinisceChiEntra = Runner ? Runner->Cell : FRTCellId();
		Out.bValido = true;
		DestroyPredWorld(World);
		return Out;
	};

	// ── Il controllo: lo scatto finisce ACCANTO alla cella bloccata, fuori dalla strada di chi entra. ──────
	const FRTCellId Accanto(-1, 1);
	const FPredRejudge Parte = Gioca(Accanto, /*bMuro*/ false, /*bChiEntra*/ true);
	if (!TestTrue(TEXT("banco del controllo"), Parte.bValido)) { return false; }
	TestTrue(TEXT("controllo — premessa: lo scatto c'e' stato"), Parte.DoveFinisceChiArma == Accanto);
	TestEqual(TEXT("controllo: nessun rifiuto"), Parte.Rifiuti, 0);
	TestEqual(TEXT("controllo: il colpo parte e va a segno"), Parte.EsitoPredittivo,
		static_cast<uint8>(ERTPredictiveOutcome::TriggerMatched));
	TestTrue(FString::Printf(TEXT("controllo: chi entra incassa il colpo (danno %d)"), Parte.DannoAChiEntra),
		Parte.DannoAChiEntra > 0);
	TestTrue(TEXT("controllo: e si ferma sulla cella bloccata"), Parte.DoveFinisceChiEntra == Bloccata);

	struct FCaso
	{
		const TCHAR* Nome;
		FRTCellId Arrivo;
		bool bMuro;
		bool bChiEntra;
		ERTActionInvalidReason Motivo;
	};
	const FCaso Casi[] = {
		// Tre celle dritto, sulla colonna di partenza e lontano dalla cella bloccata: dista 4, con portata 1.
		{ TEXT("scatto lontano"), FRTCellId(0, 4), false, true, ERTActionInvalidReason::OutOfRange },
		// Sopra la cella bloccata: distanza 0 con minimo 1. Nessuno entra, la cella e' occupata.
		{ TEXT("scatto sulla cella bloccata"), Bloccata, false, false, ERTActionInvalidReason::TooClose },
		// Accanto come nel controllo, ma con una copertura alta sul bordo verso la cella bloccata.
		{ TEXT("scatto dietro un muro"), Accanto, true, true, ERTActionInvalidReason::NoLineOfSight },
	};
	for (const FCaso& Caso : Casi)
	{
		const FPredRejudge R = Gioca(Caso.Arrivo, Caso.bMuro, Caso.bChiEntra);
		if (!TestTrue(FString::Printf(TEXT("banco: %s"), Caso.Nome), R.bValido)) { continue; }
		if (!TestTrue(FString::Printf(TEXT("%s — premessa: lo scatto c'e' stato"), Caso.Nome), R.DoveFinisceChiArma == Caso.Arrivo))
		{
			continue; // senza lo scatto il resto misurerebbe il tiratore fermo
		}
		TestEqual(FString::Printf(TEXT("%s: l'armamento c'e' stato — una attivazione nel Prep"), Caso.Nome), R.Attivazioni, 1);
		TestEqual(FString::Printf(TEXT("%s: nessuna voce predittiva — il colpo non parte, e non e' «a vuoto»"), Caso.Nome),
			R.VociPredittive, 0);
		if (TestEqual(FString::Printf(TEXT("%s: il turno lo DICHIARA, una voce di rifiuto"), Caso.Nome), R.Rifiuti, 1))
		{
			TestEqual(FString::Printf(TEXT("%s: Fallback/Cancelled"), Caso.Nome), R.Rifiuto.Outcome,
				static_cast<uint8>(ERTFallbackOutcome::Cancelled));
			TestTrue(FString::Printf(TEXT("%s: nel Move, al boundary"), Caso.Nome), R.Rifiuto.Phase == ERTMatchPhase::Move);
			TestEqual(FString::Printf(TEXT("%s: col suo motivo"), Caso.Nome), R.Rifiuto.Amount, static_cast<int32>(Caso.Motivo));
			TestTrue(FString::Printf(TEXT("%s: da dove lo scatto ha lasciato il tiratore"), Caso.Nome), R.Rifiuto.SrcCell == Caso.Arrivo);
			TestTrue(FString::Printf(TEXT("%s: verso la cella bloccata"), Caso.Nome), R.Rifiuto.TgtCell == Bloccata);
			TestTrue(FString::Printf(TEXT("%s: con la priorita' dell'azione ([D-196])"), Caso.Nome), R.Rifiuto.Priority > 0);
		}
		TestTrue(FString::Printf(TEXT("%s: la ricarica era gia' spesa all'armamento (%d)"), Caso.Nome, R.Ricarica), R.Ricarica > 0);
		if (Caso.bChiEntra)
		{
			TestEqual(FString::Printf(TEXT("%s: chi entra non viene colpito"), Caso.Nome), R.DannoAChiEntra, 0);
			TestTrue(FString::Printf(TEXT("%s: un colpo che non parte non tronca nessuno"), Caso.Nome),
				R.DoveFinisceChiEntra == FRTCellId(1, 0));
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
