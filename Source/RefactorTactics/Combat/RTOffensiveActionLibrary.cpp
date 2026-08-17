#include "Combat/RTOffensiveActionLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Terrain/RTTerrainLibrary.h" // il cap di portata e' un dato del terreno (`#1085`)
#include "Turn/RTMovementActionLibrary.h"

TArray<FRTCellId> URTOffensiveActionLibrary::LineCells(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& Toward, int32 RangeCells)
{
	TArray<FRTCellId> Cells;

	// FAIL-CLOSED: senza mappa autorevole non si sa dove sono le coperture, quindi non si traccia nulla.
	int32 Distance = 0;
	if (Map == nullptr || RangeCells <= 0 || !URTMovementActionLibrary::IsStraightLine(From, Toward, Distance))
	{
		return Cells;
	}

	// `Toward` sceglie la direzione, non la lunghezza: la linea arriva a fondo portata anche se il punto
	// mirato e' piu' vicino. Un colpo che si fermasse sul primo bersaglio mirato sarebbe l'attacco base.
	const FIntPoint UnitStep((Toward.X - From.X) / Distance, (Toward.Y - From.Y) / Distance);

	// ➕ **Il cap di portata attraverso il terreno** (`#1085`). `-1` = nessun cap attivo; `0` = esaurito.
	//
	// 🔴 Fino al 2026-08-17 `MaxTargetingRangeThrough` era **dichiarato, testato e mai letto**: il catalogo
	// dava `2` al fumo, `spec-terreni-e8.md` lo confermava in due punti — «il cap offensivo definito qui
	// resta» — e `RTTerrainTests` ne verificava il **valore**. Nessuna riga di produzione lo leggeva, quindi
	// il fumo non limitava niente e il test era verde lo stesso: e' la forma piu' insidiosa del dato senza
	// consumatore, perche' la copertura fa sembrare la feature viva.
	//
	// ⚠️ **E' un limite diverso da `bBlocksLineOfSight`, e i due convivono**: quello INTERROMPE la linea
	// prima della cella, questo la ACCORCIA dopo averla attraversata. Il fumo infatti non blocca la vista
	// (`bBlocksLineOfSight = false` nel catalogo): oscura, e chi spara attraverso non vede lontano.
	int32 CapResiduo = -1;

	for (int32 K = 1; K <= RangeCells; ++K)
	{
		if (CapResiduo == 0)
		{
			break; // il terreno attraversato ha esaurito la portata che concedeva
		}

		const FRTCellId Next(From.X + UnitStep.X * K, From.Y + UnitStep.Y * K, From.Layer);

		const FRTHexCellData* Data = Map->FindCell(Next);
		if (Data == nullptr)
		{
			break; // bordo della mappa: la linea finisce dove finisce il livello
		}
		if (Data->bBlocksLineOfSight)
		{
			break; // copertura alta: interrompe la linea PRIMA della cella, che non viene controllata
		}

		Cells.Add(Next);
		if (CapResiduo > 0)
		{
			--CapResiduo;
		}

		// Il cap si arma DOPO aver aggiunto la cella: si conta da quella di fumo in avanti. L'alternativa —
		// limitare la portata totale — non regge, perche' con il fumo alla quinta cella e un cap di 2 la
		// linea dovrebbe fermarsi PRIMA di aver incontrato cio' che la limita: un effetto che precede la
		// sua causa.
		//
		// ⚠️ Fra piu' cap attivi vince il **minimo**: due banchi di fumo non possono allungare la portata.
		// Il valore arriva dal CATALOGO, non da una costante qui: cambiare il numero nei dati cambia il
		// comportamento senza toccare il resolver.
		const int32 Cap = URTTerrainLibrary::FindTerrainDef(Data->Surface).MaxTargetingRangeThrough;
		if (Cap > 0)
		{
			CapResiduo = (CapResiduo < 0) ? Cap : FMath::Min(CapResiduo, Cap);
		}
	}

	return Cells;
}

