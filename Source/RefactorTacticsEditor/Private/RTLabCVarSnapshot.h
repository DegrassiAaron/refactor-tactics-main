// La fotografia di una console variable per il lanciatore PIE del Lab (#3541).
//
// 🔑 **Perche' `SetWithCurrentPriority` e non `Set(..., ECVF_SetByConsole)`** (stesso idioma di
// `FRTConsoleVariableGuardForTest`, #2235): `SetWithCurrentPriority` scavalca il valore digitato in console
// **e** non cambia la priorita' (`SetBy`) della variabile. `Set(..., ECVF_SetByConsole)` fa la prima cosa
// ma alza il «pavimento» a `Console`: dopo il ripristino la variabile resterebbe a quella priorita', e ogni
// `Set` successivo a priorita' inferiore (`SetByCode`, un `.ini`) verrebbe ignorato con un warning. Con
// `SetWithCurrentPriority` il `SetBy` dopo `Restore` e' quello di prima per costruzione.
//
// Dopo ogni scrittura si **rilegge**: una variabile `ReadOnly` o un valore non parsabile restano possibili, e
// una scrittura che non ha preso non deve passare per riuscita.
//
// ⚠️ Limite dichiarato: se `Apply` non prende, la variabile su cui ha fallito NON viene ripristinata da
// questa struct — di norma e' rimasta intatta, perche' l'unica causa reale e' `ReadOnly`. Il chiamante
// (il lanciatore) rimette l'ALTRA variabile, quella gia' applicata, e rifiuta con il motivo.
//
// ⚠️ Limite dichiarato: `SetByPrima` lo legge **solo il test**; `Restore` non lo riapplica. Il «per
// costruzione» di sopra vale se nessuno alza la priorita' DURANTE il PIE: una riga digitata in console in
// quel tempo lascia la variabile a `Console` anche dopo `Restore`.
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

	/** Scrive `Valore` con la priorita' corrente e rilegge. `false` (con il motivo) se non ha preso. */
	bool Apply(const FString& Valore, FString& OutError) const;

	/** Riscrive `ValorePrima` con la priorita' corrente e rilegge. `false` (con il motivo) se non ha preso. */
	bool Restore(FString& OutError) const;

	FString Nome;
	FString ValorePrima;
	EConsoleVariableFlags SetByPrima = ECVF_SetByCode;
};
