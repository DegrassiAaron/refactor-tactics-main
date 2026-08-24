#pragma once

#include "CoreMinimal.h"

/**
 * I nomi **canonici** delle schermate del frontend (CP 46.3, `#938`).
 *
 * ## Perche' esistono
 *
 * Fino a `18b3f105` l'unico posto in cui `"Main"` compariva era un blocco anonimo dentro
 * `RTFrontendNavigationTests.cpp`. Andava bene finche' l'unico chiamante era un test: adesso lo stesso
 * nome deve comparire nel `.ini` che dichiara i binding, nel codice che apre la radice e nei test che lo
 * verificano, e tre stringhe scritte a mano in tre punti sono tre occasioni di scriverne una diversa.
 *
 * 🔴 **Quel blocco e' stato ritirato insieme a questo header, e la prima stesura non l'aveva fatto.**
 * Trovato in code review: restava una copia dei letterali proprio nel file che questa motivazione cita,
 * quindi un rename del nome canonico avrebbe continuato a compilare li' asserendo contro il vecchio. Una
 * giustificazione che descrive una duplicazione ancora in piedi e' peggio di nessuna giustificazione.
 *
 * ⚠️ **Un id sbagliato non e' un errore rumoroso**: `PushScreen(TEXT("Mian"))` restituisce `Ok` — lo stack
 * accetta qualunque `FName` non vuoto — e semplicemente non disegna niente, perche' nessun binding porta
 * quel nome. E' il fallimento indistinguibile dal successo che `ERTNavResult` esiste per impedire, e qui
 * lo si impedisce a monte: un refuso su `RTScreenIds::Main` **non compila**.
 *
 * ⚠️ **Il `.ini` resta fuori da questa garanzia**, e va detto invece che sottinteso: la chiave
 * `ScreenId="Main"` in `DefaultGame.ini` e' testo, e nessun compilatore la confronta con queste costanti.
 * E' la ragione per cui `RTFrontendMainMenuTests.cpp` verifica che `StartFrontend` trovi davvero
 * `RTScreenIds::Main` fra le registrate: e' li' che i due mondi si toccano, ed e' li' che si prova.
 *
 * ## Cosa NON sta qui
 *
 * I modali. `ShowModal` prende un `FName` come le schermate, ma un modale non e' una voce di navigazione
 * dichiarata a monte: nasce da una condizione (un errore d'avvio, una conferma) e il suo nome appartiene a
 * chi lo arma. Quando E46 avra' un modale ricorrente, avra' anche un posto dove metterlo — e sara' questo.
 */
namespace RTScreenIds
{
	/** La radice del frontend: il Main Menu. `ReturnMain` torna qui, e da qui non si va indietro. */
	REFACTORTACTICS_API extern const FName Main;

	/**
	 * Il pannello impostazioni.
	 *
	 * ⚠️ In v0.1 e' **dichiarato *coming soon*** e non ha contenuto, ma l'id esiste lo stesso: il DoD di
	 * #938 vuole che il back stack lo attraversi, e che il menu non cambi forma in v0.2. Una voce che in
	 * v0.1 non fosse una schermata vera diventerebbe una schermata vera dopo, e sarebbe una seconda forma
	 * del menu invece della stessa con dentro qualcosa.
	 */
	REFACTORTACTICS_API extern const FName Settings;

	/**
	 * Il modale d'errore d'avvio.
	 *
	 * ✅ **E' il modale ricorrente che la nota qui sopra aspettava.** Diceva: «un modale nasce da una
	 * condizione e il suo nome appartiene a chi lo arma — quando E46 avra' un modale ricorrente, avra' anche
	 * un posto dove metterlo, e sara' questo». `StartMatch` (CP 46.4) lo arma da C++ su un avvio rifiutato,
	 * quindi il nome non appartiene piu' a un solo chiamante: era una costante locale nei test, e due
	 * stringhe uguali scritte in due posti sono un refuso in attesa.
	 */
	REFACTORTACTICS_API extern const FName ErrorModal;

	/**
	 * La schermata di fine partita (CP 46.5, `#940`).
	 *
	 * ⚠️ **È una schermata, non un modale**, ed è la ragione per cui sta accanto a `Main` invece che
	 * accanto a `ErrorModal`. Un modale sospende il `Back` e lascia sotto di sé lo stato da cui è nato; qui
	 * la partita è finita e sotto non c'è niente a cui tornare. Il DoD lo dice dal lato dell'utente:
	 * *«dal menu, `Back` non deve poter rientrare in una partita conclusa»* — ed è `ReturnMain`, non
	 * `CloseModal`, a garantirlo.
	 */
	REFACTORTACTICS_API extern const FName Result;

	/**
	 * Il menu di pausa (CP 46.6, `#941`).
	 *
	 * ⚠️ **E' una schermata, non un modale, e la scelta era gia' scritta.** `RTScreenStack.h` la dichiarava
	 * prima che questo id esistesse: *«`Settings` aperto dal Main e `Settings` aperto dalla **Pause** sono
	 * la stessa schermata con due ritorni diversi»*. Un modale avrebbe reso il DoD irrealizzabile — con un
	 * modale aperto `PushScreen` risponde `BlockedByModal`, quindi il `SETTINGS` della pausa non potrebbe
	 * aprire *lo stesso pannello di CP 46.3*: dovrebbe essere un secondo widget, che e' cio' che il DoD
	 * vieta con «non una seconda copia».
	 *
	 * ⛔ **Non sospende niente.** Questa schermata copre la partita e le toglie il puntatore; il turno resta
	 * dov'era perche' non avanza da solo. E' il vincolo offline-only del DoD reso architettura invece che
	 * promessa: cio' che in v0.5 non potra' esistere e' *fermare il tempo di tutti*, e qui non si ferma
	 * niente — vedi `URTFrontendNavigator::ShowPause`.
	 */
	REFACTORTACTICS_API extern const FName Pause;
}
