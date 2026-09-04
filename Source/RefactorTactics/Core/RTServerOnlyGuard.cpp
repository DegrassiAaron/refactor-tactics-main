#include "Core/RTServerOnlyGuard.h"

#include "UObject/Class.h"
#include "UObject/PropertyOptional.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace RTServerOnlyGuard
{
	const TCHAR* const ServerOnlyMetaKey = TEXT("RTServerOnly");
	const TCHAR* const FixtureMetaKey = TEXT("RTServerOnlyGuardFixture");
}

namespace
{
	const TCHAR* RouteName(ERTLeakRoute Route)
	{
		switch (Route)
		{
		case ERTLeakRoute::OwnMember:          return TEXT("OwnMember");
		case ERTLeakRoute::ReplicatedProperty: return TEXT("ReplicatedProperty");
		case ERTLeakRoute::RpcParameter:       return TEXT("RpcParameter");
		default:                               return TEXT("?");
		}
	}

	/**
	 * Le classi che l'editor tiene in giro dopo una ricompilazione di Blueprint — `REINST_`, `SKEL_`,
	 * `TRASHCLASS_` e le versioni superate. Sono duplicati della classe vera: contarle produrrebbe lo
	 * stesso leak due o tre volte, con nomi che non esistono in nessun sorgente.
	 */
	bool IsTransientEditorClass(const UClass* Class)
	{
		if (!Class)
		{
			return true;
		}
		if (Class->HasAnyClassFlags(CLASS_NewerVersionExists))
		{
			return true;
		}
		const FString Name = Class->GetName();
		return Name.StartsWith(TEXT("REINST_"))
			|| Name.StartsWith(TEXT("SKEL_"))
			|| Name.StartsWith(TEXT("TRASHCLASS_"));
	}

	/**
	 * Vero se il tipo di `Prop` **contiene** uno dei bersagli, direttamente o annidato.
	 *
	 * ⛔ **Non si scende dentro un puntatore a UObject**, e non è pigrizia: le proprietà replicate della
	 * classe puntata sono già viste dalla scansione che le incontra *come proprietà di quella classe*, e
	 * discendere farebbe camminare il cammino su tutto il grafo dell'engine. Un puntatore verso una
	 * **classe** marcata server-only resta però un leak — quello si controlla direttamente, senza scendere.
	 *
	 * `Visited` memorizza le struct che NON portano a un bersaglio: senza, una struct che si contiene
	 * indirettamente farebbe girare la ricorsione all'infinito.
	 */
	bool PropertyReaches(
		const FProperty* Prop,
		const TSet<const UStruct*>& Targets,
		TSet<const UStruct*>& Visited,
		FString& OutPath,
		const UStruct*& OutHit)
	{
		if (!Prop)
		{
			return false;
		}

		if (const FStructProperty* AsStruct = CastField<FStructProperty>(Prop))
		{
			UScriptStruct* Struct = AsStruct->Struct;
			if (!Struct)
			{
				return false;
			}
			if (Targets.Contains(Struct))
			{
				OutHit = Struct;
				OutPath = Struct->GetName();
				return true;
			}
			if (Visited.Contains(Struct))
			{
				return false;
			}
			Visited.Add(Struct);

			for (TFieldIterator<FProperty> It(Struct, EFieldIterationFlags::IncludeSuper); It; ++It)
			{
				FString InnerPath;
				if (PropertyReaches(*It, Targets, Visited, InnerPath, OutHit))
				{
					OutPath = FString::Printf(TEXT("%s::%s -> %s"), *Struct->GetName(), *It->GetName(), *InnerPath);
					return true;
				}
			}
			return false;
		}

		if (const FArrayProperty* AsArray = CastField<FArrayProperty>(Prop))
		{
			return PropertyReaches(AsArray->Inner, Targets, Visited, OutPath, OutHit);
		}
		if (const FSetProperty* AsSet = CastField<FSetProperty>(Prop))
		{
			return PropertyReaches(AsSet->ElementProp, Targets, Visited, OutPath, OutHit);
		}
		if (const FMapProperty* AsMap = CastField<FMapProperty>(Prop))
		{
			return PropertyReaches(AsMap->KeyProp, Targets, Visited, OutPath, OutHit)
				|| PropertyReaches(AsMap->ValueProp, Targets, Visited, OutPath, OutHit);
		}
		if (const FOptionalProperty* AsOptional = CastField<FOptionalProperty>(Prop))
		{
			return PropertyReaches(AsOptional->GetValueProperty(), Targets, Visited, OutPath, OutHit);
		}

		if (const FObjectPropertyBase* AsObject = CastField<FObjectPropertyBase>(Prop))
		{
			const UClass* Pointed = AsObject->PropertyClass;
			if (!Pointed)
			{
				return false;
			}
			for (const UStruct* Target : Targets)
			{
				const UClass* TargetClass = Cast<UClass>(Target);
				if (TargetClass && Pointed->IsChildOf(TargetClass))
				{
					OutHit = Target;
					OutPath = Pointed->GetName();
					return true;
				}
			}
			return false;
		}

		return false;
	}

	void AddLeak(
		TArray<FRTReplicationLeak>& Leaks,
		const UStruct* Hit,
		ERTLeakRoute Route,
		const FString& Carrier,
		const FString& Path)
	{
		FRTReplicationLeak Leak;
		Leak.ServerOnlyType = Hit ? Hit->GetFName() : NAME_None;
		Leak.Route = Route;
		Leak.Carrier = Carrier;
		Leak.Path = Path;
		Leaks.Add(MoveTemp(Leak));
	}
}

