#include "Turn/RTMatchSetupLibrary.h"

#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexCellData.h"
#include "Terrain/RTTerrainLibrary.h"
#include "Terrain/RTTerrainData.h"

namespace
{
	/**
	 * Cella della fixture showcase: superficie richiesta, costo dettato dal CATALOGO terreni.
	 * Il costo non si scrive qui, altrimenti ribilanciare `Rough` lascerebbe la showcase su un numero morto.
	 * (Nome prefissato per dominio: nella unity build questo file condivide l'unita' di traduzione con altri.)
	 */
	FRTHexCellData MakeShowcaseTerrainCell(const FRTCellId& Id, ERTHexSurface Surface)
	{
		FRTHexCellData Cell(Id);
		Cell.Surface = Surface;
		Cell.MoveCost = URTTerrainLibrary::FindTerrainDef(Surface).MoveCost;
		return Cell;
	}
}

TArray<FRTCellId> URTMatchSetupLibrary::PickStartCells(const URTHexMapAsset* Map, int32 NumPerTeam, int32 Layer)
{
	TArray<FRTCellId> Result;
	if (!Map || NumPerTeam <= 0)
	{
		return Result;
	}

	// Celle percorribili del layer. CellsInLayer garantisce gia' l'ordine stabile (Layer, X, Y): nessuna
	// dipendenza dall'ordine di una TMap, quindi l'allestimento e' deterministico (invariante #4).
	TArray<FRTCellId> Walkable;
	for (const FRTCellId& Id : Map->CellsInLayer(Layer))
	{
		const FRTHexCellData* Data = Map->FindCell(Id);
		if (Data && !Data->bBlocksMovement)
		{
			Walkable.Add(Id);
		}
	}

	// Non si allestisce a meta': o ci stanno tutte le unita', o il chiamante non allestisce affatto.
	if (Walkable.Num() < NumPerTeam * 2)
	{
		return Result;
	}

	// Team 0 dall'inizio dell'ordine, team 1 dalla fine: le squadre partono agli estremi della mappa.
	Result.Reserve(NumPerTeam * 2);
	for (int32 i = 0; i < NumPerTeam; ++i)
	{
		Result.Add(Walkable[i]);
	}
	for (int32 i = 0; i < NumPerTeam; ++i)
	{
		Result.Add(Walkable[Walkable.Num() - 1 - i]);
	}
	return Result;
}

TMap<FRTCellId, int32> URTMatchSetupLibrary::BuildOccupancy(const TArray<FRTCellId>& Cells,
	const TArray<int32>& UnitIds, const TArray<bool>& Alive)
{
	TMap<FRTCellId, int32> Occupancy;
	if (Cells.Num() != UnitIds.Num() || Cells.Num() != Alive.Num())
	{
		return Occupancy;
	}

	// Le unita' non vive non occupano celle (stessa regola di FRTHexSnapshot::Occupancy).
	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		if (Alive[i])
		{
			Occupancy.Add(Cells[i], UnitIds[i]);
		}
	}
	return Occupancy;
}

URTHexMapAsset* URTMatchSetupLibrary::MakeDemoArena(UObject* Outer, int32 Radius)
{
	if (Outer == nullptr || Radius < 1)
	{
		return nullptr; // nessuna arena a meta': il chiamante decide cosa fare senza mappa
	}

	URTHexMapAsset* Arena = NewObject<URTHexMapAsset>(Outer);
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), Radius))
	{
		Arena->AddOrUpdateCell(FRTHexCellData(Id)); // pavimento semplice: costo 1, niente blocchi
	}
	Arena->SortCells();
	return Arena;
}

URTHexMapAsset* URTMatchSetupLibrary::MakeTestArena(UObject* Outer)
{
	if (Outer == nullptr)
	{
		return nullptr;
	}

	URTHexMapAsset* Arena = NewObject<URTHexMapAsset>(Outer);

	// Base: esagono pieno di raggio 4 sul layer 0, pavimento a costo 1.
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 4))
	{
		Arena->AddOrUpdateCell(FRTHexCellData(Id));
	}

	// Ostacoli al MOVIMENTO, sparsi. Le celle di partenza stanno agli estremi (q=-4 e q=+4, vedi
	// PickStartCells): gli ostacoli non le toccano, cosi' la partita si allestisce comunque.
	for (const FRTCellId& Id : { FRTCellId(-1, 2, 0), FRTCellId(1, -2, 0), FRTCellId(2, 1, 0) })
	{
		FRTHexCellData Cell(Id);
		Cell.bBlocksMovement = true;
		Arena->AddOrUpdateCell(Cell);
	}

	// Muro che blocca la VISTA lungo q=0: separa le due meta' del campo restando attraversabile. Copre r=-2..2
	// perche' una linea fra i due estremi deriva di qualche riga: un muro piu' corto la lascerebbe passare di lato.
	for (int32 R = -2; R <= 2; ++R)
	{
		FRTHexCellData Cell(FRTCellId(0, R, 0));
		Cell.bBlocksLineOfSight = true;
		Arena->AddOrUpdateCell(Cell);
	}

	// Fascia di fango a q=-2: costo 3, non un muro. Serve a vedere il budget mordere (una cella "costa" tre passi).
	for (int32 R = -1; R <= 1; ++R)
	{
		FRTHexCellData Cell(FRTCellId(-2, R, 0));
		Cell.Surface = ERTHexSurface::Rough;
		Cell.MoveCost = 3;
		Arena->AddOrUpdateCell(Cell);
	}

	// Piattaforma sul layer 1, sopra il quadrante destro.
	for (const FRTCellId& Id : { FRTCellId(2, -1, 1), FRTCellId(2, 0, 1), FRTCellId(3, -1, 1), FRTCellId(3, 0, 1) })
	{
		Arena->AddOrUpdateCell(FRTHexCellData(Id));
	}

	// UNA sola transizione terra->piattaforma: i layer si collegano solo con archi espliciti, quindi togliendola
	// la piattaforma torna irraggiungibile. E' cio' che rende verificabile "il path FALLISCE, non teletrasporta".
	Arena->AddTransition(FRTCellId(1, 0, 0), FRTCellId(2, 0, 1), /*Cost=*/ 2);

	Arena->SortCells();
	return Arena;
}

