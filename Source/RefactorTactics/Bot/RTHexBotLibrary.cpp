#include "Bot/RTHexBotLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Turn/RTFacingLibrary.h" // CP 13.5: il facing d'arrivo si deriva con la regola, non a mano
#include "Turn/RTHexSimLibrary.h"

namespace
{
	/** Unita' di combattimento «leggera»: al calcolo della copertura servono cella e orientamento, nient'altro. */
	FRTHexCombatUnit CombatProbe(const FRTCellId& Cell, ERTHexDirection Facing)
	{
		FRTHexCombatUnit Probe;
		Probe.Cell = Cell;
		Probe.Facing = Facing;
		return Probe;
	}

	/**
	 * L'orientamento che il bot avra' a fine turno, derivato con lo STESSO ordine del resolver.
	 *
	 * Il movimento orienta verso l'ultimo passo; poi il targeting vince, perche' `D-020` dice che un'azione con
	 * bersaglio orienta l'unita' *prima* di risolvere — ed e' l'ordine che `RTTurnManager` applica davvero.
	 * Invertirlo qui farebbe valutare al bot un orientamento che non avra': non un'imprecisione di stima, ma un
	 * modello di un gioco diverso da quello che si gioca (`D-098`).
	 */
	ERTHexDirection ArrivalFacingOf(const FRTHexBotPlan& Plan, const FRTHexBotContext& Context,
		const FRTCellId& AimCell)
	{
		ERTHexDirection Facing = Context.SelfFacing;
		if (!(Plan.DestCell == Plan.FromCell))
		{
			Facing = URTFacingLibrary::FacingFromPath({ Plan.FromCell, Plan.DestCell }, Facing);
		}
		if (Plan.bHasAttack)
		{
			ERTHexDirection Towards = Facing;
			if (URTHexLibrary::DirectionTowards(Plan.DestCell, AimCell, Towards))
			{
				Facing = Towards;
			}
		}
		return Facing;
	}
}

