#include "RTScenarioPreviewSubsystem.h"

#include "RTScenarioPreviewActor.h"
#include "RTScenarioViewportModel.h"

#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTVisibilityBorder.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioKnowledge.h"

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
		PreviewUnits->ClearBorder();
		PreviewUnits->Destroy();
		PreviewUnits = nullptr;
	}
	if (PreviewMap)
	{
		PreviewMap->Destroy();
		PreviewMap = nullptr;
	}
	PreviewArena = nullptr;
	AllUnits.Reset();
	// La prospettiva torna al default del designer: aprire uno scenario nuovo con il filtro di quello
	// precedente ancora attivo mostrerebbe una vista parziale che nessuno ha chiesto per questo file.
	Perspective = RTScenarioKnowledge::OmniscientTeamId;
	LayerReadout.Reset();
}

TArray<int32> URTScenarioPreviewSubsystem::GetSelectableTeams() const
{
	return RTScenarioKnowledge::TeamIds(AllUnits);
}

int32 URTScenarioPreviewSubsystem::NumBorderPanelsShown() const
{
	return PreviewUnits ? PreviewUnits->NumBorderPanels() : 0;
}

bool URTScenarioPreviewSubsystem::SetPerspective(int32 TeamId)
{
	if (!IsShowing())
	{
		return false; // niente a schermo: non c'e' una prospettiva da cambiare, e ricordarla sarebbe stato
	}

	Perspective = TeamId;
	ApplyPerspective();
	return true;
}

void URTScenarioPreviewSubsystem::ApplyPerspective()
{
	if (!PreviewMap || !PreviewUnits)
	{
		return;
	}

	// Il roster si chiede UNA volta per applicazione e si passa: `GetHeroRoster()` costruisce quattro
	// `URTHeroData` con tutte le loro abilita' a ogni chiamata, e farlo per unita' le pagherebbe tutte per
	// leggere un intero.
	const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();

	// La conoscenza CANONICA: `Observe`, la stessa funzione che il TurnManager chiama in partita. In
	// `Omniscient` non e' il filtro spento — e' la conoscenza che vede tutto, e il percorso qui sotto non ha
	// rami.
	const FRTTeamKnowledge Knowledge = RTScenarioKnowledge::ForTeam(
		PreviewArena, AllUnits, Perspective, Roster);

	// ⚠️ La precondizione di `ApplyKnowledgeVeil`: `InstanceCells` dev'essere derivato e nessun
	// `RebuildInstances` deve stare fra il calcolo e l'applicazione. Qui la mappa e' gia' costruita e nessuno
	// la ricostruisce in mezzo — se un giorno un tool lo facesse, e' questa funzione che va richiamata dopo.
	PreviewMap->ApplyKnowledgeVeil(Knowledge);

	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	PreviewMap->GetHexContext(Origin, HexSize, LayerHeight);

	// 🔴 Il velo copre le cinque famiglie di istanze della MAPPA; i marcatori stanno su un altro actor e non
	// li tocca. Senza questa riga un nemico mai visto resterebbe a schermo con la board velata intorno —
	// l'hidden-state leak piu' facile da introdurre qui.
	PreviewUnits->ShowUnits(RTScenarioKnowledge::VisibleUnits(AllUnits, Knowledge),
		Origin, HexSize, LayerHeight);

	// Il confine di cio' che la squadra vede: dalla conoscenza canonica, non da una query propria.
	PreviewUnits->ShowBorder(URTVisibilityBorderLibrary::ExposedEdges(Knowledge.VisibleCells),
		Origin, HexSize, LayerHeight);
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

	// Le due fotografie che la prospettiva (#1754) dovra' rileggere: la facade viene chiusa da chi l'ha
	// aperta appena questa funzione ritorna, e chiederle di nuovo a lei significherebbe tenerla aperta.
	PreviewArena = Arena;
	AllUnits = Authoring->ListUnits();
	LayerReadout = RTScenarioViewport::DescribeLayers(RTScenarioViewport::LayersInUse(AllUnits));

	PreviewUnits = SpawnPreviewActor<ARTScenarioPreviewActor>(World);
	if (!PreviewUnits)
	{
		// La mappa c'e' ma le unita' no: si toglie tutto invece di mostrare un'arena vuota che sembrerebbe
		// uno scenario senza schieramento.
		ClearPreview();
		return false;
	}

	// Uno scenario si apre in `Omniscient`: il Tactical Designer e' omnisciente per costruzione, ed e' giusto
	// che lo resti finche' qualcuno non chiede un'altra prospettiva. `ClearPreview` l'ha gia' rimesso li'.
	//
	// ⚠️ Anche questa prima posa passa da `ApplyPerspective`, e non da uno `ShowUnits` diretto: un percorso
	// separato per il caso onnisciente sarebbe la seconda strada che nessun test attraversa, e divergerebbe
	// dalla prima al primo cambiamento. L'origine e la scala le legge di li' da `GetHexContext`, che resta
	// l'unico punto da cui passano le conversioni cella<->mondo.
	ApplyPerspective();
	return true;
}
