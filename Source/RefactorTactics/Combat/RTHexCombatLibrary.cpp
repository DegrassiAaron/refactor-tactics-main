#include "Combat/RTHexCombatLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h"
#include "Terrain/RTTerrainLibrary.h"

namespace
{
	/** Ordine canonico e senza duplicati: l'output non deve dipendere da come e' stato costruito. */
	void CanonicalizeCells(TArray<FRTCellId>& Cells)
	{
		Cells.Sort(URTHexLibrary::StableLess);
		for (int32 i = Cells.Num() - 1; i > 0; --i)
		{
			if (Cells[i] == Cells[i - 1]) { Cells.RemoveAt(i, EAllowShrinking::No); }
		}
	}
}

TArray<FRTCellId> URTHexCombatLibrary::HexHitCells(ERTAbilityShape Shape, const FRTCellId& From, const FRTCellId& Target,
	int32 RangeCells, int32 AreaRadius)
{
	TArray<FRTCellId> Cells;
	switch (Shape)
	{
	case ERTAbilityShape::Area:
		// Esagono pieno attorno al bersaglio (raggio 0 = solo la sua cella).
		Cells = URTHexLibrary::HexArea(Target, FMath::Max(0, AreaRadius));
		break;

	case ERTAbilityShape::Line:
		// Traiettoria attraversata, estremi inclusi da HexLine: la cella dell'attaccante non e' colpita
		// (parita' col quadrato, dove CellsInLine escludeva From).
		Cells = URTHexLibrary::HexLine(From, Target);
		Cells.Remove(From);
		break;

	case ERTAbilityShape::Cone:
		// Ventaglio 120 gradi verso il bersaglio (From gia' escluso da HexCone).
		Cells = URTHexLibrary::HexCone(From, Target, RangeCells);
		break;

	default:
		Cells.Add(Target);
		break;
	}

	CanonicalizeCells(Cells);
	return Cells;
}

namespace
{
	/**
	 * Primo bordo COPERTO che la traiettoria attraversa, andando da `From` verso `To`. E' la barriera che il
	 * colpo incontra per prima: quella oltre non la tocca nemmeno, perche' il colpo si ferma qui.
	 */
	bool FirstCoveredEdge(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
		FRTCellId& OutFrom, FRTCellId& OutTo)
	{
		const FRTCellId Origin(From.X, From.Y, To.Layer);
		if (Origin.X == To.X && Origin.Y == To.Y)
		{
			return false; // stessa cella assiale: nessun bordo attraversato
		}

		const TArray<FRTCellId> Line = URTHexLibrary::HexLine(Origin, To);
		for (int32 I = 1; I < Line.Num(); ++I)
		{
			if (URTHexCoverLibrary::CoverBetween(Map, Line[I - 1], Line[I]) != ERTHexCoverType::None)
			{
				OutFrom = Line[I - 1];
				OutTo = Line[I];
				return true;
			}
		}
		return false;
	}

	/**
	 * Prima PORTA che la traiettoria attraversa, andando da `From` verso `To`. Simmetrica a
	 * `FirstCoveredEdge`: e' il varco che il colpo incontra per primo, l'unico su cui puo' agire.
	 */
	bool FirstDoorEdge(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
		FRTCellId& OutFrom, FRTCellId& OutTo)
	{
		const FRTCellId Origin(From.X, From.Y, To.Layer);
		if (Origin.X == To.X && Origin.Y == To.Y)
		{
			return false; // stessa cella assiale: nessun bordo attraversato
		}

		const TArray<FRTCellId> Line = URTHexLibrary::HexLine(Origin, To);
		for (int32 I = 1; I < Line.Num(); ++I)
		{
			// Una porta APERTA e' comunque una porta: e' proprio quella che si vuole poter chiudere. Il
			// controllo va quindi sull'esistenza della voce, non sul fatto che blocchi.
			const FRTHexCellData* Near = Map->FindCell(Line[I - 1]);
			const FRTHexCellData* Far = Map->FindCell(Line[I]);
			ERTHexDirection Forward = ERTHexDirection::E;
			ERTHexDirection Backward = ERTHexDirection::E;
			if (!URTHexCoverLibrary::EdgeDirection(Line[I - 1], Line[I], Forward)
				|| !URTHexCoverLibrary::EdgeDirection(Line[I], Line[I - 1], Backward))
			{
				continue;
			}
			const bool bHasDoor = (Near != nullptr && Near->DoorOn(Forward) != nullptr)
				|| (Far != nullptr && Far->DoorOn(Backward) != nullptr);
			if (bHasDoor)
			{
				OutFrom = Line[I - 1];
				OutTo = Line[I];
				return true;
			}
		}
		return false;
	}

