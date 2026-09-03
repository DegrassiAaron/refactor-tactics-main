// Dentro `ResolveMovement` la posizione autorevole di un'unita' e' `State.Pos[i]`, e `ARTUnit::Cell` e' la
// cella di PARTENZA DEL TURNO fino a `PlaceOnCell` (`#2142`).
//
// Questi test misurano la conseguenza osservabile piu' cara: la COPERTURA del colpo di Overwatch. Il ramo
// `FIRE` passava `Target->Cell` a `BoundaryCoverReduction`, quindi calcolava la riduzione dalla cella in cui
// il bersaglio si trovava a inizio turno invece che da quella in cui il colpo lo ha raggiunto.
//
// 🔴 **Non e' un cambio di regola, ed e' la ragione per cui questi test sono una CORREZIONE e non una
// decisione di bilanciamento**: ADR-0004 §*«Quale cella»* prescrive *«Overwatch FIRE | la cella corrente |
// al suo micro-step e' gia' quella giusta»*. Il commento della funzione citava quella riga mentre il codice
// faceva l'opposto.
//
// ⚠️ **Perche' passano dal percorso REALE** (`LockInAndResolve` e il suo ciclo di micro-step) e non da
// `BoundaryCoverReduction` chiamata a mano: la decisione qui non e' *cosa* la funzione calcola, e' **quale
// cella il chiamante le passa**. Una prova sulla funzione pura sarebbe restata verde con il difetto vivo —
// e' la lezione di [D-312], misurata su questo stesso file di produzione.
//
// [D-169] dichiarava scoperta questa meta': *«che sia davvero `ResolveReactionBoundary` a passare
// `State.Pos[OwnerIdx]` non e' coperta»*. Da qui lo e'.
//
// Prefissi `RxCell*` negli helper: la unity build fonde i namespace anonimi fra file.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"       // LowCoverDamageReduction: la costante di bilanciamento, non un 10 scritto qui
#include "Combat/RTHexCombatLibrary.h"    // EffectiveCoverReduction: la stessa funzione che il resolver chiama
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTKnowledgeView.h"      // FRTKnowledgeSubject: i tre ingressi che `FreezeVerdict` legge
#include "Perception/RTPerceptionLibrary.h"  // FRTPerceiver: l'osservatore, con cella facing e portata
#include "Perception/RTTeamKnowledge.h"      // Observe/FreezeVerdict: le funzioni di PRODUZIONE, non una copia
#include "Tests/RTAbilityFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWorld* MakeRxCellWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyRxCellWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnRxCellMap(UWorld* World, int32 Radius = 6)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return Actor;
	}

	ARTUnit* SpawnRxCellUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell, ERTHexDirection Facing)
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
		U->Facing = Facing;
		// [D-224] Scudo a zero: qui si misura una RIDUZIONE da copertura, e lo scudo base la sommerebbe a una
		// seconda costante rendendo l'asserto illeggibile. Stessa scelta dei test predittivi, per lo stesso motivo.
		U->Shield = 0;
		return U;
	}

	/**
	 * Copertura bassa su OGNI bordo della cella: quale bordo il tiro attraversi dipende dalla geometria
	 * assiale, e fissarne uno solo legherebbe il test a un dettaglio che non sta verificando. E' la stessa
	 * cautela di `RefactorTactics.Predictive.BoundaryShotRespectsCover`.
	 */
	void CoverEveryEdge(URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		if (!Map) { return; }
		FRTHexCellData Data(Cell);
		for (const ERTHexDirection Edge : { ERTHexDirection::E, ERTHexDirection::NE, ERTHexDirection::NW,
			ERTHexDirection::W, ERTHexDirection::SW, ERTHexDirection::SE })
		{
			FRTHexCover Cover;
			Cover.Edge = Edge;
			Cover.Type = ERTHexCoverType::Low;
			Data.Covers.Add(Cover);
		}
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** La riduzione che il resolver calcolerebbe con il bersaglio in `TargetCell`: la funzione vera, non una copia. */
	int32 RxCellReduction(const URTHexMapAsset* Map, const ARTUnit* Watcher, const ARTUnit* Mover,
		const FRTCellId& TargetCell)
	{
		FRTHexCombatUnit A;
		A.UnitId = 0;
		A.TeamId = Watcher->TeamId;
		A.Cell = Watcher->Cell;
		A.Facing = Watcher->Facing;

		FRTHexCombatUnit T;
		T.UnitId = 1;
		T.TeamId = Mover->TeamId;
		T.Cell = TargetCell;
		T.Facing = Mover->Facing;

		return URTHexCombatLibrary::EffectiveCoverReduction(Map, A, T, ERTAbilityShape::Single);
	}

	/** Arma `Action.Overwatch` come AZIONE PRINCIPALE, che e' cio' che costa (catalogo §1). */
	bool ArmRxCellOverwatch(ARTUnit* Watcher, int32 SlotIndex = 3)
	{
		const int32 Index = RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), SlotIndex);
		if (Index == INDEX_NONE) { return false; }
		Watcher->PlannedAbilityIndex = Index;
		return true;
	}

	/**
	 * Un decisore che risponde `FIRE` a qualunque finestra glielo consenta.
	 *
	 * Sceglie la prima risposta legale diversa da `HOLD` invece di costruire `FireResponse(idx)`: l'indice
	 * del bersaglio e' quello di `CollectLivingUnits`, che ordina per cella e cambierebbe con la geometria.
	 * Leggere l'opzione dalla finestra e' anche cio' che fa un giocatore.
	 */
	void BindRxCellFireDecider(ARTTurnManager* TM)
	{
		TM->ReactionDecider.BindLambda(
			[](const FRTReactionOpportunity& Opportunity, int32 /*OwnerUnitId*/) -> FString
			{
				for (const FString& Response : Opportunity.AllowedResponses)
				{
					if (URTReactionOpportunityLibrary::FireResponseTarget(Response) != INDEX_NONE)
					{
						return Response;
					}
				}
				return FString();
			});
	}

	void RunRxCellTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** La voce `ReactionDecision` che dichiara un `FIRE`, o `nullptr`. */
	const FRTTurnLogEntry* FindRxCellFireEntry(const ARTTurnManager* TM)
	{
		for (const FRTTurnLogEntry& E : TM->GetTurnLog())
		{
			if (E.Category != ERTLogCategory::ReactionDecision) { continue; }
			if (URTReactionOpportunityLibrary::FireResponseTarget(E.ReactionResponse) == INDEX_NONE) { continue; }
			return &E;
		}
		return nullptr;
	}

	/** Il danno dichiarato dell'arma del Wraith, letto dal CATALOGO: `Armed.Damage` nasce di li'. */
	int32 RxCellWeaponDamage()
	{
		int32 Damage = 0;
		const URTHeroData* Wraith = URTHeroCatalogLibrary::MakeWraith();
		if (!Wraith) { return 0; }
		for (const URTActionData* Action : Wraith->Actions)
		{
			if (!Action || Action->Def.BaseActionId != FName(TEXT("Action.BasicAttack"))) { continue; }
			for (const FRTActionEffectSpec& Effect : Action->Def.Effects)
			{
				if (Effect.Effect == ERTActionEffect::Damage) { Damage += Effect.Amount; }
			}
			break;
		}
		return Damage;
	}

	/**
	 * La geometria condivisa dai due test, montata in un colpo solo.
	 *
	 * Il watcher guarda a `W` da `(3,0,0)`: `MakeSuppressiveZone` traccia una LINEA lungo quel facing, quindi
	 * controlla `(2,0,0)`, `(1,0,0)`, … Il mover parte FUORI dalla linea e ci entra con un passo — l'ingresso
	 * e' l'evento che apre la finestra, non la presenza.
	 */
	struct FRxCellStage
	{
		UWorld* World = nullptr;
		ARTHexMapActor* MapActor = nullptr;
		ARTUnit* Watcher = nullptr;
		ARTUnit* Mover = nullptr;
		ARTTurnManager* TM = nullptr;
		FRTCellId Start;
		FRTCellId Dest;
	};

	bool BuildRxCellStage(FRxCellStage& Out)
	{
		Out.World = MakeRxCellWorld();
		if (!Out.World) { return false; }
		Out.MapActor = SpawnRxCellMap(Out.World);
		if (!Out.MapActor || !Out.MapActor->MapAsset) { return false; }

		const FRTCellId WatchCell(3, 0, 0);
		Out.Dest = FRTCellId(1, 0, 0);                                   // dentro la linea guardata da W
		Out.Start = URTHexLibrary::Neighbor(Out.Dest, ERTHexDirection::SW); // fuori dalla linea, adiacente

		// Il mover guarda verso il watcher: e' la condizione perche' la copertura CONTI. Fuori dall'arco
		// frontale `EffectiveCoverReduction` annulla la riduzione (CP 16.2), e i due mondi che questo test
		// confronta collasserebbero entrambi su zero — verde, e senza aver misurato niente.
		Out.Watcher = SpawnRxCellUnit(Out.World, /*Team*/ 1, WatchCell, ERTHexDirection::W);
		Out.Mover = SpawnRxCellUnit(Out.World, /*Team*/ 0, Out.Start, ERTHexDirection::E);
		Out.TM = Out.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Out.Watcher || !Out.Mover || !Out.TM) { return false; }

		Out.Mover->PlannedCell = Out.Dest;
		return ArmRxCellOverwatch(Out.Watcher);
	}
}

