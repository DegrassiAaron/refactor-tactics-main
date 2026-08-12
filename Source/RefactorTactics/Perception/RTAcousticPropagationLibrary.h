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

	/**
	 * I due raggi della scala di precisione, dal §13 del documento sorgente sul rumore — *«Detection Level 2
	 * — Area larga: possibile sorgente entro 4-5 celle»* e *«Level 3 — Area stretta: entro 2 celle»*.
	 *
	 * ⚠️ **Sono DICHIARATI, non dedotti, ed e' la correzione che questo checkpoint ha dovuto fare.** La prima
	 * stesura ricavava il raggio all'indietro da un budget massimo teorico (cima della scala piu' la
	 * superficie piu' rumorosa): aritmeticamente onesto e praticamente inutile, perche' produceva aree di
	 * 271 celle per uno sprint sentito a due passi — cioe' «da qualche parte», che non e' un contatto.
	 * Il documento sorgente il numero ce l'aveva gia'.
	 *
	 * `4` e non `5` per la fascia larga: il sorgente scrive «4-5» e si prende l'estremo prudente, che e'
	 * anche quello che tiene l'area sotto le 61 celle.
	 */
	static constexpr int32 WideAreaRadius = 4;
	static constexpr int32 TightAreaRadius = 2;

	/**
	 * Di quanto un rumore deve superare la soglia d'udito per passare da fascia larga a fascia stretta.
	 *
	 * ⚠️ **E' il solo numero di questo checkpoint che nessun documento possiede**, e va detto invece che
	 * nasconderlo in una formula. Il sorgente dice *cosa* distingue i livelli (specializzazione, skill,
	 * distanza, riconoscibilita') e propone una statistica per eroe `NoiseIdentificationLevel` che il
	 * progetto **non ha**: aggiungerla sarebbe quattro numeri di bilanciamento nuovi, cioe' una decisione
	 * come [D-041] per l'udito. Finche' non esiste, il discriminante e' il **margine sopra la soglia**, che
	 * e' informazione che l'ascoltatore ha gia' e non costa nessun dato nuovo: piu' forte l'ha sentito, piu'
	 * vicina era la sorgente.
	 */
	static constexpr int32 TightBandMargin = 3;

	/**
	 * Valvola di sicurezza sul numero di celle, non un criterio di gioco: con i raggi dichiarati l'area sta
	 * fra 19 e 61 celle, quindi questo tetto non scatta mai in partita. Esiste perche' una mappa futura con
	 * archi a costo zero renderebbe il flood fill illimitato, e un budget di CONTEGGIO e' la difesa che il
	 * progetto usa ovunque — mai un limite sul tempo, che dipenderebbe dalla macchina.
	 */
	static constexpr int32 MaxPlausibleCells = 128;

	/**
	 * Da dove PUO' essere venuto un rumore udito in `ListenerCell` con intensita' residua `ReceivedNoise`.
	 *
	 * ⚠️ **Non riceve la sorgente, e non e' un'omissione di comodo: e' la firma che rende onesto il canale
	 * acustico.** Il risultato si costruisce dal solo punto d'ascolto e da cio' che ha sentito, quindi non
	 * puo' contenere informazione che l'ascoltatore non ha. Una versione che prendesse l'evento vero
	 * restituirebbe la stessa area *e sarebbe indistinguibile da una che restituisce la cella esatta*: la
	 * garanzia di `D13` vive qui, in cio' che il parametro NON permette.
	 *
	 * Il criterio e' la scala del §13 del sorgente, non un'inferenza: un rumore che supera la soglia di
	 * almeno `TightBandMargin` e' arrivato forte, quindi la sorgente era vicina e l'area e' **stretta**
	 * (`TightAreaRadius`); altrimenti e' **larga** (`WideAreaRadius`). Il raggio si misura in costo acustico
	 * dal punto d'ascolto — non in celle euclidee — cosi' un muro allarga l'incertezza invece di ignorarla.
	 *
	 * ⚠️ L'area e' centrata sull'ASCOLTATORE, non sulla sorgente, e non e' un ripiego: la **direzione** e' il
	 * Livello 1 della stessa scala, ed e' informazione che questo checkpoint non concede. Un'area centrata
	 * sulla sorgente ne codificherebbe la posizione nel proprio centro.
	 *
	 * Vuoto se la mappa manca (fail-closed, come `Propagate`), se `ReceivedNoise <= 0` o se il rumore non
	 * supera la soglia. Ordine stabile (`StableLess`).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Noise")
	static TArray<FRTCellId> PlausibleOriginCells(const URTHexMapAsset* Map, const FRTCellId& ListenerCell,
		int32 ReceivedNoise, int32 HearingThreshold);
};
