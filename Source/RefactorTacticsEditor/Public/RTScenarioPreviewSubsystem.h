#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"

#include "RTScenarioPreviewSubsystem.generated.h"

class ARTHexMapActor;
class ARTScenarioPreviewActor;
class URTScenarioAuthoring;

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
 */
UCLASS()
class URTScenarioPreviewSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
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

	/** C'e' un'anteprima a schermo? */
	bool IsShowing() const;

	/** Quante unita' l'anteprima sta mostrando. E' cio' che un automation test puo' contare. */
	int32 NumUnitsShown() const;

	/**
	 * Su quali layer sta ragionando l'anteprima — `L0`, `L0, L1`, `nessun layer`.
	 *
	 * ⚠️ **Non e' decorazione.** Due celle con lo stesso `X/Y` e `Layer` diverso sono celle diverse, e un
	 * viewport che non dichiara il piano che mostra produce una lettura falsa su ponte, tetto e tunnel.
	 */
	FString GetLayerReadout() const { return LayerReadout; }

private:
	/** La mappa dello scenario. Transiente: non entra nel livello (vedi il perche' esteso sulla classe). */
	UPROPERTY()
	TObjectPtr<ARTHexMapActor> PreviewMap;

	/** I marcatori delle unita'. */
	UPROPERTY()
	TObjectPtr<ARTScenarioPreviewActor> PreviewUnits;

	FString LayerReadout;
};
