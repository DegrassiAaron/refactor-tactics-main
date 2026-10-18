#include "Turn/RTMovementActionLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h"

namespace
{
	/** La cella esiste sulla mappa ed e' percorribile (il muro e' un dato della cella, non un'eccezione). */
	bool IsWalkable(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		const FRTHexCellData* Data = Map ? Map->FindCell(Cell) : nullptr;
		return Data != nullptr && !Data->bBlocksMovement;
	}

	/**
	 * La cella e' attraversabile da una mobilita' che AVANZA passo passo (Dash, Charge, Reposition).
	 *
	 * Oltre al muro c'e' il terreno: il catalogo dice che l'accidentato «Dash e Charge non lo attraversano»
	 * (E8/CP 8.1), e il dato che lo dichiara e' `bBlocksDashCharge`. A piedi ci si passa lo stesso, pagando
	 * il costo maggiore — il movimento normale non passa di qui.
	 *
	 * Il SALTO non usa questo predicato: scavalca le celle intermedie, e il terreno accidentato non ostacola
	 * chi ci passa sopra. Atterrarci resta permesso (`IsWalkable`).
	 */
	bool IsTraversableByStep(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		if (!IsWalkable(Map, Cell)) { return false; }
		const FRTHexCellData* Data = Map->FindCell(Cell);
		return !URTTerrainLibrary::FindTerrainDef(Data->Surface).bBlocksDashCharge;
	}

	/**
	 * QUALE delle due cose ha fermato il passo, per chi deve poi spiegarlo.
	 *
	 * `IsTraversableByStep` risponde si'/no ed e' cio' che serve al resolver; questa dice il motivo, che
	 * serve al TurnLog. Le due restano separate di proposito: il predicato e' usato anche dove il motivo
	 * non interessa, e restituire una coppia obbligherebbe ogni chiamante a scartarne meta'.
	 *
	 * Precedenza: il MURO prima del terreno. Una cella puo' essere entrambe le cose, e in quel caso la
	 * lezione utile e' quella che vale anche a piedi.
	 */
	ERTLinearStop StepBlockReason(const URTHexMapAsset* Map, const FRTCellId& Cell)
	{
		if (!IsWalkable(Map, Cell)) { return ERTLinearStop::BlockedByTerrain; }

		// ⚠️ **Il flag si MISURA, non si deduce per eliminazione.** Una prima stesura restituiva
		// `TerrainDeniesDash` senza guardare `bBlocksDashCharge`, contando sul fatto che l'unico altro modo
		// di non essere attraversabili fosse il muro. Il giorno in cui `IsTraversableByStep` guadagna una
		// terza condizione — una porta chiusa, un dislivello, un hazard — quella si presenterebbe al
		// giocatore come «il terreno non si attraversa di corsa», che e' precisamente il difetto di causa
		// sbagliata che questo lavoro rimuove, spostato di un livello e senza segnale di compilazione.
		const FRTHexCellData* Data = Map->FindCell(Cell);
		if (Data && URTTerrainLibrary::FindTerrainDef(Data->Surface).bBlocksDashCharge)
		{
			return ERTLinearStop::TerrainDeniesDash;
		}
		return ERTLinearStop::BlockedByTerrain;
	}
}

bool URTMovementActionLibrary::IsStraightLine(const FRTCellId& From, const FRTCellId& To, int32& OutDistance)
{
	OutDistance = 0;
	if (From.Layer != To.Layer || From == To)
	{
		return false; // le mobilita' lineari restano sul proprio piano; il salto di layer e' E9 (rampe/ponti)
	}

	const int32 DeltaX = To.X - From.X;
	const int32 DeltaY = To.Y - From.Y;

	// Allineata se lo scostamento e' un multiplo POSITIVO di una delle sei direzioni assiali.
	for (int32 D = 0; D < 6; ++D)
	{
		const FIntPoint Step = URTHexLibrary::AxialDirection(static_cast<ERTHexDirection>(D));
		for (int32 K = 1; K <= FMath::Max(FMath::Abs(DeltaX), FMath::Abs(DeltaY)); ++K)
		{
			if (Step.X * K == DeltaX && Step.Y * K == DeltaY)
			{
				OutDistance = K;
				return true;
			}
		}
	}
	return false;
}