int32 URTHexBotLibrary::ScorePlan(const URTHexMapAsset* Map, const FRTHexBotPlan& Plan, const FRTHexBotContext& Context)
{
	int32 Score = 0;

	// Dove il piano mira, e quindi come il bot sara' orientato: serve a entrambi i termini direzionali, e si
	// calcola una volta sola perche' e' la stessa figura in campo che li produce.
	const FRTCellId PlanAimCell = Context.Enemies.IsValidIndex(Plan.TargetIndex)
		? Context.Enemies[Plan.TargetIndex]
		: Plan.DestCell;
	const ERTHexDirection ArrivalFacing = ArrivalFacingOf(Plan, Context, PlanAimCell);

	// Focus-fire: danno inflitto, con bonus se il colpo uccide (danno >= HP+scudo del bersaglio).
	//
	// Il conto e' sulle celle che l'attacco INVESTE, non sul solo bersaglio: un'area di raggio 1 puo' prendere
	// piu' nemici — e, da quando il fuoco amico e' attivo di default, anche i compagni (#213). La geometria
	// viene dalla stessa `HexHitCells` che usa il resolver: il bot non stima una forma propria, legge quella.
	if (Plan.bHasAttack)
	{
		const FRTCellId AimCell = Context.Enemies.IsValidIndex(Plan.TargetIndex)
			? Context.Enemies[Plan.TargetIndex]
			: Plan.DestCell;
		const TArray<FRTCellId> HitCells = URTHexCombatLibrary::HexHitCells(
			Plan.Shape, Plan.DestCell, AimCell, Plan.RangeCells, Plan.AreaRadius);

		for (int32 I = 0; I < Context.Enemies.Num(); ++I)
		{
			if (!HitCells.Contains(Context.Enemies[I]))
			{
				continue;
			}
			Score += Context.WDamage * Plan.AttackDamage;

			// Il bersaglio mirato porta gli HP dichiarati dal piano (contratto gia' in uso); i nemici presi
			// "in piu'" dall'area li leggono dal contesto.
			const int32 Health = (I == Plan.TargetIndex)
				? Plan.TargetHealth
				: (Context.EnemyHealth.IsValidIndex(I) ? Context.EnemyHealth[I] : MAX_int32);
			if (Plan.AttackDamage >= Health)
			{
				Score += Context.WKill;
			}

			// ORIENTAMENTO, verso offensivo (CP 13.5, ADR-0005 §4a): un colpo che non arriva dall'arco frontale
			// ANNULLA la copertura del bersaglio (`EffectiveCoverReduction`, CP 16.2). Il bot preferisce quindi
			// il lato scoperto — e lo fa senza un peso proprio: il termine vale il danno che la direzione
			// aggiunge, cioe' la stessa grandezza con cui deve competere.
			//
			// ⚠️ La riduzione si CHIEDE alla funzione del gioco invece di ricalcolarla: la logica direzionale
			// vive li' dentro, e riscriverla qui sarebbe una seconda verita' destinata a divergere (`D-098`).
			// La differenza fra nominale ed effettiva e' esattamente cio' che l'orientamento ha scavalcato.
			//
			// ⚠️ **Guard resta fuori, ed e' deliberato**: anche lei viene annullata da un colpo posteriore
			// (`RTTurnManager`, `bGuardHolds`), ma al momento della pianificazione «il bersaglio si guardera'» e'
			// un INTENTO, cioe' informazione privata dell'altra squadra. Contarla renderebbe il bot piu' informato
			// del giocatore — l'opposto di cio' che CP 13.5 ha appena finito di garantire.
			if (Context.EnemyFacings.IsValidIndex(I))
			{
				const int32 Nominal = URTHexCombatLibrary::HexCoverDamageReduction(
					Map, Plan.DestCell, Context.Enemies[I], Plan.Shape);
				const int32 Effective = URTHexCombatLibrary::EffectiveCoverReduction(
					Map,
					CombatProbe(Plan.DestCell, ArrivalFacing),
					CombatProbe(Context.Enemies[I], Context.EnemyFacings[I]),
					Plan.Shape);
				Score += Context.WDamage * FMath::Max(0, Nominal - Effective);
			}
		}

		// Collaterale sugli alleati. PENALITA' PROPORZIONALE al danno, non veto (decisione 2026-08-09):
		// ferire il compagno per prendere due nemici resta una scelta legittima del bot, ma costa in
		// proporzione a quanto lo ferisce. Ucciderlo costa quanto vale un kill: e' il simmetrico di WKill.
		if (Plan.bFriendlyFire)
		{
			for (int32 I = 0; I < Context.Allies.Num(); ++I)
			{
				if (!HitCells.Contains(Context.Allies[I]))
				{
					continue;
				}
				Score -= Context.WAllyDamage * Plan.AttackDamage;

				const int32 AllyHp = Context.AllyHealth.IsValidIndex(I) ? Context.AllyHealth[I] : MAX_int32;
				if (Plan.AttackDamage >= AllyHp)
				{
					Score -= Context.WKill;
				}
			}
		}
	}

	// Minaccia sulla cella di destinazione + posizionamento rispetto al nemico piu' vicino.
	const int32 NumEnemies = FMath::Min(Context.Enemies.Num(), Context.EnemyRanges.Num());
	int32 MinDist = MAX_int32;
	for (int32 I = 0; I < NumEnemies; ++I)
	{
		const int32 Dist = URTHexLibrary::HexDistance(Plan.DestCell, Context.Enemies[I]);
		if (Dist <= Context.EnemyRanges[I]
			&& URTHexVisionLibrary::HasLineOfSight(Map, Context.Enemies[I], Plan.DestCell))
		{
			Score -= Context.WThreat; // sotto tiro E in linea di vista di questo nemico (la copertura protegge)

			// ORIENTAMENTO, verso difensivo: la copertura che questa cella offrirebbe CONTRO questo nemico non
			// vale se ci si arriva dandogli le spalle. Simmetrico al termine offensivo e con la stessa scala.
			//
			// ⚠️ **Non e' un termine inerte, ed e' l'unico punto in cui il bot legge la copertura.** Vale zero
			// dove la cella non ha copertura verso quel nemico — cioe' spesso — ma dove ce l'ha, distingue due
			// candidate che oggi il punteggio considera identiche. Il bot NON guadagna un bonus per stare al
			// coperto: guadagna il non buttarlo via, che e' la sola meta' che CP 13.5 chiede.
			//
			// La forma e' `Single`: l'azione con cui il nemico colpira' e' un suo intento, e ipotizzarne una
			// piu' larga renderebbe il bot timido su un attacco che nessuno ha dichiarato.
			const int32 CoverHere = URTHexCombatLibrary::HexCoverDamageReduction(
				Map, Context.Enemies[I], Plan.DestCell, ERTAbilityShape::Single);
			if (CoverHere > 0)
			{
				const int32 CoverKept = URTHexCombatLibrary::EffectiveCoverReduction(
					Map,
					CombatProbe(Context.Enemies[I], Context.EnemyFacings.IsValidIndex(I)
						? Context.EnemyFacings[I] : ERTHexDirection::E),
					CombatProbe(Plan.DestCell, ArrivalFacing),
					ERTAbilityShape::Single);
				Score -= Context.WDamage * FMath::Max(0, CoverHere - CoverKept);
			}
		}
		MinDist = FMath::Min(MinDist, Dist);
	}

	if (MinDist != MAX_int32)
	{
		if (Context.KiteStandoff > 0)
		{
			// Kiter: penalita' proporzionale a quanto si resta SOTTO la distanza di sicurezza.
			if (MinDist < Context.KiteStandoff)
			{
				Score -= Context.WKiteViolation * (Context.KiteStandoff - MinDist);
			}
		}
		else
		{
			// Mischia: penalita' proporzionale alla distanza (chiudere la distanza e' meglio).
			Score -= Context.WApproach * MinDist;
		}
	}

	// Elevazione: premia la quota alta della cella di destinazione (vantaggio di tiro/posizione).
	Score += Context.WElevation * Plan.DestCell.Layer;

	return Score;
}