URTHexMapAsset* URTMatchSetupLibrary::MakeShowcaseRelayLiteArena(UObject* Outer)
{
	if (Outer == nullptr)
	{
		return nullptr;
	}

	URTHexMapAsset* Arena = NewObject<URTHexMapAsset>(Outer);

	// Base: esagono pieno di raggio 5 sul layer 0 -> 3*5*6 + 1 = 91 celle di pavimento.
	for (const FRTCellId& Id : URTHexLibrary::HexArea(FRTCellId(0, 0, 0), 5))
	{
		Arena->AddOrUpdateCell(MakeShowcaseTerrainCell(Id, ERTHexSurface::Floor));
	}

	// Le superfici stanno in COPPIE SPECULARI (q,r) / (-q,-r): il centro (0,0) e' l'unico punto fisso.
	// La simmetria non e' estetica — e' cio' che rende un esito attribuibile alle scelte e non al lato.
	struct FShowcasePatch
	{
		FRTCellId Cell;
		ERTHexSurface Surface;
	};
	const FShowcasePatch Patches[] = {
		// Spina d'acqua al centro: applica `Wet` a chi entra e conduce (il payoff elettrico e' CP 8.3).
		{ FRTCellId( 0,  0, 0), ERTHexSurface::ShallowWater },
		{ FRTCellId( 0, -1, 0), ERTHexSurface::ShallowWater },
		{ FRTCellId( 0,  1, 0), ERTHexSurface::ShallowWater },
		// Conduttivo a contatto con l'acqua: la rete esiste gia' come dato, nessuna regola nuova.
		{ FRTCellId( 1, -1, 0), ERTHexSurface::Conductive },
		{ FRTCellId(-1,  1, 0), ERTHexSurface::Conductive },
		// Rough: vieta Dash/Charge su una via d'avvicinamento, cosi' la mobilita' rapida ha un prezzo.
		{ FRTCellId(-2, -1, 0), ERTHexSurface::Rough },
		{ FRTCellId( 2,  1, 0), ERTHexSurface::Rough },
		// Ice: chi TERMINA il Move qui scivola di una cella.
		{ FRTCellId(-2,  2, 0), ERTHexSurface::Ice },
		{ FRTCellId( 2, -2, 0), ERTHexSurface::Ice },
		// Fire: 10 danni + `Burning` a chi entra, dal catalogo terreni.
		{ FRTCellId( 0, -2, 0), ERTHexSurface::Fire },
		{ FRTCellId( 0,  2, 0), ERTHexSurface::Fire },
		// Smoke: cap del targeting a 2 celle attraverso la cella.
		{ FRTCellId(-1, -2, 0), ERTHexSurface::Smoke },
		{ FRTCellId( 1,  2, 0), ERTHexSurface::Smoke },
	};
	for (const FShowcasePatch& Patch : Patches)
	{
		Arena->AddOrUpdateCell(MakeShowcaseTerrainCell(Patch.Cell, Patch.Surface));
	}

	Arena->SortCells();
	return Arena;
}

TArray<FRTShowcaseSpawn> URTMatchSetupLibrary::GetShowcaseRelayLiteSpawns()
{
	// Estremi opposti dell'arena, in coppie speculari come le superfici. Celle di pavimento: nessuna squadra
	// comincia dentro un terreno che la penalizza al primo passo.
	return {
		FRTShowcaseSpawn(TEXT("Hero.Flux"),    /*TeamId=*/ 0, FRTCellId(-5,  2, 0)),
		FRTShowcaseSpawn(TEXT("Hero.Riva"),    /*TeamId=*/ 0, FRTCellId(-5,  3, 0)),
		FRTShowcaseSpawn(TEXT("Hero.Bastion"), /*TeamId=*/ 1, FRTCellId( 5, -2, 0)),
		FRTShowcaseSpawn(TEXT("Hero.Vektor"),  /*TeamId=*/ 1, FRTCellId( 5, -3, 0)),
	};
}
