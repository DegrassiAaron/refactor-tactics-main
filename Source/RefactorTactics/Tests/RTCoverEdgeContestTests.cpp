#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h" // StunnedDurationTurns
#include "Core/RTGameplayTags.h" // TAG_Status_Stunned
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Tests/RTAbilityFixtures.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason::Stunned
#include "Turn/RTActionQueueLibrary.h" // SortUnitsForResolution: l'ordine di raccolta (#2922)
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTResolvedEvent.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

// =====================================================================================================
// #3963 - chi vince un BORDO CONTESTATO.
//
// Due unita' che nello stesso Prep erigono (`Pending`) o spostano (`Moves`) sulla STESSA cella e sullo STESSO bordo
// pareggiano nei due criteri dei comparatori di `ARTTurnManager::ResolveCoverStructures` (la cella, poi il bordo). La
// prima operazione applicata vince: le altre le rifiuta `AddCover` (`CoverRejected`) o la verifica "e' gia' su quel bordo"
// dello spostamento. `TArray::Sort` e' un introsort, non e' stabile, quindi senza una chiave scritta il vincitore lo
// decide l'algoritmo, in funzione di quante operazioni ci sono e di dove stanno.
//
// La REGOLA che questo banco fissa: a parita' di cella e di bordo vince l'operazione raccolta per prima, cioe' quella
// dell'unita' che `SortUnitsForResolution` mette prima (cella, poi `StableUnitId`, poi nome). Le altre sono rifiutate e
// pagano come prima.
//
// Come e' costruito
//   - FLUSSO REALE: unita' vere e `ARTTurnManager::LockInAndResolve`. Il comparatore e la sua lista sono locali alla
//     funzione di produzione, e provarli su una copia sarebbe provare un'altra cosa.
//   - ORACOLO INDIPENDENTE: il vincitore atteso e' l'unita' con la cella di partenza minore (`StableLess`) fra chi
//     contende in quel braccio; una premessa confronta quell'ordine con quello della funzione vera, e il test non legge
//     mai il campo `Order` della produzione. Il vincitore e' letto da canali indipendenti: la voce `CoverCreated`/`CoverMoved`
//     del TurnLog e l'evento `AbilityActivated` della timeline. Per l'erezione anche la mappa: il vincitore atteso
//     porta il pannello ADATTIVO e tutti gli altri quello base, con integrita' diverse, quindi cio' che resta sul bordo
//     dice chi ha vinto anche a TurnLog muto.
//   - LA STORDITA: la stessa scena in mondi che differiscono per una riga. Stordita (col piano e con
//     `Status.Stunned`: [D-565] la rifiuta in raccolta), senza piano (il controllo "senza di lei", nello stesso mondo e
//     con gli stessi `StableUnitId`) e non stordita (contende come ogni altra: il controllo POSITIVO, in cui la stordita,
//     se e' la prima, vince). Il vincitore fra gli altri non deve cambiare fra il mondo stordita e quello senza piano, e
//     la stordita non ha nessuna voce ambientale: non e' fra i rifiutati, perche' non e' entrata.
//   - LISTE LUNGHE: spettatori che erigono (o spostano) altrove nello stesso Prep, in mezzo ai contendenti, e una scena
//     con tante operazioni da portare l'introsort sul ramo a partizione (`Algo/IntroSort.h`), che nei formati noti non
//     si raggiunge ma che l'ordine totale deve coprire lo stesso.
//   - SQUADRE DIVERSE: il comparatore non legge la squadra, ma gli `StableUnitId` si assegnano per squadra e POI per cella
//     (`MatchRosterLess`), mentre la raccolta ordina per cella: con un contendente di squadra 1 in una cella minore, il
//     primo raccolto NON ha lo `StableUnitId` minore, e la riga distingue "il primo raccolto" da "l'id minore".
//   - CONTROLLO DI CELLA: le due facce dello stesso bordo fisico, mirate da due unita' (una su (1,0) W, l'altra su
//     (0,0) E), non pareggiano: le ordina la cella, e vince la minore anche se l'altra e' raccolta per prima. Il terzo
//     criterio e' il TERZO: se prendesse il posto della cella questa riga cadrebbe.
//
// Premesse asserite in ogni run (se una cade il banco e' rosso per un guasto di fixture, e gli esiti non si leggono):
// l'azione e' nello slot 3, e' una PRINCIPALE di Prep con la `StructureOp` attesa, e' utilizzabile; la cella mirata e'
// a portata; lo stordimento c'e' dove il braccio lo dichiara; le celle di partenza sono distinte; l'ordine reale di
// raccolta coincide con quello per cella; gli `StableUnitId` sono non nulli e distinti.
//
// Cio' che il banco NON misura (limiti dichiarati)
//   - due unita' sulla STESSA cella di partenza: li' decide lo `StableUnitId` di `SortUnitsForResolution`, che ha il suo
//     banco (`Actions.UnitOrderStableIdBeatsActorName`); qui le celle di partenza sono sempre distinte;
//   - la posizione del terzo criterio rispetto al bordo: operazioni sulla stessa cella e su bordi diversi non si
//     contendono nulla, quindi l'ordine fra i due criteri non cambia nessun esito osservabile in un turno;
//   - il pagamento dello spostamento e la rotazione gratuita del pannello adattivo: la lista che li tiene e' `protected`
//     e senza accessor;
//   - determinismo, replay e digest: li misurano il corpus golden e `Replay.*`, non questo file; PIE e packaged.
//
// Prefisso `CoverTie*` negli helper: unity build, namespace anonimi fusi con gli altri file di test.
// =====================================================================================================
namespace
{
	/** Che cosa si contende: erigere (la lista `Pending` di `ResolveCoverStructures`) o spostare (la lista `Moves`). */
	enum class ECoverTieKind : uint8
	{
		Create, //< `Action.CreateCover` e `Hero.Branth.KineticPanel`: erigono sulla cella mirata, bordo W
		Move,   //< `Hero.Branth.Reconfigure`: sposta la copertura di mappa gia' sulla cella mirata, dal bordo W al bordo E
	};

	/** La parte che un'unita' recita nella scena. */
	enum class ECoverTieRole : uint8
	{
		Contender, //< pianifica il bordo contestato
		Stunned,   //< la stordita fra i contendenti: il braccio decide se ha il piano e se e' stordita
		Bystander, //< fuori dalla contesa: pianifica una struttura sulla PROPRIA cella, bordo E, nello stesso Prep
	};

	/** I mondi di una scena con la stordita: differiscono per UNA riga, e le premesse lo verificano. */
	enum class ECoverTieArm : uint8
	{
		Stunned, //< col piano e con `Status.Stunned` al commit
		Idle,    //< la stessa unita' nello stesso mondo, senza piano: il controllo "senza di lei"
		Free,    //< col piano e senza `Status.Stunned`: contende come ogni altra
	};

