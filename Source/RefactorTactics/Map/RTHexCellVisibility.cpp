#include "Map/RTHexCellVisibility.h"

#include "Map/RTHexCellData.h"
#include "Turn/RTHexSim.h" // RTObserver::Omniscient

namespace
{
	struct FRTCellFieldRow
	{
		FName Field;
		ERTCellFieldVisibility Visibility;
	};

	/**
	 * Le righe della classificazione, in un array e **non** direttamente in una `TMap`.
	 *
	 * 🔴 Una `TMap` costruita da initializer list **ingoia una chiave duplicata** e l'ultima riga vince
	 * in silenzio. Con l'array la duplicazione resta misurabile, ed e' misurata dal test di totalita'. La
	 * ragione per esteso sta in `URTReplayPrivacyLibrary`, che ha questa stessa forma e da cui e' copiata
	 * di proposito: due tabelle di visibilita' che si leggono diverse sono due regole.
	 */
	const TArray<FRTCellFieldRow>& CellVisibilityRows()
	{
		static const TArray<FRTCellFieldRow> Rows = {
			// --- La mappa, che i due giocatori guardano insieme ----------------------------------------
			//
			// 🔑 Sono TUTTI `Public`, e la ragione e' una sola per tutti e tredici: questa struct e' il
			// contenuto REDATTO dell'asset di mappa (`URTHexMapAsset::Cells`), non uno stato di partita.
			// Nessuna regola di conoscenza parziale la tocca: `FRTTeamKnowledge` riguarda le UNITA'.
			// Classificare qui un campo come `ObserverDependent` sarebbe inventare una nebbia sul terreno
			// che il gioco non ha — e andrebbe deciso in `AGENTS.md` §4, non in una tabella.
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, Id),                      ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, Height),                  ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, Surface),                 ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, BodyFill),                ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, MoveCost),                ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, OccupancySurcharge),      ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, bBlocksMovement),         ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, bMovementBlockGenerated), ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, bBlocksLineOfSight),      ERTCellFieldVisibility::Public },
			// ⚠️ `bIsObjective` e' pubblico e la risposta prudente sarebbe stata l'opposta. L'obiettivo e'
			// il punto su cui le due squadre si contendono la partita: un obiettivo che una sola squadra
			// vede non sarebbe privacy, sarebbe un'altra regola di gioco.
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, bIsObjective),            ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, Covers),                  ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, Doors),                   ERTCellFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTHexCellData, Guards),                  ERTCellFieldVisibility::Public },
		};
		return Rows;
	}
}

const TMap<FName, ERTCellFieldVisibility>& URTHexCellVisibilityLibrary::FieldVisibility()
{
	static const TMap<FName, ERTCellFieldVisibility> Table = []()
	{
		TMap<FName, ERTCellFieldVisibility> Built;
		for (const FRTCellFieldRow& Row : CellVisibilityRows())
		{
			Built.Add(Row.Field, Row.Visibility);
		}
		return Built;
	}();
	return Table;
}

FName URTHexCellVisibilityLibrary::OccupantField()
{
	// ⛔ Deliberatamente NON un `GET_MEMBER_NAME_CHECKED`: non esiste un membro da cui derivarlo, e fingere
	// che ci fosse e' precisamente l'errore che questo nome esiste per rendere visibile. L'occupante vive
	// in `FRTHexSnapshot::Occupancy`, cioe' in una struttura che porta gia' il proprio `ObserverTeamId`.
	static const FName Name(TEXT("OccupantUnitId"));
	return Name;
}

bool URTHexCellVisibilityLibrary::SnapshotEntitles(int32 SnapshotObserverTeamId, int32 ViewObserverTeamId)
{
	// Nessun caso speciale per `RTObserver::Omniscient`, e l'assenza e' la regola: l'onniscienza NON e' un
	// permesso universale, e' **una posizione fra le altre**. La frase per esteso sta in
	// `ScenarioHarness/RTScenarioKnowledge.h` — *«`Omniscient` e' una posizione NOMINATA, non il filtro
	// spento»* — ed e' quella che `Turn/RTHexSim.h` cita quando porta la costante alla portata della
	// simulazione. Uno snapshot onnisciente compone una vista onnisciente e nient'altro.
	//
	// ⚠️ E `Omniscient` e' `INDEX_NONE`, **non** `0`: `RTHexSim.h` annota che la differenza e' un difetto
	// gia' occorso, perche' `0` e' la squadra 0 e un osservatore «non specificato» che valesse `0` darebbe
	// la vista di quella squadra a chiunque dimenticasse di dichiararsi. Il confronto secco qui sotto lo
	// rispetta senza doverlo sapere.
	return SnapshotObserverTeamId == ViewObserverTeamId;
}
