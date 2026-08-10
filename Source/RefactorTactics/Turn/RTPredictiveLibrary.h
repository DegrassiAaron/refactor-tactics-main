#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Turn/RTHexSim.h"
#include "RTPredictiveLibrary.generated.h"

/**
 * Un colpo PREDITTIVO armato in pianificazione: una cella bloccata e chi l'ha scelta (D-016, CP 18.1).
 *
 * La cella e' `Locked` nel senso forte: viene scelta in Planning e non si rivaluta mai piu'. E' l'intera
 * differenza fra una previsione e un ordine — un'implementazione che al momento di sparare cercasse il
 * bersaglio piu' vicino colpirebbe sempre, e la scommessa sparirebbe. Lo scenario-specifica
 * `Spec.Predictive.WhiffOnEmptyCell` esiste per rendere quel difetto un rosso invece di una sfumatura.
 */
USTRUCT(BlueprintType)
struct FRTPredictiveShot
{
	GENERATED_BODY()

	/** Indice dell'unita' che ha armato il colpo, nell'array di unita' del turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Predictive")
	int32 SourceUnitId = INDEX_NONE;

	/** La cella dichiarata in Planning. Non viene rivalutata: e' il punto della meccanica. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Predictive")
	FRTCellId LockedCell;

	/** ID stabile dell'azione che ha armato: finisce nel TurnLog come chiave di lettura. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Predictive")
	FName ActionId;

	/** Unita' che questo colpo puo' colpire (ostili di chi l'ha armato), per indice. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Predictive")
	TSet<int32> Hostiles;

	FRTPredictiveShot() = default;
};

/**
 * Esito di UN colpo predittivo al boundary del movimento. Ogni colpo ne produce esattamente uno: o ha
 * trovato qualcuno (`bMatched`), o ha trovato il vuoto. Non esiste il terzo caso «in attesa» — e' cio' che
 * distingue una Predictive Action da una finestra di reazione (E14), e cio' che `NoResolverWait` verifica.
 */
USTRUCT(BlueprintType)
struct FRTPredictiveResolution
{
	GENERATED_BODY()

	/** Indice del colpo nell'array di input: rende l'esito attribuibile senza dipendere dall'ordine. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Predictive")
	int32 ShotIndex = INDEX_NONE;

	/** Vero se un'unita' ostile e' entrata nella cella bloccata. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Predictive")
	bool bMatched = false;

	/** Chi e' stato colto, se `bMatched`. `INDEX_NONE` sul whiff. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Predictive")
	int32 VictimUnitId = INDEX_NONE;

	/**
	 * Quanti passi della vittima restano validi: il suo `Entered` viene troncato a questa lunghezza, e
	 * l'ultimo passo e' la cella bloccata. Chi viene colpito ENTRA e si ferma li' — non viene respinto
	 * indietro, che sarebbe un altro effetto e un altro numero.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Predictive")
	int32 VictimStepsKept = 0;

	FRTPredictiveResolution() = default;
};

/**
 * Il boundary di risoluzione della **Predictive Action** (E18, CP 18.1), come funzione PURA.
 *
 * Perche' puro e non un pass dentro `ARTTurnManager`: il boundary e' una regola d'ordinamento, e una regola
 * d'ordinamento si dimostra solo se la si puo' chiamare due volte con gli stessi input permutati. Qui dentro
 * non ci sono `UWorld`, attori o tempo — `PermutationInvariant` non sarebbe scrivibile altrimenti.
 */
UCLASS()
class REFACTORTACTICS_API URTPredictiveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Risolve TUTTI i colpi predittivi contro le rotte gia' calcolate dal Move.
	 *
	 * Il boundary e' il movimento: `Moves[i].Entered` e' la sequenza di celle che l'unita' i attraversa, un
	 * passo per indice, e i passi sono SIMULTANEI fra unita' — il passo 0 di tutti avviene insieme. Un colpo
	 * scatta sulla PRIMA entrata ostile nella sua cella; a parita' di passo vince l'indice di unita' minore,
	 * e a parita' di entrambi l'indice di colpo minore. Tre chiavi totali e nessun pareggio residuo: e' cio'
	 * che rende l'esito indipendente dall'ordine degli array.
	 *
	 * Un colpo si consuma quando scatta, e una vittima troncata non entra piu' da nessuna parte: i colpi si
	 * influenzano a vicenda, quindi vanno processati in ordine crescente di passo e non uno alla volta fino
	 * in fondo. Processarli indipendentemente darebbe due colpi a segno su un'unita' che il primo aveva gia'
	 * fermato prima di arrivarci.
	 *
	 * @param Shots  I colpi armati, in qualunque ordine.
	 * @param Moves  Le rotte risolte, indicizzate per unita' (l'output di `ResolveHexPaths`).
	 * @return Un esito per colpo, nell'ordine dei colpi in input.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Predictive")
	static TArray<FRTPredictiveResolution> ResolvePredictions(const TArray<FRTPredictiveShot>& Shots,
		const TArray<FRTHexMoveResult>& Moves);

	/**
	 * Applica al risultato del Move i troncamenti decisi dal boundary: la vittima si ferma sulla cella
	 * bloccata, e `Final` torna coerente con `Entered`.
	 *
	 * Separata da `ResolvePredictions` perche' quella DECIDE e questa SCRIVE: chi vuole sapere cosa sarebbe
	 * successo (una preview, un ghost) chiama solo la prima.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Predictive")
	static void ApplyPredictionsToMoves(const TArray<FRTPredictiveResolution>& Resolutions,
		UPARAM(ref) TArray<FRTHexMoveResult>& Moves);
};
