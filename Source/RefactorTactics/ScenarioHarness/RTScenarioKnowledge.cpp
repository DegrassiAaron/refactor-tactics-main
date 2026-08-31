#include "ScenarioHarness/RTScenarioKnowledge.h"

#include "Ability/RTHeroData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"

namespace
{
	/**
	 * Il `VisionRange` dichiarato dall'eroe, o `false` se il roster non lo conosce.
	 *
	 * Fail-closed: un id ignoto non produce un raggio di ripiego. Un'unita' con un eroe che non esiste piu'
	 * — un file vecchio, un rename — non deve vedere «cinque celle» perche' cinque e' il default della
	 * struct: vedrebbe qualcosa che il gioco non le concede, e il velo mostrerebbe piu' di quanto e' lecito.
	 */
	bool FindVisionRange(const TArray<URTHeroData*>& Roster, FName HeroId, int32& OutRange)
	{
		for (const URTHeroData* Hero : Roster)
		{
			if (Hero && Hero->HeroId == HeroId)
			{
				OutRange = Hero->VisionRange;
				return true;
			}
		}
		return false;
	}
}

TArray<int32> RTScenarioKnowledge::TeamIds(const TArray<FRTScenarioUnitView>& Units)
{
	TSet<int32> Seen;
	for (const FRTScenarioUnitView& Unit : Units)
	{
		Seen.Add(Unit.TeamId);
	}

	TArray<int32> Out = Seen.Array();
	// L'ordine di un `TSet` dipende dall'hash e dall'inserimento: senza questo Sort le posizioni del
	// selettore cambierebbero riaprendo lo stesso scenario, e «Team 1» sarebbe in un punto diverso ogni
	// volta.
	Out.Sort();
	return Out;
}

TArray<FRTPerceiver> RTScenarioKnowledge::Observers(const TArray<FRTScenarioUnitView>& Units, int32 TeamId,
	const TArray<URTHeroData*>& Roster)
{
	TArray<FRTPerceiver> Out;
	for (const FRTScenarioUnitView& Unit : Units)
	{
		if (Unit.TeamId != TeamId)
		{
			continue;
		}

		int32 Range = 0;
		if (!FindVisionRange(Roster, Unit.HeroId, Range))
		{
			continue; // eroe ignoto: quest'unita' non vede, invece di vedere con un raggio inventato
		}

		FRTPerceiver P;
		P.Cell = Unit.Cell;
		P.Facing = Unit.Facing;
		P.VisionRange = Range;
		Out.Add(P);
	}
	return Out;
}

FRTTeamKnowledge RTScenarioKnowledge::ForTeam(const URTHexMapAsset* Map, const TArray<FRTScenarioUnitView>& Units,
	int32 TeamId, const TArray<URTHeroData*>& Roster)
{
	// I nemici con la loro cella attuale: un INGRESSO, non un esito. E' `Observe` a filtrarli contro cio'
	// che gli osservatori vedono davvero — passarli a chi disegna saltando quel passaggio sarebbe
	// esattamente l'hidden-state leak che questa slice esiste per chiudere.
	//
	// ⚠️ `TurnNumber` resta 0 qui: lo scrive `Observe`, unica a sapere QUANDO si osserva (stessa scelta di
	// `RTTurnManager.cpp:504`).
	TArray<FRTLastKnownContact> EnemiesNow;
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		if (Units[i].TeamId != TeamId)
		{
			EnemiesNow.Add(FRTLastKnownContact(LocalUnitId(i), Units[i].Cell, /*ignorato*/ 0));
		}
	}

	// La prospettiva ONNISCIENTE non salta il velo: costruisce la conoscenza che vede tutto, e passa dalla
	// stessa `Observe` di ogni altra. Un osservatore per cella sarebbe assurdo — l'onniscienza non e' una
	// percezione — quindi le celle si scrivono direttamente, ma la struttura che ne esce e' la stessa che
	// `ApplyKnowledgeVeil` riceve in `Team N`, e il percorso a valle non ha rami.
	if (TeamId == OmniscientTeamId)
	{
		FRTTeamKnowledge All;
		All.Version = FRTTeamKnowledge::CurrentVersion;
		All.TeamId = OmniscientTeamId; // nessuna squadra possiede questa vista, e lo dice il dato
		All.TurnNumber = 0;

		if (Map)
		{
			All.VisibleCells.Reserve(Map->Cells.Num());
			for (const FRTHexCellData& Cell : Map->Cells)
			{
				All.VisibleCells.Add(Cell.Id);
			}
			All.VisibleCells.Sort([](const FRTCellId& A, const FRTCellId& B)
			{
				return URTHexLibrary::StableLess(A, B);
			});
		}
		// Il ricordo coincide con la vista: in `Omniscient` non esiste una penombra, perche' non esiste
		// niente che sia stato visto e ora non lo sia.
		All.ExploredCells = All.VisibleCells;

		// I contatti sono i nemici di NESSUNA squadra, cioe' tutte le unita': con `TeamId` onnisciente ogni
		// unita' e' finita in `EnemiesNow`, e stanno tutte in celle visibili.
		for (const FRTLastKnownContact& Enemy : EnemiesNow)
		{
			All.Contacts.Add(FRTLastKnownContact(Enemy.StableUnitId, Enemy.Cell, /*Turn=*/ 0));
		}
		All.Contacts.Sort([](const FRTLastKnownContact& A, const FRTLastKnownContact& B)
		{
			return A.StableUnitId < B.StableUnitId;
		});
		return All;
	}

	// `Previous` vuota: uno scenario aperto ha un solo istante, e fingere un ricordo che nessuno ha
	// osservato sarebbe la vista sotto mentite spoglie. La memoria arriva col playback (#1625).
	return URTTeamKnowledgeLibrary::Observe(Map, TeamId, /*TurnNumber=*/ 0,
		Observers(Units, TeamId, Roster), EnemiesNow, FRTTeamKnowledge());
}

TArray<FRTScenarioUnitView> RTScenarioKnowledge::VisibleUnits(const TArray<FRTScenarioUnitView>& Units,
	const FRTTeamKnowledge& Knowledge)
{
	TArray<FRTScenarioUnitView> Out;
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		// La regola non e' scritta qui: `ClassifyTarget` e' gia' l'owner di CP 13.2, e sa gia' che una
		// squadra conosce sempre i propri. `Rejected` e' l'unico esito che toglie l'unita' dallo schermo —
		// `CellOnly` e' un ricordo, e [D-227] dice di conservarlo.
		const ERTTargetKnowledge Verdict = URTTeamKnowledgeLibrary::ClassifyTarget(
			Knowledge, LocalUnitId(i), Units[i].TeamId, Units[i].Cell);

		if (Verdict != ERTTargetKnowledge::Rejected)
		{
			Out.Add(Units[i]);
		}
	}
	return Out;
}
