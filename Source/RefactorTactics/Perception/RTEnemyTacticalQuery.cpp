#include "Perception/RTEnemyTacticalQuery.h"

#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTHexCombatLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTHexSimLibrary.h"
#include "Turn/RTMovementActionLibrary.h"

namespace
{
	/**
	 * Le voci della vista in ordine CANONICO per `StableUnitId`.
	 *
	 * 🔴 **Non e' una comodita': e' l'unico punto in cui l'ordine dell'ingresso smette di contare.**
	 * `FRTKnowledgeView` esce ordinata da `ViewForTeam`, ma la firma accetta una vista composta da
	 * chiunque, e l'occupazione qui sotto si costruisce con `TMap::Add` — che a parita' di cella lascia
	 * vincere l'ULTIMO inserito. Senza questo ordinamento due permutazioni della stessa conoscenza
	 * darebbero occupanti diversi e regioni diverse (invariante #3).
	 */
	TArray<FRTKnowledgeEntry> CanonicalEntries(const FRTKnowledgeView& View)
	{
		TArray<FRTKnowledgeEntry> Entries = View.Entries;
		Entries.Sort([](const FRTKnowledgeEntry& A, const FRTKnowledgeEntry& B)
			{ return A.StableUnitId < B.StableUnitId; });
		return Entries;
	}

	/**
	 * Le unita' che l'osservatore ha diritto di conoscere, come ingredienti di uno snapshot DERIVATO.
	 *
	 * ⛔ **Un'unita' che l'osservatore non vede non compare, quindi non blocca.** Non e' una semplificazione:
	 * se bloccasse, il buco nella regione disegnata sarebbe una deduzione affidabile sulla sua posizione.
	 *
	 * Solo il soggetto porta un budget: gli altri sono ostacoli, e un budget su un ostacolo non ha
	 * significato.
	 */
	TArray<FRTHexSimUnit> AuthorizedUnits(const TArray<FRTKnowledgeEntry>& Entries, int32 SubjectId, int32 SubjectBudget)
	{
		TArray<FRTHexSimUnit> Units;
		Units.Reserve(Entries.Num());
		for (const FRTKnowledgeEntry& E : Entries)
		{
			Units.Add(FRTHexSimUnit(E.StableUnitId, E.Cell,
				E.StableUnitId == SubjectId ? FMath::Max(0, SubjectBudget) : 0));
		}
		return Units;
	}

	/**
	 * Il raggiungibile del soggetto con il budget indicato, calcolato su uno snapshot costruito qui.
	 *
	 * ⚠️ **Lo snapshot derivato non esce da questa funzione**, ed e' la stessa avvertenza che
	 * `URTHexSimLibrary::ReachableCellsAfterPlan` porta gia': una fotografia costruita per rispondere a una
	 * domanda ipotetica MENTE a chi la ricevesse senza sapere di essere in un'ipotesi.
	 *
	 * Il calcolo resta di `ReachableCells`: qui non si riscrive nessuna regola di movimento.
	 */
	TArray<FRTCellId> ReachableWithBudget(const URTHexMapAsset* Map, const TArray<FRTKnowledgeEntry>& Entries,
		int32 SubjectId, int32 Budget)
	{
		const FRTHexSnapshot Derived = URTHexSimLibrary::MakeSnapshot(Map, AuthorizedUnits(Entries, SubjectId, Budget));

		TArray<FRTCellId> Cells;
		for (const FRTHexReachableCell& R : URTHexSimLibrary::ReachableCells(Derived, SubjectId))
		{
			Cells.Add(R.Cell);
		}
		return Cells;
	}

	/** Occupazione autorizzata per le primitive lineari. A parita' di cella vince l'`StableUnitId` minore,
	 *  la stessa regola che `MakeSnapshot` applica: due sorgenti di occupazione non devono divergere. */
	TMap<FRTCellId, int32> AuthorizedOccupancy(const TArray<FRTKnowledgeEntry>& Entries, int32 SubjectId)
	{
		TMap<FRTCellId, int32> Occupancy;
		for (const FRTKnowledgeEntry& E : Entries)
		{
			if (E.StableUnitId == SubjectId) { continue; } // il soggetto non blocca se stesso
			if (!Occupancy.Contains(E.Cell)) { Occupancy.Add(E.Cell, E.StableUnitId); }
		}
		return Occupancy;
	}

	/**
	 * La portata DICHIARATA di un'azione.
	 *
	 * E' l'espressione che `ARTTurnManager` e `ARTPlayerController` usano gia' per lo scatto: se l'azione e'
	 * catalogata comanda il catalogo, altrimenti il dato dell'asset. Riscriverla diversamente qui
	 * produrrebbe un'anteprima che promette una portata che il resolver non concedera'.
	 */
	int32 DeclaredRange(const URTActionData* Action)
	{
		return Action->Def.ActionId.IsNone() ? Action->RangeCells : Action->Def.RangeCells;
	}

