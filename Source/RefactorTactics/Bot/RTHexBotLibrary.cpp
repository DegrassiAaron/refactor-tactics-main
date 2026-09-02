#include "Bot/RTHexBotLibrary.h"
#include "UObject/ObjectKey.h" // FObjectKey: identita' d'asset che sopravvive al riuso d'indirizzo
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Turn/RTFacingLibrary.h" // CP 13.5: il facing d'arrivo si deriva con la regola, non a mano
#include "Turn/RTHexSimLibrary.h"
#include "Pathfinding/RTHexPath.h" // ERTHexPathStatus: una prenotazione fallita si distingue da una vuota
#include "Pathfinding/RTHexPathLibrary.h" // `GraphNeighbors`: l'avvicinamento si misura in PASSI sul grafo (#1287 strato 1)
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
	 * 🔴 **BFS a peso uniforme, e NON `FindPath`.** La prima stesura leggeva `Path.Num() - 1` da
	 * `URTHexPathLibrary::FindPath`, che e' un A* sul COSTO (`GScore` accumula `TotalMoveCost`): quel numero
	 * e' la lunghezza del percorso a costo minimo, che su una mappa con terreni diversi **non e' il numero
	 * di passi**. Su `ArenaV01` non e' un caso limite — entrambe le porte sono `Rough`, costo 2 — e il
	 * commento prometteva «passi, non costo» consegnando l'opposto. Trovato in code review.
	 *
	 * ⚠️ **Sono passi e non costo per una ragione di scala.** Il costo somma il `MoveCost` del terreno e
	 * cambierebbe la taratura di `WApproach` su ogni mappa con fango o ghiaccio — cioe' bilanciamento, che
	 * ha la sua sede in #149 e non qui. Con i passi, su un campo a costo uniforme il numero coincide
	 * **esattamente** con `HexDistance` quando non ci sono ostacoli: ogni punteggio gia' pinnato resta
	 * quello di prima, e a muoversi e' solo cio' che sta dietro un muro.
	 *
	 * ⚠️ **Il grafo si percorre AL CONTRARIO.** `GraphNeighbors(C)` da' le celle raggiungibili DA `C`, e a
	 * servire qui e' `dist(X -> Goal)` per ogni `X`. Una BFS in avanti dal goal risponderebbe a
	 * `dist(Goal -> X)`, che coincide solo se ogni arco e' bidirezionale — vero per i vicini planari, **non
	 * garantito** per le transizioni, che `FRTHexEdge` esprime come archi orientati. Si costruisce quindi
	 * l'adiacenza inversa e da li' si parte: cosi' una rampa a senso unico da' il numero giusto invece di
	 * uno plausibile.
	 *
	 * ⚠️ **Il ripiego e' la distanza esagonale, e non e' neutro**: quando il cammino non esiste — grafo
	 * disconnesso, bersaglio murato — nessun avvicinamento e' possibile, e qualunque numero qui e' una
	 * finzione. Si sceglie quello di prima perche' e' l'unico che non introduce un comportamento nuovo in un
	 * caso che questo lavoro non ha misurato.
	 */
	/**
	 * ⚠️ **Il puntatore vale fino alla prossima chiamata**, e va letto subito: una chiamata con un goal
	 * nuovo puo' far ricrescere la mappa dei campi e spostarne i valori, e una con un grafo diverso li
	 * butta tutti. `ApproachSteps` lo consuma nella riga dopo, che e' l'uso per cui esiste.
	 */
	const TMap<FRTCellId, int32>* StepsToGoalField(const URTHexMapAsset* Map, const FRTCellId& Goal)
	{
		if (Map == nullptr)
		{
			return nullptr;
		}

		// Cache dei campi di distanza, per `(asset, revisione, goal)`.
		//
		// ⚠️ **Serve alla scala, non all'eleganza.** `ChooseBestPlan` chiama `ScorePlan` una volta per
		// candidata, e `BuildCandidates` emette un piano per ogni cella raggiungibile PIU' uno per ogni
		// coppia (cella, nemico): senza cache si ricalcolerebbe lo stesso campo un centinaio di volte per
		// unita' per turno.
		//
		// 🔴 **Una voce per NEMICO, non una sola** (`#1436`). `ScorePlan` chiama `ApproachSteps` dentro il
		// ciclo sui nemici, quindi il goal cambia a ogni chiamata: con una cache da una voce sola due nemici
		// se la sfrattano a vicenda e il tasso di successo crolla a **zero** — proprio nella configurazione
		// che la v0.1 spedisce, il 2v2. Il commento che stava qui dichiarava «una BFS per nemico» ed era
		// falso da quando l'avvicinamento si misura in passi.
		//
		// 🔴 **L'identita' dell'asset e' `FObjectKey`, NON il puntatore.** Un puntatore grezzo non distingue
		// due oggetti diversi che il GC ha messo allo stesso indirizzo: se `Goal` coincide e `Revision` pure,
		// la chiave regge e il bot valuta i piani sulla topologia della mappa PRECEDENTE. `FObjectKey` porta
		// indice **e serial number**, che e' cio' che rende due oggetti distinti anche a parita' d'indirizzo.
		//
		// ⚠️ Non era un rischio teorico nemmeno prima, ma [D-196] l'ha allargato: `MakeFlatArena` muoveva
		// `Revision` una volta per cella, quindi due arene di raggio diverso portavano numeri diversi e si
		// discriminavano **per caso**. Ora ogni arena piatta nasce con `Revision == 1`, e quel caso non
		// discrimina piu' niente.
		//
		// ⚠️ La chiave include `Revision` perche' la mappa cambia IN PARTITA — una porta che si apre, un
		// ponte che compare — ed e' esattamente il campo che l'asset espone per invalidare le cache.
		struct FCampi
		{
			FObjectKey Map;
			int32 Revision = -1;
			/** L'adiacenza INVERSA del grafo: dipende da `(Map, Revision)`, non dal goal. */
			TMap<FRTCellId, TArray<FRTCellId>> Inversa;
			/** Un campo di distanze per goal. Tenuti insieme perche' condividono l'invalidazione. */
			TMap<FRTCellId, TMap<FRTCellId, int32>> PerGoal;
		};
		static thread_local FCampi Cache;

		const FObjectKey MapKey(Map);
		if (Cache.Map != MapKey || Cache.Revision != Map->Revision)
		{
			// Grafo diverso: si ricostruisce l'adiacenza inversa UNA volta e si buttano tutti i campi.
			//
			// ⚠️ Le celle che BLOCCANO il movimento non entrano come sorgente: `GraphNeighbors` filtra la
			// DESTINAZIONE di un arco, non l'origine, quindi senza questa guardia un muro riceverebbe un
			// numero di passi finito e `ApproachSteps` da una cella murata risponderebbe come se fosse
			// percorribile. Oggi nessun chiamante ci arriva — `DestCell` viene da `ReachableCells` — ma il
			// campo sarebbe sbagliato per il primo che non pre-filtra.
			Cache.Map = MapKey;
			Cache.Revision = Map->Revision;
			Cache.Inversa.Reset();
			Cache.PerGoal.Reset();
			for (const FRTHexCellData& Cella : Map->Cells)
			{
				if (Cella.bBlocksMovement)
				{
					continue;
				}
				for (const TPair<FRTCellId, int32>& Arco : URTHexPathLibrary::GraphNeighbors(Map, Cella.Id))
				{
					Cache.Inversa.FindOrAdd(Arco.Key).Add(Cella.Id);
				}
			}
		}

		if (const TMap<FRTCellId, int32>* Gia = Cache.PerGoal.Find(Goal))
		{
			return Gia;
		}

		TMap<FRTCellId, int32>& Passi = Cache.PerGoal.Add(Goal);
		Passi.Add(Goal, 0);

		TArray<FRTCellId> Coda;
		Coda.Add(Goal);
		for (int32 I = 0; I < Coda.Num(); ++I)
		{
			const FRTCellId Corrente = Coda[I];
			const int32 Passo = Passi[Corrente] + 1;
			if (const TArray<FRTCellId>* Precedenti = Cache.Inversa.Find(Corrente))
			{
				for (const FRTCellId& Prec : *Precedenti)
				{
					if (!Passi.Contains(Prec))
					{
						Passi.Add(Prec, Passo);
						Coda.Add(Prec);
					}
				}
			}
		}

		return &Passi;
	}

	int32 ApproachSteps(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
	{
		if (From == To)
		{
			return 0;
		}
		if (const TMap<FRTCellId, int32>* Campo = StepsToGoalField(Map, To))
		{
			if (const int32* Passi = Campo->Find(From))
			{
				return *Passi;
			}
		}
		return URTHexLibrary::HexDistance(From, To);
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

	// «DA QUI POSSO INGAGGIARE» — lo specchio offensivo della minaccia qui sopra (#1300, D-185).
	//
	// La minaccia penalizza le celle da cui il NEMICO puo' colpirti; questo premia quelle da cui TU vedi
	// un contatto noto. Le due condizioni non sono la stessa cosa nemmeno quando la geometria e' simmetrica:
	// la minaccia chiede anche la gittata avversaria, e questo chiede solo la linea di tiro, perche' una
	// cella da cui si vede e' dove si potra' sparare **il turno prossimo** — le candidate di movimento
	// nascono con `AttackRange = 0` per costruzione, dato che il Move viene dopo il Blast.
	//
	// 🔴 **Il decadimento non e' un dettaglio del termine: e' il termine.** Un bonus posizionale fisso sulla
	// linea di tiro paga per GUARDARE, e la cella che massimizza il guardare e' una vedetta da cui non si
	// spara: lo stato assorbente di #1088 sotto un altro nome. Misurato il 2026-08-24, intero per intero,
	// non esiste alcun valore fisso che passi entrambi gli oracoli di parcheggio — l'arena generata cade da
	// 7, la mappa d'autore si sblocca da 11, e in mezzo sono rossi tutti e due.
	//
	// ⚠️ **Solo sui piani senza attacco**, e non e' una guardia difensiva: un piano che spara vale gia'
	// `WDamage * danno`, due ordini di grandezza sopra, quindi il bonus li' non cambierebbe nessun
	// ordinamento e renderebbe soltanto il termine piu' difficile da leggere.
	//
	// ⚠️ **La linea si chiede nel verso OFFENSIVO** (`DestCell -> nemico`), lo stesso di `BuildCandidates`.
	// Non e' pedanteria: `HexLine` costruisce la linea sul layer del TIRATORE, quindi fra piani diversi i
	// due versi non coincidono — su `DA_HexMap_Arena` ci sono 91 coppie asimmetriche su 2016, tutte fra
	// layer diversi, e dalla piattaforma L1 si vedono 64 celle su 64.
	if (!Plan.bHasAttack && Context.WEngage > 0)
	{
		for (const FRTCellId& Enemy : Context.Enemies)
		{
			if (URTHexVisionLibrary::HasLineOfSight(Map, Plan.DestCell, Enemy))
			{
				// Una volta sola: e' «posso ingaggiare», non «quanti ne vedo». Contare i nemici visti
				// renderebbe il termine una seconda misura del focus-fire, che ha gia' il suo peso.
				Score += FMath::Max(0, Context.WEngage - Context.WEngageDecay * Context.IdleTurns);
				break;
			}
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

int32 URTHexBotLibrary::ScoreReaction(const URTHexMapAsset* Map, const FRTActionDef& Def,
	const FRTHexBotContext& Context)
{
	// Un nemico CONOSCIUTO minaccia una cella se la sua portata la copre **e** la vede. Le due condizioni
	// insieme, come in `ScorePlan`: la copertura protegge, e un muro fra i due rende la minaccia inesistente
	// per il resolver quanto per il punteggio. La portata e' quella che la raccolta del contesto ha gia'
	// derivato, e la cella su un contatto incerto e' quella del RICORDO: qui non si guarda niente che la
	// squadra non abbia.
	auto Threatens = [Map, &Context](const FRTCellId& Cell, int32 EnemyIndex) -> bool
	{
		const int32 Reach = Context.EnemyRanges.IsValidIndex(EnemyIndex) ? Context.EnemyRanges[EnemyIndex] : 0;
		return URTHexLibrary::HexDistance(Context.Enemies[EnemyIndex], Cell) <= Reach
			&& URTHexVisionLibrary::HasLineOfSight(Map, Context.Enemies[EnemyIndex], Cell);
	};

	// 🔴 **Nessun `default:`, e non e' pedanteria.** `URTReactionLibrary::PassPointFor` lo vieta per lo
	// stesso enum e dice perche': «con un `default` il trigger nuovo compilerebbe, non scatterebbe mai, e il
	// suo test sul catalogo resterebbe verde — il modo esatto in cui i tre moduli di #505 sono rimasti
	// fermi». Qui il costo sarebbe lo stesso a un livello diverso: un trigger nuovo varrebbe zero per
	// sempre, in silenzio.
	switch (Def.ReactionTrigger)
	{
	case ERTReactionTrigger::HitByDirectAttack:
	{
		// Una parata vale quanto vale la minaccia su di ME. Conta i nemici che possono colpirmi, non quelli
		// che conosco: un nemico noto ma fuori portata, o dietro un muro, non e' un'occasione per una
		// reazione difensiva.
		int32 Threats = 0;
		for (int32 i = 0; i < Context.Enemies.Num(); ++i)
		{
			if (Threatens(Context.Origin, i)) { ++Threats; }
		}
		return Context.WThreat * Threats;
	}

	case ERTReactionTrigger::AllyHitByDirectAttack:
	{
		// Un'interposizione vale se c'e' qualcuno da coprire: un alleato dentro la PORTATA DELL'AZIONE
		// (fuori di li' non lo si raggiunge) e che almeno un nemico conosciuto puo' colpire.
		//
		// ⚠️ Si conta UNA volta per alleato e non una per coppia: il valore e' «quanti posso proteggere»,
		// non «quante traiettorie esistono». Due nemici sullo stesso alleato restano un alleato solo.
		//
		// ⚠️ **Limite dichiarato**: `FindInterceptableHit` pretende anche che la traiettoria
		// attaccante->intercettore sia libera, e quella qui non si misura — servirebbe sapere QUALE nemico
		// sparera'. Il punteggio sovrastima, e sovrastimare e' la direzione sicura: arma una reazione che
		// potrebbe non scattare, non ne perde una che sarebbe scattata.
		int32 Coverable = 0;
		for (const FRTCellId& Ally : Context.Allies)
		{
			if (URTHexLibrary::HexDistance(Context.Origin, Ally) > Def.RangeCells)
			{
				continue;
			}
			for (int32 i = 0; i < Context.Enemies.Num(); ++i)
			{
				if (Threatens(Ally, i)) { ++Coverable; break; }
			}
		}
		return Context.WThreat * Coverable;
	}

	case ERTReactionTrigger::AboutToBeDisplaced:
	case ERTReactionTrigger::AboutToReceiveControl:
		// 🔴 Zero, e dichiarato invece che nascosto: questi due risponderebbero a una spinta o a un
		// controllo, e la conoscenza autorizzata non porta le CAPACITA' nemiche — sa dove sono e quanto
		// arrivano lontano, non che cosa sanno fare. Un termine inventato per loro sarebbe l'onniscienza
		// rientrata dalla finestra, cioe' il difetto che il filtro di percezione esiste per togliere.
		return 0;

	case ERTReactionTrigger::CellBecameHazardous:
		// Zero per una ragione DIVERSA, e vale la pena tenerle distinte: qui il soggetto non e' un nemico ma
		// il TERRENO, che e' pubblico. Il termine sarebbe scrivibile — «la mia cella sta per diventare
		// pericolosa» — ma il contesto del bot non porta gli hazard, e aggiungerceli e' un'altra issue.
		// ⚠️ Finche' resta zero, `Reaction.HazardEscape` puo' vincere solo quando e' l'unica utilizzabile.
		return 0;

	case ERTReactionTrigger::None:
		// Non e' una reazione, o non ha ancora un trigger: dato incompleto, nessun valore.
		return 0;
	}

	return 0; // irraggiungibile: lo `switch` copre l'enum, e senza `default` un valore nuovo non compila
}

FRTReactionChoice URTHexBotLibrary::SelectReaction(const TArray<FRTReactionCandidate>& Candidates)
{
	// L'ordine e' TOTALE, e per questo permutare le candidate non cambia l'esito: punteggio decrescente,
	// poi il kit prima del loadout ([D-268] retrocede [D-220] a spareggio), poi l'indice piu' basso.
	// Senza l'ultima chiave due reazioni di kit a pari punteggio si scioglierebbero per ordine di
	// enumerazione, che e' l'accidente che [D-220] aveva gia' smesso di usare.
	const FRTReactionCandidate* Best = nullptr;
	for (const FRTReactionCandidate& Candidate : Candidates)
	{
		if (Candidate.AbilityIndex == INDEX_NONE)
		{
			continue;
		}
		if (!Best)
		{
			Best = &Candidate;
			continue;
		}
		if (Candidate.Score != Best->Score)
		{
			if (Candidate.Score > Best->Score) { Best = &Candidate; }
			continue;
		}
		if (Candidate.bFromKit != Best->bFromKit)
		{
			if (Candidate.bFromKit) { Best = &Candidate; }
			continue;
		}
		if (Candidate.AbilityIndex < Best->AbilityIndex)
		{
			Best = &Candidate;
		}
	}

	FRTReactionChoice Choice;
	if (!Best)
	{
		return Choice; // AbilityIndex = INDEX_NONE, DecidedBy = None
	}

	Choice.AbilityIndex = Best->AbilityIndex;
	Choice.Score = Best->Score;

	// 🔴 La ragione si calcola QUI, dove la regola vive. Dedurla dal chiamante — «se qualcuno pareggia,
	// allora ha deciso il kit» — e' cio' che fa dire «spareggio di kit» a una scelta decisa dall'indice:
	// due delle tre chiavi collassate in una sola etichetta, e [D-245] chiede l'opposto.
	int32 TiedAtTheTop = 0;
	bool bOtherOriginTied = false;
	for (const FRTReactionCandidate& Candidate : Candidates)
	{
		if (Candidate.AbilityIndex == INDEX_NONE || Candidate.Score != Best->Score)
		{
			continue;
		}
		++TiedAtTheTop;
		bOtherOriginTied = bOtherOriginTied || Candidate.bFromKit != Best->bFromKit;
	}

	Choice.DecidedBy = TiedAtTheTop <= 1
		? ERTReactionTieBreak::Utility
		: (bOtherOriginTied ? ERTReactionTieBreak::Kit : ERTReactionTieBreak::Index);
	return Choice;
}
