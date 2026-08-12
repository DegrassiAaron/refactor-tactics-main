#include "Terrain/RTTerrainLibrary.h"
#include "Map/RTHexArcLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Pathfinding/RTHexPathLibrary.h" // GraphNeighbors: la fuga segue il grafo, non la distanza (#505)
#include "Map/RTHexMapAsset.h"

namespace
{
	/** `SlideCells` in coda e con default: solo `Ice` lo valorizza, e li' e' scritto per nome. */
	FRTTerrainDef MakeTerrain(ERTHexSurface Surface, int32 MoveCost, bool bBlocksDashCharge,
		bool bBlocksLineOfSight, bool bConductsElectricity, int32 MaxTargetingRangeThrough,
		TArray<FRTActionEffectSpec> OnEnterEffects, int32 SlideCells = 0, bool bIsFlammable = false,
		int32 NoiseDelta = 0)
	{
		FRTTerrainDef Def;
		Def.Surface = Surface;
		Def.MoveCost = MoveCost;
		Def.bBlocksDashCharge = bBlocksDashCharge;
		Def.bBlocksLineOfSight = bBlocksLineOfSight;
		Def.bConductsElectricity = bConductsElectricity;
		Def.MaxTargetingRangeThrough = MaxTargetingRangeThrough;
		Def.OnEnterEffects = MoveTemp(OnEnterEffects);
		Def.SlideCells = SlideCells;
		Def.bIsFlammable = bIsFlammable;
		Def.NoiseDelta = NoiseDelta;
		return Def;
	}
}

TArray<FRTTerrainDef> URTTerrainLibrary::GetTerrainCatalog()
{
	// `NoiseDelta` (CP 13.3) e' l'ultimo parametro di ogni riga. Derivazione: `Noise_Mod - 1` dal workbook
	// (`08_Terreni`), terreno libero = 0. Le due eccezioni sono scritte accanto alla riga che le porta, non in
	// un commento generale: chi cambia un numero deve leggere li' perche' non e' quello della formula.
	TArray<FRTTerrainDef> Catalog;
	Catalog.Add(MakeTerrain(ERTHexSurface::Floor,        1, false, false, false, 0, {}, /*Slide*/ 0, /*Flammable*/ true,
		/*NoiseDelta*/ 0));  // TERRAIN_CLEAR (1): e' il riferimento della scala
	Catalog.Add(MakeTerrain(ERTHexSurface::Rough,        2, true,  false, false, 0, {}, /*Slide*/ 0, /*Flammable*/ true,
		/*NoiseDelta*/ 1));  // ⚠️ NESSUNA riga nel workbook: deciso +1 (2026-08-11), come il ghiaccio —
		                     // terreno accidentato, si sente. `TERRAIN_MUD` (3) e' vicino per semantica ma
		                     // non e' la stessa superficie, e assumerlo sarebbe inventare un numero.
	Catalog.Add(MakeTerrain(ERTHexSurface::ShallowWater, 2, false, false, true,  0,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Wet, 0) }, /*Slide*/ 0, /*Flammable*/ false,
		/*NoiseDelta*/ 2));  // ⚠️ **D-042**: +2, NON il +3 che `Noise_Mod 4` darebbe. Vince il documento
		                     // sorgente; il workbook resta RESEARCH. Unico punto dove formula e decisione
		                     // divergono, ed e' pinnato da `Noise.AttenuationBySurface`.
	Catalog.Add(MakeTerrain(ERTHexSurface::Fire,         2, false, false, false, 0,
		{ FRTActionEffectSpec(ERTActionEffect::Damage, 10),
		  FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Burning, 2) }, /*Slide*/ 0, /*Flammable*/ false,
		/*NoiseDelta*/ 4));  // TERRAIN_FIRE (5): il fuoco copre quasi tutto
	Catalog.Add(MakeTerrain(ERTHexSurface::Conductive,   1, false, false, true,  0, {}, /*Slide*/ 0, /*Flammable*/ false,
		/*NoiseDelta*/ 0));  // ⚠️ NESSUNA riga nel workbook: deciso 0 (2026-08-11). Ha gia' un owner, ed e'
		                     // elettrico (D-039): il rumore non e' il suo mestiere.
	Catalog.Add(MakeTerrain(ERTHexSurface::Smoke,        1, false, false, false, 2,
		{ FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Obscured, 0) }, /*Slide*/ 0, /*Flammable*/ false,
		/*NoiseDelta*/ 0));  // TERRAIN_SMOKE (1): il fumo acceca, non ammutolisce
	Catalog.Add(MakeTerrain(ERTHexSurface::Ice,          1, false, false, false, 0, {}, /*SlideCells*/ 1,
		/*Flammable*/ false, /*NoiseDelta*/ 1));  // TERRAIN_ICE (2), e il documento sorgente concorda
	Catalog.Add(MakeTerrain(ERTHexSurface::HighGround,   1, false, false, false, 0, {}, /*Slide*/ 0,
		/*Flammable*/ false, /*NoiseDelta*/ 0));  // TERRAIN_HIGH (1)
	return Catalog;
}

