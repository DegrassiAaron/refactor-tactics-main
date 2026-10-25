#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexArcLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Player/RTPlayerController.h"
#include "Tests/RTAbilityFixtures.h"
#include "Tests/RTWorldFixtures.h"
#include "Turn/RTActionFallbackLibrary.h"
#include "Turn/RTPlanPreview.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTHUD.h"
#include "UI/RTHudViewModel.h"
#include "UI/RTScreenHudWidgets.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

// =====================================================================================================
// `#3848`, [D-540] — LA PIANIFICAZIONE AMMETTE LA MIRA LUNGO UN ARCO.
//
// [D-393] voleva che un'azione bersagliasse il proprio piano; la risoluzione, da [D-509], ferma il COLPO che
// attraversa i piani e lascia l'ARCO: dall'interfaccia, pero', quella mira non si poteva dichiarare. [D-536] (3) la
// ammette, e [D-540] sceglie QUALI azioni: quelle che la risoluzione gia' tratta.
//
// 🔑 **Gli esiti attesi sono scritti a mano**: i numeri del catalogo (35 alla struttura per la carica da breccia) e
// gli stati degli archi come li legge il Blast. Confrontare la pianificazione con una funzione che la pianificazione
// stessa chiama resterebbe verde con la regola sbagliata.
// =====================================================================================================

namespace
{
	const FRTCellId GXlaGround(0, 0, 0);
	const FRTCellId GXlaBridgeEnd(1, 0, 1);
	const FRTCellId GXlaNoBridgeEnd(2, 0, 1);
	const FRTCellId GXlaFar(3, 0, 1);

	/** Raggio 4 sul piano 0 e tre celle sul piano 1. Nessun arco: ciascun test dichiara i propri. */
	URTHexMapAsset* MakeXlaMap(const TArray<FRTCellId>& SightBlockers = TArray<FRTCellId>())
	{
		URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 4))
		{
			FRTHexCellData Data(Id);
			Data.bBlocksLineOfSight = SightBlockers.Contains(Id);
			Map->AddOrUpdateCell(Data);
		}
		for (const FRTCellId& Id : { GXlaBridgeEnd, GXlaNoBridgeEnd, GXlaFar })
		{
			Map->AddOrUpdateCell(FRTHexCellData(Id));
		}
		Map->SortCells();
		return Map;
	}

	ERTHexTargetReason XlaClassify(const URTHexMapAsset* Map, const FRTCellId& To, ERTCrossLayerAim Aim,
		int32 Range = 6, int32 MinRange = 0, ERTLineOfSightPolicy Policy = ERTLineOfSightPolicy::Required)
	{
		return URTCombatLibrary::ClassifyHexTargeting(Map, GXlaGround, To, Range, Policy, MinRange, Aim);
	}
}

