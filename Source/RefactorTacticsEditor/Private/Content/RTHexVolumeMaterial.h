// Il materiale dei volumi della mappa (#3706, [D-508]): muri, coperture, porte, blocchi e corpi dicono il piano con
// una tinta, e ricevono il trattamento di D-501. Costruito da codice e salvato dal commandlet
// `RTBuildHexVolumeMaterial`, con la disciplina di `D-229`: la sorgente e' questo codice, l'asset e' il suo output.

#pragma once

#include "CoreMinimal.h"

class UMaterial;

namespace RTHexVolumeMaterial
{
	/** I pin del nodo Custom del colore, nell'ordine in cui la ricetta li crea. */
	extern const TCHAR* const PinOwn;
	extern const TCHAR* const PinTint;
	extern const TCHAR* const PinDesaturation;
	extern const TCHAR* const PinExposureStops;
	extern const TCHAR* const PinTintStrength;

	/** La descrizione del nodo Custom: e' il nome con cui il test lo ritrova nell'asset. */
	extern const TCHAR* const CustomDescription;

	/** La funzione del motore che fa la maschera a retino: la stessa che `M_HexCell` usa per i piani sopra. */
	extern const TCHAR* const DitherFunctionPath;
	extern const TCHAR* const DitherInputName;

	/**
	 * Quante espressioni ha il grafo: il colore proprio, la tinta, i quattro dati per istanza scalari, la ruvidita',
	 * il nodo Custom, `1 - x` e la funzione del retino. Una in piu' e' un orfano.
	 */
	inline constexpr int32 ExpressionCount = 10;

	/**
	 * Il codice HLSL del nodo Custom che compone il colore.
	 *
	 * 🔑 **L'ordine e' la regola**: la desaturazione di D-501 sul colore proprio, poi la tinta del piano, poi la luce.
	 * Una tinta messa prima della desaturazione perderebbe il suo viola su un piano sotto; messa dopo la luce
	 * dominerebbe un corpo scurito e lo renderebbe piu' chiaro di un volume non tinto.
	 */
	FString CustomCode();

	/**
	 * Ricostruisce il grafo su `Material`, svuotandolo prima: superficie illuminata, `Masked`, usabile sulle mesh
	 * istanziate, coi sette dati per istanza di `RTLayerPresentation`. Non salva. Falso se un nodo non nasce.
	 */
	bool BuildGraph(UMaterial* Material);

	/** Crea (o riusa) l'asset al percorso di `RTLayerPresentation`, ne ricostruisce il grafo e lo salva. Zero = riuscito. */
	int32 Build(bool bDryRun);
}
