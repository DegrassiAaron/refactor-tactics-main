#include "RTDevToolset.h"

#include "RTDeveloperToolsLog.h"

#include "Editor.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"

#include "Map/RTHexCellData.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Pathfinding/RTHexPath.h"
#include "Pathfinding/RTHexPathLibrary.h"

namespace
{
	/**
	 * Il world dell'EDITOR, che e' l'unico che questo bridge guarda.
	 *
	 * ⚠️ Non e' il world di PIE: durante una partita in editor esistono entrambi, e questi tool continuano a
	 * rispondere sulla mappa APERTA, non su quella giocata. E' voluto — il bridge ispeziona il contenuto
	 * autorato, e leggere lo stato di una partita in corso e' un'altra funzione con altri vincoli di privacy
	 * (gli intenti di squadra vivono li').
	 */
	UWorld* EditorWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	/** Nome del livello aperto, o stringa vuota se non c'e' un world. */
	FString CurrentLevelName(const UWorld* World)
	{
		return World ? World->GetMapName() : FString();
	}

	/**
	 * L'asset mappa dell'`ARTHexMapActor` presente nel world dell'Editor, o nullptr.
	 *
	 * Due assenze diverse collassano qui in un nullptr solo: nessun actor di mappa nel livello, oppure un
	 * actor senza asset assegnato. Chi deve distinguerle guarda `OutActor`.
	 */
	const URTHexMapAsset* FindActiveMap(ARTHexMapActor** OutActor = nullptr)
	{
		if (OutActor) { *OutActor = nullptr; }

		UWorld* World = EditorWorld();
		if (!World) { return nullptr; }

		ARTHexMapActor* Actor = ARTHexMapActor::FindInWorld(World);
		if (!Actor) { return nullptr; }

		if (OutActor) { *OutActor = Actor; }
		return Actor->MapAsset;
	}

	/** Nome leggibile di un valore d'enum riflesso (`Success`, `Floor`, ...). */
	template <typename TEnum>
	FString EnumName(TEnum Value)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Value)) : FString();
	}

	/**
	 * Il canale d'errore dei toolset: `RaiseScriptError` — lo stesso che usano quelli dell'engine.
	 *
	 * ⚠️ Chiamato da C++ FUORI da uno stack frame Blueprint non fa nulla, in silenzio. Non e' un difetto da
	 * aggirare: e' la ragione per cui i test di questo modulo passano da `FToolCallExceptionHandler`, che il
	 * frame lo costruisce. Un test che chiamasse un tool direttamente aspettandosi un errore misurerebbe il
	 * silenzio e passerebbe comunque.
	 */
	void RaiseToolError(const FString& Message)
	{
		UE_LOG(LogRTDevTools, Warning, TEXT("%s"), *Message);
		UKismetSystemLibrary::RaiseScriptError(Message);
	}

	const TCHAR* NoMapMessage()
	{
		return TEXT("No tactical hex map is loaded in the editor world: open a level containing an "
		            "ARTHexMapActor with a hex map asset assigned.");
	}
}

// =========================================================================================================
// Tool MCP: risolvono la mappa dal world e delegano alle varianti sotto.
// =========================================================================================================

FRTDevProjectStatus URTDevToolset::ProjectStatus()
{
	const double Started = FPlatformTime::Seconds();

	FRTDevProjectStatus Status;
	Status.ProjectName = FApp::GetProjectName();
	Status.EngineVersion = FEngineVersion::Current().ToString();

	const UWorld* World = EditorWorld();
	Status.LevelName = CurrentLevelName(World);

	// `ProjectStatus` e' il tool che si chiama PER PRIMO, quando ancora non si sa se c'e' una mappa:
	// l'assenza e' una risposta legittima (`bHasTacticalMap = false`), non un errore.
	if (const URTHexMapAsset* Map = FindActiveMap())
	{
		Status.bHasTacticalMap = true;
		Status.MapAssetPath = Map->GetPathName();
		Status.NumCells = Map->NumCells();
		Status.GraphRevision = Map->Revision;
	}

	UE_LOG(LogRTDevTools, Log, TEXT("ProjectStatus: level='%s' map=%s cells=%d rev=%d (%.2f ms)"),
		*Status.LevelName, Status.bHasTacticalMap ? TEXT("yes") : TEXT("no"),
		Status.NumCells, Status.GraphRevision, (FPlatformTime::Seconds() - Started) * 1000.0);

	return Status;
}

