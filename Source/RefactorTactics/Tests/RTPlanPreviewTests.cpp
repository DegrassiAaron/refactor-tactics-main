// La TIMELINE del piano: una voce per fase, e la reazione che fase non e' — `CP 11.5` (#172).
//
// Questi test coprono il PRODUTTORE e non il wiring, come gia' `RTBlastPreviewTests.cpp`. La distinzione
// conta: i `Preview.*` piu' vecchi verificano che `ARTHexMapActor` conservi le celle che gli si passano — un
// setter — e quelle celle gliele calcola il test. Nessuno di quei verdi dice che il gioco le calcola giuste.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTFacingLibrary.h" // l'oracolo del facing: la stessa derivazione pura che usa la preview
#include "Turn/RTPlanPreview.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: nella unity build condividono la translation unit.
	URTHexMapAsset* MakePlanPreviewMap(int32 Radius)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
		{
			M->AddOrUpdateCell(FRTHexCellData(Id));
		}
		M->SortCells();
		return M;
	}

	FRTHexCombatUnit MakePlanPreviewCombatUnit(int32 UnitId, int32 TeamId, const FRTCellId& Cell)
	{
		FRTHexCombatUnit U;
		U.UnitId = UnitId;
		U.TeamId = TeamId;
		U.Cell = Cell;
		U.bAlive = true;
		return U;
	}

	/** La voce di una fase, o `nullptr`: la timeline non porta fasi vuote, quindi cercarla e' il modo giusto. */
	const FRTPhasePreviewEntry* PhaseOf(const FRTPlanPreview& Preview, ERTResolutionPhase Phase)
	{
		for (const FRTPhasePreviewEntry& E : Preview.Phases)
		{
			if (E.Phase == Phase)
			{
				return &E;
			}
		}
		return nullptr;
	}
}

/**
 * 🔴 **Il ghost del Move percorre la STESSA rotta che il resolver percorrera'** — voce della DoD di `CP 11.5`:
 * *«origine, destinazione, celle bersaglio e area coincidono con quelle che userebbe il resolver: stesso A*,
 * stesso snapshot dell'autorita'. Nessuna seconda implementazione delle regole nel renderer»*.
 *
 * ## Perche' l'oracolo e' `BuildCompositeHexPath` e non un percorso scritto a mano
 *
 * Un test che confrontasse il ghost con una rotta calcolata **dal test** proverebbe che il test e la preview
 * sono d'accordo, non che la preview e il gioco lo sono. L'oracolo qui e' la funzione che riempie davvero
 * `ARTUnit::PlannedPath` (`RTPlayerController.cpp:1830`) e che `ResolveMovement` poi consuma: se la preview
 * ne usasse un'altra, anche equivalente oggi, questo test cadrebbe il giorno in cui le due divergono.
 *
 * ⚠️ **Si asserisce anche che la rotta NON sia banale** — piu' di due celle, e una deviazione — altrimenti
 * «uguale al resolver» sarebbe vero su qualunque implementazione sbagliata che azzecca partenza e arrivo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewGhostMatchesResolverPathTest,
	"RefactorTactics.Preview.GhostMatchesResolverPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewGhostMatchesResolverPathTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 4);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(/*UnitId=*/ 0, FRTCellId(-3, 0, 0), /*MoveBudget=*/ 12));
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(M, Units);

	// Due waypoint, cosi' il percorso composito ha una DEVIAZIONE: con un waypoint solo, «stesso A*» e
	// «stessa destinazione» sarebbero indistinguibili.
	FRTPlanPreviewInput Plan;
	Plan.UnitId = 0;
	Plan.PlannedWaypoints = { FRTCellId(0, -2, 0), FRTCellId(2, 0, 0) };
	Plan.MoveActionId = TEXT("Action.Move");

	const FRTPlanPreview Preview = URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, {});

	const FRTPhasePreviewEntry* Move = PhaseOf(Preview, ERTResolutionPhase::NormalMovement);
	if (!TestNotNull(TEXT("la timeline porta la fase Move"), Move))
	{
		return false;
	}

	// L'ORACOLO: la funzione del resolver, chiamata qui con gli stessi argomenti.
	const FRTHexPathResult Atteso =
		URTHexSimLibrary::BuildCompositeHexPath(Snapshot, /*UnitId=*/ 0, Plan.PlannedWaypoints);

	if (!TestTrue(TEXT("premessa: il percorso del resolver non e' banale (>2 celle)"), Atteso.Path.Num() > 2))
	{
		return false;
	}
	AddInfo(FString::Printf(TEXT("il resolver percorre %d celle da %s a %s"),
		Atteso.Path.Num(), *Atteso.Path[0].ToString(), *Atteso.Path.Last().ToString()));

	TestEqual(TEXT("il ghost percorre lo stesso numero di celle del resolver"),
		Move->PreviewPath.Num(), Atteso.Path.Num());
	bool bStessaRotta = Move->PreviewPath.Num() == Atteso.Path.Num();
	for (int32 I = 0; bStessaRotta && I < Atteso.Path.Num(); ++I)
	{
		bStessaRotta = Move->PreviewPath[I] == Atteso.Path[I];
	}
	// 🔑 Cella per cella e NELLO STESSO ORDINE: due rotte con le stesse celle in ordine diverso sono due
	// movimenti diversi, e il giocatore ne vedrebbe uno mentre ne subisce un altro.
	TestTrue(TEXT("e cella per cella, nello stesso ordine"), bStessaRotta);

	TestTrue(TEXT("l'origine del ghost e' la cella da cui il resolver parte"),
		Move->PreviewOrigin == Atteso.Path[0]);
	TestTrue(TEXT("e la destinazione e' quella a cui il resolver arriva"),
		Move->PreviewDestination == Atteso.Path.Last());

	// «Muoversi basta» per `Uncertain`: la definizione e' dell'enum, non di questo test.
	TestEqual(TEXT("una fase che sposta non puo' essere certa"),
		Move->Certainty, ERTIntentCertainty::Uncertain);

	return true;
}