	struct FCoverTieUnit
	{
		FRTCellId Start;
		int32 TeamId = 0;
		ECoverTieRole Role = ECoverTieRole::Contender;
	};

	struct FCoverTieScene
	{
		FString Name;
		TArray<FCoverTieUnit> Units;     //< nell'ordine di SPAWN, che non e' l'ordine di raccolta: lo decide la cella di partenza
		bool bWinnerNotLowestId = false; //< premessa di riga: il vincitore atteso NON ha lo StableUnitId minore fra i contendenti
	};

	/** Lo slot dell'azione sotto prova, lo stesso del banco di [D-565]. */
	constexpr int32 CoverTieSlot = 3;

	/** L'integrita' della copertura di mappa posta prima del turno nelle scene di spostamento: non e' un default del catalogo. */
	constexpr int32 CoverTiePreplacedIntegrity = 18;

	/** La cella che riceve la struttura contestata. */
	FRTCellId CoverTieTargetCell() { return FRTCellId(1, 0); }

	/** L'avversario inerte e lontano: senza nemici la partita finirebbe al primo turno (`EvaluateOutcome`). */
	FRTCellId CoverTieFoeCell() { return FRTCellId(-4, 0); }

	const TCHAR* CoverTieArmName(ECoverTieArm Arm)
	{
		switch (Arm)
		{
		case ECoverTieArm::Stunned: return TEXT("stordita");
		case ECoverTieArm::Idle:    return TEXT("senza piano");
		case ECoverTieArm::Free:    return TEXT("non stordita");
		}
		return TEXT("?");
	}

	/** Ha un piano in questo braccio? */
	bool CoverTiePlans(const FCoverTieUnit& Unit, ECoverTieArm Arm)
	{
		return Unit.Role != ECoverTieRole::Stunned || Arm != ECoverTieArm::Idle;
	}

	/** Contende per il bordo contestato in questo braccio? La stordita no: [D-565] la rifiuta prima che entri. */
	bool CoverTieContends(const FCoverTieUnit& Unit, ECoverTieArm Arm)
	{
		return Unit.Role == ECoverTieRole::Contender || (Unit.Role == ECoverTieRole::Stunned && Arm == ECoverTieArm::Free);
	}

	bool CoverTieIsStunnedNow(const FCoverTieUnit& Unit, ECoverTieArm Arm)
	{
		return Unit.Role == ECoverTieRole::Stunned && Arm == ECoverTieArm::Stunned;
	}

	/** Un mondo di prova che si distrugge da solo, anche sulle uscite anticipate. */
	struct FCoverTieWorld
	{
		UWorld* World = nullptr;
		~FCoverTieWorld() { RTWorldFixtures::DestroyWorld(World); }
	};

	/** Tutto cio' che serve a una run: il mondo, la mappa, le unita' di scena, l'avversario inerte e il gestore del turno. */
	struct FCoverTieBed
	{
		FCoverTieWorld Holder;
		URTHexMapAsset* Map = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTUnit* Foe = nullptr;
		TArray<ARTUnit*> Units;
		TArray<TWeakObjectPtr<ARTUnit>> Weak;
	};

	URTHexMapAsset* CoverTieSpawnMap(UWorld* World)
	{
		URTHexMapAsset* Map = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		if (Actor == nullptr)
		{
			return nullptr;
		}
		Actor->MapAsset = Map;
		return Map;
	}

