#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTMatchFormatData.h"
#include "RTMatchFormatLibrary.generated.h"

/**
 * Validazione e risoluzione del formato di partita. Pura, deterministica, senza stato.
 *
 * **Non ripiega mai**: senza formato (o con un formato invalido) rifiuta e dice perche'. La politica di
 * ripiego vive in `ARTGameMode`, dove sta gia' quella dell'arena generata — stessa separazione di
 * `MakeDemoArena`, che ritorna una mappa senza decidere di usarla (issue #185, spec §16.1).
 */
UCLASS()
class REFACTORTACTICS_API URTMatchFormatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Identita' riservata al formato di ripiego: distingue nel log una partita giocata SENZA formato dichiarato. */
	static const FName FallbackFormatId;

	/**
	 * Errori delle regole risolte: identita' assente, `RoundLimit` non positivo, soglia negativa.
	 * Array vuoto = regole utilizzabili (stessa forma di `ValidateMap`/`ValidateHeroes`).
	 */
	static TArray<FString> ValidateRules(const FRTMatchRules& Rules);

	/**
	 * Errori dell'asset: quelli delle regole che ne derivano, piu' `FormatVersion` non dichiarata e round
	 * attesi oltre il limite. `nullptr` produce un errore, non un crash. Elenca TUTTI gli errori
	 * indipendenti: chi corregge l'asset vuole la lista, non un errore per volta. I controlli DERIVATI da un
	 * campo gia' invalido non si sommano (con `RoundLimit` invalido, confrontarci i round attesi direbbe due
	 * volte lo stesso difetto).
	 */
	static TArray<FString> ValidateFormat(const URTMatchFormatData* Format);

	/** Vero se le regole sono utilizzabili; in caso contrario `OutReason` elenca i motivi. */
	static bool AreRulesUsable(const FRTMatchRules& Rules, FString& OutReason);

	/**
	 * Regole in vigore a partire dall'asset. **Fail-closed**: con `Format` nullo o invalido ritorna false,
	 * compila `OutReason` e **non tocca** `OutRules` — il chiamante non allestisce a meta'.
	 */
	static bool ResolveRules(const URTMatchFormatData* Format, FRTMatchRules& OutRules, FString& OutReason);

	/**
	 * Regole di RIPIEGO, valide secondo lo stesso validator: `RoundLimit` 12, cioe' il valore iniziale del
	 * 2v2 v0.1 (spec §6), e l'identita' riservata `FallbackFormatId`.
	 *
	 * Le ritorna soltanto: non le applica e non decide nulla. Chi ripiega e' `ARTGameMode`, che lo dichiara
	 * in Warning — un ripiego silenzioso non produce una partita rotta, produce numeri di playtest attribuiti
	 * a un formato fantasma (conseguenza vincolante di D1, issue #185).
	 */
	static FRTMatchRules MakeFallbackRules();
};
