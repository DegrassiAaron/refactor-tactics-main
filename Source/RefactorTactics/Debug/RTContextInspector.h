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
 * Cosa ha chiesto chi ha digitato `rt.Debug.ContextInspector`.
 *
 * 🔴 **Esiste perche' `0` significava DUE cose, e una era spegnere.** La prima stesura trattava `0`
 * come l'interruttore, mentre in ogni altro `rt.Debug.*` di questo file il primo argomento e' il
 * **TeamId** — `rt.Debug.DrawIntent 1` mostra i piani del team 1. Chi scriveva `rt.Debug.ContextInspector
 * 0` intendendo *«la squadra 0»* spegneva il pannello, e non c'era modo di accorgersene se non a schermo.
 * ⚠️ La precondizione della voce `PIE-DEBUG-CONTEXT` diceva proprio quello: la seduta sarebbe fallita su
 * un pannello che funziona.
 *
 * Ora l'interruttore e' la **parola** `off`, che nessun `TeamId` puo' essere.
 */
USTRUCT()
struct FRTContextInspectorRequest
{
	GENERATED_BODY()

	/** `true` se il pannello va tolto dal viewport. */
	UPROPERTY()
	bool bOff = false;

	/** Per chi comporre, quando non si spegne. `0` e' la squadra 0, non un valore speciale. */
	UPROPERTY()
	int32 ObserverTeamId = 0;
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

	/**
	 * Gli argomenti di console, letti. **Pura**: nessun mondo, nessun widget — percio' provabile.
	 *
	 * | argomenti | esito |
	 * |---|---|
	 * | *nessuno* | mostra, squadra `0` |
	 * | `0`, `1`, … | mostra, per quella squadra |
	 * | `-1` | mostra, osservatore **onnisciente** (`RTObserver::Omniscient`) |
	 * | `off` | spegne (maiuscole indifferenti) |
	 *
	 * ⛔ **`0` NON spegne**, ed e' il difetto che questa funzione esiste per rendere impossibile: pinnato
	 * da `RefactorTactics.Debug.ContextInspectorArgsDoNotCollide`.
	 */
	static FRTContextInspectorRequest ParseCommandArgs(const TArray<FString>& Args);

	/** Le righe della vista in ordine di lettura: cella, intenti, eventi, tecnica. */
	static TArray<FString> AllLines(const FRTContextInspectorView& View);

	/**
	 * Le righe che ENTRANO in `MaxLines`, col taglio **dichiarato**. **Pura**: nessun widget, nessun
	 * mondo — percio' provabile, che e' l'intero punto di #3320.
	 *
	 * 🔴 **Esiste perche' il troncamento era muto e invisibile ai test.** `RebuildWidget` tagliava con
	 * un `IsValidIndex` dentro la Slate: `[tecnico]` e' l'ULTIMA riga di `AllLines`, quindi la prima a
	 * cadere. Misurato nella seduta `U59` del 2026-09-24: sulla cella `(q=0,r=0,L=0)` del banco
	 * `Visual.Combat.GuardVsBraceUnderSmallHits` la vista componeva **17** righe contro un tetto di 12.
	 *
	 * ⛔ **Cosa si perdeva davvero, perche' la prima stesura di questo commento lo diceva sbagliato.**
	 * NON l'osservatore: quello vive nell'intestazione (`GetHeaderText`), che ha uno slot proprio
	 * aggiunto PRIMA del ciclo e non e' soggetta al tetto — col pannello pieno si leggeva gia'
	 * `(q=0,r=0,L=0) — onnisciente`. A cadere era il resto della riga tecnica:
	 * `snapshot costruito per … · autorizza=… · rev=…`.
	 *
	 * ⚠️ **E quelli non sono un doppione dell'intestazione.** `vista per X` contro `snapshot costruito
	 * per Y` possono DIVERGERE, ed e' esattamente la divergenza per cui `DescribeCell` rifiuta di
	 * comporre l'occupante e stampa `NON-COMPOSTO(snapshot per X, vista per Y)`; `autorizza` dice se
	 * l'osservatore ne aveva diritto, e `rev` quale stato si sta guardando. Un pannello pieno che li
	 * perde non dice piu' **su quale stato** e **con quale titolo** e' composto.
	 *
	 * 🔑 E la meta' piu' grave era il taglio MUTO: chi guarda un pannello pieno conclude che su quella
	 * cella non sia successo altro.
	 *
	 * Cosa sopravvive al taglio, e perche':
	 *
	 * | riga | sorte | ragione |
	 * |---|---|---|
	 * | cella | **tenuta**, per prima | senza, non si sa di quale esagono si parla |
	 * | intenti + eventi | tagliate dal fondo | sono il corpo, ed e' quello che eccede |
	 * | marcatore | **inserito** | `AGENTS.md`: un'azione che non avviene deve dirlo |
	 * | tecnica | **tenuta**, per ultima | la provenienza, che era quella che cadeva |
	 *
	 * ⛔ Se nemmeno le riservate entrano in `MaxLines`, degrada al taglio secco: non c'e' spazio per
	 * dichiarare niente, e inventarlo produrrebbe un pannello che parla solo del proprio troncamento.
	 */
	static TArray<FString> VisibleLines(const FRTContextInspectorView& View, int32 MaxLines);