TSet<FGameplayTag> URTTerrainLibrary::CellBoundStatusesFor(ERTHexSurface Surface)
{
	TSet<FGameplayTag> Sustained;
	for (const FRTActionEffectSpec& Effect : FindTerrainDef(Surface).OnEnterEffects)
	{
		// Durata 0 su un effetto di stato = "finche' sulla cella" (spec-terreni-e8.md §2.1). Le durate
		// esplicite (Burning 2) non sono sostenute dalla cella: scadono a tempo anche restando nel fuoco.
		if (Effect.Effect == ERTActionEffect::Status && Effect.StatusDuration == 0 && Effect.StatusTag.IsValid())
		{
			Sustained.Add(Effect.StatusTag);
		}
	}
	return Sustained;
}

FRTTerrainDef URTTerrainLibrary::FindTerrainDef(ERTHexSurface Surface)
{
	for (const FRTTerrainDef& Def : GetTerrainCatalog())
	{
		if (Def.Surface == Surface) { return Def; }
	}
	return FRTTerrainDef();
}

int32 URTTerrainLibrary::EffectiveTargetingRange(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& To, int32 RangeCells)
{
	if (Map == nullptr)
	{
		return RangeCells; // niente terreno da leggere: il fail-closed e' del chiamante
	}

	// Catalogo letto UNA volta e non per cella: FindTerrainDef ricostruisce le 8 righe a ogni chiamata (e ogni
	// riga alloca il suo TArray di effetti), e questa funzione sta nel ciclo interno di BuildCandidates —
	// celle raggiungibili x nemici x lunghezza della linea. Stesso risultato, senza il lavoro ripetuto.
	const TArray<FRTTerrainDef> Catalog = GetTerrainCatalog();

	// `HexLine` e' la stessa primitiva usata da HexHitCells per Shape::Line: la linea che il cap misura e'
	// quella su cui si spara davvero, non una seconda approssimazione.
	int32 Effective = RangeCells;
	for (const FRTCellId& LineCell : URTHexLibrary::HexLine(From, To))
	{
		const FRTHexCellData* CellData = Map->FindCell(LineCell);
		if (CellData == nullptr)
		{
			continue;
		}
		for (const FRTTerrainDef& Def : Catalog)
		{
			if (Def.Surface == CellData->Surface && Def.MaxTargetingRangeThrough > 0)
			{
				Effective = FMath::Min(Effective, Def.MaxTargetingRangeThrough);
				break;
			}
		}
	}
	return Effective;
}

TArray<FString> URTTerrainLibrary::ValidateTerrainCatalog()
{
	return ValidateCatalogEntries(GetTerrainCatalog());
}