FString FRTReplicationLeak::Describe() const
{
	return FString::Printf(
		TEXT("%s raggiungibile via %s — portato da %s — cammino: %s"),
		*ServerOnlyType.ToString(),
		RouteName(Route),
		*Carrier,
		*Path);
}

bool RTServerOnlyGuard::IsMetadataAvailable()
{
#if WITH_METADATA
	return true;
#else
	return false;
#endif
}

TArray<UStruct*> RTServerOnlyGuard::CollectServerOnlyTypes(bool bIncludeFixtures)
{
	TArray<UStruct*> Out;

#if WITH_METADATA
	auto Consider = [&Out, bIncludeFixtures](UStruct* Type)
	{
		if (!Type || !Type->HasMetaData(ServerOnlyMetaKey))
		{
			return;
		}
		if (!bIncludeFixtures && Type->HasMetaData(FixtureMetaKey))
		{
			return;
		}
		Out.Add(Type);
	};

	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		Consider(*It);
	}
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (!IsTransientEditorClass(*It))
		{
			Consider(*It);
		}
	}

	// Ordine stabile: l'insieme dei tipi caricati non ha un ordine garantito, e un messaggio di
	// fallimento che cambia da run a run non si confronta con quello di ieri.
	Out.Sort([](const UStruct& A, const UStruct& B) { return A.GetPathName() < B.GetPathName(); });
#endif

	return Out;
}

TArray<FRTReplicationLeak> RTServerOnlyGuard::FindLeaks(const TArray<UStruct*>& ServerOnlyTypes)
{
	TArray<FRTReplicationLeak> Leaks;
	if (ServerOnlyTypes.Num() == 0)
	{
		return Leaks;
	}

	TSet<const UStruct*> Targets;
	for (UStruct* Type : ServerOnlyTypes)
	{
		if (Type)
		{
			Targets.Add(Type);
		}
	}

	// ROTTA 1 — il tipo ha esso stesso una proprietà replicata. È il caso letterale del DoD, e per una
	// `USTRUCT` è anche il più raro: la replica si dichiara sulla classe che la trasporta.
	for (UStruct* Type : ServerOnlyTypes)
	{
		if (!Type)
		{
			continue;
		}
		for (TFieldIterator<FProperty> It(Type, EFieldIterationFlags::IncludeSuper); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Net))
			{
				AddLeak(Leaks, Type, ERTLeakRoute::OwnMember,
					FString::Printf(TEXT("%s::%s"), *Type->GetName(), *It->GetName()),
					Type->GetName());
			}
		}
	}

	// ROTTA 2 — una `UPROPERTY(Replicated)` ovunque nel grafo lo contiene. È il *leak indiretto*.
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;
		if (IsTransientEditorClass(Class))
		{
			continue;
		}

		// `EFieldIterationFlags::None`: solo le proprietà DICHIARATE da questa classe. Con la super, una
		// proprietà di `AActor` verrebbe riportata una volta per ogni classe derivata caricata.
		for (TFieldIterator<FProperty> It(Class, EFieldIterationFlags::None); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_Net))
			{
				continue;
			}
			TSet<const UStruct*> Visited;
			FString Path;
			const UStruct* Hit = nullptr;
			if (PropertyReaches(*It, Targets, Visited, Path, Hit))
			{
				const FString Carrier = FString::Printf(TEXT("%s::%s"), *Class->GetName(), *It->GetName());
				AddLeak(Leaks, Hit, ERTLeakRoute::ReplicatedProperty, Carrier,
					FString::Printf(TEXT("%s -> %s"), *Carrier, *Path));
			}
		}
	}

	// ROTTA 3 — parametro di un RPC. PDR-04 §9 passo 3 nomina gli RPC accanto alle proprietà: un intento
	// che viaggia come argomento parte esattamente come uno che viaggia come stato replicato.
	for (TObjectIterator<UFunction> FunctionIt; FunctionIt; ++FunctionIt)
	{
		UFunction* Function = *FunctionIt;
		if (!Function || !Function->HasAnyFunctionFlags(FUNC_Net))
		{
			continue;
		}
		if (IsTransientEditorClass(Function->GetOwnerClass()))
		{
			continue;
		}

		for (TFieldIterator<FProperty> It(Function, EFieldIterationFlags::None); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_Parm))
			{
				continue;
			}
			TSet<const UStruct*> Visited;
			FString Path;
			const UStruct* Hit = nullptr;
			if (PropertyReaches(*It, Targets, Visited, Path, Hit))
			{
				const UClass* Owner = Function->GetOwnerClass();
				const FString Carrier = FString::Printf(TEXT("%s::%s(%s)"),
					Owner ? *Owner->GetName() : TEXT("?"), *Function->GetName(), *It->GetName());
				AddLeak(Leaks, Hit, ERTLeakRoute::RpcParameter, Carrier,
					FString::Printf(TEXT("%s -> %s"), *Carrier, *Path));
			}
		}
	}

	Leaks.Sort([](const FRTReplicationLeak& A, const FRTReplicationLeak& B)
	{
		return A.Describe() < B.Describe();
	});

	return Leaks;
}

TArray<FRTReplicationLeak> RTServerOnlyGuard::FindLeaksForType(UStruct* ServerOnlyType)
{
	TArray<UStruct*> One;
	if (ServerOnlyType)
	{
		One.Add(ServerOnlyType);
	}
	return FindLeaks(One);
}
