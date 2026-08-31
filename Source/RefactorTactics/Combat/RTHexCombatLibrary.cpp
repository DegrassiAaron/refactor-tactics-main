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
	 * C'e' una porta sul bordo fra due celle ADIACENTI?
	 *
	 * Una porta APERTA e' comunque una porta — e' proprio quella che si vuole poter chiudere — quindi il
	 * controllo e' sull'esistenza della voce, non sul fatto che blocchi. E si legge da ENTRAMBI i lati: la
	 * voce puo' vivere sull'una o sull'altra cella, e guardarne una sola renderebbe la stessa porta presente
	 * o assente a seconda di da dove la si guarda.
	 *
	 * Esiste come funzione perche' la regola era scritta due volte: dentro `FirstDoorEdge` e nel ramo del
	 * bordo dichiarato di CP 10.1. Due copie della stessa lettura divergono alla prima modifica di una sola.
	 */
	bool HasDoorOnEdge(const URTHexMapAsset* Map, const FRTCellId& A, const FRTCellId& B)
	{
		if (Map == nullptr) { return false; }

		ERTHexDirection Forward = ERTHexDirection::E;
		ERTHexDirection Backward = ERTHexDirection::E;
		if (!URTHexCoverLibrary::EdgeDirection(A, B, Forward)
			|| !URTHexCoverLibrary::EdgeDirection(B, A, Backward))
		{
			return false; // non adiacenti: non c'e' un bordo da guardare
		}

		const FRTHexCellData* Near = Map->FindCell(A);
		const FRTHexCellData* Far = Map->FindCell(B);
		return (Near != nullptr && Near->DoorOn(Forward) != nullptr)
			|| (Far != nullptr && Far->DoorOn(Backward) != nullptr);
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
			if (HasDoorOnEdge(Map, Line[I - 1], Line[I]))
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
	// La redirezione cambia il BERSAGLIO, quindi cambia anche la geometria della copertura: ricalcolare il
	// danno e lasciare la vecchia attribuzione direzionale darebbe una traccia che parla della vittima
	// precedente.
	const int32 Nominal = HexCoverDamageReduction(Map, Units[Hit.AttackerId].Cell, Units[NewTargetId].Cell,
		Intent.Shape);
	Out.CoverBypassedByFacing = FMath::Max(0, Nominal - Reduction);
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
			bool bFoundDoor = false;

			if (Intent.bHasDeclaredDoorEdge)
			{
				// 🔴 **Il bordo DICHIARATO vince sulla traiettoria** (CP 10.1, `#74`, difetto aperto da
				// [D-149]). Una cella adiacente ha sei facce, e `FirstDoorEdge` ne guarda una sola: quella
				// condivisa con l'attaccante. Se il giocatore ne ha cliccata un'altra, la traiettoria o non
				// trovava niente — silenzio — o trovava **un'altra porta**, e l'apriva.
				//
				// 🔴 **Il bordo appartiene alla cella BERSAGLIO, non a quella dell'attaccante**, ed e' il
				// contratto che il puntatore stabilisce: `HandleTargetEdge(Cell, Edge)` scrive
				// `PlannedAttackCell = Cell` e `PlannedCoverEdge = Edge`, e il ramo delle COPERTURE lo legge
				// cosi' — `Pending.Add({ TargetCell, Edge, ... })` poi `AddCover(Map, Op.Cell, Op.Edge, ...)`.
				// La prima stesura ancorava all'attaccante: con il giocatore in `(0,0)` e la porta verso
				// `(1,0)`, il puntatore produce `(Cell=(1,0), Edge=W)` e la ricerca finiva su `(-1,0)` —
				// l'azione veniva RIFIUTATA e la porta cliccata restava chiusa. Trovato in code review; i due
				// test non lo vedevano perche' la fixture costruiva il piano con la stessa assunzione
				// sbagliata invece che come lo costruisce il puntatore.
				const FRTCellId Beyond = URTHexLibrary::Neighbor(AimCell, Intent.DeclaredDoorEdge);
				if (HasDoorOnEdge(Map, AimCell, Beyond))
				{
					DoorFrom = AimCell;
					DoorTo = Beyond;
					bFoundDoor = true;
				}
			}
			else
			{
				// Nessun bordo dichiarato: resta il comportamento di CP 9.3, dove l'operazione nasce dalla
				// traiettoria di un colpo e non da un click — un'azione ad area che apre la porta che
				// attraversa non ha un bordo scelto da nessuno.
				bFoundDoor = FirstDoorEdge(Map, Attacker.Cell, AimCell, DoorFrom, DoorTo);
			}

			if (bFoundDoor)
			{
				Plan.DoorOps.Add(FRTDoorOp(DoorFrom, DoorTo, Intent.DoorState, Intent.AttackerId));
			}
			else
			{
				// ⛔ **Il caso che prima era silenzioso.** L'azione e' stata dichiarata e valutata, e non ha
				// trovato niente su cui agire: lo dice, invece di lasciare che il turno passi come se il
				// giocatore non avesse fatto nulla.
				Plan.DoorlessIntents.Add(IntentIdx);

				// 🔴 **E si FERMA qui.** Senza il `continue` lo stesso intento proseguiva e poteva finire
				// anche in `BlockedIntents` — una cella oltre un muro basta — e il TurnLog riceveva DUE voci
				// con motivi incompatibili per una sola azione: «nessuna linea di tiro» accanto a «non aveva
				// niente su cui agire». Un'azione ha un motivo, quello vero.
				continue;
			}
		}
		if (!URTHexVisionLibrary::HasLineOfSight(Map, Attacker.Cell, AimCell))
		{
			Plan.BlockedIntents.Add(IntentIdx);
			continue;
		}

		// Un colpo nasce solo da cio' che si DICHIARA aggressione ([`INT-8`], `#1491`). Il cancello sta QUI e
		// non nel trigger delle reazioni ne' nel guadagno d'energia: il colpo e' un concetto SOLO, quindi i suoi
		// quattro consumatori -- danno, `HitByDirectAttack`, `EnergyOnHit` e `Marked` -- lo ereditano da un punto
		// unico invece di ricontrollarlo ciascuno a modo proprio. Prima il danno a 0 li lasciava passare tutti:
		// `Action.Interact` puntata su un'unita' incassava un contrattacco e caricava 15 di energia per aver
		// aperto una porta.
		//
		// ⚠️ DOPO la raccolta dell'op sulla porta, e non prima: `Action.Interact` non colpisce nessuno, ma il suo
		// lavoro vero e' gia' in `Plan.DoorOps` qui sopra. Spegnerla piu' in alto la renderebbe inerte.
		// ⚠️ E dopo la linea di tiro, cosi' un'azione non-aggressiva bloccata resta registrata in
		// `Plan.BlockedIntents`: non produce colpi, ma il motivo per cui non li produce non e' quello.
		if (!Intent.bCountsAsAttack)
		{
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

				// Quanto la DIREZIONE ha annullato: la differenza fra la copertura che il bordo offre e quella
				// che vale davvero da questa provenienza. Si calcola qui e non altrove perche' qui ci sono
				// entrambe le figure — chi spara e chi subisce — e `EffectiveCoverReduction` non puo' dirlo da
				// sola: restituisce `0` sia quando la copertura non c'e' sia quando la direzione l'ha tolta.
				const int32 Nominal = HexCoverDamageReduction(Map, Attacker.Cell, Other.Cell, Intent.Shape);
				Plan.Hits.Add(FRTHexAttackHit(Intent.AttackerId, u,
					FMath::Max(0, Intent.Power - Reduction), IntentIdx,
					/*CoverBypassedByFacing*/ FMath::Max(0, Nominal - Reduction)));
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
	// Il terzo canale alimenta voci di TurnLog con lo stesso peso degli altri due, quindi ha bisogno della
	// stessa rete: oggi risulterebbe ordinato solo perche' `IntentIdx` cresce, cioe' per un dettaglio del
	// ciclo chiamante e non per una garanzia. Un secondo produttore che raccogliesse fuori ordine renderebbe
	// non deterministica la sequenza delle voci nella traccia archiviata.
	Plan.DoorlessIntents.Sort();
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
		// `AttackerId` non si scarta piu' ([D-212]): senza, una mitigazione direzionale per-colpo non e'
		// esprimibile dentro il resolver, che vede solo bersaglio e potenza.
		Attacks.Add(FRTAttack(Hit.TargetId, Hit.Power, Hit.AttackerId));
	}
	return Attacks;
}