/**
 * 🔴 **IL DIFETTO (1)**: chi parte dietro un riparo ed esce allo scoperto incassa il colpo di Overwatch
 * **pieno**.
 *
 * *Mutazione che lo rende rosso*: rimettere `Target->Cell` al posto di `State.Pos[TargetIdx]` nella chiamata
 * a `BoundaryCoverReduction` di `ApplyReactionDecision`. Con quella riga il colpo arriva ridotto di
 * `LowCoverDamageReduction` da una copertura che il bersaglio ha gia' lasciato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchCoverReadsMicroStepCellTest,
	"RefactorTactics.Reactions.OverwatchCoverReadsTheMicroStepCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchCoverReadsMicroStepCellTest::RunTest(const FString&)
{
	FRxCellStage S;
	if (!TestTrue(TEXT("la scena si monta"), BuildRxCellStage(S)))
	{
		DestroyRxCellWorld(S.World);
		return false;
	}

	// Il riparo sta sulla cella che il mover LASCIA. Quella d'arrivo resta scoperta.
	CoverEveryEdge(S.MapActor->MapAsset, S.Start);

	// 🔑 **L'ANTI-VACUITA', e va misurata prima di girare il turno.** Le due celle devono dare riduzioni
	// DIVERSE: se il riparo coprisse anche l'arrivo, il test sarebbe verde con la correzione e senza, e non
	// direbbe niente. Sono anche i due mondi che la mutazione mette a confronto.
	const int32 ReduzioneDaPartenza = RxCellReduction(S.MapActor->MapAsset, S.Watcher, S.Mover, S.Start);
	const int32 ReduzioneDaArrivo = RxCellReduction(S.MapActor->MapAsset, S.Watcher, S.Mover, S.Dest);
	TestTrue(FString::Printf(TEXT("setup: la cella di PARTENZA e' coperta (riduzione %d)"), ReduzioneDaPartenza),
		ReduzioneDaPartenza > 0);
	TestEqual(TEXT("setup: la cella d'ARRIVO e' scoperta"), ReduzioneDaArrivo, 0);

	const int32 Prima = S.Mover->Health;
	BindRxCellFireDecider(S.TM);
	RunRxCellTurn(S.TM);

	// PREMESSA: un `FIRE` c'e' stato davvero. Senza questa riga un turno in cui la finestra non si apre —
	// zero danno — soddisferebbe l'asserto sotto solo se scritto male, e comunque non proverebbe niente.
	const FRTTurnLogEntry* Fire = FindRxCellFireEntry(S.TM);
	if (!TestNotNull(TEXT("il watcher ha sparato: una voce ReactionDecision con risposta FIRE"), Fire))
	{
		DestroyRxCellWorld(S.World);
		return false;
	}

	// E il movimento e' stato troncato dove il colpo l'ha raggiunto: e' la cella su cui la copertura
	// dev'essere misurata, ed e' osservabile.
	TestTrue(TEXT("il mover si e' fermato nella cella raggiunta"), S.Mover->Cell == S.Dest);

	// 🔴 IL PUNTO: danno PIENO. Il riparo era dietro di lui.
	const int32 Inflitto = Prima - S.Mover->Health;
	const int32 Dichiarato = RxCellWeaponDamage();
	TestTrue(FString::Printf(TEXT("premessa: l'arma dichiara un danno (%d)"), Dichiarato), Dichiarato > 0);
	TestEqual(FString::Printf(
			TEXT("chi esce dal riparo incassa PIENO (inflitto %d, dichiarato %d, riduzione lasciata %d)"),
			Inflitto, Dichiarato, ReduzioneDaPartenza),
		Inflitto, Dichiarato);

	// La stessa affermazione nella traccia autorevole, non solo negli HP: `Amount` e' il danno EFFETTIVO.
	TestEqual(TEXT("e la voce del TurnLog dichiara lo stesso danno"), Fire->Amount, Dichiarato);

	DestroyRxCellWorld(S.World);
	return true;
}

/**
 * Il verso opposto, e con esso **il difetto (3)**: chi ENTRA in copertura ne beneficia, e la riga che il
 * giocatore legge annuncia il danno **inflitto** invece di quello dichiarato.
 *
 * *Mutazioni che lo rendono rosso*: (a) rimettere `Target->Cell` nella chiamata a `BoundaryCoverReduction`
 * — il colpo arriva pieno perche' la cella di partenza e' scoperta; (b) rimettere `Armed.Damage` nella
 * `AddLogEvent` finale di `ApplyReactionDecision` — la riga annuncia un danno mai inflitto, che e' lo stesso
 * difetto che `#888` aveva corretto nella traccia e non nel canale derivato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOverwatchLogLineAnnouncesDealtDamageTest,
	"RefactorTactics.Reactions.OverwatchLogLineAnnouncesDealtDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOverwatchLogLineAnnouncesDealtDamageTest::RunTest(const FString&)
{
	FRxCellStage S;
	if (!TestTrue(TEXT("la scena si monta"), BuildRxCellStage(S)))
	{
		DestroyRxCellWorld(S.World);
		return false;
	}

	// Il riparo sta sulla cella in cui il mover ARRIVA: e' li' che il colpo lo raggiunge.
	CoverEveryEdge(S.MapActor->MapAsset, S.Dest);

	const int32 ReduzioneDaPartenza = RxCellReduction(S.MapActor->MapAsset, S.Watcher, S.Mover, S.Start);
	const int32 ReduzioneDaArrivo = RxCellReduction(S.MapActor->MapAsset, S.Watcher, S.Mover, S.Dest);
	TestEqual(TEXT("setup: la cella di PARTENZA e' scoperta"), ReduzioneDaPartenza, 0);
	TestTrue(FString::Printf(TEXT("setup: la cella d'ARRIVO e' coperta (riduzione %d)"), ReduzioneDaArrivo),
		ReduzioneDaArrivo > 0);

	const int32 Prima = S.Mover->Health;
	BindRxCellFireDecider(S.TM);
	RunRxCellTurn(S.TM);

	const FRTTurnLogEntry* Fire = FindRxCellFireEntry(S.TM);
	if (!TestNotNull(TEXT("il watcher ha sparato: una voce ReactionDecision con risposta FIRE"), Fire))
	{
		DestroyRxCellWorld(S.World);
		return false;
	}

	const int32 Dichiarato = RxCellWeaponDamage();
	const int32 Atteso = FMath::Max(0, Dichiarato - ReduzioneDaArrivo);
	// La premessa del difetto (3): i due numeri devono DIVERGERE, o «annuncia quello inflitto» e «annuncia
	// quello dichiarato» sarebbero la stessa riga e nessuna mutazione la farebbe cadere.
	TestTrue(FString::Printf(TEXT("premessa: dichiarato %d e inflitto %d divergono"), Dichiarato, Atteso),
		Atteso != Dichiarato);

	const int32 Inflitto = Prima - S.Mover->Health;
	TestEqual(TEXT("chi entra in copertura incassa RIDOTTO"), Inflitto, Atteso);

	// 🔴 IL PUNTO: la riga che il giocatore legge porta il danno EFFETTIVO. Il combat log e' il canale
	// derivato, cioe' l'unico dei due che qualcuno guarda davvero durante una partita.
	const TArray<FString> Righe = S.TM->GetRecentEvents();
	bool bTrovataConDealt = false;
	bool bTrovataConDichiarato = false;
	for (const FString& Riga : Righe)
	{
		if (!Riga.Contains(TEXT("overwatch su"))) { continue; }
		if (Riga.Contains(FString::Printf(TEXT("%d danni"), Atteso))) { bTrovataConDealt = true; }
		if (Riga.Contains(FString::Printf(TEXT("%d danni"), Dichiarato))) { bTrovataConDichiarato = true; }
	}
	TestTrue(FString::Printf(TEXT("la riga di overwatch annuncia il danno INFLITTO (%d)"), Atteso),
		bTrovataConDealt);
	TestFalse(FString::Printf(TEXT("e non quello DICHIARATO (%d), mai inflitto"), Dichiarato),
		bTrovataConDichiarato);

	DestroyRxCellWorld(S.World);
	return true;
}

/**
 * 🔴 **IL DIFETTO (2)**: la voce scritta DENTRO il ciclo dei micro-step congela il proprio verdetto di
 * visibilita' — e il soggetto d'audit di [D-313] — sulla cella dove il bersaglio e' **arrivato**, non su
 * quella da cui era partito.
 *
 * La conseguenza che l'issue nomina: *«chi esce dalla nebbia sotto il fuoco di un Overwatch si vede la voce
 * NASCOSTA a una squadra che lo vede benissimo»*. Qui la nebbia e' la portata visiva del watcher, ristretta
 * apposta: il mover parte fuori dal raggio e ci entra con un passo.
 *
 * ⚠️ **Lo scarto fra i due istanti e' REALE e non un artificio del test**: la conoscenza di squadra ha due
 * sole assegnazioni per turno ([D-223]), mentre il trigger dell'Overwatch ricostruisce la propria a ogni
 * micro-step. E' precisamente per questo che il mover puo' essere bersagliabile e — con la cella stantia —
 * invisibile nella stessa voce.
 *
 * *Mutazione che lo rende rosso*: rimettere `S.Cell = U->Cell` in `FreezeVerdictFor` (e/o
 * `Entry.VerdictSubject.Cell = Actor->Cell` in `AppendLogEntry`).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionVerdictFreezesOnImpactCellTest,
	"RefactorTactics.Reactions.VerdictFreezesOnTheImpactCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionVerdictFreezesOnImpactCellTest::RunTest(const FString&)
{
	FRxCellStage S;
	if (!TestTrue(TEXT("la scena si monta"), BuildRxCellStage(S)))
	{
		DestroyRxCellWorld(S.World);
		return false;
	}

	// La nebbia: il watcher vede fino a due celle. `Start` ne dista tre, `Dest` due — un passo, e il mover
	// passa da invisibile a visibile per la squadra che gli sta sparando.
	S.Watcher->VisionRange = 2;

	// 🔑 **L'ANTI-VACUITA', misurata con le funzioni di PRODUZIONE.** Se le due celle dessero lo stesso
	// verdetto, l'asserto sotto sarebbe vero con la correzione e senza — un test verde che non difende
	// niente. Si ricostruisce la conoscenza della squadra del watcher esattamente come fa
	// `RefreshTeamKnowledgeForPlanning`: stessi osservatori, stessa `Observe`, stessa `FreezeVerdict`.
	FRTPerceiver Osservatore;
	Osservatore.Cell = S.Watcher->Cell;
	Osservatore.Facing = S.Watcher->Facing;
	Osservatore.VisionRange = S.Watcher->VisionRange;

	const FRTTeamKnowledge Conoscenza = URTTeamKnowledgeLibrary::Observe(
		S.MapActor->MapAsset, S.Watcher->TeamId, /*TurnNumber*/ 1, { Osservatore },
		{ FRTLastKnownContact(S.Mover->StableUnitId, S.Start, 0) }, FRTTeamKnowledge());

	FRTKnowledgeSubject SoggettoAllaPartenza;
	SoggettoAllaPartenza.StableUnitId = S.Mover->StableUnitId;
	SoggettoAllaPartenza.TeamId = S.Mover->TeamId;
	SoggettoAllaPartenza.Cell = S.Start;
	SoggettoAllaPartenza.bAlive = true;

	FRTKnowledgeSubject SoggettoAllImpatto = SoggettoAllaPartenza;
	SoggettoAllImpatto.Cell = S.Dest;

	const bool bVistoAllaPartenza = URTTeamKnowledgeLibrary::FreezeVerdict({ Conoscenza }, SoggettoAllaPartenza)
		.AllowsTeam(S.Watcher->TeamId);
	const bool bVistoAllImpatto = URTTeamKnowledgeLibrary::FreezeVerdict({ Conoscenza }, SoggettoAllImpatto)
		.AllowsTeam(S.Watcher->TeamId);

	TestFalse(FString::Printf(TEXT("setup: alla PARTENZA %s e' fuori dalla vista della squadra %d"),
		*S.Start.ToString(), S.Watcher->TeamId), bVistoAllaPartenza);
	TestTrue(FString::Printf(TEXT("setup: all'IMPATTO %s e' dentro"), *S.Dest.ToString()), bVistoAllImpatto);

	BindRxCellFireDecider(S.TM);
	RunRxCellTurn(S.TM);

	// La voce di CHI SUBISCE: `HitCameFromSide` si congela sul bersaglio (`IsSubjectTheSufferer`), che e'
	// l'unita' in movimento — quindi e' la voce in cui il difetto si vede.
	const FRTTurnLogEntry* Subita = nullptr;
	for (const FRTTurnLogEntry& E : S.TM->GetTurnLog())
	{
		if (E.Category == ERTLogCategory::Facing
			&& E.Outcome == static_cast<uint8>(ERTFacingOutcome::HitCameFromSide))
		{
			Subita = &E;
			break;
		}
	}
	if (!TestNotNull(TEXT("il colpo di Overwatch ha lasciato la sua voce direzionale"), Subita))
	{
		DestroyRxCellWorld(S.World);
		return false;
	}

	// 🔴 IL PUNTO (a): il verdetto e' quello della cella dell'IMPATTO. La squadra che gli sta sparando puo'
	// leggere la riga di cio' che ha appena fatto.
	TestTrue(TEXT("la voce e' leggibile dalla squadra che al momento del colpo lo vede"),
		Subita->Verdict.AllowsTeam(S.Watcher->TeamId));

	// 🔴 IL PUNTO (b): e il SOGGETTO registrato dice contro quale cella e' stato congelato ([D-313]). Le due
	// scritture leggono lo stesso soggetto: prima erano due letture di `Actor->Cell`, coerenti fra loro e
	// entrambe sbagliate — cioe' un audit che le confrontava non poteva accorgersene.
	TestTrue(FString::Printf(TEXT("il soggetto d'audit porta la cella dell'impatto (%s, non %s)"),
			*Subita->VerdictSubject.Cell.ToString(), *S.Start.ToString()),
		Subita->VerdictSubject.Cell == S.Dest);

	// E la squadra del mover la legge sempre: e' la sua unita'. Senza questa riga un verdetto degenere che
	// concede a tutti soddisferebbe la (a) senza aver deciso niente.
	TestTrue(TEXT("e dalla squadra del mover, che vede sempre la propria unita'"),
		Subita->Verdict.AllowsTeam(S.Mover->TeamId));

	DestroyRxCellWorld(S.World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
