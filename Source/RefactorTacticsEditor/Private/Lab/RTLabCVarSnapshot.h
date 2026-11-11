// La fotografia di una console variable per il lanciatore PIE del Lab (#3541).
//
// 🔑 **Come scrive: `Set` esplicito alla priorita' `P = max(SetByPrima, ECVF_SetByCode)`**, poi rilegge.
// `Apply` e `Restore` usano la stessa `P`, derivata una volta da `SetByPrima` catturato in `Capture`.
//
// Perche' non le due scorciatoie:
//  - `Set(..., ECVF_SetByConsole)` scavalca il valore digitato in console, ma alza il «pavimento»: dopo il
//    ripristino la variabile resterebbe a `Console`, e ogni `Set` successivo a priorita' inferiore
//    (`SetByCode`, un `.ini`) verrebbe ignorato con un warning.
//  - `SetWithCurrentPriority` conserva la priorita', ma su una CVar **mai impostata** (la storia contiene
//    solo `Constructor`: lo stato normale di `rt.Test.Scenario` e `rt.Debug.PlaybackControls` in un Editor
//    appena aperto) risolve a `SETBY_ERROR`: il valore PRENDE (`CanChange(0x01000000 >= 0)`), ma il `SetBy`
//    diventa il valore riservato `0x01000000`, con il log «Trying to Replace Cvar ... Set By
//    Constructor Value implicitly» (`ConsoleManager.cpp:986-1025`, `FindHighestPriorityAndTag` :783-810).
//
// In prosa, per ogni `SetBy` catturato (l'ordine e' quello di `IConsoleManager.h:155-187`: `Constructor` <
// ... < `Commandline` (0x0D) < `Code` (0x0E) < `Temp` < `Console` (0x10), la piu' alta):
//  - **Sotto `Code`** (`Constructor`, cioe' mai impostata, ma anche `Scalability`, ini, `Commandline`,
//    `Hotfix`...): `P = Code`. `Apply` e `Restore` prendono. ⚠️ **Limite dichiarato**: dopo `Restore` il
//    `SetBy` e' `Code`, non quello di prima — un `Set` a una priorita' piu' bassa di quella corrente e'
//    rifiutato, e non si puo' scendere. Il valore e' quello di prima; il pavimento e' quello di qualunque
//    `Set` da codice.
//  - `Code`, `Temp`, `Console`: `P` e' la stessa priorita' di prima. `Apply` e `Restore` prendono e il
//    `SetBy` dopo `Restore` e' quello di prima — con la conseguenza che era gia' vera prima del lancio: dopo
//    un `Restore` a `Console` un `Set` a `Code` resta ignorato, perche' l'utente l'aveva digitata.
//  - **Alzata DURANTE il PIE** (una riga digitata in console con `P = Code`): `Restore` non puo' scrivere —
//    `CanChange` rifiuta —, ritorna `false` con il motivo, e la variabile resta com'e' stata alzata.
//
// Dopo ogni scrittura si **rilegge**: una variabile `ReadOnly`, un valore non parsabile o una priorita'
// superiore restano possibili, e una scrittura che non ha preso non deve passare per riuscita. L'errore dice
// quale delle tre.
//
// ⚠️ Diverge dall'idioma di `FRTConsoleVariableGuardForTest` (#2235), che usa `SetWithCurrentPriority`:
// quel guard vive nei test, che impostano SEMPRE la variabile prima di fotografarla, quindi non incontrano
// mai `Constructor`. Qui la variabile e' quella reale di un Editor appena aperto. Non si include il guard da
// questo modulo (e' un header `ForTest`: produzione che lo include e' un odore); un helper condiviso e' un
// FOLLOW-UP CANDIDATE.
//
// ⚠️ Limite dichiarato: se `Apply` non prende, la variabile su cui ha fallito NON viene ripristinata da
// questa struct — di norma e' rimasta intatta. Il chiamante (il lanciatore) rimette l'ALTRA variabile,
// quella gia' applicata, e rifiuta con il motivo.
//
// ⛔ Nessuna dipendenza da `GEditor`: la struct si prova in un automation test su una CVar di prova.

#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

/** Fotografia di una console variable: valore e priorita' (SetBy) al momento della cattura. */
struct FRTLabCVarSnapshot
{
	/** Cattura valore e `SetBy` di `Nome`. `false` (con il motivo) se la variabile non esiste. */
	static bool Capture(const TCHAR* Nome, FRTLabCVarSnapshot& Out, FString& OutError);

	/** Scrive `Valore` a `max(SetByPrima, Code)` e rilegge. `false` (con il motivo) se non ha preso. */
	bool Apply(const FString& Valore, FString& OutError) const;

	/** Riscrive `ValorePrima` a `max(SetByPrima, Code)` e rilegge. `false` (con il motivo) se non ha preso. */
	bool Restore(FString& OutError) const;

	FString Nome;
	FString ValorePrima;
	EConsoleVariableFlags SetByPrima = ECVF_SetByCode;
};
