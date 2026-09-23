#include "RTHexWorkGrid.h"

#include "Map/RTHexLibrary.h"

namespace RTHexWorkGrid
{
namespace
{
	/** Quante celle ha un esagono pieno di raggio `R`. La stessa formula che `URTHexLibrary::HexArea` realizza. */
	int32 WorkGridAreaSize(int32 Radius)
	{
		const int32 R = FMath::Max(0, Radius);
		return 3 * R * (R + 1) + 1;
	}
}

bool FWatch::operator==(const FWatch& Other) const
{
	// ⚠️ **Le grandezze float si confrontano esattamente, e va bene cosi'.** `HexSize`, `LayerHeight` e
	// `Origin` non sono il risultato di un calcolo: sono copiate da `GetHexContext` e dalla trasformata
	// dell'actor. Una tolleranza qui nasconderebbe uno spostamento minuscolo — che a schermo si vede,
	// perche' la griglia resterebbe dov'era.
	return MapActorId == Other.MapActorId
		&& Revision == Other.Revision
		&& NumCells == Other.NumCells
		&& ActiveLayer == Other.ActiveLayer
		&& Origin.Equals(Other.Origin, 0.f)
		&& HexSize == Other.HexSize
		&& LayerHeight == Other.LayerHeight
		&& bShow == Other.bShow
		&& Margin == Other.Margin
		&& SeedRadius == Other.SeedRadius;
}

FColor GhostColour()
{
	return FColor(235, 235, 245);
}

const TCHAR* SourceName(ESource Source)
{
	switch (Source)
	{
	case ESource::Dilated: return TEXT("dilatata");
	case ESource::Seeded:  return TEXT("seme");
	case ESource::None:    return TEXT("nessun asset");
	}
	return TEXT("?");
}

FPlan BuildPlan(const FInput& In)
{
	FPlan Out;

	// ⛔ **Nessun asset, nessuna griglia — e non e' un caso limite, e' IL caso.** Senza asset la board
	// disegna gia' `DemoRadius` celle che il dato non contiene (`RTHexMapActor.cpp:1870-1878`,
	// `DemoRadius = 4` di default, cioe' 61 esagoni). Aggiungerci sopra una griglia di lavoro
	// raddoppierebbe la bugia proprio nella configurazione in cui e' gia' in corso. Si tace, e il
	// chiamante lo dice a chi guarda.
	if (!In.bHasAsset)
	{
		Out.Source = ESource::None;
		return Out;
	}

	if (In.MaxCells <= 0)
	{
		// Un tetto a zero e' una richiesta legittima («non mostrarmela») e va distinta da «non c'e' dato».
		Out.Source = In.ExistingOnLayer.Num() > 0 ? ESource::Dilated : ESource::Seeded;
		Out.RequestedReach = FMath::Max(0, Out.Source == ESource::Seeded ? In.SeedRadius : In.Margin);
		Out.bClamped = Out.RequestedReach > 0;
		return Out;
	}

	// ── Ramo SEME: il layer attivo e' vuoto ───────────────────────────────────────────────────────────
	//
	// ⚠️ **`SeedRadius` e' una proprieta' distinta da `Margin`, e la distinzione e' il punto.** Un margine
	// dice «quanto oltre il gia' disegnato»; un seme dice «da dove si comincia quando non c'e' niente».
	// Usare lo stesso numero per due significati e' il modo in cui un parametro smette di essere
	// regolabile: chi lo alza per vedere piu' bordo si ritrova un seme enorme su una mappa vuota.
	//
	// ⛔ **E il seme NON e' `DemoRadius`**: quel campo vive sull'actor e questa funzione non lo vede
	// (vedi il docstring del namespace). E' una proprieta' del mode, che l'autore regola nel pannello.
	if (In.ExistingOnLayer.Num() == 0)
	{
		Out.Source = ESource::Seeded;
		Out.RequestedReach = FMath::Max(0, In.SeedRadius);

		int32 Reach = Out.RequestedReach;
		while (Reach > 0 && WorkGridAreaSize(Reach) > In.MaxCells)
		{
			--Reach;
			Out.bClamped = true;
		}
		Out.AppliedReach = Reach;

		if (WorkGridAreaSize(Reach) > In.MaxCells)
		{
			// Nemmeno la cella singola entra nel tetto: si dichiara il taglio invece di posarne una sola.
			Out.bClamped = true;
			return Out;
		}

		Out.Cells = URTHexLibrary::HexArea(FRTCellId(0, 0, In.Layer), Reach);
		Out.Cells.Sort(URTHexLibrary::StableLess);
		return Out;
	}

	// ── Ramo DILATATO: attorno a cio' che esiste gia' ─────────────────────────────────────────────────
	Out.Source = ESource::Dilated;
	Out.RequestedReach = FMath::Max(0, In.Margin);

	TSet<FRTCellId> Existing;
	Existing.Reserve(In.ExistingOnLayer.Num());
	for (const FRTCellId& Cell : In.ExistingOnLayer)
	{
		// ⚠️ Si normalizza il layer invece di fidarsi del chiamante: una cella di un altro piano finita qui
		// dentro produrrebbe fantasmi su un layer su cui i pennelli non scrivono, cioe' una promessa falsa.
		Existing.Add(FRTCellId(Cell.X, Cell.Y, In.Layer));
	}

	TSet<FRTCellId> Ghosts;
	TArray<FRTCellId> Frontier = Existing.Array();

	// 🔑 **Si cresce un anello per volta, e il tetto si controlla PRIMA di accettarlo.** E' cio' che rende
	// il troncamento «per anelli interi» un fatto del codice e non una speranza: o l'anello entra tutto, o
	// non entra per niente e `AppliedReach` lo dichiara.
	for (int32 Ring = 1; Ring <= Out.RequestedReach; ++Ring)
	{
		// ⚠️ **Un `TSet` e non un `TArray` con `AddUnique`.** Sei celle su sette della frontiera condividono
		// dei vicini: su una board piena la versione lineare fa milioni di confronti per un anello solo, e
		// questo giro avviene a ogni modifica dell'asset. L'ordine non si perde perche' non c'e' da
		// perderlo — `Cells` viene ordinato con `StableLess` in coda, ed e' li' che il determinismo vive.
		TSet<FRTCellId> Next;
		for (const FRTCellId& Cell : Frontier)
		{
			for (const FRTCellId& Neighbour : URTHexLibrary::Neighbors(Cell))
			{
				if (Existing.Contains(Neighbour) || Ghosts.Contains(Neighbour))
				{
					continue;
				}
				Next.Add(Neighbour);
			}
		}

		if (Next.Num() == 0)
		{
			// Il layer e' saturo in quella direzione: non c'e' altro da marcare, e non e' un troncamento.
			Out.AppliedReach = Ring;
			continue;
		}

		if (Ghosts.Num() + Next.Num() > In.MaxCells)
		{
			Out.bClamped = true;
			break;
		}

		Ghosts.Append(Next);
		Out.AppliedReach = Ring;
		Frontier = Next.Array();
	}

	Out.Cells = Ghosts.Array();
	Out.Cells.Sort(URTHexLibrary::StableLess);
	return Out;
}
}
