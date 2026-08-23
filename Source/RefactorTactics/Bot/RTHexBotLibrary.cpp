#include "Bot/RTHexBotLibrary.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Turn/RTFacingLibrary.h" // CP 13.5: il facing d'arrivo si deriva con la regola, non a mano
#include "Turn/RTHexSimLibrary.h"
#include "Pathfinding/RTHexPath.h" // ERTHexPathStatus: una prenotazione fallita si distingue da una vuota
#include "Pathfinding/RTHexPathLibrary.h" // la distanza di avvicinamento si misura PER CAMMINO (#1287 strato 1)
#include "RefactorTactics.h"       // LogRT

namespace
{
	/**
	 * Quanti PASSI separano davvero due celle sul grafo della mappa.
	 *
	 * 🔴 **E' lo strato 1 di #1287, che quel fix ha nominato e non ha toccato.** Il suo consuntivo scrive
	 * «La metrica mente: `(-1,1)` e `(1,-1)` distano 2, ma sono ai lati opposti di `(0,0)`, che blocca vista
	 * e passo» — e poi ha corretto gli strati 2 e 3, cioe' il dominio delle candidate e la meta della ricerca.
	 * Il risultato e' stato un ciclo di periodo due: la cella cieca resta la piu' vicina IN LINEA D'ARIA,
	 * quindi appena il filtro si spegne il bot ci torna. Misurato su `L_HexArena` il 2026-08-23: otto
	 * alternanze in dodici turni, e la partita che non si decide in quaranta.
	 *
	 * ⚠️ **Sono PASSI, non costo.** `TotalCost` somma il `MoveCost` del terreno, e usarlo qui cambierebbe la
	 * scala di `WApproach` su ogni mappa con fango o ghiaccio — cioe' bilanciamento, che ha la sua sede in
	 * #149 e non qui. Con i passi, su un campo aperto senza ostacoli il numero coincide **esattamente** con
	 * `HexDistance`: ogni punteggio gia' pinnato resta quello di prima, e a muoversi e' solo cio' che sta
	 * dietro un muro.
	 *
	 * ⚠️ **Il ripiego e' la distanza esagonale, e non e' neutro**: quando il cammino non esiste — grafo
	 * disconnesso, bersaglio murato — nessun avvicinamento e' possibile, e qualunque numero qui e' una
	 * finzione. Si sceglie quello di prima perche' e' l'unico che non introduce un comportamento nuovo in un
	 * caso che questo lavoro non ha misurato.
	 */
	int32 ApproachSteps(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
	{
		if (Map == nullptr)
		{
			return URTHexLibrary::HexDistance(From, To);
		}
		if (From == To)
		{
			return 0;
		}
		const FRTHexPathResult Path = URTHexPathLibrary::FindPath(Map, From, To);
		if (Path.Status != ERTHexPathStatus::Success || Path.Path.Num() < 2)
		{
			return URTHexLibrary::HexDistance(From, To);
		}
		return Path.Path.Num() - 1;
	}

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
		// ⚠️ **Due distanze, e la differenza e' di merito.** La MINACCIA e' geometrica — un proiettile
		// non cammina, e chi spara ha bisogno di gittata e linea di tiro, non di un percorso. L'AVVICINAMENTO
		// no: quello lo si paga in passi, ed e' la correzione dello strato 1 di #1287.
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
		MinDist = FMath::Min(MinDist, ApproachSteps(Map, Plan.DestCell, Context.Enemies[I]));
	}

