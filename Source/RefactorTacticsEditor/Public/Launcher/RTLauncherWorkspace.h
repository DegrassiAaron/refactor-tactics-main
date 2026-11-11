#pragma once

#include "CoreMinimal.h"

enum class ERTScenarioAuthoringResult : uint8;

/**
 * Perche' `Start Session` non e' partita (#1682, slice `L6`).
 *
 * ⚠️ **Le cause non si fondono in un «non si puo'».** Chi legge deve sapere quale gesto lo sblocca:
 * scegliere una riga, correggere il file, o guardare altrove perche' il difetto non e' nel suo scenario.
 * E' lo stesso motivo per cui `ERTLauncherListState` tiene separate le due cause di un elenco vuoto, e per
 * cui `ERTScenarioAuthoringResult` distingue `Invalid` da `RunFailed` un livello piu' in basso.
 */
enum class ERTLauncherStartRefusal : uint8
{
	/** Nessun rifiuto: la sessione parte. */
	None,

	/** Non e' stato scelto nessuno scenario. Il gesto che sblocca e' selezionare una riga. */
	NoSelection,

	/** L'id non esiste nell'indice, oppure il file che dichiara non si legge. */
	NotFound,

	/** Lo scenario c'e' e non passa `Validate`. Il gesto che sblocca e' correggere il file. */
	Invalid,

	/**
	 * Guasto dello STRUMENTO, non dello scenario.
	 *
	 * ⚠️ Esiste per lo stesso motivo di `ERTScenarioAuthoringResult::RunFailed`: dire `Invalid` a chi ha
	 * scritto uno scenario corretto lo manda a cercare un difetto che non ha. Ed e' anche il ramo che
	 * raccoglie un valore dell'enum della facade che questa mappatura non conosce — un `default` che
	 * concedesse l'avvio sarebbe il `silent fallback` che il guardrail della issue vieta.
	 */
	ToolFailure,
};

/**
 * L'esito della decisione, con la frase che la spiega.
 *
 * ⚠️ `Reason` non e' decorativa ed e' parte del contratto: l'acceptance criterion dice *«non puo' avviare
 * una sessione, e il motivo e' visibile»*. Un rifiuto senza frase soddisfa la meta' sbagliata.
 */
struct FRTLauncherStartDecision
{
	bool bAllowed = false;
	ERTLauncherStartRefusal Refusal = ERTLauncherStartRefusal::None;
	FString Reason;
};

/** Come si raggiunge una superficie: sono due meccanismi d'editor diversi, e chi attiva deve saperlo. */
enum class ERTLauncherActivationKind : uint8
{
	/** Un `FEditorModeID` da attivare sul mode manager. */
	EditorMode,

	/** Un tab id da invocare sul `FGlobalTabmanager`. */
	Tab,
};

/**
 * Una superficie del workspace: dove il designer atterra dopo `Start Session`.
 *
 * ⚠️ **Una superficie e' dichiarata solo se esiste.** Le due che ancora non esistono restano nel registro
 * come `Pending` con il numero della issue che le porta, e questo NON e' un dettaglio di presentazione: e'
 * cio' che rende il criterio di #1682 chiudibile oggi senza mentire su cio' che c'e'. Un elenco fisso di
 * cinque voci avrebbe imposto o di aspettare due issue altrui, o di dichiarare raggiungibile qualcosa che
 * non si puo' raggiungere.
 */
struct FRTLauncherSurface
{
	/** Chiave stabile, unica in tutto il registro. E' l'identita' della superficie, non la sua etichetta. */
	FName Key;

	/** Vero se la superficie esiste ed e' raggiungibile adesso. */
	bool bDeclared = false;

	/** Come attivarla. Significativo solo se `bDeclared`. */
	ERTLauncherActivationKind ActivationKind = ERTLauncherActivationKind::Tab;

	/**
	 * Che cosa attivare: un `FEditorModeID` oppure un tab id, secondo `ActivationKind`.
	 *
	 * ⚠️ **Due superfici possono condividere lo stesso bersaglio, e non e' un difetto.** Scenario e
	 * Validation vivono oggi nello stesso pannello: il registro dichiara *dove si atterra*, non che ogni
	 * superficie abbia una finestra propria. Pretendere l'unicita' qui costringerebbe a inventare un tab
	 * per far tornare un invariante.
	 */
	FName ActivationTarget;

	/** La issue che porta questa superficie. Significativo solo se NON `bDeclared`. */
	int32 PendingIssue = 0;
};

/**
 * La parte di #1682 che si puo' misurare senza un editor vivo.
 *
 * Stessa scelta di `URTDevSandboxLauncherSubsystem::ShouldOpenFor` (#1680) e di
 * `FRTLauncherScenarioBrowser` (#1705), e per la stessa ragione: di una slice fatta di Slate, cio' che un
 * automation test vede e' solo la decisione. Qui stanno il rifiuto dell'avvio e il registro delle
 * superfici — cioe' i due punti in cui questa slice puo' sbagliare in silenzio.
 *
 * ⛔ **Niente qui apre, valida o esegue.** La sessione d'authoring **e'** `URTScenarioAuthoring`
 * (ADR-0010): questa classe riceve il suo esito e lo classifica. Non c'e' un terzo oggetto «sessione», e
 * introdurlo sarebbe il terzo significato che il referto del 2026-08-29 §8 avverte di non creare.
 */
class FRTLauncherWorkspace
{
public:
	/**
	 * L'avvio e' concesso? Funzione PURA: prende l'esito che la facade ha gia' prodotto e lo classifica.
	 *
	 * ⚠️ **Prende `OpenResult` invece di aprire da sola**, e la differenza e' l'intera testabilita' della
	 * slice: aprire richiede il corpus su disco, classificare no. E' anche cio' che impedisce a questa
	 * classe di diventare una seconda porta verso lo scenario.
	 *
	 * @param SelectedId  l'id scelto nell'elenco; vuoto = nessuna selezione.
	 * @param OpenResult  cio' che `URTScenarioAuthoring::OpenById` (o `Validate`) ha restituito.
	 * @param OpenError   la frase della facade, quando c'e'. Se vuota, la decisione ne fornisce una propria:
	 *                    un rifiuto muto e' un rifiuto che non soddisfa l'AC.
	 */
	static FRTLauncherStartDecision DecideStart(const FString& SelectedId, ERTScenarioAuthoringResult OpenResult, const FString& OpenError);

	/**
	 * Il registro completo: dichiarate e pendenti, nell'ordine in cui il workspace le presenta.
	 *
	 * ⚠️ **E' la sorgente unica.** La UI ci itera sopra invece di elencare pulsanti a mano, ed e' cio' che
	 * rende vero l'AC *«aggiungere una superficie non richiede di modificare nessun criterio»*: una voce
	 * nuova compare nel pannello e nei test senza che si tocchi ne' l'uno ne' gli altri.
	 */
	static const TArray<FRTLauncherSurface>& Surfaces();

	/** La superficie con questa chiave, oppure `nullptr`. */
	static const FRTLauncherSurface* Find(FName Key);

	/** Frase leggibile per una superficie non ancora costruita: dice **quale issue** la porta. */
	static FString PendingLabel(const FRTLauncherSurface& Surface);
};