FRTDevMapReport URTDevToolset::GetCurrentMap()
{
	return DescribeMap(FindActiveMap(), CurrentLevelName(EditorWorld()));
}

FRTDevCellReport URTDevToolset::DumpCell(int32 X, int32 Y, int32 Layer)
{
	return DumpCellOnMap(FindActiveMap(), FRTCellId(X, Y, Layer));
}

FRTDevPathReport URTDevToolset::FindPath(FRTCellId Start, FRTCellId Goal, int32 MaxCost, int32 MaxNodes)
{
	return FindPathOnMap(FindActiveMap(), Start, Goal, MaxCost, MaxNodes);
}

FRTDevValidationReport URTDevToolset::ValidateTacticalMap()
{
	ARTHexMapActor* Actor = nullptr;
	const URTHexMapAsset* Map = FindActiveMap(&Actor);

	// La raggiungibilita' la calcola l'ACTOR, non l'asset: se non l'ha mai eseguita la lista e' vuota, ed e'
	// un'assenza di analisi — non una prova che ogni cella sia raggiungibile.
	static const TArray<FRTCellId> Empty;
	return ValidateMapAsset(Map, Actor ? Actor->GetUnreachableCells() : Empty);
}

// =========================================================================================================
// Varianti su mappa data: il calcolo, senza il world.
// =========================================================================================================

FRTDevMapReport URTDevToolset::DescribeMap(const URTHexMapAsset* Map, const FString& LevelName)
{
	const double Started = FPlatformTime::Seconds();

	FRTDevMapReport Report;
	if (!Map)
	{
		RaiseToolError(NoMapMessage());
		return Report;
	}

	Report.bLoaded = true;
	Report.AssetPath = Map->GetPathName();
	Report.LevelName = LevelName;
	Report.NumCells = Map->NumCells();
	Report.NumTransitions = Map->Transitions.Num();
	Report.Layers = Map->GetLayers();
	Report.GraphRevision = Map->Revision;
	Report.FormatVersion = Map->FormatVersion;
	// `ComputeHash` e' un uint32: passa per int64 perche' un hash che arriva al client come numero negativo
	// si legge male, e perche' l'intero senza segno a 32 bit non e' un tipo di proprieta' riflessa comodo qui.
	Report.ContentHash = static_cast<int64>(Map->ComputeHash());
	Report.CenterCell = Map->GetCenterCell();

	UE_LOG(LogRTDevTools, Log, TEXT("GetCurrentMap: '%s' cells=%d transitions=%d layers=%d rev=%d (%.2f ms)"),
		*Report.AssetPath, Report.NumCells, Report.NumTransitions, Report.Layers.Num(),
		Report.GraphRevision, (FPlatformTime::Seconds() - Started) * 1000.0);

	return Report;
}