TArray<FString> URTTerrainLibrary::ValidateCatalogEntries(const TArray<FRTTerrainDef>& Catalog)
{
	TArray<FString> Errors;
	TSet<ERTHexSurface> Seen;
	for (const FRTTerrainDef& Def : Catalog)
	{
		bool bAlreadySeen = false;
		Seen.Add(Def.Surface, &bAlreadySeen);
		if (bAlreadySeen)
		{
			Errors.Add(FString::Printf(TEXT("Terreno duplicato: %d"), static_cast<int32>(Def.Surface)));
		}
		if (Def.MoveCost < 0)
		{
			Errors.Add(FString::Printf(TEXT("Terreno %d: MoveCost negativo (%d)"), static_cast<int32>(Def.Surface), Def.MoveCost));
		}
		if (Def.MaxTargetingRangeThrough < 0)
		{
			Errors.Add(FString::Printf(TEXT("Terreno %d: MaxTargetingRangeThrough negativo (%d)"), static_cast<int32>(Def.Surface), Def.MaxTargetingRangeThrough));
		}
	}
	return Errors;
}

TArray<FRTPropagationHit> URTTerrainLibrary::CollectElectricPropagation(const URTHexMapAsset* Map,
	const FRTCellId& SourceCell, int32 MaxSteps, int32 InitialDamage, int32 PropagatedDamage,
	const TArray<FRTHexCombatUnit>& Units)
{
	TArray<FRTPropagationHit> Hits;
	if (!Map)
	{
		return Hits; // fail-closed: senza mappa non si sa quali celle conducano
	}

	// Indice cella -> unita' vive che la occupano. Costruito una volta: il BFS non deve riscorrere l'array
	// delle unita' per ogni cella visitata, e l'ordine di questa mappa non influenza il risultato perche'
	// l'output viene ordinato alla fine con una chiave totale.
	TMultiMap<FRTCellId, int32> UnitsByCell;
	for (const FRTHexCombatUnit& Unit : Units)
	{
		if (Unit.bAlive)
		{
			UnitsByCell.Add(Unit.Cell, Unit.UnitId);
		}
	}

	auto Conducts = [Map](const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = Map->FindCell(Cell);
		return Data && FindTerrainDef(Data->Surface).bConductsElectricity;
	};

	auto AddHitsOn = [&Hits, &UnitsByCell](const FRTCellId& Cell, int32 Steps, int32 Damage)
	{
		TArray<int32> Occupants;
		UnitsByCell.MultiFind(Cell, Occupants);
		for (const int32 UnitId : Occupants)
		{
			FRTPropagationHit Hit;
			Hit.UnitId = UnitId;
			Hit.Steps = Steps;
			Hit.Damage = Damage;
			Hit.Cell = Cell;
			Hits.Add(Hit);
		}
	};

	// Passo 0: chi sta sulla cella sorgente incassa il colpo DIRETTO. Vale anche se la cella non conduce —
	// elettrificare un bersaglio all'asciutto lo colpisce comunque, semplicemente non propaga oltre.
	AddHitsOn(SourceCell, /*Steps*/ 0, InitialDamage);

	// Visita in AMPIEZZA sul grafo delle celle conduttive: ogni cella e' visitata una volta sola, quindi
	// ogni unita' compare una volta sola anche quando piu' cammini d'acqua la raggiungono (vincolo del
	// catalogo §2). Il BFS garantisce inoltre che la prima volta che si tocca una cella la si tocchi alla
	// distanza MINIMA: il numero di passi e' una proprieta' della cella, non dell'ordine di scoperta.
	if (MaxSteps > 0 && Conducts(SourceCell))
	{
		TSet<FRTCellId> Visited;
		Visited.Add(SourceCell);

		TArray<FRTCellId> Frontier;
		Frontier.Add(SourceCell);

		for (int32 Step = 1; Step <= MaxSteps && Frontier.Num() > 0; ++Step)
		{
			TArray<FRTCellId> Next;
			for (const FRTCellId& Cell : Frontier)
			{
				// Ordine dei vicini FISSO (`URTHexLibrary::Neighbors` restituisce le sei direzioni in ordine
				// di enum): il risultato non dipende dall'iterazione di un container non ordinato (#4).
				// I sei vicini planari, PIU' i ponti conduttivi uscenti (CP 9.4): senza questi la scarica non
				// sale mai di layer, e «ponte conduttivo» non significherebbe niente. L'ordine e' fisso —
				// prima i vicini in ordine di enum, poi gli archi nell'ordine dell'array, che `SortCells` e
				// `AddTransition` tengono stabile — quindi il risultato non dipende da container non ordinati.
				TArray<FRTCellId> Adjacent = URTHexLibrary::Neighbors(Cell);
				for (const FRTHexEdge& Arc : Map->Transitions)
				{
					if (Arc.From == Cell && URTHexArcLibrary::ArcConductsElectricity(Map, Arc.From, Arc.To))
					{
						Adjacent.Add(Arc.To);
					}
				}

				for (const FRTCellId& Neighbor : Adjacent)
				{
					if (Visited.Contains(Neighbor) || !Conducts(Neighbor))
					{
						continue; // gia' raggiunta, oppure la catena si interrompe qui
					}
					Visited.Add(Neighbor);
					Next.Add(Neighbor);
					AddHitsOn(Neighbor, Step, PropagatedDamage);
				}
			}
			Frontier = MoveTemp(Next);
		}
	}

	// Ordine DICHIARATO dal catalogo: distanza -> cella -> unita'. E' totale (due unita' non possono
	// condividere cella e id), quindi la sequenza e' la stessa a ogni esecuzione.
	Hits.Sort([](const FRTPropagationHit& A, const FRTPropagationHit& B)
	{
		if (A.Steps != B.Steps) { return A.Steps < B.Steps; }
		if (!(A.Cell == B.Cell)) { return URTHexLibrary::StableLess(A.Cell, B.Cell); }
		return A.UnitId < B.UnitId;
	});
	return Hits;
}