	ARTUnit* CoverTieSpawnUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit) { return nullptr; }
		Unit->TeamId = TeamId;
		Unit->bIsBotControlled = false;
		Unit->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		Unit->PlannedCell = Cell;
		return Unit;
	}

	/** Apre il mondo di una run. Falso, e rosso, se qualcosa manca. */
	bool CoverTieOpenBed(FAutomationTestBase& Test, const FString& Run, const TArray<FCoverTieUnit>& Specs, FCoverTieBed& Bed)
	{
		Bed.Holder.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), *Run), Bed.Holder.World)) { return false; }
		UWorld* World = Bed.Holder.World;
		Bed.Map = CoverTieSpawnMap(World);
		for (const FCoverTieUnit& Spec : Specs)
		{
			ARTUnit* Unit = CoverTieSpawnUnit(World, Spec.TeamId, Spec.Start);
			Bed.Units.Add(Unit);
			Bed.Weak.Add(Unit);
		}
		Bed.Foe = CoverTieSpawnUnit(World, 1, CoverTieFoeCell());
		Bed.TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());

		bool bOk = true;
		bOk &= Test.TestNotNull(*FString::Printf(TEXT("%s: mappa"), *Run), Bed.Map);
		bOk &= Test.TestNotNull(*FString::Printf(TEXT("%s: avversario inerte"), *Run), Bed.Foe);
		bOk &= Test.TestNotNull(*FString::Printf(TEXT("%s: TM"), *Run), Bed.TM);
		for (int32 I = 0; I < Bed.Units.Num(); ++I)
		{
			bOk &= Test.TestNotNull(*FString::Printf(TEXT("%s: unita' di spawn %d"), *Run, I), Bed.Units[I]);
		}
		return bOk;
	}

	/** Un'azione del kit di Branth, cercata per ActionId e non per indice: l'indice scivola se un'azione core manca (vedi `MakeBranth`). */
	URTActionData* CoverTieBranthAction(const TCHAR* ActionId)
	{
		URTHeroData* Branth = URTHeroCatalogLibrary::MakeBranth();
		if (!Branth) { return nullptr; }
		const TObjectPtr<URTActionData>* Found = Branth->Actions.FindByPredicate(
			[ActionId](const TObjectPtr<URTActionData>& Action) { return Action && Action->Def.ActionId == FName(ActionId); });
		return Found ? Found->Get() : nullptr;
	}

	/** Il piano di una struttura di bordo: bersaglio-CELLA piu' il bordo, che a portata 3 non e' derivabile dalla coppia di celle. */
	void CoverTieWritePlan(ARTUnit* Unit, const FRTCellId& AimCell, ERTHexDirection Edge)
	{
		Unit->PlannedAbilityIndex = CoverTieSlot;
		Unit->PlannedAttackTarget = nullptr;
		Unit->PlannedAttackCell = AimCell;
		Unit->bAttackTargetsCell = true;
		Unit->PlannedCoverEdge = Edge;
		Unit->bHasPlannedCoverEdge = true;
	}

	/** `Action.CreateCover` del catalogo core: erige, con l'integrita' di default per due turni. */
	bool CoverTiePlanBase(ARTUnit* Unit, const FRTCellId& AimCell, ERTHexDirection Edge)
	{
		if (RTAbilityFixtures::AddCoreAbilityInSlot(Unit, TEXT("Action.CreateCover"), CoverTieSlot) != CoverTieSlot) { return false; }
		CoverTieWritePlan(Unit, AimCell, Edge);
		return true;
	}

	/** Un'azione di Branth nello slot dell'unita', con la variante indicata (o nessuna). */
	bool CoverTiePlanBranth(ARTUnit* Unit, const TCHAR* ActionId, const FName& VariantId, const FRTCellId& AimCell, ERTHexDirection Edge)
	{
		URTActionData* Action = CoverTieBranthAction(ActionId);
		if (Action == nullptr || !Unit->Abilities.IsValidIndex(CoverTieSlot)) { return false; }
		Unit->Abilities[CoverTieSlot] = Action;
		Unit->ActiveVariantId = VariantId;
		CoverTieWritePlan(Unit, AimCell, Edge);
		return true;
	}

	/** Integrita' della copertura su quel bordo, o 0 se il bordo e' scoperto. */
	int32 CoverTieIntegrityOn(const URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge)
	{
		const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
		const FRTHexCover* Entry = Data ? Data->CoverEntryOn(Edge) : nullptr;
		return Entry ? Entry->Integrity : 0;
	}

	/** Quante coperture ci sono sulla cella, su qualunque bordo. */
	int32 CoverTieCoversOn(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		int32 Count = 0;
		if (const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr)
		{
			for (const FRTHexCover& Cover : Data->Covers)
			{
				if (Cover.Type != ERTHexCoverType::None) { ++Count; }
			}
		}
		return Count;
	}

	/** Una voce di rifiuto per stordimento, di CHIUNQUE: `Fallback`/`Cancelled` con la causa `Stunned` in `Amount`. */
	bool CoverTieIsStunRefusal(const FRTTurnLogEntry& E)
	{
		return E.Category == ERTLogCategory::Fallback && E.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
			&& E.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned);
	}

	/** Gli eventi `AbilityActivated` che hanno quell'unita' per sorgente. */
	int32 CoverTieCountActivations(const ARTTurnManager* TM, int32 UnitId)
	{
		int32 Count = 0;
		for (const FRTResolvedEvent& Event : TM->ResolvedTimelineForTest())
		{
			if (Event.Type == ERTResolvedEventType::AbilityActivated && Event.SourceStableUnitId == UnitId) { ++Count; }
		}
		return Count;
	}

	/** Gli indici di spawn dei `Members`, nell'ordine REALE di raccolta: la funzione vera, `SortUnitsForResolution`, sulle unita' del mondo. */
	TArray<int32> CoverTieRealOrder(const FCoverTieBed& Bed, const TArray<int32>& Members)
	{
		TArray<ARTUnit*> All = Bed.Units;
		All.Add(Bed.Foe);
		URTActionQueueLibrary::SortUnitsForResolution(All);
		TArray<int32> Out;
		for (ARTUnit* Unit : All)
		{
			const int32 Index = Bed.Units.IndexOfByKey(Unit);
			if (Index != INDEX_NONE && Members.Contains(Index)) { Out.Add(Index); }
		}
		return Out;
	}

	/** Il turno: lock-in e tick fino alla fine della risoluzione. */
	void CoverTieRunTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
	}

	/** Gli `StableUnitId` delle unita' di scena dopo il turno (li assegna `EnsureMatchRoster` al primo lock-in). */
	TArray<int32> CoverTieIdsAfterTurn(const FCoverTieBed& Bed)
	{
		TArray<int32> Ids;
		for (const TWeakObjectPtr<ARTUnit>& Weak : Bed.Weak)
		{
			const ARTUnit* After = Weak.Get(/*bEvenIfPendingKill=*/ true);
			Ids.Add(After ? After->StableUnitId : 0);
		}
		return Ids;
	}

	bool CoverTieIdsAreUsable(const TArray<int32>& Ids)
	{
		for (int32 I = 0; I < Ids.Num(); ++I)
		{
			if (Ids[I] == 0) { return false; }
			for (int32 J = I + 1; J < Ids.Num(); ++J)
			{
				if (Ids[I] == Ids[J]) { return false; }
			}
		}
		return true;
	}

	/**
	 * UNA run di una scena: costruisce il mondo, pianifica, gioca UN turno e asserisce. Ritorna falso se una PREMESSA cade
	 * (e in quel caso non legge gli esiti: un guasto di fixture non deve passare per un difetto del prodotto).
	 */
	bool CoverTieRun(FAutomationTestBase& Test, ECoverTieKind Kind, const FCoverTieScene& Scene,
		ECoverTieArm Arm = ECoverTieArm::Stunned)
	{
		const bool bCreate = Kind == ECoverTieKind::Create;
		// Il braccio si nomina solo dove la scena ha una stordita: altrove sarebbe un'etichetta che non dice niente.
		bool bHasStunnedRole = false;
		for (const FCoverTieUnit& Spec : Scene.Units) { bHasStunnedRole |= Spec.Role == ECoverTieRole::Stunned; }
		const FString Run = bHasStunnedRole
			? FString::Printf(TEXT("%s - %s - %s"), bCreate ? TEXT("erigere") : TEXT("spostare"), *Scene.Name, CoverTieArmName(Arm))
			: FString::Printf(TEXT("%s - %s"), bCreate ? TEXT("erigere") : TEXT("spostare"), *Scene.Name);
		const int32 N = Scene.Units.Num();
		const FRTCellId Target = CoverTieTargetCell();
		const ERTHexDirection ContestedEdge = bCreate ? ERTHexDirection::W : ERTHexDirection::E;

		// Chi contende in QUESTO braccio, e chi vince secondo la regola: la cella di partenza minore.
		TArray<int32> Group;
		for (int32 I = 0; I < N; ++I)
		{
			if (CoverTieContends(Scene.Units[I], Arm)) { Group.Add(I); }
		}
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: premessa: i contendenti sono almeno due"), *Run), Group.Num() >= 2))
		{
			return false;
		}
		TArray<int32> GroupByCell = Group;
		GroupByCell.Sort([&Scene](int32 A, int32 B)
		{
			return URTHexLibrary::StableLess(Scene.Units[A].Start, Scene.Units[B].Start);
		});
		const int32 ExpectedWinner = GroupByCell[0];

		FCoverTieBed Bed;
		if (!CoverTieOpenBed(Test, Run, Scene.Units, Bed)) { return false; }

		bool bOk = true;

		// Celle di partenza distinte fra loro e dall'avversario: due unita' sulla stessa cella sono un altro banco.
		for (int32 I = 0; I < N; ++I)
		{
			bOk &= Test.TestFalse(*FString::Printf(TEXT("%s: premessa: l'unita' %d non sta sulla cella dell'avversario"), *Run, I),
				Scene.Units[I].Start == CoverTieFoeCell());
			for (int32 J = I + 1; J < N; ++J)
			{
				bOk &= Test.TestFalse(*FString::Printf(TEXT("%s: premessa: le unita' %d e %d hanno celle di partenza distinte"), *Run, I, J),
					Scene.Units[I].Start == Scene.Units[J].Start);
			}
		}

		// Lo stato della mappa prima del turno.
		if (bCreate)
		{
			// Il bordo contestato e la sua altra faccia partono scoperti.
			bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il bordo contestato parte scoperto"), *Run),
				CoverTieIntegrityOn(Bed.Map, Target, ERTHexDirection::W), 0);
			bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la sua altra faccia parte scoperta"), *Run),
				CoverTieIntegrityOn(Bed.Map, FRTCellId(0, 0), ERTHexDirection::E), 0);
		}
		else
		{
			// La copertura di mappa che i contendenti si contendono di spostare, e quella di ogni spettatore.
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la copertura contesa e' posta"), *Run),
				URTHexCoverLibrary::AddCover(Bed.Map, Target, ERTHexDirection::W, ERTHexCoverType::Low, CoverTiePreplacedIntegrity));
			for (int32 I = 0; I < N; ++I)
			{
				if (Scene.Units[I].Role != ECoverTieRole::Bystander) { continue; }
				bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la copertura dello spettatore %d e' posta"), *Run, I),
					URTHexCoverLibrary::AddCover(Bed.Map, Scene.Units[I].Start, ERTHexDirection::W, ERTHexCoverType::Low,
						CoverTiePreplacedIntegrity));
			}
		}

		// I piani. Per l'erezione il vincitore atteso porta il pannello ADATTIVO e gli altri quello base: cio' che resta sul
		// bordo dice chi ha vinto anche a TurnLog muto.
		for (int32 I = 0; I < N; ++I)
		{
			const FCoverTieUnit& Spec = Scene.Units[I];
			if (!CoverTiePlans(Spec, Arm)) { continue; }
			const bool bBystander = Spec.Role == ECoverTieRole::Bystander;
			const FRTCellId AimCell = bBystander ? Spec.Start : Target;
			const ERTHexDirection AimEdge = bBystander ? ERTHexDirection::E : ContestedEdge;

			bool bPlanned = false;
			if (!bCreate)
			{
				bPlanned = CoverTiePlanBranth(Bed.Units[I], TEXT("Hero.Branth.Reconfigure"), NAME_None, AimCell, AimEdge);
			}
			else if (I == ExpectedWinner)
			{
				bPlanned = CoverTiePlanBranth(Bed.Units[I], TEXT("Hero.Branth.KineticPanel"),
					FName(TEXT("Hero.Branth.KineticPanel.Adaptive")), AimCell, AimEdge);
			}
			else
			{
				bPlanned = CoverTiePlanBase(Bed.Units[I], AimCell, AimEdge);
			}
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha ricevuto il piano"), *Run, I), bPlanned);
		}

		// L'UNICA riga che separa i bracci della stordita, DOPO il piano.
		for (int32 I = 0; I < N; ++I)
		{
			if (CoverTieIsStunnedNow(Scene.Units[I], Arm) && Bed.Units[I])
			{
				Bed.Units[I]->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns);
			}
		}

		// Le premesse sulle unita': il piano, la classe dell'azione, la portata, lo stordimento.
		int32 AdaptiveIntegrity = 0;
		for (int32 I = 0; I < N && bOk; ++I)
		{
			const FCoverTieUnit& Spec = Scene.Units[I];
			const ARTUnit* Unit = Bed.Units[I];
			bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: l'unita' %d e' stordita solo dove il braccio lo dichiara"), *Run, I),
				Unit->HasStatus(TAG_Status_Stunned), CoverTieIsStunnedNow(Spec, Arm));
			if (!CoverTiePlans(Spec, Arm))
			{
				bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: l'unita' %d non ha piano"), *Run, I),
					Unit->PlannedAbilityIndex, static_cast<int32>(INDEX_NONE));
				continue;
			}
			const URTActionData* Installed = Unit->GetAbility(CoverTieSlot);
			bOk &= Test.TestNotNull(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha l'azione nello slot %d"), *Run, I, CoverTieSlot), Installed);
			if (Installed == nullptr) { continue; }
			bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha il piano nello slot %d"), *Run, I, CoverTieSlot),
				Unit->PlannedAbilityIndex, CoverTieSlot);
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha una PRINCIPALE di Prep"), *Run, I),
				Installed->Def.Slot == ERTActionSlot::Main
				&& URTCatalogLibrary::MapResolutionPhase(Installed->Def.ResolutionPhase) == ERTMatchPhase::Prep);
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'azione dell'unita' %d erige o sposta, come la scena"), *Run, I),
				Installed->Def.StructureOp == (bCreate ? ERTStructureOp::CreateCover : ERTStructureOp::MoveCover));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'unita' %d e' utilizzabile"), *Run, I), Unit->CanUseAbility(CoverTieSlot));
			const FRTCellId Aimed = Spec.Role == ECoverTieRole::Bystander ? Spec.Start : Target;
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la cella mirata dall'unita' %d e' a portata"), *Run, I),
				URTHexLibrary::HexDistance(Spec.Start, Aimed) <= Installed->Def.RangeCells);
			if (bCreate && I == ExpectedWinner)
			{
				const FRTAbilityVariant* Variant = Installed->FindVariant(Unit->ActiveVariantId);
				const int32* Declared = Variant ? Variant->Parameters.Find(TEXT("Integrity")) : nullptr;
				AdaptiveIntegrity = Declared ? *Declared : 0;
			}
		}
		const int32 BaseIntegrity = FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low);
		if (bCreate)
		{
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: i due pannelli hanno integrita' diverse (adattivo %d, base %d)"), *Run,
				AdaptiveIntegrity, BaseIntegrity), AdaptiveIntegrity > 0 && AdaptiveIntegrity != BaseIntegrity);
		}

		// L'ordine di raccolta: la funzione vera, sulle unita' del mondo, deve dare l'ordine per cella di partenza.
		const TArray<int32> RealOrder = CoverTieRealOrder(Bed, Group);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'ordine reale di raccolta dei contendenti e' quello per cella di partenza"), *Run),
			RealOrder == GroupByCell);

		if (!bOk) { return false; }

		// ---- il turno ----
		CoverTieRunTurn(Bed.TM);

		const TArray<int32> Ids = CoverTieIdsAfterTurn(Bed);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: gli StableUnitId delle unita' sono non nulli e distinti"), *Run),
			CoverTieIdsAreUsable(Ids));
		if (!bOk) { return false; }
		if (Scene.bWinnerNotLowestId)
		{
			int32 LowestId = MAX_int32;
			for (const int32 Member : Group) { LowestId = FMath::Min(LowestId, Ids[Member]); }
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il primo raccolto NON ha lo StableUnitId minore dei contendenti"), *Run),
				Ids[ExpectedWinner] > LowestId);
			if (!bOk) { return false; }
		}

		// ---- gli osservabili ----
		int32 AppliedTotal = 0;
		int32 AppliedOnContested = 0;
		int32 WinnerSeen = INDEX_NONE;
		int32 RejectedTotal = 0;
		int32 StunRefusalsTotal = 0;
		TArray<int32> AppliedBy;
		TArray<int32> RejectedBy;
		TArray<int32> EnvironmentBy;
		TArray<int32> StunRefusalsBy;
		AppliedBy.Init(0, N);
		RejectedBy.Init(0, N);
		EnvironmentBy.Init(0, N);
		StunRefusalsBy.Init(0, N);
		for (const FRTTurnLogEntry& E : Bed.TM->GetTurnLog())
		{
			const int32 Who = Ids.IndexOfByKey(E.UnitId);
			if (E.Category == ERTLogCategory::Environment)
			{
				if (Who != INDEX_NONE) { ++EnvironmentBy[Who]; }
				if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverCreated)
					|| E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverMoved))
				{
					++AppliedTotal;
					if (Who != INDEX_NONE) { ++AppliedBy[Who]; }
					if (E.SrcCell == Target) { ++AppliedOnContested; WinnerSeen = Who; }
				}
				else if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverRejected))
				{
					++RejectedTotal;
					if (Who != INDEX_NONE) { ++RejectedBy[Who]; }
				}
			}
			else if (CoverTieIsStunRefusal(E) && E.Phase == ERTMatchPhase::Prep)
			{
				++StunRefusalsTotal;
				if (Who != INDEX_NONE) { ++StunRefusalsBy[Who]; }
			}
		}

		int32 Bystanders = 0;
		for (int32 I = 0; I < N; ++I)
		{
			if (Scene.Units[I].Role == ECoverTieRole::Bystander) { ++Bystanders; }
		}
		const int32 WinnerIntegrity = bCreate ? CoverTieIntegrityOn(Bed.Map, Target, ContestedEdge) : -1;

		Test.AddInfo(FString::Printf(
			TEXT("%s -> vincitore atteso u%d@%s, visto %s; applicati sul bordo %d; rifiutati in tutto %d; rifiuti per stordimento %d; ")
			TEXT("integrita' sul bordo contestato %d"),
			*Run, ExpectedWinner, *Scene.Units[ExpectedWinner].Start.ToString(),
			WinnerSeen == INDEX_NONE ? TEXT("nessuno") : *FString::Printf(TEXT("u%d@%s"), WinnerSeen, *Scene.Units[WinnerSeen].Start.ToString()),
			AppliedOnContested, RejectedTotal, StunRefusalsTotal, WinnerIntegrity));

		// 1. Esattamente UNA struttura applicata sul bordo contestato, ed e' quella del primo raccolto.
		Test.TestEqual(*FString::Printf(TEXT("%s: una sola struttura applicata sul bordo contestato"), *Run), AppliedOnContested, 1);
		Test.TestEqual(*FString::Printf(TEXT("%s: il vincitore e' il primo raccolto (indice di spawn; atteso u%d@%s)"), *Run,
			ExpectedWinner, *Scene.Units[ExpectedWinner].Start.ToString()), WinnerSeen, ExpectedWinner);
		Test.TestEqual(*FString::Printf(TEXT("%s: la timeline ha UN AbilityActivated del vincitore atteso"), *Run),
			CoverTieCountActivations(Bed.TM, Ids[ExpectedWinner]), 1);

		// 2. Tutti gli altri contendenti sono rifiutati, ognuno una volta, e nessun'altra voce di rifiuto esiste.
		for (const int32 Member : Group)
		{
			if (Member == ExpectedWinner) { continue; }
			Test.TestEqual(*FString::Printf(TEXT("%s: il contendente %d (%s) e' rifiutato una volta"), *Run, Member,
				*Scene.Units[Member].Start.ToString()), RejectedBy[Member], 1);
			Test.TestEqual(*FString::Printf(TEXT("%s: il contendente %d (%s) non applica nulla"), *Run, Member,
				*Scene.Units[Member].Start.ToString()), AppliedBy[Member], 0);
			Test.TestEqual(*FString::Printf(TEXT("%s: il contendente %d (%s) non attiva nulla"), *Run, Member,
				*Scene.Units[Member].Start.ToString()), CoverTieCountActivations(Bed.TM, Ids[Member]), 0);
		}
		Test.TestEqual(*FString::Printf(TEXT("%s: il vincitore atteso non e' rifiutato"), *Run), RejectedBy[ExpectedWinner], 0);
		Test.TestEqual(*FString::Printf(TEXT("%s: i rifiutati sono i contendenti meno il vincitore, e nessun altro"), *Run),
			RejectedTotal, Group.Num() - 1);

		// 3. La stordita: nel braccio stordito ha UN rifiuto per stordimento e nessuna voce ambientale (non e' fra i rifiutati, perche'
		// non e' entrata); nel braccio senza piano non ha niente; nel braccio libero contende come le altre (punto 1 e 2).
		int32 ExpectedRefusals = 0;
		for (int32 I = 0; I < N; ++I)
		{
			if (Scene.Units[I].Role != ECoverTieRole::Stunned) { continue; }
			if (Arm == ECoverTieArm::Free) { continue; }
			const int32 Refusals = Arm == ECoverTieArm::Stunned ? 1 : 0;
			ExpectedRefusals += Refusals;
			Test.TestEqual(*FString::Printf(TEXT("%s: la stordita %d (%s): voci di rifiuto per stordimento in Prep"), *Run, I,
				*Scene.Units[I].Start.ToString()), StunRefusalsBy[I], Refusals);
			Test.TestEqual(*FString::Printf(TEXT("%s: la stordita %d (%s): nessuna voce ambientale, non e' entrata"), *Run, I,
				*Scene.Units[I].Start.ToString()), EnvironmentBy[I], 0);
			Test.TestEqual(*FString::Printf(TEXT("%s: la stordita %d (%s) non attiva nulla"), *Run, I,
				*Scene.Units[I].Start.ToString()), CoverTieCountActivations(Bed.TM, Ids[I]), 0);
		}
		Test.TestEqual(*FString::Printf(TEXT("%s: nessun altro rifiuto per stordimento in Prep"), *Run), StunRefusalsTotal, ExpectedRefusals);

		// 4. Gli spettatori erigono (o spostano) altrove, ognuno una volta, e la contesa non li tocca.
		for (int32 I = 0; I < N; ++I)
		{
			if (Scene.Units[I].Role != ECoverTieRole::Bystander) { continue; }
			Test.TestEqual(*FString::Printf(TEXT("%s: lo spettatore %d (%s) applica la sua struttura una volta"), *Run, I,
				*Scene.Units[I].Start.ToString()), AppliedBy[I], 1);
			Test.TestEqual(*FString::Printf(TEXT("%s: lo spettatore %d (%s) non e' rifiutato"), *Run, I,
				*Scene.Units[I].Start.ToString()), RejectedBy[I], 0);
			Test.TestEqual(*FString::Printf(TEXT("%s: la copertura dello spettatore %d sta sul suo bordo E"), *Run, I),
				CoverTieIntegrityOn(Bed.Map, Scene.Units[I].Start, ERTHexDirection::E),
				bCreate ? BaseIntegrity : CoverTiePreplacedIntegrity);
			if (!bCreate)
			{
				Test.TestEqual(*FString::Printf(TEXT("%s: e il suo bordo W e' stato lasciato"), *Run),
					CoverTieIntegrityOn(Bed.Map, Scene.Units[I].Start, ERTHexDirection::W), 0);
			}
		}
		Test.TestEqual(*FString::Printf(TEXT("%s: le strutture applicate sono quella contesa e quelle degli spettatori"), *Run),
			AppliedTotal, 1 + Bystanders);

		// 5. La mappa. Erigere: sul bordo resta il pannello del vincitore atteso (l'adattivo, unico con quell'integrita').
		// Spostare: la copertura e' passata da W a E con la sua integrita'.
		if (bCreate)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: sul bordo contestato resta il pannello del primo raccolto (adattivo)"), *Run),
				WinnerIntegrity, AdaptiveIntegrity);
			Test.TestEqual(*FString::Printf(TEXT("%s: la cella contesa porta una sola copertura"), *Run), CoverTieCoversOn(Bed.Map, Target), 1);
		}
		else
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: la copertura contesa e' sul bordo E"), *Run),
				CoverTieIntegrityOn(Bed.Map, Target, ERTHexDirection::E), CoverTiePreplacedIntegrity);
			Test.TestEqual(*FString::Printf(TEXT("%s: e il bordo W e' stato lasciato"), *Run),
				CoverTieIntegrityOn(Bed.Map, Target, ERTHexDirection::W), 0);
			Test.TestEqual(*FString::Printf(TEXT("%s: la cella contesa porta una sola copertura"), *Run), CoverTieCoversOn(Bed.Map, Target), 1);
		}
		return true;
	}

	/**
	 * Il controllo di CELLA: le due facce dello stesso bordo fisico, (0,0) E e (1,0) W, mirate da due unita'. Non pareggiano:
	 * le ordina la cella, e vince quella che mira la faccia della cella MINORE, anche se l'altra e' raccolta per prima.
	 * `bLoserCollectedFirst` decide quale delle due partenze e' la minore.
	 */
	bool CoverTieRunFaces(FAutomationTestBase& Test, ECoverTieKind Kind, bool bLoserCollectedFirst)
	{
		const bool bCreate = Kind == ECoverTieKind::Create;
		const FString Run = FString::Printf(TEXT("%s - le due facce dello stesso bordo, %s"), bCreate ? TEXT("erigere") : TEXT("spostare"),
			bLoserCollectedFirst ? TEXT("chi perde e' raccolto per primo") : TEXT("chi vince e' raccolto per primo"));

		const FRTCellId LowCell(0, 0);   // la cella minore: la sua faccia E vince
		const FRTCellId HighCell(1, 0);  // la sua faccia W perde
		// Le partenze: spawn (vincitore, perdente); la minore e' raccolta per prima.
		const FRTCellId WinnerStart = bLoserCollectedFirst ? FRTCellId(0, 2) : FRTCellId(0, 1);
		const FRTCellId LoserStart = bLoserCollectedFirst ? FRTCellId(0, 1) : FRTCellId(0, 2);

		TArray<FCoverTieUnit> Specs;
		FCoverTieUnit Winner;
		Winner.Start = WinnerStart;
		FCoverTieUnit Loser;
		Loser.Start = LoserStart;
		Specs.Add(Winner);
		Specs.Add(Loser);

		FCoverTieBed Bed;
		if (!CoverTieOpenBed(Test, Run, Specs, Bed)) { return false; }

		bool bOk = true;
		if (bCreate)
		{
			bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la faccia E della cella minore parte scoperta"), *Run),
				CoverTieIntegrityOn(Bed.Map, LowCell, ERTHexDirection::E), 0);
			bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la faccia W della cella maggiore parte scoperta"), *Run),
				CoverTieIntegrityOn(Bed.Map, HighCell, ERTHexDirection::W), 0);
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il vincitore ha il piano"), *Run),
				CoverTiePlanBase(Bed.Units[0], LowCell, ERTHexDirection::E));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il perdente ha il piano"), *Run),
				CoverTiePlanBase(Bed.Units[1], HighCell, ERTHexDirection::W));
		}
		else
		{
			// Una copertura su ciascuna cella, ognuna sul bordo che NON e' quello comune: ciascun mover la porta sul bordo comune.
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la copertura della cella minore e' posta"), *Run),
				URTHexCoverLibrary::AddCover(Bed.Map, LowCell, ERTHexDirection::W, ERTHexCoverType::Low, CoverTiePreplacedIntegrity));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la copertura della cella maggiore e' posta"), *Run),
				URTHexCoverLibrary::AddCover(Bed.Map, HighCell, ERTHexDirection::E, ERTHexCoverType::Low, CoverTiePreplacedIntegrity));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il vincitore ha il piano"), *Run),
				CoverTiePlanBranth(Bed.Units[0], TEXT("Hero.Branth.Reconfigure"), NAME_None, LowCell, ERTHexDirection::E));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il perdente ha il piano"), *Run),
				CoverTiePlanBranth(Bed.Units[1], TEXT("Hero.Branth.Reconfigure"), NAME_None, HighCell, ERTHexDirection::W));
		}
		if (!bOk) { return false; }

		for (int32 I = 0; I < 2; ++I)
		{
			const ARTUnit* Unit = Bed.Units[I];
			const URTActionData* Installed = Unit->GetAbility(CoverTieSlot);
			bOk &= Test.TestNotNull(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha l'azione nello slot %d"), *Run, I, CoverTieSlot), Installed);
			if (Installed == nullptr) { continue; }
			bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha il piano nello slot %d"), *Run, I, CoverTieSlot),
				Unit->PlannedAbilityIndex, CoverTieSlot);
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha una PRINCIPALE di Prep che erige o sposta"), *Run, I),
				Installed->Def.Slot == ERTActionSlot::Main
				&& URTCatalogLibrary::MapResolutionPhase(Installed->Def.ResolutionPhase) == ERTMatchPhase::Prep
				&& Installed->Def.StructureOp == (bCreate ? ERTStructureOp::CreateCover : ERTStructureOp::MoveCover));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'unita' %d e' utilizzabile e non stordita"), *Run, I),
				Unit->CanUseAbility(CoverTieSlot) && !Unit->HasStatus(TAG_Status_Stunned));
			const FRTCellId Aimed = I == 0 ? LowCell : HighCell;
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la cella mirata dall'unita' %d e' a portata"), *Run, I),
				URTHexLibrary::HexDistance(Unit->Cell, Aimed) <= Installed->Def.RangeCells);
		}
		const TArray<int32> Members = { 0, 1 };
		const TArray<int32> RealOrder = CoverTieRealOrder(Bed, Members);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'ordine reale di raccolta e' quello dichiarato dalla riga"), *Run),
			RealOrder.Num() == 2 && RealOrder[0] == (bLoserCollectedFirst ? 1 : 0));
		if (!bOk) { return false; }

		CoverTieRunTurn(Bed.TM);

		const TArray<int32> Ids = CoverTieIdsAfterTurn(Bed);
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: premessa: gli StableUnitId sono non nulli e distinti"), *Run), CoverTieIdsAreUsable(Ids)))
		{
			return false;
		}

		int32 Applied = 0;
		int32 Rejected = 0;
		int32 AppliedBy = INDEX_NONE;
		int32 RejectedBy = INDEX_NONE;
		for (const FRTTurnLogEntry& E : Bed.TM->GetTurnLog())
		{
			if (E.Category != ERTLogCategory::Environment) { continue; }
			if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverCreated)
				|| E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverMoved))
			{
				++Applied;
				AppliedBy = Ids.IndexOfByKey(E.UnitId);
			}
			else if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverRejected))
			{
				++Rejected;
				RejectedBy = Ids.IndexOfByKey(E.UnitId);
			}
		}
		Test.TestEqual(*FString::Printf(TEXT("%s: una sola struttura applicata"), *Run), Applied, 1);
		Test.TestEqual(*FString::Printf(TEXT("%s: vince chi mira la faccia della cella minore (indice di spawn 0)"), *Run), AppliedBy, 0);
		Test.TestEqual(*FString::Printf(TEXT("%s: un solo rifiutato"), *Run), Rejected, 1);
		Test.TestEqual(*FString::Printf(TEXT("%s: e' chi mira la faccia della cella maggiore (indice di spawn 1)"), *Run), RejectedBy, 1);

		if (bCreate)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: la faccia E della cella minore ha la copertura"), *Run),
				CoverTieIntegrityOn(Bed.Map, LowCell, ERTHexDirection::E), FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low));
			Test.TestEqual(*FString::Printf(TEXT("%s: la faccia W della cella maggiore resta scoperta"), *Run),
				CoverTieIntegrityOn(Bed.Map, HighCell, ERTHexDirection::W), 0);
		}
		else
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: la copertura della cella minore e' passata sul bordo comune"), *Run),
				CoverTieIntegrityOn(Bed.Map, LowCell, ERTHexDirection::E), CoverTiePreplacedIntegrity);
			Test.TestEqual(*FString::Printf(TEXT("%s: e ha lasciato il bordo W"), *Run),
				CoverTieIntegrityOn(Bed.Map, LowCell, ERTHexDirection::W), 0);
			Test.TestEqual(*FString::Printf(TEXT("%s: quella della cella maggiore e' restata dov'era"), *Run),
				CoverTieIntegrityOn(Bed.Map, HighCell, ERTHexDirection::E), CoverTiePreplacedIntegrity);
			Test.TestEqual(*FString::Printf(TEXT("%s: senza spostarsi sul bordo comune"), *Run),
				CoverTieIntegrityOn(Bed.Map, HighCell, ERTHexDirection::W), 0);
		}
		return true;
	}

	// ---- le scene ----

	FCoverTieUnit CoverTieUnit(int32 X, int32 Y, int32 TeamId = 0, ECoverTieRole Role = ECoverTieRole::Contender)
	{
		FCoverTieUnit Unit;
		Unit.Start = FRTCellId(X, Y);
		Unit.TeamId = TeamId;
		Unit.Role = Role;
		return Unit;
	}

	FCoverTieScene CoverTieMakeScene(const TCHAR* Name, const TArray<FCoverTieUnit>& Units, bool bWinnerNotLowestId = false)
	{
		FCoverTieScene Scene;
		Scene.Name = Name;
		Scene.Units = Units;
		Scene.bWinnerNotLowestId = bWinnerNotLowestId;
		return Scene;
	}

	/** Contese semplici fra unita' non stordite. Le celle di partenza (0,0) < (0,1) < (0,2) sono tutte a portata di (1,0). */
	TArray<FCoverTieScene> CoverTiePlainScenes()
	{
		TArray<FCoverTieScene> Scenes;
		Scenes.Add(CoverTieMakeScene(TEXT("due contendenti, spawn contro l'ordine delle celle"),
			{ CoverTieUnit(0, 1), CoverTieUnit(0, 0) }));
		Scenes.Add(CoverTieMakeScene(TEXT("due contendenti, spawn nell'ordine delle celle"),
			{ CoverTieUnit(0, 0), CoverTieUnit(0, 1) }));
		Scenes.Add(CoverTieMakeScene(TEXT("tre contendenti"),
			{ CoverTieUnit(0, 2), CoverTieUnit(0, 0), CoverTieUnit(0, 1) }));
		// La squadra 1 sta nella cella minore (0,0) e ha lo StableUnitId MAGGIORE (gli id si assegnano per squadra e poi per cella):
		// il primo raccolto non e' quello dell'id minore.
		Scenes.Add(CoverTieMakeScene(TEXT("squadre diverse, il primo raccolto ha l'id maggiore"),
			{ CoverTieUnit(0, 1, 0), CoverTieUnit(0, 0, 1) }, /*bWinnerNotLowestId=*/ true));
		return Scenes;
	}

	/** La stordita prima, in mezzo e ultima nell'ordine di raccolta, fra due contendenti non storditi (X prima di Y). */
	TArray<FCoverTieScene> CoverTieStunnedScenes()
	{
		TArray<FCoverTieScene> Scenes;
		// raccolta: S(0,0) X(0,1) Y(0,2)
		Scenes.Add(CoverTieMakeScene(TEXT("la stordita e' la prima raccolta"),
			{ CoverTieUnit(0, 1), CoverTieUnit(0, 0, 0, ECoverTieRole::Stunned), CoverTieUnit(0, 2) }));
		// raccolta: X(0,0) S(0,1) Y(0,2)
		Scenes.Add(CoverTieMakeScene(TEXT("la stordita e' in mezzo"),
			{ CoverTieUnit(0, 2), CoverTieUnit(0, 1, 0, ECoverTieRole::Stunned), CoverTieUnit(0, 0) }));
		// raccolta: X(0,0) Y(0,1) S(0,2)
		Scenes.Add(CoverTieMakeScene(TEXT("la stordita e' l'ultima"),
			{ CoverTieUnit(0, 2, 0, ECoverTieRole::Stunned), CoverTieUnit(0, 1), CoverTieUnit(0, 0) }));
		return Scenes;
	}

	/**
	 * Liste lunghe. La prima mescola tre contendenti e tre spettatori in un ordine di raccolta che li alterna:
	 * B(-3,1) C(-2,1) B(-1,2) C(0,0) C(0,1) B(2,-2). La seconda ha tanti contendenti da portare l'introsort sul ramo a
	 * partizione (`Algo/IntroSort.h`): le prime celle a distanza 3 dal bersaglio, bersaglio escluso, con lo spawn al contrario.
	 */
	TArray<FCoverTieScene> CoverTieLongScenes()
	{
		TArray<FCoverTieScene> Scenes;
		Scenes.Add(CoverTieMakeScene(TEXT("tre contendenti fra tre spettatori"),
			{ CoverTieUnit(2, -2, 0, ECoverTieRole::Bystander), CoverTieUnit(0, 1), CoverTieUnit(-3, 1, 0, ECoverTieRole::Bystander),
			  CoverTieUnit(-2, 1), CoverTieUnit(-1, 2, 0, ECoverTieRole::Bystander), CoverTieUnit(0, 0) }));

		constexpr int32 ManyCount = 10;
		TArray<FRTCellId> Area = URTHexLibrary::HexArea(CoverTieTargetCell(), 3);
		Area.Remove(CoverTieTargetCell());
		Area.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
		TArray<FCoverTieUnit> Many;
		for (int32 I = FMath::Min(ManyCount, Area.Num()) - 1; I >= 0; --I)
		{
			Many.Add(CoverTieUnit(Area[I].X, Area[I].Y));
		}
		Scenes.Add(CoverTieMakeScene(*FString::Printf(TEXT("%d contendenti, ramo a partizione dell'introsort"), Many.Num()), Many));
		return Scenes;
	}

	void CoverTieRunPlain(FAutomationTestBase& Test, ECoverTieKind Kind)
	{
		for (const FCoverTieScene& Scene : CoverTiePlainScenes()) { CoverTieRun(Test, Kind, Scene); }
	}

	void CoverTieRunStunned(FAutomationTestBase& Test, ECoverTieKind Kind)
	{
		for (const FCoverTieScene& Scene : CoverTieStunnedScenes())
		{
			for (const ECoverTieArm Arm : { ECoverTieArm::Stunned, ECoverTieArm::Idle, ECoverTieArm::Free })
			{
				CoverTieRun(Test, Kind, Scene, Arm);
			}
		}
	}

	void CoverTieRunLong(FAutomationTestBase& Test, ECoverTieKind Kind)
	{
		for (const FCoverTieScene& Scene : CoverTieLongScenes()) { CoverTieRun(Test, Kind, Scene); }
	}

	void CoverTieRunFacesBothWays(FAutomationTestBase& Test, ECoverTieKind Kind)
	{
		CoverTieRunFaces(Test, Kind, /*bLoserCollectedFirst=*/ true);
		CoverTieRunFaces(Test, Kind, /*bLoserCollectedFirst=*/ false);
	}
}

