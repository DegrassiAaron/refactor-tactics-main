#include "Frontend/RTFrontendScreenIds.h"

// Le definizioni stanno in un `.cpp` e non nell'header: un `const FName` a scope di namespace dentro un
// header darebbe una copia per unita' di traduzione, e il confronto fra due `FName` uguali continuerebbe a
// funzionare — sono indici nella tabella globale — ma il costruttore girerebbe una volta per file incluso.
// Una definizione sola costa una riga in piu' e toglie la domanda.
namespace RTScreenIds
{
	const FName Main(TEXT("Main"));
	const FName Settings(TEXT("Settings"));
}