	/**
	 * Somma il danno su un bordo gia' presente, o ne aggiunge uno nuovo. La coppia di celle e' NORMALIZZATA
	 * (ordine stabile) perche' due attaccanti ai lati opposti colpiscono la stessa barriera: senza, il muro
	 * risulterebbe colpito due volte a meta' della forza.
	 */
	void AccumulateStructureHit(TArray<FRTStructureHit>& Hits, const FRTCellId& A, const FRTCellId& B,
		int32 Amount, int32 AttackerId)
	{
		const bool bForward = URTHexLibrary::StableLess(A, B);
		const FRTCellId& Low = bForward ? A : B;
		const FRTCellId& High = bForward ? B : A;

		for (FRTStructureHit& Existing : Hits)
		{
			if (Existing.From == Low && Existing.To == High)
			{
				Existing.Amount += Amount;
				Existing.AttackerId = FMath::Min(Existing.AttackerId, AttackerId); // deterministico, per il log
				return;
			}
		}
		Hits.Add(FRTStructureHit(Low, High, Amount, AttackerId));
	}
}

bool URTHexCombatLibrary::IsInFrontalArc(const FRTCellId& DefenderCell, ERTHexDirection Facing,
	const FRTCellId& OriginCell)
{
	const int32 Distance = URTHexLibrary::HexDistance(DefenderCell, OriginCell);
	if (Distance <= 0)
	{
		return true; // stessa cella: nessun lato da cui il colpo arrivi
	}

	// Il cono si costruisce verso il vicino nella direzione del facing, profondo quanto dista l'origine.
	// PLANARE come `HexCone` e `HexDistance`: chi spara da un piano piu' alto sta comunque davanti o dietro,
	// e l'origine si confronta proiettata sul layer del difensore invece di non essere mai trovata nel cono.
	const FRTCellId Ahead = URTHexLibrary::Neighbor(DefenderCell, Facing);
	const FRTCellId Projected(OriginCell.X, OriginCell.Y, DefenderCell.Layer);
	return URTHexLibrary::HexCone(DefenderCell, Ahead, Distance).Contains(Projected);
}

int32 URTHexCombatLibrary::HexCoverDamageReduction(const URTHexMapAsset* Map, const FRTCellId& From,
	const FRTCellId& Target, ERTAbilityShape Shape)
{
	// Un'area investe la cella da ogni lato: nessun bordo da attraversare, nessuna copertura che tenga.
	if (Map == nullptr || Shape == ERTAbilityShape::Area)
	{
		return 0;
	}

	const FRTHexCellData* Data = Map->FindCell(Target);
	if (Data == nullptr || Data->Covers.Num() == 0)
	{
		return 0;
	}

	// Bordo attraversato = ultimo passo della linea attaccante -> bersaglio, calcolata sul piano del
	// bersaglio (il Layer non entra nella geometria: i piani si collegano con archi espliciti).
	const FRTCellId Origin(From.X, From.Y, Target.Layer);
	if (Origin.X == Target.X && Origin.Y == Target.Y)
	{
		return 0; // stessa cella assiale: il colpo non attraversa nessun bordo
	}
	const TArray<FRTCellId> Line = URTHexLibrary::HexLine(Origin, Target);
	if (Line.Num() < 2)
	{
		return 0;
	}
	const FRTCellId& Previous = Line[Line.Num() - 2];

	// I sei vicini sono restituiti in ordine di direzione (E..SE): l'indice E' la direzione.
	const TArray<FRTCellId> Ring = URTHexLibrary::Neighbors(Target);
	for (int32 D = 0; D < Ring.Num(); ++D)
	{
		if (Ring[D].X == Previous.X && Ring[D].Y == Previous.Y)
		{
			return Data->CoverOn(static_cast<ERTHexDirection>(D)) == ERTHexCoverType::Low
				? URTCombatLibrary::LowCoverDamageReduction
				: 0;
		}
	}
	return 0;
}

