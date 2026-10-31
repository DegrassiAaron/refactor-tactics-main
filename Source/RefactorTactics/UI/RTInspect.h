#pragma once

#include "CoreMinimal.h"
#include "Ability/RTEquipmentData.h"
#include "Map/RTCellId.h"
#include "Perception/RTKnowledgeView.h"
#include "Player/RTPointerInteraction.h" // ERTPointerContext: quando la scheda si nasconde (#3748)
#include "UI/RTHudViewModel.h"
#include "RTInspect.generated.h"

/**
 * LA SCHEDA DELL'ISPEZIONE: il soggetto pubblico e il compositore delle righe (`#3748`, fetta F3 della spec
 * `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`).
 *
 * 🔑 **Il compositore non vede mai l'attore.** Riceve un soggetto di VALORI, ricavato da una sola funzione che legge
 * l'attore (`ARTHUD::BuildInspectSubject`), e restituisce righe di testo. Del nemico entrano nome, carta, stati, verso
 * ed equipaggiamento (pubblici per decisione d'autore del 2026-10-20); NON entrano il piano, gli slot del turno,
 * l'azione armata, le ricariche vere: il soggetto non ha un campo in cui metterli.
 *
 * ⛔ **Il kit non e' sulla scheda.** Lo mostra la barra delle skill dell'unita' cliccata, in sola lettura
 * (`URTHudViewModel::BuildPublicBar`, decisione d'autore del 2026-10-30): il soggetto non ha un campo per il kit. La scheda mostra i
 * dettagli di UNA skill, quella sotto il cursore (qui sotto), e mai l'elenco del kit.
 *
 * 🔑 **La scheda e' una TESTATA e, col cursore su uno slot della barra, i DETTAGLI di quella skill** (decisione d'autore del
 * 2026-10-31): `FRTInspectSubject::HoveredSkill` porta cio' che il tooltip dello stesso slot dice, piu' i pezzi di equipaggiamento che
 * la riguardano e la stima della ricarica se la squadra l'ha vista usare. Senza cursore, la testata chiude con una riga di invito.
 */

