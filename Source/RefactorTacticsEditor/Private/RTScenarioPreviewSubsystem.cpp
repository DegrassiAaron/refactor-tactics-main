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

void URTScenarioPreviewSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Togliere l'anteprima PRIMA che il mondo che la ospita venga distrutto. Vedi il campo `MapLoadHandle`
	// per quale delegate e' e perche' non e' `OnMapOpened`.
	MapLoadHandle = FEditorDelegates::OnMapLoad.AddUObject(this, &URTScenarioPreviewSubsystem::HandleMapLoad);
}

void URTScenarioPreviewSubsystem::Deinitialize()
{
	if (MapLoadHandle.IsValid())
	{
		FEditorDelegates::OnMapLoad.Remove(MapLoadHandle);
		MapLoadHandle.Reset();
	}
	ClearPreview();
	Super::Deinitialize();
}

void URTScenarioPreviewSubsystem::HandleMapLoad(const FString& /*Filename*/, FCanLoadMap& /*OutCanLoadMap*/)
{
	// ⛔ Non si tocca `OutCanLoadMap`: un'anteprima a schermo non e' una ragione per **vietare** il
	// caricamento di un livello. Questo aggancio serve a farsi da parte, non a mettersi di traverso.
	ClearPreview();
}

bool URTScenarioPreviewSubsystem::IsShowing() const
{
	// Con i deboli, «mostrato» vuol dire «l'attore esiste ANCORA»: se il mondo se l'e' portato via,
	// l'anteprima non c'e' piu' davvero, e dirlo e' piu' onesto che ricordare un puntatore.
	return PreviewUnits.IsValid() || PreviewMap.IsValid();
}

int32 URTScenarioPreviewSubsystem::NumUnitsShown() const
{
	return PreviewUnits.IsValid() ? PreviewUnits->NumMarkers() : 0;
}

void URTScenarioPreviewSubsystem::ClearPreview()
{
	// ⚠️ **`IsValid()` e non un semplice test di non-nullita'.** Con i puntatori deboli di `#2115` gli attori
	// possono essere gia' morti col mondo che li conteneva, e chiamare `Destroy()` su un puntatore stantio
	// sarebbe il crash che questa issue toglie, spostato di due righe. Il `Reset()` finale vale in entrambi
	// i casi: se erano gia' andati, non resta niente da azzerare se non il nostro riferimento.
	if (ARTScenarioPreviewActor* Units = PreviewUnits.Get())
	{
		Units->ClearUnits();
		Units->ClearBorder();
		Units->Destroy();
	}
	PreviewUnits.Reset();

	if (ARTHexMapActor* Map = PreviewMap.Get())
	{
		Map->Destroy();
	}
	PreviewMap.Reset();
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
	return PreviewUnits.IsValid() ? PreviewUnits->NumBorderPanels() : 0;
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
	// I due attori si prendono UNA volta e si usano come puntatori normali: con i deboli di `#2115`, un
	// `.Get()` per riga chiederebbe dieci volte la stessa domanda, e — peggio — lascerebbe pensare che la
	// risposta possa cambiare a meta' funzione.
	ARTHexMapActor* Map = PreviewMap.Get();
	ARTScenarioPreviewActor* Units = PreviewUnits.Get();
	if (!Map || !Units)
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
	Map->ApplyKnowledgeVeil(Knowledge);

	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;
	Map->GetHexContext(Origin, HexSize, LayerHeight);

	// 🔴 Il velo copre le cinque famiglie di istanze della MAPPA; i marcatori stanno su un altro actor e non
	// li tocca. Senza questa riga un nemico mai visto resterebbe a schermo con la board velata intorno —
	// l'hidden-state leak piu' facile da introdurre qui.
	Units->ShowUnits(RTScenarioKnowledge::VisibleUnits(AllUnits, Knowledge),
		Origin, HexSize, LayerHeight);

	// Il confine di cio' che la squadra vede: dalla conoscenza canonica, non da una query propria.
	Units->ShowBorder(URTVisibilityBorderLibrary::ExposedEdges(Knowledge.VisibleCells),
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

	ARTHexMapActor* SpawnedMap = SpawnPreviewActor<ARTHexMapActor>(World);
	PreviewMap = SpawnedMap;
	if (!SpawnedMap)
	{
		return false;
	}
	SpawnedMap->MapAsset = Arena;
	// Da qui in poi celle, rilievo, blocchi, bordi (copertura e porte) e glifi di superficie li disegna
	// `ARTHexMapActor` con il kit che gia' usa: questa slice non ne aggiunge uno secondo.
	SpawnedMap->RebuildInstances();

	// Le due fotografie che la prospettiva (#1754) dovra' rileggere: la facade viene chiusa da chi l'ha
	// aperta appena questa funzione ritorna, e chiederle di nuovo a lei significherebbe tenerla aperta.
	PreviewArena = Arena;
	AllUnits = Authoring->ListUnits();
	LayerReadout = RTScenarioViewport::DescribeLayers(RTScenarioViewport::LayersInUse(AllUnits));

	ARTScenarioPreviewActor* SpawnedUnits = SpawnPreviewActor<ARTScenarioPreviewActor>(World);
	PreviewUnits = SpawnedUnits;
	if (!SpawnedUnits)
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
