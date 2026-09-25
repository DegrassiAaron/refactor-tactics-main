#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "RTHexMapSummary.generated.h"

class URTHexMapAsset;

/**
 * CHE COSA CONTIENE la mappa aperta, come dato (#1186).
 *
 * 🔑 **Nasce da mezza giornata persa in seduta `U21`.** La domanda era *«quanti piani ha questa mappa?»*,
 * e il pannello del mode non la sapeva: la risposta è arrivata per quattro strade sbagliate di fila —
 * `rt.Arena.Check`, che riporta le celle e **non** i layer; un numero letto dal Play, che descriveva
 * un'altra mappa perché `MapSource=GeneratedTestArena` ignora quella del livello; una prenotazione emessa
 * su una necessità inesistente; e un cambio di `MapAsset` della sandbox che nessuno aveva notato.
 *
 * ⛔ **Nessun conteggio nuovo, ed è un vincolo della issue**: le celle vengono da `NumCells()`, i layer da
 * `GetLayers()`. Un secondo conteggio è una seconda risposta alla stessa domanda, e prima o poi le due
 * divergono — che è precisamente il difetto che `U21` ha pagato.
 *
 * 🔴 **Il caso che conta è `Map == nullptr`, e per questo si compone con una STATICA.** Un metodo
 * d'istanza non può dire *«non c'è asset»*: non lo si può chiamare. E dire *«non c'è»* invece di mostrare
 * degli zeri è un criterio esplicito del DoD, perché è **esattamente** la condizione in cui la sandbox si
 * è trovata senza che nessuno se ne accorgesse.
 */
USTRUCT(BlueprintType)
struct FRTHexMapSummary
{
	GENERATED_BODY()

	/** Se c'è davvero un asset. `false` non significa «mappa vuota»: significa «nessuna mappa». */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	bool bHasAsset = false;

	/** Il nome dell'asset collegato. Vuoto quando non ce n'è uno: la formattazione lo dice a parole. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	FString AssetName;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	int32 Cells = 0;

	/** I layer che ESISTONO, ordinati: da `GetLayers()`, mai ricontati. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<int32> Layers;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	int32 ActiveLayer = 0;

	/**
	 * Se il layer attivo è fra quelli che esistono.
	 *
	 * ⚠️ Un `ActiveLayer` che non esiste non è un errore — si dipinge su un piano nuovo proprio così — ma
	 * chi guarda deve poterlo distinguere da un piano già popolato, o crede di stare lavorando dove non c'è
	 * niente.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	bool bActiveLayerExists = false;
};

/**
 * Compone il riassunto e lo mette in parole.
 *
 * ⚠️ **Anche la formattazione vive nel runtime, ed è una scelta.** `spec-tactical-designer.md` §3 vuole la
 * logica di lettura qui; le tre `Descrivi*` ci stanno accanto perché sono ciò che un test può esercitare
 * **senza aprire l'Editor** — e il criterio del DoD è che pannello e libreria non possano divergere. Un
 * formattatore scritto dentro il pannello sarebbe la seconda risposta che questa issue esiste per evitare.
 */
UCLASS()
class REFACTORTACTICS_API URTHexMapSummaryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Legge l'asset. `Map` nullo è il caso normale, non un errore: vedi `bHasAsset`. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexMap")
	static FRTHexMapSummary Summarise(const URTHexMapAsset* Map, int32 ActiveLayer);

	/** Il nome dell'asset, o il fatto che non ce n'è. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexMap")
	static FString DescriviAsset(const FRTHexMapSummary& S);

	/** Quante celle — oppure perché la domanda non si pone. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexMap")
	static FString DescriviCelle(const FRTHexMapSummary& S);

	/** Quanti layer e **quali**: il conteggio da solo è ciò che `U21` aveva già, e non bastava. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexMap")
	static FString DescriviLayer(const FRTHexMapSummary& S);

	/** Il layer attivo, e se esiste fra quelli della mappa. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexMap")
	static FString DescriviLayerAttivo(const FRTHexMapSummary& S);

	/**
	 * CHE COSA DICE IL VALIDATORE sulla mappa aperta — il canale della casella 8 di `#1864`.
	 *
	 * 🔴 **Esiste perche' nel modulo Editor `ValidateMap` non la chiamava nessuno.** Misurato il
	 * 2026-09-24: un test e cinque commenti, **zero** chiamate di produzione. La issue chiede *«si rifiuta
	 * il gesto **o si segnala**»*; il rifiuto tipizzato c'era — `ERTMapEditOutcome` — e il segnalare non
	 * aveva una sede.
	 *
	 * ⛔ **Un `UE_LOG(Warning)` non e' quella sede**: l'Output Log e' un pannello che chi disegna deve
	 * avere aperto e scorrere, ed e' il difetto che `RTMapTemplateValidationHookTests` documenta — regole
	 * scritte, testate e verdi che non eseguiva nessuno. Il readout del mode invece si **vede guardando**,
	 * che e' il criterio con cui `#1186` ha deciso questo pannello.
	 *
	 * 🔑 **Prende il `Map` e non un `FRTHexMapSummary`, a differenza delle quattro qui sopra.** Quelle
	 * mettono in parole un riassunto gia' calcolato; questa deve interrogare il validatore, che e' un
	 * lavoro di natura diversa e di costo diverso. Infilarlo dentro `Summarise` avrebbe cambiato la
	 * semantica di una funzione che dichiara di *«leggere l'asset»*, e reso costoso ogni suo chiamante.
	 *
	 * ⚠️ **Chiama `ValidateMap()`, non `ValidateMapDetailed()`, e la seconda e' un SOTTOINSIEME della prima.**
	 * `ValidateMap` porta ventitre `Error:` e tre `Warning:` propri **e in coda chiama l'altra**,
	 * formattandone le voci: e' il superset. Un readout costruito su `ValidateMapDetailed` direbbe
	 * «nessuna segnalazione» su una mappa che `ValidateMap` dichiara in errore.
	 *
	 * ⛔ **`Map` nullo NON risponde «zero segnalazioni»**: uno zero si legge come «va tutto bene», ed e'
	 * la stessa bugia che `#1186` ha tolto agli altri readout dopo che la sandbox si era trovata con una
	 * mappa staccata senza che nessuno se ne accorgesse.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexMap")
	static FString DescriviValidazione(const URTHexMapAsset* Map);
};