/** Un pezzo di equipaggiamento: nome, slot, vantaggio e svantaggio d'autore. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTInspectEquipmentLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText Name;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	ERTEquipmentSlot Slot = ERTEquipmentSlot::Gadget;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText Advantage;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText Drawback;
};

/**
 * Una ricarica DEDOTTA (`#3751`, F6): l'ultimo turno in cui la squadra ha visto usare l'azione, e il turno da cui,
 * a quel conteggio, torna disponibile. E' una STIMA e si legge come tale: un effetto non visto che allunghi o accorci
 * la ricarica la rende sbagliata.
 *
 * ⛔ Quattro campi e nessuno e' la ricarica vera: non c'e' un contatore residuo, ne' un `bUsableNow`. Il test
 * `UI.InspectDeducedRowCarriesNoTrueCooldown` lo pinna per reflection.
 */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTInspectDeducedCooldown
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FName ActionId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText Title;

	/** L'ultimo turno in cui la squadra ha visto usare l'azione. Sempre >= 1: senza avvistamento non c'e' riga. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	int32 LastSeenTurn = 0;

	/** `LastSeenTurn` + durata di catalogo: il turno di pianificazione in cui l'azione, a questo conto, e' di nuovo usabile. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	int32 AvailableFromTurn = 0;
};

/**
 * I DETTAGLI DELLA SKILL SOTTO IL CURSORE (`#3748`, decisione d'autore del 2026-10-31): cio' che il box di destra mostra di
 * UNA posizione di kit, quando il cursore e' sul suo slot della barra in sola lettura.
 *
 * 🔑 **Stessa riga di catalogo del tooltip dello slot, nessuna seconda fonte**: titolo e righe vengono da
 * `URTHudViewModel::BuildCatalogActionRow(Action, Index, NAME_None)` e `BuildCatalogTooltip`, via `ComposeTooltipText`. Descrizione,
 * fase, slot, portata, forma, ricarica di catalogo e danno non si rileggono altrove. `NAME_None` perche' la variante attiva e' stato di
 * partita ([D-562]).
 *
 * ⛔ **Solo valori, e solo pubblici.** Nessun puntatore, nessun attore. Nessuno stato vero: niente ricarica residua, niente «usabile
 * ora», niente piano, niente azione armata, niente tasto. L'unica ricarica che puo' comparire e' `Deduced`, e si legge come stima.
 * Un campo che nomini una di quelle cose lo ferma `UI.InspectSubjectCarriesNoRawTypes` (insieme chiuso dei nomi dei campi).
 *
 * 🔑 **`Deduced` e' un array di zero o una voce e non un `TOptional`**: un `TOptional` non e' una proprieta' riflessa, e il test per
 * reflection non vedrebbe di che tipo e' (un campo che portasse la ricarica vera passerebbe in silenzio). Un array e' visto.
 */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTInspectSkillDetail
{
	GENERATED_BODY()

	/** L'azione della posizione di kit. `None` = nessun dettaglio (nessun cursore, indice fuori kit, posizione senza azione). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FName ActionId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText Title;

	/** Le righe del tooltip dopo il titolo, nell'ordine del tooltip: frase d'autore, poi «Etichetta  valore». */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FText> Lines;

	/**
	 * I pezzi che RIGUARDANO questa skill: quello che la concede (`EquipmentId == ActionId`: l'azione di un gadget o di un modulo porta
	 * l'id del pezzo, vedi `URTCatalogLibrary::MakeEquipmentAction`) e, per l'attacco base, la variante d'arma. Nessun altro.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FRTInspectEquipmentLine> Equipment;

	/** La stima della ricarica, solo se la squadra ha visto usare l'azione e il catalogo le da' una ricarica. Zero o una voce. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FRTInspectDeducedCooldown> Deduced;

	bool IsSet() const { return !ActionId.IsNone(); }
};

/** Cio' che di un'unita' ispezionata e' pubblico. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTInspectSubject
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FName HeroId;

	/** La carta: la stessa che la targa mostra gia' (PV, scudo). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FRTUnitCardView Card;

	/** Nell'ordine della targa (`BuildStatusBadges`). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FRTStatusBadgeView> Statuses;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	ERTHexDirection Facing = ERTHexDirection::E;

	/** Spento in `ResolutionPlayback`, come per lo sguardo (P12): segue la targa. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	bool bShowFacing = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FRTInspectEquipmentLine> Equipment;

	/**
	 * Le ricariche DEDOTTE di un nemico, nell'ordine del kit (`#3751`). Solo per le azioni che la squadra ha visto
	 * usare e che hanno una ricarica; vuoto per un'unita' propria o alleata, di cui si leggono le ricariche vere.
	 *
	 * ⛔ **Non si disegna da sola**: la sezione «Ricariche» e' sparita dal box (decisione d'autore del 2026-10-31). La riga di UNA
	 * azione compare fra i dettagli della skill sotto il cursore (`HoveredSkill.Deduced`, copiata da qui), e solo li'.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FRTInspectDeducedCooldown> DeducedCooldowns;

	/**
	 * I dettagli della skill sotto il cursore (`#3748`, 2026-10-31). Vuoto (`ActionId` a `None`) senza cursore su uno slot di questa
	 * unita', con un indice fuori kit o con una posizione senza azione: e' allora che `Compose` chiude con la riga di invito.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	FRTInspectSkillDetail HoveredSkill;
};

/** Le righe da disegnare. */
USTRUCT(BlueprintType)
struct REFACTORTACTICS_API FRTInspectView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Inspect")
	TArray<FString> Lines;
};

