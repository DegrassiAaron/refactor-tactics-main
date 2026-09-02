#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"

#include "RTScenarioPreviewSubsystem.generated.h"

class ARTHexMapActor;
class ARTScenarioPreviewActor;
class URTHexMapAsset;
class URTScenarioAuthoring;
struct FRTScenarioUnitView;
struct FCanLoadMap; // il secondo parametro di `FEditorDelegates::OnMapLoad`

/**
 * Materializza nel viewport d'editor lo stato INIZIALE dello scenario aperto: la mappa che il runner
 * costruira', e le unita' dove il file le dichiara. Owner: #1753, sotto l'epic #1105.
 *
 * ## Cosa NON e'
 *
 * ⛔ **Non e' una sessione d'authoring.** Non possiede la facade, non la tiene aperta e non la richiude: la
 * riceve gia' aperta, ne legge una fotografia e la lascia com'era. La sessione e' di #1682, e un sottosistema
 * che se ne appropriasse la renderebbe impossibile da costruire dove va costruita.
 *
 * ⛔ **Non e' un secondo simulatore.** L'arena viene da `URTScenarioAuthoring::BuildArena`, cioe' dallo
 * stesso `URTScenarioArenaLibrary::BuildArena` che `FRTScenarioSession` chiama prima di eseguire. Se
 * l'anteprima e la partita divergessero, il difetto sarebbe in quel builder e non qui — che e' il punto.
 *
 * ⛔ **Nessun `Tick`.** L'anteprima si aggiorna quando qualcuno la aggiorna. `RTHexMapActor` disegna con
 * istanze e non ha bisogno di essere rifrescato per fotogramma.
 *
 * ## 🔴 Perche' due actor TRANSIENTI e non quelli del livello
 *
 * La via corta sarebbe prendere l'`ARTHexMapActor` gia' posato in `L_DevSandbox` e assegnargli l'arena
 * dello scenario. E' anche la via che **sporca il livello**: `MapAsset` e' un `UPROPERTY(EditAnywhere)`, e
 * cambiarlo mette il designer davanti a una richiesta di salvataggio per aver soltanto guardato uno
 * scenario. Peggio: se accettasse, la mappa d'autore su cui stava lavorando sarebbe stata sostituita dalla
 * fixture di uno scenario, e nessun messaggio glielo direbbe.
 *
 * Gli actor dell'anteprima nascono quindi `RF_Transient`, fuori dall'outliner, e muoiono con essa.
 *
 * ## 🔑 La prospettiva tecnica (#1754), e perche' le FOTOGRAFIE restano qui
 *
 * `Omniscient · Team 0 · Team 1 ...` cambia **cosa si mostra**, mai cosa lo scenario dichiara. Ricalcolarlo
 * richiede l'arena e le unita', e la facade a quel punto e' gia' chiusa — `SRTLauncherScenarioPanel` la
 * chiude appena posata l'anteprima, e deve continuare a farlo perche' possederla e' di #1682.
 *
 * ∴ questo sottosistema **conserva le due fotografie** che `ShowScenario` ha gia' letto: l'arena costruita e
 * l'elenco delle unita'. ⚠️ **Non e' possedere la sessione**: sono copie morte, non un modello — cambiarle
 * non cambia nessuno scenario, e la facade resta di chi la apre. Rileggerle dalla facade a ogni cambio di
 * prospettiva sarebbe l'alternativa, e vorrebbe dire tenerla aperta.
 */
