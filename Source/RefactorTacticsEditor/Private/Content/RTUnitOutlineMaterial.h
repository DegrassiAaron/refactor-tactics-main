// Il materiale di post-process del contorno delle unita' (#3705, [D-508]): costruito da codice e salvato dal
// commandlet `RTBuildUnitOutlineMaterial`, con la disciplina di `D-229` — la sorgente e' questo codice, l'asset e'
// il suo output. Editor-only: qui si AUTORA il materiale, non si gioca.

#pragma once

#include "CoreMinimal.h"

class UMaterial;

namespace RTUnitOutlineMaterial
{
	/** I pin del nodo Custom, nell'ordine in cui la ricetta li crea. I primi quattro sono scene texture. */
	extern const TCHAR* const PinScene;
	extern const TCHAR* const PinStencil;
	extern const TCHAR* const PinUnitDepth;
	extern const TCHAR* const PinSceneDepth;

	/** La descrizione del nodo Custom: e' il nome con cui il test lo ritrova nell'asset. */
	extern const TCHAR* const CustomDescription;

	/**
	 * Il codice HLSL del nodo Custom.
	 *
	 * 🔑 **Generato dalle costanti di `RTUnitLayerOutline`, non riscritto.** Gli stencil, e quale di loro passa
	 * attraverso la geometria, hanno una sede sola nel runtime: qui diventano i numeri del codice. Il test
	 * `UnitOutline.MaterialMatchesTheRecipe` confronta questa stringa con quella salvata nell'asset, quindi un asset
	 * rimasto indietro rispetto al runtime non passa in silenzio.
	 */
	FString CustomCode();

	/**
	 * Ricostruisce il grafo su `Material`, svuotandolo prima: dominio post-process, quattro scene texture, i tre
	 * colori e lo spessore come parametri, un nodo Custom verso l'emissivo. Non salva. Falso se un nodo non nasce.
	 */
	bool BuildGraph(UMaterial* Material);

	/**
	 * Crea (o riusa) l'asset al percorso di `RTUnitLayerOutline`, ne ricostruisce il grafo e lo salva.
	 * Idempotente. Ritorna zero se e' riuscito; con `bDryRun` stampa che cosa farebbe e non scrive nulla.
	 */
	int32 Build(bool bDryRun);
}
