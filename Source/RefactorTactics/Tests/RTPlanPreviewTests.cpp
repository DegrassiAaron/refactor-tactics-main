// La TIMELINE del piano: una voce per fase, e la reazione che fase non e' — `CP 11.5` (#172).
//
// Questi test coprono il PRODUTTORE e non il wiring, come gia' `RTBlastPreviewTests.cpp`. La distinzione
// conta: i `Preview.*` piu' vecchi verificano che `ARTHexMapActor` conservi le celle che gli si passano — un
// setter — e quelle celle gliele calcola il test. Nessuno di quei verdi dice che il gioco le calcola giuste.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexMapActor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h" // TActorIterator: gli Actor del mondo si contano guardando il mondo
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
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshotOmniscient(M, Units);

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
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshotOmniscient(M, Units);

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
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshotOmniscient(M, Units);

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
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshotOmniscient(M, Units);

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
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshotOmniscient(M, Units);

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

/**
 * ⛔ **Il ghost NON anticipa l'intento avversario** — voce della DoD: *«nessun planned facing avversario, in
 * nessun DTO che raggiunga il client»*.
 *
 * ## Perché questo test non duplica `Facing.IntentIsTeamFiltered`
 *
 * Quello verifica il **filtro**: che `FilterForTeam` tolga ciò che un osservatore non ha diritto di sapere.
 * Questo verifica che qui un filtro **non serva**, perché il dato avversario non entra mai — e la ragione è
 * la **firma**. `MakePlanPreview` prende UN `FRTPlanPreviewInput`, cioè il piano di una sola unità: non
 * riceve la lista dei piani, quindi non può guardarne un altro neanche per sbaglio. È lo stesso argomento
 * con cui `ClassifyPlan` è stata scritta — *«prende UN intento, non la lista, e la firma è il punto»*.
 *
 * ⚠️ **La prova è che le unità avversarie SONO nella scena** — occupano celle, compaiono in `CombatUnits`,
 * sono bersagliabili — e ciò nonostante nessuna voce della timeline parla di loro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewNoEnemyIntentTest,
	"RefactorTactics.Preview.TimelineCarriesOnlyThePlanningUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewNoEnemyIntentTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 4);

	const FRTCellId Mia(-2, 0, 0);
	const FRTCellId Nemica(2, 0, 0);

	TArray<FRTHexSimUnit> Units;
	FRTHexSimUnit Io(/*UnitId=*/ 0, Mia, /*MoveBudget=*/ 8);
	Io.Facing = ERTHexDirection::E;
	FRTHexSimUnit Lui(/*UnitId=*/ 1, Nemica, /*MoveBudget=*/ 8);
	// Un facing avversario ben riconoscibile: se finisse nella timeline, si vedrebbe.
	Lui.Facing = ERTHexDirection::NW;
	Units.Add(Io);
	Units.Add(Lui);
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshotOmniscient(M, Units);

	TArray<FRTHexCombatUnit> CombatUnits;
	CombatUnits.Add(MakePlanPreviewCombatUnit(0, /*TeamId=*/ 0, Mia));
	CombatUnits.Add(MakePlanPreviewCombatUnit(1, /*TeamId=*/ 1, Nemica));

	FRTPlanPreviewInput Plan;
	Plan.UnitId = 0;
	Plan.PlannedWaypoints = { FRTCellId(0, 0, 0) };
	Plan.MoveActionId = TEXT("Action.Move");
	Plan.Blast.AttackerId = 0;
	Plan.Blast.bHasAction = true;
	Plan.Blast.Shape = ERTAbilityShape::Single;
	Plan.Blast.RangeCells = 5;
	Plan.Blast.TargetId = 1;
	Plan.BlastActionId = TEXT("Action.Shoot");

	const FRTPlanPreview Preview = URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, CombatUnits);

	if (!TestTrue(TEXT("premessa: la timeline non e' vuota"), Preview.Phases.Num() > 0))
	{
		return false;
	}

	// 🔴 Ogni voce parla della PROPRIA unita', nessuna dell'avversaria.
	bool bSoloMia = true;
	for (const FRTPhasePreviewEntry& E : Preview.Phases)
	{
		bSoloMia = bSoloMia && E.UnitId == 0;
	}
	TestTrue(TEXT("ogni voce della timeline parla della sola unita' che pianifica"), bSoloMia);
	TestTrue(TEXT("e la reazione, quando c'e', pure"),
		!Preview.Reaction.bArmed || Preview.Reaction.UnitId == 0);

	// ⚠️ **La cella dell'avversario COMPARE**, ed e' giusto: e' il bersaglio dichiarato, cioe' informazione
	// che il giocatore ha gia' perche' l'ha scelta lui. Cio' che non deve comparire e' il suo INTENTO — dove
	// sta andando, dove guardera'. Il test distingue le due cose invece di vietare la cella.
	const FRTPhasePreviewEntry* Blast = PhaseOf(Preview, ERTResolutionPhase::Attack);
	if (TestNotNull(TEXT("il Blast c'e'"), Blast))
	{
		TestTrue(TEXT("e il bersaglio dichiarato e' la cella nemica, che il giocatore ha scelto"),
			Blast->TargetCells.Contains(Nemica));
	}

	// 🔑 Nessuna voce porta il facing dell'avversario. `NW` e' il suo, e nessuna fase puo' averlo per caso:
	// tutte le derivazioni di questo piano guardano verso EST.
	bool bFacingNemicoTrapelato = false;
	for (const FRTPhasePreviewEntry& E : Preview.Phases)
	{
		if (E.Facing == ERTHexDirection::NW)
		{
			bFacingNemicoTrapelato = true;
		}
	}
	TestFalse(TEXT("nessuna voce porta il facing dell'avversario"), bFacingNemicoTrapelato);

	return true;
}

