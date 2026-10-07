#include "RTLabCVarSnapshot.h"

namespace
{
	/** Scrive con la priorita' corrente e rilegge: `false` se il valore letto non e' quello chiesto. */
	bool ScriviERileggi(const FString& Nome, const FString& Valore, FString& OutError)
	{
		IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(*Nome);
		if (!Var)
		{
			OutError = FString::Printf(TEXT("la console variable '%s' non esiste in questo binario"), *Nome);
			return false;
		}

		Var->SetWithCurrentPriority(*Valore);

		const FString Letto = Var->GetString();
		if (Letto != Valore)
		{
			OutError = FString::Printf(
				TEXT("la console variable '%s' non ha accettato \"%s\" (vale \"%s\"): e' ReadOnly, oppure il valore non e' parsabile"),
				*Nome, *Valore, *Letto);
			return false;
		}
		return true;
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
	return ScriviERileggi(Nome, Valore, OutError);
}

bool FRTLabCVarSnapshot::Restore(FString& OutError) const
{
	return ScriviERileggi(Nome, ValorePrima, OutError);
}