UCLASS()
class URTScenarioPreviewSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Mostra lo stato iniziale dello scenario **aperto** in `Authoring`.
	 *
	 * @return `false` se non c'e' uno scenario aperto, se la sua arena non si costruisce, o se non esiste un
	 *         mondo d'editor. In tutti e tre i casi l'anteprima precedente viene **tolta**: lasciarla a
	 *         schermo mostrerebbe uno scenario diverso da quello selezionato, che e' peggio del vuoto.
	 */
	bool ShowScenario(const URTScenarioAuthoring* Authoring);

	/** Toglie l'anteprima e distrugge i suoi actor. Idempotente. */
	void ClearPreview();

	/**
	 * Le squadre fra cui il selettore puo' scegliere, dal **dato** dello scenario mostrato.
	 *
	 * Vuoto se non c'e' un'anteprima. Il selettore ha `1 + N` posizioni — `Omniscient` piu' queste — e non
	 * tre: uno scenario a squadra sola ne ha due, il 4v4 cinque. Vedi `RTScenarioKnowledge::TeamIds`.
	 */
	TArray<int32> GetSelectableTeams() const;

	/**
	 * Cambia la prospettiva TECNICA e ridisegna: velo, marcatori e confine.
	 *
	 * `RTScenarioKnowledge::OmniscientTeamId` per la vista onnisciente, che e' una posizione **nominata** e
	 * non «il filtro spento»: passa dalla stessa conoscenza canonica e dallo stesso `ApplyKnowledgeVeil`.
	 *
	 * 🔴 **Non altera niente di cio' che lo scenario dichiara**: non tocca il draft, il simulator state, lo
	 * snapshot, il replay ne' `ARTPlayerController::PlayerTeamId`. Cambia solo quali istanze sono posate.
	 *
	 * @return `false` se non c'e' un'anteprima a schermo. Una squadra che lo scenario non schiera e'
	 *         **accettata**: produce una conoscenza vuota, cioe' il velo steso su tutto — che e' la risposta
	 *         onesta, non un errore.
	 */
	bool SetPerspective(int32 TeamId);

	/** La prospettiva corrente. `OmniscientTeamId` finche' nessuno la cambia: e' il default del designer. */
	int32 GetPerspective() const { return Perspective; }

	/** C'e' un'anteprima a schermo? */
	bool IsShowing() const;

	/** Quante unita' l'anteprima sta mostrando. E' cio' che un automation test puo' contare. */
	int32 NumUnitsShown() const;

	/**
	 * Quanti pannelli di confine sono posati. Zero e' un esito legittimo: nessuna vista, nessun perimetro.
	 *
	 * Come `NumUnitsShown`, si legge dallo stato reale delle istanze e non da un contatore.
	 */
	int32 NumBorderPanelsShown() const;

	/**
	 * Su quali layer sta ragionando l'anteprima — `L0`, `L0, L1`, `nessun layer`.
	 *
	 * ⚠️ **Non e' decorazione.** Due celle con lo stesso `X/Y` e `Layer` diverso sono celle diverse, e un
	 * viewport che non dichiara il piano che mostra produce una lettura falsa su ponte, tetto e tunnel.
	 */
	FString GetLayerReadout() const { return LayerReadout; }