int32 URTHexCombatLibrary::EffectiveCoverReduction(const URTHexMapAsset* Map,
	const FRTHexCombatUnit& Attacker, const FRTHexCombatUnit& Target, ERTAbilityShape Shape)
{
	const int32 Reduction = HexCoverDamageReduction(Map, Attacker.Cell, Target.Cell, Shape);

	// CP 16.2: l'emisfero posteriore e' SCOPERTO. Un colpo che non arriva dall'arco frontale annulla la
	// riduzione — non aggiunge danno. E' la differenza fra togliere una protezione e introdurre un bonus di
	// fianco: il secondo avrebbe richiesto un numero nuovo da bilanciare, il primo no.
	if (Reduction > 0 && !IsInFrontalArc(Target.Cell, Target.Facing, Attacker.Cell))
	{
		return 0;
	}
	return Reduction;
}

FRTHexAttackHit URTHexCombatLibrary::RedirectHitTo(int32 NewTargetId, const FRTHexAttackHit& Hit,
	const TArray<FRTHexCombatUnit>& Units, const TArray<FRTHexAttackIntent>& Intents,
	const URTHexMapAsset* Map)
{
	FRTHexAttackHit Out = Hit;
	Out.TargetId = NewTargetId;

	if (!Units.IsValidIndex(NewTargetId) || !Units.IsValidIndex(Hit.AttackerId)
		|| !Intents.IsValidIndex(Hit.IntentIndex))
	{
		// Niente da rivalidare: si conserva il colpo com'era. E' lo stesso fail-closed della raccolta —
		// senza attaccante, bersaglio o intento la geometria non e' valutabile, e inventarne una sarebbe
		// peggio che lasciare il dato che c'e'.
		return Out;
	}

	const FRTHexAttackIntent& Intent = Intents[Hit.IntentIndex];
	const int32 Reduction = EffectiveCoverReduction(Map, Units[Hit.AttackerId], Units[NewTargetId],
		Intent.Shape);
	Out.Power = FMath::Max(0, Intent.Power - Reduction);
	return Out;
}