	/** Il prefisso del marcatore di taglio. Un DATO, cosi' che il test non lo riscriva a mano. */
	static const TCHAR* TruncationMarkerPrefix();

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
 * DOVE si posa il pannello.
 *
 * 🔴 **E' un dato e non quattro numeri sparsi nel layout**, per la stessa ragione misurata di
 * `FRTPieOverlayPlacement`: un posizionamento che vive dentro `RebuildWidget` non ha modo di essere
 * verificato senza uno schermo; questo si'.
 *
 * ⛔ **BASSO A DESTRA, e la scelta e' vincolata da tre misure, non dal gusto.**
 * 1. Il **centro non si copre**: e' il contratto dello Screen HUD (`progettazione-hud.md` §3.1), e la
 *    board e' cio' che si sta guardando.
 * 2. Ogni zona di §6 ha gia' un proprietario dichiarato — alto sinistra il **team roster**, alto destra
 *    l'**obiettivo**, basso sinistra l'**unita' selezionata**, lato destro il **team intent**, basso
 *    centro **ghost timeline** e **action dock** (§6.6, §6.7).
 *    ⌫ **Questa riga diceva «`§6` non assegna il basso destra», ed era falso**: `progettazione-hud.md`
 *    ha una **§6.8 «Bottom right»** e la assegna a `CONFIRM PLAN`, `UNDO`, stato piano, warning count e
 *    invalid state. L'enumerazione qui sopra saltava §6.1 e §6.8 e concludeva che l'angolo fosse libero:
 *    la posa e' stata scelta su una premessa che il documento citato smentisce. Corretta il 2026-09-24,
 *    trovata dalla seduta `U59` e dalla verifica di #3319.
 *    🔑 **La convivenza e' quindi DICHIARATA, non assente**, ed e' stata giudicata accettabile a schermo
 *    il 2026-09-24: il pannello condivide la fascia bassa con `§6.7` e `§6.8`. Cio' che resta vietato e'
 *    il **centro**, punto 1, dove si gioca.
 * 3. 🔑 E questo pannello viene giudicato **durante una seduta PIE**, quando `URTPieVerdictOverlay`
 *    occupa la colonna **sinistra, centrata in verticale** (#3242). Posarlo li' significherebbe ripetere
 *    esattamente il difetto che quella issue ha chiuso, un anno di lezioni dopo.
 */
struct FRTContextInspectorPlacement
{
	EHorizontalAlignment Horizontal = HAlign_Right;
	EVerticalAlignment Vertical = VAlign_Bottom;
	/** Distanza dai bordi destro e inferiore, in pixel di Slate. */
	float Margin = 24.f;
	/** ⛔ Il tetto esiste perche' il CENTRO resta libero: la board non si copre, per contratto. */
	float MaxWidth = 460.f;
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

	/**
	 * Tutte le righe, in ordine di lettura, per un layout che non voglia quattro liste.
	 *
	 * ⚠️ **Non tronca**, ed e' deliberato: chi vuole cio' che ENTRA a schermo chiede
	 * `GetVisibleLines()`. Tenerle separate e' cio' che rende il taglio una domanda con una risposta,
	 * invece di un effetto della Slate.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Debug")
	TArray<FText> GetLines() const;

	/**
	 * Le righe che ENTRANO nel tetto di resa, col taglio dichiarato — cio' che il pannello mostra
	 * davvero. E' `URTContextInspectorLibrary::VisibleLines` applicata a `MaxRighe` (#3320).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Debug")
	TArray<FText> GetVisibleLines() const;

	/** L'intestazione: quale cella, e per chi. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Debug")
	FText GetHeaderText() const;

	/**
	 * Il posizionamento che `RebuildWidget` applica — la sola parte del layout che e' un dato.
	 *
	 * ⚠️ `virtual`: un `WBP_` derivato puo' spostarlo senza riscrivere la Slate, che e' il motivo per cui
	 * la classe base esiste.
	 */
	virtual FRTContextInspectorPlacement Placement() const;

	/** La vista corrente. Sola lettura dal layout. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Debug")
	FRTContextInspectorView View;

	/**
	 * Quante righe la Slate costruisce, una volta sola.
	 *
	 * 🔴 **Slate si costruisce UNA volta e il contenuto cambia a ogni frame.** Aggiungere uno slot per
	 * riga dentro `RebuildWidget` congelerebbe il pannello alla PRIMA vista mostrata: una cella con piu'
	 * eventi della prima perderebbe le righe in eccesso, in silenzio. Gli slot sono fissi e le righe le
	 * riempiono per lambda; quelle che avanzano restano vuote.
	 *
	 * ⚠️ **Dodici e' un tetto di RESA, non un limite del contenuto**: `AllLines` restituisce tutto, e
	 * `VisibleLines` decide che cosa entra.
	 *
	 * ⌫ **Era `protected`, e questo commento diceva che il troncamento stava «dove si vede — invece che
	 * nel compositore, dove sarebbe invisibile ai test».** Il contrario: dentro `RebuildWidget` — Slate,
	 * `protected` — non era osservabile da NESSUN test, perche' `GetLines()` non tronca e a
	 * `2c9b7ea75^` l'header non dichiarava nessuna eccezione d'accesso per i test. Un test avrebbe
	 * dovuto riscrivere `12` a mano, cioe' il numero che invecchia da solo. Reso pubblico e spostato in
	 * `VisibleLines` con #3320.
	 */
	static constexpr int32 MaxRighe = 12;

protected:

	/**
	 * La resa, in C++ e senza `.uasset`: lo stesso `RebuildWidget` di `URTPieVerdictOverlay`.
	 *
	 * ⚠️ Cio' che sta qui dentro **non e' verificabile senza uno schermo**, ed e' la ragione per cui la
	 * sola parte che si puo' sbagliare in silenzio — la posa — e' un dato (`Placement`) invece di quattro
	 * numeri scritti qui.
	 */
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
