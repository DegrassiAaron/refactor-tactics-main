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
}