	if (MinDist != MAX_int32)
	{
		if (Context.KiteStandoff > 0)
		{
			// Kiter: la distanza di sicurezza e' un OTTIMO, non un pavimento. Sotto costa
			// `WKiteViolation` per cella; sopra costa `WApproach` per cella, cioe' quanto costa a un
			// mischia stare lontano. A `MinDist == KiteStandoff` entrambi i termini valgono zero.
			//
			// 🔴 **Il ramo `else` non c'era, e lasciava un buco che l'invariante di #1088 non copriva.**
			// Sopra lo standoff nessun termine di distanza si applicava, quindi per un kiter l'elevazione
			// diventava l'UNICO termine posizionale: restare in quota batteva scendere con qualunque
			// `WElevation > 0`, e `WElevation * MaxLayer < WApproach` non proteggeva nulla — `WApproach`
			// non era nemmeno in gioco. Il conto su Phase (`PressureJet` portata 5 -> standoff 3), su una
			// mappa dove puo' salire: restare a L1 e distanza 4 valeva `+WElevation`, scendere valeva 0.
			//
			// ⚠️ Non toglie il kiting: allontanarsi OLTRE la distanza utile e' sempre stato inutile, e ora
			// costa. Il comportamento «resta a standoff e spara» e' esattamente il punto in cui entrambi i
			// termini si annullano.
			if (MinDist < Context.KiteStandoff)
			{
				Score -= Context.WKiteViolation * (Context.KiteStandoff - MinDist);
			}
			else
			{
				// 🔴 **La penalita' parte dallo STANDOFF, e costa al kiter due celle di gittata.** Il costo si
				// dichiara qui perche' e' una scelta, non una svista: Phase (`PressureJet` portata 5 ->
				// standoff 3) si avvicinera' fino a 3 invece di sparare da 5, cioe' dentro la portata 4 di
				// Gadget e Wraith. `DeriveKiteStandoff` dice che «chi colpisce da lontano ha qualcosa da
				// guadagnare a restare lontano», e questo termine gliene toglie una parte.
				//
				// ⚠️ **L'alternativa e' stata scritta e MISURATA, e riapre il difetto.** Facendo partire la
				// penalita' dalla portata invece che dallo standoff, la banda `[standoff, portata]` resta
				// piatta — e li' l'elevazione torna a essere l'unico termine posizionale, quindi il kiter si
				// parcheggia in quota esattamente come prima. Provato il 2026-08-23:
				// `ElevationNeverOutweighsClosingOneCell` e' tornato rosso con `scelto (0,0,L2)`.
				//
				// ∴ le due cose sono incompatibili con un termine di distanza: o il kiter tiene la gittata e
				// puo' parcheggiarsi, o scende e non si parcheggia. Si sceglie la seconda perche' un bot che
				// non conclude la partita e' un difetto, mentre due celle di gittata sono bilanciamento — e
				// il bilanciamento del bot ha la sua sede in #149, dove serve il banco di prova che D-102
				// richiede.
				Score -= Context.WApproach * (MinDist - Context.KiteStandoff);
			}
		}
		else
		{
			// Mischia: penalita' proporzionale alla distanza (chiudere la distanza e' meglio).
			Score -= Context.WApproach * MinDist;
		}
	}

