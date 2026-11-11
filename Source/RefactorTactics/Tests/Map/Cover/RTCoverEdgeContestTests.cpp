#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/Catalog/RTCatalogLibrary.h"
#include "Ability/Catalog/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h" // StunnedDurationTurns
#include "Core/RTGameplayTags.h" // TAG_Status_Stunned
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/Cover/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/Actor/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Tests/Shared/RTAbilityFixtures.h"
#include "Tests/Shared/RTWorldFixtures.h"
#include "Turn/Actions/RTActionFallbackLibrary.h" // ERTActionInvalidReason::Stunned
#include "Turn/Actions/RTActionQueueLibrary.h" // SortUnitsForResolution: l'ordine di raccolta (#2922)
#include "Turn/Format/RTMatchSetupLibrary.h"
#include "Turn/Resolution/RTResolvedEvent.h"
#include "Turn/Log/RTTurnLog.h"
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
// pagano la ricarica come il vincitore: il banco la legge, per ogni contendente, dalla ricarica pubblica dell'unita'.
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
//   - LA STORDITA: la stessa scena in mondi diversi. Stordita (col piano e con `Status.Stunned`: [D-565] la rifiuta in
//     raccolta), senza piano (il controllo "senza di lei", nello stesso mondo e con gli stessi `StableUnitId`) e non
//     stordita (contende come ogni altra: il controllo POSITIVO, in cui la stordita, se e' la prima, vince). Stordita e
//     non stordita differiscono per UNA riga (lo stato, scritto dopo il piano); senza piano e non stordita per UN fatto
//     (il piano); stordita e senza piano ne differiscono DUE, piano e stato. Le premesse verificano ciascuna differenza.
//     Il vincitore fra gli altri non deve cambiare fra il mondo stordita e quello senza piano, e la stordita non ha
//     nessuna voce ambientale: non e' fra i rifiutati, perche' non e' entrata.
//   - LISTE LUNGHE: spettatori che erigono (o spostano) altrove nello stesso Prep, in mezzo ai contendenti, e una scena
//     con tante operazioni da portare l'introsort sul ramo a partizione (`Algo/IntroSort.h`), che nei formati noti non
//     si raggiunge ma che l'ordine totale deve coprire lo stesso. Che la scena lo raggiunga e' una PREMESSA asserita
//     (`bPartitionBranch`, `CoverTieIntroSortSelectionMax`): fino a 8 elementi l'introsort di UE 5.8 ordina per
//     selezione, da 9 partiziona, e su una lista corta un comparatore che non e' un ordine debole stretto (per esempio
//     `A.Order != B.Order`) puo' restare verde per caso.
//   - SQUADRE DIVERSE: il comparatore non legge la squadra, ma gli `StableUnitId` si assegnano per squadra e POI per cella
//     (`MatchRosterLess`), mentre la raccolta ordina per cella: con un contendente di squadra 1 in una cella minore, il
//     primo raccolto NON ha lo `StableUnitId` minore, e la riga distingue "il primo raccolto" da "l'id minore".
//   - CONTROLLO DI CELLA: le due facce dello stesso bordo fisico, mirate da due unita' (la cella minore sulla propria
//     faccia, la maggiore sulla sua), non pareggiano: le ordina la cella, e vince la minore anche se l'altra e' raccolta
//     per prima. Il terzo criterio e' il TERZO: se prendesse il posto della cella questa riga cadrebbe. Le coppie: E
//     contro W ((0,0) e (1,0)), dove l'ordine di cella e quello dell'enum COINCIDONO (E < W) e la riga non distingue
//     "ordina la cella" da "ordina il bordo"; SE contro NW ((0,0) e (0,1)), dove DISCORDANO (NW < SE) e vincerebbe l'altra
//     cella se il bordo precedesse la cella o se la cella mancasse.
//   - BORDI DIVERSI SULLA STESSA CELLA (solo spostare): due spostamenti sulla stessa cella con bordi di destinazione
//     DIVERSI si contendono l'unica copertura della cella (ciclo `Moves`): il primo applicato la porta dal bordo di
//     partenza al proprio, il secondo la trova sul bordo del primo e la porta al proprio, quindi il bordo finale e' quello
//     dell'ULTIMO applicato. Con (cella, bordo, raccolta) e' il bordo di valore enum maggiore; con (cella, raccolta,
//     bordo) sarebbe quello dell'ultimo raccolto. La scena li fa DISCORDARE (il primo raccolto mira il bordo di valore
//     enum maggiore) e l'oracolo e' il valore enum maggiore dei due bordi mirati, non il risultato della produzione.
//
// Premesse asserite in ogni run (se una cade il banco e' rosso per un guasto di fixture, e gli esiti non si leggono):
// l'azione e' nello slot 3, e' una PRINCIPALE di Prep con la `StructureOp` attesa, e' utilizzabile; la cella mirata e'
// a portata; la ricarica dichiarata e lo specchio coincidono e superano uno (altrimenti "pagata" e "mai pagata" si
// confonderebbero dopo il decremento del Cleanup); lo stordimento c'e' dove il braccio lo dichiara; le celle di partenza
// sono distinte; l'ordine reale di raccolta coincide con quello per cella; gli `StableUnitId` sono non nulli e distinti.
//
// Cio' che il banco NON misura (limiti dichiarati)
//   - due unita' sulla STESSA cella di partenza: li' decide lo `StableUnitId` di `SortUnitsForResolution`, che ha il suo
//     banco (`Actions.UnitOrderStableIdBeatsActorName`); qui le celle di partenza sono sempre distinte;
//   - la posizione del terzo criterio rispetto al bordo, per le EREZIONI: due erezioni sulla stessa cella e su bordi
//     diversi non si contendono nulla (`AddCover` le applica entrambe in qualunque ordine e la mappa finale e' la stessa),
//     quindi quella posizione non cambia ne' chi vince ne' la mappa. Cambia l'ORDINE in cui le due si applicano, cioe'
//     quello dei loro `AbilityActivated` in timeline (l'ordine di emissione, `EmitAbilityActivated`): non e' un esito di
//     contesa e nessuna riga lo legge. Per gli SPOSTAMENTI la posizione conta, perche' i due si contendono l'unica
//     copertura della cella, ed e' misurata (`MoveDifferentEdgesKeepEdgeOrder`);
//   - la rotazione gratuita del pannello adattivo: i crediti stanno in `DynamicCovers`, che e' `protected` e senza
//     accessor, e nessuna scena sposta una copertura che ne porti uno. Il PAGAMENTO invece e' pubblico (la ricarica
//     dell'unita', `GetAbilityCooldown`) e il banco lo legge per ogni contendente;
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

	/**
	 * I mondi di una scena con la stordita. Stordita e non stordita differiscono per UNA riga (lo stato, scritto dopo il
	 * piano); senza piano e non stordita per UN fatto (il piano); stordita e senza piano ne differiscono DUE. Le premesse
	 * verificano ciascuna differenza.
	 */
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
		bool bPartitionBranch = false;   //< premessa di riga: i contendenti superano `CoverTieIntroSortSelectionMax`, quindi l'introsort PARTIZIONA
	};

	/** Lo slot dell'azione sotto prova, lo stesso del banco di [D-565]. */
	constexpr int32 CoverTieSlot = 3;

	/** L'integrita' della copertura di mappa posta prima del turno nelle scene di spostamento: non e' un default del catalogo. */
	constexpr int32 CoverTiePreplacedIntegrity = 18;

	/**
	 * Fino a quanti elementi l'introsort di UE 5.8 ordina per SELEZIONE (`Algo/IntroSort.h`, `if( Count <= 8 )`): da uno in
	 * piu' parte il ramo a PARTIZIONE. E' un valore dell'Engine copiato qui: se l'Engine si aggiorna va riletto.
	 */
	constexpr int32 CoverTieIntroSortSelectionMax = 8;

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

	const TCHAR* CoverTieEdgeName(ERTHexDirection Edge)
	{
		switch (Edge)
		{
		case ERTHexDirection::E:  return TEXT("E");
		case ERTHexDirection::NE: return TEXT("NE");
		case ERTHexDirection::NW: return TEXT("NW");
		case ERTHexDirection::W:  return TEXT("W");
		case ERTHexDirection::SW: return TEXT("SW");
		case ERTHexDirection::SE: return TEXT("SE");
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
		// La scena che promette il ramo a partizione dell'introsort lo raggiunge davvero: fino a `CoverTieIntroSortSelectionMax`
		// elementi UE 5.8 ordina per selezione, e su quel ramo un comparatore che non e' un ordine debole stretto resta verde per caso.
		// Senza questa premessa una scena ristretta (`ManyCount`, il raggio di `HexArea`) manterrebbe il nome e mentirebbe in silenzio.
		if (Scene.bPartitionBranch
			&& !Test.TestTrue(*FString::Printf(TEXT("%s: premessa: i contendenti sono piu' di %d (ora %d): fino a %d elementi l'introsort ")
				TEXT("di UE 5.8 ordina per selezione, solo da %d partiziona, e questa scena promette il ramo a partizione"), *Run,
				CoverTieIntroSortSelectionMax, Group.Num(), CoverTieIntroSortSelectionMax, CoverTieIntroSortSelectionMax + 1),
				Group.Num() > CoverTieIntroSortSelectionMax))
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

		// L'UNICA riga che separa la stordita da quella non stordita (stesso piano), DOPO il piano. Il braccio senza piano si
		// distingue dalla non stordita per il solo piano, che il ciclo qui sopra non scrive (`CoverTiePlans`).
		for (int32 I = 0; I < N; ++I)
		{
			if (CoverTieIsStunnedNow(Scene.Units[I], Arm) && Bed.Units[I])
			{
				Bed.Units[I]->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns);
			}
		}

		// Le premesse sulle unita': il piano, la classe dell'azione, la portata, la ricarica, lo stordimento.
		int32 AdaptiveIntegrity = 0;
		// La ricarica attesa a fine turno di ogni unita' col piano: lo specchio meno il decremento del Cleanup, come nel banco di [D-565].
		TArray<int32> ExpectedCooldownAfter;
		ExpectedCooldownAfter.Init(static_cast<int32>(INDEX_NONE), N);
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
			// La ricarica che il turno addebita: `ConsumeAbility` scrive lo specchio, che deve coincidere con la dichiarata e superare
			// uno, o "pagata" (1) e "mai pagata" (0) si confonderebbero dopo il decremento del Cleanup.
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la ricarica dell'unita' %d e' dichiarata, uguale allo specchio e maggiore di uno"), *Run, I),
				Installed->Def.CooldownTurns > 1 && Installed->Def.CooldownTurns == Installed->CooldownTurns);
			ExpectedCooldownAfter[I] = Installed->CooldownTurns - 1;
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

		// 2b. Tutti i contendenti PAGANO, il vincitore e i rifiutati. L'erezione addebita la ricarica in raccolta, prima del rifiuto;
		// lo spostamento in applicazione (riuscito) o nel rifiuto, e nessuna scena ha un credito di rotazione gratuita da spendere.
		// Il Cleanup decrementa di uno: lo stesso valore atteso del banco di [D-565] (`Mirror - 1`, la ricarica e' PARTITA).
		for (const int32 Member : Group)
		{
			const ARTUnit* After = Bed.Weak[Member].Get(/*bEvenIfPendingKill=*/ true);
			Test.TestEqual(*FString::Printf(TEXT("%s: il contendente %d (%s) ha la ricarica pagata (specchio meno il decremento del Cleanup)"), *Run,
				Member, *Scene.Units[Member].Start.ToString()),
				After ? After->GetAbilityCooldown(CoverTieSlot) : static_cast<int32>(INDEX_NONE), ExpectedCooldownAfter[Member]);
		}

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
	 * Le due facce dello stesso bordo fisico: la cella MINORE (`StableLess`) con la sua faccia verso la maggiore, la cella
	 * maggiore con la sua faccia verso la minore. `EarlyStart` e `LateStart` sono le partenze delle due unita' (la minore e'
	 * raccolta per prima), a portata di entrambe le celle mirate. `bEdgeOrderDiscordsFromCellOrder` e' la premessa di riga:
	 * la faccia della cella minore ha il valore enum MAGGIORE, quindi un ordine per bordo sceglierebbe l'altra cella.
	 */
	struct FCoverTieFacePair
	{
		const TCHAR* Name = TEXT("");
		FRTCellId LowCell;
		ERTHexDirection LowFace = ERTHexDirection::E;
		FRTCellId HighCell;
		ERTHexDirection HighFace = ERTHexDirection::E;
		FRTCellId EarlyStart;
		FRTCellId LateStart;
		bool bEdgeOrderDiscordsFromCellOrder = false;
	};

	/**
	 * Il controllo di CELLA: le due facce dello stesso bordo fisico (`Pair`), mirate da due unita'. Non pareggiano: le ordina
	 * la cella, e vince quella che mira la faccia della cella MINORE, anche se l'altra e' raccolta per prima.
	 * `bLoserCollectedFirst` decide quale delle due partenze e' la minore.
	 */
	bool CoverTieRunFaces(FAutomationTestBase& Test, ECoverTieKind Kind, const FCoverTieFacePair& Pair, bool bLoserCollectedFirst)
	{
		const bool bCreate = Kind == ECoverTieKind::Create;
		const FString Run = FString::Printf(TEXT("%s - le due facce dello stesso bordo (%s), %s"), bCreate ? TEXT("erigere") : TEXT("spostare"),
			Pair.Name, bLoserCollectedFirst ? TEXT("chi perde e' raccolto per primo") : TEXT("chi vince e' raccolto per primo"));

		const FRTCellId LowCell = Pair.LowCell;   // la cella minore: la sua faccia vince
		const FRTCellId HighCell = Pair.HighCell; // la sua faccia perde
		const ERTHexDirection LowFace = Pair.LowFace;
		const ERTHexDirection HighFace = Pair.HighFace;
		// Lo spostamento parte da una copertura per cella, sul bordo OPPOSTO a quello comune: la faccia dell'altra cella e' il suo valore.
		const ERTHexDirection LowFrom = HighFace;
		const ERTHexDirection HighFrom = LowFace;

		// La geometria della coppia, riletta dal progetto invece che data per buona: la minore e' minore per `StableLess`, ognuna
		// vede l'altra dalla faccia dichiarata (e' lo stesso bordo fisico), la partenza raccolta per prima e' la minore, e la riga
		// e' concorde o discorde fra ordine di cella e ordine dell'enum come dichiara.
		ERTHexDirection LowToHigh = ERTHexDirection::E;
		ERTHexDirection HighToLow = ERTHexDirection::E;
		bool bPair = true;
		bPair &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la cella %s e' minore della cella %s per StableLess"), *Run,
			*LowCell.ToString(), *HighCell.ToString()), URTHexLibrary::StableLess(LowCell, HighCell));
		bPair &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la faccia %s della cella minore guarda la cella maggiore"), *Run, CoverTieEdgeName(LowFace)),
			URTHexLibrary::DirectionBetween(LowCell, HighCell, LowToHigh) && LowToHigh == LowFace);
		bPair &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la faccia %s della cella maggiore guarda la cella minore"), *Run, CoverTieEdgeName(HighFace)),
			URTHexLibrary::DirectionBetween(HighCell, LowCell, HighToLow) && HighToLow == HighFace);
		bPair &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la faccia della cella minore ha il valore enum maggiore solo dove la riga lo dichiara"), *Run),
			static_cast<uint8>(LowFace) > static_cast<uint8>(HighFace), Pair.bEdgeOrderDiscordsFromCellOrder);
		bPair &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la partenza raccolta per prima e' minore dell'altra"), *Run),
			URTHexLibrary::StableLess(Pair.EarlyStart, Pair.LateStart));
		if (!bPair) { return false; }

		// Le partenze: spawn (vincitore, perdente); la partenza minore e' raccolta per prima.
		const FRTCellId WinnerStart = bLoserCollectedFirst ? Pair.LateStart : Pair.EarlyStart;
		const FRTCellId LoserStart = bLoserCollectedFirst ? Pair.EarlyStart : Pair.LateStart;

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
			bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la faccia %s della cella minore parte scoperta"), *Run, CoverTieEdgeName(LowFace)),
				CoverTieIntegrityOn(Bed.Map, LowCell, LowFace), 0);
			bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: la faccia %s della cella maggiore parte scoperta"), *Run, CoverTieEdgeName(HighFace)),
				CoverTieIntegrityOn(Bed.Map, HighCell, HighFace), 0);
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il vincitore ha il piano"), *Run),
				CoverTiePlanBase(Bed.Units[0], LowCell, LowFace));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il perdente ha il piano"), *Run),
				CoverTiePlanBase(Bed.Units[1], HighCell, HighFace));
		}
		else
		{
			// Una copertura su ciascuna cella, ognuna sul bordo che NON e' quello comune: ciascun mover la porta sul bordo comune.
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la copertura della cella minore e' posta sul bordo %s"), *Run, CoverTieEdgeName(LowFrom)),
				URTHexCoverLibrary::AddCover(Bed.Map, LowCell, LowFrom, ERTHexCoverType::Low, CoverTiePreplacedIntegrity));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la copertura della cella maggiore e' posta sul bordo %s"), *Run, CoverTieEdgeName(HighFrom)),
				URTHexCoverLibrary::AddCover(Bed.Map, HighCell, HighFrom, ERTHexCoverType::Low, CoverTiePreplacedIntegrity));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il vincitore ha il piano"), *Run),
				CoverTiePlanBranth(Bed.Units[0], TEXT("Hero.Branth.Reconfigure"), NAME_None, LowCell, LowFace));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il perdente ha il piano"), *Run),
				CoverTiePlanBranth(Bed.Units[1], TEXT("Hero.Branth.Reconfigure"), NAME_None, HighCell, HighFace));
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
			Test.TestEqual(*FString::Printf(TEXT("%s: la faccia %s della cella minore ha la copertura"), *Run, CoverTieEdgeName(LowFace)),
				CoverTieIntegrityOn(Bed.Map, LowCell, LowFace), FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low));
			Test.TestEqual(*FString::Printf(TEXT("%s: la faccia %s della cella maggiore resta scoperta"), *Run, CoverTieEdgeName(HighFace)),
				CoverTieIntegrityOn(Bed.Map, HighCell, HighFace), 0);
		}
		else
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: la copertura della cella minore e' passata sul bordo comune %s"), *Run, CoverTieEdgeName(LowFace)),
				CoverTieIntegrityOn(Bed.Map, LowCell, LowFace), CoverTiePreplacedIntegrity);
			Test.TestEqual(*FString::Printf(TEXT("%s: e ha lasciato il bordo %s"), *Run, CoverTieEdgeName(LowFrom)),
				CoverTieIntegrityOn(Bed.Map, LowCell, LowFrom), 0);
			Test.TestEqual(*FString::Printf(TEXT("%s: quella della cella maggiore e' restata dov'era (%s)"), *Run, CoverTieEdgeName(HighFrom)),
				CoverTieIntegrityOn(Bed.Map, HighCell, HighFrom), CoverTiePreplacedIntegrity);
			Test.TestEqual(*FString::Printf(TEXT("%s: senza spostarsi sul bordo comune %s"), *Run, CoverTieEdgeName(HighFace)),
				CoverTieIntegrityOn(Bed.Map, HighCell, HighFace), 0);
		}
		return true;
	}

	/**
	 * Due spostamenti sulla STESSA cella con bordi di destinazione DIVERSI. Non pareggiano nei due criteri, ma l'ordine fra il
	 * bordo e la raccolta conta lo stesso, perche' si contendono l'UNICA copertura della cella (ciclo `Moves`): il primo applicato
	 * la porta dal bordo di partenza al proprio, il secondo la trova sul bordo del primo e la porta al proprio, quindi il bordo
	 * finale e' quello dell'ULTIMO applicato. Con (cella, bordo, raccolta) e' il bordo di valore enum maggiore; con
	 * (cella, raccolta, bordo) sarebbe quello dell'ultimo raccolto. La scena li fa DISCORDARE: il primo raccolto mira il bordo
	 * di valore enum maggiore. L'oracolo e' il valore enum maggiore dei due bordi mirati, letto dalla scena e non dalla produzione.
	 */
	bool CoverTieRunMoveDifferentEdges(FAutomationTestBase& Test)
	{
		const FString Run = TEXT("spostare - due bordi di destinazione diversi sulla stessa cella");
		const FRTCellId Target = CoverTieTargetCell();
		const ERTHexDirection FromEdge = ERTHexDirection::W; // dove sta la copertura di mappa prima del turno

		// In ordine di SPAWN, che non e' l'ordine di raccolta (lo decide la cella di partenza): u0 parte da (0,1) e mira E, u1 parte
		// da (0,0), e' raccolta per prima e mira SE, il valore enum maggiore.
		TArray<FCoverTieUnit> Specs;
		FCoverTieUnit SpawnedFirst;
		SpawnedFirst.Start = FRTCellId(0, 1);
		FCoverTieUnit SpawnedSecond;
		SpawnedSecond.Start = FRTCellId(0, 0);
		Specs.Add(SpawnedFirst);
		Specs.Add(SpawnedSecond);
		const ERTHexDirection ToEdges[2] = { ERTHexDirection::E, ERTHexDirection::SE };

		// Dalla scena e non dalla produzione: chi e' raccolto per primo (la cella di partenza minore) e quale bordo deve restare
		// (il valore enum maggiore dei due mirati).
		TArray<int32> ByCell = { 0, 1 };
		ByCell.Sort([&Specs](int32 A, int32 B) { return URTHexLibrary::StableLess(Specs[A].Start, Specs[B].Start); });
		const int32 First = ByCell[0];
		const int32 Second = ByCell[1];
		const ERTHexDirection HighestEdge = static_cast<uint8>(ToEdges[0]) > static_cast<uint8>(ToEdges[1]) ? ToEdges[0] : ToEdges[1];

		bool bOk = true;
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: i due bordi mirati (%s, %s) sono distinti fra loro e dal bordo di partenza (%s)"), *Run,
			CoverTieEdgeName(ToEdges[0]), CoverTieEdgeName(ToEdges[1]), CoverTieEdgeName(FromEdge)),
			ToEdges[0] != ToEdges[1] && ToEdges[0] != FromEdge && ToEdges[1] != FromEdge);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il primo raccolto (u%d) mira il bordo di valore enum MAGGIORE (%s), il secondo (u%d) il minore (%s): ")
			TEXT("raccolta ed enum discordano"), *Run, First, CoverTieEdgeName(ToEdges[First]), Second, CoverTieEdgeName(ToEdges[Second])),
			static_cast<uint8>(ToEdges[First]) > static_cast<uint8>(ToEdges[Second]));
		if (!bOk) { return false; }

		FCoverTieBed Bed;
		if (!CoverTieOpenBed(Test, Run, Specs, Bed)) { return false; }

		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la copertura contesa e' posta sul bordo di partenza %s"), *Run, CoverTieEdgeName(FromEdge)),
			URTHexCoverLibrary::AddCover(Bed.Map, Target, FromEdge, ERTHexCoverType::Low, CoverTiePreplacedIntegrity));
		bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: sulla cella bersaglio c'e' una sola copertura, quella da spostare"), *Run),
			CoverTieCoversOn(Bed.Map, Target), 1);
		for (int32 I = 0; I < Specs.Num(); ++I)
		{
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha ricevuto il piano di spostare sul bordo %s"), *Run, I,
				CoverTieEdgeName(ToEdges[I])),
				CoverTiePlanBranth(Bed.Units[I], TEXT("Hero.Branth.Reconfigure"), NAME_None, Target, ToEdges[I]));
		}
		if (!bOk) { return false; }

		for (int32 I = 0; I < Specs.Num(); ++I)
		{
			const ARTUnit* Unit = Bed.Units[I];
			const URTActionData* Installed = Unit->GetAbility(CoverTieSlot);
			bOk &= Test.TestNotNull(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha l'azione nello slot %d"), *Run, I, CoverTieSlot), Installed);
			if (Installed == nullptr) { continue; }
			bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha il piano nello slot %d"), *Run, I, CoverTieSlot),
				Unit->PlannedAbilityIndex, CoverTieSlot);
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'unita' %d ha una PRINCIPALE di Prep che sposta"), *Run, I),
				Installed->Def.Slot == ERTActionSlot::Main
				&& URTCatalogLibrary::MapResolutionPhase(Installed->Def.ResolutionPhase) == ERTMatchPhase::Prep
				&& Installed->Def.StructureOp == ERTStructureOp::MoveCover);
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'unita' %d e' utilizzabile e non stordita"), *Run, I),
				Unit->CanUseAbility(CoverTieSlot) && !Unit->HasStatus(TAG_Status_Stunned));
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il piano dell'unita' %d dichiara il bordo %s"), *Run, I, CoverTieEdgeName(ToEdges[I])),
				Unit->bHasPlannedCoverEdge && Unit->PlannedCoverEdge == ToEdges[I]);
			bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la cella bersaglio e' a portata dell'unita' %d"), *Run, I),
				URTHexLibrary::HexDistance(Unit->Cell, Target) <= Installed->Def.RangeCells);
		}
		const TArray<int32> Members = { 0, 1 };
		const TArray<int32> RealOrder = CoverTieRealOrder(Bed, Members);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'ordine reale di raccolta e' quello per cella di partenza"), *Run), RealOrder == ByCell);
		if (!bOk) { return false; }

		CoverTieRunTurn(Bed.TM);

		const TArray<int32> Ids = CoverTieIdsAfterTurn(Bed);
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: premessa: gli StableUnitId sono non nulli e distinti"), *Run), CoverTieIdsAreUsable(Ids)))
		{
			return false;
		}

		// Le voci del TurnLog: due spostamenti riusciti, uno a testa, ognuno verso il bordo mirato dalla propria unita'.
		int32 MovedTotal = 0;
		int32 RejectedTotal = 0;
		int32 CreatedTotal = 0;
		TArray<int32> MovedBy;
		MovedBy.Init(0, Specs.Num());
		for (const FRTTurnLogEntry& E : Bed.TM->GetTurnLog())
		{
			if (E.Category != ERTLogCategory::Environment) { continue; }
			if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverMoved))
			{
				++MovedTotal;
				const int32 Who = Ids.IndexOfByKey(E.UnitId);
				if (!MovedBy.IsValidIndex(Who)) { continue; }
				++MovedBy[Who];
				Test.TestEqual(*FString::Printf(TEXT("%s: la voce di spostamento dell'unita' %d parte dalla cella bersaglio"), *Run, Who),
					E.SrcCell.ToString(), Target.ToString());
				Test.TestEqual(*FString::Printf(TEXT("%s: la voce di spostamento dell'unita' %d guarda verso il bordo che ha mirato (%s)"), *Run, Who,
					CoverTieEdgeName(ToEdges[Who])), E.TgtCell.ToString(), URTHexLibrary::Neighbor(Target, ToEdges[Who]).ToString());
				Test.TestEqual(*FString::Printf(TEXT("%s: la voce di spostamento dell'unita' %d porta l'integrita' della copertura spostata"), *Run, Who),
					E.Amount, CoverTiePreplacedIntegrity);
			}
			else if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverRejected)) { ++RejectedTotal; }
			else if (E.Outcome == static_cast<uint8>(ERTEnvironmentOutcome::CoverCreated)) { ++CreatedTotal; }
		}
		Test.TestEqual(*FString::Printf(TEXT("%s: due spostamenti riusciti in tutto"), *Run), MovedTotal, 2);
		for (int32 I = 0; I < Specs.Num(); ++I)
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: l'unita' %d ha UNA voce di spostamento"), *Run, I), MovedBy[I], 1);
			Test.TestEqual(*FString::Printf(TEXT("%s: l'unita' %d ha UN AbilityActivated"), *Run, I), CoverTieCountActivations(Bed.TM, Ids[I]), 1);
		}
		Test.TestEqual(*FString::Printf(TEXT("%s: nessun rifiuto: i due bordi di destinazione non pareggiano"), *Run), RejectedTotal, 0);
		Test.TestEqual(*FString::Printf(TEXT("%s: nessuna erezione"), *Run), CreatedTotal, 0);

		// La mappa: la copertura e' una sola, e sta sul bordo che l'ultimo applicato le ha dato.
		int32 SeenEdge = static_cast<int32>(INDEX_NONE);
		for (int32 D = 0; D < 6; ++D)
		{
			if (CoverTieIntegrityOn(Bed.Map, Target, static_cast<ERTHexDirection>(D)) > 0) { SeenEdge = D; }
		}
		Test.AddInfo(FString::Printf(TEXT("%s -> il primo raccolto u%d mira %s, il secondo u%d mira %s; bordo finale atteso %s (valore enum maggiore), visto %s"),
			*Run, First, CoverTieEdgeName(ToEdges[First]), Second, CoverTieEdgeName(ToEdges[Second]), CoverTieEdgeName(HighestEdge),
			SeenEdge == INDEX_NONE ? TEXT("nessuno") : CoverTieEdgeName(static_cast<ERTHexDirection>(SeenEdge))));
		Test.TestEqual(*FString::Printf(TEXT("%s: la cella bersaglio porta una sola copertura"), *Run), CoverTieCoversOn(Bed.Map, Target), 1);
		Test.TestEqual(*FString::Printf(TEXT("%s: il bordo finale e' %s, il valore enum maggiore dei due mirati (per ordine di raccolta sarebbe %s)"), *Run,
			CoverTieEdgeName(HighestEdge), CoverTieEdgeName(ToEdges[Second])), SeenEdge, static_cast<int32>(HighestEdge));
		Test.TestEqual(*FString::Printf(TEXT("%s: la copertura ha conservato la propria integrita'"), *Run),
			CoverTieIntegrityOn(Bed.Map, Target, HighestEdge), CoverTiePreplacedIntegrity);
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

	FCoverTieScene CoverTieMakeScene(const TCHAR* Name, const TArray<FCoverTieUnit>& Units, bool bWinnerNotLowestId = false,
		bool bPartitionBranch = false)
	{
		FCoverTieScene Scene;
		Scene.Name = Name;
		Scene.Units = Units;
		Scene.bWinnerNotLowestId = bWinnerNotLowestId;
		Scene.bPartitionBranch = bPartitionBranch;
		return Scene;
	}

	FCoverTieFacePair CoverTieMakeFacePair(const TCHAR* Name, const FRTCellId& LowCell, ERTHexDirection LowFace, const FRTCellId& HighCell,
		ERTHexDirection HighFace, const FRTCellId& EarlyStart, const FRTCellId& LateStart, bool bEdgeOrderDiscordsFromCellOrder)
	{
		FCoverTieFacePair Pair;
		Pair.Name = Name;
		Pair.LowCell = LowCell;
		Pair.LowFace = LowFace;
		Pair.HighCell = HighCell;
		Pair.HighFace = HighFace;
		Pair.EarlyStart = EarlyStart;
		Pair.LateStart = LateStart;
		Pair.bEdgeOrderDiscordsFromCellOrder = bEdgeOrderDiscordsFromCellOrder;
		return Pair;
	}

	/**
	 * Le coppie di facce del controllo di cella. E contro W, (0,0) e (1,0): la cella minore ha il valore enum minore (E < W), quindi
	 * l'ordine di cella e quello dell'enum COINCIDONO e la riga non li distingue. SE contro NW, (0,0) e (0,1): la cella minore ha il
	 * valore enum MAGGIORE (NW < SE), quindi un ordine per bordo, o senza la cella, farebbe vincere l'altra. Le partenze stanno a
	 * portata di entrambe le celle mirate e mai su di esse.
	 */
	TArray<FCoverTieFacePair> CoverTieFacePairs()
	{
		TArray<FCoverTieFacePair> Pairs;
		Pairs.Add(CoverTieMakeFacePair(TEXT("E contro W"), FRTCellId(0, 0), ERTHexDirection::E, FRTCellId(1, 0), ERTHexDirection::W,
			FRTCellId(0, 1), FRTCellId(0, 2), /*bEdgeOrderDiscordsFromCellOrder=*/ false));
		Pairs.Add(CoverTieMakeFacePair(TEXT("SE contro NW"), FRTCellId(0, 0), ERTHexDirection::SE, FRTCellId(0, 1), ERTHexDirection::NW,
			FRTCellId(-1, 1), FRTCellId(-1, 2), /*bEdgeOrderDiscordsFromCellOrder=*/ true));
		return Pairs;
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
	 * partizione (`Algo/IntroSort.h`): le prime celle (per `StableLess`) entro distanza 3 dal bersaglio (`HexArea` e' un disco),
	 * bersaglio escluso, con lo spawn al contrario. Che i contendenti superino la soglia dell'introsort lo asserisce la premessa
	 * `bPartitionBranch`: se `ManyCount` o il raggio si riducessero la riga cadrebbe, invece di mentire sul proprio nome.
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
		Scenes.Add(CoverTieMakeScene(*FString::Printf(TEXT("%d contendenti, ramo a partizione dell'introsort"), Many.Num()), Many,
			/*bWinnerNotLowestId=*/ false, /*bPartitionBranch=*/ true));
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
		for (const FCoverTieFacePair& Pair : CoverTieFacePairs())
		{
			CoverTieRunFaces(Test, Kind, Pair, /*bLoserCollectedFirst=*/ true);
			CoverTieRunFaces(Test, Kind, Pair, /*bLoserCollectedFirst=*/ false);
		}
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

/**
 * Erigere: il terzo criterio e' il TERZO. Le due facce dello stesso bordo fisico le ordina la cella, non la raccolta ne' il
 * bordo: nella coppia SE contro NW l'ordine di cella e quello dell'enum discordano, in E contro W coincidono.
 */
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

/**
 * Spostare: il terzo criterio e' il TERZO. Due mover che portano la propria copertura sulle due facce dello stesso bordo: li
 * ordina la cella, non il bordo (coppia SE contro NW, dove discordano) ne' la raccolta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverTieMoveCellOrderStillComesFirstTest,
	"RefactorTactics.Actions.CoverStructures.ContestedEdge.MoveCellOrderStillComesFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverTieMoveCellOrderStillComesFirstTest::RunTest(const FString&)
{
	CoverTieRunFacesBothWays(*this, ECoverTieKind::Move);
	return true;
}

/**
 * Spostare con bordi di destinazione DIVERSI sulla stessa cella: non pareggiano, ma si contendono l'unica copertura, e il bordo
 * finale e' quello dell'ultimo applicato. Con (cella, bordo, raccolta) e' il bordo di valore enum maggiore, non quello dell'ultimo
 * raccolto: pinna la posizione del terzo criterio rispetto al bordo in `Moves.Sort`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCoverTieMoveDifferentEdgesKeepEdgeOrderTest,
	"RefactorTactics.Actions.CoverStructures.ContestedEdge.MoveDifferentEdgesKeepEdgeOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCoverTieMoveDifferentEdgesKeepEdgeOrderTest::RunTest(const FString&)
{
	CoverTieRunMoveDifferentEdges(*this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