/**
 * 🔴 **I ghost sono ISTANZE IN POOL, non Actor, e non costano un fotogramma** — voce della DoD di `CP 11.5`:
 * *«budget di presentazione: pooling di mesh/decal, nessun Actor persistente per preview, aggiornamento a
 * frequenza limitata (non ogni Tick)»*.
 *
 * ⚠️ **Le tre promesse si misurano separatamente**, perché si rompono separatamente: si può fare pooling e
 * spawnare comunque un Actor per il ghost «principale», e si può evitare gli Actor e ridisegnare tutto a
 * ogni Tick. Qui si contano gli Actor del mondo, le istanze del componente e la loro sopravvivenza a un
 * `Tick` che non viene mai chiamato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewGhostsArePooledTest,
	"RefactorTactics.Preview.GhostsArePooledNotSpawned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewGhostsArePooledTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }
	if (GEngine)
	{
		FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
		Ctx.SetCurrentWorld(World);
	}

	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 3);
	ARTHexMapActor* HexMap = World->SpawnActorDeferred<ARTHexMapActor>(
		ARTHexMapActor::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("actor mappa"), HexMap))
	{
		if (GEngine) { GEngine->DestroyWorldContext(World); }
		World->DestroyWorld(false);
		return false;
	}
	HexMap->MapAsset = M;
	HexMap->FinishSpawning(FTransform::Identity);

	// Quanti Actor esistono PRIMA: è il numero che nessun ghost deve muovere.
	int32 AttoriPrima = 0;
	for (TActorIterator<AActor> It(World); It; ++It) { ++AttoriPrima; }

	// Una timeline con tre fasi, costruita a mano: qui si misura il CONSUMO, non la derivazione.
	FRTPlanPreview Timeline;
	for (int32 I = 0; I < 3; ++I)
	{
		FRTPhasePreviewEntry E;
		E.Phase = ERTResolutionPhase::NormalMovement;
		E.UnitId = 0;
		E.PreviewDestination = FRTCellId(I, 0, 0);
		E.Certainty = ERTIntentCertainty::Predicted;
		Timeline.Phases.Add(E);
	}

	HexMap->SetPlanPreview(Timeline);

	// ── Pooling: una istanza per fase, su un componente solo.
	TestEqual(TEXT("un ghost per fase, in istanze"), HexMap->PlanGhostInstanceCount(), 3);
	TestEqual(TEXT("e la mappatura cella->ghost e' lunga uguale"),
		HexMap->GetPlanGhostCells().Num(), 3);

	// ── Nessun Actor: il conteggio del mondo non si e' mosso.
	int32 AttoriDopo = 0;
	for (TActorIterator<AActor> It(World); It; ++It) { ++AttoriDopo; }
	TestEqual(TEXT("nessun Actor e' stato spawnato per i ghost"), AttoriDopo, AttoriPrima);

	// ── Non ogni Tick: posare i ghost non ACCENDE il Tick dell'actor.
	//
	// ⌫ **Una prima stesura rileggeva `PlanGhostInstanceCount()` una seconda volta, e non misurava niente**:
	// fra le due letture non c'era un `Tick`, un avanzamento di frame, niente. Qualunque implementazione che
	// passava la prima passava la seconda per costruzione, e la terza delle tre promesse del budget — quella
	// che la PR dichiara «misurate separatamente» — non era misurata affatto.
	//
	// 🔑 La proprietà che la DoD chiede è questa: `DrawPlanningPreview` riemette le proprie
	// `DrawDebugLine` a ogni fotogramma e per farlo tiene ACCESO il tick dell'actor; i ghost no, perché posare
	// un'istanza è definitivo. Si misura l'interruttore, non il conteggio.
	const bool bTickPrima = HexMap->IsActorTickEnabled();
	HexMap->SetPlanPreview(Timeline);
	TestEqual(TEXT("posare i ghost non accende il Tick dell'actor"),
		HexMap->IsActorTickEnabled(), bTickPrima);
	TestEqual(TEXT("e le istanze restano quelle"), HexMap->PlanGhostInstanceCount(), 3);

	// ── L'annullamento: una timeline vuota li toglie.
	HexMap->SetPlanPreview(FRTPlanPreview());
	TestEqual(TEXT("una timeline vuota toglie i ghost"), HexMap->PlanGhostInstanceCount(), 0);
	TestEqual(TEXT("e svuota la mappatura con loro"), HexMap->GetPlanGhostCells().Num(), 0);

	if (GEngine) { GEngine->DestroyWorldContext(World); }
	World->DestroyWorld(false);
	return true;
}

// =========================================================================================================
// L'azione principale nella SUA fase e nel SUO posto ([D-470], #3554)
// =========================================================================================================
namespace
{
	// La scena comune: Partenza (-2,0,0) col facing a W, scatto a (0,0,0) (verso E), Move a (0,2,0) (verso SE), e
	// un nemico vivo in (3,0,0), che da entrambe le origini sta a E. Tre direzioni diverse, cosi' «ereditato dal
	// Move», «ereditato dallo scatto», «girato verso il bersaglio» e «quello di adesso» si distinguono.
	const FRTCellId GD470Partenza(-2, 0, 0);
	const FRTCellId GD470DopoScatto(0, 0, 0);
	const FRTCellId GD470DopoIlMove(0, 2, 0);
	const FRTCellId GD470Bersaglio(3, 0, 0);

	FRTHexSnapshot MakeD470Snapshot(URTHexMapAsset* Map)
	{
		TArray<FRTHexSimUnit> Units;
		FRTHexSimUnit U(/*UnitId=*/ 0, GD470Partenza, /*MoveBudget=*/ 8);
		U.Facing = ERTHexDirection::W;
		Units.Add(U);
		Units.Add(FRTHexSimUnit(/*UnitId=*/ 1, GD470Bersaglio, /*MoveBudget=*/ 0));
		return URTHexSimLibrary::MakeSnapshotOmniscient(Map, Units);
	}

	TArray<FRTHexCombatUnit> MakeD470CombatUnits()
	{
		return { MakePlanPreviewCombatUnit(0, /*TeamId=*/ 0, GD470Partenza),
			MakePlanPreviewCombatUnit(1, /*TeamId=*/ 1, GD470Bersaglio) };
	}

	/** Uno scatto che si applica, un Move dopo, e un'azione principale della fase data sul nemico vivo. */
	FRTPlanPreviewInput MakeD470Plan(ERTResolutionPhase Fase, const TCHAR* ActionId, bool bConMove)
	{
		FRTPlanPreviewInput Plan;
		Plan.UnitId = 0;
		Plan.bDashPlanned = true;
		Plan.bDashResolves = true;
		Plan.PlannedDashCell = GD470DopoScatto;
		Plan.DashActionId = TEXT("Action.Dodge");
		Plan.Blast.AttackerId = 0;
		Plan.Blast.bDashResolves = true;
		Plan.Blast.PlannedDashCell = GD470DopoScatto;
		Plan.Blast.Phase = Fase;
		Plan.Blast.bHasAction = true;
		Plan.Blast.Shape = ERTAbilityShape::Single;
		Plan.Blast.RangeCells = 6;
		Plan.Blast.TargetId = 1;
		Plan.BlastActionId = ActionId;
		if (bConMove)
		{
			Plan.PlannedWaypoints = { GD470DopoIlMove };
			Plan.MoveActionId = TEXT("Action.Move");
		}
		return Plan;
	}

	/** L'indice della prima voce che soddisfa `Pred`, o `INDEX_NONE`: l'ordine della lista e' cio' che si prova. */
	template <typename TPred>
	int32 D470IndexOf(const FRTPlanPreview& Preview, TPred Pred)
	{
		for (int32 I = 0; I < Preview.Phases.Num(); ++I)
		{
			if (Pred(Preview.Phases[I])) { return I; }
		}
		return INDEX_NONE;
	}

	int32 D470IndexOfPhase(const FRTPlanPreview& Preview, ERTResolutionPhase Phase)
	{
		return D470IndexOf(Preview, [Phase](const FRTPhasePreviewEntry& E) { return E.Phase == Phase; });
	}

	/** La voce dell'azione principale si cerca per `ActionId`: la fase e' proprio cio' che il test verifica. */
	int32 D470IndexOfAction(const FRTPlanPreview& Preview, FName ActionId)
	{
		return D470IndexOf(Preview, [ActionId](const FRTPhasePreviewEntry& E) { return E.ActionId == ActionId; });
	}
}

