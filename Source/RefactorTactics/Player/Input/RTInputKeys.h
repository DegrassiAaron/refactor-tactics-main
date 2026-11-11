#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

/**
 * **LE TABELLE DEI TASTI DEL GIOCATORE**, e le etichette che l'HUD ne scrive (#4065, S1).
 *
 * Stavano nel controller come funzioni statiche. Non toccano il mondo, e le leggono in quattro: la mappatura
 * (`ARTPlayerController::BuildInputMappings`), i binding (`ARTPlayerController::BindInputActions`), l'HUD e i test.
 * Una sede sola fa si' che il tasto mostrato e il tasto che risponde non possano dire due cose diverse: e' la
 * ragione di quasi ogni commento qui sotto, scritta quando la sede era il controller.
 *
 * ⚠️ **Cosa NON c'e'.** Le `UInputAction` e il mapping context restano sul controller: sono `UObject` con `Outer` il
 * controller, e la loro vita e' la sua.
 */
namespace RTInputKeys
{
	/**
	 * I tasti che armano una POSIZIONE del kit, in ordine: l'indice nell'elenco **e'** l'indice nel kit
	 * dell'unita' selezionata. Dieci, `1`..`9` piu' `0`.
	 *
	 * ⚠️ **E' la sorgente unica del conteggio**, ed e' il motivo per cui esiste come funzione invece che
	 * come sequenza di `MapKey` scritti a mano: la mappatura la percorre, la bindatura la percorre, e un
	 * test puo' chiedere *«quante posizioni raggiunge l'input?»* senza confrontare un letterale che
	 * invecchia al primo tasto aggiunto. Interrogata da `PlayerInput.EveryKitEntryIsReachable`.
	 *
	 * Il tasto sceglie una **posizione**, non un'azione: quale abilita' occupi l'indice N dipende dall'eroe.
	 */
	REFACTORTACTICS_API const TArray<FKey>& AbilityHotkeys();

	/**
	 * L'ETICHETTA del tasto che arma questa voce di kit, o testo **vuoto** se nessun tasto la raggiunge
	 * (`#2987`, estesa da [D-397] §4).
	 *
	 * 🔴 **Esiste perche' quell'etichetta veniva CALCOLATA invece che letta, e per l'ultima posizione era
	 * falsa.** `ARTHUD::ComposeAbilityLine` scriveva `AbilityIndex + 1`, quindi per la posizione `9` — che
	 * `AbilityHotkeys()` chiude con `EKeys::Zero` — la riga diceva **«10.»** mentre il tasto e' **`0`**.
	 * L'aritmetica e la tabella erano due verita', e divergevano proprio dove il giocatore non ha un
	 * secondo modo di scoprirlo.
	 *
	 * ⚠️ **Il vuoto e' un risultato, non un errore.** Il commento di `GenericHotkeys()` dichiara il caso:
	 * *«un eroe con sei azioni porta il kit a undici voci contro i dieci tasti numerici»*. L'undicesima
	 * posizione non ha un tasto, e mostrare `11.` la annuncerebbe come premibile — la forma scritta del
	 * *«verde che mente»* che quel commento gia' nomina.
	 *
	 * 🔑 **L'ordine di risoluzione e' `GenericHotkeys()` per `ActionId` -> `AbilityHotkeys()` per posizione
	 * -> vuoto, e lo fissa [D-397] §4.** `Action.Wait` si arma con `Z` **e** col numero della propria
	 * posizione; si mostra la lettera perche' e' legata all'`ActionId`, mentre il numero dipende da quante
	 * azioni porta l'eroe — *«una barra che rinumerasse le generiche insegnerebbe una mappa che scade»*.
	 *
	 * ⛔ **Una etichetta sola per slot**: il numero resta funzionante senza essere mostrato.
	 *
	 * ⚠️ Restituisce l'etichetta della `FKey`, non una stringa composta qui: la traduzione da tasto a nome
	 * visibile appartiene all'engine, e ricomporla sarebbe la seconda verita' daccapo.
	 */
	REFACTORTACTICS_API FText HotkeyLabelFor(const FName& ActionId, int32 KitIndex);

