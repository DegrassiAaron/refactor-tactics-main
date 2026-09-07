#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"

#include "RTMapTemplateLibrary.generated.h"

class ARTHexMapActor;
class URTHexMapAsset;

/**
 * PERCHE' un livello tattico non e' allestibile.
 *
 * ⚠️ **Enum distinto da `ERTMapValidationReason`, e non e' una svista.** Quello descrive un ASSET che si
 * contraddice — una cella dove nessuno puo' stare, un riparo irraggiungibile — e lo si valuta senza mondo.
 * Questo descrive il LIVELLO: quanti attori mappa ci sono, se portano l'asset, dove cadono i marker. Sono
 * due domande diverse su due soggetti diversi, e fonderle costringerebbe `ValidateMapDetailed` a conoscere
 * un `UWorld` che oggi non tocca.
 *
 * Aggiungere valori solo IN CODA: il valore finisce nei messaggi di Map Check e nei test che li pinnano.
 */
UENUM(BlueprintType)
enum class ERTMapTemplateIssue : uint8
{
	None,
	/** Il livello non ha nessun `ARTHexMapActor`: senza di lui non c'e' ne' origine ne' scala. */
	MissingMapActor,
	/**
	 * Piu' di uno. `ARTHexMapActor::FindInWorld` ritorna **il primo** dell'iterazione, e l'ordine di
	 * `TActorIterator` non e' garantito: due attori mappa significano che quale origine vinca lo decide
	 * l'ordine di caricamento, cioe' nessuno.
	 */
	DuplicateMapActor,
	/** L'attore mappa c'e' ma non porta l'asset autorevole: la partita girerebbe sul graybox di ripiego. */
	MissingMapAsset,
	/** Due marker dichiarano la stessa coppia `(TeamId, SlotIndex)`: due posti per la stessa unita'. */
	DuplicateSpawnSlot,
	/** La posizione del marker non cade su nessuna cella dell'asset. */
	SpawnOffMap
};

/** Un marker gia' RISOLTO: cio' che la validazione riceve, senza doversi procurare un mondo. */
USTRUCT(BlueprintType)
struct FRTSpawnPlacement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|MapTemplate")
	int32 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|MapTemplate")
	int32 SlotIndex = 0;

	/** Significativa solo se `bResolved`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|MapTemplate")
	FRTCellId Cell;

	/** La posizione cade su una cella che l'asset contiene davvero. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|MapTemplate")
	bool bResolved = false;

	/**
	 * Nome dell'attore che l'ha prodotto: serve a rimandare la segnalazione al marker giusto, e a dare
	 * all'ordinamento un ultimo criterio STABILE quando due marker condividono `(Team, Slot)`.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|MapTemplate")
	FString Label;
};

/** Una segnalazione, con abbastanza contesto perche' chi la legge sappia quale attore aprire. */
USTRUCT(BlueprintType)
struct FRTMapTemplateIssue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|MapTemplate")
	ERTMapTemplateIssue Reason = ERTMapTemplateIssue::None;

	/** Vuoto per le segnalazioni che riguardano il livello e non un marker. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|MapTemplate")
	FString Label;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|MapTemplate")
	int32 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|MapTemplate")
	int32 SlotIndex = 0;
};

/**
 * Le regole di allestimento di un livello tattico: cio' che deve essere vero prima che una mappa d'autore
 * si possa giocare.
 *
 * 🔑 **Pura e senza `UWorld`, come `URTMatchSetupLibrary`**: chi valida riceve gia' il conteggio degli
 * attori mappa e i marker risolti. E' cio' che rende queste regole verificabili headless — un test le
 * esercita senza aprire un livello, senza presentazione e senza PIE.
 *
 * ⛔ **Non e' un framework di validazione.** Non registra validator, non definisce categorie, non ha
 * estensioni: e' una funzione che confronta cio' che ha ricevuto con cinque regole. L'aggancio all'Editor e'
 * `CheckForErrors` sui due attori, cioe' l'infrastruttura Map Check che l'engine gia' possiede.
 */
UCLASS()
class REFACTORTACTICS_API URTMapTemplateLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * La cella su cui cade un punto-mondo, se l'asset la contiene davvero.
	 *
	 * ⚠️ **Passa da `URTHexLibrary::WorldToCellId`** e non da un arrotondamento scritto qui: la convenzione
	 * pointy-top ha un solo posto in cui vive, ed e' quello. Poi CHIEDE la cella all'asset — una coordinata
	 * calcolabile non e' una cella esistente, e un marker appoggiato fuori dal tabellone produce comunque
	 * una coppia assiale perfettamente valida.
	 */
	static bool ResolveWorldToCell(const URTHexMapAsset* MapAsset, const FVector& GridOrigin,
		float HexSize, float LayerHeight, const FVector& WorldLocation, FRTCellId& OutCell);

	/**
	 * Le cinque regole. `OutIssues` viene SVUOTATO e riempito in ordine STABILE — `(Reason, TeamId,
	 * SlotIndex, Label)` — perche' l'ordine con cui i marker arrivano dipende da `TActorIterator`, che non
	 * ne garantisce nessuno (invariante determinismo n. 3).
	 *
	 * ⚠️ **`MissingMapAsset` non si somma a `MissingMapActor`**: senza attore l'asset e' assente per
	 * conseguenza, e segnalare due volte una causa sola manderebbe chi legge a cercare due difetti.
	 *
	 * ⚠️ **Un gruppo duplicato produce UNA segnalazione per ogni marker coinvolto**, non una per gruppo:
	 * chi legge il Map Check clicca sull'attore, e un solo messaggio ne lascerebbe uno dei due muto.
	 */
	static void ValidateTemplate(int32 MapActorCount, const URTHexMapAsset* MapAsset,
		const TArray<FRTSpawnPlacement>& Spawns, TArray<FRTMapTemplateIssue>& OutIssues);

	/**
	 * Raccoglie dal livello cio' che `ValidateTemplate` pretende: quanti attori mappa, quale asset, e i
	 * marker gia' risolti in ordine stabile.
	 *
	 * E' l'unica funzione qui che tocca un `UWorld`, ed e' sottile apposta: non decide niente.
	 */
	static void CollectFromWorld(const UWorld* World, int32& OutMapActorCount,
		const URTHexMapAsset*& OutMapAsset, TArray<FRTSpawnPlacement>& OutSpawns);

	/** Il testo di una segnalazione, in una forma sola: Map Check e log non devono divergere. */
	static FString DescribeIssue(const FRTMapTemplateIssue& Issue);
};
