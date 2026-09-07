#include "Map/RTMapTemplateLibrary.h"

#include "EngineUtils.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTSpawnPoint.h"

#if WITH_EDITOR
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#endif

bool URTMapTemplateLibrary::ResolveWorldToCell(const URTHexMapAsset* MapAsset, const FVector& GridOrigin,
	float HexSize, float LayerHeight, const FVector& WorldLocation, FRTCellId& OutCell)
{
	OutCell = FRTCellId();
	if (!MapAsset || HexSize <= 0.f)
	{
		// Senza scala non c'e' geometria: la stessa guardia che `URTHexLibrary` applica altrove. Un HexSize
		// nullo non produce una cella sbagliata, non produce nessuna cella.
		return false;
	}

	const FRTCellId Candidate = URTHexLibrary::WorldToCellId(WorldLocation, GridOrigin, HexSize, LayerHeight);

	// ⚠️ La coppia assiale esiste SEMPRE — l'arrotondamento cubico non fallisce mai — quindi il fatto che
	// interessa non e' «si calcola» ma «l'asset la contiene». Un marker appoggiato tre metri fuori dal
	// tabellone produce una coordinata perfettamente valida che non e' una cella di questa mappa.
	if (!MapAsset->FindCell(Candidate))
	{
		return false;
	}

	OutCell = Candidate;
	return true;
}

void URTMapTemplateLibrary::ValidateTemplate(int32 MapActorCount, const URTHexMapAsset* MapAsset,
	const TArray<FRTSpawnPlacement>& Spawns, TArray<FRTMapTemplateIssue>& OutIssues)
{
	OutIssues.Reset();

	auto AddIssue = [&OutIssues](ERTMapTemplateIssue Reason, const FRTSpawnPlacement* Spawn)
	{
		FRTMapTemplateIssue Issue;
		Issue.Reason = Reason;
		if (Spawn)
		{
			Issue.Label = Spawn->Label;
			Issue.TeamId = Spawn->TeamId;
			Issue.SlotIndex = Spawn->SlotIndex;
		}
		OutIssues.Add(MoveTemp(Issue));
	};

	if (MapActorCount <= 0)
	{
		AddIssue(ERTMapTemplateIssue::MissingMapActor, nullptr);
	}
	else
	{
		if (MapActorCount > 1)
		{
			AddIssue(ERTMapTemplateIssue::DuplicateMapActor, nullptr);
		}
		if (!MapAsset)
		{
			// Solo QUI: senza attore mappa l'asset manca per conseguenza, e due segnalazioni per una causa
			// sola manderebbero chi legge a cercare due difetti.
			AddIssue(ERTMapTemplateIssue::MissingMapAsset, nullptr);
		}
	}

	// Quante volte ogni coppia (Team, Slot) e' stata dichiarata. La `TMap` serve a CONTARE e non a
	// ordinare: nulla di cio' che segue itera su di lei (invariante determinismo n. 3).
	TMap<TPair<int32, int32>, int32> SlotUsage;
	for (const FRTSpawnPlacement& Spawn : Spawns)
	{
		++SlotUsage.FindOrAdd(TPair<int32, int32>(Spawn.TeamId, Spawn.SlotIndex));
	}

	for (const FRTSpawnPlacement& Spawn : Spawns)
	{
		if (const int32* Count = SlotUsage.Find(TPair<int32, int32>(Spawn.TeamId, Spawn.SlotIndex)))
		{
			if (*Count > 1)
			{
				AddIssue(ERTMapTemplateIssue::DuplicateSpawnSlot, &Spawn);
			}
		}

		if (!Spawn.bResolved)
		{
			AddIssue(ERTMapTemplateIssue::SpawnOffMap, &Spawn);
		}
	}

	// L'ordine STABILE dell'uscita. I marker arrivano nell'ordine di `TActorIterator`, che non ne garantisce
	// nessuno: senza questo, due esecuzioni sullo stesso livello potrebbero elencare gli stessi difetti in
	// ordine diverso, e un test che li confronta diventerebbe intermittente.
	OutIssues.Sort([](const FRTMapTemplateIssue& A, const FRTMapTemplateIssue& B)
	{
		if (A.Reason != B.Reason)
		{
			return static_cast<uint8>(A.Reason) < static_cast<uint8>(B.Reason);
		}
		if (A.TeamId != B.TeamId)
		{
			return A.TeamId < B.TeamId;
		}
		if (A.SlotIndex != B.SlotIndex)
		{
			return A.SlotIndex < B.SlotIndex;
		}
		return A.Label < B.Label;
	});
}