bool URTMovementActionLibrary::MovementLine(const FRTCellId& From, const FRTCellId& To, TArray<FRTCellId>& OutLine)
{
	OutLine.Reset();
	if (From.Layer != To.Layer || From == To)
	{
		return false; // le mobilita' lineari restano sul proprio piano; il salto di layer e' E9 (rampe/ponti)
	}

	// [D-524] (2): la retta e' quella di `HexLine`, la stessa di vista, forme e cono, col suo spareggio intero. Qui
	// non c'e' un'altra geometria e non c'e' uno spareggio proprio: `HexLine` e' planare, e il vincolo di piano sopra
	// e' l'unica cosa che questa funzione aggiunge — la stessa che `IsStraightLine` ha sempre avuto.
	OutLine = URTHexLibrary::HexLine(From, To);
	return OutLine.Num() >= 2;
}

FRTLinearMoveResult URTMovementActionLibrary::ResolveLinearMove(const URTHexMapAsset* Map,
	const FRTCellId& From, const FRTCellId& Target, int32 MaxCells, ERTMovementStyle Style,
	const TMap<FRTCellId, int32>& Occupancy, const TSet<int32>& Hostiles)
{
	FRTLinearMoveResult Result;
	Result.Final = From;
	Result.Stop = ERTLinearStop::NotAligned;

	// FAIL-CLOSED: senza mappa autorevole non si sa cosa ci sia davanti, quindi non ci si muove.
	TArray<FRTCellId> Line;
	if (Map == nullptr || MaxCells <= 0 || !MovementLine(From, Target, Line))
	{
		return Result;
	}
	const int32 Distance = Line.Num() - 1; // [D-524] (3): la portata e' la distanza esagonale, i passi di `HexLine`

	// 🔑 **[D-516] Una CARICA su un bersaglio oltre la portata non si annulla: corre per tutta la portata.** Ogni
	// altro stile resta com'era — oltre la portata non parte. La retta si costruisce FINO AL BERSAGLIO e poi si
	// tronca alla portata ([D-524] (3)): il prefisso di `HexLine(From, Target)` non e' `HexLine(From, <cella a
	// distanza MaxCells>)`, e il bersaglio e' cio' che la carica ha scelto. Gli ostacoli entro la portata la fermano
	// come sempre: un nemico incontrato e' un impatto.
	const bool bChargeBeyondRange = Distance > MaxCells && Style == ERTMovementStyle::LinearCharge;
	if (Distance > MaxCells && !bChargeBeyondRange)
	{
		return Result;
	}
	const int32 Steps = bChargeBeyondRange ? MaxCells : Distance;
	Result.bTargetBeyondRange = bChargeBeyondRange;

	// Il salto guarda solo dove atterra: le celle in mezzo non lo riguardano (unita', coperture basse), ma la
	// cella d'arrivo si' — la si "subisce", quindi dev'essere percorribile e libera.
	if (Style == ERTMovementStyle::LinearLeap)
	{
		if (!IsWalkable(Map, Target))
		{
			Result.Stop = ERTLinearStop::BlockedByTerrain;
			return Result;
		}
		if (Occupancy.Contains(Target))
		{
			Result.Stop = ERTLinearStop::BlockedByUnit;
			return Result;
		}
		Result.Final = Target;
		Result.Entered.Add(Target);
		Result.Stop = ERTLinearStop::Completed;
		return Result;
	}

	// Dash, Charge e Reposition avanzano una cella alla volta: cio' che incontrano le ferma.
	FRTCellId Current = From;
	for (int32 K = 1; K <= Steps; ++K)
	{
		const FRTCellId& Next = Line[K];

		// 🔑 **[D-524] (5) Il BORDO prima della cella**: da `Current` a `Next` si attraversa il lato che le divide, e
		// solo dopo si entra nella cella. Copertura alta e porta chiusa o bloccata lo negano, e
		// `URTHexCoverLibrary::BlocksTraversal` e' l'unica funzione che vista, grafo e combat interrogano per
		// dirlo. Gli scatti erano l'eccezione (`IMPLEMENTATION DRIFT`, `RT_TestMatrix_v0.1.md` riga 3). Le due
		// celle sono consecutive su `HexLine`, quindi adiacenti e sullo stesso piano. Il salto non passa di qui.
		if (URTHexCoverLibrary::BlocksTraversal(Map, Current, Next))
		{
			Result.Stop = ERTLinearStop::BlockedByEdge;
			break;
		}

		if (!IsTraversableByStep(Map, Next))
		{
			// Muro/bordo e terreno-che-nega-lo-scatto erano lo STESSO esito, e il commento che stava qui
			// («muro, bordo mappa, o terreno che nega lo scatto») lo dichiarava senza che nulla a valle
			// potesse distinguerli. Ora la causa viaggia, perche' e' cio' che il TurnLog deve poter dire.
			Result.Stop = StepBlockReason(Map, Next);
			break;
		}

		if (const int32* Occupant = Occupancy.Find(Next))
		{
			// La lama ATTRAVERSA e tira dritto — tranne che sulla cella d'arrivo: si passa in mezzo a
			// qualcuno, non ci si ferma dentro. Due unita' nella stessa cella non sono rappresentabili, e
			// un'eccezione qui la renderebbe possibile per una sola azione.
			if (Style == ERTMovementStyle::LinearPass && K < Distance)
			{
				Result.PassedThroughUnitIds.Add(*Occupant);
				Current = Next;
				Result.Entered.Add(Next);
				continue;
			}

			// La carica si ferma ADIACENTE al nemico e lo colpisce (impatto); per tutti gli altri e' solo un
			// ostacolo. Il `break` qui sotto lascia `Current` dov'e', quindi `Final` e' la cella PRECEDENTE
			// all'occupante: e' la regola, non un dettaglio del ciclo — [D-296].
			Result.Stop = (Style == ERTMovementStyle::LinearCharge && Hostiles.Contains(*Occupant))
				? ERTLinearStop::Impact
				: ERTLinearStop::BlockedByUnit;
			if (Result.Stop == ERTLinearStop::Impact)
			{
				Result.ImpactUnitId = *Occupant;
			}
			break;
		}

		Current = Next;
		Result.Entered.Add(Next);
		if (K == Steps)
		{
			// [D-516]: l'ultima cella della portata non e' il bersaglio, e l'esito lo dice.
			Result.Stop = bChargeBeyondRange ? ERTLinearStop::OutOfRange : ERTLinearStop::Completed;
		}
	}

	Result.Final = Current;
	return Result;
}