/**
 * **La mira attraverso un arco e' ammessa solo dove la risoluzione la esegue** ([D-540]).
 *
 * 🔑 **Ogni asserto ha la sua meta' falsificante.** Il ponte c'e' e l'altro capo e' ammesso; senza il ponte, stessa
 * quota, e' `OtherLayer`; con il ponte ma senza eccezione, `OtherLayer`. Un test che vedesse `Ok` e basta non
 * distinguerebbe «ammesso per l'arco» da «ammesso perche' la regola del piano non c'e' piu'».
 *
 * ⚠️ **Gli stati dell'arco sono quelli di `DamageArc`**: spento incassa ancora, crollato no. E' la domanda che la
 * risoluzione porra' dopo, e che `HasDamageableArc` condivide con lei.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTXlaTargetingAdmitsAimAlongAnArcTest,
	"RefactorTactics.Combat.TargetingAdmitsAimAlongADamageableArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTXlaTargetingAdmitsAimAlongAnArcTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeXlaMap();
	Map->AddTransition(GXlaGround, GXlaBridgeEnd, /*Cost*/ 1, ERTHexTransitionKind::Bridge, /*bBidirectional*/ true);

	// La meta' di [D-393] che NON cambia: senza eccezione, l'altro capo di un ponte e' un altro piano.
	TestEqual(TEXT("senza eccezione (il default) l'altro capo di un ponte resta OtherLayer"),
		URTCombatLibrary::ClassifyHexTargeting(Map, GXlaGround, GXlaBridgeEnd, 6, ERTLineOfSightPolicy::Required),
		ERTHexTargetReason::OtherLayer);
	TestEqual(TEXT("e dichiarare «None» a mano e' la stessa cosa del default"),
		XlaClassify(Map, GXlaBridgeEnd, ERTCrossLayerAim::None), ERTHexTargetReason::OtherLayer);

	// La regola nuova, con la sua meta' falsificante.
	TestEqual(TEXT("lungo un arco da scalfire: l'altro capo del ponte e' ammesso"),
		XlaClassify(Map, GXlaBridgeEnd, ERTCrossLayerAim::AlongDamageableArc), ERTHexTargetReason::Ok);
	TestEqual(TEXT("controllo negativo: stesso piano alto, MA senza ponte, resta OtherLayer"),
		XlaClassify(Map, GXlaNoBridgeEnd, ERTCrossLayerAim::AlongDamageableArc), ERTHexTargetReason::OtherLayer);
	TestEqual(TEXT("e il ponte vale nei due versi: dall'altro capo verso il suolo"),
		URTCombatLibrary::ClassifyHexTargeting(Map, GXlaBridgeEnd, GXlaGround, 6, ERTLineOfSightPolicy::Required, 0,
			ERTCrossLayerAim::AlongDamageableArc),
		ERTHexTargetReason::Ok);

	// Sullo STESSO piano l'eccezione non cambia niente.
	TestEqual(TEXT("sul proprio piano il verdetto non dipende dall'eccezione"),
		XlaClassify(Map, FRTCellId(2, 0, 0), ERTCrossLayerAim::AlongDamageableArc),
		XlaClassify(Map, FRTCellId(2, 0, 0), ERTCrossLayerAim::None));

	// `ModifyArc` getta un arco dove ancora non c'e': qualunque cella dell'altro piano, ponte o no.
	TestEqual(TEXT("verso il capo di un arco da gettare: ammesso anche dove il ponte non c'e'"),
		XlaClassify(Map, GXlaNoBridgeEnd, ERTCrossLayerAim::ToOtherLayerCell), ERTHexTargetReason::Ok);

	// L'ORDINE e' quello della risoluzione: portata, minimo, poi il piano.
	TestEqual(TEXT("fuori portata E con un ponte: il motivo resta OutOfRange"),
		XlaClassify(Map, GXlaBridgeEnd, ERTCrossLayerAim::AlongDamageableArc, /*Range*/ 0), ERTHexTargetReason::OutOfRange);
	TestEqual(TEXT("troppo vicino E con un ponte: il motivo resta TooClose"),
		XlaClassify(Map, GXlaBridgeEnd, ERTCrossLayerAim::AlongDamageableArc, /*Range*/ 6, /*MinRange*/ 2),
		ERTHexTargetReason::TooClose);
	TestEqual(TEXT("gettare un arco fuori portata e' fuori portata"),
		XlaClassify(Map, GXlaFar, ERTCrossLayerAim::ToOtherLayerCell, /*Range*/ 2), ERTHexTargetReason::OutOfRange);

	// Il tiro indiretto toglie la linea e nient'altro: un mortaio senza «DamageStructure» non scavalca un piano.
	TestEqual(TEXT("senza eccezione nemmeno un tiro indiretto raggiunge l'altro capo di un ponte"),
		XlaClassify(Map, GXlaBridgeEnd, ERTCrossLayerAim::None, 6, 0, ERTLineOfSightPolicy::NotRequired),
		ERTHexTargetReason::OtherLayer);

	// LA PRIVACY ([D-225]): il verdetto non dipende da chi sta sulla cella, e un ignoto non lo cambia.
	TestEqual(TEXT("bersaglio NOTO oltre il ponte: nessun rifiuto"),
		URTCombatLibrary::RefusalForKnownTarget(Map, GXlaGround, GXlaBridgeEnd, 6, ERTLineOfSightPolicy::Required,
			/*bKnown*/ true, 0, ERTCrossLayerAim::AlongDamageableArc),
		ERTTargetRefusal::None);
	TestEqual(TEXT("bersaglio NOTO senza il ponte: «altro piano»"),
		URTCombatLibrary::RefusalForKnownTarget(Map, GXlaGround, GXlaNoBridgeEnd, 6, ERTLineOfSightPolicy::Required,
			/*bKnown*/ true, 0, ERTCrossLayerAim::AlongDamageableArc),
		ERTTargetRefusal::OtherLayer);
	TestEqual(TEXT("bersaglio IGNOTO oltre il ponte: Nothing"),
		URTCombatLibrary::RefusalForKnownTarget(Map, GXlaGround, GXlaBridgeEnd, 6, ERTLineOfSightPolicy::Required,
			/*bKnown*/ false, 0, ERTCrossLayerAim::AlongDamageableArc),
		ERTTargetRefusal::Nothing);
	TestEqual(TEXT("bersaglio IGNOTO senza il ponte: Nothing, identico — la presenza non trapela dalla differenza"),
		URTCombatLibrary::RefusalForKnownTarget(Map, GXlaGround, GXlaNoBridgeEnd, 6, ERTLineOfSightPolicy::Required,
			/*bKnown*/ false, 0, ERTCrossLayerAim::AlongDamageableArc),
		ERTTargetRefusal::Nothing);

	// La porta a CELLA, che non ha un soggetto da velare.
	TestEqual(TEXT("cella oltre il ponte: nessun rifiuto"),
		URTCombatLibrary::DescribeCellTargetRefusal(Map, GXlaGround, GXlaBridgeEnd, 6, ERTLineOfSightPolicy::Required, 0,
			ERTCrossLayerAim::AlongDamageableArc).Refusal,
		ERTTargetRefusal::None);
	TestEqual(TEXT("cella oltre il ponte, senza eccezione: «altro piano»"),
		URTCombatLibrary::DescribeCellTargetRefusal(Map, GXlaGround, GXlaBridgeEnd, 6, ERTLineOfSightPolicy::Required)
			.Refusal,
		ERTTargetRefusal::OtherLayer);

	// `CanTargetHexCell` non riceve l'eccezione: resta la domanda di [D-393].
	TestFalse(TEXT("CanTargetHexCell non e' toccata"),
		URTCombatLibrary::CanTargetHexCell(Map, GXlaGround, GXlaBridgeEnd, 6, ERTLineOfSightPolicy::Required));

	// GLI STATI DELL'ARCO, come li legge `DamageArc`.
	URTHexArcLibrary::SetArcState(Map, GXlaGround, GXlaBridgeEnd, ERTHexArcState::Inactive);
	TestEqual(TEXT("arco SPENTO: incassa ancora, quindi si puo' mirare"),
		XlaClassify(Map, GXlaBridgeEnd, ERTCrossLayerAim::AlongDamageableArc), ERTHexTargetReason::Ok);
	URTHexArcLibrary::SetArcState(Map, GXlaGround, GXlaBridgeEnd, ERTHexArcState::Destroyed);
	TestEqual(TEXT("arco CROLLATO: non incassa altro, quindi la mira torna OtherLayer"),
		XlaClassify(Map, GXlaBridgeEnd, ERTCrossLayerAim::AlongDamageableArc), ERTHexTargetReason::OtherLayer);
	TestEqual(TEXT("…ma l'arco da gettare non guarda quelli che ci sono"),
		XlaClassify(Map, GXlaBridgeEnd, ERTCrossLayerAim::ToOtherLayerCell), ERTHexTargetReason::Ok);
	return true;
}