void URTMapTemplateLibrary::CollectFromWorld(const UWorld* World, int32& OutMapActorCount,
	const URTHexMapAsset*& OutMapAsset, TArray<FRTSpawnPlacement>& OutSpawns)
{
	OutMapActorCount = 0;
	OutMapAsset = nullptr;
	OutSpawns.Reset();

	if (!World)
	{
		return;
	}

	// ⚠️ Il PRIMO attore mappa incontrato, che e' esattamente cio' che `ARTHexMapActor::FindInWorld` fa a
	// runtime: se il livello ne ha due, la validazione deve giudicare la stessa origine che la partita
	// userebbe, non una migliore.
	const ARTHexMapActor* MapActor = nullptr;
	for (TActorIterator<ARTHexMapActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		++OutMapActorCount;
		if (!MapActor)
		{
			MapActor = *It;
		}
	}

	FVector GridOrigin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	if (MapActor)
	{
		OutMapAsset = MapActor->GetHexContext(GridOrigin, HexSize, LayerHeight);
	}

	for (TActorIterator<ARTSpawnPoint> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const ARTSpawnPoint* Spawn = *It;
		if (!Spawn)
		{
			continue;
		}

		FRTSpawnPlacement Placement;
		Placement.TeamId = Spawn->TeamId;
		Placement.SlotIndex = Spawn->SlotIndex;
		Placement.Label = Spawn->GetName();
		Placement.bResolved = ResolveWorldToCell(OutMapAsset, GridOrigin, HexSize, LayerHeight,
			Spawn->GetActorLocation(), Placement.Cell);
		OutSpawns.Add(MoveTemp(Placement));
	}

	// Stesso motivo dell'ordinamento delle segnalazioni: la raccolta non deve dipendere dall'ordine di
	// iterazione degli attori.
	OutSpawns.Sort([](const FRTSpawnPlacement& A, const FRTSpawnPlacement& B)
	{
		if (A.TeamId != B.TeamId)
		{
			return A.TeamId < B.TeamId;
		}
		if (A.SlotIndex != B.SlotIndex)
		{
			return A.SlotIndex < B.SlotIndex;
		}
		return A.Label < B.Label;
	});
}

FString URTMapTemplateLibrary::DescribeIssue(const FRTMapTemplateIssue& Issue)
{
	switch (Issue.Reason)
	{
	case ERTMapTemplateIssue::MissingMapActor:
		return TEXT("Il livello non contiene nessun ARTHexMapActor: la griglia tattica non ha ne' origine ne' scala.");
	case ERTMapTemplateIssue::DuplicateMapActor:
		return TEXT("Il livello contiene piu' di un ARTHexMapActor: quale origine valga lo deciderebbe l'ordine di iterazione.");
	case ERTMapTemplateIssue::MissingMapAsset:
		return TEXT("L'ARTHexMapActor non ha un URTHexMapAsset assegnato: la partita girerebbe sul graybox di ripiego.");
	case ERTMapTemplateIssue::DuplicateSpawnSlot:
		return FString::Printf(
			TEXT("Spawn duplicato: la coppia (Team %d, Slot %d) e' dichiarata da piu' di un ARTSpawnPoint."),
			Issue.TeamId, Issue.SlotIndex);
	case ERTMapTemplateIssue::SpawnOffMap:
		return FString::Printf(
			TEXT("Lo spawn (Team %d, Slot %d) non cade su nessuna cella della mappa."),
			Issue.TeamId, Issue.SlotIndex);
	default:
		return TEXT("Nessuna segnalazione.");
	}
}

#if WITH_EDITOR
void URTMapTemplateLibrary::EmitIssueToMapCheck(const FRTMapTemplateIssue& Issue, const UObject* TokenTarget)
{
	TSharedRef<FTokenizedMessage> Message = FMessageLog("MapCheck").Warning();
	if (TokenTarget)
	{
		// Il token rende il messaggio CLICCABILE: chi legge il Map Check arriva all'attore da correggere
		// invece di doverlo cercare per nome nell'outliner.
		Message->AddToken(FUObjectToken::Create(const_cast<UObject*>(TokenTarget)));
	}
	Message->AddToken(FTextToken::Create(FText::FromString(DescribeIssue(Issue))));
}

int32 URTMapTemplateLibrary::ReportToMapCheck(const UWorld* World)
{
	if (!World)
	{
		return 0;
	}

	int32 MapActorCount = 0;
	const URTHexMapAsset* MapAsset = nullptr;
	TArray<FRTSpawnPlacement> Spawns;
	CollectFromWorld(World, MapActorCount, MapAsset, Spawns);

	TArray<FRTMapTemplateIssue> Issues;
	ValidateTemplate(MapActorCount, MapAsset, Spawns, Issues);
	if (Issues.Num() == 0)
	{
		return 0;
	}

	// Nome -> attore, per dare a ogni segnalazione il token del proprio marker. Si costruisce UNA volta:
	// cercare l'attore dentro il ciclo delle segnalazioni farebbe una scansione per messaggio.
	TMap<FString, const AActor*> ByName;
	const AActor* AnyMapActor = nullptr;
	for (TActorIterator<ARTSpawnPoint> It(const_cast<UWorld*>(World)); It; ++It)
	{
		ByName.Add(It->GetName(), *It);
	}
	for (TActorIterator<ARTHexMapActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (!AnyMapActor)
		{
			AnyMapActor = *It;
		}
	}

	for (const FRTMapTemplateIssue& Issue : Issues)
	{
		// Le segnalazioni di LIVELLO non hanno un marker: il loro token e' l'attore mappa, quando c'e'.
		// Quando non c'e' — ed e' precisamente il caso `MissingMapActor` — il messaggio resta senza token
		// invece di puntare a un attore arbitrario che non ha nulla a che vedere col difetto.
		const UObject* Target = AnyMapActor;
		if (!Issue.Label.IsEmpty())
		{
			if (const AActor* const* Found = ByName.Find(Issue.Label))
			{
				Target = *Found;
			}
		}
		EmitIssueToMapCheck(Issue, Target);
	}

	return Issues.Num();
}
#endif
