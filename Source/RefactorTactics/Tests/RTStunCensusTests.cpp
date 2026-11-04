// #3962 - Il CENSIMENTO delle azioni principali che `Status.Stunned` nega (follow-up di #3958 / RIC-5).
//
// ## Il difetto che questo file chiude, a meta'
//
// Lo stordimento toglie l'azione principale (D-416 (1)), e il rifiuto e' scritto PER SITO: ogni funzione di
// `ARTTurnManager` che consuma una principale deve chiamare `RefuseMainActionIfStunned` (anche `ResolveDash`, per la
// mobilita' che prende pure la principale: D-567, #3964; nessuna azione di catalogo la percorre, vedi
// `EveryMainActionHasAKnownSite`). L'elenco si ricava dal codice.
//
// DA D-569 (#3973) LE MOBILITA' NON SONO PIU' UN CASO A PARTE: la stordita non compie nessun movimento volontario, quindi
// `ResolveDash` rifiuta OGNI mobilita' rapida (qualunque slot occupi) e `BeginMovementResolution` rifiuta il camminare, entrambi
// con la stessa `RefuseMainActionIfStunned`. Non sono "azioni principali" e non hanno una categoria in questo file: le prova
// `EveryMobilityIsRefusedWhenStunned` (in `RTStunnedMovementTests.cpp`, sotto lo stesso prefisso `StunCensus`), guidato dai
// cataloghi con `IsFastMovement` come questo lo e' con `TakesMainSlot`, e il banco `Status.StunnedMovement.*`. Il comando qui
// sotto li elenca entrambi: i siti del movimento (`ResolveDash` e `BeginMovementResolution`) sono la differenza rispetto
// alla tabella `GuardedSites()`.
//
//     git grep -n 'RefuseMainActionIfStunned(' -- Source/RefactorTactics ':!Source/RefactorTactics/Tests'
//
// Alcuni siti sono stati trovati DOPO, ciascuno leggendo o misurando e mai per un test che scorresse il catalogo:
// l'Overwatch armato da uno stordito (`Status.StunSilencesAnArmedWatcher`), la cura e la purificazione (#3954) e le
// strutture di bordo (#3958). I test scritti a mano hanno una riga per sito (`Status.StunDeniedMainActionPaysItsCooldown`)
// o per azione (`Actions.CoverStructures.*`): un sito nuovo che dimenticasse la guardia non cadrebbe in nessuno.
//
// ## I test, e che cosa ciascuno vede
//
// - `EveryMainActionHasAKnownSite` (STATICO): ogni azione con `TakesMainSlot` dei cataloghi (kit d'eroe, generiche,
//   core, equipaggiamento) ha una macro-fase che un sito noto consuma. PUO' rilevare, dicendo che cosa decidere: una
//   principale sul percorso Dash (`Slot == MovementAndMain` o macro-fase Dash: D-567, #3964), che ha la guardia ma non
//   ha una categoria con la sua riga comportamentale; una principale con
//   macro-fase Move o Planning, che nessuna categoria dichiara (un'anomalia di dato); un VALORE DI ENUM che il resolver
//   usa per instradare e che questo file non dichiara (`RTActionDef.h` avverte che aggiungere un `ERTStructureOp` NON fa
//   fallire la compilazione). NON PUO' rilevare un sito NUOVO che consuma una principale: non cambia nessuna chiave che
//   legge (dopo lo scarto di quelle fasi restano Prep, Blast e Cleanup, e una categoria le prende tutte), e lo vede solo
//   una riga comportamentale, e solo se la fixture ci passa.
// - `EveryMainActionIsRefusedWhenStunned` (COMPORTAMENTALE): per ogni azione del catalogo un turno vero, su mondi
//   identici fuori dallo stordimento e dal piano (vedi `EArm`). Premesse asserite, controllo positivo (la stessa
//   pianificazione da una NON stordita produce l'effetto), poi il divieto: una sola voce di rifiuto `Fallback` /
//   `Cancelled` / `Stunned` che nomina l'azione, nella fase che la categoria dichiara; nessun `AbilityActivated`;
//   la ricarica parte (uno o piu' addebiti non si distinguono: `ConsumeAbility` scrive un valore fisso); nessun effetto.
// - `EveryGuardedSiteHasARow` (STATICO): ogni CATEGORIA della tabella ha almeno un'azione del catalogo, o e' dichiarata
//   senza azioni per nome e ragione in `AllowedEmptyCategories`; quindi ogni sito ha almeno una riga comportamentale.
//   E i test che coprono altrove le categorie dichiarate "non coperte" esistono ancora.
//
// ## La tabella azione -> categoria -> sito NON e' scritta a mano
//
// Ogni run del secondo test la stampa, una riga per azione: `STUNCENSUS|origine|ActionId|categoria|sito|fase|esito|controllo`.
// Si rigenera con
//
//     python tools/suite/esegui.py RefactorTactics.Status.StunCensus -v --log <file>
//
// e si legge con `grep STUNCENSUS <file>`. E' la sola forma che non invecchia: nessun conteggio qui, nessuna riga
// copiata in un documento. Le righe `STUNCENSUS-DEBOLE` nominano le azioni il cui unico canale di controllo e'
// l'attivazione (la clausola "nessun effetto" e' vuota per loro); le righe `STUNCENSUS-SENZA-ATTIVAZIONE` quelle il cui
// controllo non emette nessun `AbilityActivated` (la clausola "nessuna attivazione" e' vuota per loro).
//
// ## Cosa il censimento NON vede (dichiarato, non nascosto)
//
// 1. UN SITO NUOVO che consuma una principale (una funzione nuova, in una categoria esistente o fuori da tutte): non
//    cambia nessuna chiave che il controllo statico legge, e lo vede una riga solo se la fixture passa da li'. La
//    fixture e' fissa (mappa piatta, un nemico, unita' viva, azione disponibile, piano scritto a mano): i percorsi che
//    chiedono altro (caduta, ricarica residua, piano incompleto) sono coperti dai test per sito citati in `NotCovered`.
// 2. IL SITO NON SI LEGGE DALLA FASE: `ResolvePrep` e `ResolveCoverStructures` scrivono entrambe Prep;
//    `ResolveCleanseActions`, `CollectHealActions` e `CollectAttackIntents` scrivono tutte Blast. Che UN sito sia
//    osservato lo prova la riga le cui azioni PASSANO da quel sito e solo da quello (la mutazione per sito), non la fase.
// 3. LA TABELLA DI INSTRADAMENTO E' UNA SECONDA SEDE della conoscenza del resolver: se il resolver sposta un'azione su un
//    sito che scrive una fase DIVERSA, la clausola "il rifiuto compare nella fase che la categoria dichiara" diventa rossa
//    invece di mentire. Fra siti che scrivono la stessa fase (punto 2) non cade: la colonna `sito` di `STUNCENSUS` e' la
//    dichiarazione del predicato, non una misura.
// 4. LA PIANIFICAZIONE: il click e `ValidatePlan` accettano il piano di una stordita (RIC-1, #3134). La riga scrive il
//    piano a mano, come i test per sito.
// 5. Le azioni fuori catalogo: vedi `NotCovered`.
//
// Helper in un namespace NOMINATO (`RTStunCensusTestsInternal`): la unity build condivide la translation unit con gli altri
// file di test, e un namespace anonimo con firme omonime e' una ridefinizione (`Meta.AnonymousHelpersDoNotCollideUnderUnity`).

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Tests/RTAbilityFixtures.h"
#include "Tests/RTWorldFixtures.h" // MakeWorld/DestroyWorld: il mondo di prova condiviso (spawn e porta restano locali, vedi l'header)
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTMatchStateHash.h"
#include "Turn/RTResolvedEvent.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTStunCensusTestsInternal
{
	// ---------------------------------------------------------------------------------------------
	// COSTANTI DELLA FIXTURE
	// ---------------------------------------------------------------------------------------------

	/**
	 * La ricarica forzata dell'azione sotto prova. Abbastanza da sopravvivere al decremento del Cleanup dello stesso turno
	 * e restare leggibile: con una ricarica da 1 il Cleanup la riporterebbe a zero prima che il test guardi, e un'asserzione
	 * "ha pagato" sarebbe verde anche senza pagamento (la trappola di `PagaCooldown` in `RTPayWithoutHitTests.cpp`).
	 * Residuo a fine turno: `CensusCooldown - 1` = la ricarica e' partita; `0` = non e' partita. Il residuo NON dice quanti
	 * addebiti sono avvenuti: `ConsumeAbility` scrive un valore FISSO (`RTUnit.cpp`), quindi un secondo addebito dopo il
	 * primo lascia lo stesso valore. Un doppio addebito si distingue solo da una ricarica gia' in corso, ed e' la riga
	 * "gia' in ricarica" di `Status.StunDeniedMainActionPaysItsCooldown`; qui la ricarica prima del turno e' zero (asserito).
	 */
	constexpr int32 CensusCooldown = 3;

	/** Lo slot dove si monta un'azione core che nessun eroe porta: dentro l'array dei cooldown, e non lo slot 0. */
	constexpr int32 CoreSlot = 3;

	/**
	 * La portata dell'arma di chi porta l'azione. Il catalogo scrive `RangeCells <= 0` per "portata dell'arma" (il resolver
	 * la prende dallo specchio `URTActionData::RangeCells`, `RTTurnManager_Blast.cpp`): senza, un attacco generico che
	 * nessun eroe porta finirebbe in `OutOfRange` anche per una non stordita, e il controllo positivo sarebbe vuoto.
	 */
	constexpr int32 WeaponRange = 3;

	/** Dove sta il nemico quando l'azione non lo bersaglia: lontano, inerte, e fuori da ogni cella mirata. */
	constexpr int32 FarFoeX = -3;

	// ---------------------------------------------------------------------------------------------
	// MONDO DI PROVA
	// ---------------------------------------------------------------------------------------------

	/**
	 * Il mondo si distrugge su OGNI uscita: una riga che esce presto per una premessa mancata non lo lascia aperto.
	 * Crea e distrugge col mondo di prova condiviso (`RTWorldFixtures::MakeWorld` / `DestroyWorld`), non con una copia locale.
	 */
	struct FCensusWorldScope
	{
		UWorld* World = nullptr;

		FCensusWorldScope() : World(RTWorldFixtures::MakeWorld()) {}
		~FCensusWorldScope() { RTWorldFixtures::DestroyWorld(World); }
		FCensusWorldScope(const FCensusWorldScope&) = delete;
		FCensusWorldScope& operator=(const FCensusWorldScope&) = delete;
	};

	ARTUnit* SpawnCensusUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit) { return nullptr; }
		Unit->TeamId = TeamId;
		Unit->bIsBotControlled = false; // i piani li scriviamo noi
		Unit->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		Unit->PlannedCell = Cell; // ferma: qui si misura l'azione principale, non il movimento
		Unit->Shield = 0;         // [D-224]: qui si misura se un effetto avviene, non quanto scudo lo assorbe
		return Unit;
	}

	void RunCensusTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	// ---------------------------------------------------------------------------------------------
	// LE CATEGORIE: azione -> sito che la consuma -> fase del rifiuto
	// ---------------------------------------------------------------------------------------------

	ERTMatchPhase MacroPhaseOf(const FRTActionDef& Def)
	{
		return URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase);
	}

	/** Come `IsCoreAction` del resolver: l'`ActionId` o la derivazione. Il resolver instrada queste per NOME. */
	bool DerivesFromCore(const FRTActionDef& Def, const TCHAR* CoreId)
	{
		const FName Core(CoreId);
		return Def.ActionId == Core || Def.DerivedFromActionId == Core;
	}

	// L'ORDINE della tabella conta solo dove due predicati possono corrispondere alla stessa azione: la prima categoria che
	// corrisponde e' quella che la prende (`Classify`). Le precedenze sono queste, lette dal resolver:
	//  - 'struttura di bordo' per prima: `ResolveCoverStructures` gira dentro `ResolvePrep` prima di ogni altro ciclo e prende
	//    ogni `StructureOp` diverso da `None` e da `SetDoorState`, qualunque sia la fase dichiarata;
	//  - le specifiche prima del catch-all della loro fase: 'armare predittiva', poi 'armare Overwatch' (l'ordine in cui
	//    `ResolvePrep` le prova), poi 'Prep su chi agisce'; 'topologia' e 'porta' prima di 'attacco e controllo';
	//  - 'purificazione' e 'cura', che il resolver prende per derivazione qualunque sia la fase dichiarata, dopo 'Prep su chi
	//    agisce' (`ResolvePrep` gira per primo) e prima di 'ambientale' e di 'attacco e controllo' (`ResolveEnvironment` e il
	//    ciclo di `CollectAttackIntents` vengono dopo).
	// Per le altre coppie le macro-fasi sono disgiunte: l'ordine non decide niente.

	/**
	 * `ResolveCoverStructures` (dal D-565): ogni `StructureOp` diverso da `None` e da `SetDoorState`. E' la regola NEGATIVA
	 * del resolver (`RTTurnManager.cpp`: il ciclo esce su `None` e su `SetDoorState`, tratta `MoveCover` in un ramo proprio e
	 * manda tutto il resto al ramo che erige una copertura), e `RTActionDef.h` avverte che un valore nuovo di `ERTStructureOp`
	 * erige una copertura senza che il compilatore se ne accorga: elencare `CreateCover` e `MoveCover` farebbe cadere un valore
	 * nuovo nel catch-all della sua fase (`IsPrepOnSelf`, `IsBlastIntent`), mentre il resolver lo manda a questo sito.
	 */
	bool IsCoverStructure(const FRTActionDef& Def)
	{
		return Def.StructureOp != ERTStructureOp::None && Def.StructureOp != ERTStructureOp::SetDoorState;
	}

	/** `ResolvePrep`, ramo predittivo: si ARMA li' e risolve al boundary del Move. */
	bool IsPredictiveArming(const FRTActionDef& Def)
	{
		return MacroPhaseOf(Def) == ERTMatchPhase::Prep && Def.PredictiveTargeting != ERTPredictiveTargeting::None;
	}

	/** `ResolvePrep`, ramo Overwatch: il resolver lo riconosce per `ActionId`, perche' e' una generica. */
	bool IsOverwatchArming(const FRTActionDef& Def)
	{
		return MacroPhaseOf(Def) == ERTMatchPhase::Prep && Def.ActionId == FName(TEXT("Action.Overwatch"));
	}

	/** `ResolvePrep`, ramo "istanza": stance, scudo e stati su chi agisce. */
	bool IsPrepOnSelf(const FRTActionDef& Def)
	{
		return MacroPhaseOf(Def) == ERTMatchPhase::Prep;
	}

	/** `ResolveCleanseActions`: prende `Action.Cleanse` o chi ne deriva, qualunque sia la fase dichiarata. */
	bool IsCleanse(const FRTActionDef& Def)
	{
		return DerivesFromCore(Def, TEXT("Action.Cleanse"));
	}

	/** `CollectHealActions`: prende `Action.Heal` o chi ne deriva. Una cura che non deriva da li' e' un attacco per il resolver. */
	bool IsHeal(const FRTActionDef& Def)
	{
		return DerivesFromCore(Def, TEXT("Action.Heal"));
	}

	/** `ResolveEnvironment`: le azioni la cui macro-fase e' Cleanup (`Environment` e `Cleanup`). */
	bool IsEnvironmental(const FRTActionDef& Def)
	{
		return MacroPhaseOf(Def) == ERTMatchPhase::Cleanup;
	}

	/** `CollectAttackIntents`, ramo `ModifyArc`: si intercetta in cima al ciclo, DOPO la guardia. */
	bool IsTopology(const FRTActionDef& Def)
	{
		return MacroPhaseOf(Def) == ERTMatchPhase::Blast && DerivesFromCore(Def, TEXT("Action.ModifyArc"));
	}

	/** `CollectAttackIntents`: le porte non sono coperture, e tornano al Blast con il bordo dichiarato. */
	bool IsDoor(const FRTActionDef& Def)
	{
		return MacroPhaseOf(Def) == ERTMatchPhase::Blast && Def.StructureOp == ERTStructureOp::SetDoorState;
	}

	/** `CollectAttackIntents`: tutto cio' che e' ancora pianificato in `Control` e `Attack`. */
	bool IsBlastIntent(const FRTActionDef& Def)
	{
		return MacroPhaseOf(Def) == ERTMatchPhase::Blast;
	}

	struct FCategory
	{
		const TCHAR* Name;
		const TCHAR* Site;              //< la funzione di `ARTTurnManager` che consuma l'azione e chiama la guardia
		ERTMatchPhase RefusalPhase;     //< la fase che quel sito scrive nella voce di rifiuto
		bool (*Matches)(const FRTActionDef&);
	};

	const TArray<FCategory>& AllCategories()
	{
		static const TArray<FCategory> Table = {
			{ TEXT("struttura di bordo"),        TEXT("ResolveCoverStructures"), ERTMatchPhase::Prep,    &IsCoverStructure },
			{ TEXT("armare predittiva"),         TEXT("ResolvePrep"),            ERTMatchPhase::Prep,    &IsPredictiveArming },
			{ TEXT("armare Overwatch"),          TEXT("ResolvePrep"),            ERTMatchPhase::Prep,    &IsOverwatchArming },
			{ TEXT("Prep su chi agisce"),        TEXT("ResolvePrep"),            ERTMatchPhase::Prep,    &IsPrepOnSelf },
			{ TEXT("purificazione"),             TEXT("ResolveCleanseActions"),  ERTMatchPhase::Blast,   &IsCleanse },
			{ TEXT("cura"),                      TEXT("CollectHealActions"),     ERTMatchPhase::Blast,   &IsHeal },
			{ TEXT("ambientale"),                TEXT("ResolveEnvironment"),     ERTMatchPhase::Cleanup, &IsEnvironmental },
			{ TEXT("topologia (ModifyArc)"),     TEXT("CollectAttackIntents"),   ERTMatchPhase::Blast,   &IsTopology },
			{ TEXT("porta"),                     TEXT("CollectAttackIntents"),   ERTMatchPhase::Blast,   &IsDoor },
			{ TEXT("attacco e controllo"),       TEXT("CollectAttackIntents"),   ERTMatchPhase::Blast,   &IsBlastIntent },
		};
		return Table;
	}

	/**
	 * I siti che consumano un'azione principale e chiamano `RefuseMainActionIfStunned`, DICHIARATI qui a parte dalle
	 * categorie: e' il confronto fra i siti e le categorie a dire se un sito resta senza categorie, o una categoria senza un
	 * sito dichiarato (`CoverageProblems`, in `EveryGuardedSiteHasARow`).
	 * Si tiene d'accordo col comando in testa al file; un sito nuovo si aggiunge qui E come categoria.
	 * ⚠️ **Eccezione dichiarata: `ResolveDash`** (D-567, #3964; da D-569 per OGNI mobilita') chiama la guardia, ma non sta qui ne'
	 * in una categoria: non e' un sito di azioni PRINCIPALI. Per la mobilita' che prende anche la principale nessuna azione di
	 * catalogo lo percorre, e `EveryMainActionHasAKnownSite` la vieta finche' non ha la sua riga: la prova
	 * `Status.StunDeniesAMobilityThatAlsoCostsTheMainAction`, con un'azione costruita. Per le mobilita' del solo movimento, che
	 * sono tutte quelle spedite, la riga e' `Status.StunCensus.EveryMobilityIsRefusedWhenStunned`. Stesso discorso per
	 * `BeginMovementResolution` (D-569), che la chiama in fase Move per il camminare.
	 */
	const TArray<const TCHAR*>& GuardedSites()
	{
		static const TArray<const TCHAR*> Sites = {
			TEXT("ResolveCoverStructures"),
			TEXT("ResolvePrep"),
			TEXT("ResolveCleanseActions"),
			TEXT("CollectHealActions"),
			TEXT("CollectAttackIntents"),
			TEXT("ResolveEnvironment"),
		};
		return Sites;
	}

	/** L'indice della categoria che consuma l'azione, o `INDEX_NONE` se nessuna la conosce. */
	int32 Classify(const FRTActionDef& Def)
	{
		const TArray<FCategory>& Categories = AllCategories();
		for (int32 I = 0; I < Categories.Num(); ++I)
		{
			if (Categories[I].Matches(Def)) { return I; }
		}
		return INDEX_NONE;
	}

	template <typename TEnum>
	FString EnumNameOf(TEnum Value)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Value)) : FString::FromInt(static_cast<int32>(Value));
	}

	// ---------------------------------------------------------------------------------------------
	// I VALORI DI ENUM CON CUI IL RESOLVER INSTRADA, e che cosa ne e' di ciascuno
	// ---------------------------------------------------------------------------------------------

	struct FEnumRole
	{
		const TCHAR* Value;
		const TCHAR* Role;
	};

	const TArray<FEnumRole>& StructureOpRoles()
	{
		static const TArray<FEnumRole> Roles = {
			{ TEXT("None"),         TEXT("non tocca le strutture di bordo") },
			{ TEXT("CreateCover"),  TEXT("sito ResolveCoverStructures (Prep): erige una copertura") },
			{ TEXT("MoveCover"),    TEXT("sito ResolveCoverStructures (Prep): sposta una copertura") },
			{ TEXT("SetDoorState"), TEXT("sito CollectAttackIntents (Blast): ramo porta, con il bordo dichiarato") },
		};
		return Roles;
	}

	const TArray<FEnumRole>& ResolutionPhaseRoles()
	{
		static const TArray<FEnumRole> Roles = {
			{ TEXT("Snapshot"),       TEXT("macro-fase Planning: nessuna azione risolve qui e nessuna categoria la dichiara; una principale con questa fase e' un'anomalia di dato (CollectAttackIntents salta solo il Cleanup: il Blast la prenderebbe come intento)") },
			{ TEXT("Preparation"),    TEXT("macro-fase Prep: ResolvePrep e ResolveCoverStructures") },
			{ TEXT("FastMovement"),   TEXT("macro-fase Dash: ResolveDash, mobilita' (Slot Movement); una principale qui passa dalla guardia di ResolveDash (D-567, #3964) ma nessuna categoria di questo file la dichiara: vietata finche' qualcuno non la dichiara con la sua riga") },
			{ TEXT("NormalMovement"), TEXT("macro-fase Move: ResolveMovement, mobilita' (Slot Movement); una principale qui e' un'anomalia di dato che nessuna categoria dichiara (CollectAttackIntents salta solo il Cleanup: il Blast la prenderebbe come intento)") },
			{ TEXT("Control"),        TEXT("macro-fase Blast: ResolveCleanseActions e CollectAttackIntents") },
			{ TEXT("Attack"),         TEXT("macro-fase Blast: CollectHealActions e CollectAttackIntents") },
			{ TEXT("Environment"),    TEXT("macro-fase Cleanup: ResolveEnvironment") },
			{ TEXT("Cleanup"),        TEXT("macro-fase Cleanup: ResolveEnvironment") },
		};
		return Roles;
	}

	const TArray<FEnumRole>& ActionSlotRoles()
	{
		static const TArray<FEnumRole> Roles = {
			{ TEXT("None"),            TEXT("non occupa slot (Action.Wait): non e' una principale") },
			{ TEXT("Movement"),        TEXT("mobilita': negata alla stordita da ResolveDash (D-569, #3973: nessun movimento volontario, scatti compresi) e provata per ogni mobilita' dei cataloghi da EveryMobilityIsRefusedWhenStunned") },
			{ TEXT("Main"),            TEXT("l'azione principale: e' il soggetto di questo censimento") },
			{ TEXT("MovementAndMain"), TEXT("consuma anche la principale: negata alla stordita da ResolveDash (D-567, #3964; da D-569 la nega come ogni mobilita', e il criterio non e' piu' lo slot), ma nessuna categoria di questo file la dichiara: vietato dal controllo statico finche' qualcuno non la dichiara con la sua riga") },
			{ TEXT("Reaction"),        TEXT("le reazioni le toglie ReactionBlockedThisTurn / DisarmPreparedReactions, non questa guardia") },
		};
		return Roles;
	}

	const TArray<FEnumRole>& PredictiveTargetingRoles()
	{
		static const TArray<FEnumRole> Roles = {
			{ TEXT("None"),     TEXT("non e' predittiva") },
			{ TEXT("LockCell"), TEXT("sito ResolvePrep (Prep): si arma su una cella bloccata") },
		};
		return Roles;
	}

	/**
	 * Ogni valore dell'enum e' dichiarato in `Roles`, e ogni voce di `Roles` e' un valore dell'enum. Falso, e rosso, per un
	 * valore non dichiarato (il resolver puo' instradarci senza che il compilatore se ne accorga) o per una voce fantasma.
	 * `WhatANewValueDoes` dice che cosa il resolver fa di un valore nuovo di QUELL'enum: e' la frase che il messaggio
	 * di un valore non dichiarato porta a chi lo ha aggiunto, e non e' la stessa per tutti gli enum controllati.
	 */
	template <typename TEnum>
	bool CheckEnumIsDeclared(FAutomationTestBase& Test, const TCHAR* EnumLabel, const TArray<FEnumRole>& Roles,
		const TCHAR* WhatANewValueDoes)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: l'enum e' riflesso"), EnumLabel), Enum)) { return false; }

		bool bOk = true;
		TSet<FString> Real;
		for (int32 I = 0; I < Enum->NumEnums() - 1; ++I) // l'ultimo e' il `_MAX` sintetico che UHT aggiunge
		{
			const FString Name = Enum->GetNameStringByIndex(I);
			Real.Add(Name);
			const bool bDeclared = Roles.ContainsByPredicate([&Name](const FEnumRole& Role) { return Name == Role.Value; });
			if (!bDeclared)
			{
				bOk = false;
				Test.AddError(FString::Printf(
					TEXT("%s::%s non e' dichiarato in questo test. %s Decidi a quale sito appartiene, se ne ha uno, e dichiaralo in "
						 "RTStunCensusTests.cpp (le tabelle *Roles e, se serve, la tabella delle categorie)."),
					EnumLabel, *Name, WhatANewValueDoes));
			}
		}
		for (const FEnumRole& Role : Roles)
		{
			if (!Real.Contains(FString(Role.Value)))
			{
				bOk = false;
				Test.AddError(FString::Printf(
					TEXT("%s::%s e' dichiarato in questo test ma non esiste piu' nell'enum: toglilo."), EnumLabel, Role.Value));
			}
		}
		return bOk;
	}

	// ---------------------------------------------------------------------------------------------
	// LE CATEGORIE NON COPERTE, per nome e col motivo
	// ---------------------------------------------------------------------------------------------

	struct FNotCovered
	{
		const TCHAR* Name;
		const TCHAR* Reason;
		TArray<const TCHAR*> CoveredBy;  //< i test che la coprono altrove (percorso completo di ciascuno); vuoto se nessuno
	};

	const TArray<FNotCovered>& NotCovered()
	{
		static const TArray<FNotCovered> List = {
			{ TEXT("Slot MovementAndMain (#3964)"),
			  TEXT("nessuna azione dei cataloghi lo dichiara: la guardia di ResolveDash (D-567) non si prova con una riga del "
				   "catalogo ma con un'azione COSTRUITA nel test, e il controllo statico (EveryMainActionHasAKnownSite) la VIETA "
				   "finche' qualcuno non dichiara la categoria con la sua riga comportamentale"),
			  { TEXT("RefactorTactics.Status.StunDeniesAMobilityThatAlsoCostsTheMainAction") } },
			{ TEXT("abilita' legacy senza ActionId (ARTUnit::EnsureDefaultAbilities)"),
			  TEXT("fuori da ogni catalogo, quindi non enumerabili; passano da CollectAttackIntents, coperto dalla categoria "
				   "'attacco e controllo'"),
			  {} },
			{ TEXT("Def sintetiche costruite dai test (per esempio Test.Stun, lo stordente di RTUnbalancedProneTests.cpp)"),
			  TEXT("fuori da ogni catalogo, quindi non enumerabili da un test guidato dal catalogo; se una stordita ne pianifica "
				   "una passa comunque dal sito della sua categoria"),
			  {} },
			{ TEXT("pre-condizione: azione gia' in ricarica"),
			  TEXT("non e' una categoria ma una pre-condizione: ConsumeAbility scrive un valore FISSO e la riporterebbe al valore "
				   "pieno. Il censimento NON la ripete per ogni azione: moltiplicherebbe le righe senza aggiungere un sito. La fissano "
				   "Status.StunDeniedMainActionPaysItsCooldown (Prep, Blast, Cleanup, cura, purificazione) e "
				   "Actions.CoverStructures.StunnedCasterChangesNothing (strutture di bordo)"),
			  { TEXT("RefactorTactics.Status.StunDeniedMainActionPaysItsCooldown"),
				TEXT("RefactorTactics.Actions.CoverStructures.StunnedCasterChangesNothing") } },
			{ TEXT("pre-condizione: attore caduto"),
			  TEXT("un cadavere stordito col piano addosso e' rifiutato dove il ciclo non filtra i morti (CollectAttackIntents, "
				   "ResolveEnvironment) e saltato altrove. Lo fissano Status.StunDeniedMainActionPaysItsCooldown (Blast, Cleanup, "
				   "cura, purificazione) e Actions.CoverStructures.StunnedCasterChangesNothing (strutture di bordo); per "
				   "ResolvePrep (armare, Prep su chi agisce) nessuno di questi test ha una riga dell'attore caduto"),
			  { TEXT("RefactorTactics.Status.StunDeniedMainActionPaysItsCooldown"),
				TEXT("RefactorTactics.Actions.CoverStructures.StunnedCasterChangesNothing") } },
			{ TEXT("pre-condizione: piano incompleto o fuori portata (strutture di bordo)"),
			  TEXT("il rifiuto per stordimento precede i Reject di raccolta e li sostituisce"),
			  { TEXT("RefactorTactics.Actions.CoverStructures.StunnedCasterChangesNothing") } },
			{ TEXT("rotazione gratuita di Reconfigure"),
			  TEXT("serve un turno in piu' per vederla sopravvivere: non e' una proprieta' per-azione del catalogo"),
			  { TEXT("RefactorTactics.Actions.CoverStructures.StunnedCasterKeepsTheFreeRotation") } },
			{ TEXT("Overwatch gia' armato e disarmo da stordimento"),
			  TEXT("lo stato armato e' privato e non entra ne' nell'hash ne' nel log: lo mostra solo un movimento che lo innesca"),
			  { TEXT("RefactorTactics.Status.StunSilencesAnArmedWatcher") } },
			{ TEXT("il movimento di una stordita (D-569: negato, scatti e camminare)"),
			  TEXT("non e' un'azione Main e non sta nelle categorie di questo file: lo nega ResolveDash per OGNI mobilita' rapida e "
				   "BeginMovementResolution per il camminare, con una voce Fallback/Cancelled/Stunned nella fase che risolve (Dash o "
				   "Move). Il censimento delle mobilita' guidato dai cataloghi e il banco del camminare lo coprono altrove"),
			  { TEXT("RefactorTactics.Status.StunCensus.EveryMobilityIsRefusedWhenStunned"),
				TEXT("RefactorTactics.Status.StunnedMovement.TheWalkIsRefusedWhenStunned"),
				TEXT("RefactorTactics.Status.StunnedMovement.TheStatusIsReadAtTheResolvingPhase") } },
		};
		return List;
	}

	/** I test che coprono altrove una voce di `NotCovered`, per la stampa: "nessun test" se nessuno. */
	FString CoveredByText(const FNotCovered& Item)
	{
		if (Item.CoveredBy.Num() == 0) { return TEXT("nessun test"); }
		TArray<FString> Names;
		for (const TCHAR* TestName : Item.CoveredBy) { Names.Add(FString(TestName)); }
		return FString::Join(Names, TEXT(", "));
	}

	// ---------------------------------------------------------------------------------------------
	// LE CATEGORIE CHE POSSONO RESTARE SENZA AZIONI, per nome e col motivo
	// ---------------------------------------------------------------------------------------------

	struct FAllowedEmpty
	{
		const TCHAR* Category;  //< il NOME di una categoria di `AllCategories()`
		const TCHAR* Reason;    //< perche' puo' restare senza azioni nei cataloghi, e che cosa ne e' del codice del suo sito
	};

	/**
	 * Le categorie che possono NON avere nessuna azione nei cataloghi, ognuna con la sua ragione. E' VUOTA: oggi ogni
	 * categoria ne ha almeno una (la colonna `categoria` delle righe `STUNCENSUS|` lo mostra).
	 *
	 * Il caso che questa lista aspetta e' D-264: se `Action.Cleanse` esce dai cataloghi, la categoria 'purificazione' si
	 * svuota e `ResolveCleanseActions` resta nel codice senza che nessuna riga lo tocchi. O si toglie il codice con la sua
	 * categoria e il suo sito, o si dichiara qui la categoria con la ragione: non si tace togliendo il sito dal censimento.
	 *
	 * Modello: `Dichiarate` in `Catalog.EveryCoreActionIsReachableOrDeclared`, verificata come `NotCovered` (`CoverageProblems`):
	 * una eccezione deve nominare una categoria che esiste, e quella categoria deve essere DAVVERO senza azioni; se ne ha di
	 * nuovo l'eccezione e' stantia e il test cade.
	 */
	const TArray<FAllowedEmpty>& AllowedEmptyCategories()
	{
		static const TArray<FAllowedEmpty> List = {};
		return List;
	}

	/**
	 * Gli errori di copertura di categorie e siti, dati la tabella, i siti dichiarati, quante azioni cadono in ogni categoria
	 * (stesso indice della tabella) e le eccezioni ammesse. Vuoto = tutto coperto. Cade per:
	 *  1. una categoria senza azioni che non e' dichiarata fra le eccezioni — anche quando un'altra categoria tiene coperto lo
	 *     stesso sito: e' lo scenario di una categoria che dipende da UNA azione, quella esce dal catalogo, e il sito resta
	 *     verde per le altre;
	 *  2. una eccezione stantia: la categoria dichiarata senza azioni ne ha di nuovo;
	 *  3. una eccezione che nomina una categoria che non esiste;
	 *  4. un sito dichiarato che nessuna categoria serve;
	 *  5. una categoria su un sito che non e' fra quelli dichiarati.
	 * Funzione PURA: il test la chiama coi dati veri e, come controllo positivo, con dati costruiti. L'elenco vero delle
	 * eccezioni e' vuoto, quindi i versi 2 e 3 non avrebbero mai una voce da giudicare, e un verificatore che non puo' cadere
	 * non verifica niente.
	 */
	TArray<FString> CoverageProblems(const TArray<FCategory>& Categories, const TArray<const TCHAR*>& Sites,
		const TArray<int32>& Hits, const TArray<FAllowedEmpty>& Allowed)
	{
		TArray<FString> Problems;
		if (Hits.Num() != Categories.Num())
		{
			Problems.Add(TEXT("la popolazione non ha una cella per ogni categoria: il verificatore non guarda niente"));
			return Problems;
		}

		const auto IsAllowedEmpty = [&Allowed](const TCHAR* CategoryName)
		{
			return Allowed.ContainsByPredicate([CategoryName](const FAllowedEmpty& Entry)
				{ return FCString::Strcmp(Entry.Category, CategoryName) == 0; });
		};

		for (int32 I = 0; I < Categories.Num(); ++I)
		{
			const FCategory& Category = Categories[I];
			if (Hits[I] == 0 && !IsAllowedEmpty(Category.Name))
			{
				Problems.Add(FString::Printf(
					TEXT("la categoria '%s' (sito %s) non ha nessuna azione nei cataloghi, quindi nessuna riga di "
						 "EveryMainActionIsRefusedWhenStunned la esercita. Se il catalogo non la esercita piu', e' codice che resta in "
						 "piedi senza che niente lo tocchi: toglilo con la sua categoria, oppure dichiara la categoria in "
						 "AllowedEmptyCategories() con la ragione"),
					Category.Name, Category.Site));
			}
			else if (Hits[I] > 0 && IsAllowedEmpty(Category.Name))
			{
				Problems.Add(FString::Printf(
					TEXT("la categoria '%s' (sito %s) e' dichiarata senza azioni in AllowedEmptyCategories(), ma ne ha di nuovo: "
						 "l'eccezione e' stantia, togli la riga"),
					Category.Name, Category.Site));
			}
		}

		for (const FAllowedEmpty& Entry : Allowed)
		{
			const bool bExists = Categories.ContainsByPredicate([&Entry](const FCategory& Category)
				{ return FCString::Strcmp(Category.Name, Entry.Category) == 0; });
			if (!bExists)
			{
				Problems.Add(FString::Printf(
					TEXT("AllowedEmptyCategories() nomina la categoria '%s' (%s), che non esiste fra le categorie: e' un residuo, "
						 "toglila"),
					Entry.Category, Entry.Reason));
			}
		}

		for (const TCHAR* Site : Sites)
		{
			const bool bServed = Categories.ContainsByPredicate([Site](const FCategory& Category)
				{ return FCString::Strcmp(Category.Site, Site) == 0; });
			if (!bServed)
			{
				Problems.Add(FString::Printf(
					TEXT("il sito %s e' dichiarato fra i siti guardati ma nessuna categoria lo serve, quindi nessuna riga puo' "
						 "esercitarlo: aggiungi la categoria, oppure togli il sito"),
					Site));
			}
		}
		for (const FCategory& Category : Categories)
		{
			const bool bDeclared = Sites.ContainsByPredicate([&Category](const TCHAR* Site)
				{ return FCString::Strcmp(Category.Site, Site) == 0; });
			if (!bDeclared)
			{
				Problems.Add(FString::Printf(
					TEXT("la categoria '%s' dichiara il sito %s, che non e' fra i siti guardati: una riga che non chiude nessun "
						 "sito. Aggiungilo a GuardedSites()"),
					Category.Name, Category.Site));
			}
		}
		return Problems;
	}

	// ---------------------------------------------------------------------------------------------
	// I CANDIDATI: le azioni principali dei cataloghi
	// ---------------------------------------------------------------------------------------------

	enum class EProvenance : uint8
	{
		Kit,        //< nel kit di un eroe del roster: l'unita' nasce dall'eroe
		Generic,    //< una generica (D-025): `ConfigureFromHeroData` la accoda a ogni kit
		Core,       //< nel catalogo core ma portata da nessuno: si monta con `AddCoreAbilityInSlot`
		Equipment,  //< concessa da un pezzo di equipaggiamento: si monta con `EquipLoadout`
	};

	const TCHAR* ProvenanceName(EProvenance Provenance)
	{
		switch (Provenance)
		{
		case EProvenance::Kit:       return TEXT("kit d'eroe");
		case EProvenance::Generic:   return TEXT("generica");
		case EProvenance::Core:      return TEXT("core");
		case EProvenance::Equipment: return TEXT("equipaggiamento");
		}
		return TEXT("?");
	}

	struct FCandidate
	{
		EProvenance Provenance = EProvenance::Kit;
		int32 HeroIndex = INDEX_NONE;  //< nel roster: l'eroe che porta l'azione (solo Kit)
		FName HeroId;                  //< solo per la stampa
		FName EquipmentId;             //< solo Equipment
		FRTActionDef Def;

		FString OriginText() const
		{
			switch (Provenance)
			{
			case EProvenance::Kit:       return FString::Printf(TEXT("kit %s"), *HeroId.ToString());
			case EProvenance::Generic:   return TEXT("generica");
			case EProvenance::Core:      return TEXT("core");
			case EProvenance::Equipment: return TEXT("equipaggiamento");
			}
			return TEXT("?");
		}

		FString Label() const
		{
			return FString::Printf(TEXT("%s (%s)"), *Def.ActionId.ToString(), *OriginText());
		}
	};

	/**
	 * Le azioni che occupano la principale, DAI CATALOGHI: kit d'eroe, generiche, core e azioni concesse
	 * dall'equipaggiamento. Nessun `ActionId` e' scritto qui: l'insieme e' la sorgente.
	 */
	TArray<FCandidate> CollectCandidates()
	{
		TArray<FCandidate> Out;

		const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
		for (int32 H = 0; H < Roster.Num(); ++H)
		{
			if (!Roster[H]) { continue; }
			for (const TObjectPtr<URTActionData>& Action : Roster[H]->Actions)
			{
				if (Action && URTCatalogLibrary::TakesMainSlot(Action->Def))
				{
					FCandidate C;
					C.Provenance = EProvenance::Kit;
					C.HeroIndex = H;
					C.HeroId = Roster[H]->HeroId;
					C.Def = Action->Def;
					Out.Add(C);
				}
			}
		}

		for (const URTActionData* Generic : URTCatalogLibrary::MakeGenericActions(GetTransientPackage()))
		{
			if (Generic && URTCatalogLibrary::TakesMainSlot(Generic->Def))
			{
				FCandidate C;
				C.Provenance = EProvenance::Generic;
				C.Def = Generic->Def;
				Out.Add(C);
			}
		}

		// Le core che una generica gia' rappresenta non si ripetono: stessa azione, un'altra strada per arrivare all'unita'.
		const TArray<FName> GenericIds = URTCatalogLibrary::GetGenericActionIds();
		for (const FRTActionDef& Core : URTCatalogLibrary::GetCoreActionCatalog())
		{
			if (URTCatalogLibrary::TakesMainSlot(Core) && !GenericIds.Contains(Core.ActionId))
			{
				FCandidate C;
				C.Provenance = EProvenance::Core;
				C.Def = Core;
				Out.Add(C);
			}
		}

		auto AddEquipment = [&Out](const TArray<URTEquipmentData*>& Items)
		{
			for (const URTEquipmentData* Item : Items)
			{
				const URTActionData* Granted = Item ? URTCatalogLibrary::MakeEquipmentAction(Item, GetTransientPackage()) : nullptr;
				if (Granted && URTCatalogLibrary::TakesMainSlot(Granted->Def))
				{
					FCandidate C;
					C.Provenance = EProvenance::Equipment;
					C.EquipmentId = Item->EquipmentId;
					C.Def = Granted->Def;
					Out.Add(C);
				}
			}
		};
		AddEquipment(URTCatalogLibrary::MakeGadgets());
		AddEquipment(URTCatalogLibrary::MakeReactionModules());

		return Out;
	}

	// ---------------------------------------------------------------------------------------------
	// LA SCENA di una riga: che cosa il piano dichiara e che cosa deve gia' esserci perche' l'azione abbia qualcosa da fare
	// ---------------------------------------------------------------------------------------------

	enum class EPlanForm : uint8
	{
		NoTarget,       //< agisce su chi la usa: nessun bersaglio nel piano (stance, scudi, cura, purificazione)
		OnUnit,         //< bersaglio-unita': il nemico
		OnCell,         //< bersaglio-cella (le predittive)
		OnCellAndEdge,  //< cella e bordo (strutture di bordo e porte)
	};

	/**
	 * La ricetta di pianificazione del Lab (`URTAbilityLabLibrary::BuildFixture`), guidata dal catalogo e non dal nome:
	 * niente bersaglio per chi agisce su se' stesso, una cella e un bordo per chi tocca una struttura, una cella per le
	 * predittive, l'unita' per tutto il resto. Cura e purificazione agiscono su chi le usa: un nemico dichiarato come
	 * bersaglio le renderebbe un'altra azione.
	 */
	EPlanForm PlanFormFor(const FRTActionDef& Def)
	{
		if (Def.bSelfTarget || IsHeal(Def) || IsCleanse(Def)) { return EPlanForm::NoTarget; }
		if (Def.StructureOp != ERTStructureOp::None)          { return EPlanForm::OnCellAndEdge; }
		if (Def.PredictiveTargeting != ERTPredictiveTargeting::None) { return EPlanForm::OnCell; }
		return EPlanForm::OnUnit;
	}

	/** La cella mirata dalle forme a cella, adiacente a chi agisce. */
	FRTCellId AimedCell()
	{
		return FRTCellId(1, 0, 0);
	}

	struct FScene
	{
		EPlanForm Form = EPlanForm::OnUnit;
		ERTHexDirection Edge = ERTHexDirection::W; //< il bordo dichiarato dalle forme con bordo
		bool bWoundTheCaster = false;  //< la cura ha qualcosa da curare
		bool bRootTheCaster = false;   //< la purificazione ha uno stato da togliere
		bool bCoverToMove = false;     //< lo spostamento ha una copertura da spostare (sul bordo opposto a quello dichiarato)
		bool bDoorToToggle = false;    //< l'interazione ha una porta chiusa sul bordo dichiarato
	};

	FScene SceneFor(const FRTActionDef& Def)
	{
		FScene Scene;
		Scene.Form = PlanFormFor(Def);
		Scene.bWoundTheCaster = IsHeal(Def);
		Scene.bRootTheCaster = IsCleanse(Def);
		Scene.bCoverToMove = Def.StructureOp == ERTStructureOp::MoveCover;
		Scene.bDoorToToggle = Def.StructureOp == ERTStructureOp::SetDoorState;
		Scene.Edge = Scene.bCoverToMove ? ERTHexDirection::E : ERTHexDirection::W;
		return Scene;
	}

	// ---------------------------------------------------------------------------------------------
	// I MONDI di una riga
	// ---------------------------------------------------------------------------------------------

	/**
	 * I mondi di una riga, costruiti da zero con la STESSA pre-condizione: cio' che li separa e' solo lo stordimento e il piano.
	 *
	 * - `Control`: NON stordita, pianifica. E' il controllo positivo: se la stessa pianificazione non produce niente da
	 *   una non stordita, la riga non distingue una guardia che funziona da una fixture che non fa niente.
	 * - `Stun`: stordita, pianifica. E' il soggetto della riga.
	 * - `Base`: stordita, NESSUN piano. Cio' che `Stun` deve eguagliare, piu' la sola voce di rifiuto.
	 * - `Base0`: NON stordita, nessun piano. Cio' che `Control` deve differenziare per aver prodotto un effetto.
	 */
	enum class EArm : uint8 { Control, Stun, Base, Base0 };

	const TCHAR* ArmName(EArm Arm)
	{
		switch (Arm)
		{
		case EArm::Control: return TEXT("controllo");
		case EArm::Stun:    return TEXT("stordita");
		case EArm::Base:    return TEXT("base stordita senza piano");
		case EArm::Base0:   return TEXT("base senza piano");
		}
		return TEXT("?");
	}

	struct FArmResult
	{
		bool bValid = false;                 //< il mondo e' stato costruito e le premesse della scena tengono
		int32 SlotIndex = INDEX_NONE;
		int32 ActorStableId = 0;
		int32 Activations = 0;               //< eventi `AbilityActivated` che hanno l'attore per sorgente
		int32 Refusals = 0;                  //< voci `Fallback` / `Cancelled` / `Stunned` dell'attore
		int32 RefusalsNamingTheAction = 0;   //< ... che nominano l'azione installata e la cella di chi agisce
		int32 RefusalsOfOthers = 0;          //< voci di rifiuto per stordimento di QUALUNQUE altra unita'
		ERTMatchPhase RefusalPhase = ERTMatchPhase::Planning;
		int32 CooldownBefore = -1;
		int32 CooldownAfter = -1;
		int32 OtherSlotsBusyBefore = 0;      //< slot DIVERSI da quello sotto prova con una ricarica in corso
		int32 OtherSlotsBusyAfter = 0;
		TArray<FRTTurnLogEntry> Log;
		uint32 StateHash = 0;                //< unita' + mappa, con le ricariche azzerate: il pagamento si legge a parte
	};

	bool IsStunRefusal(const FRTTurnLogEntry& Entry)
	{
		return Entry.Category == ERTLogCategory::Fallback
			&& Entry.Outcome == static_cast<uint8>(ERTFallbackOutcome::Cancelled)
			&& Entry.Amount == static_cast<int32>(ERTActionInvalidReason::Stunned);
	}

	/** La chiave con cui due voci di TurnLog sono "la stessa voce" nel confronto fra mondi. */
	FString LogKey(const FRTTurnLogEntry& Entry)
	{
		return FString::Printf(TEXT("%d|%d|%d|%d|%d,%d,%d|%d,%d,%d|%d|%s|%s"),
			static_cast<int32>(Entry.Phase), static_cast<int32>(Entry.Category), static_cast<int32>(Entry.Outcome),
			Entry.UnitId, Entry.SrcCell.X, Entry.SrcCell.Y, Entry.SrcCell.Layer,
			Entry.TgtCell.X, Entry.TgtCell.Y, Entry.TgtCell.Layer, Entry.Amount,
			*Entry.ActionId.ToString(), *Entry.BaseActionId.ToString());
	}

	/** Le voci di A che non hanno una gemella in B (multiinsieme). */
	TArray<FRTTurnLogEntry> EntriesWithoutTwin(const TArray<FRTTurnLogEntry>& A, const TArray<FRTTurnLogEntry>& B)
	{
		TMap<FString, int32> InB;
		for (const FRTTurnLogEntry& Entry : B) { ++InB.FindOrAdd(LogKey(Entry)); }

		TArray<FRTTurnLogEntry> Left;
		for (const FRTTurnLogEntry& Entry : A)
		{
			int32* Count = InB.Find(LogKey(Entry));
			if (Count && *Count > 0) { --(*Count); }
			else { Left.Add(Entry); }
		}
		return Left;
	}

	/** Quante unita' del roster ha il catalogo: serve a scegliere un portatore per le azioni che nessun eroe porta. */
	int32 CarrierIndexFor(const FCandidate& C, int32 RosterNum)
	{
		return C.HeroIndex != INDEX_NONE ? C.HeroIndex : RosterNum - 1;
	}

	/** Una porta chiusa sul bordo dato. Falso se la cella non c'e': il chiamante lo asserisce, come fa per `AddCover`. */
	bool AddClosedDoor(URTHexMapAsset* Map, const FRTCellId& Cell, ERTHexDirection Edge)
	{
		const FRTHexCellData* Existing = Map ? Map->FindCell(Cell) : nullptr;
		if (!Existing) { return false; }
		FRTHexCellData Data = *Existing;
		Data.Doors.Add(FRTHexDoor(Edge, ERTHexDoorState::Closed));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
		return true;
	}

	/**
	 * UN turno su UNO dei mondi di `EArm`. Le premesse sono asserite qui (la fixture e' quella che la riga dichiara), e
	 * `bValid` e' falso, e il test rosso, se una manca.
	 */
	FArmResult RunArm(FAutomationTestBase& Test, const FCandidate& C, EArm Arm)
	{
		FArmResult Out;
		const bool bStunned = Arm == EArm::Stun || Arm == EArm::Base;
		const bool bPlans = Arm == EArm::Control || Arm == EArm::Stun;
		const FString Run = FString::Printf(TEXT("%s [%s]"), *C.Label(), ArmName(Arm));
		const FScene Scene = SceneFor(C.Def);

		FCensusWorldScope Scope;
		UWorld* World = Scope.World;
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: world di prova"), *Run), World)) { return Out; }

		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: attore mappa"), *Run), MapActor)) { return Out; }
		MapActor->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6);
		URTHexMapAsset* Map = MapActor->MapAsset;
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: mappa"), *Run), Map)) { return Out; }

		// Chi agisce: l'eroe che porta l'azione, o un portatore qualunque per le azioni che nessun eroe porta.
		const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
		const int32 CarrierIndex = CarrierIndexFor(C, Roster.Num());
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: premessa: il portatore e' nel roster"), *Run),
				Roster.IsValidIndex(CarrierIndex) && Roster[CarrierIndex] != nullptr))
		{
			return Out;
		}
		ARTUnit* Caster = SpawnCensusUnit(World, 0, Roster[CarrierIndex], FRTCellId(0, 0, 0));
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: attore"), *Run), Caster)) { return Out; }

		// L'azione sotto prova: gia' nel kit (kit e generiche), montata dal catalogo core, o concessa da un pezzo.
		if (C.Provenance == EProvenance::Core)
		{
			RTAbilityFixtures::AddCoreAbilityInSlot(Caster, *C.Def.ActionId.ToString(), CoreSlot);
		}
		else if (C.Provenance == EProvenance::Equipment)
		{
			Caster->EquipLoadout({ C.EquipmentId });
		}
		int32 Slot = INDEX_NONE;
		for (int32 I = Caster->Abilities.Num() - 1; I >= 0; --I)
		{
			if (Caster->Abilities[I] && Caster->Abilities[I]->Def.ActionId == C.Def.ActionId) { Slot = I; break; }
		}
		if (!Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'azione e' montata sull'attore"), *Run), Slot != INDEX_NONE))
		{
			return Out;
		}
		Out.SlotIndex = Slot;

		URTActionData* Action = Caster->Abilities[Slot];
		bool bPremises = true;
		bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: e' un'azione PRINCIPALE"), *Run),
			URTCatalogLibrary::TakesMainSlot(Action->Def));

		// La ricarica e' resa OSSERVABILE (specchio legacy che `ConsumeAbility` legge, e `Def` che l'HUD legge).
		Action->CooldownTurns = CensusCooldown;
		Action->Def.CooldownTurns = CensusCooldown;
		// Portata dell'arma per le azioni che dichiarano 0 e bersagliano un'unita' (vedi `WeaponRange`).
		if (Scene.Form == EPlanForm::OnUnit && Action->Def.RangeCells <= 0)
		{
			Action->RangeCells = WeaponRange;
		}

		// Il nemico: a portata se l'azione lo bersaglia, altrimenti lontano e inerte. Mai sulla cella mirata.
		const int32 RangeSeen = Action->Def.RangeCells > 0 ? Action->Def.RangeCells : FMath::Max(1, Action->RangeCells);
		const int32 FoeDistance = FMath::Max(FMath::Clamp(RangeSeen, 1, 3), Action->Def.MinRangeCells);
		const FRTCellId FoeCell = Scene.Form == EPlanForm::OnUnit ? FRTCellId(FoeDistance, 0, 0) : FRTCellId(FarFoeX, 0, 0);
		ARTUnit* Foe = SpawnCensusUnit(World, 1, URTHeroCatalogLibrary::MakeBranth(), FoeCell);
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: nemico"), *Run), Foe)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s: turn manager"), *Run), TM))
		{
			return Out;
		}

		// Cio' che l'azione deve trovare perche' una NON stordita abbia qualcosa da fare.
		if (Scene.bWoundTheCaster) { Caster->Health = FMath::Max(1, Caster->Health - 30); }
		if (Scene.bRootTheCaster)
		{
			Caster->ApplyStatus(TAG_Status_Root, 2);
			Caster->PlannedCleansePriority = { TAG_Status_Root };
		}
		if (Scene.bCoverToMove)
		{
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la copertura da spostare e' posta"), *Run),
				URTHexCoverLibrary::AddCover(Map, AimedCell(), ERTHexDirection::W, ERTHexCoverType::Low, 18));
		}
		if (Scene.bDoorToToggle)
		{
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la porta chiusa da aprire e' posta"), *Run),
				AddClosedDoor(Map, AimedCell(), ERTHexDirection::W));
		}

		// Il piano, scritto PRIMA dello stordimento: e' l'unica riga che separa `Stun` da `Control`, e l'ordine e'
		// quello degli altri test per sito.
		if (bPlans)
		{
			Caster->PlannedAbilityIndex = Slot;
			switch (Scene.Form)
			{
			case EPlanForm::NoTarget:
				break;
			case EPlanForm::OnUnit:
				Caster->DeclareAttackOnUnit(Foe);
				break;
			case EPlanForm::OnCell:
				Caster->DeclareAttackOnCell(AimedCell());
				break;
			case EPlanForm::OnCellAndEdge:
				Caster->DeclareAttackOnCell(AimedCell());
				Caster->PlannedCoverEdge = Scene.Edge;
				Caster->bHasPlannedCoverEdge = true;
				break;
			}
		}
		if (bStunned) { Caster->ApplyStatus(TAG_Status_Stunned, URTCombatLibrary::StunnedDurationTurns); }

		// Le premesse: la fixture e' quella che la riga dichiara.
		Out.CooldownBefore = Caster->GetAbilityCooldown(Slot);
		for (int32 I = 0; I < Caster->Abilities.Num(); ++I)
		{
			if (I != Slot && Caster->GetAbilityCooldown(I) != 0) { ++Out.OtherSlotsBusyBefore; }
		}
		bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: lo stordimento c'e' solo dove il braccio lo dichiara"), *Run),
			Caster->HasStatus(TAG_Status_Stunned), bStunned);
		bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: il piano e' nello slot dell'azione solo dove il braccio lo dichiara"), *Run),
			Caster->PlannedAbilityIndex == Slot, bPlans);
		bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: l'azione e' utilizzabile (nessuna ricarica residua)"), *Run),
			Out.CooldownBefore, 0);
		bPremises &= Test.TestEqual(*FString::Printf(TEXT("%s: premessa: nessun altro slot e' in ricarica prima del turno"), *Run),
			Out.OtherSlotsBusyBefore, 0);
		bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: la ricarica dichiarata e lo specchio coincidono e sono positivi"), *Run),
			Action->Def.CooldownTurns > 0 && Action->Def.CooldownTurns == Action->CooldownTurns);
		bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'attore e il nemico sono vivi e in squadre diverse"), *Run),
			Caster->IsAlive() && Foe->IsAlive() && Caster->TeamId != Foe->TeamId);
		// Nessuna rotazione di mira nel mondo: l'attore guarda gia' verso la cella mirata, quindi una differenza di stato
		// non e' mai la sola riorientazione che il Blast fa prima di risolvere (D-020).
		if (Scene.Form != EPlanForm::NoTarget)
		{
			ERTHexDirection Toward = Caster->Facing;
			const FRTCellId Aim = Scene.Form == EPlanForm::OnUnit ? FoeCell : AimedCell();
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'attore guarda gia' verso la cella mirata"), *Run),
				URTHexLibrary::DirectionTowards(Caster->Cell, Aim, Toward) && Toward == Caster->Facing);
		}
		if (Scene.bWoundTheCaster)
		{
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: chi cura e' ferito"), *Run), Caster->Health < Caster->MaxHealth);
		}
		if (Scene.bRootTheCaster)
		{
			bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: lo stato da togliere c'e'"), *Run), Caster->HasStatus(TAG_Status_Root));
		}
		if (!bPremises) { return Out; }

		const TWeakObjectPtr<ARTUnit> CasterWeak(Caster);
		const TWeakObjectPtr<ARTUnit> FoeWeak(Foe);
		RunCensusTurn(TM);

		// L'attore caduto resterebbe leggibile finche' non gira il garbage collector; qui nessuno cade, ma si rilegge
		// da un riferimento debole come fanno gli altri test per sito.
		ARTUnit* After = CasterWeak.Get(/*bEvenIfPendingKill=*/ true);
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: l'attore e' ancora leggibile"), *Run), After)) { return Out; }
		Out.ActorStableId = After->StableUnitId;
		Out.CooldownAfter = After->GetAbilityCooldown(Slot);
		for (int32 I = 0; I < After->Abilities.Num(); ++I)
		{
			if (I != Slot && After->GetAbilityCooldown(I) != 0) { ++Out.OtherSlotsBusyAfter; }
		}

		Out.Log = TM->GetTurnLog();
		for (const FRTTurnLogEntry& Entry : Out.Log)
		{
			if (!IsStunRefusal(Entry)) { continue; }
			if (Entry.UnitId == Out.ActorStableId)
			{
				++Out.Refusals;
				Out.RefusalPhase = Entry.Phase;
				if (Entry.ActionId == C.Def.ActionId && Entry.SrcCell == FRTCellId(0, 0, 0)) { ++Out.RefusalsNamingTheAction; }
			}
			else
			{
				++Out.RefusalsOfOthers;
			}
		}
		for (const FRTResolvedEvent& Event : TM->ResolvedTimelineForTest())
		{
			if (Event.Type == ERTResolvedEventType::AbilityActivated && Event.SourceStableUnitId == Out.ActorStableId)
			{
				++Out.Activations;
			}
		}

		// L'hash di stato (unita' + mappa) con le ricariche azzerate: e' l'oracolo "nessun effetto" senza un osservatore
		// per categoria. Si ricalcola con `HashMatchState`, come fa il replay: `GetPendingFinalStateHash()` vale 0 senza
		// registrazione e un confronto fra due run diventerebbe `0 == 0`.
		TArray<ARTUnit*> UnitsNow;
		UnitsNow.Add(After);
		if (ARTUnit* FoeAfter = FoeWeak.Get(/*bEvenIfPendingKill=*/ true)) { UnitsNow.Add(FoeAfter); }
		TArray<FRTUnitStateDigest> Digests = URTMatchStateHashLibrary::BuildUnitDigests(UnitsNow);
		for (FRTUnitStateDigest& Digest : Digests) { Digest.AbilityCooldowns.Reset(); }
		const TArray<int32> Scores = { 0, 0 };
		Out.StateHash = URTMatchStateHashLibrary::HashMatchState(Map, Digests, Scores);

		bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'attore ha uno StableUnitId"), *Run), Out.ActorStableId != 0);
		bPremises &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: l'hash di stato non e' nullo (un confronto fra due hash nulli e' vacuo)"), *Run),
			Out.StateHash != 0);
		Out.bValid = bPremises;
		return Out;
	}

	// ---------------------------------------------------------------------------------------------
	// IL VERDETTO di una riga
	// ---------------------------------------------------------------------------------------------

	/**
	 * Con quali canali il controllo positivo mostra che la pianificazione, da una NON stordita, produce qualcosa.
	 *
	 * L'attivazione e lo stato (l'hash di unita' e mappa) PROVANO l'effetto, e fanno il controllo positivo della riga.
	 * Il TurnLog non lo prova: un colpo mancato o un'azione rifiutata dalla mappa lasciano comunque voci non `Fallback`
	 * (`Combat`, `Environment/CoverRejected`), quindi serve solo a dire se la clausola "nessun effetto" ha un osservatore
	 * in piu' (una riga con la sola attivazione e' la prova piu' debole).
	 */
	struct FControlChannels
	{
		bool bActivation = false;  //< l'attore emette `AbilityActivated`
		bool bState = false;       //< unita' e mappa, a fine turno, differiscono da quelle senza piano
		bool bLog = false;         //< il TurnLog ha voci non `Fallback` che la base senza piano non ha

		bool ProvesTheEffect() const { return bActivation || bState; }
		bool IsWeak() const { return bActivation && !bState && !bLog; }

		FString Text() const
		{
			TArray<FString> Parts;
			if (bActivation) { Parts.Add(TEXT("attivazione")); }
			if (bState)      { Parts.Add(TEXT("stato")); }
			if (bLog)        { Parts.Add(TEXT("log")); }
			return Parts.Num() > 0 ? FString::Join(Parts, TEXT("+")) : FString(TEXT("NESSUNO"));
		}
	};

	FControlChannels ControlChannelsOf(const FArmResult& Control, const FArmResult& Base0)
	{
		FControlChannels Channels;
		Channels.bActivation = Control.Activations > 0;
		Channels.bState = Control.StateHash != Base0.StateHash;
		for (const FRTTurnLogEntry& Entry : EntriesWithoutTwin(Control.Log, Base0.Log))
		{
			if (Entry.Category != ERTLogCategory::Fallback) { Channels.bLog = true; break; }
		}
		return Channels;
	}

	bool JudgeRow(FAutomationTestBase& Test, const FCandidate& C, const FCategory& Category,
		const FArmResult& Control, const FArmResult& Stun, const FArmResult& Base, const FArmResult& Base0,
		const FControlChannels& Channels)
	{
		const FString Row = C.Label();
		bool bOk = true;

		// --- Il controllo positivo --------------------------------------------------------------------------------------
		bOk &= Test.TestTrue(*FString::Printf(
			TEXT("%s: controllo positivo: la stessa pianificazione da una NON stordita produce l'effetto (attivazione dell'attore o stato diverso dalla base). "
				 "Senza, la riga e' vacua: non distingue una guardia che funziona da una fixture che non fa niente"), *Row),
			Channels.ProvesTheEffect());
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: controllo: una non stordita non lascia nessuna voce di rifiuto per stordimento"), *Row),
			Control.Refusals == 0 && Control.RefusalsOfOthers == 0);

		// --- Il divieto ---------------------------------------------------------------------------------------------------
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: la stordita lascia UNA sola voce di rifiuto 'Fallback/Cancelled/Stunned' (ne ha %d)"), *Row, Stun.Refusals),
			Stun.Refusals == 1);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: la voce di rifiuto nomina l'azione e la cella di chi agisce"), *Row),
			Stun.RefusalsNamingTheAction == 1);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: nessun'altra unita' lascia un rifiuto per stordimento"), *Row),
			Stun.RefusalsOfOthers == 0);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: il rifiuto compare nella fase che la categoria '%s' dichiara (%s, sito %s): e' %s"),
				*Row, Category.Name, *EnumNameOf(Category.RefusalPhase), Category.Site, *EnumNameOf(Stun.RefusalPhase)),
			Stun.Refusals == 1 && Stun.RefusalPhase == Category.RefusalPhase);
		bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: nessun AbilityActivated dell'attore (un rifiuto non si attiva)"), *Row),
			Stun.Activations, 0);
		// Il residuo vale `CensusCooldown - 1` se la ricarica e' partita (uno o piu' addebiti non si distinguono:
		// `ConsumeAbility` scrive un valore fisso) e `0` se non e' partita; la ricarica prima del turno e' zero, asserita in `RunArm`.
		bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: la ricarica parte (residuo a fine turno; un secondo addebito non si distingue dal primo)"), *Row),
			Stun.CooldownAfter, CensusCooldown - 1);
		bOk &= Test.TestEqual(*FString::Printf(TEXT("%s: nessun altro slot e' toccato dal pagamento"), *Row),
			Stun.OtherSlotsBusyAfter, 0);

		// --- Nessun effetto, senza un osservatore per categoria ---------------------------------------------------------------
		const TArray<FRTTurnLogEntry> Added = EntriesWithoutTwin(Stun.Log, Base.Log);
		const TArray<FRTTurnLogEntry> Lost = EntriesWithoutTwin(Base.Log, Stun.Log);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: nel TurnLog la stordita aggiunge UNA sola voce rispetto alla stessa base senza piano (prima in piu': %s)"),
				*Row, Added.Num() > 0 ? *LogKey(Added[0]) : TEXT("-")),
			Added.Num() == 1);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: ... e quella voce e' il rifiuto dell'attore"), *Row),
			Added.Num() == 1 && IsStunRefusal(Added[0]) && Added[0].UnitId == Stun.ActorStableId);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: ... e non perde nessuna voce della base (prima persa: %s)"),
				*Row, Lost.Num() > 0 ? *LogKey(Lost[0]) : TEXT("-")),
			Lost.Num() == 0);
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: lo stato (unita' e mappa, ricariche escluse) e' quello della base senza piano"), *Row),
			Stun.StateHash == Base.StateHash);

		// La base stessa deve discriminare lo stordimento: un hash che non vede la differenza fra le due basi
		// renderebbe il confronto qui sopra cieco al solo stato che le separa.
		bOk &= Test.TestTrue(*FString::Printf(TEXT("%s: premessa: lo stato vede lo stordimento (le due basi differiscono)"), *Row),
			Base.StateHash != Base0.StateHash);
		return bOk;
	}
}