	// Elevazione: premia la quota della cella di destinazione.
	//
	// 🔴 **QUI SI E' FORMATO LO STATO ASSORBENTE DI #1088, e la difesa e' UN NUMERO — non questa formula.**
	// Il termine compete con l'avvicinamento: finche' `WElevation * Layer` supera quello che `WApproach`
	// rende scendendo, restare in alto batte muoversi e il bot si parcheggia. Misurato su
	// `GeneratedTestArena` con `WElevation` 20: Riktor saliva sulla piattaforma al turno 3 e non scendeva
	// fino al 12 — restare valeva `+20 - 40 = -20` contro `-30` dello scendere.
	//
	// ⛔ **Non si prova a renderlo RELATIVO all'origine: sarebbe un no-op.** `Context.Origin` e' fisso per
	// l'intera chiamata di `ChooseBestPlan`, quindi `- WElevation * Origin.Layer` e' la stessa costante
	// sottratta a OGNI candidata: sposta tutti i punteggi e non cambia ne' l'argmax ne' il tie-break.
	// E' stato scritto, misurato e tolto il 2026-08-22 — con la forma relativa e `WElevation` 20 il
	// parcheggio si riproduce identico (`-40` contro `-50`, stesso ordine di `-20` contro `-30`).
	//
	// ⛔ **E non si condiziona a `bHasAttack`.** La ragione NON e' che il bot perderebbe la candidata con cui
	// sale — quella resta: `BuildCandidates` emette un piano di solo movimento per OGNI cella raggiungibile,
	// prima di qualunque controllo di gittata. E' che il bonus finirebbe quasi solo sui piani che NON si
	// muovono: i piani con attacco nascono in gran parte da `StaySnapshot` con `MoveBudget = 0`, cioe' dalla
	// cella attuale. La guardia avrebbe premiato lo stare fermi — la stessa asimmetria dello stato
	// assorbente, condizionata.
	//
	// ⚠️ **INVARIANTE: `WElevation * MaxLayer < WApproach`.** E' l'unica difesa reale, ed e' un vincolo
	// numerico: nessuna forma rende il difetto impossibile, perche' un bonus di posizione sufficientemente
	// grande batte sempre l'avvicinamento. Pinnato da `HexBot.ElevationNeverOutweighsClosingOneCell`, che
	// lo misura confrontando l'ESITO di `ChooseBestPlan` — non il punteggio di un piano isolato, che puo'
	// cambiare senza che l'ordinamento si muova.
	//
	// 🔴 **Quindi `WElevation` e' modificabile in editor a proprio rischio** (`PIE-BU2b` lo documenta come
	// workflow): alzarlo oltre l'invariante riapre lo stato assorbente, e nessun gate lo impedisce.
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

	// ⛔ **QUI STAVA IL «LIVELLO 2» DI #1287, e con la metrica corretta la sua premessa non esiste piu'.**
	//
	// Filtrava le candidate alle sole celle DA CUI SI VEDE, e solo quando il bot non poteva colpire **e non
	// vedeva gia' nessuno**. La sua giustificazione era scritta: «senza questo filtro il bot che ha perso il
	// tiro sceglie col punteggio geometrico, che misura la distanza in linea d'aria dal contatto noto: una
	// cella cieca a due passi batte una che vede a tre». Vero finche' l'avvicinamento si misurava in linea
	// d'aria; falso da quando si misura in PASSI (`ApproachSteps`), perche' una cella dietro un muro adesso
	// **e' lontana**, e il punteggio la scarta da solo.
	//
	// 🔴 **E la condizione lo rendeva un ciclo di periodo due.** Il commento affermava che restringere il
	// dominio «spezza l'oscillazione fra cerca e avvicinati senza introdurre stato», perche' «uscire dalla
	// ricerca non puo' riportare su una cella cieca: quelle non sono piu' candidate». Uscire dalla ricerca
	// significa pero' `bVedeGia == true`, e con quella condizione il filtro **e' spento**: le celle cieche
	// tornano candidate nello stesso istante. Misurato su `L_HexArena` il 2026-08-23 — Riktor fra
	// `(1,-1,L0)` e la piattaforma `(3,-3,L1)`, otto alternanze in dodici turni — e pinnato da
	// `Match.Autobattle.NobodyOscillatesOnTheAuthoredMap`.
	//
	// ⚠️ **Il livello 3 resta** (`PlanBots`, punto di osservazione): risponde a una domanda diversa — dove
	// andare quando NON si sa dove sia nessuno — e non e' quella che ha prodotto il ciclo.

	return Out;
}

FRTHexBotPlan URTHexBotLibrary::PlanUnit(const FRTHexSnapshot& Snapshot, int32 UnitId, const FRTHexBotContext& Context)
{
	return ChooseBestPlan(Snapshot.Map, BuildCandidates(Snapshot, UnitId, Context), Context);
}