/**
 * 🔴 **La reazione NON e' una quinta fase** — voce esplicita della DoD di `CP 11.5`:
 * *«`ReactionPreview` e' una struttura separata, non un elemento della lista delle fasi»*.
 *
 * ## Perche' e' una regola e non una preferenza di forma
 *
 * Le fasi sono un **ordinamento**: accadono una dopo l'altra, in un momento che la timeline dichiara. Una
 * reazione armata e' una **condizione**: non ha un posto nell'ordine, scatta se e quando il suo innesco si
 * verifica, e puo' non scattare affatto. Infilarla nella lista costringerebbe chi disegna a rispondere
 * «quando?» inventando una risposta.
 *
 * ⚠️ **Il test asserisce le due meta', e la seconda e' quella che si dimentica**: che la reazione NON sia
 * fra le fasi, e che ci sia lo stesso. Solo la prima passerebbe anche su un'implementazione che perde la
 * reazione per strada.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewReactionIsNotAPhaseTest,
	"RefactorTactics.Preview.ReactionIsNotAPhaseEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewReactionIsNotAPhaseTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 3);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(/*UnitId=*/ 0, FRTCellId(0, 0, 0), /*MoveBudget=*/ 6));
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(M, Units);

	FRTPlanPreviewInput Plan;
	Plan.UnitId = 0;
	Plan.bReactionArmed = true;
	Plan.ReactionProfileId = TEXT("Reaction.Overwatch");
	Plan.PrepActionId = TEXT("Action.Brace");

	const FRTPlanPreview Preview = URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, {});

	// ── La meta' che si dimentica: la reazione c'e'.
	TestTrue(TEXT("la reazione armata e' riportata"), Preview.Reaction.bArmed);
	TestEqual(TEXT("col profilo che il piano ha armato"),
		Preview.Reaction.ReactionProfileId, FName(TEXT("Reaction.Overwatch")));
	TestTrue(TEXT("e sorveglia dalla cella dell'unita'"),
		Preview.Reaction.WatchOrigin == FRTCellId(0, 0, 0));

	// ── La meta' dichiarata dalla DoD: non e' fra le fasi.
	//
	// ⚠️ Si controlla che NESSUNA fase porti l'azione della reazione, non solo che le fasi siano poche: un
	// conteggio passerebbe su una timeline che ha messo la reazione al posto del Prep.
	bool bReazioneFraLeFasi = false;
	for (const FRTPhasePreviewEntry& E : Preview.Phases)
	{
		if (E.ActionId == FName(TEXT("Reaction.Overwatch")))
		{
			bReazioneFraLeFasi = true;
		}
	}
	TestFalse(TEXT("la reazione NON compare fra le fasi"), bReazioneFraLeFasi);

	// E il Prep c'e' comunque: armare una reazione E' un'azione di preparazione, e quella una fase lo e'.
	const FRTPhasePreviewEntry* Prep = PhaseOf(Preview, ERTResolutionPhase::Preparation);
	if (TestNotNull(TEXT("il Prep resta una fase, e c'e'"), Prep))
	{
		TestEqual(TEXT("con la propria azione, non con quella della reazione"),
			Prep->ActionId, FName(TEXT("Action.Brace")));
	}

	// 🔑 **`Uncertain` per DEFINIZIONE dell'enum**, non per scelta di questa funzione: *«vale sempre per una
	// reazione armata, che per definizione attende un trigger che non decidiamo noi»*.
	TestEqual(TEXT("una reazione armata non puo' essere certa"),
		Preview.Reaction.Certainty, ERTIntentCertainty::Uncertain);

	return true;
}