// =====================================================================================================
// 1. STATICO: ogni azione principale dei cataloghi ha un sito che la consuma
// =====================================================================================================

/**
 * **Ogni azione che occupa la principale ha una macro-fase che un sito noto consuma, e ogni valore con cui il resolver
 * instrada e' dichiarato.**
 *
 * L'insieme in esame viene dai cataloghi, mai da un elenco di nomi: kit d'eroe (`GetHeroRoster`), generiche
 * (`MakeGenericActions`), core (`GetCoreActionCatalog`) e azioni concesse dall'equipaggiamento (`MakeGadgets`,
 * `MakeReactionModules`), filtrate con `TakesMainSlot`. Non si asserisce quante sono: l'insieme e' la sorgente.
 *
 * Cade, con un messaggio che dice che cosa decidere, quando:
 * 1. una principale passa dal percorso Dash: `Slot == MovementAndMain` o macro-fase Dash. `ResolveDash` ha la guardia
 *    (D-567, #3964, che scioglie D-565 (4)(b)): `ARTUnit::PlannedDashDeniedByStun` nega la mobilita' che prende la
 *    principale e il rifiuto paga. La prova e' `Status.StunDeniesAMobilityThatAlsoCostsTheMainAction`, con un'azione
 *    COSTRUITA; qui manca la categoria con la sua riga comportamentale per un'azione di CATALOGO, e il divieto resta;
 * 2. una principale ha macro-fase Move o Planning: nessuna categoria la dichiara. Non e' un percorso senza guardia
 *    (`CollectAttackIntents` salta solo il Cleanup, la prenderebbe come intento e la rifiuterebbe nel Blast) ma
 *    un'anomalia di dato: si corregge la fase, o si decide a quale sito appartiene e si dichiara;
 * 3. un valore di `ERTStructureOp`, `ERTResolutionPhase`, `ERTActionSlot` o `ERTPredictiveTargeting` non e' dichiarato
 *    in questo file, anche senza azioni che lo usino: aggiungere un valore a `ERTStructureOp` non fa fallire la
 *    compilazione, e `ResolveCoverStructures` prende ogni valore diverso da `None` e da `SetDoorState`;
 * 4. un'azione non cade in nessuna categoria. OGGI E' IRRAGGIUNGIBILE: dopo lo scarto di 1 e 2 la macro-fase e' Prep,
 *    Blast o Cleanup (`MapResolutionPhase` e' totale e non restituisce altro), e `IsPrepOnSelf`, `IsBlastIntent` e
 *    `IsEnvironmental` sono catch-all su quelle macro-fasi. Il ramo resta come difesa per il giorno in cui qualcuno STRINGE uno di
 *    quei predicati in questo file.
 *
 * Cio' che NON puo' rilevare: un sito NUOVO del resolver che consuma una principale (una funzione che non chiama
 * `RefuseMainActionIfStunned`, in una categoria esistente o in una nuova), come furono la cura e la purificazione (#3954)
 * e le strutture di bordo (#3958). Non cambia nessuna chiave che questo test legge: lo vede solo una riga di
 * `EveryMainActionIsRefusedWhenStunned`, e solo se la fixture passa da quel sito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunCensusKnownSiteTest,
	"RefactorTactics.Status.StunCensus.EveryMainActionHasAKnownSite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunCensusKnownSiteTest::RunTest(const FString&)
{
	namespace Census = RTStunCensusTestsInternal;

	const TArray<Census::FCandidate> Candidates = Census::CollectCandidates();
	// Anti-vacuita': senza azioni principali il ciclo qui sotto sarebbe verde e non direbbe niente.
	if (!TestTrue(TEXT("i cataloghi offrono azioni principali da censire"), Candidates.Num() > 0)) { return false; }

	// Anti-vacuita' dell'ENUMERAZIONE: un filtro che riconosce zero elementi assolve. Ogni strada che porta un'azione
	// principale a un'unita' deve contribuirne almeno una, o quella strada e' cieca (o il catalogo l'ha svuotata) e il
	// verde non dice niente su di lei. Non e' un conteggio: si chiede solo che l'insieme non sia vuoto.
	const Census::EProvenance Provenances[] = {
		Census::EProvenance::Kit, Census::EProvenance::Generic, Census::EProvenance::Core, Census::EProvenance::Equipment };
	for (const Census::EProvenance Provenance : Provenances)
	{
		TestTrue(*FString::Printf(TEXT("almeno un'azione principale arriva dalla strada '%s'"), Census::ProvenanceName(Provenance)),
			Candidates.ContainsByPredicate([Provenance](const Census::FCandidate& C) { return C.Provenance == Provenance; }));
	}

	bool bAllKnown = true;
	for (const Census::FCandidate& C : Candidates)
	{
		const FRTActionDef& Def = C.Def;
		const ERTMatchPhase Macro = Census::MacroPhaseOf(Def);

		// Il percorso Dash: una principale qui e' una mobilita' che prende anche la principale. La regola c'e' (D-567) e la
		// guardia sta in `ResolveDash`, ma il censimento non ha una categoria con la sua riga comportamentale per un'azione di
		// CATALOGO di questo tipo, quindi la vieta finche' qualcuno non la dichiara.
		if (Def.Slot == ERTActionSlot::MovementAndMain || Macro == ERTMatchPhase::Dash)
		{
			bAllKnown = false;
			AddError(FString::Printf(
				TEXT("%s: occupa la principale (Slot %s) ma la sua fase %s -> %s la manda a ResolveDash. La regola c'e' (D-567, #3964): "
					 "una stordita non la compie e il rifiuto paga la ricarica, con la guardia in ARTUnit::PlannedDashDeniedByStun letta da "
					 "ResolveDash e provata da Status.StunDeniesAMobilityThatAlsoCostsTheMainAction con un'azione costruita nel test. Quello "
					 "che manca e' la RIGA di questo censimento per un'azione di catalogo: dichiara qui la categoria (predicato, sito "
					 "ResolveDash, fase Dash) con la sua riga comportamentale, e solo allora togli questo divieto per lei."),
				*C.Label(), *Census::EnumNameOf(Def.Slot), *Census::EnumNameOf(Def.ResolutionPhase), *Census::EnumNameOf(Macro)));
			continue;
		}

		// Move e Planning: NON sono percorsi senza guardia — `CollectAttackIntents` salta solo il Cleanup, quindi una principale
		// con queste fasi arriverebbe al Blast come intento e li' sarebbe rifiutata — ma nessuna categoria le dichiara: e' un'anomalia
		// di dato (una principale con la fase di una mobilita' o del congelamento), e va decisa, non assorbita.
		if (Macro == ERTMatchPhase::Move || Macro == ERTMatchPhase::Planning)
		{
			bAllKnown = false;
			AddError(FString::Printf(
				TEXT("%s: occupa la principale (Slot %s) ma la sua fase %s -> %s non e' di nessuna categoria di questo censimento. "
					 "Non e' un percorso senza guardia (CollectAttackIntents salta solo il Cleanup: la prenderebbe come intento e la "
					 "rifiuterebbe nel Blast) ma un'anomalia di dato, una principale con la fase di una mobilita' o del congelamento: "
					 "correggi la fase nel catalogo, oppure decidi a quale sito appartiene e dichiaralo qui (predicato e categoria)."),
				*C.Label(), *Census::EnumNameOf(Def.Slot), *Census::EnumNameOf(Def.ResolutionPhase), *Census::EnumNameOf(Macro)));
			continue;
		}

		// DIFESA, OGGI IRRAGGIUNGIBILE: dopo lo scarto qui sopra la macro-fase e' Prep, Blast o Cleanup (`MapResolutionPhase` e'
		// totale e non restituisce altro) e `IsPrepOnSelf`, `IsBlastIntent` e `IsEnvironmental` sono catch-all su quelle macro-fasi.
		// Scatterebbe se qualcuno STRINGESSE uno di quei predicati in questo file; NON rileva un sito nuovo del resolver.
		if (Census::Classify(Def) == INDEX_NONE)
		{
			bAllKnown = false;
			AddError(FString::Printf(
				TEXT("%s: azione principale con Slot %s, fase %s -> %s, StructureOp %s, PredictiveTargeting %s: nessuna categoria la "
					 "prende, e i predicati catch-all di Prep, Blast e Cleanup dovrebbero prenderle tutte: uno e' stato ristretto. Se la "
					 "consuma un sito NUOVO, quel sito DEVE chiamare ARTTurnManager::RefuseMainActionIfStunned (RTTurnManager.h, "
					 "docstring della funzione) e va dichiarato come categoria in RTStunCensusTests.cpp con la sua riga comportamentale; "
					 "se e' un sito gia' coperto, estendi il predicato della categoria."),
				*C.Label(), *Census::EnumNameOf(Def.Slot), *Census::EnumNameOf(Def.ResolutionPhase), *Census::EnumNameOf(Macro),
				*Census::EnumNameOf(Def.StructureOp), *Census::EnumNameOf(Def.PredictiveTargeting)));
		}
	}
	TestTrue(TEXT("ogni azione principale dei cataloghi cade in una categoria con un sito guardato"), bAllKnown);

	// I valori con cui il resolver instrada, anche senza azioni che li usino. Il quarto argomento dice che cosa il resolver fa
	// di un valore nuovo di quell'enum (letto dal codice), perche' la frase non e' la stessa per tutti.
	TestTrue(TEXT("ERTStructureOp: ogni valore e' dichiarato"),
		Census::CheckEnumIsDeclared<ERTStructureOp>(*this, TEXT("ERTStructureOp"), Census::StructureOpRoles(),
			TEXT("Un valore nuovo passa da ResolveCoverStructures, che prende ogni StructureOp diverso da None e da SetDoorState "
				 "e ERIGE UNA COPERTURA; e aggiungerlo NON fa fallire la compilazione (RTActionDef.h).")));
	TestTrue(TEXT("ERTResolutionPhase: ogni valore e' dichiarato"),
		Census::CheckEnumIsDeclared<ERTResolutionPhase>(*this, TEXT("ERTResolutionPhase"), Census::ResolutionPhaseRoles(),
			TEXT("MapResolutionPhase lo porta in una macro-fase, e la macro-fase decide il sito: questo file classifica Prep, Blast "
				 "e Cleanup, e scarta Dash, Move e Planning.")));
	TestTrue(TEXT("ERTActionSlot: ogni valore e' dichiarato"),
		Census::CheckEnumIsDeclared<ERTActionSlot>(*this, TEXT("ERTActionSlot"), Census::ActionSlotRoles(),
			TEXT("TakesMainSlot conta fra le principali solo Main e MovementAndMain; ResolveDash nega la mobilita' che la prende "
					 "(ARTUnit::PlannedDashDeniedByStun, D-567) e legge Slot == MovementAndMain per spendere la principale di chi scatta.")));
	TestTrue(TEXT("ERTPredictiveTargeting: ogni valore e' dichiarato"),
		Census::CheckEnumIsDeclared<ERTPredictiveTargeting>(*this, TEXT("ERTPredictiveTargeting"), Census::PredictiveTargetingRoles(),
			TEXT("ResolvePrep tratta come predittivo ogni valore diverso da None.")));
	return true;
}

// =====================================================================================================
// 2. COMPORTAMENTALE: una stordita che pianifica l'azione non produce effetto, e la ricarica parte
// =====================================================================================================

/**
 * **Per ogni azione principale dei cataloghi, una stordita che la pianifica non produce nessun effetto, lascia UNA
 * voce di rifiuto nella fase del suo sito, non si attiva, e la sua ricarica parte.**
 *
 * Le azioni sono quelle di `CollectCandidates()` (dal catalogo). Ogni riga costruisce da zero i mondi di `EArm` con la
 * stessa pre-condizione, e il verdetto e' in `JudgeRow`:
 *
 * - **premesse** asserite in `RunArm`: l'azione e' montata e Main, la ricarica forzata e' osservabile, nessun altro
 *   slot e' in ricarica, lo stordimento e il piano ci sono solo dove il braccio li dichiara, chi cura e' ferito, chi
 *   purifica e' bloccato, la copertura da spostare e la porta da aprire ci sono;
 * - **controllo positivo**: la stessa pianificazione da una non stordita produce un'attivazione dell'attore o uno stato
 *   diverso da quello senza piano. Senza, la riga e' vacua e cade con il suo nome;
 * - **divieto**: una sola voce `Fallback`/`Cancelled`/`Stunned` che nomina l'azione e la cella di chi agisce, nella
 *   fase che la categoria dichiara; nessun `AbilityActivated`; la ricarica parte (residuo `CensusCooldown - 1`, da zero:
 *   `ConsumeAbility` scrive un valore fisso, quindi uno o piu' addebiti non si distinguono); il TurnLog della stordita
 *   e' quello della base senza piano piu' quella sola voce; lo stato e' quello della base.
 *
 * Attenzione: il controllo positivo e' l'UNIONE di attivazione e stato (l'hash di unita' e mappa). Il TurnLog si riporta
 * (`FControlChannels::bLog`) ma NON lo prova: un colpo mancato lascia voci `Combat`, una struttura rifiutata dalla mappa
 * una voce `Environment`. Per le azioni il cui unico canale e' l'attivazione (uno scudo temporaneo, l'Overwatch armato,
 * `Action.Interrupt` senza vittima) la clausola "nessun effetto" e' vuota: restano rifiuto, attivazione, fase e
 * ricarica. La run le nomina con `STUNCENSUS-DEBOLE`.
 *
 * Attenzione: `AbilityActivated` non e' emesso da `ResolveEnvironment` (Cleanup): per quel sito "nessuna attivazione" e'
 * vuoto anche nel controllo, e l'oracolo regge sul rifiuto, sulla fase, sulla ricarica e sull'hash di stato. La run lo
 * dichiara, riga per riga, con `STUNCENSUS-SENZA-ATTIVAZIONE`: e' la riga il cui controllo (la non stordita) non emette
 * nessun `AbilityActivated`, qualunque ne sia la ragione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunCensusRefusedWhenStunnedTest,
	"RefactorTactics.Status.StunCensus.EveryMainActionIsRefusedWhenStunned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunCensusRefusedWhenStunnedTest::RunTest(const FString&)
{
	namespace Census = RTStunCensusTestsInternal;

	// Il debito dichiarato, per nome: cio' che la matrice NON copre e perche'.
	for (const Census::FNotCovered& Item : Census::NotCovered())
	{
		AddInfo(FString::Printf(TEXT("STUNCENSUS-NONCOPERTO|%s|%s|%s"), Item.Name, Item.Reason, *Census::CoveredByText(Item)));
	}

	const TArray<Census::FCandidate> Candidates = Census::CollectCandidates();
	if (!TestTrue(TEXT("i cataloghi offrono azioni principali da censire"), Candidates.Num() > 0)) { return false; }

	for (const Census::FCandidate& C : Candidates)
	{
		const int32 CategoryIndex = Census::Classify(C.Def);
		if (CategoryIndex == INDEX_NONE)
		{
			// Lo dice gia' EveryMainActionHasAKnownSite, col messaggio che spiega dove aggiungerla: qui la riga non ha una
			// fase attesa, quindi non si puo' giudicare.
			AddError(FString::Printf(TEXT("%s: nessun sito noto (vedi EveryMainActionHasAKnownSite): la riga non si puo' giudicare"), *C.Label()));
			continue;
		}
		const Census::FCategory& Category = Census::AllCategories()[CategoryIndex];

		const Census::FArmResult Control = Census::RunArm(*this, C, Census::EArm::Control);
		const Census::FArmResult Stun = Census::RunArm(*this, C, Census::EArm::Stun);
		const Census::FArmResult Base = Census::RunArm(*this, C, Census::EArm::Base);
		const Census::FArmResult Base0 = Census::RunArm(*this, C, Census::EArm::Base0);
		if (!Control.bValid || !Stun.bValid || !Base.bValid || !Base0.bValid)
		{
			AddInfo(FString::Printf(TEXT("STUNCENSUS|%s|%s|%s|%s|%s|PREMESSA-MANCATA"), *C.OriginText(), *C.Def.ActionId.ToString(),
				Category.Name, Category.Site, *Census::EnumNameOf(Category.RefusalPhase)));
			continue;
		}

		const Census::FControlChannels Channels = Census::ControlChannelsOf(Control, Base0);
		const bool bOk = Census::JudgeRow(*this, C, Category, Control, Stun, Base, Base0, Channels);
		AddInfo(FString::Printf(TEXT("STUNCENSUS|%s|%s|%s|%s|%s|%s|%s"), *C.OriginText(), *C.Def.ActionId.ToString(),
			Category.Name, Category.Site, *Census::EnumNameOf(Category.RefusalPhase), bOk ? TEXT("ok") : TEXT("ROSSO"), *Channels.Text()));
		if (Channels.IsWeak())
		{
			AddInfo(FString::Printf(TEXT("STUNCENSUS-DEBOLE|%s|controllo solo per attivazione: la clausola 'nessun effetto' e' vuota per questa riga"),
				*C.Def.ActionId.ToString()));
		}
		// L'altra clausola vuota: se la NON stordita non emette `AbilityActivated`, "nessun AbilityActivated" della stordita e'
		// vero per costruzione e non prova niente. E' il caso di `ResolveEnvironment`, che non lo emette; la riga regge su
		// rifiuto, fase, ricarica e stato.
		if (!Channels.bActivation)
		{
			AddInfo(FString::Printf(TEXT("STUNCENSUS-SENZA-ATTIVAZIONE|%s|%s|la non stordita non emette AbilityActivated (e' il caso di "
					"ResolveEnvironment, che non lo emette): la clausola 'nessun AbilityActivated' e' vuota per questa riga"),
				*C.Def.ActionId.ToString(), Category.Site));
		}
	}
	return true;
}

// =====================================================================================================
// 3. STATICO: ogni categoria ha una riga (o un'eccezione dichiarata), e le categorie non coperte hanno ancora chi le copre
// =====================================================================================================

/**
 * **Ogni categoria ha almeno un'azione nel catalogo, o e' dichiarata senza azioni per nome e ragione; quindi ogni sito ha
 * almeno una riga di `EveryMainActionIsRefusedWhenStunned`. E i test che coprono altrove le categorie dichiarate "non
 * coperte" esistono ancora.**
 *
 * Si verifica per CATEGORIA e non per sito, perche' piu' categorie condividono lo stesso sito ('armare predittiva' e
 * 'armare Overwatch' stanno su `ResolvePrep`; 'topologia' e 'porta' su `CollectAttackIntents`). Una categoria che dipende da
 * una sola azione si svuota se quell'azione esce dal catalogo, e un controllo per sito resterebbe verde perche' un'altra
 * categoria tiene coperto lo stesso sito: e' il caso in cui una guardia spostata sotto quel ramo (il difetto di `ModifyArc`,
 * descritto nel resolver) non farebbe cadere niente.
 *
 * Una categoria senza nessuna azione e' codice che il catalogo non esercita piu' (per esempio `ResolveCleanseActions` se
 * D-264 togliesse `Action.Cleanse`): o si toglie il codice con la sua categoria e il suo sito, o si dichiara la categoria in
 * `AllowedEmptyCategories` con la ragione, come `Dichiarate` in `Catalog.EveryCoreActionIsReachableOrDeclared`. Una
 * eccezione non dichiarata e' codice che resta in piedi senza che nessuna riga lo tocchi; una dichiarata e' verificata:
 * nomina una categoria che esiste ed e' DAVVERO senza azioni (se ne ha di nuovo e' stantia, e il test cade). Il verificatore
 * (`CoverageProblems`) e' provato anche su dati costruiti, perche' l'elenco vero e' vuoto e i versi sulle eccezioni non
 * avrebbero altrimenti mai una voce da giudicare.
 *
 * I nomi dei test citati in `NotCovered` si leggono dal registro dei test (come `Naming.NoTestNameIsAPrefixOfAnother`):
 * un riferimento a un test che non esiste piu' e' una copertura dichiarata e non piu' vera.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTStunCensusEverySiteHasARowTest,
	"RefactorTactics.Status.StunCensus.EveryGuardedSiteHasARow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTStunCensusEverySiteHasARowTest::RunTest(const FString&)
{
	namespace Census = RTStunCensusTestsInternal;

	const TArray<Census::FCandidate> Candidates = Census::CollectCandidates();
	if (!TestTrue(TEXT("i cataloghi offrono azioni principali da censire"), Candidates.Num() > 0)) { return false; }

	// La popolazione per CATEGORIA, non per sito: quante azioni dei cataloghi cadono in ciascuna voce della tabella (stesso
	// indice). Le azioni senza categoria le dice `EveryMainActionHasAKnownSite`.
	TArray<int32> Hits;
	Hits.Init(0, Census::AllCategories().Num());
	for (const Census::FCandidate& C : Candidates)
	{
		const int32 CategoryIndex = Census::Classify(C.Def);
		if (CategoryIndex != INDEX_NONE) { ++Hits[CategoryIndex]; }
	}

	// Ogni categoria ha azioni (o e' un'eccezione dichiarata e davvero vuota), ogni eccezione nomina una categoria che esiste,
	// ogni sito e' servito da una categoria e ogni categoria sta su un sito dichiarato.
	for (const FString& Problem : Census::CoverageProblems(
			Census::AllCategories(), Census::GuardedSites(), Hits, Census::AllowedEmptyCategories()))
	{
		AddError(Problem);
	}

	// Il verificatore stesso, su dati costruiti. L'elenco vero delle eccezioni e' vuoto: i versi che lo riguardano (eccezione
	// stantia, eccezione che nomina una categoria inesistente, sito escusato) non avrebbero mai una voce da giudicare, e un
	// verificatore che non puo' cadere non verifica niente. Le categorie e i siti qui sono finti, e non toccano quelli veri.
	{
		const TArray<Census::FCategory> FakeCategories = {
			{ TEXT("A"), TEXT("SitoUno"), ERTMatchPhase::Prep,  nullptr },
			{ TEXT("B"), TEXT("SitoUno"), ERTMatchPhase::Prep,  nullptr },
			{ TEXT("C"), TEXT("SitoDue"), ERTMatchPhase::Blast, nullptr },
		};
		const TArray<const TCHAR*> BothSites = { TEXT("SitoUno"), TEXT("SitoDue") };
		const TArray<const TCHAR*> OnlyTheFirstSite = { TEXT("SitoUno") };
		const TArray<const TCHAR*> ASiteNobodyServes = { TEXT("SitoUno"), TEXT("SitoDue"), TEXT("SitoTre") };

		const TArray<int32> AllPopulated = { 1, 1, 1 };
		const TArray<int32> BEmpty = { 1, 0, 1 };           // B vuota, ma A tiene coperto il suo sito
		const TArray<int32> TheFirstSiteEmpty = { 0, 0, 1 };  // A e B vuote: SitoUno resta senza righe
		const TArray<int32> OneCellShort = { 1, 1 };

		const TArray<Census::FAllowedEmpty> NoExceptions;
		const TArray<Census::FAllowedEmpty> ExceptionForB = { { TEXT("B"), TEXT("prova") } };
		const TArray<Census::FAllowedEmpty> ExceptionsForAAndB = { { TEXT("A"), TEXT("prova") }, { TEXT("B"), TEXT("prova") } };
		const TArray<Census::FAllowedEmpty> ExceptionForAGhost = { { TEXT("Z"), TEXT("prova") } };

		const auto Expect = [this, &FakeCategories](const TCHAR* Label, const TArray<const TCHAR*>& Sites,
			const TArray<int32>& FakeHits, const TArray<Census::FAllowedEmpty>& Allowed, int32 ExpectedProblems)
		{
			TestEqual(*FString::Printf(TEXT("verificatore su dati costruiti: %s"), Label),
				Census::CoverageProblems(FakeCategories, Sites, FakeHits, Allowed).Num(), ExpectedProblems);
		};
		Expect(TEXT("tutto popolato, nessun problema (il verificatore non grida al lupo)"), BothSites, AllPopulated, NoExceptions, 0);
		Expect(TEXT("una categoria vuota su un sito che l'altra tiene coperto: cade LEI, e non e' il sito a dirlo"),
			BothSites, BEmpty, NoExceptions, 1);
		Expect(TEXT("la stessa, dichiarata senza azioni: nessun problema"), BothSites, BEmpty, ExceptionForB, 0);
		Expect(TEXT("dichiarata senza azioni ma popolata: l'eccezione e' stantia"), BothSites, AllPopulated, ExceptionForB, 1);
		Expect(TEXT("un'eccezione che nomina una categoria inesistente"), BothSites, AllPopulated, ExceptionForAGhost, 1);
		Expect(TEXT("tutte le categorie di un sito dichiarate senza azioni: il sito e' escusato"),
			BothSites, TheFirstSiteEmpty, ExceptionsForAAndB, 0);
		Expect(TEXT("una categoria su un sito che non e' dichiarato"), OnlyTheFirstSite, AllPopulated, NoExceptions, 1);
		Expect(TEXT("un sito dichiarato che nessuna categoria serve"), ASiteNobodyServes, AllPopulated, NoExceptions, 1);
		Expect(TEXT("una popolazione che non ha una cella per categoria"), BothSites, OneCellShort, NoExceptions, 1);
	}

	// I test che coprono altrove le categorie "non coperte" esistono ancora.
	TArray<FAutomationTestInfo> Registered;
	FAutomationTestFramework::Get().GetValidTestNames(Registered);
	TSet<FString> RegisteredPaths;
	for (const FAutomationTestInfo& Info : Registered)
	{
		RegisteredPaths.Add(Info.GetFullTestPath());
	}
	// Premessa: il registro che sto leggendo contiene questo stesso test, altrimenti i riferimenti sotto passerebbero a vuoto.
	if (!TestTrue(TEXT("il registro dei test contiene questo stesso test (altrimenti il controllo e' vacuo)"),
			RegisteredPaths.Contains(GetBeautifiedTestName())))
	{
		return false;
	}
	for (const Census::FNotCovered& Item : Census::NotCovered())
	{
		for (const TCHAR* CoveringTest : Item.CoveredBy)
		{
			TestTrue(*FString::Printf(TEXT("'%s' e' coperta altrove da %s, e quel test esiste"), Item.Name, CoveringTest),
				RegisteredPaths.Contains(FString(CoveringTest)));
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
