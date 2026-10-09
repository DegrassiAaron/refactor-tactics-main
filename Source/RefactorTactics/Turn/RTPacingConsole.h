#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTPacingConsole.generated.h"

/** Che cosa ha chiesto chi ha digitato `rt.Debug.RecordPacing`. */
enum class ERTRecordPacingRequest : uint8
{
	/** Nessun argomento: dichiara lo stato e non tocca niente. */
	Query,
	/** `1`, `true`, `on`, `si`: il CSV va scritto. */
	Enable,
	/** `0`, `false`, `off`, `no`: il CSV non va scritto. */
	Disable,
	/** Argomento non interpretabile, o piu' di uno. Si rifiuta invece di indovinare. */
	Invalid,
};

/**
 * La parte di `rt.Debug.RecordPacing` che si puo' provare senza un mondo — #3398.
 *
 * 🔑 **Vive in una libreria e non dentro il comando**, per la stessa ragione di
 * `URTHexLosConsoleLibrary` (`Map/RTHexLosConsole.h`): un `FAutoConsoleCommand` prende un
 * `FOutputDevice` e non restituisce niente, quindi un test non puo' interrogarlo. Con la lettura degli
 * argomenti qui, *«un argomento non interpretabile viene rifiutato e non trattato come `1`»* diventa
 * un'assertion invece di una prova a occhio in PIE.
 */
UCLASS()
class REFACTORTACTICS_API URTPacingConsoleLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Interpreta gli argomenti di `rt.Debug.RecordPacing`.
	 *
	 * ⚠️ **`Invalid` non e' un sinonimo di `Disable`.** Un refuso come `rt.Debug.RecordPacing tru`
	 * deve produrre un rifiuto visibile: trattarlo come «spento» lascerebbe chi esegue `PIE-PACING-1`
	 * a giocare un turno intero convinto di stare registrando, e il difetto si scoprirebbe solo dopo,
	 * cercando un CSV che non esiste. E' l'unico modo in cui questo comando puo' far perdere una misura.
	 */
	static ERTRecordPacingRequest ParseArgs(const TArray<FString>& Args);

	/**
	 * Le righe che il comando stampa, una volta noto l'esito.
	 *
	 * ⚠️ **Dice sempre che i turni gia' chiusi non entrano nel CSV.** Il flag e' letto da
	 * `FRTPacingRecorder::Close`, quindi arma il **primo turno che si chiude da qui in avanti**: quelli
	 * conclusi prima restano solo in memoria, dove li legge `rt.Debug.Pacing`. `PIE-PACING-1` chiede
	 * *«una riga per turno giocato»*, e chi armasse a partita iniziata otterrebbe un CSV piu' corto della
	 * partita senza che niente glielo segnali.
	 *
	 * @param Request         che cosa e' stato chiesto
	 * @param bWasEnabled     lo stato del flag **prima** della chiamata
	 * @param CsvDir          la cartella in cui il CSV nascera'
	 * @param ExistingCsvPath il CSV di questa sessione se e' gia' stato creato, altrimenti vuoto
	 */
	static TArray<FString> Describe(ERTRecordPacingRequest Request, bool bWasEnabled,
		const FString& CsvDir, const FString& ExistingCsvPath);
};