FRTHexBlastPlan URTHexCombatLibrary::CollectHexAttacks(const TArray<FRTHexCombatUnit>& Units,
	const TArray<FRTHexAttackIntent>& Intents, const URTHexMapAsset* Map)
{
	FRTHexBlastPlan Plan;

	for (int32 IntentIdx = 0; IntentIdx < Intents.Num(); ++IntentIdx)
	{
		const FRTHexAttackIntent& Intent = Intents[IntentIdx];
		if (!Units.IsValidIndex(Intent.AttackerId))
		{
			continue;
		}

		const FRTHexCombatUnit& Attacker = Units[Intent.AttackerId];
		if (!Attacker.bAlive)
		{
			continue;
		}

		// Il bersaglio puo' essere un'unita' oppure una CELLA (`Fallback.AttackCell`: il bersaglio e' sparito,
		// l'area parte comunque dove era stata puntata). Da qui in poi conta solo dove si mira.
		const bool bTargetsUnit = Units.IsValidIndex(Intent.TargetId);
		const FRTCellId AimCell = bTargetsUnit ? Units[Intent.TargetId].Cell : Intent.TargetCell;

		if (bTargetsUnit)
		{
			const FRTHexCombatUnit& Target = Units[Intent.TargetId];
			// Un alleato e' un bersaglio legittimo SOLO per un'area a fuoco amico: e' la scelta di chi la
			// lancia, non un errore di mira da correggere in silenzio.
			const bool bSameTeam = Attacker.TeamId == Target.TeamId;
			if (!Target.bAlive || (bSameTeam && !Intent.bFriendlyFire))
			{
				continue; // bersaglio non ingaggiabile: nessun esito da registrare
			}
		}

		// Il Fumo lungo la linea di tiro CAPPA la portata effettiva (indipendente da RangeCells). La regola sta
		// in UN SOLO posto (URTTerrainLibrary): la stessa che usano la preview del giocatore
		// (ClassifyHexTargeting), la validazione degli ordini (ValidateInstance) e il bot (BuildCandidates) —
		// altrimenti quelli accettano un intento che qui viene scartato in silenzio.
		// Con `Map == nullptr` il cap e' un no-op: l'intento lo scarta comunque il fail-closed qui sotto.
		const int32 EffectiveRange =
			URTTerrainLibrary::EffectiveTargetingRange(Map, Attacker.Cell, AimCell, Intent.RangeCells);

		if (URTHexLibrary::HexDistance(Attacker.Cell, AimCell) > EffectiveRange)
		{
			continue; // fuori portata (anche per il cap del Fumo): scartato in silenzio (come il quadrato)
		}

		// FAIL-CLOSED: senza mappa autorevole non si colpisce. Il motivo resta pero' DISTINTO da una
		// copertura: «non valutabile» e' un difetto di configurazione del livello, non un esito di gioco.
		if (Map == nullptr)
		{
			Plan.UnverifiableIntents.Add(IntentIdx);
			continue;
		}

		// STRUTTURE (CP 9.2): il colpo che sbatte contro una barriera la danneggia. Si raccoglie PRIMA del
		// controllo sulla linea di tiro, perche' il caso principale e' proprio quello — il muro alto che
		// ferma il colpo e' anche l'unico bersaglio che l'attaccante puo' avere, visto che gli impedisce di
		// vedere chiunque stia dietro. Il danno si somma per bordo e viene applicato a fase conclusa.
		if (Intent.StructurePower > 0)
		{
			FRTCellId EdgeFrom, EdgeTo;
			if (FirstCoveredEdge(Map, Attacker.Cell, AimCell, EdgeFrom, EdgeTo))
			{
				AccumulateStructureHit(Plan.StructureHits, EdgeFrom, EdgeTo, Intent.StructurePower,
					Intent.AttackerId);
			}
		}

		// PORTE (CP 9.3): l'azione che dichiara `SetDoorState` agisce sulla prima porta che la sua traiettoria
		// attraversa. Come per le strutture si raccoglie PRIMA del controllo sulla linea di tiro — una porta
		// chiusa toglie la vista a chi sta oltre, e chiuderla sarebbe impossibile se il proprio effetto
		// venisse scartato dalla porta stessa.
		if (Intent.bChangesDoor)
		{
			FRTCellId DoorFrom, DoorTo;
			if (FirstDoorEdge(Map, Attacker.Cell, AimCell, DoorFrom, DoorTo))
			{
				Plan.DoorOps.Add(FRTDoorOp(DoorFrom, DoorTo, Intent.DoorState));
			}
		}
		if (!URTHexVisionLibrary::HasLineOfSight(Map, Attacker.Cell, AimCell))
		{
			Plan.BlockedIntents.Add(IntentIdx);
			continue;
		}

		const TArray<FRTCellId> HitCells =
			HexHitCells(Intent.Shape, Attacker.Cell, AimCell, Intent.RangeCells, Intent.AreaRadius);

		// Colpisce ogni unita' VIVA su una cella dell'area: i nemici sempre, gli alleati solo se l'azione
		// dichiara il fuoco amico. Chi lancia l'area non si colpisce mai da solo.
		for (int32 u = 0; u < Units.Num(); ++u)
		{
			const FRTHexCombatUnit& Other = Units[u];
			const bool bAlly = Other.TeamId == Attacker.TeamId;
			if (!Other.bAlive || u == Intent.AttackerId || (bAlly && !Intent.bFriendlyFire))
			{
				continue;
			}
			if (HitCells.Contains(Other.Cell))
			{
				// La copertura si applica QUI, sul singolo colpo: dipende da dove sta CHI lo subisce, non
				// dall'intento — due bersagli della stessa azione possono essere riparati in modo diverso.
				// Il danno si ferma a 0: il colpo resta avvenuto (trigger e marchi contano lo stesso).
				const int32 Reduction = EffectiveCoverReduction(Map, Attacker, Other, Intent.Shape);
				Plan.Hits.Add(FRTHexAttackHit(Intent.AttackerId, u,
					FMath::Max(0, Intent.Power - Reduction), IntentIdx));
			}
		}
	}

	// Ordine canonico: permutare gli intenti in ingresso non deve cambiare il piano in uscita.
	Plan.Hits.Sort([](const FRTHexAttackHit& A, const FRTHexAttackHit& B)
	{
		if (A.AttackerId != B.AttackerId) { return A.AttackerId < B.AttackerId; }
		if (A.TargetId != B.TargetId) { return A.TargetId < B.TargetId; }
		if (A.Power != B.Power) { return A.Power < B.Power; }
		return A.IntentIndex < B.IntentIndex; // ordine TOTALE: Sort non e' stabile
	});
	Plan.BlockedIntents.Sort();
	Plan.UnverifiableIntents.Sort();
	Plan.StructureHits.Sort([](const FRTStructureHit& A, const FRTStructureHit& B)
	{
		if (!(A.From == B.From)) { return URTHexLibrary::StableLess(A.From, B.From); }
		return URTHexLibrary::StableLess(A.To, B.To);
	});
	Plan.DoorOps.Sort([](const FRTDoorOp& A, const FRTDoorOp& B)
	{
		if (!(A.From == B.From)) { return URTHexLibrary::StableLess(A.From, B.From); }
		if (!(A.To == B.To)) { return URTHexLibrary::StableLess(A.To, B.To); }
		return static_cast<uint8>(A.State) < static_cast<uint8>(B.State); // ordine TOTALE: Sort non e' stabile
	});

	return Plan;
}