/**
 * **Un'azione principale di `Preparation` sta PRIMA dello scatto, sulla cella corrente** ([D-470], #3554).
 *
 * `ResolvePrep` la risolve prima del Dash: `Action.Overwatch` si arma da dove l'unita' e' adesso, col facing di
 * adesso. ⏱️ *Fino a [D-470] la timeline la chiamava `Attack` e la metteva fra lo scatto e il Move.*
 *
 * ⚠️ Il piano ha uno scatto E un Move, e il bersaglio e' un'unita' viva fuori dal facing corrente: senza lo scatto
 * «prima» e «dopo» coinciderebbero, e senza il bersaglio una rotazione prevista per sbaglio non si vedrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewPreparationBeforeTheDashTest,
	"RefactorTactics.Preview.PreparationActionResolvesBeforeTheDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewPreparationBeforeTheDashTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 4);
	const FRTHexSnapshot Snapshot = MakeD470Snapshot(M);
	const FRTPlanPreviewInput Plan = MakeD470Plan(ERTResolutionPhase::Preparation, TEXT("Action.Overwatch"),
		/*bConMove=*/ true);
	const FRTPlanPreview Preview = URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, MakeD470CombatUnits());

	const int32 IndiceAzione = D470IndexOfAction(Preview, TEXT("Action.Overwatch"));
	const int32 IndiceScatto = D470IndexOfPhase(Preview, ERTResolutionPhase::FastMovement);
	if (!TestTrue(TEXT("la timeline porta la voce dell'azione"), IndiceAzione != INDEX_NONE)
		|| !TestTrue(TEXT("premessa: e lo scatto"), IndiceScatto != INDEX_NONE)
		|| !TestTrue(TEXT("premessa: e il Move"), D470IndexOfPhase(Preview, ERTResolutionPhase::NormalMovement) != INDEX_NONE))
	{
		return false;
	}
	const FRTPhasePreviewEntry& Azione = Preview.Phases[IndiceAzione];
	TestTrue(TEXT("premessa: lo scatto porta l'unita' altrove"),
		Preview.Phases[IndiceScatto].PreviewDestination == GD470DopoScatto && !(GD470DopoScatto == GD470Partenza));

	TestEqual(TEXT("la voce porta la sua fase, Preparation"), Azione.Phase, ERTResolutionPhase::Preparation);
	TestEqual(TEXT("e nessuna voce si chiama Attack"), D470IndexOfPhase(Preview, ERTResolutionPhase::Attack),
		static_cast<int32>(INDEX_NONE));
	TestTrue(TEXT("sta PRIMA dello scatto"), IndiceAzione < IndiceScatto);
	TestTrue(TEXT("parte dalla cella corrente"), Azione.PreviewOrigin == GD470Partenza);
	TestTrue(TEXT("e il ghost sta sulla cella corrente"), Azione.PreviewDestination == GD470Partenza);
	TestEqual(TEXT("col facing di adesso: il Prep non gira chi agisce"), Azione.Facing, ERTHexDirection::W);
	TestEqual(TEXT("ed e' quello autorevole"), Azione.FacingSource, ERTPreviewFacingSource::Authoritative);
	return true;
}