/**
 * **Cio' che la risoluzione non valuta, la pianificazione non lo chiede** ([D-540]).
 *
 * `CollectHexAttacks` ferma il colpo per piano PRIMA della linea di tiro. Se la pianificazione, ammessa la mira,
 * chiedesse anche la linea, rifiuterebbe per «copertura» un piano che scalfisce l'arco.
 *
 * ⛔ **Il controllo positivo non e' decorativo**: la stessa cella raggiunta sul PROPRIO piano, con lo stesso muro,
 * dice «nessuna linea». Senza, un `Ok` potrebbe venire da un muro che non blocca niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTXlaAimAcrossLayersSkipsTheLineOfSightTest,
	"RefactorTactics.Combat.AimAcrossLayersDoesNotAskForTheLineOfSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTXlaAimAcrossLayersSkipsTheLineOfSightTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeXlaMap({ FRTCellId(1, 0, 0) });

	TestEqual(TEXT("controllo: sul proprio piano il muro in (1,0,0) ferma la linea"),
		XlaClassify(Map, FRTCellId(3, 0, 0), ERTCrossLayerAim::None), ERTHexTargetReason::NoLineOfSight);
	if (!TestFalse(TEXT("premessa: la linea fra le due celle di piani diversi e' davvero interrotta"),
			URTHexVisionLibrary::HasLineOfSight(Map, GXlaGround, GXlaFar)))
	{
		return false;
	}
	TestEqual(TEXT("la mira all'altro capo di un arco da gettare esce Ok senza passare dalla linea"),
		XlaClassify(Map, GXlaFar, ERTCrossLayerAim::ToOtherLayerCell), ERTHexTargetReason::Ok);

	Map->AddTransition(GXlaGround, GXlaFar, 1, ERTHexTransitionKind::Bridge, true);
	TestEqual(TEXT("e lo stesso lungo un arco da scalfire"),
		XlaClassify(Map, GXlaFar, ERTCrossLayerAim::AlongDamageableArc), ERTHexTargetReason::Ok);
	return true;
}

/**
 * **`HasDamageableArc` e `DamageArc` rispondono alla stessa domanda** ([D-540]).
 *
 * La pianificazione ammette la mira se `HasDamageableArc` e' vero; la risoluzione scalfisce se `DamageArc` trova un
 * arco. Se divergessero, o si ammetterebbe una mira che non scalfisce niente, o si rifiuterebbe una che scalfirebbe.
 *
 * 🔑 **Due oracoli**: la tabella e' scritta a mano, e il confronto con `DamageArc` e' fatto su un'altra mappa
 * identica (la funzione muta la mappa che riceve).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTXlaHasDamageableArcAgreesWithDamageArcTest,
	"RefactorTactics.Combat.HasDamageableArcAgreesWithDamageArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTXlaHasDamageableArcAgreesWithDamageArcTest::RunTest(const FString&)
{
	struct FCaso
	{
		const TCHAR* Nome;
		bool bAtteso;
		TFunction<void(URTHexMapAsset*)> Prepara;
	};
	const FRTCellId A = GXlaGround;
	const FRTCellId B = GXlaBridgeEnd;
	const FCaso Casi[] = {
		{ TEXT("arco attivo"), true,
			[&](URTHexMapAsset* M) { M->AddTransition(A, B, 1, ERTHexTransitionKind::Bridge, true); } },
		{ TEXT("arco spento"), true,
			[&](URTHexMapAsset* M)
			{
				M->AddTransition(A, B, 1, ERTHexTransitionKind::Bridge, true);
				URTHexArcLibrary::SetArcState(M, A, B, ERTHexArcState::Inactive);
			} },
		{ TEXT("arco crollato"), false,
			[&](URTHexMapAsset* M)
			{
				M->AddTransition(A, B, 1, ERTHexTransitionKind::Bridge, true);
				URTHexArcLibrary::SetArcState(M, A, B, ERTHexArcState::Destroyed);
			} },
		{ TEXT("nessun arco"), false, [](URTHexMapAsset*) {} },
		{ TEXT("arco a senso unico, scritto nel verso inverso"), true,
			[&](URTHexMapAsset* M) { M->AddTransition(B, A, 1, ERTHexTransitionKind::Bridge, false); } },
		{ TEXT("un arco fra altre celle"), false,
			[&](URTHexMapAsset* M) { M->AddTransition(A, GXlaNoBridgeEnd, 1, ERTHexTransitionKind::Bridge, true); } },
	};

	for (const FCaso& Caso : Casi)
	{
		URTHexMapAsset* Letta = MakeXlaMap();
		Caso.Prepara(Letta);
		URTHexMapAsset* Colpita = MakeXlaMap();
		Caso.Prepara(Colpita);

		TestEqual(*FString::Printf(TEXT("%s: la pianificazione risponde come da tabella"), Caso.Nome),
			URTHexArcLibrary::HasDamageableArc(Letta, A, B), Caso.bAtteso);
		TestEqual(*FString::Printf(TEXT("%s: e nell'altro verso"), Caso.Nome),
			URTHexArcLibrary::HasDamageableArc(Letta, B, A), Caso.bAtteso);
		TestEqual(*FString::Printf(TEXT("%s: la risoluzione scalfisce se e solo se la pianificazione ammette"), Caso.Nome),
			URTHexArcLibrary::DamageArc(Colpita, A, B, 1).Num() > 0, Caso.bAtteso);
	}
	TestFalse(TEXT("senza mappa non c'e' nessun arco"), URTHexArcLibrary::HasDamageableArc(nullptr, A, B));
	return true;
}

/**
 * **L'eccezione la concede l'AZIONE, e solo quelle che la risoluzione esegue** ([D-540]).
 *
 * 🔑 **La lista e' scritta a mano**: nel catalogo core, `Action.HeavyAttack` (e' l'unica a dichiarare
 * `DamageStructure`) e `Action.ModifyArc`. Un'azione nuova che dichiarasse il danno alle strutture ne avrebbe
 * l'eccezione per costruzione, come ha l'arco danneggiato dal Blast: il test cade e chiede di dirlo.
 *
 * ⚠️ **Il controllo positivo**: le due azioni attese ci sono davvero nel catalogo, altrimenti una lista che non
 * trova piu' niente darebbe lo stesso verde vuoto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTXlaCrossLayerAimFollowsTheActionTest,
	"RefactorTactics.Combat.CrossLayerAimFollowsTheActionTheResolutionExecutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTXlaCrossLayerAimFollowsTheActionTest::RunTest(const FString&)
{
	const TMap<FName, ERTCrossLayerAim> Attese = {
		{ FName(TEXT("Action.HeavyAttack")), ERTCrossLayerAim::AlongDamageableArc },
		{ FName(TEXT("Action.ModifyArc")), ERTCrossLayerAim::ToOtherLayerCell },
	};

	TSet<FName> Trovate;
	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		const ERTCrossLayerAim Atteso = Attese.Contains(Def.ActionId) ? Attese[Def.ActionId] : ERTCrossLayerAim::None;
		TestEqual(*FString::Printf(TEXT("%s: l'eccezione concessa"), *Def.ActionId.ToString()),
			static_cast<int32>(URTCombatLibrary::CrossLayerAimFor(Def)), static_cast<int32>(Atteso));
		if (Attese.Contains(Def.ActionId))
		{
			Trovate.Add(Def.ActionId);
		}
	}
	for (const TPair<FName, ERTCrossLayerAim>& Coppia : Attese)
	{
		TestTrue(*FString::Printf(TEXT("controllo positivo: %s e' nel catalogo core"), *Coppia.Key.ToString()),
			Trovate.Contains(Coppia.Key));
	}

	// La carica da breccia sostituisce gli effetti di HeavyAttack: perde il danno alle unita', non quello alle strutture.
	const URTEquipmentData* Breach = URTCatalogLibrary::FindEquipment(TEXT("Gadget.BreachCharge"));
	if (TestNotNull(TEXT("Gadget.BreachCharge"), Breach))
	{
		const URTActionData* Carica = URTCatalogLibrary::MakeEquipmentAction(Breach, nullptr);
		if (TestNotNull(TEXT("l'azione della carica"), Carica))
		{
			TestEqual(TEXT("la carica da breccia danneggia le strutture, quindi gli archi"),
				static_cast<int32>(URTCombatLibrary::CrossLayerAimFor(Carica->Def)),
				static_cast<int32>(ERTCrossLayerAim::AlongDamageableArc));
		}
	}

	// Una derivata di `ModifyArc` e' `ModifyArc`, come per il Blast.
	FRTActionDef Derivata;
	Derivata.ActionId = TEXT("Gadget.Test");
	Derivata.DerivedFromActionId = TEXT("Action.ModifyArc");
	TestEqual(TEXT("un'azione derivata da ModifyArc riceve la sua eccezione"),
		static_cast<int32>(URTCombatLibrary::CrossLayerAimFor(Derivata)),
		static_cast<int32>(ERTCrossLayerAim::ToOtherLayerCell));

	// Un `DamageStructure` a zero non scalfisce niente: il Blast scarta `StructurePower <= 0`, e il primo effetto vince.
	FRTActionDef APiuEffetti;
	APiuEffetti.Effects = { FRTActionEffectSpec(ERTActionEffect::DamageStructure, 0),
		FRTActionEffectSpec(ERTActionEffect::DamageStructure, 20) };
	TestEqual(TEXT("il primo DamageStructure e' a zero: nessuna eccezione, come per il Blast"),
		static_cast<int32>(URTCombatLibrary::CrossLayerAimFor(APiuEffetti)), static_cast<int32>(ERTCrossLayerAim::None));
	APiuEffetti.Effects = { FRTActionEffectSpec(ERTActionEffect::DamageStructure, 20) };
	TestEqual(TEXT("controllo: a 20 l'eccezione c'e'"),
		static_cast<int32>(URTCombatLibrary::CrossLayerAimFor(APiuEffetti)),
		static_cast<int32>(ERTCrossLayerAim::AlongDamageableArc));

	// Il danno all'arco lo raccoglie il Blast dagli intenti della sua fase: un'azione che risolve altrove non concede
	// niente, e `Control` — che risolve nel Blast — si'.
	const struct { ERTResolutionPhase Fase; ERTCrossLayerAim Atteso; const TCHAR* Nome; } PerFase[] = {
		{ ERTResolutionPhase::Attack, ERTCrossLayerAim::AlongDamageableArc, TEXT("Attack") },
		{ ERTResolutionPhase::Control, ERTCrossLayerAim::AlongDamageableArc, TEXT("Control: risolve nel Blast") },
		{ ERTResolutionPhase::Preparation, ERTCrossLayerAim::None, TEXT("Preparation") },
		{ ERTResolutionPhase::FastMovement, ERTCrossLayerAim::None, TEXT("FastMovement") },
		{ ERTResolutionPhase::NormalMovement, ERTCrossLayerAim::None, TEXT("NormalMovement") },
		{ ERTResolutionPhase::Environment, ERTCrossLayerAim::None, TEXT("Environment: risolve nel Cleanup") },
		{ ERTResolutionPhase::Cleanup, ERTCrossLayerAim::None, TEXT("Cleanup") },
	};
	for (const auto& Riga : PerFase)
	{
		APiuEffetti.ResolutionPhase = Riga.Fase;
		TestEqual(*FString::Printf(TEXT("fase %s"), Riga.Nome),
			static_cast<int32>(URTCombatLibrary::CrossLayerAimFor(APiuEffetti)), static_cast<int32>(Riga.Atteso));
	}
	return true;
}

/**
 * **L'anteprima non disegna un colpo che la risoluzione non fa partire** ([D-540], [D-509]).
 *
 * Un colpo che mira a un altro piano e' fermato dalla raccolta (`OtherLayerIntents`): nessuna area, nessun segno. Dal
 * momento in cui la pianificazione puo' dichiarare quella mira, l'anteprima deve dire lo stesso. La guardia segue il
 * criterio della raccolta, `bCountsAsAttack`: `Action.ModifyArc` non e' un colpo, e la sua cella resta disegnata.
 *
 * Meta' falsificanti: lo stesso piano disegna; un'azione che non e' un colpo disegna; il colpo fra piani no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTXlaBlastPreviewAcrossLayersTest,
	"RefactorTactics.Combat.BlastPreviewDoesNotPaintAHitAcrossLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTXlaBlastPreviewAcrossLayersTest::RunTest(const FString&)
{
	TArray<FRTHexCombatUnit> Unita;
	FRTHexCombatUnit Tiratore;
	Tiratore.UnitId = 0;
	Tiratore.TeamId = 0;
	Tiratore.Cell = GXlaGround;
	Tiratore.bAlive = true;
	Unita.Add(Tiratore);

	FRTBlastPreviewPlan Piano;
	Piano.AttackerId = 0;
	Piano.bHasAction = true;
	Piano.Shape = ERTAbilityShape::Single;
	Piano.RangeCells = 3;
	Piano.bTargetsCell = true;

	Piano.TargetCell = FRTCellId(1, 0, 0);
	Piano.bCountsAsAttack = true;
	TestEqual(TEXT("controllo: un colpo sul proprio piano disegna la sua cella"),
		URTHexCombatLibrary::MakeBlastPreview(Piano, Unita).HitCells.Num(), 1);

	Piano.TargetCell = GXlaBridgeEnd;
	TestEqual(TEXT("un colpo che mira a un altro piano non disegna niente"),
		URTHexCombatLibrary::MakeBlastPreview(Piano, Unita).HitCells.Num(), 0);

	Piano.bCountsAsAttack = false;
	TestEqual(TEXT("un'azione che non e' un colpo (ModifyArc) resta disegnata sull'altro piano"),
		URTHexCombatLibrary::MakeBlastPreview(Piano, Unita).HitCells.Num(), 1);

	// L'origine e' quella della fase: con uno scatto pianificato sullo stesso piano della mira il colpo disegna.
	Piano.bCountsAsAttack = true;
	Piano.bDashResolves = true;
	Piano.PlannedDashCell = FRTCellId(2, 0, 1);
	TestEqual(TEXT("dall'origine della fase: dopo uno scatto sul piano della mira il colpo disegna"),
		URTHexCombatLibrary::MakeBlastPreview(Piano, Unita).HitCells.Num(), 1);
	return true;
}

namespace
{
	struct FXlaScene
	{
		UWorld* World = nullptr;
		URTHexMapAsset* Map = nullptr;
		ARTHexMapActor* MapActor = nullptr;
		ARTTurnManager* TM = nullptr;
		ARTPlayerController* PC = nullptr;
		ARTUnit* Mine = nullptr;
		ARTUnit* Other = nullptr;
		URTActionDockWidget* Dock = nullptr;
	};

	/** Nomi distinti per file: il progetto usa unity build. */
	ARTUnit* SpawnXlaUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell,
		const TArray<FName>& Loadout)
	{
		ARTUnit* Unit = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!Unit) { return nullptr; }
		Unit->TeamId = TeamId;
		Unit->ConfigureFromHeroData(Hero);
		if (Loadout.Num() > 0)
		{
			Unit->EquipLoadout(Loadout);
		}
		UGameplayStatics::FinishSpawningActor(Unit, FTransform::Identity);
		Unit->bIsBotControlled = false;
		Unit->DispatchBeginPlay();
		Unit->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		Unit->PlannedCell = Cell; // fermo: si misura la mira e il Blast, non il movimento
		return Unit;
	}

	int32 XlaKitIndex(const ARTUnit* Unit, const TCHAR* ActionId)
	{
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* Azione = Unit->GetAbility(I);
			if (Azione && Azione->Def.ActionId == FName(ActionId))
			{
				return I;
			}
		}
		return INDEX_NONE;
	}

	/**
	 * Il banco: chi pianifica a (0,0,0), l'altro sul piano sopra in (1,0,1) e, se richiesto, un ponte fra i due.
	 *
	 * ⛔ **Non nella stessa colonna**: chi sta sotto una piattaforma non vede chi ci sta sopra ([D-511]), quindi la
	 * squadra non lo conoscerebbe e la risoluzione annullerebbe l'azione con `TargetUnknown` prima di toccare l'arco.
	 * Dall'interfaccia quella mira non si potrebbe nemmeno dichiarare: il click chiede un bersaglio osservato.
	 */
	bool BuildXlaScene(FAutomationTestBase& Test, FXlaScene& S, bool bWithBridge, int32 OtherTeam,
		const TArray<FName>& Loadout)
	{
		S.World = RTWorldFixtures::MakeWorld();
		if (!Test.TestNotNull(TEXT("banco: mondo"), S.World)) { return false; }
		S.Map = MakeXlaMap();
		if (bWithBridge)
		{
			S.Map->AddTransition(GXlaGround, GXlaBridgeEnd, 1, ERTHexTransitionKind::Bridge, true);
		}
		S.MapActor = S.World->SpawnActor<ARTHexMapActor>();
		if (!Test.TestNotNull(TEXT("banco: mappa"), S.MapActor)) { return false; }
		S.MapActor->MapAsset = S.Map;
		S.TM = S.World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		S.Mine = SpawnXlaUnit(S.World, 0, URTHeroCatalogLibrary::MakeMuiren(), GXlaGround, Loadout);
		S.Other = SpawnXlaUnit(S.World, OtherTeam, URTHeroCatalogLibrary::MakeAevik(), GXlaBridgeEnd, {});
		S.PC = S.World->SpawnActor<ARTPlayerController>();
		S.Dock = NewObject<URTActionDockWidget>(S.World);
		if (!Test.TestNotNull(TEXT("banco: TM"), S.TM) || !Test.TestNotNull(TEXT("banco: chi pianifica"), S.Mine)
			|| !Test.TestNotNull(TEXT("banco: l'altro"), S.Other) || !Test.TestNotNull(TEXT("banco: PC"), S.PC)
			|| !Test.TestNotNull(TEXT("banco: dock"), S.Dock))
		{
			return false;
		}
		S.Other->SetKnownToObserver(true);
		S.PC->SelectUnit(S.Mine, /*bRecordAsPlayerInput=*/ false);
		S.Dock->SetCommandControllerForTest(S.PC);
		S.Dock->SetSelectedUnitForTest(S.Mine);
		return true;
	}

	/** Come `BuildXlaScene`, ma se il banco non si regge il mondo non resta vivo: i chiamanti escono subito. */
	bool SetUpXlaScene(FAutomationTestBase& Test, FXlaScene& S, bool bWithBridge, int32 OtherTeam,
		const TArray<FName>& Loadout)
	{
		if (BuildXlaScene(Test, S, bWithBridge, OtherTeam, Loadout))
		{
			return true;
		}
		RTWorldFixtures::DestroyWorld(S.World);
		S.World = nullptr;
		return false;
	}

	/**
	 * Il catalogo spedito da' alla carica da breccia portata 0 (e' costruita su `Action.HeavyAttack`, che la lascia
	 * all'arma): il banco la porta a 3 sull'ISTANZA per mirare la colonna accanto. La portata non e' cio' che si misura.
	 */
	void WidenXlaRange(ARTUnit* Unit, int32 AbilityIndex, int32 Range)
	{
		URTActionData* Azione = Unit->Abilities[AbilityIndex];
		Azione->RangeCells = Range;
		Azione->Def.RangeCells = Range;
	}

	void RunXlaBlastTurn(ARTTurnManager* TM)
	{
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** «Lo slot accenderebbe `Invalid` col puntatore sull'altro?» — la lettura A di [D-459], dalla stessa strada del gioco. */
	bool XlaSlotRefuses(const FXlaScene& S, int32 Index)
	{
		S.MapActor->SetHoveredCell(S.Other->Cell, /*bValid=*/ true);
		const TArray<FRTAbilityCooldownView> Righe = S.Dock->GetActions();
		return Righe.IsValidIndex(Index) && Righe[Index].bTargetRefused;
	}
}

