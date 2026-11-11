#include "Player/Input/RTInputKeys.h"

namespace RTInputKeys
{
	const TArray<FKey>& AbilityHotkeys()
	{
		// `1`..`9` piu' `0`: dieci posizioni, che e' quanto basta al kit di oggi — cinque voci d'eroe piu' le
		// cinque generiche di D-025. Non e' un numero scelto per stare largo: e' la dimensione misurata del kit,
		// e il test che la difende confronta le due cose invece di fidarsi di questo commento.
		//
		// ⚠️ Nessuno di questi tasti collide: mappati altrove sono A D E F Q R S T V W, Home, Escape, BackSpace,
		// Spazio e i pulsanti del mouse. Verificato sull'elenco completo dei `MapKey` di `BuildInputMappings`.
		static const TArray<FKey> Hotkeys = {
			EKeys::One,  EKeys::Two,   EKeys::Three, EKeys::Four, EKeys::Five,
			EKeys::Six,  EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero };
		return Hotkeys;
	}

	const FKey& SneakHotkey()
	{
		// `M` per «muoversi piano»: non collide con nessun altro `MapKey` di `BuildInputMappings`, e il controllo
		// che lo prova e' `PlayerInput.HotkeysDoNotCollide`, che legge il contesto vero e non questa riga.
		static const FKey Tasto = EKeys::M;
		return Tasto;
	}

	const FKey& DeclarePlanHotkey()
	{
		static const FKey Tasto = EKeys::Enter;
		return Tasto;
	}

	const FKey& UndoKeyboardHotkey()
	{
		static const FKey Tasto = EKeys::BackSpace;
		return Tasto;
	}

	const FKey& StrategicViewHotkey()
	{
		static const FKey Tasto = EKeys::Tab;
		return Tasto;
	}

	const FKey& CycleSelectionHotkey()
	{
		// `N` per «next». Libero in `BuildInputMappings` quando D-488 l'ha scelto (misurato su `ae805683a`), e
		// a tenerlo libero non e' questa riga ma `PlayerInput.HotkeysDoNotCollide`, che legge il contesto vero.
		static const FKey Tasto = EKeys::N;
		return Tasto;
	}

	FText KeyBadgeLabel(const FKey& Key)
	{
		// [D-496]: i tasti che un badge dell'HUD mostra per nome. Le cifre e le lettere restano quelle dell'engine.
		struct FVoce { FKey Tasto; const TCHAR* Nome; };
		static const FVoce Voci[] = {
			{ EKeys::Enter,     TEXT("INVIO") },
			{ EKeys::BackSpace, TEXT("BACKSPACE") },
			{ EKeys::Escape,    TEXT("ESC") },
			{ EKeys::SpaceBar,  TEXT("SPAZIO") },
			{ EKeys::Tab,       TEXT("TAB") },
		};
		for (const FVoce& Voce : Voci)
		{
			if (Voce.Tasto == Key)
			{
				return FText::FromString(Voce.Nome);
			}
		}
		return FText::FromString(Key.GetDisplayName(/*bLongDisplayName=*/ false).ToString().ToUpper());
	}

	FText ReactionOptionKeyLabel(int32 OptionIndex)
	{
		return AbilityHotkeys().IsValidIndex(OptionIndex)
			? AbilityHotkeys()[OptionIndex].GetDisplayName(/*bLongDisplayName=*/ false)
			: FText::GetEmpty();
	}

	FText HotkeyLabelFor(const FName& ActionId, int32 KitIndex)
	{
		// 🔑 **Le GENERICHE per prime, e per NOME** ([D-397] §4). Entrambi i tasti armano — `OnAbility6` passa da
		// `SelectAbilityForCurrent(5)`, `OnGeneric1` da `SelectAbilityByIdForCurrent` — quindi la domanda non e'
		// quale funzioni, ma quale si MOSTRA. Il numero e' legato alla posizione, la lettera all'`ActionId`, e la
		// posizione cambia col kit dell'eroe: *«una barra che rinumerasse le generiche insegnerebbe una mappa che
		// scade»*.
		//
		// ⛔ **Una etichetta sola per slot**: il numero resta funzionante senza essere mostrato. E' l'estensione
		// alla presentazione della disciplina che `PlayerInput.GenericHotkeyResolvesByNameNotPosition` pinna gia'
		// sull'input.
		if (!ActionId.IsNone())
		{
			for (const TPair<FName, FKey>& Generica : GenericHotkeys())
			{
				if (Generica.Key == ActionId)
				{
					return Generica.Value.GetDisplayName(/*bLongDisplayName=*/ false);
				}
			}
		}


		// 🔑 **Si INTERROGA la tabella, e il vuoto fuori range e' la meta' che conta.** Un `Index + 1` risponde
		// a qualunque indice — anche a quelli che nessun tasto raggiunge — e quella e' esattamente la risposta
		// che non esiste. Qui `IsValidIndex` e' il confine, ed e' lo stesso della bindatura.
		if (!AbilityHotkeys().IsValidIndex(KitIndex))
		{
			return FText::GetEmpty();
		}

		// `bLongDisplayName = false`: si vuole cio' che sta su uno slot largo un'icona — `0`, non `Zero`.
		return AbilityHotkeys()[KitIndex].GetDisplayName(/*bLongDisplayName=*/ false);
	}

	const TArray<TPair<FName, FKey>>& GenericHotkeys()
	{
		// Le cinque generiche di D-025 che entrano nel kit, ognuna col suo tasto STABILE: non cambiano da eroe
		// a eroe, quindi non hanno ragione di occupare una posizione della fila dei numeri.
		//
		// 🔴 **Questa tabella e' la ragione per cui un eroe puo' portare una sesta azione.** Prima le generiche
		// stavano nei numeri, il kit faceva undici voci contro dieci tasti, e la sesta abilita' finiva in una
		// posizione che il giocatore non poteva premere: raggiungibile per il gate del catalogo — che conta i
		// kit, non i tasti — e impremibile in partita. Un verde che mente.
		//
		// ⚠️ **I tasti sono scelti per la MANO, non per l'iniziale.** `O` sarebbe mnemonico per Overwatch e `I`
		// per Interact, ma stanno dall'altra parte della tastiera: queste si premono mentre la sinistra guida
		// la camera su `WASD`. `G` e `B` cadono bene e sono anche mnemonici; `C`, `X` e `Z` sono vicini e liberi.
		//
		// ⚠️ **Nessuno di questi collide**: mappati altrove sono `A D E F Q R S T V W`, `Home`, `Escape`,
		// `BackSpace`, `Spazio`, i pulsanti del mouse e le dieci cifre. Verificato sull'elenco completo dei
		// `MapKey` di `BuildInputMappings`, ed e' un controllo che `PlayerInput.HotkeysDoNotCollide` rifa'.
		static const TArray<TPair<FName, FKey>> Hotkeys = {
			{ TEXT("Action.Guard"),     EKeys::G },
			{ TEXT("Action.Brace"),     EKeys::B },
			{ TEXT("Action.Overwatch"), EKeys::C },
			{ TEXT("Action.Interact"),  EKeys::X },
			{ TEXT("Action.Wait"),      EKeys::Z } };
		return Hotkeys;
	}
}