/**
 * **Un'azione principale di `Environment` sta DOPO il Move, col ghost dove sara' l'unita'** ([D-470], #3554).
 *
 * `ResolveEnvironment` la risolve nel Cleanup, dopo il Move. L'origine resta quella di mira, la cella da cui e'
 * stata pianificata ([D-464]); il ghost va dove l'unita' sara' allora. Senza un Move, dove lo scatto la lascia.
 * `ResolveEnvironment` non gira chi agisce: il facing e' quello che il Move le lascia.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewEnvironmentAfterTheMoveTest,
	"RefactorTactics.Preview.EnvironmentActionResolvesAfterTheMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewEnvironmentAfterTheMoveTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 4);
	const FRTHexSnapshot Snapshot = MakeD470Snapshot(M);

	// CON IL MOVE
	{
		const FRTPlanPreviewInput Plan = MakeD470Plan(ERTResolutionPhase::Environment, TEXT("Action.Electrify"),
			/*bConMove=*/ true);
		const FRTPlanPreview Preview =
			URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, MakeD470CombatUnits());

		const int32 IndiceAzione = D470IndexOfAction(Preview, TEXT("Action.Electrify"));
		const int32 IndiceScatto = D470IndexOfPhase(Preview, ERTResolutionPhase::FastMovement);
		const int32 IndiceMove = D470IndexOfPhase(Preview, ERTResolutionPhase::NormalMovement);
		if (!TestTrue(TEXT("la timeline porta la voce dell'azione"), IndiceAzione != INDEX_NONE)
			|| !TestTrue(TEXT("premessa: e lo scatto"), IndiceScatto != INDEX_NONE)
			|| !TestTrue(TEXT("premessa: e il Move"), IndiceMove != INDEX_NONE))
		{
			return false;
		}
		const FRTPhasePreviewEntry& Azione = Preview.Phases[IndiceAzione];
		const FRTPhasePreviewEntry& Move = Preview.Phases[IndiceMove];
		// Le premesse che rendono distinguibili le asserzioni qui sotto.
		TestTrue(TEXT("premessa: il Move arriva a (0,2,0)"), Move.PreviewDestination == GD470DopoIlMove);
		TestEqual(TEXT("premessa: e lascia l'unita' verso SE"), Move.Facing, ERTHexDirection::SE);
		TestEqual(TEXT("premessa: lo scatto la lascia verso E"), Preview.Phases[IndiceScatto].Facing,
			ERTHexDirection::E);

		TestEqual(TEXT("la voce porta la sua fase, Environment"), Azione.Phase, ERTResolutionPhase::Environment);
		TestEqual(TEXT("e nessuna voce si chiama Attack"), D470IndexOfPhase(Preview, ERTResolutionPhase::Attack),
			static_cast<int32>(INDEX_NONE));
		TestTrue(TEXT("sta DOPO il Move"), IndiceAzione > IndiceMove && IndiceMove > IndiceScatto);
		TestTrue(TEXT("l'origine e' quella di mira, la cella corrente"), Azione.PreviewOrigin == GD470Partenza);
		TestTrue(TEXT("e il ghost sta dove sara' l'unita', dopo il Move"),
			Azione.PreviewDestination == Move.PreviewDestination);
		TestEqual(TEXT("col facing che il Move le lascia: il Cleanup non gira chi agisce"), Azione.Facing,
			ERTHexDirection::SE);
		TestEqual(TEXT("ereditato dalla fase prima"), Azione.FacingSource,
			ERTPreviewFacingSource::InheritedFromPreviousPhase);
	}

	// SENZA IL MOVE — l'unita' resta dove lo scatto la lascia.
	{
		const FRTPlanPreviewInput Plan = MakeD470Plan(ERTResolutionPhase::Environment, TEXT("Action.Electrify"),
			/*bConMove=*/ false);
		const FRTPlanPreview Preview =
			URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, MakeD470CombatUnits());

		const int32 IndiceAzione = D470IndexOfAction(Preview, TEXT("Action.Electrify"));
		const int32 IndiceScatto = D470IndexOfPhase(Preview, ERTResolutionPhase::FastMovement);
		if (!TestTrue(TEXT("senza Move: la timeline porta la voce dell'azione"), IndiceAzione != INDEX_NONE)
			|| !TestTrue(TEXT("senza Move: premessa, e lo scatto"), IndiceScatto != INDEX_NONE))
		{
			return false;
		}
		const FRTPhasePreviewEntry& Azione = Preview.Phases[IndiceAzione];
		TestTrue(TEXT("senza Move: la voce sta dopo lo scatto"), IndiceAzione > IndiceScatto);
		TestTrue(TEXT("senza Move: il ghost sta dove lo scatto la lascia"),
			Azione.PreviewDestination == GD470DopoScatto);
		TestTrue(TEXT("senza Move: e l'origine resta la cella corrente"), Azione.PreviewOrigin == GD470Partenza);
		TestEqual(TEXT("senza Move: col facing dello scatto"), Azione.Facing, ERTHexDirection::E);
	}
	return true;
}