/**
 * 🔑 **Le quattro fasi hanno origini DIVERSE, ed e' il difetto che questo checkpoint toglie.**
 *
 * L'anteprima che c'era mostrava la cella di destinazione e l'area del colpo, e le mostrava entrambe come se
 * partissero da dove l'unita' si trova adesso. Ma il Dash risolve PRIMA del Blast: chi pianifica una carica e
 * poi spara, sparera' da dove sara' arrivato.
 *
 * ⚠️ **Il test asserisce che le origini siano diverse fra loro**, non solo che abbiano un certo valore: con
 * la sola asserzione sui valori, un'implementazione che li azzeccasse per caso su questa board passerebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewPhasesChainTest,
	"RefactorTactics.Preview.PhaseOriginsFollowTheResolutionOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewPhasesChainTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 4);

	const FRTCellId Partenza(-2, 0, 0);
	const FRTCellId DopoScatto(0, 0, 0);
	const FRTCellId Bersaglio(2, 0, 0);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(/*UnitId=*/ 0, Partenza, /*MoveBudget=*/ 8));
	Units.Add(FRTHexSimUnit(/*UnitId=*/ 1, Bersaglio, /*MoveBudget=*/ 0));
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(M, Units);

	TArray<FRTHexCombatUnit> CombatUnits;
	CombatUnits.Add(MakePlanPreviewCombatUnit(0, /*TeamId=*/ 0, Partenza));
	CombatUnits.Add(MakePlanPreviewCombatUnit(1, /*TeamId=*/ 1, Bersaglio));

	FRTPlanPreviewInput Plan;
	Plan.UnitId = 0;
	Plan.bDashPlanned = true;
	Plan.bDashResolves = true;
	Plan.PlannedDashCell = DopoScatto;
	Plan.DashActionId = TEXT("Action.Dash");
	Plan.Blast.AttackerId = 0;
	Plan.Blast.bDashResolves = true;
	Plan.Blast.PlannedDashCell = DopoScatto;
	Plan.Blast.bHasAction = true;
	Plan.Blast.Shape = ERTAbilityShape::Single;
	Plan.Blast.RangeCells = 4;
	Plan.Blast.TargetId = 1;
	Plan.BlastActionId = TEXT("Action.Shoot");

	const FRTPlanPreview Preview = URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, CombatUnits);

	const FRTPhasePreviewEntry* Dash = PhaseOf(Preview, ERTResolutionPhase::FastMovement);
	const FRTPhasePreviewEntry* Blast = PhaseOf(Preview, ERTResolutionPhase::Attack);
	if (!TestNotNull(TEXT("la timeline porta il Dash"), Dash)) { return false; }
	if (!TestNotNull(TEXT("e il Blast"), Blast)) { return false; }

	TestTrue(TEXT("il Dash parte da dove l'unita' e' adesso"), Dash->PreviewOrigin == Partenza);
	TestTrue(TEXT("e arriva alla cella dichiarata"), Dash->PreviewDestination == DopoScatto);

	// 🔴 Il punto dell'intero checkpoint: il Blast NON parte da `Partenza`.
	TestTrue(TEXT("il Blast parte da DOPO lo scatto, non da dove l'unita' si trova"),
		Blast->PreviewOrigin == DopoScatto);
	TestTrue(TEXT("cioe' le due fasi hanno origini diverse"),
		Dash->PreviewOrigin != Blast->PreviewOrigin);

	// L'ordine della lista e' quello di RISOLUZIONE: il Dash prima del Blast, come ADR-0003 §3 stabilisce.
	int32 IndiceDash = INDEX_NONE;
	int32 IndiceBlast = INDEX_NONE;
	for (int32 I = 0; I < Preview.Phases.Num(); ++I)
	{
		if (Preview.Phases[I].Phase == ERTResolutionPhase::FastMovement) { IndiceDash = I; }
		if (Preview.Phases[I].Phase == ERTResolutionPhase::Attack) { IndiceBlast = I; }
	}
	TestTrue(TEXT("e la lista e' in ordine di risoluzione: Dash prima del Blast"),
		IndiceDash != INDEX_NONE && IndiceBlast != INDEX_NONE && IndiceDash < IndiceBlast);

	// Il bersaglio dichiarato e cio' che l'azione tocca sono campi distinti, e su un `Single` coincidono.
	TestEqual(TEXT("il bersaglio dichiarato e' una cella sola"), Blast->TargetCells.Num(), 1);
	TestTrue(TEXT("ed e' quella del bersaglio"),
		Blast->TargetCells.Num() == 1 && Blast->TargetCells[0] == Bersaglio);
	TestTrue(TEXT("l'area contiene la cella colpita"), Blast->AffectedCells.Contains(Bersaglio));

	return true;
}