	/**
	 * Le celle investite dalle azioni di SLOT PRINCIPALE valutate da `Origin`.
	 *
	 * La LOS filtra la cella MIRATA, non ogni cella investita: e' la mira che deve essere possibile, e da li'
	 * l'impronta la decide `HexHitCells`. Per una forma ad area i due insiemi differiscono, ed e' il punto —
	 * mostrare i centri bersagliabili direbbe al giocatore una portata che non subira'.
	 */
	void AddMainSlotFootprint(const URTHexMapAsset* Map, const FRTCellId& Origin, const URTHeroData* Hero,
		TSet<FRTCellId>& Out)
	{
		for (const TObjectPtr<URTActionData>& Ptr : Hero->Actions)
		{
			const URTActionData* Action = Ptr.Get();
			if (!Action) { continue; }
			if (!URTCatalogLibrary::TakesMainSlot(Action->Def)) { continue; }
			if (Action->bSelfTarget) { continue; } // supporto su di se': non minaccia nessuno

			const int32 Range = FMath::Max(0, Action->RangeCells);
			if (Range <= 0) { continue; }

			for (const FRTCellId& Aim : URTHexLibrary::HexArea(Origin, Range))
			{
				if (Aim == Origin) { continue; }
				if (!Map->FindCell(Aim)) { continue; } // fuori mappa: non e' una cella che si possa mirare
				if (!URTHexVisionLibrary::HasLineOfSight(Map, Origin, Aim)) { continue; }

				for (const FRTCellId& Hit : URTHexCombatLibrary::HexHitCells(
					Action->Shape, Origin, Aim, Range, FMath::Max(0, Action->AreaRadius)))
				{
					Out.Add(Hit);
				}
			}
		}
	}
}

