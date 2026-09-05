// Helper condivisi dai due file che ispezionano i `WBP_*` come asset (#2423).
//
// 🔴 **Esiste per una collisione di unity build, non per eleganza.** `RTFrontendWidgetAssetTests.cpp` e
// `RTMatchWidgetAssetTests.cpp` sono quasi-cloni, e portavano **quattro** funzioni libere omonime e di firma
// identica nei rispettivi namespace anonimi. Il namespace anonimo le isola finche' i due file restano unita'
// di traduzione separate; la unity build li concatena, e li' due definizioni identiche sono `C2084`. E' lo
// stesso guasto che ha fermato `main` il 2026-09-05 con le due `StandStill` (#2397), su una coppia di file
// diversa.
//
// ⚠️ **Qui dentro stanno SOLO le due che avevano anche il corpo identico.** Le altre due — `DescribeWidget`
// e `ReportAsset` — hanno la stessa firma e **contratti diversi**: quella del match stampa i soli `Offsets`
// e cerca le `BlueprintPure` non consumate, quella del frontend stampa anche `Anchors`/`Alignment` e cammina
// il widget tree. Fonderle cambierebbe cio' che uno dei due gruppi di test afferma, ed e' esattamente il
// divieto che #2397 ha scritto: *«il nome era in comune, il contratto no»*. Sono state **rinominate** nel
// file piu' recente, non unificate.
//
// ⛔ **Namespace NOMINATO e funzioni `inline`.** 🔴 **La prima stesura di questa riga motivava la scelta
// con un guasto che non esiste**, ed e' stata corretta in code review: un namespace anonimo in un header da'
// a **ogni** unita' di traduzione la propria copia a linkage interno, quindi non produce ne' `C2084` ne'
// `LNK2005` — compilerebbe. Le ragioni vere sono altre due, e bastano: una copia per TU e' codice duplicato
// nel binario, e un helper anonimo incluso in un header e' una trappola ODR appena qualcuno lo usa da un
// template o da una funzione `inline`. Il namespace nominato non ha nessuno dei due problemi.
//
// ⚠️ **E una funzione NON `inline` in un header sarebbe invece la collisione vera**: due `.cpp` che la
// includono la definiscono due volte nello stesso blob. E' il limite 3 dell'oracolo di `RTTestGuardTests.cpp`
// — gli header di `Tests/` non sono guardati — e questo file e' dentro quel punto cieco.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"

/** Il perimetro e' dichiarato nel nome: helper per i test che leggono i `WBP_*` come asset. */
namespace RTWidgetAssetTest
{
	/**
	 * Carica la generated class di un `WBP_*`. `nullptr` se l'asset non c'e': il chiamante lo dichiara
	 * fallimento con un messaggio che nomina il path, perche' «cast fallito» non direbbe quale asset.
	 */
	inline UWidgetBlueprintGeneratedClass* LoadWidgetClass(const TCHAR* Path)
	{
		return Cast<UWidgetBlueprintGeneratedClass>(
			StaticLoadObject(UWidgetBlueprintGeneratedClass::StaticClass(), nullptr, Path));
	}

	/** Il testo di un binding, nella forma in cui serve leggerlo in un log di automation. */
	inline FString DescribeBinding(const FDelegateRuntimeBinding& Binding)
	{
		return FString::Printf(TEXT("  %s.%s <- %s()  [Kind=%s]"),
			*Binding.ObjectName,
			*Binding.PropertyName.ToString(),
			*Binding.FunctionName.ToString(),
			Binding.Kind == EBindingKind::Function ? TEXT("Function") : TEXT("Property"));
	}
}
