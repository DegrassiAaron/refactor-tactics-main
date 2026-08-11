#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Perception/RTNoiseTypes.h"
#include "RTAcousticPropagationLibrary.generated.h"

class URTHexMapAsset;

/**
 * Il canale acustico: come un rumore si propaga sul grafo tattico e chi lo sente (CP 13.3).
 *
 * **Flood fill intero limitato dall'intensita'**, non `SphereOverlap` (D14 del brief — sigla locale, da non
 * confondere col `D-014` globale, che sono le azioni generiche). La differenza e' il punto del checkpoint:
 * una sfera attraversa i muri e ignora la topologia dei livelli, quindi darebbe una regola *geometrica* dove
 * il gioco ne ha una *topologica*. Il suono gira intorno agli ostacoli come farebbe un'unita', e sale una
 * rampa perche' la rampa e' un arco.
 *
 * Tutto puro e headless: nessun Actor, nessun `UWorld`, nessuna scrittura. E' anche il motivo per cui questa
 * libreria non vive nell'HUD ne' nel `TurnManager`, come la DoD chiede per nome — il TurnLog e la HUD sono
 * consumatori, e un consumatore che contenga la regola la rende inverificabile senza montare una partita.
 *
 * ⚠️ Questa e' la **prima cliente** di una query di propagazione che il brief §12 vuole generica: la
 * propagazione elettrica (CP 8.3) e il calore chiedono la stessa forma. Non e' stata generalizzata qui —
 * generalizzare su un solo cliente produce l'astrazione sbagliata — ma quando arrivera' il secondo, e'
 * questa la funzione da guardare.
 */
UCLASS()
class REFACTORTACTICS_API URTAcousticPropagationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Quanto rumore SPENDE un arco che non si puo' attraversare: muro, o porta chiusa.
	 *
	 * **2**, dal documento sorgente (`docs/archive/src/design/rumore-e-percezione-acustica.md`, «Wall
	 * Attenuation = 2», usato nel suo esempio). E' lo stesso documento che [D-042] ha fatto vincere sul
	 * workbook per l'acqua bassa, e con cui il `+1` del ghiaccio e' stato verificato per incrocio.
	 *
	 * ⚠️ **Da consolidare nel Decision Log**: a differenza dei modificatori di superficie, questo numero non
	 * ha ancora una voce `D-nnn`. E' dichiarato qui invece che inciso in una formula proprio perche' si veda.
	 *
	 * Un arco bloccato ATTENUA e non interrompe: il suono passa attraverso una porta, il movimento no. E' la
	 * differenza fra questa costante e il grafo — senza di essa `OcclusionAttenuates` e
	 * `UsesGraphNotEuclideanRadius` verificherebbero la stessa cosa.
	 */
	static constexpr int32 BlockedEdgeAttenuation = 2;

	/** Il costo acustico di base di un passo, prima del `NoiseDelta` della superficie in cui si entra. */
	static constexpr int32 StepCost = 1;

	/**
	 * Quanto una superficie ALZA il rumore di chi ci agisce sopra, dal catalogo terreni.
	 *
	 * ⚠️ **Amplifica alla sorgente, non attenua in transito**, ed e' il verso che [D-042] dichiara alla
	 * lettera: *«l'acqua bassa **aggiunge** +2 al rumore ... sprintarci fa 7 su 10, come un Dash su terreno
	 * libero»*. La scala lo conferma da sola — il fuoco (`+4`) crepita, non ovatta; il ghiaccio (`+1`)
	 * scricchiola sotto i passi; il fumo, che acceca, vale `0`.
	 *
	 * *(Il documento sorgente archiviato ha anche un `CellSoundAttenuation` in transito, ma senza valori e
	 * dichiarato brainstorming: una decisione consolidata batte una quantita' mai decisa.)*
	 *
	 * Una superficie assente dal catalogo (`Void`) vale `0`: non e' una scelta di bilanciamento, e' che
	 * nessuna unita' ci sta sopra a far rumore.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Noise")
	static int32 SurfaceNoiseDelta(ERTHexSurface Surface);

	/**
	 * Dove arriva un rumore, e con quanta intensita' residua. Ordine stabile (`StableLess`), mai quello di
	 * scoperta: la coda del flood fill dipende dall'ordine dei vicini, e senza ordinamento canonico due
	 * esecuzioni identiche produrrebbero due array diversi (invariante #3).
	 *
	 * La cella d'origine c'e' sempre, con l'intensita' piena. Le celle a `ReceivedNoise <= 0` **non**
	 * compaiono: «arriva a zero» e «non arriva» sono la stessa cosa, e tenerle costringerebbe ogni
	 * consumatore a rifiltrarle.
	 *
	 * `Map == nullptr` -> solo l'origine. Fail-closed, come `VisibleCells`: senza mappa non si valuta ne'
	 * superficie ne' arco, e propagare comunque sarebbe la piu' pericolosa delle due risposte.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Noise")
	static TArray<FRTNoiseReception> Propagate(const URTHexMapAsset* Map, const FRTNoiseEvent& Event);

	/**
	 * Il rumore che arriva in `ListenerCell`, o `0` se non ne arriva. Comodita' sul risultato di `Propagate`,
	 * non un secondo algoritmo.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Noise")
	static int32 NoiseAtCell(const TArray<FRTNoiseReception>& Received, const FRTCellId& ListenerCell);

	/**
	 * La soglia, e nient'altro: `ReceivedNoise >= HearingThreshold` (D-041).
	 *
	 * Sta in una funzione invece che in un `>=` sparso perche' e' **la** regola del canale acustico, e perche'
	 * il confronto ha un verso controintuitivo — soglia **bassa** = orecchio fine. Scritto a mano in due posti,
	 * il secondo prima o poi lo invertirebbe.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Noise")
	static bool IsAudible(int32 ReceivedNoise, int32 HearingThreshold);
};
