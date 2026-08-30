#include "Map/RTHexCoverPlacementLibrary.h"

namespace
{
	/** I dodici bit validi di una maschera di settori. Fuori da qui non c'e' nulla da leggere. */
	constexpr int32 RT_AllWedges = (1 << RT_OccupancySectorCount) - 1;

	/** Il settore precedente sull'anello. La contiguita' e' CIRCOLARE: il precedente di `0` e' `11`. */
	FORCEINLINE int32 PrevWedge(int32 Wedge)
	{
		return (Wedge + RT_OccupancySectorCount - 1) % RT_OccupancySectorCount;
	}

	FORCEINLINE int32 NextWedge(int32 Wedge)
	{
		return (Wedge + 1) % RT_OccupancySectorCount;
	}

	FORCEINLINE bool IsFree(int32 FreeMask, int32 Wedge)
	{
		return ((FreeMask >> Wedge) & 1) != 0;
	}

	/**
	 * La regione piu' i due settori che la delimitano.
	 *
	 * Serve a una domanda sola — *«questo bordo e' di fianco a chi sta qui?»* — e non e' una regione: chi
	 * la riceve non ci puo' posare nulla. Sta in questo namespace anonimo, e non fra i metodi pubblici,
	 * proprio perche' non deve poter essere scambiata per uno spazio libero.
	 */
	int32 ExpandRegion(const FRTPlacementRegion& Region)
	{
		int32 Expanded = Region.WedgeMask;
		for (int32 Wedge = 0; Wedge < RT_OccupancySectorCount; ++Wedge)
		{
			if (((Region.WedgeMask >> Wedge) & 1) != 0)
			{
				Expanded |= (1 << PrevWedge(Wedge));
				Expanded |= (1 << NextWedge(Wedge));
			}
		}
		return Expanded & RT_AllWedges;
	}
}

void URTHexCoverPlacementLibrary::ComputeFreeRegions(const FRTOccupancyMask& Mask,
	TArray<FRTPlacementRegion>& OutRegions)
{
	OutRegions.Reset();

	const int32 FreeMask = (~Mask.Sectors) & RT_AllWedges;
	if (FreeMask == 0)
	{
		return;
	}

	// L'anello interamente libero non ha un settore «il cui precedente e' occupato»: il ciclo sotto non
	// troverebbe alcun inizio e restituirebbe zero regioni per una cella completamente sgombra, che e' il
	// contrario del vero. E' l'unico caso degenere, e si dichiara invece di correggerlo con una guardia
	// dentro il ciclo.
	if (FreeMask == RT_AllWedges)
	{
		FRTPlacementRegion Whole;
		Whole.WedgeMask = RT_AllWedges;
		Whole.FirstWedge = 0;
		Whole.Size = RT_OccupancySectorCount;
		OutRegions.Add(Whole);
		return;
	}

	// In ordine di `FirstWedge` crescente per costruzione: si percorre l'anello da 0 a 11 una volta sola.
	for (int32 Start = 0; Start < RT_OccupancySectorCount; ++Start)
	{
		if (!IsFree(FreeMask, Start) || IsFree(FreeMask, PrevWedge(Start)))
		{
			continue;
		}

		FRTPlacementRegion Region;
		Region.FirstWedge = Start;

		int32 Cursor = Start;
		while (IsFree(FreeMask, Cursor))
		{
			Region.WedgeMask |= (1 << Cursor);
			++Region.Size;
			Cursor = NextWedge(Cursor);
			// Non serve una guardia sul giro completo: se ci fosse, `FreeMask` sarebbe `RT_AllWedges`, ed
			// e' il ramo gia' uscito sopra.
		}

		OutRegions.Add(Region);
	}
}

bool URTHexCoverPlacementLibrary::HasLegalPlacement(const FRTOccupancyMask& Mask,
	const FRTFootprintProfile& Footprint)
{
	if (Footprint.bRequiresFreeCore && Mask.bCoreBlocked)
	{
		return false;
	}

	TArray<FRTPlacementRegion> Regions;
	ComputeFreeRegions(Mask, Regions);
	return FindPlacementRegion(Regions, Footprint) != INDEX_NONE;
}

