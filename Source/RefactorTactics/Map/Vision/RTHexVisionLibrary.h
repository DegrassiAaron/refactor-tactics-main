#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTHexVisionLibrary.generated.h"

class URTHexMapAsset;

/**
 * PERCHE' la linea di tiro e' bloccata. Tre valori oltre a `None`, e sono tre perche' il corpo di
 * `HasLineOfSight` distingue esattamente tre predicati su tre domini diversi — non perche' tre sembrassero
 * il numero giusto.
 *
 * ⛔ **Non e' una tassonomia estendibile a piacere.** Aggiungere un valore qui significa che
 * `DescribeLineOfSight` ha imparato a distinguere una causa che prima confondeva, e va aggiunto **li'**:
 * un valore che nessun ramo produce e' un'etichetta che il debug non emettera' mai.
 */
UENUM(BlueprintType)
enum class ERTLineOfSightBlock : uint8
{
	/** Via libera. E' anche la risposta dei tre casi che rispondono `true` senza che ci sia nulla in mezzo. */
	None = 0,

	/**
	 * Il BORDO attraversato nega l'attraversamento: `URTHexCoverLibrary::BlocksTraversal` (CP 9.2/9.3).
	 *
	 * ⚠️ **Non e' solo la copertura alta**, e il nome del valore non deve promettere di piu' di cio' che il
	 * predicato sa: `BlocksTraversal` risponde `true` anche per una PORTA chiusa o bloccata. Distinguere
	 * qui i due sottocasi vorrebbe dire fare al dominio copertura una seconda domanda e poi dedurre — cioe'
	 * costruire la seconda verita' che questa struttura esiste per impedire. Chi vuole il dettaglio chiede a
	 * `URTHexCoverLibrary`, che ne e' l'autorita'.
	 *
	 * Conta anche il PRIMO e l'ULTIMO passo: e' una proprieta' FRA due celle, non di una cella.
	 */
	EdgeBlocker = 1,

	/** La CELLA attraversata porta `FRTHexCellData::bBlocksLineOfSight`. Gli estremi sono esclusi. */
	CellBlocker = 2,

	/**
	 * La GEOMETRIA INTRA-CELLA ha fermato la linea: un `FRTHexInteriorWall` alto, dentro una cella,
	 * incrociato dalla corda d'attraversamento — `D-269`, `D-270`, `#1830`.
	 *
	 * 🔑 **Gli estremi NON sono esclusi**, a differenza di `CellBlocker`, e non e' un'incoerenza: un muro
	 * dentro la cella del tiratore sta FRA lui e l'uscita, non sopra di lui. E' la stessa ragione per cui
	 * `EdgeBlocker` conta il primo e l'ultimo passo — cio' che si esclude e' che qualcuno si copra da solo,
	 * non che una barriera davanti a lui smetta di esistere.
	 *
	 * ⚠️ **Non dice quale muro**: la ragione nomina la CELLA (`BlockedAt`), non il segmento. Chi vuole il
	 * dettaglio lo chiede a `URTHexOcclusionLibrary`, che ne e' l'autorita' — la stessa disciplina con cui
	 * `EdgeBlocker` non distingue la copertura alta dalla porta chiusa.
	 */
	InteriorGeometry = 3,
};

/**
 * L'esito della linea di tiro con la sua ragione e il suo punto: cio' che `HasLineOfSight` riduce a un bool.
 *
 * 🔑 **Il layer non ha un campo proprio, e non e' una dimenticanza.** `URTHexLibrary::HexLine` tiene la
 * linea sul layer del TIRATORE (regola d'elevazione, `HasLineOfSight` qui sotto), quindi il layer su cui si
 * sta ragionando e' `From.Layer` e lo portano anche `BlockedFrom` e `BlockedAt`. Un overlay che dichiara il
 * layer lo legge da li' invece di riceverne una copia che potrebbe non coincidere.
 */