/**
 * ⛔ **Il facing non lo inventa il renderer** — voce della DoD: *«derivato dalle regole e dagli intenti
 * autorizzati, mai inventato»*.
 *
 * 🔴 **E non si legge con `ReadFacingForConsumer`**, che e' la trappola di questo requisito: quella funzione
 * **scrive una voce di TurnLog**, cioe' dichiara che il RESOLVER ha letto quel facing. Un'anteprima che la
 * chiamasse inciderebbe nel registro canonico una lettura mai avvenuta in partita — esattamente il difetto
 * che `RTTurnLog.h` descrive con parole sue. La derivazione pura e' `FacingFromPath`, ed e' questa che il
 * test usa come oracolo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewFacingIsDerivedTest,
	"RefactorTactics.Preview.FacingComesFromTheRuleNotTheRenderer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewFacingIsDerivedTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 4);

	const FRTCellId Partenza(0, 0, 0);
	TArray<FRTHexSimUnit> Units;
	FRTHexSimUnit U(/*UnitId=*/ 0, Partenza, /*MoveBudget=*/ 8);
	// Un facing di partenza DIVERSO da quello che il movimento produrra': senza, «derivato» e «lasciato
	// com'era» sarebbero indistinguibili.
	U.Facing = ERTHexDirection::W;
	Units.Add(U);
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(M, Units);

	FRTPlanPreviewInput Plan;
	Plan.UnitId = 0;
	Plan.PlannedWaypoints = { FRTCellId(3, 0, 0) };
	Plan.MoveActionId = TEXT("Action.Move");

	const FRTPlanPreview Preview = URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, {});
	const FRTPhasePreviewEntry* Move = PhaseOf(Preview, ERTResolutionPhase::NormalMovement);
	if (!TestNotNull(TEXT("la timeline porta la fase Move"), Move)) { return false; }

	// L'ORACOLO: la stessa derivazione che il resolver usa, chiamata qui sul percorso del resolver.
	const FRTHexPathResult Rotta =
		URTHexSimLibrary::BuildCompositeHexPath(Snapshot, /*UnitId=*/ 0, Plan.PlannedWaypoints);
	const ERTHexDirection Atteso = URTFacingLibrary::FacingFromPath(Rotta.Path, ERTHexDirection::W);

	TestEqual(TEXT("il facing del ghost e' quello che la regola deriva dal percorso"), Move->Facing, Atteso);
	TestEqual(TEXT("e la voce dichiara da dove viene"),
		Move->FacingSource, ERTPreviewFacingSource::DerivedFromPath);

	// La premessa che rende il test falsificabile: il movimento ha DAVVERO cambiato l'orientamento.
	TestTrue(TEXT("premessa: il percorso cambia il facing, altrimenti il test non prova niente"),
		Atteso != ERTHexDirection::W);

	return true;
}

