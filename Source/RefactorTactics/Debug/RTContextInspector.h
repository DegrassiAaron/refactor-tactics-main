#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Debug/RTDebugReportLibrary.h" // ERTContextView: le due modalita', gia' owner
#include "Map/RTCellId.h"
#include "RTContextInspector.generated.h"

struct FRTHexCellData;
struct FRTHexSnapshot;
struct FRTPlannedIntent;
struct FRTTurnLogEntry;
struct FRTPublicReplayEntry;

/**
 * IL CONTENUTO DEL CONTEXT INSPECTOR, gia' composto per un osservatore (#2485, handoff §4C).
 *
 * 🔑 **Porta solo `FString` derivate, e nessun tipo grezzo.** E' l'AC *«verificabile per assenza»*:
 * qui dentro non c'e' un `FRTPlannedIntent` ne' un `FRTTurnLogEntry` da cui qualcuno possa leggere cio'
 * che la vista non mostra. La vista non si costruisce e poi si nasconde — cio' che non e' autorizzato
 * non entra mai in questa struct. Lo misura `RefactorTactics.Debug.ContextViewCarriesNoRawTypes`, per
 * reflection.
 *
 * ⚠️ **`ObserverTeamId` e' dentro la vista, non solo un argomento del compositore.** Una vista che non
 * sapesse per chi e' stata composta si potrebbe mostrare al giocatore sbagliato senza che niente protesti,
 * ed e' la stessa forma di difetto che #2485 ha chiuso su `DescribeCell`: un dato corretto e **senza
 * provenienza**.
 */
USTRUCT(BlueprintType)
struct FRTContextInspectorView
{
	GENERATED_BODY()

	/** L'esagono ispezionato. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Debug")
	FRTCellId Cell;

	/** Per chi e' composta. `INDEX_NONE` = onnisciente, cioe' una superficie di audit. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Debug")
	int32 ObserverTeamId = INDEX_NONE;

	/** Se le righe tecniche sono state composte. ⛔ In Shipping resta `false`: non esistono. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Debug")
	bool bTechnical = false;

	/** La cella, da `URTDebugReportLibrary::DescribeCell`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Debug")
	FString CellLine;

	/** Gli intenti visibili, da `DescribeIntents` — che filtra con `FilterForTeam`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Debug")
	TArray<FString> IntentLines;

	/** Cos'e' successo su questa cella, dalla proiezione **pubblica** della traccia. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Debug")
	TArray<FString> EventLines;

	/** La provenienza della vista. Vuoto fuori da `Technical`, e sempre vuoto in Shipping. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Debug")
	TArray<FString> TechnicalLines;
};

/**
 * IL COMPOSITORE, nella forma gia' provata di `DescribeIntents`: prende il grezzo e passa dagli OWNER dei
 * due confini, invece di ricevere un risultato filtrato da qualcun altro senza sapere quale.
 *
 * | confine | owner, non riscritto qui |
 * |---|---|
 * | quali intenti | `URTIntentPrivacyLibrary::FilterForTeam`, via `DescribeIntents` |
 * | quale occupante | `URTHexCellVisibilityLibrary::SnapshotEntitles`, via `DescribeCell` |
 * | **quali voci** | `URTReplayPrivacyLibrary::FilterEntriesForObserver` |
 * | **quali colonne** | `URTReplayPrivacyLibrary::ToPublicTrace` |
 *
 * 🔴 **Le ultime due sono DUE domande e si compongono**, e lo dice l'owner alla lettera: *«chi vuole
 * entrambi i confini compone con `ToPublicTrace`»*. Una sola avrebbe lasciato o le voci altrui, o le
 * colonne di audit su voci proprie.
 */
UCLASS()
class REFACTORTACTICS_API URTContextInspectorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Il contesto dell'esagono, per un osservatore.
	 *
	 * ⚠️ **`AuditTrace` e' la traccia VIVA, non una deserializzata.** `FilterEntriesForObserver` legge
	 * `FRTTurnLogEntry::Verdict`, che e' `Transient`: su una traccia riletta da disco la maschera e'
	 * azzerata e il filtro, fail-closed, non lascia passare **nulla**. Un inspector su un replay deve
	 * quindi partire da una traccia gia' filtrata alla registrazione, non da questa funzione.
	 */
	static FRTContextInspectorView Compose(int32 ObserverTeamId, const FRTHexCellData& Cell,
		const FRTHexSnapshot& Snapshot, const TArray<FRTPlannedIntent>& Intents,
		const TArray<FRTTurnLogEntry>& AuditTrace, ERTContextView Mode);

	/** Le righe della vista in ordine di lettura: cella, intenti, eventi, tecnica. */
	static TArray<FString> AllLines(const FRTContextInspectorView& View);

	/**
	 * Una voce **pubblica** in una riga.
	 *
	 * ⛔ **NON passa da `URTTurnLogLibrary::DescribeEntry`, e la ragione e' misurata.** Quel traduttore
	 * legge `Entry.ReactionResponse`, che `URTReplayPrivacyLibrary::FieldVisibility()` classifica
	 * **`AuditOnly`**, e lo stampa alla lettera: usarlo qui violerebbe l'AC che questa classe esiste per
	 * soddisfare. ⚠️ **E svuotare quel campo non e' una redazione, e' una bugia**: senza il token la voce
	 * `Chosen` cade nel ramo del vuoto e si legge *«tiene il colpo, resta armata»* — cioe' il contrario di
	 * cio' che e' accaduto.
	 *
	 * 🔑 **E per questo la riga qui e' STRUTTURATA e non narrativa**: non e' una seconda traduzione in
	 * italiano dello stesso evento — che darebbe due frasi diverse per lo stesso fatto, il difetto che
	 * `RTDebugConsoleTests` nomina — ma un elenco di colonne pubbliche. La prosa resta di `DescribeEntry`,
	 * dove il combat log ha il proprio confine.
	 */
	static FString DescribePublicEvent(const FRTPublicReplayEntry& Entry);
};

/**
 * IL PANNELLO. Un consumatore **sottile**: riceve una vista gia' composta e la espone al layout.
 *
 * 🔑 **In C++ e senza `.uasset`, deliberatamente** — la stessa scelta di `URTPieVerdictOverlay`,
 * e per la stessa ragione: l'authoring di un widget appartiene al clone principale, e una logica che
 * vivesse nel Blueprint non avrebbe modo di essere rossa senza aprirlo. Chi vuole una resa curata deriva
 * un `WBP_` da questa classe ed eredita il comportamento senza toccarlo.
 *
 * ⛔ **Nessuna decisione vive qui, e nessun filtro.** Se questo widget dovesse decidere cosa nascondere,
 * la vista sarebbe gia' stata costruita sbagliata: e' precisamente cio' che #1805 vieta — *«la privacy
 * non e' non disegnare, e' non costruire la vista»*. Qui si disegna tutto cio' che si riceve.
 */
UCLASS()
class REFACTORTACTICS_API URTContextInspectorWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Arma il pannello con una vista gia' composta. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Debug")
	void ShowFor(const FRTContextInspectorView& InView);

	/** Il pannello ha qualcosa da mostrare? Una cella senza righe non merita di coprire la board. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Debug")
	bool HasContent() const;

	/** Tutte le righe, in ordine di lettura, per un layout che non voglia quattro liste. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Debug")
	TArray<FText> GetLines() const;

	/** L'intestazione: quale cella, e per chi. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Debug")
	FText GetHeaderText() const;

	/** La vista corrente. Sola lettura dal layout. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Debug")
	FRTContextInspectorView View;
};