bool URTEnemyTacticalQueryLibrary::RegionsFor(const URTHexMapAsset* Map, const FRTKnowledgeView& View,
	int32 SubjectStableUnitId, const URTHeroData* Hero, FRTEnemyTacticalRegions& OutRegions)
{
	// L'uscita si azzera SEMPRE e per prima: un chiamante che ignorasse il valore di ritorno non deve
	// poter leggere la risposta di una domanda precedente come se fosse quella di adesso.
	OutRegions = FRTEnemyTacticalRegions();

	// Fail-closed sugli ingredienti: senza mappa non si sa cosa ci sia davanti, senza profilo non si sa
	// cosa il soggetto possa fare. Nessuno dei due si inventa.
	if (!Map || !Hero) { return false; }

	const FRTKnowledgeEntry* Entry = URTKnowledgeViewLibrary::FindEntry(View, SubjectStableUnitId);
	if (!Entry) { return false; } // ⛔ nessuna voce, nessuna regione — e nessun motivo che dica perche'

	// Il profilo deve essere QUELLO della voce autorizzata. E' cio' che impedisce a un chiamante di
	// interrogare un eroe con la scheda di un altro per allargare la regione.
	if (Hero->HeroId != Entry->HeroId) { return false; }

	const TArray<FRTKnowledgeEntry> Entries = CanonicalEntries(View);
	const FRTCellId Origin = Entry->Cell;

	// --- Raggiungibile ------------------------------------------------------------------------------------
	// `MovePoints` e' la BASELINE di catalogo, mai `FRTHexSimUnit::MoveBudget`: un `Action.Slow` che
	// l'osservatore non vede non deve restringere la regione, o la regione diventerebbe la spia dello status.
	TSet<FRTCellId> Reachable;
	for (const FRTCellId& C : ReachableWithBudget(Map, Entries, SubjectStableUnitId, Hero->MovePoints))
	{
		Reachable.Add(C);
	}

	// --- Minaccia immediata: dalla sola cella corrente ----------------------------------------------------
	TSet<FRTCellId> Immediate;
	AddMainSlotFootprint(Map, Origin, Hero, Immediate);

	// --- Origini di mobilita' rapida, e impronta della carica ---------------------------------------------
	const TMap<FRTCellId, int32> Occupancy = AuthorizedOccupancy(Entries, SubjectStableUnitId);

	// Tutte le altre unita' note sono bersagli possibili per una carica. La vista non porta `TeamId`
	// (`FRTKnowledgeEntry`), quindi la query non puo' distinguere i lati: trattarle tutte come urtabili non
	// sottostima mai la minaccia, ed e' la scelta conservativa per un'anteprima.
	TSet<int32> Hostiles;
	for (const FRTKnowledgeEntry& E : Entries)
	{
		if (E.StableUnitId != SubjectStableUnitId) { Hostiles.Add(E.StableUnitId); }
	}

	TSet<FRTCellId> DashOrigins;

	for (const TObjectPtr<URTActionData>& Ptr : Hero->Actions)
	{
		const URTActionData* Action = Ptr.Get();
		if (!Action) { continue; }

		// ⛔ IL FILTRO CHE DEFINISCE `PostDashThreat`. `NormalMovement` risolve in `ERTMatchPhase::Move`,
		// cioe' DOPO il Blast: arrivarci non abilita nessun attacco in questo turno, e ammetterlo qui
		// mostrerebbe una minaccia che il ruleset non consente.
		if (!URTCatalogLibrary::IsFastMovement(Action->Def)) { continue; }

		// Guardia sul futuro, non sul presente: nessuna mobilita' rapida dei cataloghi usa oggi
		// `MovementAndMain`, ma l'enum lo prevede. Una che lo usasse spenderebbe anche lo slot principale,
		// e non avrebbe piu' un attacco da portare all'arrivo.
		if (URTCatalogLibrary::TakesMainSlot(Action->Def)) { continue; }

		const int32 Declared = DeclaredRange(Action);
		if (Declared <= 0) { continue; }

		if (Action->Def.MovementStyle == ERTMovementStyle::Budget)
		{
			// Scatto a budget (`Action.Sprint`): stesso Dijkstra del movimento, altra quantita'.
			for (const FRTCellId& C : ReachableWithBudget(Map, Entries, SubjectStableUnitId, Declared))
			{
				if (C != Origin) { DashOrigins.Add(C); }
			}
			continue;
		}

		if (!URTMovementActionLibrary::IsLinear(Action->Def.MovementStyle)) { continue; }

		// Mobilita' lineare: la risolve `URTMovementActionLibrary`, non questa libreria. Reimplementare la
		// linearita' sarebbe la seconda autorita' che l'invariante #1 vieta, ed e' la divergenza che #140 ha
		// gia' chiuso una volta.
		for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
		{
			const ERTHexDirection Dir = static_cast<ERTHexDirection>(DirIndex);
			const FIntPoint Step = URTHexLibrary::AxialDirection(Dir);

			// ⚠️ **Ogni distanza fino alla portata, non solo la massima.** La destinazione la sceglie chi si
			// muove, e fermarsi a meta' e' una mossa legale. Prendere il solo estremo SOTTOSTIMEREBBE la
			// minaccia: le celle attorno a un arrivo intermedio non sono un sottoinsieme di quelle attorno
			// all'arrivo lontano, e un'anteprima che le omette promette una sicurezza che non c'e'.
			// Lo scatto a budget non ha questo problema — `ReachableCells` gli restituisce gia' ogni arrivo.
			for (int32 Steps = 1; Steps <= Declared; ++Steps)
			{
				const FRTCellId Target(Origin.X + Step.X * Steps, Origin.Y + Step.Y * Steps, Origin.Layer);

				const FRTLinearMoveResult Move = URTMovementActionLibrary::ResolveLinearMove(
					Map, Origin, Target, Steps, Action->Def.MovementStyle, Occupancy, Hostiles);

				// 🔑 `Action.Charge` — LATO A. L'impronta d'impatto e' minaccia IMMEDIATA: parte dalla cella
				// corrente, non segue nessuno scatto, e risolve in `Dash`, cioe' PRIMA del Blast. Senza
				// questo ramo la carica sparirebbe dall'anteprima: non e' un'azione di slot principale,
				// quindi `AddMainSlotFootprint` non la vede.
				if (Action->Def.MovementStyle == ERTMovementStyle::LinearCharge)
				{
					for (const FRTCellId& C : Move.Entered) { Immediate.Add(C); }
				}

				// 🔑 `Action.Charge` — LATO B, e vale per ogni mobilita' lineare: la cella d'ARRIVO e'
				// un'origine post-scatto, perche' lo slot principale resta libero. `Final == Origin`
				// significa che lo scatto non e' partito: non e' una situazione post-scatto, ed entrarci
				// confonderebbe le due regioni.
				if (Move.Final != Origin) { DashOrigins.Add(Move.Final); }
			}
		}
	}

	// --- Minaccia post-scatto -----------------------------------------------------------------------------
	//
	// ⚠️ Costo dichiarato: origini x celle mirabili x impronta. Nessun consumatore chiama oggi questa
	// funzione (#2597 e' un'altra issue); quando lo fara' a ogni selezione, il costo diventa una domanda
	// vera e va misurato, non stimato qui.
	TSet<FRTCellId> PostDash;
	for (const FRTCellId& DashOrigin : DashOrigins)
	{
		AddMainSlotFootprint(Map, DashOrigin, Hero, PostDash);
	}

	// --- Uscita canonica ----------------------------------------------------------------------------------
	//
	// L'ordine di un `TSet` dipende dall'hash e dall'inserimento: ordinare qui e' cio' che impedisce a due
	// esecuzioni identiche di produrre due sequenze diverse (invariante #3). `StableLess` e' lo stesso
	// comparatore con cui escono `ReachableCells` e `HexHitCells`: l'ordine si conserva, non si reinventa.
	auto Canonical = [](const TSet<FRTCellId>& Source)
	{
		TArray<FRTCellId> Out = Source.Array();
		Out.Sort([](const FRTCellId& A, const FRTCellId& B) { return URTHexLibrary::StableLess(A, B); });
		return Out;
	};

	OutRegions.StableUnitId = SubjectStableUnitId;
	OutRegions.ReachableCells = Canonical(Reachable);
	OutRegions.ImmediateThreat = Canonical(Immediate);
	OutRegions.PostDashThreat = Canonical(PostDash);
	return true;
}
