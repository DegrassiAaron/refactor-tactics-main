#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"
#include "Turn/RTTurnRules.h"
#include "RTCatalogLibrary.generated.h"

/**
 * Lettura e validazione del catalogo azioni: pura, deterministica, senza Actor e senza asset.
 *
 * Il validator e' una funzione pura per costruzione, cosi' a M11 puo' diventare un commandlet di CI senza
 * riscritture (stessa disciplina di URTHexMapAsset::ValidateMap).
 */
UCLASS()
class REFACTORTACTICS_API URTCatalogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Macro-fase di Atlas in cui l'azione risolve davvero. Funzione TOTALE: ogni valore dell'enum ha una
	 * macro-fase, nessun default silenzioso (il test lo verifica valore per valore).
	 *
	 * Rimappatura (ADR-0003 §3): Snapshot -> Planning · Preparation -> Prep · FastMovement -> **Dash** ·
	 * NormalMovement -> **Move** (dopo il Blast: qui il catalogo divergeva) · Control -> Blast ·
	 * Attack -> Blast · Environment -> Cleanup (dopo il Move) · Cleanup -> Cleanup.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static ERTMatchPhase MapResolutionPhase(ERTResolutionPhase Phase);

	/** Codice numerico del catalogo (0/10/20/30/40/50/60): serve a rileggere i PDF, non alla risoluzione. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static int32 ResolutionPhaseCode(ERTResolutionPhase Phase);

	/**
	 * Errori strutturali di un catalogo di azioni (vuoto = catalogo valido). Rifiuta:
	 * ID assente o duplicato · priorita' negativa · portata/costo/cooldown negativi · azione che dichiara di
	 * risolvere nello Snapshot (fase di congelamento: nessuna azione risolve li') · azione di movimento con
	 * fallback diverso da `Stop` (regola del vertical slice: ci si ferma nell'ultima cella valida).
	 *
	 * Ogni messaggio nomina l'azione colpevole: un errore che non dice QUALE riga e' rotta costringe a
	 * ricontrollare tutto il catalogo a mano.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Catalog")
	static TArray<FString> ValidateActions(const TArray<FRTActionDef>& Actions);
};