/**
 * **Dall'interfaccia: l'altro capo di un ponte si mira, e chi sta li' non e' bersaglio** ([D-540], DoD di #3848).
 *
 * E' il caso che `Combat.Aim.AcrossABridgeTheBridgeIsHitAndTheUnitIsNot` pinna con un piano scritto a mano, qui con il
 * piano che dichiara il CLICK: il click, lo stato dello slot, la timeline e il warning dicono tutti «mira valida», e
 * la risoluzione ferma il colpo e scalfisce l'arco di 35, il numero del catalogo.
 *
 * 🔑 **Tre meta' falsificanti nello stesso banco**: (1) senza il ponte lo stesso click e' rifiutato; (2) con il ponte
 * ma con un'azione che non danneggia le strutture il click e' rifiutato; (3) la carica da breccia contro un ALLEATO un
 * piano sopra fa lo stesso: la mira non guarda a chi appartiene la cella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTXlaBridgeIsAimedAtFromThePlanTest,
	"RefactorTactics.Combat.Aim.ABridgeCanBeAimedAtFromThePlanAndOnlyTheBridgeIsHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTXlaBridgeIsAimedAtFromThePlanTest::RunTest(const FString&)
{
	const TArray<FName> Carica = { FName(TEXT("Gadget.BreachCharge")) };

	// (A) Il caso: la carica da breccia, un nemico noto un piano sopra, un ponte.
	{
		FXlaScene S;
		if (!SetUpXlaScene(*this, S, /*bWithBridge=*/ true, /*OtherTeam=*/ 1, Carica)) { return false; }
		const int32 Breach = XlaKitIndex(S.Mine, TEXT("Gadget.BreachCharge"));
		const FRTHexEdge* Prima = URTHexArcLibrary::FindArc(S.Map, GXlaGround, S.Other->Cell);
		if (!TestTrue(TEXT("premessa: la carica e' nel kit"), Breach != INDEX_NONE)
			|| !TestNotNull(TEXT("premessa: il ponte c'e'"), Prima))
		{
			RTWorldFixtures::DestroyWorld(S.World);
			return false;
		}
		WidenXlaRange(S.Mine, Breach, 3);
		const int32 IntegritaPrima = Prima->Integrity;
		const int32 SalutePrima = S.Other->Health;
		TestEqual(TEXT("premessa: l'altro e' sul piano sopra"), S.Other->Cell.Layer, 1);
		TestFalse(TEXT("premessa: la classificazione SENZA eccezione lo rifiuterebbe"),
			URTCombatLibrary::ClassifyHexTargeting(S.Map, S.Mine->Cell, S.Other->Cell, S.Mine->GetAbility(Breach)->RangeCells,
				S.Mine->GetAbility(Breach)->Def.LineOfSightPolicy) == ERTHexTargetReason::Ok);

		S.PC->ArmKitAbility(Breach);
		if (!TestEqual(TEXT("premessa: la carica e' armata"), S.Mine->SelectedAbilityIndex, Breach))
		{
			RTWorldFixtures::DestroyWorld(S.World);
			return false;
		}
		TestFalse(TEXT("slot: col puntatore sull'altro capo non rifiuta"), XlaSlotRefuses(S, Breach));

		S.PC->HandleClickOnUnitForTest(S.Other);
		if (!TestTrue(TEXT("click: la mira all'altro capo del ponte e' dichiarata"),
				S.Mine->PlannedAttackTarget.Get() == S.Other && S.Mine->PlannedAbilityIndex == Breach))
		{
			RTWorldFixtures::DestroyWorld(S.World);
			return false;
		}
		const TArray<FRTPhasePreviewEntry>& Fasi = S.MapActor->GetPlanPreview().Phases;
		const FRTPhasePreviewEntry* Colpo = Fasi.FindByPredicate(
			[](const FRTPhasePreviewEntry& Voce) { return Voce.Phase == ERTResolutionPhase::Attack; });
		if (TestNotNull(TEXT("timeline: c'e' la voce del Blast"), Colpo))
		{
			TestEqual(TEXT("timeline: non rifiuta il bersaglio che il click ha accettato"), Colpo->TargetRefusal,
				ERTTargetRefusal::None);
			TestEqual(TEXT("timeline: ma non promette celle colpite, perche' il colpo fra piani non parte"),
				Colpo->AffectedCells.Num(), 0);
			TestTrue(TEXT("timeline: e non e' una previsione di colpo"), Colpo->Certainty != ERTIntentCertainty::Predicted);
		}
		// L'ANTEPRIMA DICE CIO' CHE LA RISOLUZIONE FARA' (D-540): il colpo fra piani non parte, quindi nessuna area e
		// nessun segno di colpo su chi sta all'altro capo. Prima di D-540 quella mira non si poteva dichiarare, e
		// nessuno aveva visto cosa l'anteprima ne avrebbe disegnato.
		TestFalse(TEXT("anteprima: nessuna area disegnata"), S.MapActor->IsPreviewAttackValid());
		TestFalse(TEXT("anteprima: la cella dell'altro capo non e' colpita"), S.MapActor->IsPreviewHitCell(S.Other->Cell));
		TSet<FRTCellId> Segnate;
		TSet<FRTCellId> AlleateSegnate;
		ARTHUD::ComputePlannedHitMarks({ S.Mine, S.Other }, /*PlayerTeamId=*/ 0, Segnate, AlleateSegnate);
		TestFalse(TEXT("segni: nessun segno di colpo sull'unita' dell'altro capo"), Segnate.Contains(S.Other->Cell));
		const TArray<FRTAbilityCooldownView> Righe = URTHudViewModel::BuildAbilityCooldowns(S.Mine);
		if (TestTrue(TEXT("warning: la riga della carica c'e'"), Righe.IsValidIndex(Breach)))
		{
			TestFalse(TEXT("warning: il piano accettato non e' degradato"), Righe[Breach].bPlanDegraded);
		}

		RunXlaBlastTurn(S.TM);

		// PREMESSA VERA: la squadra conosceva il bersaglio. Se la risoluzione avesse annullato per «ignoto» l'arco non
		// avrebbe incassato per una ragione che non c'entra con la mira, e il test accuserebbe la mira.
		int32 AnnullatePerIgnoto = 0;
		int32 FermatePerPiano = 0;
		for (const FRTTurnLogEntry& Voce : S.TM->GetTurnLog())
		{
			if (Voce.Category != ERTLogCategory::Fallback
				|| Voce.Outcome != static_cast<uint8>(ERTFallbackOutcome::Cancelled))
			{
				continue;
			}
			AnnullatePerIgnoto += Voce.Amount == static_cast<int32>(ERTActionInvalidReason::TargetUnknown) ? 1 : 0;
			FermatePerPiano += Voce.Amount == static_cast<int32>(ERTActionInvalidReason::OtherLayer) ? 1 : 0;
		}
		TestEqual(TEXT("premessa: la risoluzione non ha annullato l'azione per bersaglio ignoto"), AnnullatePerIgnoto, 0);
		// IL FATTO DEL TURNLOG, dichiarato in D-540: il colpo fermato per piano e' una voce `Fallback`/`Cancelled`/
		// `OtherLayer` ([D-509]), e vive accanto al ponte danneggiato. Sono due fatti, non una contraddizione: il colpo non
		// parte, l'arco incassa. La risoluzione non cambia.
		TestEqual(TEXT("il turno dichiara il colpo fermato per piano, una volta"), FermatePerPiano, 1);

		// La risoluzione NON cambia: il colpo e' fermato per piano, l'arco incassa il numero del catalogo.
		TestEqual(TEXT("chi sta un piano sopra non perde un punto"), SalutePrima - S.Other->Health, 0);
		const FRTHexEdge* Dopo = URTHexArcLibrary::FindArc(S.Map, GXlaGround, S.Other->Cell);
		if (TestNotNull(TEXT("il ponte c'e' ancora"), Dopo))
		{
			TestEqual(TEXT("e ha incassato i 35 della carica da breccia"), IntegritaPrima - Dopo->Integrity, 35);
		}
		RTWorldFixtures::DestroyWorld(S.World);
	}

	// (B) Meta' falsificante 1: lo stesso click SENZA il ponte e' rifiutato, e nessun arco incassa.
	{
		FXlaScene S;
		if (!SetUpXlaScene(*this, S, /*bWithBridge=*/ false, /*OtherTeam=*/ 1, Carica)) { return false; }
		const int32 Breach = XlaKitIndex(S.Mine, TEXT("Gadget.BreachCharge"));
		if (!TestTrue(TEXT("premessa (senza ponte): la carica e' nel kit"), Breach != INDEX_NONE))
		{
			RTWorldFixtures::DestroyWorld(S.World);
			return false;
		}
		WidenXlaRange(S.Mine, Breach, 3);
		TestEqual(TEXT("premessa (senza ponte): il motivo e' il piano, non la portata"),
			URTCombatLibrary::RefusalForKnownTarget(S.Map, S.Mine->Cell, S.Other->Cell, 3,
				S.Mine->GetAbility(Breach)->Def.LineOfSightPolicy, /*bKnown*/ true, 0,
				URTCombatLibrary::CrossLayerAimFor(S.Mine->GetAbility(Breach)->Def)),
			ERTTargetRefusal::OtherLayer);
		S.PC->ArmKitAbility(Breach);
		TestTrue(TEXT("slot (senza ponte): col puntatore sull'altro piano rifiuta"), XlaSlotRefuses(S, Breach));
		S.PC->HandleClickOnUnitForTest(S.Other);
		TestTrue(TEXT("click (senza ponte): la mira su un altro piano resta rifiutata"),
			S.Mine->PlannedAttackTarget.Get() == nullptr);
		RTWorldFixtures::DestroyWorld(S.World);
	}

	// (C) Meta' falsificante 2: il ponte c'e', ma l'azione non danneggia le strutture.
	{
		FXlaScene S;
		if (!SetUpXlaScene(*this, S, /*bWithBridge=*/ true, /*OtherTeam=*/ 1, {})) { return false; }
		const int32 Getto = XlaKitIndex(S.Mine, TEXT("Hero.Muiren.PressureJet"));
		if (!TestTrue(TEXT("premessa (getto): PressureJet e' nel kit"), Getto != INDEX_NONE))
		{
			RTWorldFixtures::DestroyWorld(S.World);
			return false;
		}
		TestEqual(TEXT("premessa (getto): il motivo e' il piano, non la portata"),
			URTCombatLibrary::RefusalForKnownTarget(S.Map, S.Mine->Cell, S.Other->Cell, S.Mine->GetAbility(Getto)->RangeCells,
				S.Mine->GetAbility(Getto)->Def.LineOfSightPolicy, /*bKnown*/ true, 0,
				URTCombatLibrary::CrossLayerAimFor(S.Mine->GetAbility(Getto)->Def)),
			ERTTargetRefusal::OtherLayer);
		S.PC->ArmKitAbility(Getto);
		TestTrue(TEXT("slot (getto): un'azione senza danno alle strutture rifiuta anche col ponte"),
			XlaSlotRefuses(S, Getto));
		S.PC->HandleClickOnUnitForTest(S.Other);
		TestTrue(TEXT("click (getto): il ponte non concede niente a chi non lo scalfisce"),
			S.Mine->PlannedAttackTarget.Get() == nullptr);
		RTWorldFixtures::DestroyWorld(S.World);
	}
	return true;
}

