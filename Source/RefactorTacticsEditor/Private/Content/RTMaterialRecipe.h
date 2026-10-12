// Cio' che le ricette dei materiali generati (`D-229`) hanno in comune. Editor-only.

#pragma once

#include "CoreMinimal.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"

namespace RTMaterialRecipe
{
	/**
	 * Svuota il grafo di `Material`: e' il primo passo di una ricetta, che il grafo lo RICOSTRUISCE.
	 *
	 * 🔴 **Non `UMaterialEditingLibrary::DeleteAllMaterialExpressions`, che ne lascia indietro la meta'.** Quella
	 * funzione (UE 5.8, `MaterialEditingLibrary.cpp`) scorre `GetExpressions()` mentre `DeleteMaterialExpression`
	 * toglie dalla stessa collezione: a ogni cancellazione l'elemento dopo scala di un posto e viene saltato.
	 * Misurato il 2026-10-13 su `M_HexVolume`: dopo una seconda passata della ricetta il materiale aveva DUE nodi
	 * Custom, e il test di idempotenza lo ha preso. Su un asset gia' esistente ogni rigenerazione lascerebbe nodi
	 * orfani nel binario — niente a schermo, e un parametro col nome doppio a chi lo apre.
	 *
	 * Qui si copia l'elenco prima di cancellare.
	 */
	inline void ClearGraph(UMaterial* Material)
	{
		if (Material == nullptr)
		{
			return;
		}
		TArray<UMaterialExpression*> Tutte;
		for (UMaterialExpression* Expression : Material->GetExpressions())
		{
			Tutte.Add(Expression);
		}
		for (UMaterialExpression* Expression : Tutte)
		{
			UMaterialEditingLibrary::DeleteMaterialExpression(Material, Expression);
		}
	}

	/** Quante espressioni ha il grafo: e' cio' che un test confronta per accorgersi di un nodo orfano. */
	inline int32 ExpressionCount(const UMaterial* Material)
	{
		return Material != nullptr ? Material->GetExpressions().Num() : 0;
	}
}