/**
 * Erigere: a parita' di cella e di bordo vince la prima operazione raccolta. Senza il terzo criterio in `Pending.Sort` e'
 * rosso: il vincitore lo decide l'introsort, e con due contendenti e' il secondo raccolto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverTieCreateFirstCollectedWinsTest,
	"RefactorTactics.Actions.CoverStructures.ContestedEdge.CreateFirstCollectedWins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverTieCreateFirstCollectedWinsTest::RunTest(const FString&)
{
	CoverTieRunPlain(*this, ECoverTieKind::Create);
	return true;
}

/** Erigere con una stordita fra i contendenti: non entra, e il vincitore fra gli altri e' il primo raccolto. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverTieCreateStunnedContenderDoesNotEnterTest,
	"RefactorTactics.Actions.CoverStructures.ContestedEdge.CreateStunnedContenderDoesNotEnter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverTieCreateStunnedContenderDoesNotEnterTest::RunTest(const FString&)
{
	CoverTieRunStunned(*this, ECoverTieKind::Create);
	return true;
}

/** Erigere in liste lunghe: fra spettatori che erigono altrove, e con tanti contendenti da portare l'introsort sulla partizione. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverTieCreateLongListsKeepTheRuleTest,
	"RefactorTactics.Actions.CoverStructures.ContestedEdge.CreateLongListsKeepTheRule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverTieCreateLongListsKeepTheRuleTest::RunTest(const FString&)
{
	CoverTieRunLong(*this, ECoverTieKind::Create);
	return true;
}

/** Erigere: il terzo criterio e' il TERZO. Le due facce dello stesso bordo fisico le ordina la cella, non la raccolta. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverTieCreateCellOrderStillComesFirstTest,
	"RefactorTactics.Actions.CoverStructures.ContestedEdge.CreateCellOrderStillComesFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverTieCreateCellOrderStillComesFirstTest::RunTest(const FString&)
{
	CoverTieRunFacesBothWays(*this, ECoverTieKind::Create);
	return true;
}

/** Spostare: a parita' di cella e di bordo di destinazione vince il primo spostamento raccolto. Stessa forma del comparatore `Moves.Sort`. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverTieMoveFirstCollectedWinsTest,
	"RefactorTactics.Actions.CoverStructures.ContestedEdge.MoveFirstCollectedWins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverTieMoveFirstCollectedWinsTest::RunTest(const FString&)
{
	CoverTieRunPlain(*this, ECoverTieKind::Move);
	return true;
}

/** Spostare con una stordita fra i contendenti: non entra, e il vincitore fra gli altri e' il primo raccolto. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverTieMoveStunnedContenderDoesNotEnterTest,
	"RefactorTactics.Actions.CoverStructures.ContestedEdge.MoveStunnedContenderDoesNotEnter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverTieMoveStunnedContenderDoesNotEnterTest::RunTest(const FString&)
{
	CoverTieRunStunned(*this, ECoverTieKind::Move);
	return true;
}

/** Spostare in liste lunghe: fra spettatori che spostano la propria copertura, e con tanti contendenti da portare l'introsort sulla partizione. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverTieMoveLongListsKeepTheRuleTest,
	"RefactorTactics.Actions.CoverStructures.ContestedEdge.MoveLongListsKeepTheRule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverTieMoveLongListsKeepTheRuleTest::RunTest(const FString&)
{
	CoverTieRunLong(*this, ECoverTieKind::Move);
	return true;
}

/** Spostare: il terzo criterio e' il TERZO. Due mover che portano la propria copertura sulle due facce dello stesso bordo: li ordina la cella. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverTieMoveCellOrderStillComesFirstTest,
	"RefactorTactics.Actions.CoverStructures.ContestedEdge.MoveCellOrderStillComesFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverTieMoveCellOrderStillComesFirstTest::RunTest(const FString&)
{
	CoverTieRunFacesBothWays(*this, ECoverTieKind::Move);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