UCLASS()
class REFACTORTACTICS_API URTInspectLibrary : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * LA TESTATA e, con `HoveredSkill`, i dettagli di quella skill (decisione d'autore del 2026-10-31).
	 *
	 * La testata: nome, PV e scudo, verso (se acceso), stati col nome e l'effetto del glossario, e UNA riga «Equipaggiamento: A · B · C»
	 * coi soli nomi (assente senza pezzi). ⛔ Non il kit (lo mostra la barra delle skill), non le righe `+`/`-` dei pezzi, non la sezione
	 * «Ricariche»: quelle stanno nei dettagli.
	 *
	 * I dettagli, con `HoveredSkill` impostato: il titolo, le righe del tooltip, la riga della stima (se c'e', dichiarata tale) e le righe di
	 * ciascun pezzo che la riguarda (nome e slot, `+` vantaggio, `-` svantaggio). Senza: una riga di invito.
	 * 🔑 L'ordine e' quello del taglio: la finestra della scheda taglia la coda, e la stima viene dall'osservazione mentre i pezzi sono catalogo.
	 * ⛔ Nessuna riga e' una ricarica residua.
	 */
	static FRTInspectView Compose(const FRTInspectSubject& Subject);

	/**
	 * Una riga di ricarica dedotta, o `false` se non c'e' niente da dire (`#3751`).
	 *
	 * Non c'e' riga per un'azione mai vista usare (`LastSeenTurn <= 0`), ne' per una senza ricarica (`CooldownTurns
	 * <= 0`: sarebbe «torna disponibile subito»), ne' senza identita'. La deduzione tace, non indovina.
	 *
	 * 🔑 **`AvailableFromTurn = LastSeenTurn + CooldownTurns`, e non +/-1.** `ARTUnit::ConsumeAbility` scrive
	 * `CooldownTurns` durante la risoluzione del turno T, `TickCooldowns` ne toglie uno nel Cleanup dello stesso T:
	 * a fine turno T restano `CooldownTurns - 1`, e a zero si arriva dopo il Cleanup di T + `CooldownTurns` - 1,
	 * cioe' pianificando il turno T + `CooldownTurns`. Una ricarica di 1 e' «ogni turno» (T + 1). Lo pinna
	 * `UI.InspectDeducedCooldownMatchesTheRealCountdown` contro `ConsumeAbility`/`TickCooldowns` veri.
	 */
	static bool MakeDeducedCooldown(FName ActionId, const FText& Title, int32 CooldownTurns, int32 LastSeenTurn,
		FRTInspectDeducedCooldown& OutRow);

	/**
	 * IL CANCELLO DI CONOSCENZA (#3748, P2): una scheda esiste solo se la vista ha una voce `Live` per l'unita'. Un
	 * nemico non osservato, appena nato, ricordato o ignoto NON ha scheda: stessa uscita di una cella vuota. Non guarda
	 * `ARTUnit::IsKnownToObserver()`, che nasce vero.
	 */
	static bool IsInspectable(const FRTKnowledgeView& View, int32 StableUnitId);

	/**
	 * LA VOCE `Live` di un'unita', o `nullptr` (`#3748`): il cancello di `IsInspectable` che restituisce anche la voce, cosi' chi ha
	 * bisogno della CELLA con cui la squadra la vede (il fuoco, l'hover di uno slot della barra in sola lettura) non rifa' il confronto da se'. Una voce
	 * `Remembered` NON e' una voce `Live`: la sua cella e' quella del ricordo, e puo' essere una cella che oggi si vede.
	 *
	 * 🔑 **E' il solo punto in cui si decide**: le tre copie che c'erano (`GatherFocusInputs`, il ramo nemico di
	 * `GatherSkillHoverInputs`, `IsInspectable`) possono divergere, e una sola di esse coperta da un test lascia le altre a un
	 * rifacimento. `Entry->Cell` di una voce non `Live` non si legge mai.
	 */
	static const FRTKnowledgeEntry* LiveEntry(const FRTKnowledgeView& View, int32 StableUnitId);

	/** Il cancello e il compositore insieme: vuota se l'unita' non e' `Live`. Il soggetto arriva comunque gia' costruito. */
	static FRTInspectView ResolveInspect(const FRTKnowledgeView& View, int32 StableUnitId, const FRTInspectSubject& Subject);

	/**
	 * LA SCHEDA SI VEDE? (`#3748`, spec dell'ispettore §3 regola 11). Una finestra di reazione o un modale la nascondono **senza
	 * chiuderla**: il soggetto resta fissato, e la scheda torna quando si chiudono. Chiuderla e' del tasto destro (passo `Inspector`
	 * del Back) e della conoscenza (`ARTPlayerController::ReleaseInspectionUnlessLive`), mai del contesto.
	 *
	 * ⚠️ Non ha un parametro per la sessione non presidiata: li' `OnSelect` esce gia' su `IsPlanningInputInert`, quindi nessuna
	 * scheda viene fissata e `bPinned` e' falso.
	 */
	static bool ShouldShowCard(ERTPointerContext Context, bool bPinned);

	/**
	 * La prima riga che si puo' mostrare quando le righe sono `TotalLines` e ne entrano `MaxLines`: `FirstLine` riportata nel
	 * campo. A fondo corsa il marcatore in basso non serve e il suo posto lo prende quello in alto, quindi l'ultima finestra parte da
	 * `TotalLines - (MaxLines - 1)`.
	 */
	static int32 ClampFirstLine(int32 TotalLines, int32 MaxLines, int32 FirstLine);

	/**
	 * La finestra di `MaxLines` righe da `FirstLine`, e **un taglio non e' mai muto** (spec §7: la lezione di
	 * `URTContextInspectorLibrary::VisibleLines`, #3320). Se non entrano tutte, la prima riga dice quante ne restano sopra
	 * (`… N righe sopra`, dal secondo giro in poi) e l'ultima quante ne restano sotto (`… e altre K righe`): i marcatori occupano uno
	 * slot ciascuno, e il corpo e' cio' che avanza. Mostrate piu' nascoste sopra e sotto fanno sempre il totale.
	 *
	 * ⛔ Sotto le tre righe non c'e' posto per dichiarare niente: si degrada al taglio secco dalle prime righe, come
	 * `VisibleLines`. Un pannello che usasse gli ultimi slot per parlare del proprio troncamento direbbe meno di uno che mostra
	 * le prime righe e basta. `MaxRighe` del widget ha un minimo che lo evita.
	 */
	static FRTInspectView VisibleWindow(const FRTInspectView& View, int32 FirstLine, int32 MaxLines);
};
