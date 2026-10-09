#include "RTLabCVarSnapshot.h"

namespace
{
	/**
	 * Scrive con `Set` esplicito alla priorita' `P = max(SetByPrima, ECVF_SetByCode)` e rilegge. `false`, col
	 * motivo, se il valore letto non e' quello chiesto. Perche' una `P` esplicita e non la priorita'
	 * corrente: vedi l'header (su una CVar mai impostata quest'ultima risolve a `SETBY_ERROR` e viene rifiutata).
	 */
	bool ScriviERileggi(const FRTLabCVarSnapshot& Foto, const FString& Valore, FString& OutError)
	{
		IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(*Foto.Nome);
		if (!Var)
		{
			OutError = FString::Printf(TEXT("la console variable '%s' non esiste in questo binario"), *Foto.Nome);
			return false;
		}

		// I valori `SetBy*` sono ordinati per priorita' crescente, quindi `FMath::Max` sui valori numerici
		// sceglie la piu' alta. `Constructor` (CVar mai impostata) sta SOTTO `Code`: un `Set` a quella
		// priorita' e' rifiutato, e si scrive a `Code`.
		const uint32 P = FMath::Max(static_cast<uint32>(Foto.SetByPrima), static_cast<uint32>(ECVF_SetByCode));
		Var->Set(*Valore, static_cast<EConsoleVariableFlags>(P));

		const FString Letto = Var->GetString();
		if (Letto == Valore)
		{
			return true;
		}

		// La scrittura non ha preso: dire PERCHE', perche' tre cause diverse hanno tre rimedi diversi.
		const uint32 SetByLetto = static_cast<uint32>(Var->GetFlags() & ECVF_SetByMask);
		if (Var->TestFlags(ECVF_ReadOnly))
		{
			OutError = FString::Printf(TEXT("la console variable '%s' e' ReadOnly: \"%s\" non e' stato scritto (vale \"%s\")"),
				*Foto.Nome, *Valore, *Letto);
		}
		else if (SetByLetto > P)
		{
			OutError = FString::Printf(
				TEXT("la scrittura di '%s' a priorita' 0x%08x non ha preso: la variabile e' tenuta da una priorita' superiore (SetBy letto: 0x%08x, vale \"%s\")"),
				*Foto.Nome, P, SetByLetto, *Letto);
		}
		else
		{
			OutError = FString::Printf(TEXT("la console variable '%s' non ha accettato \"%s\": valore non parsabile (riletto: \"%s\")"),
				*Foto.Nome, *Valore, *Letto);
		}
		return false;
	}
}

bool FRTLabCVarSnapshot::Capture(const TCHAR* Nome, FRTLabCVarSnapshot& Out, FString& OutError)
{
	IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Nome);
	if (!Var)
	{
		OutError = FString::Printf(TEXT("la console variable '%s' non esiste in questo binario"), Nome);
		return false;
	}

	Out.Nome = Nome;
	Out.ValorePrima = Var->GetString();
	Out.SetByPrima = static_cast<EConsoleVariableFlags>(Var->GetFlags() & ECVF_SetByMask);
	return true;
}

bool FRTLabCVarSnapshot::Apply(const FString& Valore, FString& OutError) const
{
	return ScriviERileggi(*this, Valore, OutError);
}

bool FRTLabCVarSnapshot::Restore(FString& OutError) const
{
	return ScriviERileggi(*this, ValorePrima, OutError);
}