FRTHexBotPlan URTHexBotLibrary::ChooseBestPlan(const URTHexMapAsset* Map, const TArray<FRTHexBotPlan>& Candidates,
	const FRTHexBotContext& Context)
{
	// Fallback: nessuna candidata -> resta a Origin (c'e' sempre un piano valido).
	FRTHexBotPlan Best;
	Best.DestCell = Context.Origin;
	// E da li' ci si arriva restando fermi: senza questa riga `FromCell` resterebbe `(0,0,0)` di default, e chi
	// derivasse il facing dal piano di ripiego leggerebbe uno spostamento dall'origine degli assi che non e'
	// mai avvenuto. Oggi nessuno lo fa — il fallback non passa da `ScorePlan` — ma il campo sarebbe gia' falso.
	Best.FromCell = Context.Origin;
	if (Candidates.Num() == 0)
	{
		return Best;
	}

	int32 BestScore = TNumericLimits<int32>::Lowest();
	int32 BestMove = MAX_int32;
	bool bHave = false;
	for (const FRTHexBotPlan& Cand : Candidates)
	{
		const int32 CandScore = ScorePlan(Map, Cand, Context);
		const int32 CandMove = URTHexLibrary::HexDistance(Cand.DestCell, Context.Origin);

		bool bBetter = !bHave || CandScore > BestScore;
		if (!bBetter && CandScore == BestScore)
		{
			// Tie-break ASSOLUTO: mossa minima da Origin, poi ordine stabile della cella (Layer, X, Y).
			// Non dipende dall'ordine di enumerazione -> permutare le candidate non cambia l'esito.
			if (CandMove < BestMove)
			{
				bBetter = true;
			}
			else if (CandMove == BestMove)
			{
				bBetter = URTHexLibrary::StableLess(Cand.DestCell, Best.DestCell);
			}
		}

		if (bBetter)
		{
			bHave = true;
			BestScore = CandScore;
			BestMove = CandMove;
			Best = Cand;
		}
	}
	return Best;
}