FRTDevCellReport URTDevToolset::DumpCellOnMap(const URTHexMapAsset* Map, const FRTCellId& Cell)
{
	const double Started = FPlatformTime::Seconds();

	FRTDevCellReport Report;
	Report.Cell = Cell;

	if (!Map)
	{
		RaiseToolError(NoMapMessage());
		return Report;
	}

	Report.GraphRevision = Map->Revision;

	// ⚠️ L'esistenza la decide la MAPPA, non la cella. `FRTCellId::IsValid()` verifica la coerenza cubica
	// q+r+s==0, che e' vera per costruzione: usarlo qui risponderebbe «esiste» a qualunque coordinata.
	const FRTHexCellData* Data = Map->FindCell(Cell);
	if (!Data)
	{
		RaiseToolError(FString::Printf(
			TEXT("Cell %s does not exist in the active tactical map (%d cells, revision %d). "
			     "Coordinates are axial: X is q, Y is r."),
			*Cell.ToString(), Map->NumCells(), Map->Revision));
		return Report;
	}

	Report.bExists = true;
	Report.Height = Data->Height;
	Report.Surface = EnumName(Data->Surface);
	Report.MoveCost = Data->MoveCost;
	Report.OccupancySurcharge = Data->OccupancySurcharge;
	Report.TotalMoveCost = Data->TotalMoveCost();
	Report.bBlocksMovement = Data->bBlocksMovement;
	Report.bBlocksLineOfSight = Data->bBlocksLineOfSight;
	Report.NumCovers = Data->Covers.Num();
	Report.NumDoors = Data->Doors.Num();

	// I vicini li costruisce il PATHFINDER, non questo tool: e' la stessa funzione con cui l'A* espande un
	// nodo, quindi cio' che il client vede qui e' cio' che il gioco percorre davvero.
	for (const TPair<FRTCellId, int32>& Neighbor : URTHexPathLibrary::GraphNeighbors(Map, Cell))
	{
		FRTDevNeighbor Entry;
		Entry.Cell = Neighbor.Key;
		Entry.Cost = Neighbor.Value;
		Report.Neighbors.Add(Entry);
	}

	UE_LOG(LogRTDevTools, Log, TEXT("DumpCell %s: surface=%s cost=%d blocked=%s neighbors=%d rev=%d (%.2f ms)"),
		*Cell.ToString(), *Report.Surface, Report.TotalMoveCost,
		Report.bBlocksMovement ? TEXT("yes") : TEXT("no"), Report.Neighbors.Num(), Report.GraphRevision,
		(FPlatformTime::Seconds() - Started) * 1000.0);

	return Report;
}

FRTDevPathReport URTDevToolset::FindPathOnMap(const URTHexMapAsset* Map, const FRTCellId& Start,
	const FRTCellId& Goal, int32 MaxCost, int32 MaxNodes)
{
	FRTDevPathReport Report;

	if (!Map)
	{
		RaiseToolError(NoMapMessage());
		return Report;
	}

	Report.GraphRevision = Map->Revision;

	// La misura circonda SOLO la chiamata al pathfinder: round-trip MCP, costruzione del report e
	// serializzazione restano fuori. Attribuirgli il costo del trasporto renderebbe il numero
	// inconfrontabile con il target di progetto sulle query di path.
	const double Started = FPlatformTime::Seconds();
	const FRTHexPathResult Result = URTHexPathLibrary::FindPath(Map, Start, Goal, MaxCost, MaxNodes);
	Report.PathfinderMilliseconds = (FPlatformTime::Seconds() - Started) * 1000.0;

	// Il pathfinder ha gia' il suo vocabolario di esiti: si riporta, non si traduce in un secondo modello
	// d'errore. Una destinazione irraggiungibile e' una RISPOSTA, non un guasto del bridge — per questo qui
	// non si solleva nulla.
	Report.Status = EnumName(Result.Status);
	Report.bSuccess = (Result.Status == ERTHexPathStatus::Success);
	Report.Path = Result.Path;
	Report.TotalCost = Result.TotalCost;
	Report.NodesVisited = Result.NodesVisited;

	UE_LOG(LogRTDevTools, Log,
		TEXT("FindPath %s -> %s: status=%s cost=%d steps=%d nodes=%d rev=%d (pathfinder %.3f ms)"),
		*Start.ToString(), *Goal.ToString(), *Report.Status, Report.TotalCost, Report.Path.Num(),
		Report.NodesVisited, Report.GraphRevision, Report.PathfinderMilliseconds);

	return Report;
}

FRTDevValidationReport URTDevToolset::ValidateMapAsset(const URTHexMapAsset* Map,
	const TArray<FRTCellId>& UnreachableCells)
{
	const double Started = FPlatformTime::Seconds();

	FRTDevValidationReport Report;
	if (!Map)
	{
		RaiseToolError(NoMapMessage());
		return Report;
	}

	Report.bLoaded = true;
	Report.GraphRevision = Map->Revision;
	Report.Issues = Map->ValidateMap();
	Report.bValid = Report.Issues.IsEmpty();
	Report.UnreachableCells = UnreachableCells;

	UE_LOG(LogRTDevTools, Log, TEXT("ValidateTacticalMap: valid=%s issues=%d unreachable=%d rev=%d (%.2f ms)"),
		Report.bValid ? TEXT("yes") : TEXT("no"), Report.Issues.Num(), Report.UnreachableCells.Num(),
		Report.GraphRevision, (FPlatformTime::Seconds() - Started) * 1000.0);

	return Report;
}
