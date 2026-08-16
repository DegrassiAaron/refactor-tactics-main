#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTScenarioIndex.generated.h"

/**
 * Una voce dell'indice: **dove** vive uno scenario e con **quali parole** lo si trova.
 *
 * `ScenarioId` è dichiarato dal file, non dedotto dal percorso. È la differenza che rende possibile
 * spostare uno scenario in un'altra cartella senza cambiarne l'identità — e raggrupparlo per più criteri
 * insieme, cosa che una gerarchia di cartelle non sa fare.
 */
struct FRTScenarioEntry
{
	/** Identità stabile, dichiarata nel campo `scenarioId` del file. */
	FString ScenarioId;

	/** Percorso assoluto del file che lo contiene. Dettaglio di storage: nessuno lo scrive a mano. */
	FString Path;

	/** Parole per cui filtrare, normalizzate a minuscolo e ordinate. Tipologia e lente sullo stesso asse. */
	TArray<FString> Tags;
};

/**
 * Indice degli scenari: `ScenarioId -> { percorso, tag }`.
 *
 * Prima di questo indice l'ID **era** il percorso (`Movement.Basic` -> `Scenarios/Movement/Basic.json`), e
 * la cartella era quindi l'unico raggruppamento possibile. Non basta: lo stesso scenario si apre per
 * verificare una regola oppure per guardare un'animazione, e queste lenti si incrociano fra loro
 * (`reactions` **e** `gadget`). Una gerarchia sola ne esprime una.
 *
 * Staccando l'ID dal percorso si paga un prezzo — due file possono ora dichiarare lo stesso `scenarioId`,
 * cosa che il filesystem prima rendeva impossibile — e l'indice è il solo posto dove accorgersene:
 * `ResolvePath` rifiuta un ID ambiguo invece di sceglierne uno.
 *
 * Funzioni pure dove possibile: `ReadHeader`, `BuildFrom` e `ApplyRedirects` non toccano il disco e si
 * verificano headless.
 */
UCLASS()
class REFACTORTACTICS_API URTScenarioIndex : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * File della tabella di redirect, sotto `Scenarios/`. Il prefisso `_` non è decorativo: i file che
	 * cominciano per underscore **non sono scenari** e la scansione li salta, altrimenti la tabella
	 * comparirebbe nella tendina come uno scenario senza unità.
	 */
	static const TCHAR* RedirectsFileName;

	/** Un redirect può puntare a un altro redirect, ma non all'infinito: oltre questo salto è un ciclo. */
	static constexpr int32 MaxRedirectHops = 8;

	/**
	 * Legge dal JSON di uno scenario **solo** ciò che serve all'indice: `scenarioId` e `tags`.
	 *
	 * Non interpreta unità, turni e assertion: costruire l'indice non deve costare quanto caricare ogni
	 * scenario, e soprattutto uno scenario con un intent malformato deve restare **trovabile** — altrimenti
	 * un errore in fondo al file lo farebbe sparire dalla tendina invece di farlo fallire con un motivo.
	 */
	static bool ReadHeader(const FString& JsonText, FString& OutId, TArray<FString>& OutTags, FString& OutError);

	/**
	 * Costruisce l'indice da coppie `(percorso, testo JSON)` già in memoria. Pura: è la funzione che i test
	 * esercitano senza scrivere file.
	 *
	 * @param OutProblems una riga per ogni file illeggibile o ID duplicato. Non svuota l'indice: un file
	 *        rotto non deve nascondere gli altri.
	 */
	static TArray<FRTScenarioEntry> BuildFrom(const TArray<TPair<FString, FString>>& Files, TArray<FString>& OutProblems);

	/** Come `BuildFrom`, leggendo ricorsivamente i `.json` sotto `Scenarios/`. Salta i file che cominciano per `_`. */
	static TArray<FRTScenarioEntry> Scan(TArray<FString>& OutProblems);

	/**
	 * Percorso del file che dichiara questo `ScenarioId`, seguendo i redirect se l'ID non risulta più.
	 *
	 * Stringa vuota + `OutError` valorizzato quando l'ID non esiste **oppure** quando è ambiguo. Un ID
	 * dichiarato da due file non fa vincere il primo trovato: sceglierne uno in silenzio significherebbe
	 * eseguire uno scenario diverso da quello che si è chiesto.
	 */
	static FString ResolvePath(const FString& ScenarioId, FString& OutError);

	/**
	 * ID degli scenari che portano **entrambi** i tag dati, in ordine alfabetico. Un filtro vuoto non
	 * restringe nulla, quindi `ListIds("", "")` è l'elenco completo.
	 *
	 * Intersezione e non unione: il caso che serve è «reactions **e** gadget», non «reactions o gadget».
	 */
	static TArray<FString> ListIds(const FString& FilterA, const FString& FilterB);

	/**
	 * Vocabolario dei tag: l'unione di quelli **realmente presenti** nei file, ordinata.
	 *
	 * Nessun elenco dichiarato da qualche parte, per la stessa ragione per cui la tendina degli scenari
	 * legge i file: un vocabolario scritto a mano invecchia, e una categoria vuota nel menu è un invito a
	 * cercare qualcosa che non c'è. Il prezzo è che un tag scritto male compare come voce nuova — il che
	 * lo rende visibile subito, invece che silenzioso.
	 */
	static TArray<FString> ListTags();

	/** Tabella `vecchio ID -> nuovo ID` da `Scenarios/_redirects.json`. File assente = mappa vuota, non un errore. */
	static TMap<FString, FString> LoadRedirects();

	/** Segue la catena di redirect fino a un ID che non ne ha altri, o fino a `MaxRedirectHops`. Pura. */
	static FString ApplyRedirects(const TMap<FString, FString>& Redirects, const FString& ScenarioId);

	// ⚠️ L'esempio qui sotto mostra lo STESSO tag in due forme, ed e' l'unico modo in cui puo' dimostrare
	// qualcosa. Il rename del roster (#753) lo aveva rotto proprio li': la riga usava il nome ritirato
	// dell'eroe maiuscolo e minuscolo, il replace ha aggiornato solo la forma maiuscola, e l'esempio si e'
	// trovato a confrontare due tag DIVERSI — vero come frase, muto come esempio.
	// Un rename che passa sulla prosa puo' invalidare un esempio senza rompere il codice, e nessun gate lo
	// vede perche' il file compila: se un giorno queste due parole smettono di essere la stessa, e' rotto.
	/** Forma canonica di un tag: senza spazi ai bordi, minuscolo. `Gadget` e `gadget ` sono lo stesso filtro. */
	static FString NormalizeTag(const FString& Tag);
};