TArray<FRTHexBotPlan> URTHexBotLibrary::BuildCandidates(const FRTHexSnapshot& Snapshot, int32 UnitId,
	const FRTHexBotContext& Context)
{
	TArray<FRTHexBotPlan> Out;

	// Le celle raggiungibili hanno gia' rispettato budget, blocchi, occupanti e archi verticali: il bot non
	// rifa' pathfinding e non puo' proporre una mossa illegale.
	const TArray<FRTHexReachableCell> Reachable = URTHexSimLibrary::ReachableCells(Snapshot, UnitId);
	const int32 NumEnemies = FMath::Min3(Context.Enemies.Num(), Context.EnemyRanges.Num(), Context.EnemyHealth.Num());

	for (const FRTHexReachableCell& Cell : Reachable)
	{
		// Restare/spostarsi senza attaccare e' sempre un'opzione.
		FRTHexBotPlan Move;
		Move.DestCell = Cell.Cell;
		// Da dove ci si arriva: e' il predecessore che `ReachableCells` conserva, e da cui `ScorePlan` deriva
		// l'orientamento senza rifare il pathfinding per ogni candidata.
		Move.FromCell = Cell.FromCell;
		Out.Add(Move);

		if (Context.AttackRange <= 0 || Context.AttackDamage <= 0)
		{
			continue; // niente attacco disponibile: solo posizionamento
		}

		for (int32 I = 0; I < NumEnemies; ++I)
		{
			const FRTCellId& Enemy = Context.Enemies[I];
			// Gittata EFFETTIVA: il terreno (Fumo) la cappa. Senza, il bot propone un attacco che
			// CollectHexAttacks poi scarta — l'invariante "il bot non propone mosse illegali" vale anche qui.
			if (URTHexLibrary::HexDistance(Cell.Cell, Enemy)
				> URTTerrainLibrary::EffectiveTargetingRange(Snapshot.Map, Cell.Cell, Enemy, Context.AttackRange))
			{
				continue; // fuori gittata da questa cella
			}
			if (!URTHexVisionLibrary::HasLineOfSight(Snapshot.Map, Cell.Cell, Enemy))
			{
				continue; // linea di tiro interrotta
			}

			FRTHexBotPlan Attack;
			Attack.DestCell = Cell.Cell;
			Attack.bHasAttack = true;
			Attack.TargetIndex = I;
			Attack.AttackDamage = Context.AttackDamage;
			Attack.TargetHealth = Context.EnemyHealth[I];
			// La forma viaggia col piano: e' cio' che permette a ChooseBestPlan di confrontare candidate di
			// abilita' diverse senza che una prenda la geometria dell'altra.
			Attack.Shape = Context.AttackShape;
			Attack.AreaRadius = Context.AttackAreaRadius;
			Attack.RangeCells = Context.AttackRange;
			Attack.bFriendlyFire = Context.bAttackFriendlyFire;
			Attack.FromCell = Cell.FromCell;
			Out.Add(Attack);
		}
	}
	return Out;
}

FRTHexBotPlan URTHexBotLibrary::PlanUnit(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTHexBotContext& Context)
{
	return ChooseBestPlan(Snapshot.Map, BuildCandidates(Snapshot, UnitId, Context), Context);
}

void URTHexBotLibrary::ReservePlannedRoute(FRTHexSnapshot& Snapshot, int32 UnitId, const FRTCellId& DestCell)
{
	// La rotta AUTOREVOLE, cioe' quella che la fase Move calcolera' — non una stima fatta qui. Se la
	// prenotazione partisse da un percorso diverso da quello che verra' davvero percorso, riserverebbe celle
	// che nessuno usa e ne lascerebbe libere altre che due unita' si contenderanno lo stesso.
	const TArray<FRTCellId> Route = URTHexSimLibrary::FindPathForUnit(Snapshot, UnitId, DestCell).Path;

	// Percorso vuoto o di una sola cella = l'unita' resta dov'e': la sua cella e' gia' in `Occupancy`, e non
	// c'e' niente da prenotare.
	for (const FRTCellId& Cell : Route)
	{
		// `Add` sovrascriverebbe l'occupante di una cella gia' presa. Non deve mai succedere — la rotta viene
		// da `FindPathForUnit`, che le celle altrui le evita — ma la sovrascrittura sarebbe silenziosa e
		// cancellerebbe una prenotazione precedente, cioe' il difetto che questa funzione esiste per chiudere.
		if (!Snapshot.Occupancy.Contains(Cell))
		{
			Snapshot.Occupancy.Add(Cell, UnitId);
		}
	}
}