int32 URTHexCoverPlacementLibrary::FindPlacementRegion(const TArray<FRTPlacementRegion>& Regions,
	const FRTFootprintProfile& Footprint)
{
	const int32 Required = FMath::Max(1, Footprint.MinContiguousWedges);

	// `Regions` arriva gia' ordinata per `FirstWedge` crescente da `ComputeFreeRegions`: la prima idonea
	// E' la canonica, e non serve un secondo ordinamento.
	for (int32 Index = 0; Index < Regions.Num(); ++Index)
	{
		if (Regions[Index].Size >= Required)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

int32 URTHexCoverPlacementLibrary::RegionIndexForWedge(const TArray<FRTPlacementRegion>& Regions, int32 Wedge)
{
	for (int32 Index = 0; Index < Regions.Num(); ++Index)
	{
		if (Regions[Index].Contains(Wedge))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void URTHexCoverPlacementLibrary::AxisHalfPlanes(ERTTacticalAxis Axis, int32& OutMaskA, int32& OutMaskB)
{
	// Il confine `b` e' il punto in cui l'asse buca il perimetro, e il confine `j` separa il settore `j-1`
	// dal settore `j`: la retta per `b` e `b+6` lascia quindi da una parte i sei settori `[b .. b+5]` e
	// dall'altra i sei opposti. La corrispondenza asse -> confine non e' riscritta qui.
	const int32 Boundary = URTGeometryGrammarLibrary::AxisBoundaryIndex(Axis);

	OutMaskA = 0;
	for (int32 Step = 0; Step < RT_OccupancySectorCount / 2; ++Step)
	{
		OutMaskA |= (1 << ((Boundary + Step) % RT_OccupancySectorCount));
	}
	OutMaskB = RT_AllWedges & ~OutMaskA;
}

ERTCoverSide URTHexCoverPlacementLibrary::SideOfWedge(int32 Wedge, ERTTacticalAxis Axis)
{
	int32 MaskA = 0;
	int32 MaskB = 0;
	AxisHalfPlanes(Axis, MaskA, MaskB);

	if (Wedge < 0 || Wedge >= RT_OccupancySectorCount)
	{
		return ERTCoverSide::None;
	}
	return ((MaskA >> Wedge) & 1) != 0 ? ERTCoverSide::A : ERTCoverSide::B;
}

void URTHexCoverPlacementLibrary::EnumerateCoverOptions(const FRTHexCellData& Cell,
	const TArray<FRTGeometrySegment>& Segments, const FRTOccupancyMask& Mask,
	TArray<FRTCoverOption>& OutOptions)
{
	OutOptions.Reset();

	TArray<FRTPlacementRegion> Regions;
	ComputeFreeRegions(Mask, Regions);
	if (Regions.Num() == 0)
	{
		// Nessuna posa legale: non c'e' nessuno che possa usare una copertura, quindi non c'e' nessuna
		// opzione. Le coperture della cella restano dato valido — il bordo resta murato per chi passa
		// accanto — ma «opzione» significa «modo in cui un occupante la usa», e qui l'occupante non c'e'.
		return;
	}

	// 1 · I BORDI, in ordine di `ERTHexDirection`, non nell'ordine in cui l'array li porta: un asset
	//     rieditato riordina `Covers`, e l'enumerazione non deve cambiare con esso.
	for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
	{
		const ERTHexDirection Edge = static_cast<ERTHexDirection>(EdgeIndex);
		const FRTHexCover* Entry = Cell.CoverEntryOn(Edge);
		if (Entry == nullptr || Entry->Type == ERTHexCoverType::None)
		{
			continue;
		}

		// Il lato `k` dell'esagono copre i due settori `2k` e `2k+1` — stessa convenzione dichiarata in
		// `RTGeometryBake.h`, e non un angolo ricalcolato qui.
		const int32 EdgeWedgeMask = (1 << (2 * EdgeIndex)) | (1 << (2 * EdgeIndex + 1));

		for (const FRTPlacementRegion& Region : Regions)
		{
			// ADIACENZA, non appartenenza. Delle rocce che occupano i due settori del bordo lo tolgono
			// dalla regione senza togliercelo di fianco: chi sta subito accanto ci si ripara lo stesso. Un
			// muro continuo che taglia la cella, invece, mette fra i due almeno un settore occupato per
			// parte, e li' l'esclusione e' vera — ed e' quella che impedisce alla scelta di diventare un
			// teletrasporto.
			if ((ExpandRegion(Region) & EdgeWedgeMask) == 0)
			{
				continue;
			}

			FRTCoverOption Option;
			Option.Source.Kind = ERTCoverSourceKind::Edge;
			Option.Source.AxisOrEdge = static_cast<uint8>(EdgeIndex);
			Option.Side = ERTCoverSide::None;
			Option.Type = Entry->Type;
			Option.AccessMask = Region.WedgeMask;
			OutOptions.Add(Option);
		}
	}

	// 2 · I SEGMENTI INTERNI. Due facce, e per ciascuna la parte di regione che ci sta davanti.
	for (const FRTGeometrySegment& Segment : Segments)
	{
		if (Segment.Layer != Cell.Id.Layer)
		{
			continue;
		}
		if (URTGeometryGrammarLibrary::ValidateSegment(Segment) != ERTGeometryViolation::None)
		{
			// Chi non ha validato non ottiene geometria, come in `ToPolyline`: rifiutare qui e correggere
			// altrove sarebbero due verita' sulla stessa regola.
			continue;
		}

		FRTCoverSourceId SourceId;
		SourceId.Kind = ERTCoverSourceKind::InteriorSegment;
		SourceId.AxisOrEdge = static_cast<uint8>(Segment.Axis);
		SourceId.Offset = Segment.Offset;
		SourceId.AlongMin = FMath::Min(Segment.AlongStart, Segment.AlongEnd);
		SourceId.AlongMax = FMath::Max(Segment.AlongStart, Segment.AlongEnd);

		int32 MaskA = 0;
		int32 MaskB = 0;
		AxisHalfPlanes(Segment.Axis, MaskA, MaskB);

		for (const FRTPlacementRegion& Region : Regions)
		{
			// `A` prima di `B`, e la faccia senza settori davanti non produce opzione: un muro che tocca la
			// regione da un lato solo espone da li' una faccia sola, ed e' cio' che distingue il muro
			// continuo — due opzioni in due regioni — dal raggio centro-vertice, due opzioni nella stessa.
			const int32 Faces[2] = { MaskA, MaskB };
			const ERTCoverSide Sides[2] = { ERTCoverSide::A, ERTCoverSide::B };

			for (int32 Face = 0; Face < 2; ++Face)
			{
				const int32 Access = Region.WedgeMask & Faces[Face];
				if (Access == 0)
				{
					continue;
				}

				FRTCoverOption Option;
				Option.Source = SourceId;
				Option.Side = Sides[Face];
				Option.Type = Segment.WallType;
				Option.AccessMask = Access;
				OutOptions.Add(Option);
			}
		}
	}
}

ERTIntraCellTraversal URTHexCoverPlacementLibrary::ClassifyIntraCellTraversal(const FRTOccupancyMask& Mask,
	int32 FromWedge, int32 ToWedge)
{
	if (FromWedge < 0 || FromWedge >= RT_OccupancySectorCount
		|| ToWedge < 0 || ToWedge >= RT_OccupancySectorCount)
	{
		return ERTIntraCellTraversal::Blocked;
	}

	TArray<FRTPlacementRegion> Regions;
	ComputeFreeRegions(Mask, Regions);

	const int32 From = RegionIndexForWedge(Regions, FromWedge);
	const int32 To = RegionIndexForWedge(Regions, ToWedge);

	// `INDEX_NONE` da una delle due parti significa settore occupato: non c'e' posa da cui partire o a cui
	// arrivare, e la risposta e' la stessa di due regioni diverse — non si passa.
	const bool bSame = (From != INDEX_NONE) && (From == To);
	return bSame ? ERTIntraCellTraversal::SameRegion : ERTIntraCellTraversal::Blocked;
}

bool URTHexCoverPlacementLibrary::IsOptionReachableFromWedge(const FRTOccupancyMask& Mask,
	const FRTCoverOption& Option, int32 FromWedge)
{
	if (FromWedge < 0 || FromWedge >= RT_OccupancySectorCount)
	{
		return false;
	}

	TArray<FRTPlacementRegion> Regions;
	ComputeFreeRegions(Mask, Regions);

	const int32 Index = RegionIndexForWedge(Regions, FromWedge);
	if (Index == INDEX_NONE)
	{
		// Settore occupato: non c'e' posa da cui partire.
		return false;
	}

	return (Option.AccessMask & Regions[Index].WedgeMask) != 0;
}