bool URTMovementActionLibrary::IsLineOpenToTarget(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& Target)
{
	TArray<FRTCellId> Line;
	if (!MovementLine(From, Target, Line))
	{
		return false; // anche `From == Target`, e un altro piano: non c'e' una retta da percorrere
	}
	// `IsLinearReachable` con uno stile a passi, la portata pari alla distanza e nessuno sulla strada: contano solo
	// il terreno e i lati ([D-524] (5)). Un predicato solo sulla stessa chiamata, non un secondo (review della PR #3767).
	const TMap<FRTCellId, int32> Nessuno;
	const TSet<int32> NessunNemico;
	return IsLinearReachable(Map, From, Target, Line.Num() - 1, ERTMovementStyle::LinearDash, Nessuno, NessunNemico);
}

FRTChargeRun URTMovementActionLibrary::DescribeChargeRun(const FRTLinearMoveResult& Linear, const FRTCellId& From,
	const FRTCellId& Target, int32 Range)
{
	FRTChargeRun Run;
	Run.From = From;
	Run.Target = Target;
	Run.Range = Range;
	Run.RunCells = Linear.Entered.Num();

	// L'unita' urtata sta sulla cella che SEGUE l'ultima percorsa, perche' la carica si ferma ADIACENTE a lei ([D-296]):
	// `Final` e' la cella precedente. La si legge dalla retta, che e' la stessa del resolver, invece di cercarla fra le unita'.
	if (Linear.Stop == ERTLinearStop::Impact && Linear.ImpactUnitId != INDEX_NONE)
	{
		TArray<FRTCellId> Retta;
		if (MovementLine(From, Target, Retta) && Retta.IsValidIndex(Run.RunCells + 1))
		{
			Run.bHasImpact = true;
			Run.ImpactCell = Retta[Run.RunCells + 1];
		}
	}

	// Arriva sulla cella scelta, oppure urta proprio chi sta li': in tutti gli altri casi — la portata finisce prima, un altro
	// nemico e' in mezzo, un ostacolo — la carica manca il bersaglio, e il tratto che resta e' quello che non fara'.
	const bool bArriva = Linear.Stop == ERTLinearStop::Completed;
	const bool bUrtaIlBersaglio = Run.bHasImpact && Run.ImpactCell == Target;
	Run.bMissesTarget = !(bArriva || bUrtaIlBersaglio);
	return Run;
}

bool URTMovementActionLibrary::IsLinearReachable(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& Target, int32 MaxCells, ERTMovementStyle Style,
	const TMap<FRTCellId, int32>& Occupancy, const TSet<int32>& Hostiles)
{
	if (From == Target)
	{
		return true; // "resto dove sono" e' sempre praticabile
	}
	return ResolveLinearMove(Map, From, Target, MaxCells, Style, Occupancy, Hostiles).Final == Target;
}