private:
	// -------------------------------------------------------------------------------------------------
	// 🔴 **I DUE ATTORI SI TENGONO DEBOLI, E NON E' UNA MICRO-OTTIMIZZAZIONE — `#2115`.**
	//
	// Questo e' un `UEditorSubsystem`: **sopravvive a ogni mondo**. Gli attori qui sotto vivono invece nel
	// `PersistentLevel` del mondo corrente. Con un `UPROPERTY` FORTE il sottosistema dichiarava «questi
	// attori mi appartengono e vivono quanto me», che e' falso — vivono quanto il **livello** — e la
	// conseguenza non era una svista di stile: aprire un altro livello con un'anteprima a schermo
	// **terminava l'editor**.
	//
	//     UnrealEdEngine::AddReferencedObjects( RTScenarioPreviewSubsystem )
	//       -> URTScenarioPreviewSubsystem::PreviewUnits = RTScenarioPreviewActor
	//        -> ARTScenarioPreviewActor:: = Level ...PersistentLevel
	//         -> ULevel:: = World L_GrayKitPlayground
	//             ^ This reference is preventing the old World from being GC'd ^
	//
	// ⚠️ **`RF_Transient` e `bTemporaryEditorActor` non c'entravano, ed e' la confusione facile.**
	// `SpawnPreviewActor` li usa gia' e sono corretti: risolvono *«l'anteprima non sporca il livello»* —
	// infatti `git status` resta pulito. Ma un attore transiente sta comunque **dentro il mondo**, e chi lo
	// trattiene trattiene il mondo. Sporcizia e durata sono due problemi diversi che il nome «transient»
	// invita a scambiare.
	//
	// ✅ Debole e' anche la descrizione VERA della relazione: l'anteprima non ha bisogno di sopravvivere al
	// livello che la contiene, e quando quel livello se ne va e' giusto che sparisca con lui.
	// -------------------------------------------------------------------------------------------------

	/** La mappa dello scenario. Transiente: non entra nel livello (vedi il perche' esteso sulla classe). */
	TWeakObjectPtr<ARTHexMapActor> PreviewMap;

	/** I marcatori delle unita'. */
	TWeakObjectPtr<ARTScenarioPreviewActor> PreviewUnits;

	/**
	 * L'arena mostrata: la **stessa** che `ShowScenario` ha dato a `PreviewMap`, tenuta per poter ricalcolare
	 * la conoscenza quando la prospettiva cambia. Fotografia, non modello.
	 */
	UPROPERTY()
	TObjectPtr<URTHexMapAsset> PreviewArena;

	/** Le unita' dello scenario mostrato, **tutte**: il filtro di prospettiva si applica al momento di posarle. */
	TArray<FRTScenarioUnitView> AllUnits;

	/** La prospettiva corrente. `OmniscientTeamId` all'inizio e dopo ogni `ClearPreview`. */
	int32 Perspective = INDEX_NONE;

	/**
	 * L'iscrizione a `FEditorDelegates::OnMapLoad`, che toglie l'anteprima **prima** che il mondo muoia.
	 *
	 * ⚠️ **Non e' cio' che impedisce il crash** — quello lo fanno i puntatori deboli, su ogni strada. Questo
	 * serve a non lasciare a schermo l'anteprima del livello precedente mentre si carica il successivo.
	 *
	 * ⛔ E **non basterebbe da solo**: `OnMapLoad` e' trasmesso da `FEditorFileUtils::LoadMap`
	 * (`FileHelpers.cpp:3238`), non da `UEditorEngine::Map_Load`. Una mappa aperta per una strada che salti
	 * `LoadMap` — un `MAP LOAD` da console, uno script — distruggerebbe il mondo **senza annunciarlo**.
	 *
	 * ⚠️ E non e' `OnMapOpened`, che pure il launcher usa (`RTDevSandboxLauncherSubsystem.cpp:90`): quello
	 * scatta a mondo nuovo pronto, cioe' **dopo** `EditorDestroyWorld` — dopo il punto in cui si moriva.
	 */
	FDelegateHandle MapLoadHandle;

	/**
	 * Toglie l'anteprima quando l'editor sta per caricare un'altra mappa.
	 *
	 * ⛔ Non tocca `OutCanLoadMap`: un'anteprima a schermo non e' una ragione per **vietare** il caricamento
	 * di un livello. L'aggancio serve a farsi da parte, non a mettersi di traverso.
	 */
	void HandleMapLoad(const FString& Filename, FCanLoadMap& OutCanLoadMap);

	/**
	 * Ridisegna velo, marcatori e confine secondo `Perspective`.
	 *
	 * ⚠️ **Da chiamare DOPO ogni `RebuildInstances`**, e non solo al cambio di prospettiva:
	 * `ApplyKnowledgeVeil` dichiara che i suoi indici derivano da `InstanceCells`, e un rebuild in mezzo li
	 * lascia stantii. L'esito non e' un crash ma **celle velate sbagliate**, che si legge come «problema
	 * grafico» per settimane.
	 */
	void ApplyPerspective();

	FString LayerReadout;
};