/**
 * **`Action.ModifyArc` mira all'altro capo di un arco che ancora non c'e', e lo getta** ([D-540]).
 *
 * L'arco di `ModifyArc` si getta dove ancora non c'e', quindi la pianificazione non puo' chiederne l'esistenza: ammette
 * qualunque cella dell'altro piano entro portata. La risoluzione e' quella di sempre — `ModifyArc` non passa dal
 * rifiuto per piano — e lo pinna `Actions.ModifyArc.*`; qui si pinna che dall'interfaccia ci si arrivi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTXlaModifyArcIsAimedAcrossLayersTest,
	"RefactorTactics.Combat.Aim.ModifyArcCanBeAimedAcrossLayersAndThrowsTheArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTXlaModifyArcIsAimedAcrossLayersTest::RunTest(const FString&)
{
	FXlaScene S;
	if (!SetUpXlaScene(*this, S, /*bWithBridge=*/ false, /*OtherTeam=*/ 0, {})) { return false; }
	const int32 Arco = RTAbilityFixtures::AddCoreAbility(S.Mine, TEXT("Action.ModifyArc"));
	if (!TestTrue(TEXT("premessa: ModifyArc e' nel kit"), Arco != INDEX_NONE))
	{
		RTWorldFixtures::DestroyWorld(S.World);
		return false;
	}
	const int32 ArchiPrima = S.Map->Transitions.Num();
	TestNull(TEXT("premessa: nessun arco fra le due celle"), URTHexArcLibrary::FindArc(S.Map, GXlaGround, S.Other->Cell));
	TestFalse(TEXT("premessa: senza eccezione la mira sarebbe rifiutata"),
		URTCombatLibrary::ClassifyHexTargeting(S.Map, S.Mine->Cell, S.Other->Cell, S.Mine->GetAbility(Arco)->RangeCells,
			S.Mine->GetAbility(Arco)->Def.LineOfSightPolicy) == ERTHexTargetReason::Ok);

	S.PC->ArmKitAbility(Arco);
	if (!TestEqual(TEXT("premessa: ModifyArc e' armato"), S.Mine->SelectedAbilityIndex, Arco))
	{
		RTWorldFixtures::DestroyWorld(S.World);
		return false;
	}
	TestFalse(TEXT("slot: col puntatore sull'altro piano non rifiuta"), XlaSlotRefuses(S, Arco));
	S.PC->HandleClickOnUnitForTest(S.Other);
	if (!TestTrue(TEXT("click: la mira all'altro piano e' dichiarata"),
			S.Mine->PlannedAttackTarget.Get() == S.Other && S.Mine->PlannedAbilityIndex == Arco))
	{
		RTWorldFixtures::DestroyWorld(S.World);
		return false;
	}

	RunXlaBlastTurn(S.TM);
	TestTrue(TEXT("la risoluzione getta l'arco, che prima non c'era"), S.Map->Transitions.Num() > ArchiPrima);
	TestNotNull(TEXT("…proprio fra le due celle"), URTHexArcLibrary::FindArc(S.Map, GXlaGround, S.Other->Cell));
	RTWorldFixtures::DestroyWorld(S.World);
	return true;
}

