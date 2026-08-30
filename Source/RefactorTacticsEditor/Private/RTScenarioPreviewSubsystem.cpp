#include "RTScenarioPreviewSubsystem.h"

#include "RTScenarioPreviewActor.h"
#include "RTScenarioViewportModel.h"

#include "Editor.h"
#include "Engine/World.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"

namespace
{
	/**
	 * Il mondo su cui l'editor sta lavorando. `nullptr` in un contesto senza editor — un commandlet, o una
	 * suite headless — ed e' un caso normale, non un errore: l'anteprima semplicemente non si posa.
	 */
	UWorld* EditorWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	/** Spawn transiente e fuori dall'outliner: l'anteprima non deve poter finire nel livello salvato. */
	template <typename TActor>
	TActor* SpawnPreviewActor(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		FActorSpawnParameters Params;
		Params.ObjectFlags = RF_Transient;
		Params.bHideFromSceneOutliner = true;
		Params.bTemporaryEditorActor = true;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		TActor* Spawned = World->SpawnActor<TActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (Spawned)
		{
			// Il tag e' cio' che permette a `FindTargetMapActor` di non contare questi actor come "la mappa
			// del livello". Senza, aprire un'anteprima spegne i tool di disegno in silenzio.
			Spawned->Tags.AddUnique(ARTScenarioPreviewActor::PreviewTag);
		}
		return Spawned;
	}
}

void URTScenarioPreviewSubsystem::Deinitialize()
{
	ClearPreview();
	Super::Deinitialize();
}

bool URTScenarioPreviewSubsystem::IsShowing() const
{
	return PreviewUnits != nullptr || PreviewMap != nullptr;
}

int32 URTScenarioPreviewSubsystem::NumUnitsShown() const
{
	return PreviewUnits ? PreviewUnits->NumMarkers() : 0;
}

void URTScenarioPreviewSubsystem::ClearPreview()
{
	if (PreviewUnits)
	{
		PreviewUnits->ClearUnits();
		PreviewUnits->Destroy();
		PreviewUnits = nullptr;
	}
	if (PreviewMap)
	{
		PreviewMap->Destroy();
		PreviewMap = nullptr;
	}
	LayerReadout.Reset();
}

bool URTScenarioPreviewSubsystem::ShowScenario(const URTScenarioAuthoring* Authoring)
{
	// Il vuoto prima di tutto: qualunque via d'uscita qui sotto deve lasciare lo schermo senza uno scenario
	// che non e' piu' quello selezionato.
	ClearPreview();

	if (!Authoring)
	{
		return false;
	}

	// L'arena canonica: la STESSA che il runner costruira'. Non si legge `Fixture`/`MapRadius` dal summary
	// per rifarla qui — quella strada perde gli override di cella e mostrerebbe una mappa che non si gioca.
	URTHexMapAsset* Arena = Authoring->BuildArena(GetTransientPackage());
	if (!Arena)
	{
		return false;
	}

	// Il mondo si chiede DOPO: cosi' «facade chiusa» e «nessun mondo» restano due risposte distinte, e le
	// prime due si possono provare headless.
	UWorld* World = EditorWorld();
	if (!World)
	{
		return false;
	}

	PreviewMap = SpawnPreviewActor<ARTHexMapActor>(World);
	if (!PreviewMap)
	{
		return false;
	}
	PreviewMap->MapAsset = Arena;
	// Da qui in poi celle, rilievo, blocchi, bordi (copertura e porte) e glifi di superficie li disegna
	// `ARTHexMapActor` con il kit che gia' usa: questa slice non ne aggiunge uno secondo.
	PreviewMap->RebuildInstances();

	const TArray<FRTScenarioUnitView> Units = Authoring->ListUnits();
	LayerReadout = RTScenarioViewport::DescribeLayers(RTScenarioViewport::LayersInUse(Units));

	PreviewUnits = SpawnPreviewActor<ARTScenarioPreviewActor>(World);
	if (!PreviewUnits)
	{
		// La mappa c'e' ma le unita' no: si toglie tutto invece di mostrare un'arena vuota che sembrerebbe
		// uno scenario senza schieramento.
		ClearPreview();
		return false;
	}

	// L'origine e la scala vengono dall'actor che disegna la mappa, non ricalcolate: `GetHexContext` e'
	// l'unico punto da cui passano le conversioni cella<->mondo, e due sorgenti diverse metterebbero i
	// marcatori accanto alle celle invece che sopra.
	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	PreviewMap->GetHexContext(Origin, HexSize, LayerHeight);

	PreviewUnits->ShowUnits(Units, Origin, HexSize, LayerHeight);
	return true;
}