/**
 * **Le azioni del Blast restano dove sono, con la fase vera** ([D-470], #3554): `Control` e `Attack` fra lo
 * scatto e il Move, dall'origine dopo lo scatto ([D-464]), girate verso un bersaglio vivo come le gira
 * `CollectAttackIntents`. ⏱️ *Fino a [D-470] anche un `Control` si chiamava `Attack`.*
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewBlastActionsKeepTheirPlaceTest,
	"RefactorTactics.Preview.BlastActionsKeepTheirPlaceAndPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewBlastActionsKeepTheirPlaceTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 4);
	const FRTHexSnapshot Snapshot = MakeD470Snapshot(M);

	for (const ERTResolutionPhase Fase : { ERTResolutionPhase::Control, ERTResolutionPhase::Attack })
	{
		const FString Nome = Fase == ERTResolutionPhase::Control ? TEXT("Control") : TEXT("Attack");
		const FRTPlanPreviewInput Plan = MakeD470Plan(Fase, TEXT("Action.Strike"), /*bConMove=*/ true);
		const FRTPlanPreview Preview =
			URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, MakeD470CombatUnits());

		const int32 IndiceAzione = D470IndexOfAction(Preview, TEXT("Action.Strike"));
		const int32 IndiceScatto = D470IndexOfPhase(Preview, ERTResolutionPhase::FastMovement);
		const int32 IndiceMove = D470IndexOfPhase(Preview, ERTResolutionPhase::NormalMovement);
		if (!TestTrue(FString::Printf(TEXT("%s: la timeline porta la voce, lo scatto e il Move"), *Nome),
			IndiceAzione != INDEX_NONE && IndiceScatto != INDEX_NONE && IndiceMove != INDEX_NONE))
		{
			return false;
		}
		const FRTPhasePreviewEntry& Azione = Preview.Phases[IndiceAzione];
		TestEqual(FString::Printf(TEXT("%s: la voce porta la sua fase"), *Nome), Azione.Phase, Fase);
		TestTrue(FString::Printf(TEXT("%s: sta fra lo scatto e il Move"), *Nome),
			IndiceScatto < IndiceAzione && IndiceAzione < IndiceMove);
		TestTrue(FString::Printf(TEXT("%s: parte da dopo lo scatto"), *Nome),
			Azione.PreviewOrigin == GD470DopoScatto);
		TestTrue(FString::Printf(TEXT("%s: e il ghost sta li'"), *Nome),
			Azione.PreviewDestination == GD470DopoScatto);
		TestEqual(FString::Printf(TEXT("%s: il Blast gira chi agisce verso il bersaglio vivo"), *Nome),
			Azione.FacingSource, ERTPreviewFacingSource::DerivedFromPath);
	}
	return true;
}