TArray<FRTHexBotPlan> URTHexBotLibrary::PlanTeam(const FRTHexSnapshot& Snapshot, const TArray<int32>& UnitIds,
	const TArray<FRTHexBotContext>& Contexts)
{
	TArray<FRTHexBotPlan> Out;

	// Copia locale: le prenotazioni servono a questa pianificazione e NON devono uscire di qui. Lo snapshot
	// del chiamante resta quello autorevole del turno — se le celle prenotate finissero dentro, la fase Move
	// troverebbe occupate delle celle su cui non c'e' nessuno.
	FRTHexSnapshot Working = Snapshot;

	const int32 Num = FMath::Min(UnitIds.Num(), Contexts.Num());
	Out.Reserve(Num);
	for (int32 I = 0; I < Num; ++I)
	{
		const FRTHexBotPlan Plan = PlanUnit(Working, UnitIds[I], Contexts[I]);
		Out.Add(Plan);
		ReservePlannedRoute(Working, UnitIds[I], Plan.DestCell);
	}

	return Out;
}

FRTCellId URTHexBotLibrary::BestKiteCell(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTCellId& Threat)
{
	// Le candidate arrivano da ReachableCells: budget, celle bloccate, occupanti e archi sono gia' applicati,
	// quindi la fuga non puo' proporre una mossa illegale (stessa disciplina di BuildCandidates).
	const TArray<FRTHexReachableCell> Reachable = URTHexSimLibrary::ReachableCells(Snapshot, UnitId);

	FRTCellId Best;
	int32 BestDistance = -1;
	int32 BestCost = MAX_int32;
	bool bFound = false;
	for (const FRTHexReachableCell& Candidate : Reachable)
	{
		const int32 Distance = URTHexLibrary::HexDistance(Candidate.Cell, Threat);
		// Piu' lontano dalla minaccia; a parita', il percorso piu' economico; a parita' ancora, l'ordine
		// stabile della cella -> l'esito non dipende dall'ordine di enumerazione.
		const bool bBetter =
			!bFound
			|| Distance > BestDistance
			|| (Distance == BestDistance && Candidate.Cost < BestCost)
			|| (Distance == BestDistance && Candidate.Cost == BestCost && URTHexLibrary::StableLess(Candidate.Cell, Best));
		if (bBetter)
		{
			Best = Candidate.Cell;
			BestDistance = Distance;
			BestCost = Candidate.Cost;
			bFound = true;
		}
	}

	if (!bFound)
	{
		// Nessuna cella raggiungibile (unita' assente dallo snapshot): resta dov'e' se la si ritrova.
		for (const FRTHexSimUnit& Unit : Snapshot.Units)
		{
			if (Unit.UnitId == UnitId) { return Unit.Cell; }
		}
	}
	return Best;
}

FString URTHexBotLibrary::DecideReactionResponse(const FRTReactionOpportunity& Opportunity)
{
	// Il primo `FIRE:` dell'elenco. L'ordine NON e' quello di arrivo: `BuildOverwatchTriggers` ordina i
	// bersagli per `UnitId` crescente prima di costruire le risposte, proprio perche' `AllowedResponses` sia
	// una funzione dello stato — quindi «il primo» qui e' una scelta stabile, non la prima che capita.
	for (const FString& Response : Opportunity.AllowedResponses)
	{
		if (URTReactionOpportunityLibrary::FireResponseTarget(Response) != INDEX_NONE)
		{
			return Response;
		}
	}

	// Nessun `FIRE` legale — la condizione dichiarata li ha filtrati tutti, o la finestra offre solo `HOLD`.
	// Si risponde `HOLD` esplicitamente invece di lasciare la stringa vuota: vuota significa «non ho
	// risposto», cioe' una scadenza, e il bot non e' scaduto — ha deciso, e non aveva altro da decidere.
	return URTReactionOpportunityLibrary::HoldResponse();
}