/**
 * **Anche la mira a CELLA attraversa un arco** ([D-540]): lo slot sotto il puntatore e il click su una cella.
 *
 * Sono i due siti che passano da `DescribeCellTargetRefusal`, e nessun'azione spedita li raggiunge: il catalogo non ha
 * un'azione ad area che dichiari `DamageStructure`. Il banco ne costruisce una sull'ISTANZA (`Shape::Area` su
 * `Action.HeavyAttack`), come `Combat.Aim.AcrossABridgeTheBridgeIsHitAndTheUnitIsNot` simula l'effetto: la regola e'
 * dell'azione, non del catalogo.
 *
 * Con il ponte il click dichiara il piano a cella e lo slot non rifiuta; senza, tutti e due rifiutano. L'anteprima
 * non disegna l'area: il colpo fra piani non parte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTXlaCellAcrossABridgeTest,
	"RefactorTactics.Combat.Aim.ACellAcrossABridgeIsAimedAtByAnAreaStructureBreaker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTXlaCellAcrossABridgeTest::RunTest(const FString&)
{
	for (const bool bConPonte : { true, false })
	{
		const TCHAR* Quando = bConPonte ? TEXT("con il ponte") : TEXT("senza il ponte");
		FXlaScene S;
		if (!SetUpXlaScene(*this, S, bConPonte, /*OtherTeam=*/ 1, {})) { return false; }
		const int32 Ordigno = RTAbilityFixtures::AddCoreAbility(S.Mine, TEXT("Action.HeavyAttack"));
		if (!TestTrue(TEXT("premessa: l'azione c'e'"), Ordigno != INDEX_NONE))
		{
			RTWorldFixtures::DestroyWorld(S.World);
			return false;
		}
		URTActionData* Azione = S.Mine->Abilities[Ordigno];
		Azione->Shape = ERTAbilityShape::Area;
		Azione->AreaRadius = 1;
		WidenXlaRange(S.Mine, Ordigno, 3);

		S.PC->ArmKitAbility(Ordigno);
		if (!TestEqual(*FString::Printf(TEXT("premessa (%s): l'azione e' armata"), Quando),
				S.Mine->SelectedAbilityIndex, Ordigno))
		{
			RTWorldFixtures::DestroyWorld(S.World);
			return false;
		}
		TestEqual(*FString::Printf(TEXT("slot (%s): il puntatore sulla cella rifiuta solo senza il ponte"), Quando),
			XlaSlotRefuses(S, Ordigno), !bConPonte);

		S.PC->HandleClickOnCellForTest(GXlaBridgeEnd);
		const bool bDichiarato = S.Mine->bAttackTargetsCell && S.Mine->PlannedAttackCell == GXlaBridgeEnd
			&& S.Mine->PlannedAbilityIndex == Ordigno;
		TestEqual(*FString::Printf(TEXT("click (%s): il piano a cella e' dichiarato solo con il ponte"), Quando),
			bDichiarato, bConPonte);
		if (bConPonte)
		{
			TestFalse(TEXT("anteprima: l'area non e' disegnata sull'altro piano"), S.MapActor->IsPreviewAttackValid());
			TestFalse(TEXT("anteprima: la cella dell'altro capo non e' colpita"), S.MapActor->IsPreviewHitCell(GXlaBridgeEnd));
		}
		RTWorldFixtures::DestroyWorld(S.World);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
