#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTHexLosConsole.generated.h"

struct FRTCellId;
struct FRTLineOfSightResult;

/**
 * La resa testuale del verdetto LOS — la meta' di `#1712` che si puo' provare senza un mondo.
 *
 * 🔴 **Vive in una libreria e non dentro il comando**, ed e' il motivo per cui esiste questo file: un
 * `FAutoConsoleCommand` prende un `FOutputDevice` e non restituisce niente, quindi un test non puo'
 * interrogarlo. Con la composizione qui, il comando diventa tre righe di raccolta argomenti e cio' che
 * afferma — *«nomina tutte e quattro le cause, e dichiara il layer»* — e' un'assertion invece di una
 * verifica a occhio in PIE.
 */
UCLASS()
class REFACTORTACTICS_API URTHexLosConsoleLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Il verdetto reso leggibile: che cosa ferma la linea, dove, e **su quale layer** si sta ragionando.
	 *
	 * ⚠️ **Il layer si dichiara sempre, anche quando e' zero**, ed e' il caveat che `#1712` chiede
	 * esplicitamente: `HasLineOfSight` **non guarda il layer** — la linea resta su quello del tiratore, ed e'
	 * la regola d'elevazione dichiarata in `RTHexVisionLibrary.h:27` (*«da terra si spara sotto un ponte»*).
	 * Su una mappa multilivello una risposta che tace sul layer si legge come se valesse per tutti, e non e'
	 * vero. Tacerlo sarebbe il difetto piu' facile da introdurre e il piu' difficile da vedere.
	 *
	 * @param Result   il verdetto prodotto da `URTHexVisionLibrary::DescribeLineOfSight`
	 * @param From     la cella del tiratore, il cui layer governa l'intera linea
	 * @param To       la cella del bersaglio
	 */
	static TArray<FString> DescribeVerdict(const FRTLineOfSightResult& Result, const FRTCellId& From,
		const FRTCellId& To);
};