namespace
{
	/**
	 * Passo per passo lungo (StepX, StepY) a partire da Target, fino a Distance celle: si ferma sulla cella
	 * libera PRIMA di un bordo mappa, un ostacolo o una cella occupata. Nucleo comune di spinta e trazione —
	 * cambia solo la direzione con cui il chiamante lo invoca.
	 */
	FRTCellId StepUntilBlocked(const FRTCellId& Target, int32 StepX, int32 StepY, int32 Distance,
		const URTHexMapAsset* Map, const TArray<FRTCellId>& Occupied)
	{
		FRTCellId Current = Target;
		for (int32 Step = 0; Step < Distance; ++Step)
		{
			const FRTCellId Next(Current.X + StepX, Current.Y + StepY, Target.Layer);
			const FRTHexCellData* Data = Map->FindCell(Next);
			if (Data == nullptr || Data->bBlocksMovement || Occupied.Contains(Next))
			{
				break; // bordo della mappa, ostacolo o unita': ci si ferma sulla cella libera precedente
			}
			Current = Next;
		}
		return Current;
	}
}

FRTCellId URTHexCombatLibrary::HexKnockbackDestination(const FRTCellId& Attacker, const FRTCellId& Target, int32 Distance,
	const URTHexMapAsset* Map, const TArray<FRTCellId>& Occupied)
{
	// FAIL-CLOSED: senza mappa autorevole non si sposta nessuno (non si sa cosa c'e' oltre il bersaglio).
	if (Distance <= 0 || Map == nullptr)
	{
		return Target;
	}

	// Direzione della spinta: l'ULTIMO passo della linea attaccante -> bersaglio, cioe' una delle sei
	// direzioni esagonali. La linea si calcola sul piano del bersaglio (il Layer non entra nella geometria).
	const FRTCellId From(Attacker.X, Attacker.Y, Target.Layer);
	if (From.X == Target.X && From.Y == Target.Y)
	{
		return Target; // stessa cella assiale: nessuna direzione da cui allontanarsi
	}

	const TArray<FRTCellId> Line = URTHexLibrary::HexLine(From, Target);
	if (Line.Num() < 2)
	{
		return Target;
	}
	const FRTCellId& Previous = Line[Line.Num() - 2];
	// Si allontana: la direzione e' quella con cui si arriva AL bersaglio, proseguita oltre.
	return StepUntilBlocked(Target, Target.X - Previous.X, Target.Y - Previous.Y, Distance, Map, Occupied);
}

FRTCellId URTHexCombatLibrary::HexPullDestination(const FRTCellId& Puller, const FRTCellId& Target, int32 Distance,
	const URTHexMapAsset* Map, const TArray<FRTCellId>& Occupied)
{
	// FAIL-CLOSED: stessa disciplina di HexKnockbackDestination.
	if (Distance <= 0 || Map == nullptr)
	{
		return Target;
	}

	const FRTCellId From(Puller.X, Puller.Y, Target.Layer);
	if (From.X == Target.X && From.Y == Target.Y)
	{
		return Target; // stessa cella assiale: nessuna direzione verso cui avvicinarsi
	}

	const TArray<FRTCellId> Line = URTHexLibrary::HexLine(From, Target);
	if (Line.Num() < 2)
	{
		return Target;
	}
	const FRTCellId& Previous = Line[Line.Num() - 2];
	// Si avvicina: direzione INVERTITA rispetto alla spinta. La cella di chi tira, se e' fra Occupied
	// (lo e' sempre: e' un'unita' viva), ferma la trazione un passo prima — non si finisce mai addosso a chi
	// tira, per la stessa regola per cui non si finisce mai dentro un'altra unita'.
	return StepUntilBlocked(Target, -(Target.X - Previous.X), -(Target.Y - Previous.Y), Distance, Map, Occupied);
}

TArray<FRTAttack> URTHexCombatLibrary::ToAttacks(const FRTHexBlastPlan& Plan)
{
	TArray<FRTAttack> Attacks;
	Attacks.Reserve(Plan.Hits.Num());
	for (const FRTHexAttackHit& Hit : Plan.Hits)
	{
		Attacks.Add(FRTAttack(Hit.TargetId, Hit.Power));
	}
	return Attacks;
}
