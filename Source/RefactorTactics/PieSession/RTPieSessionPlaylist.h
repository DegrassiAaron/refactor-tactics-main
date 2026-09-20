#pragma once

// Comporre la coda di una seduta da un selettore, e dichiarare cosa resta fuori (#3208).

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PieSession/RTPieSessionTypes.h"

#include "RTPieSessionPlaylist.generated.h"

/** Cosa produce un selettore: i passi, e — altrettanto importante — quello che non ci sta. */
struct FRTPieSessionPlan
{
	TArray<FRTPieSessionStep> Steps;

	/** Voci chieste che nessuno scenario dichiara: si stampano, non si ignorano. */
	TArray<FString> Excluded;

	/**
	 * Voci dichiarate da piu' di uno scenario. Quando ce n'e' una, `Steps` resta VUOTO: una coda che
	 * sceglie in silenzio quale allestimento usare e' peggio di nessuna coda, perche' chi guarda
	 * crederebbe di giudicare la voce nell'allestimento che ha in mente.
	 */
	TArray<FString> Ambiguous;

	bool IsRunnable() const { return Steps.Num() > 0 && Ambiguous.Num() == 0; }
};

UCLASS()
class REFACTORTACTICS_API URTPieSessionPlaylist : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Compone la coda.
	 *
	 * Due forme di selettore:
	 *  - **prefisso di scenario** (`Visual.Perception.*`, o un id con punti): seleziona SCENARI, e ogni
	 *    voce del loro `verifies` diventa un passo, nell'ordine in cui il file la dichiara. E' il
	 *    guadagno principale — un allestimento che copre sette voci si apre una volta;
	 *  - **elenco di voci** (`PIE-A,PIE-B`): seleziona VOCI, e per ciascuna si risale allo scenario che
	 *    la dichiara.
	 */
	static FRTPieSessionPlan Compose(const FString& Selector);

	/**
	 * Il campo `verifies` letto senza caricare l'intero scenario.
	 *
	 * ⚠️ Deliberatamente NON usa `URTScenarioLoader::LoadFromString`: quello valida tutto, e uno scenario
	 * con un intent malformato sparirebbe dalla coda invece di fallire con un motivo quando lo si avvia.
	 * E' la stessa ragione per cui `URTScenarioIndex::ReadHeader` esiste separato dal loader.
	 */
	static bool ReadVerifies(const FString& JsonText, TArray<FString>& OutVerifies);
};