FRTLineAttackResult URTOffensiveActionLibrary::ResolveLineAttack(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& Toward, int32 RangeCells, const TMap<FRTCellId, int32>& Occupancy,
	const TSet<int32>& Hostiles)
{
	FRTLineAttackResult Result;

	if (Map == nullptr)
	{
		Result.Stop = ERTLineStop::NoMap;
		return Result;
	}

	int32 Distance = 0;
	if (RangeCells <= 0 || !URTMovementActionLibrary::IsStraightLine(From, Toward, Distance))
	{
		Result.Stop = ERTLineStop::NotAligned;
		return Result;
	}

	// La stessa geometria della soppressione: chi si copre da un attacco lineare si copre anche da quella.
	// Ricavare da qui il motivo dello stop costa una scansione in piu' della linea completa, ma evita di
	// riscrivere l'avanzamento cella per cella in due posti (che e' il modo in cui le due regole divergono).
	const TArray<FRTCellId> Traced = LineCells(Map, From, Toward, RangeCells);

	for (const FRTCellId& Cell : Traced)
	{
		Result.Cells.Add(Cell);

		const int32* Occupant = Occupancy.Find(Cell);
		if (Occupant != nullptr && Hostiles.Contains(*Occupant))
		{
			// Primo bersaglio VALIDO: il colpo si ferma qui. Un alleato invece non e' un bersaglio e non
			// interrompe la linea — a interromperla e' solo la copertura alta (catalogo §3).
			Result.HitUnitId = *Occupant;
			Result.Stop = ERTLineStop::Hit;
			return Result;
		}
	}

	// Nessun bersaglio: il motivo dice se la linea si e' esaurita, e' finita fuori mappa o l'ha fermata una
	// copertura. Sono difetti diversi da correggere per chi gioca, e confonderli rende il log una bugia.
	if (Traced.Num() >= RangeCells)
	{
		Result.Stop = ERTLineStop::NoTarget;
		return Result;
	}

	const FIntPoint UnitStep((Toward.X - From.X) / Distance, (Toward.Y - From.Y) / Distance);
	const int32 K = Traced.Num() + 1; // la prima cella NON tracciata: e' quella che ha fermato la linea
	const FRTCellId Blocker(From.X + UnitStep.X * K, From.Y + UnitStep.Y * K, From.Layer);
	Result.Stop = (Map->FindCell(Blocker) == nullptr) ? ERTLineStop::OffMap : ERTLineStop::BlockedByCover;
	return Result;
}

FRTSuppressiveZone URTOffensiveActionLibrary::MakeSuppressiveZone(const URTHexMapAsset* Map, int32 OwnerUnitId,
	int32 OwnerTeamId, const FRTCellId& From, const FRTCellId& Toward, int32 RangeCells, int32 Damage)
{
	FRTSuppressiveZone Zone;
	Zone.OwnerUnitId = OwnerUnitId;
	Zone.OwnerTeamId = OwnerTeamId;
	Zone.Cells = LineCells(Map, From, Toward, RangeCells);
	Zone.Damage = FMath::Max(0, Damage);
	return Zone;
}

FRTSuppressionHit URTOffensiveActionLibrary::ResolveSuppression(const FRTSuppressiveZone& Zone,
	const TArray<FRTSuppressionMover>& Movers)
{
	FRTSuppressionHit Hit;
	if (Zone.Cells.Num() == 0 || Zone.Damage <= 0)
	{
		return Hit; // linea mai preparata, o preparata dove non c'e' nulla da controllare
	}

	// TSet per l'appartenenza, mai per l'ordine: l'ordine di scansione e' quello dei micro-step, che e'
	// dichiarato dai percorsi.
	const TSet<FRTCellId> Controlled(Zone.Cells);

	// Ordine TOTALE del trigger: micro-step piu' basso, poi UnitId piu' basso. Non l'ordine di `Movers` —
	// altrimenti chi la prende dipenderebbe da come il chiamante ha costruito l'array, cioe' da nulla di
	// osservabile in partita.
	for (const FRTSuppressionMover& Mover : Movers)
	{
		if (Mover.TeamId == Zone.OwnerTeamId || Mover.UnitId == Zone.OwnerUnitId)
		{
			continue; // la linea non scatta sui propri
		}

		for (int32 Step = 0; Step < Mover.Path.Num(); ++Step)
		{
			if (!Controlled.Contains(Mover.Path[Step]))
			{
				continue;
			}

			const bool bEarlier = !Hit.IsValid()
				|| Step < Hit.StepIndex
				|| (Step == Hit.StepIndex && Mover.UnitId < Hit.VictimUnitId);
			if (bEarlier)
			{
				Hit.VictimUnitId = Mover.UnitId;
				Hit.StepIndex = Step;
				Hit.StopCell = Mover.Path[Step];   // ci resta: la linea ferma il movimento, non lo annulla
				Hit.Damage = Zone.Damage;
			}
			break; // di questa unita' conta solo il PRIMO ingresso: i passi successivi non aggiungono nulla
		}
	}

	return Hit;
}
