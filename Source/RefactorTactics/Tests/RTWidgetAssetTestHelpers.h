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
// ⛔ **Namespace NOMINATO, non anonimo.** Un namespace anonimo in un header da' a ogni unita' di traduzione
// la propria copia — il che sotto unity significa di nuovo una copia sola, e il problema tornerebbe il
// giorno in cui i due file finissero in blob diversi con un terzo consumatore in mezzo. Un namespace
// nominato con funzioni `inline` non ha quel modo di fallire.

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
