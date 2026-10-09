#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"

/**
 * I CAMPI DI UNA `USTRUCT`, LETTI PER REFLECTION. Due sole funzioni, e servono a un gate solo: quello che
 * obbliga a **classificare** ogni campo di un modello invece di lasciarne uno fuori per distrazione.
 *
 * 🔴 **Perche' un header e non due copie.** Erano definite identiche in due file di test, ciascuna in
 * namespace anonimo, e sotto **Unity build** le due traduzioni finiscono nella stessa — che e' il difetto
 * che `RefactorTactics.Meta.AnonymousHelpersDoNotCollideUnderUnity` esiste per prendere, e che ha preso
 * mentre #2485 ne aggiungeva la terza copia. ⚠️ Rinominare la terza avrebbe fatto passare il gate e
 * lasciato le prime due: due copie divergono in silenzio appena una delle due viene indurita.
 *
 * **Namespace NOMINATO, non anonimo**, e `inline`: e' precisamente la forma che non collide.
 */
namespace RTTestReflection
{
	/** I nomi di tutte le `UPROPERTY` riflesse di un tipo, in un insieme. */
	inline TSet<FName> ReflectedNames(const UStruct* Type)
	{
		TSet<FName> Out;
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			Out.Add(It->GetFName());
		}
		return Out;
	}

	/**
	 * L'insieme, ordinato, per un messaggio di fallimento leggibile.
	 *
	 * ⚠️ **Ordinato, e non per estetica**: l'iterazione di un `TSet` non ha ordine garantito, e un
	 * messaggio che cambia fra due esecuzioni identiche non si puo' confrontare con quello di ieri.
	 */
	inline FString Listed(const TSet<FName>& Names)
	{
		TArray<FString> Out;
		for (const FName& Name : Names) { Out.Add(Name.ToString()); }
		Out.Sort();
		return FString::Join(Out, TEXT(", "));
	}
}
