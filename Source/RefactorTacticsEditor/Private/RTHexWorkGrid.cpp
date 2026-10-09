#include "RTHexWorkGrid.h"

#include "Map/RTHexLibrary.h"

namespace RTHexWorkGrid
{
namespace
{
	/**
	 * Quante celle ha un esagono pieno di raggio `R`, in `int64`.
	 *
	 * ⚠️ **`int64` e non `int32`, e non e' pignoleria.** `3·R·(R+1)+1` supera l'`int32` gia' a `R ≈ 26 000`,
	 * e il raggio arriva qui da una `UPROPERTY(config)` che `LoadConfig()` legge da un `.ini` **senza**
	 * applicare il `ClampMax` del pannello. In overflow il risultato puo' uscire piccolo o negativo, il
	 * confronto col tetto passa, e `HexArea` parte su un ciclo che non finisce.
	 */
	int64 WorkGridAreaSize(int32 Radius)
	{
		const int64 R = FMath::Max(0, Radius);
		return 3 * R * (R + 1) + 1;
	}
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

FRTCellId AnchorFor(const TArray<FRTCellId>& Cells, int32 Layer)
{
	if (Cells.Num() == 0)
	{
		return FRTCellId(0, 0, Layer);
	}

	// ⚠️ Somma in `int64`: una mappa autorata lontano puo' avere coordinate grandi, e la somma su molte
	// celle e' il posto in cui un `int32` si rompe per primo.
	int64 SumX = 0;
	int64 SumY = 0;
	for (const FRTCellId& Cell : Cells)
	{
		SumX += Cell.X;
		SumY += Cell.Y;
	}

	// `RoundToInt` sulla divisione e non troncamento: con coordinate negative il troncamento sposta
	// l'ancora verso lo zero, cioe' proprio verso l'origine da cui questa funzione esiste per staccarsi.
	const int32 MeanX = FMath::RoundToInt(static_cast<double>(SumX) / Cells.Num());
	const int32 MeanY = FMath::RoundToInt(static_cast<double>(SumY) / Cells.Num());
	return FRTCellId(MeanX, MeanY, Layer);
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

	// I due raggi si restringono QUI: il `ClampMax` del pannello non sopravvive a `LoadConfig()`.
	const int32 Margin = FMath::Clamp(In.Margin, 0, MaxReach);
	const int32 SeedRadius = FMath::Clamp(In.SeedRadius, 0, MaxReach);

	if (In.MaxCells <= 0)
	{
		// Un tetto a zero e' una richiesta legittima («non mostrarmela») e va distinta da «non c'e' dato».
		Out.Source = In.ExistingOnLayer.Num() > 0 ? ESource::Dilated : ESource::Seeded;
		Out.RequestedReach = Out.Source == ESource::Seeded ? SeedRadius : Margin;
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
		Out.RequestedReach = SeedRadius;

		int32 Reach = SeedRadius;
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

		// 🔑 **Attorno a `SeedAnchor`, non all'origine.** Il perche' sta nel docstring di `AnchorFor`: su
		// una mappa autorata lontano, seminare su `(0,0)` disegna la griglia dove non c'e' nulla e lascia
		// senza fantasmi proprio le coordinate sopra le celle esistenti.
		Out.Cells = URTHexLibrary::HexArea(FRTCellId(In.SeedAnchor.X, In.SeedAnchor.Y, In.Layer), Reach);
		Out.Cells.Sort(URTHexLibrary::StableLess);
		return Out;
	}

	// ── Ramo DILATATO: attorno a cio' che esiste gia' ─────────────────────────────────────────────────
	Out.Source = ESource::Dilated;
	Out.RequestedReach = Margin;

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

		if (Ghosts.Num() + Next.Num() > In.MaxCells)
		{
			Out.bClamped = true;

			// 🔴 **L'unica eccezione al troncamento per anelli interi, e solo dove l'alternativa e' il
			// nulla.** Se anche il PRIMO anello sfonda il tetto, fermarsi qui lascerebbe la griglia vuota
			// su una mappa enorme o frammentata — cioe' proprio dove vedere il bordo serve di piu', e il
			// primo criterio del DoD non sarebbe soddisfatto. Si prende un pezzo di anello in ordine
			// stabile e lo si **dichiara** con `bPartialRing`. Dal secondo anello in poi la regola resta
			// intera: li' un pezzo non aggiungerebbe una vista, la renderebbe irregolare.
			if (Ring == 1 && Ghosts.Num() == 0)
			{
				TArray<FRTCellId> Parziale = Next.Array();
				Parziale.Sort(URTHexLibrary::StableLess);
				Parziale.SetNum(FMath::Min(Parziale.Num(), In.MaxCells));
				Out.Cells = MoveTemp(Parziale);
				Out.bPartialRing = true;
				return Out;
			}
			break;
		}

		Ghosts.Append(Next);
		Out.AppliedReach = Ring;

		// ⚠️ Nessun caso speciale per `Next` vuoto: una frontiera vuota fa girare a vuoto i giri rimanenti
		// senza costo, e `AppliedReach` arriva onestamente a quello chiesto. La stesura precedente aveva un
		// ramo apposito che rimetteva la STESSA frontiera in gioco a ogni anello e dichiarava applicato un
		// anello che non aveva posato niente — il log diceva `anelli 6/6` per una griglia ferma al primo.
		Frontier = Next.Array();
	}

	Out.Cells = Ghosts.Array();
	Out.Cells.Sort(URTHexLibrary::StableLess);
	return Out;
}
}