/**
 * 🔴 **Il rifiuto che l'anteprima mostra e' quello DICIBILE, non la classificazione interna.**
 *
 * `ERTHexTargetReason` distingue `OutOfRange`, `NoLineOfSight` e `NoMap`; il suo stesso header dichiara che
 * *«dice il vero anche quando il vero non e' dicibile»*. Portarlo in un view model che la presentazione
 * consuma darebbe al giocatore un rilevatore di presenze: la differenza fra due messaggi sarebbe **essa
 * stessa** il canale, contro [D-225].
 *
 * ⚠️ **Questo test non verifica una traduzione** — la traduzione avviene a monte, dove si sa cosa
 * l'osservatore osserva — ma che il tipo TRASPORTATO sia quello giusto, e che il livello di certezza scenda
 * quando il bersaglio e' rifiutato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewRefusalIsSpeakableTest,
	"RefactorTactics.Preview.RefusalIsTheSpeakableOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewRefusalIsSpeakableTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 3);

	const FRTCellId Partenza(0, 0, 0);
	const FRTCellId Mira(2, 0, 0);
	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(/*UnitId=*/ 0, Partenza, /*MoveBudget=*/ 4));
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshot(M, Units);

	TArray<FRTHexCombatUnit> CombatUnits;
	CombatUnits.Add(MakePlanPreviewCombatUnit(0, /*TeamId=*/ 0, Partenza));

	FRTPlanPreviewInput Plan;
	Plan.UnitId = 0;
	Plan.Blast.AttackerId = 0;
	Plan.Blast.bHasAction = true;
	Plan.Blast.Shape = ERTAbilityShape::Single;
	Plan.Blast.RangeCells = 4;
	Plan.Blast.bTargetsCell = true;
	Plan.Blast.TargetCell = Mira;
	Plan.BlastActionId = TEXT("Action.Shoot");
	Plan.BlastTargetRefusal = ERTTargetRefusal::Cover;

	const FRTPlanPreview Preview = URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, CombatUnits);
	const FRTPhasePreviewEntry* Blast = PhaseOf(Preview, ERTResolutionPhase::Attack);
	if (!TestNotNull(TEXT("la timeline porta il Blast"), Blast)) { return false; }

	TestEqual(TEXT("il rifiuto arriva nella forma dicibile al giocatore"),
		Blast->TargetRefusal, ERTTargetRefusal::Cover);

	// 🔑 Un bersaglio rifiutato non e' un piano `Predicted`: non produrra' niente.
	TestEqual(TEXT("e un bersaglio rifiutato abbassa la certezza"),
		Blast->Certainty, ERTIntentCertainty::Uncertain);

	// Il controllo NEGATIVO, che e' quello che rende il test utile: senza rifiuto, lo stesso piano sale.
	FRTPlanPreviewInput Accettato = Plan;
	Accettato.BlastTargetRefusal = ERTTargetRefusal::None;
	const FRTPlanPreview Buono = URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Accettato, CombatUnits);
	const FRTPhasePreviewEntry* BlastBuono = PhaseOf(Buono, ERTResolutionPhase::Attack);
	if (TestNotNull(TEXT("anche senza rifiuto il Blast c'e'"), BlastBuono))
	{
		TestEqual(TEXT("e senza rifiuto il piano e' previsto, non incerto"),
			BlastBuono->Certainty, ERTIntentCertainty::Predicted);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