	/**
	 * Il tasto che sceglie l'opzione in posizione `OptionIndex` di una finestra di reazione ([D-491], #3615): il
	 * tasto del kit nella stessa posizione, `AbilityHotkeys()[OptionIndex]`. Vuoto oltre l'ultimo tasto.
	 *
	 * 🔑 Viene dalla lista che mappa i tasti, come `HotkeyLabelFor`: il badge e il tasto che risponde non possono
	 * dire due cose diverse.
	 */
	REFACTORTACTICS_API FText ReactionOptionKeyLabel(int32 OptionIndex);

	/**
	 * Le azioni GENERICHE e il tasto che le arma, in coppia. Sono l'altro canale di selezione del kit, e
	 * risolvono per **nome** invece che per posizione.
	 *
	 * 🔴 **Il nome non e' un vezzo, e' l'unica forma corretta.** Le generiche sono ACCODATE al kit
	 * (`ARTUnit::EnsureDefaultAbilities`), quindi il loro indice dipende da quante azioni porta l'eroe: con
	 * cinque stanno a `5..9`, con sei a `6..10`. Un tasto legato a una posizione fissa punterebbe a cose
	 * diverse su eroi diversi — e su quello che ne ha una in piu' punterebbe a un'abilita' d'eroe.
	 *
	 * ⚠️ **Perche' esistono**: dieci posizioni non bastano piu'. Un eroe con sei azioni porta il kit a
	 * **undici** voci contro i dieci tasti numerici, e `PlayerInput.EveryKitEntryIsReachable` lo dichiara.
	 * Togliendo le cinque generiche dalla fila dei numeri, quella fila torna a servire i soli eroi.
	 */
	REFACTORTACTICS_API const TArray<TPair<FName, FKey>>& GenericHotkeys();

	/**
	 * Il tasto che DICHIARA `Sneak` ([D-425]): l'unico profilo di movimento che si dichiara invece di derivarsi.
	 *
	 * 🔑 **Una sede sola, letta da due parti** (`#3470`): `BuildInputMappings` ci lega `IA_DeclareSneak`, e la
	 * lettura del movimento nella barra lo mostra come badge. Scritto due volte, il giorno in cui qualcuno
	 * rimappa il gesto la barra continuerebbe a dire `M` per un tasto che non fa piu' niente.
	 * `PlayerInput.SneakIsMappedOnTheKeyTheBarShows` lo pinna sul contesto di input reale.
	 */
	REFACTORTACTICS_API const FKey& SneakHotkey();

	/**
	 * Il tasto che DICHIARA il piano dell'unita' selezionata (`Invio`, #3145) — lo stesso gesto che il pulsante
	 * `Conferma` della HUD inoltra ([D-458]). Una sede sola, letta dalla mappatura e dall'etichetta del pulsante.
	 */
	REFACTORTACTICS_API const FKey& DeclarePlanHotkey();

	/**
	 * Il tasto da tastiera del Back (`BackSpace`), gemello del tasto destro — lo stesso gesto che il pulsante
	 * `Annulla` della HUD inoltra ([D-458]). ⚠️ Il destro resta mappato a parte: e' anche il dolly della camera
	 * con `Ctrl`, e un pulsante non ha un modificatore da tenere.
	 */
	REFACTORTACTICS_API const FKey& UndoKeyboardHotkey();

	/**
	 * Il tasto della vista strategica (`Tab`, [D-488]) e quello del ciclo della selezione (`N`, [D-421]).
	 *
	 * 🔑 **Sono una coppia, e una sede sola li tiene insieme**: D-488 sposta il ciclo da `Tab` a `N` nello
	 * stesso commit che lega `Tab`, perche' separati, per un commit, `Tab` farebbe due cose oppure nessuna.
	 * `PlayerInput.StrategicViewIsOnTabAndSelectionCycleOnN` li pinna sul contesto di input reale.
	 */
	REFACTORTACTICS_API const FKey& StrategicViewHotkey();
	REFACTORTACTICS_API const FKey& CycleSelectionHotkey();

	/**
	 * Il nome che un BADGE scrive per un tasto ([D-496], #3633): in italiano per i tasti che l'HUD mostra in un
	 * badge (`Enter` -> «INVIO», `BackSpace` -> «BACKSPACE»), altrimenti il nome dell'engine in maiuscolo.
	 *
	 * 🔑 **Riceve il tasto, non un nome**: il badge legge sempre il tasto vero della mappatura, e questa tabella
	 * ne traduce soltanto l'etichetta. Un tasto rimappato cambia il badge senza toccare la tabella.
	 */
	REFACTORTACTICS_API FText KeyBadgeLabel(const FKey& Key);
}
