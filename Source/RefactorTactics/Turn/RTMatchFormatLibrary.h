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

	/**
	 * Errori dell'ACCOPPIATA formato/mappa (CP 19.1): una mappa disegnata per una classe diversa da quella che
	 * il formato richiede. Sta a parte da `ValidateFormat` perche' e' una domanda diversa — l'asset formato puo'
	 * essere perfettamente valido e restare sbagliato *per quella mappa* — e perche' i due sono validati in
	 * momenti diversi: il formato quando si carica, l'accoppiata quando si allestisce.
	 *
	 * Array vuoto = accoppiata utilizzabile. `nullptr` produce un errore, non un crash.
	 */
	static TArray<FString> ValidateAgainstMap(const FRTMatchRules& Rules, const class URTHexMapAsset* Map);

	/** Vero se le regole sono utilizzabili; in caso contrario `OutReason` elenca i motivi. */
	static bool AreRulesUsable(const FRTMatchRules& Rules, FString& OutReason);

	/**
	 * Regole in vigore a partire dall'asset. **Fail-closed**: con `Format` nullo o invalido ritorna false,
	 * compila `OutReason` e **non tocca** `OutRules` — il chiamante non allestisce a meta'.
	 */
	static bool ResolveRules(const URTMatchFormatData* Format, FRTMatchRules& OutRules, FString& OutReason);

	/**
	 * I formati SPEDITI col gioco, per identita' stabile. Ritorna `nullptr` per un id sconosciuto — il
	 * chiamante decide se ripiegare o rifiutare, come per `URTCatalogLibrary::FindCoreAction`.
	 *
	 * Perche' in C++ e non come `.uasset`: e' lo stesso posto in cui vivono le istanze di `URTActionData` e
	 * `URTHeroData` (`URTCatalogLibrary`, `URTHeroCatalogLibrary`). La CLASSE resta un data asset — un
	 * designer puo' crearne uno e assegnarlo al GameMode, e quello vince — ma il formato canonico della v0.1
	 * non puo' dipendere da un file che qualcuno deve ricordarsi di creare in editor: senza, il gioco
	 * pacchettizzato gira sul RIPIEGO, ed e' esattamente cio' che CP 12.5 ha misurato.
	 *
	 * Oggi ne contiene **uno**: `Format.Skirmish2v2`. Gli altri entrano quando un checkpoint li consuma —
	 * la stessa regola (D2) che tiene i campi della spec fuori da `URTMatchFormatData` finche' non hanno
	 * un lettore.
	 */
	static URTMatchFormatData* FindShippedFormat(FName FormatId);

	/** Gli id dei formati spediti, per i test e per la validazione dei dati. */
	static TArray<FName> ShippedFormatIds();

	/** Identita' del formato canonico della v0.1: 2v2 Skirmish. */
	static const FName Skirmish2v2FormatId;

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