USTRUCT(BlueprintType)
struct FRTLineOfSightResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTLineOfSightBlock Block = ERTLineOfSightBlock::None;

	/** La cella in cui la linea stava ENTRANDO quando il blocco e' scattato. Ha senso solo se `Block != None`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId BlockedAt;

	/**
	 * La cella da cui si entrava. Con `EdgeBlocker` il bordo colpevole e' il lato `BlockedFrom -> BlockedAt`;
	 * con `CellBlocker` e' solo il passo precedente, e la colpevole e' `BlockedAt`.
	 *
	 * Con `InteriorGeometry` e' la cella da cui la corda ENTRA in `BlockedAt` — e vale `BlockedAt` stessa
	 * quando il muro sta nella cella del tiratore, che e' il caso in cui non si entra da nessuna parte.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId BlockedFrom;

	/**
	 * Indice lungo `HexLine` della cella in cui il blocco e' scattato, cioe' di `BlockedAt`.
	 * `INDEX_NONE` quando la vista passa.
	 *
	 * ⚠️ Vale `1..N-1` per `EdgeBlocker` e `CellBlocker`, che sono proprieta' di un PASSO; puo' valere anche
	 * `0` per `InteriorGeometry`, che e' una proprieta' di una CELLA e comprende quella del tiratore.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 StepIndex = INDEX_NONE;

	bool IsClear() const { return Block == ERTLineOfSightBlock::None; }
};

/**
 * Linea di vista sulla mappa esagonale: decide se un bersaglio e' colpibile (logica autoritativa, non
 * presentazione). Pura e deterministica, interi soltanto (la linea viene da URTHexLibrary::HexLine).
 *
 * Separata da URTHexLibrary perche' ha bisogno dei dati d'asset (bBlocksLineOfSight), come URTHexPathLibrary
 * per il grafo di movimento. Vedi docs/technical/systems/h6-4-hex-vision-spec.md.
 */
UCLASS()
class REFACTORTACTICS_API URTHexVisionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Vero se la linea di tiro From->To non attraversa celle che bloccano la vista.
	 *
	 * Regola di ELEVAZIONE (ereditata dalla LOS quadrata, rimossa al CP 7.2): un ostacolo blocca solo se
	 * sta sul layer del TIRATORE -> da terra si spara sotto un ponte, da un piano superiore si spara oltre le
	 * coperture basse. Gli ESTREMI non bloccano mai (tiratore e bersaglio non si coprono da soli). Una cella
	 * ASSENTE dall'asset non blocca (il vuoto non e' un muro). Mappa nulla o From == To -> vero.
	 *
	 * 🔑 **La firma non e' cambiata, e il corpo non decide piu' da solo**: delega a `DescribeLineOfSight` e
	 * ne butta via la ragione. E' l'unico modo per cui osservabilita' e autorita' NON possono divergere —
	 * vedi il perche' esteso sulla funzione che segue.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool HasLineOfSight(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);

	/**
	 * La stessa decisione di `HasLineOfSight`, con la RAGIONE e il PUNTO in cui la linea si ferma.
	 *
	 * ## 🔴 Perche' e' questa la primitiva, e `HasLineOfSight` il guscio
	 *
	 * La forma ovvia sarebbe una funzione «accanto» che ripercorre `HexLine` con le stesse due condizioni, e
	 * un test di parita' su un corpus di coppie a sorvegliarle. Quella forma ha un difetto che nessun test
	 * chiude del tutto: sono **due** implementazioni, la parita' vale sul corpus provato, e il giorno in cui
	 * una regola cambia in una sola delle due il debug comincia a mentire proprio quando serve.
	 *
	 * Qui la parita' non e' asserita, e' **strutturale**: esiste un solo attraversamento della linea, e il
	 * bool e' `Reason == None`. Non c'e' una seconda LOS da tenere allineata perche' non c'e' una seconda LOS.
	 *
	 * ⚠️ Il vincolo che #1712 pone — *«la reason non si ottiene cambiando la firma di `HasLineOfSight`»* —
	 * resta rispettato: la firma e' identica a prima, e i suoi quattro test di comportamento
	 * (`RefactorTactics.HexVision.*`) sono gli stessi e restano verdi. Cio' che e' cambiato e' da dove il
	 * `bool` viene, non cosa vale.
	 *
	 * ⛔ **Sola lettura, come `HasLineOfSight`**: nessuno stato, nessun Actor, nessun `UWorld`. Chiamabile
	 * headless e dal modulo Editor, che e' il punto: un ispettore d'editor consuma QUESTA e non si scrive
	 * una LOS propria.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static FRTLineOfSightResult DescribeLineOfSight(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);
};
