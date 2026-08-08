#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTCatalogLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Combat/RTCombatResolver.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Combat/RTOffensiveActionLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTActionEffectLibrary.h"
#include "Turn/RTActionQueue.h"
#include "Turn/RTTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Definizione dal catalogo delle azioni generiche. Nome distinto per file (unity build). */
	FRTActionDef OffensiveDef(const TCHAR* Id)
	{
		return URTCatalogLibrary::FindCoreAction(FName(Id));
	}

	/** Danno dichiarato da un'azione (0 se non ne dichiara). */
	int32 DeclaredDamage(const FRTActionDef& Def)
	{
		for (const FRTActionEffectSpec& Spec : Def.Effects)
		{
			if (Spec.Effect == ERTActionEffect::Damage) { return Spec.Amount; }
		}
		return 0;
	}

	/** Esagono pieno di raggio N, tutte le celle percorribili e trasparenti. */
	URTHexMapAsset* MakeOffensiveMap(int32 Radius = 7)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	/** Copertura ALTA: blocca la linea di tiro (nel modello dati di oggi e' l'unico dato che la rappresenta). */
	void MakeHighCover(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	FRTHexCombatUnit OffensiveCombatUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		return U;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOffensiveActionsMatchCatalogTest,
	"RefactorTactics.Actions.OffensiveActionsMatchCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOffensiveActionsMatchCatalogTest::RunTest(const FString&)
{
	// Le sei offensive con i numeri della tabella §3 del catalogo. Documento e codice non possono divergere
	// in silenzio: se una riga cambia di la', questo test diventa rosso.
	struct FExpected
	{
		const TCHAR* Id;
		ERTMatchPhase Macro;
		int32 Priority;
		int32 Damage;
		int32 Cooldown;
		ERTActionFallback Fallback;
	};
	const FExpected Expected[] = {
		{ TEXT("Action.MarkTarget"),      ERTMatchPhase::Blast, 40,  0, 1, ERTActionFallback::Cancel },
		{ TEXT("Action.LineAttack"),      ERTMatchPhase::Blast, 55, 22, 1, ERTActionFallback::AttackCell },
		{ TEXT("Action.PrecisionAttack"), ERTMatchPhase::Blast, 60, 24, 1, ERTActionFallback::Cancel },
		{ TEXT("Action.CircularAoE"),     ERTMatchPhase::Blast, 65, 18, 2, ERTActionFallback::AttackCell },
		{ TEXT("Action.HeavyAttack"),     ERTMatchPhase::Blast, 80, 35, 2, ERTActionFallback::Cancel },
		{ TEXT("Action.SuppressiveLine"), ERTMatchPhase::Prep,  30, 16, 2, ERTActionFallback::Cancel },
	};

	for (const FExpected& E : Expected)
	{
		const FRTActionDef Def = OffensiveDef(E.Id);
		if (!TestTrue(FString::Printf(TEXT("%s e' nel catalogo"), E.Id), Def.ActionId == FName(E.Id)))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: macro-fase"), E.Id),
			URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase) == E.Macro);
		TestEqual(FString::Printf(TEXT("%s: priorita'"), E.Id), Def.Priority, E.Priority);
		TestEqual(FString::Printf(TEXT("%s: danno dichiarato"), E.Id), DeclaredDamage(Def), E.Damage);
		TestEqual(FString::Printf(TEXT("%s: cooldown"), E.Id), Def.CooldownTurns, E.Cooldown);
		TestTrue(FString::Printf(TEXT("%s: fallback"), E.Id), Def.Fallback == E.Fallback);
		TestTrue(FString::Printf(TEXT("%s: occupa l'azione principale"), E.Id), Def.Slot == ERTActionSlot::Main);
	}

	// Il fuoco amico e' un dato dell'AZIONE, ed e' l'eccezione di UNA sola: le altre cinque risparmiano gli
	// alleati senza che nessuno debba ricordarsene al momento di costruire l'intento.
	TestTrue(TEXT("solo l'area circolare colpisce i propri"),
		OffensiveDef(TEXT("Action.CircularAoE")).bFriendlyFire);
	for (const FRTActionDef& Def : URTCatalogLibrary::GetCoreActionCatalog())
	{
		if (Def.ActionId != FName(TEXT("Action.CircularAoE")))
		{
			TestFalse(FString::Printf(TEXT("%s non colpisce gli alleati"), *Def.ActionId.ToString()),
				Def.bFriendlyFire);
		}
	}

	// La priorita' e' cio' che DIFFERENZIA le offensive dentro il Blast (obiettivo della issue): il marchio
	// prima di tutti perche' il suo +6 deve valere sui colpi dello stesso turno, il pesante per ultimo.
	TestTrue(TEXT("il marchio risolve prima della linea"),
		OffensiveDef(TEXT("Action.MarkTarget")).Priority < OffensiveDef(TEXT("Action.LineAttack")).Priority);
	TestTrue(TEXT("il pesante risolve dopo ogni altra offensiva"),
		OffensiveDef(TEXT("Action.HeavyAttack")).Priority > OffensiveDef(TEXT("Action.CircularAoE")).Priority);

	// La soppressione si PREPARA: e' l'unica offensiva fuori dal Blast, e non e' interrompibile.
	const FRTActionDef Suppressive = OffensiveDef(TEXT("Action.SuppressiveLine"));
	TestFalse(TEXT("una linea preparata non si interrompe"), Suppressive.bCanBeInterrupted);
	TestEqual(TEXT("la portata della linea e' 5"), Suppressive.RangeCells, 5);
	TestEqual(TEXT("il centro dell'area si sceglie entro 4 celle"),
		OffensiveDef(TEXT("Action.CircularAoE")).RangeCells, 4);
	TestEqual(TEXT("l'attacco lineare arriva a 5"), OffensiveDef(TEXT("Action.LineAttack")).RangeCells, 5);

	// L'intero catalogo generico passa dal validator, nuove voci comprese.
	const TArray<FString> Errors = URTCatalogLibrary::ValidateActions(URTCatalogLibrary::GetCoreActionCatalog());
	for (const FString& Err : Errors) { AddError(Err); }
	TestEqual(TEXT("le azioni generiche restano valide"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPrecisionAttackTest,
	"RefactorTactics.Actions.PrecisionAttack.WeaponRangePlusOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPrecisionAttackTest::RunTest(const FString&)
{
	// 24 danni fissi e portata dell'arma +1 (catalogo §3): a differenza dell'attacco base, il danno NON
	// dipende dalla fascia — e' il colpo che si paga con un cooldown per arrivare una cella piu' in la'.
	const FRTActionDef Short = URTCatalogLibrary::MakePrecisionAttack(3);
	TestTrue(TEXT("un solo ActionId"), Short.ActionId == FName(TEXT("Action.PrecisionAttack")));
	TestEqual(TEXT("arma a 3 -> portata 4"), Short.RangeCells, 4);
	TestEqual(TEXT("danno fisso 24"), DeclaredDamage(Short), 24);

	const FRTActionDef Long = URTCatalogLibrary::MakePrecisionAttack(6);
	TestEqual(TEXT("arma a 6 -> portata 7"), Long.RangeCells, 7);
	TestEqual(TEXT("il danno non cambia con la portata"), DeclaredDamage(Long), 24);

	// Il bonus e' SEMPRE +1 sopra l'attacco base della stessa arma: e' l'identita' dell'azione.
	for (int32 Weapon = 1; Weapon <= 6; ++Weapon)
	{
		TestEqual(FString::Printf(TEXT("arma %d: una cella oltre l'attacco base"), Weapon),
			URTCatalogLibrary::MakePrecisionAttack(Weapon).RangeCells,
			URTCatalogLibrary::MakeBasicAttack(Weapon).RangeCells + 1);
	}
	// Portata degenere: si ricade sul corpo a corpo (1) +1, non su una portata 1 "nuda".
	TestEqual(TEXT("arma 0 -> portata 2"), URTCatalogLibrary::MakePrecisionAttack(0).RangeCells, 2);

	// **Usabile dopo Sprint** da D-028, e non perche' lo dica un `if` sull'ActionId: lo Sprint occupa il solo
	// slot movimento, quindi l'attacco trova ancora la principale libera. E' *corro e sparo*, la stessa forma
	// di *schivo e sparo* — cio' che si rinuncia e' il `Move` normale, non il colpo.
	//
	// Prima di D-028 questo test verificava il rifiuto, e non era sbagliato: era la regola di allora.
	const FRTActionDef Sprint = OffensiveDef(TEXT("Action.Sprint"));
	const TArray<FString> AfterSprint = URTCatalogLibrary::ValidateActionSlots({ Sprint, Short });
	TestEqual(TEXT("Sprint + precisione: piano accettato"), AfterSprint.Num(), 0);

	// Cio' che resta vietato e' DUE movimenti: e' li' che lo Sprint ha smesso di essere gratis.
	const TArray<FString> SprintAndMove =
		URTCatalogLibrary::ValidateActionSlots({ Sprint, OffensiveDef(TEXT("Action.Move")) });
	TestEqual(TEXT("Sprint + Move: piano rifiutato"), SprintAndMove.Num(), 1);
	if (SprintAndMove.Num() > 0)
	{
		TestTrue(TEXT("e l'errore nomina il movimento"), SprintAndMove[0].Contains(TEXT("Action.")));
	}

	// La stessa precisione dopo un movimento NORMALE resta valida: e' lo Sprint a costare la principale.
	const TArray<FString> AfterMove =
		URTCatalogLibrary::ValidateActionSlots({ OffensiveDef(TEXT("Action.Move")), Short });
	TestEqual(TEXT("Move + precisione: piano valido"), AfterMove.Num(), 0);

	// Limite dichiarato: «ignora la copertura bassa» non e' verificabile finche' la copertura bassa non
	// esiste — `FRTHexCellData` ha `bBlocksMovement` e `bBlocksLineOfSight`, non un campo cover (epic E9).
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeavyAttackInterruptedTest,
	"RefactorTactics.Actions.HeavyAttack.NoEffectIfInterrupted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeavyAttackInterruptedTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. Il colpo pesante e' tutto o niente: interrotto prima del Blast non produce
	// NULLA — non mezzo danno, non un effetto parziale.
	FRTActionInstance Heavy;
	Heavy.Def = URTCatalogLibrary::MakeWeaponAttack(TEXT("Action.HeavyAttack"), /*Arma*/ 4);
	Heavy.SourceUnitId = 0;
	Heavy.TargetUnitId = 1;

	if (!TestTrue(TEXT("Action.HeavyAttack e' nel catalogo"),
		Heavy.Def.ActionId == FName(TEXT("Action.HeavyAttack"))))
	{
		return false;
	}
	TestTrue(TEXT("e' interrompibile: e' cio' che si paga per 35 danni"), Heavy.Def.bCanBeInterrupted);

	const TArray<FRTActionEvent> Normal = URTActionEffectLibrary::ProduceEvents(Heavy);
	if (!TestEqual(TEXT("non interrotta: un evento di danno"), Normal.Num(), 1)) { return false; }
	TestEqual(TEXT("e sono 35 danni"), Normal[0].Amount, 35);

	Heavy.bInterrupted = true;
	const TArray<FRTActionEvent> Interrupted = URTActionEffectLibrary::ProduceEvents(Heavy);
	TestEqual(TEXT("interrotta: nessun evento, nemmeno parziale"), Interrupted.Num(), 0);

	// La regola sta in un solo punto e vale per ogni azione, non solo per questa.
	FRTActionInstance Precision;
	Precision.Def = URTCatalogLibrary::MakePrecisionAttack(4);
	Precision.SourceUnitId = 0;
	Precision.TargetUnitId = 1;
	Precision.bInterrupted = true;
	TestEqual(TEXT("vale anche per la precisione"), URTActionEffectLibrary::ProduceEvents(Precision).Num(), 0);

	// Chi dichiara di non essere interrompibile non lo e' nemmeno col flag alzato: `bCanBeInterrupted` resta
	// l'ultima parola, e la linea preparata continua a esistere (catalogo §3).
	FRTActionInstance Suppressive;
	Suppressive.Def = OffensiveDef(TEXT("Action.SuppressiveLine"));
	Suppressive.SourceUnitId = 0;
	Suppressive.TargetUnitId = 1;
	Suppressive.bInterrupted = true;
	TestEqual(TEXT("la soppressione non si interrompe"),
		URTActionEffectLibrary::ProduceEvents(Suppressive).Num(), 1);

	// Limiti dichiarati: a METTERE il flag sara' `Action.Interrupt` (CP 4.7, issue #48). Qui c'e' la
	// conseguenza, non ancora la causa. I 20 danni alle coperture distruttibili restano fuori: le strutture
	// non esistono nel modello dati (epic E9).
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTLineAttackFirstTargetTest,
	"RefactorTactics.Actions.LineAttack.FirstValidTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTLineAttackFirstTargetTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOffensiveMap();
	const FRTCellId From(0, 0);

	TMap<FRTCellId, int32> Occupancy;
	Occupancy.Add(FRTCellId(2, 0), 5);
	Occupancy.Add(FRTCellId(4, 0), 6);
	const TSet<int32> Hostiles = { 5, 6 };

	// Due nemici in fila: colpisce il PRIMO. E' cio' che distingue l'attacco lineare da una `Shape::Line`,
	// che li prenderebbe entrambi.
	const FRTLineAttackResult Hit = URTOffensiveActionLibrary::ResolveLineAttack(
		Map, From, FRTCellId(5, 0), /*Range*/ 5, Occupancy, Hostiles);
	TestTrue(TEXT("colpisce il primo bersaglio"), Hit.Stop == ERTLineStop::Hit && Hit.HitUnitId == 5);
	TestEqual(TEXT("il colpo si ferma li'"), Hit.Cells.Num(), 2);

	// Il punto mirato sceglie solo la DIREZIONE: mirare a una cella vicina non accorcia la linea.
	const FRTLineAttackResult SameDirection = URTOffensiveActionLibrary::ResolveLineAttack(
		Map, From, FRTCellId(1, 0), 5, Occupancy, Hostiles);
	TestTrue(TEXT("stessa direzione, stesso bersaglio"), SameDirection.HitUnitId == 5);

	// Una COPERTURA ALTA fra l'attaccante e il bersaglio interrompe la linea.
	URTHexMapAsset* Covered = MakeOffensiveMap();
	MakeHighCover(Covered, FRTCellId(1, 0));
	const FRTLineAttackResult Blocked = URTOffensiveActionLibrary::ResolveLineAttack(
		Covered, From, FRTCellId(5, 0), 5, Occupancy, Hostiles);
	TestTrue(TEXT("la copertura alta ferma il colpo"), Blocked.Stop == ERTLineStop::BlockedByCover);
	TestEqual(TEXT("e nessuno viene colpito"), Blocked.HitUnitId, INDEX_NONE);
	TestEqual(TEXT("la linea non arriva nemmeno alla copertura"), Blocked.Cells.Num(), 0);

	// Un ALLEATO non e' un bersaglio valido e non fa da scudo: il colpo prosegue oltre.
	TMap<FRTCellId, int32> WithAlly;
	WithAlly.Add(FRTCellId(1, 0), 9);   // alleato: non e' in Hostiles
	WithAlly.Add(FRTCellId(3, 0), 6);
	const FRTLineAttackResult PastAlly = URTOffensiveActionLibrary::ResolveLineAttack(
		Map, From, FRTCellId(5, 0), 5, WithAlly, Hostiles);
	TestTrue(TEXT("supera l'alleato e colpisce il nemico dietro"), PastAlly.HitUnitId == 6);

	// Direzione non esagonale: l'azione non parte (nessun bersaglio "piu' vicino" scelto al posto nostro —
	// il targeting automatico e' vietato dal catalogo §17).
	const FRTLineAttackResult Oblique = URTOffensiveActionLibrary::ResolveLineAttack(
		Map, From, FRTCellId(2, 1), 5, Occupancy, Hostiles);
	TestTrue(TEXT("direzione obliqua: non parte"), Oblique.Stop == ERTLineStop::NotAligned);

	// Portata: un nemico a 6 celle resta fuori dalle 5 dichiarate dal catalogo.
	TMap<FRTCellId, int32> TooFar;
	TooFar.Add(FRTCellId(6, 0), 6);
	const FRTLineAttackResult OutOfRange = URTOffensiveActionLibrary::ResolveLineAttack(
		Map, From, FRTCellId(5, 0), 5, TooFar, Hostiles);
	TestTrue(TEXT("oltre la portata non si colpisce"), OutOfRange.Stop == ERTLineStop::NoTarget);
	TestEqual(TEXT("ma la linea ha percorso tutte e 5 le celle"), OutOfRange.Cells.Num(), 5);

	// FAIL-CLOSED: senza mappa autorevole non si valuta la copertura, quindi non si colpisce.
	const FRTLineAttackResult NoMap = URTOffensiveActionLibrary::ResolveLineAttack(
		nullptr, From, FRTCellId(5, 0), 5, Occupancy, Hostiles);
	TestTrue(TEXT("senza mappa non si colpisce"), NoMap.Stop == ERTLineStop::NoMap && NoMap.HitUnitId == INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAoEFriendlyFireTest,
	"RefactorTactics.Actions.AoE.FriendlyFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAoEFriendlyFireTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. L'area circolare colpisce chiunque stia nel raggio, alleati compresi: e' il
	// prezzo dei suoi 18 danni su sette celle, e la ragione per cui il centro si sceglie con attenzione.
	URTHexMapAsset* Map = MakeOffensiveMap();

	const TArray<FRTHexCombatUnit> Units = {
		OffensiveCombatUnit(0, /*Team*/ 0, FRTCellId(0, 0)),   // chi lancia
		OffensiveCombatUnit(1, /*Team*/ 1, FRTCellId(3, 0)),   // nemico al centro
		OffensiveCombatUnit(2, /*Team*/ 0, FRTCellId(2, 0)),   // ALLEATO, adiacente al centro: dentro il raggio 1
		OffensiveCombatUnit(3, /*Team*/ 0, FRTCellId(0, 1)),   // alleato lontano: fuori dall'area
	};

	// Il fuoco amico arriva dai DATI dell'azione, non da una scelta di questo test: se domani il catalogo lo
	// togliesse a `CircularAoE`, l'intento cambierebbe da solo (ed e' questo test a diventare rosso).
	const FRTActionDef Def = OffensiveDef(TEXT("Action.CircularAoE"));
	TestTrue(TEXT("l'area dichiara il fuoco amico nel catalogo"), Def.bFriendlyFire);

	FRTHexAttackIntent Intent;
	Intent.AttackerId = 0;
	Intent.TargetId = 1;
	Intent.Shape = ERTAbilityShape::Area;
	Intent.AreaRadius = 1;
	Intent.RangeCells = Def.RangeCells;
	Intent.Power = DeclaredDamage(Def);
	Intent.bFriendlyFire = Def.bFriendlyFire;

	const FRTHexBlastPlan Plan = URTHexCombatLibrary::CollectHexAttacks(Units, { Intent }, Map);
	TestEqual(TEXT("colpisce il nemico e l'alleato vicino"), Plan.Hits.Num(), 2);

	TSet<int32> HitIds;
	for (const FRTHexAttackHit& H : Plan.Hits)
	{
		HitIds.Add(H.TargetId);
		TestEqual(TEXT("18 danni a testa"), H.Power, 18);
	}
	TestTrue(TEXT("il nemico e' colpito"), HitIds.Contains(1));
	TestTrue(TEXT("l'alleato dentro il raggio anche"), HitIds.Contains(2));
	TestFalse(TEXT("l'alleato fuori raggio no"), HitIds.Contains(3));
	TestFalse(TEXT("chi lancia non si colpisce da solo"), HitIds.Contains(0));

	// Senza il flag, la stessa area risparmia l'alleato: il fuoco amico e' una scelta DICHIARATA dall'azione,
	// non il comportamento di base del Blast.
	FRTHexAttackIntent Safe = Intent;
	Safe.bFriendlyFire = false;
	const FRTHexBlastPlan SafePlan = URTHexCombatLibrary::CollectHexAttacks(Units, { Safe }, Map);
	TestEqual(TEXT("senza fuoco amico colpisce solo il nemico"), SafePlan.Hits.Num(), 1);

	// Un'area a fuoco amico puo' anche essere CENTRATA su un alleato: e' un errore di chi gioca, non un
	// intento da scartare in silenzio.
	FRTHexAttackIntent OnAlly = Intent;
	OnAlly.TargetId = 2;
	const FRTHexBlastPlan AllyPlan = URTHexCombatLibrary::CollectHexAttacks(Units, { OnAlly }, Map);
	TestTrue(TEXT("centrata sull'alleato, esplode lo stesso"), AllyPlan.Hits.Num() >= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSuppressiveLineTest,
	"RefactorTactics.Actions.SuppressiveLine.SingleActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSuppressiveLineTest::RunTest(const FString&)
{
	URTHexMapAsset* Map = MakeOffensiveMap();
	const FRTActionDef Def = OffensiveDef(TEXT("Action.SuppressiveLine"));

	const FRTSuppressiveZone Zone = URTOffensiveActionLibrary::MakeSuppressiveZone(
		Map, /*Owner*/ 0, /*OwnerTeam*/ 0, FRTCellId(0, 0), FRTCellId(1, 0),
		Def.RangeCells, DeclaredDamage(Def));
	TestEqual(TEXT("la linea controlla 5 celle"), Zone.Cells.Num(), 5);
	TestEqual(TEXT("e infligge 16 danni"), Zone.Damage, 16);

	// Due nemici entrano nella linea: scatta una SOLA volta, e la prende chi entra al passo piu' basso.
	FRTSuppressionMover Late;
	Late.UnitId = 2;
	Late.TeamId = 1;
	Late.Path = { FRTCellId(3, 2), FRTCellId(3, 1), FRTCellId(3, 0) }; // entra al passo 2

	FRTSuppressionMover Early;
	Early.UnitId = 3;
	Early.TeamId = 1;
	Early.Path = { FRTCellId(2, 0), FRTCellId(2, 1) };                 // entra al passo 0

	const FRTSuppressionHit Hit = URTOffensiveActionLibrary::ResolveSuppression(Zone, { Late, Early });
	TestTrue(TEXT("scatta"), Hit.IsValid());
	TestEqual(TEXT("la prende chi entra prima"), Hit.VictimUnitId, 3);
	TestEqual(TEXT("al passo 0"), Hit.StepIndex, 0);
	TestEqual(TEXT("per 16 danni"), Hit.Damage, 16);
	TestTrue(TEXT("e resta nella cella appena raggiunta"), Hit.StopCell == FRTCellId(2, 0));

	// Permutare l'ordine dei movimenti non cambia chi la prende: l'ordine e' totale (passo, poi UnitId), non
	// quello dell'array (invariante #4).
	const FRTSuppressionHit Swapped = URTOffensiveActionLibrary::ResolveSuppression(Zone, { Early, Late });
	TestEqual(TEXT("permutazione: stessa vittima"), Swapped.VictimUnitId, Hit.VictimUnitId);
	TestEqual(TEXT("permutazione: stesso passo"), Swapped.StepIndex, Hit.StepIndex);

	// Pareggio sul passo: decide l'UnitId piu' basso, non l'ordine di dichiarazione.
	FRTSuppressionMover TieHigh;
	TieHigh.UnitId = 7;
	TieHigh.TeamId = 1;
	TieHigh.Path = { FRTCellId(1, 0) };
	FRTSuppressionMover TieLow;
	TieLow.UnitId = 4;
	TieLow.TeamId = 1;
	TieLow.Path = { FRTCellId(2, 0) };
	const FRTSuppressionHit Tie = URTOffensiveActionLibrary::ResolveSuppression(Zone, { TieHigh, TieLow });
	TestEqual(TEXT("stesso passo: vince l'id piu' basso"), Tie.VictimUnitId, 4);

	// La linea non scatta sui propri: e' una trappola per il nemico, non un campo minato per la squadra.
	FRTSuppressionMover Ally;
	Ally.UnitId = 1;
	Ally.TeamId = 0;
	Ally.Path = { FRTCellId(1, 0), FRTCellId(2, 0) };
	TestFalse(TEXT("un alleato ci cammina dentro senza conseguenze"),
		URTOffensiveActionLibrary::ResolveSuppression(Zone, { Ally }).IsValid());

	// Chi passa accanto alla linea senza entrarci non la fa scattare.
	FRTSuppressionMover Beside;
	Beside.UnitId = 5;
	Beside.TeamId = 1;
	Beside.Path = { FRTCellId(1, 1), FRTCellId(2, 1) };
	TestFalse(TEXT("passare accanto non basta"),
		URTOffensiveActionLibrary::ResolveSuppression(Zone, { Beside }).IsValid());

	// Una copertura alta accorcia la linea tanto quanto ferma un attacco lineare: stessa geometria.
	URTHexMapAsset* Covered = MakeOffensiveMap();
	MakeHighCover(Covered, FRTCellId(3, 0));
	const FRTSuppressiveZone Short = URTOffensiveActionLibrary::MakeSuppressiveZone(
		Covered, 0, 0, FRTCellId(0, 0), FRTCellId(1, 0), Def.RangeCells, DeclaredDamage(Def));
	TestEqual(TEXT("la copertura alta accorcia la linea a 2 celle"), Short.Cells.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTMarkTargetConsumedOnceTest,
	"RefactorTactics.Actions.MarkTarget.ConsumedOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTMarkTargetConsumedOnceTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. Il marchio vale per UN colpo: il secondo attacco alleato dello stesso turno
	// arriva intero. Passa dalla stessa `ApplyFirstHitDelta` di `Exposed` e `Guard`, quindi "consumato" resta
	// ordine-indipendente invece di diventare una corsa fra attaccanti.
	TestEqual(TEXT("il marchio vale +6"), URTCombatLibrary::MarkedFirstHitBonus, 6);

	TArray<int32> Delta;
	Delta.Init(0, 3);
	Delta[1] = URTCombatLibrary::MarkedFirstHitBonus;

	const TArray<FRTAttack> Attacks = { FRTAttack(1, 20), FRTAttack(1, 20), FRTAttack(2, 20) };
	const TArray<FRTAttack> Marked = URTCombatResolver::ApplyFirstHitDelta(Attacks, Delta);

	if (!TestEqual(TEXT("tre colpi restano tre"), Marked.Num(), 3)) { return false; }
	TestEqual(TEXT("il primo colpo sul marchiato: 20 + 6"), Marked[0].Power, 26);
	TestEqual(TEXT("il secondo arriva intero: il marchio e' consumato"), Marked[1].Power, 20);
	TestEqual(TEXT("chi non e' marchiato non c'entra"), Marked[2].Power, 20);

	// Marchiato E in guardia: i due delta si cumulano (+6 -15 = -9). Esito prevedibile, non una precedenza
	// nascosta dell'uno sull'altro.
	TArray<int32> Both;
	Both.Init(0, 2);
	Both[1] = URTCombatLibrary::MarkedFirstHitBonus - URTCombatLibrary::GuardFirstHitReduction;
	const TArray<FRTAttack> Mixed = URTCombatResolver::ApplyFirstHitDelta({ FRTAttack(1, 20) }, Both);
	TestEqual(TEXT("marchiato e in guardia: 20 + 6 - 15"), Mixed[0].Power, 11);

	// L'azione che lo produce e' DATI: dichiara uno stato, non un numero nell'orchestratore.
	const FRTActionDef Mark = OffensiveDef(TEXT("Action.MarkTarget"));
	TestEqual(TEXT("il marchio non fa danno di suo"), DeclaredDamage(Mark), 0);
	TestEqual(TEXT("dichiara un solo effetto"), Mark.Effects.Num(), 1);
	TestTrue(TEXT("ed e' Status.Marked per un turno"), Mark.Effects.Num() == 1
		&& Mark.Effects[0].Effect == ERTActionEffect::Status
		&& Mark.Effects[0].StatusTag == TAG_Status_Marked
		&& Mark.Effects[0].StatusDuration == 1);

	// L'istanza produce l'evento di stato, non un danno: e' cio' che il registry traduce.
	FRTActionInstance Instance;
	Instance.Def = Mark;
	Instance.SourceUnitId = 0;
	Instance.TargetUnitId = 1;
	const TArray<FRTActionEvent> Events = URTActionEffectLibrary::ProduceEvents(Instance);
	if (!TestEqual(TEXT("un solo evento"), Events.Num(), 1)) { return false; }
	TestTrue(TEXT("ed e' l'applicazione dello stato"), Events[0].Kind == ERTActionEffect::Status
		&& Events[0].StatusTag == TAG_Status_Marked);

	// Limite dichiarato: «non aumenta il danno ambientale» e' vero per costruzione — il danno ambientale non
	// passa da `ApplyFirstHitDelta`, che vede solo i colpi diretti. Gli hazard arrivano con l'epic E8 e non
	// c'e' nulla da verificare qui prima di allora.
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