// =========================================================================================================
// Il ghost di uno scatto che non si applica ([D-474], #3565)
// =========================================================================================================

/**
 * **Uno scatto che non si applica lascia il ghost sulla cella corrente** ([D-474], #3565).
 *
 * `bDashResolves` falso vuol dire che il resolver rifiutera' lo scatto per fatti gia' noti in pianificazione
 * (`ARTUnit::PlannedDashMoves()`, [D-471]). La voce Dash resta, perche' lo scatto e' pianificato; ma il ghost
 * sta dove sara' l'unita', e la voce si comporta come una rotta rifiutata. ⏱️ *Fino a [D-474] il ghost stava
 * sulla cella dello scatto in ogni caso.*
 *
 * ⚠️ Il CONTROLLO e' lo stesso piano con lo scatto che si applica: arriva, si gira e porta la rotta. Senza, «sta
 * sulla cella corrente» sarebbe vero anche per una mappa in cui lo scatto non puo' andare da nessuna parte.
 * ⬜ La certezza della voce non si asserisce: [D-474] la lascia aperta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewDashThatDoesNotApplyTest,
	"RefactorTactics.Preview.DashThatDoesNotApplyLeavesTheGhostHere",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewDashThatDoesNotApplyTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 4);
	const FRTCellId Partenza(-2, 0, 0);
	const FRTCellId CellaScatto(0, 0, 0);
	const FRTCellId DopoIlMove(0, 2, 0);

	TArray<FRTHexSimUnit> Units;
	FRTHexSimUnit U(/*UnitId=*/ 0, Partenza, /*MoveBudget=*/ 8);
	U.Facing = ERTHexDirection::W;
	Units.Add(U);
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshotOmniscient(M, Units);

	const auto Piano = [&](bool bSiApplica)
	{
		FRTPlanPreviewInput Plan;
		Plan.UnitId = 0;
		Plan.bDashPlanned = true;
		Plan.bDashResolves = bSiApplica;
		Plan.PlannedDashCell = CellaScatto;
		Plan.DashActionId = TEXT("Action.Dodge");
		Plan.PlannedWaypoints = { DopoIlMove };
		Plan.MoveActionId = TEXT("Action.Move");
		return URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, {});
	};

	// CONTROLLO — lo scatto si applica: il ghost arriva alla sua cella, girato dalla rotta.
	const FRTPlanPreview Applicato = Piano(/*bSiApplica=*/ true);
	const FRTPhasePreviewEntry* ScattoVero = PhaseOf(Applicato, ERTResolutionPhase::FastMovement);
	if (!TestNotNull(TEXT("controllo: la timeline porta lo scatto che si applica"), ScattoVero)) { return false; }
	TestTrue(TEXT("controllo: il ghost arriva alla cella dello scatto"), ScattoVero->PreviewDestination == CellaScatto);
	TestTrue(TEXT("controllo: e la voce porta la rotta"), ScattoVero->PreviewPath.Num() >= 2);
	TestEqual(TEXT("controllo: e l'unita' si gira verso E"), ScattoVero->Facing, ERTHexDirection::E);

	// IL CUORE — lo scatto non si applica: la voce resta, il ghost sta dove sara' l'unita'.
	const FRTPlanPreview Negato = Piano(/*bSiApplica=*/ false);
	const FRTPhasePreviewEntry* Scatto = PhaseOf(Negato, ERTResolutionPhase::FastMovement);
	if (!TestNotNull(TEXT("la voce Dash resta: lo scatto e' pianificato"), Scatto)) { return false; }
	TestTrue(TEXT("il ghost sta sulla cella corrente"), Scatto->PreviewDestination == Partenza);
	TestTrue(TEXT("da dove parte"), Scatto->PreviewOrigin == Partenza);
	TestEqual(TEXT("nessun percorso: la fase non ne percorre uno"), Scatto->PreviewPath.Num(), 0);
	TestEqual(TEXT("e il facing di adesso: l'unita' non si gira"), Scatto->Facing, ERTHexDirection::W);
	TestEqual(TEXT("ereditato, come per una rotta rifiutata"), Scatto->FacingSource,
		ERTPreviewFacingSource::InheritedFromPreviousPhase);

	// E il Move parte da dove l'unita' e' rimasta: lo era gia' prima di [D-474], e qui si guarda che resti cosi'.
	const FRTPhasePreviewEntry* Move = PhaseOf(Negato, ERTResolutionPhase::NormalMovement);
	if (TestNotNull(TEXT("la timeline porta il Move"), Move))
	{
		TestTrue(TEXT("il Move parte dalla cella corrente"), Move->PreviewOrigin == Partenza);
	}
	return true;
}