bool URTTerrainLibrary::IsHazardousSurface(ERTHexSurface Surface)
{
	// Il catalogo decide, non un elenco qui: pericolosa = infligge danno a chi ci entra.
	for (const FRTActionEffectSpec& Effect : FindTerrainDef(Surface).OnEnterEffects)
	{
		if (Effect.Effect == ERTActionEffect::Damage && Effect.Amount > 0)
		{
			return true;
		}
	}
	return false;
}

FRTCellId URTTerrainLibrary::FindEscapeCell(const URTHexMapAsset* Map, const FRTCellId& From,
	ERTHexDirection Facing, const TArray<FRTCellId>& Occupied)
{
	if (!Map)
	{
		return From; // fail-closed: senza mappa non si sa dove si puo' andare
	}

	// Le celle davvero raggiungibili: il grafo, non i sei vicini geometrici. Un dislivello senza arco o un
	// muro non sono una via di fuga solo perche' la cella e' adiacente sulla griglia.
	TArray<FRTCellId> Reachable;
	for (const TPair<FRTCellId, int32>& Step : URTHexPathLibrary::GraphNeighbors(Map, From))
	{
		Reachable.Add(Step.Key);
	}

	auto IsEscapable = [Map, &Reachable, &Occupied](const FRTCellId& Candidate)
	{
		if (!Reachable.Contains(Candidate) || Occupied.Contains(Candidate)) { return false; }
		const FRTHexCellData* Data = Map->FindCell(Candidate);
		return Data != nullptr && !URTTerrainLibrary::IsHazardousSurface(Data->Surface);
	};

	// 1) Dove sta guardando: e' la scelta che il giocatore puo' prevedere senza conoscere l'ordine interno
	//    delle direzioni.
	const FRTCellId Ahead = URTHexLibrary::Neighbor(From, Facing);
	if (IsEscapable(Ahead))
	{
		return Ahead;
	}

	// 2) Ripiego nell'ordine canonico delle direzioni (E, NE, NW, W, SW, SE): arbitrario per il giocatore ma
	//    DICHIARATO e stabile, quindi due situazioni identiche danno la stessa fuga.
	for (const FRTCellId& Candidate : URTHexLibrary::Neighbors(From))
	{
		if (IsEscapable(Candidate))
		{
			return Candidate;
		}
	}

	// 3) Circondati da fuoco, unita' o bordo mappa: la reazione e' scattata e non ha dove mandarti. E' un
	//    esito, e chi chiama lo registra — come `EmergencyDash` quando non ha una cella libera.
	return From;
}