TArray<FRTCellId> URTHexBotLibrary::ReservePlannedRoute(FRTHexSnapshot& Snapshot, int32 UnitId,
	const FRTCellId& DestCell)
{
	const FRTHexPathResult Found = URTHexSimLibrary::FindPathForUnit(Snapshot, UnitId, DestCell);

	// 🔴 **Una rotta che non esiste NON e' una prenotazione riuscita, e tacerlo riporta il difetto.** Se il
	// pathfinding fallisce — `NoPath`, `GoalInvalid`, `StartInvalid`, tetto di nodi — il ciclo qui sotto non
	// gira, la funzione non prenota nulla, e la compagna successiva trova la stessa destinazione libera: la
	// contesa di #1088, stavolta senza traccia. Si prenota allora almeno la DESTINAZIONE, che e' l'unica
	// cella su cui la contesa e' certa, e si dice che e' successo.
	if (Found.Path.Num() < 2)
	{
		// Restare fermi e' il caso NORMALE, non un fallimento: la cella dell'unita' e' gia' in `Occupancy` e
		// non c'e' nessuna rotta da prenotare. Si distingue confrontando la destinazione con la posizione.
		const FRTHexSimUnit* Self = Snapshot.Units.FindByPredicate(
			[UnitId](const FRTHexSimUnit& U) { return U.UnitId == UnitId; });
		const bool bStayingPut = Self && Self->Cell == DestCell;

		if (!bStayingPut)
		{
			UE_LOG(LogRT, Warning,
				TEXT("[RT] Prenotazione rotta u%d -> %s: nessun percorso (stato %d). Prenotata la sola destinazione."),
				UnitId, *DestCell.ToString(), static_cast<int32>(Found.Status));
			if (!Snapshot.Occupancy.Contains(DestCell))
			{
				Snapshot.Occupancy.Add(DestCell, UnitId);
			}
		}
		return TArray<FRTCellId>();
	}

	for (const FRTCellId& Cell : Found.Path)
	{
		// `Add` sovrascriverebbe l'occupante di una cella gia' presa. Non deve mai succedere — la rotta viene
		// da `FindPathForUnit`, che le celle altrui le evita — ma la sovrascrittura sarebbe silenziosa e
		// cancellerebbe una prenotazione precedente, cioe' il difetto che questa funzione esiste per chiudere.
		// Quindi si NOTIFICA invece di ingoiare: se questa riga compare, l'invariante e' rotta a monte.
		if (const int32* Occupant = Snapshot.Occupancy.Find(Cell))
		{
			if (*Occupant != UnitId)
			{
				UE_LOG(LogRT, Warning,
					TEXT("[RT] Prenotazione rotta u%d: la cella %s risulta gia' di u%d — invariante rotta a monte."),
					UnitId, *Cell.ToString(), *Occupant);
			}
			continue;
		}
		Snapshot.Occupancy.Add(Cell, UnitId);
	}

	return Found.Path;
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

	// Nessun `FIRE` legale — la condizione dichiarata li ha filtrati tutti, o la finestra non ne offre.
	// Si risponde esplicitamente invece di lasciare la stringa vuota: vuota significa «non ho risposto»,
	// cioe' una scadenza, e il bot non e' scaduto — ha deciso, e non aveva altro da decidere.
	//
	// 🔴 **`SafeResponse` e non la costante `HoldResponse()`, dal 2026-08-19.** Era `HOLD` fisso, e con
	// l'arrivo del `Brace` di [D-047] quella costante e' diventata una risposta **illegale**: una finestra di
	// `Brace` offre `{Hold Ground, SIDESTEP}`, dove `HOLD` non compare. Il resolver la rifiutava con
	// `HoldRejected` — l'esito riservato a una risposta *stale o inventata* — quindi ogni bot in `Brace` con
	// un profilo si vedeva registrare come illegale una risposta perfettamente ragionevole. In v0.1, che e'
	// **2v2 offline vs bot**, quello era il caso normale e non un caso limite.
	//
	// ⚠️ Questo era un **omonimo**: la stessa correzione era gia' stata applicata ai sei ripieghi di
	// `ARTTurnManager::AskReactionDecision` e non era stata cercata qui, che e' il settimo produttore della
	// stessa scelta. `grep` della forma corretta, non la memoria di dove si e' scritto.
	return URTReactionOpportunityLibrary::SafeResponse(Opportunity);
}