// =========================================================================================================
// Il Move senza percorso tiene il facing del Blast (#3566)
// =========================================================================================================

/**
 * **Un Move che non sposta l'unita' le lascia il facing del Blast** (#3566), come il resolver.
 *
 * Il Blast gira chi agisce verso un bersaglio vivo (`CollectAttackIntents`, [D-020]); `ResolveMovement` salta chi
 * non si e' mosso — *«chi non si e' mosso non deriva nessun orientamento»*. ⏱️ *Fino a #3566 la voce Move senza
 * percorso prendeva il facing di PRIMA del Blast.*
 *
 * ⚠️ Il bersaglio sta a SE, fuori dal facing di partenza (W): senza, «girata dal Blast» e «come prima» sarebbero
 * indistinguibili. Il CONTROLLO e' un Move vero, che il facing lo deriva dal proprio percorso come prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanPreviewMoveWithoutAPathKeepsTheBlastFacingTest,
	"RefactorTactics.Preview.MoveWithoutAPathKeepsTheBlastFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanPreviewMoveWithoutAPathKeepsTheBlastFacingTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakePlanPreviewMap(/*Radius=*/ 4);
	const FRTCellId Partenza(-2, 0, 0);
	const FRTCellId Bersaglio(-2, 2, 0);

	TArray<FRTHexSimUnit> Units;
	FRTHexSimUnit U(/*UnitId=*/ 0, Partenza, /*MoveBudget=*/ 8);
	U.Facing = ERTHexDirection::W;
	Units.Add(U);
	Units.Add(FRTHexSimUnit(/*UnitId=*/ 1, Bersaglio, /*MoveBudget=*/ 0));
	const FRTHexSnapshot Snapshot = URTHexSimLibrary::MakeSnapshotOmniscient(M, Units);
	const TArray<FRTHexCombatUnit> CombatUnits = { MakePlanPreviewCombatUnit(0, /*TeamId=*/ 0, Partenza),
		MakePlanPreviewCombatUnit(1, /*TeamId=*/ 1, Bersaglio) };

	const auto Piano = [&](const FRTCellId& Waypoint)
	{
		FRTPlanPreviewInput Plan;
		Plan.UnitId = 0;
		Plan.Blast.AttackerId = 0;
		Plan.Blast.bHasAction = true;
		Plan.Blast.Shape = ERTAbilityShape::Single;
		Plan.Blast.RangeCells = 6;
		Plan.Blast.TargetId = 1;
		Plan.BlastActionId = TEXT("Action.Strike");
		Plan.PlannedWaypoints = { Waypoint };
		Plan.MoveActionId = TEXT("Action.Move");
		return URTPlanPreviewLibrary::MakePlanPreview(Snapshot, Plan, CombatUnits);
	};

	// IL CUORE — un waypoint fuori dalla mappa: il percorso e' rifiutato, e l'unita' non si sposta.
	const FRTPlanPreview Rifiutato = Piano(FRTCellId(9, 0, 0));
	const FRTPhasePreviewEntry* Colpo = PhaseOf(Rifiutato, ERTResolutionPhase::Attack);
	const FRTPhasePreviewEntry* Move = PhaseOf(Rifiutato, ERTResolutionPhase::NormalMovement);
	if (!TestNotNull(TEXT("la timeline porta il Blast"), Colpo) || !TestNotNull(TEXT("e il Move"), Move))
	{
		return false;
	}
	TestEqual(TEXT("premessa: il Blast gira l'unita' verso il bersaglio, a SE"), Colpo->Facing, ERTHexDirection::SE);
	TestEqual(TEXT("premessa: il percorso del Move e' rifiutato"), Move->PreviewPath.Num(), 0);
	TestEqual(TEXT("il Move senza percorso tiene il facing del Blast"), Move->Facing, ERTHexDirection::SE);
	TestEqual(TEXT("ereditato dalla fase prima"), Move->FacingSource, ERTPreviewFacingSource::InheritedFromPreviousPhase);

	// CONTROLLO — un Move vero deriva il facing dal proprio percorso, come prima.
	const FRTPlanPreview Vero = Piano(FRTCellId(0, 0, 0));
	const FRTPhasePreviewEntry* MoveVero = PhaseOf(Vero, ERTResolutionPhase::NormalMovement);
	if (TestNotNull(TEXT("controllo: la timeline porta il Move vero"), MoveVero))
	{
		TestTrue(TEXT("controllo: il percorso c'e'"), MoveVero->PreviewPath.Num() >= 2);
		TestEqual(TEXT("controllo: e il facing viene dal percorso, verso E"), MoveVero->Facing, ERTHexDirection::E);
		TestEqual(TEXT("controllo: derivato dal percorso"), MoveVero->FacingSource,
			ERTPreviewFacingSource::DerivedFromPath);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
