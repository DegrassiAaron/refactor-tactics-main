#pragma once

#include "CoreMinimal.h"

/**
 * Primitive di geometria 2D condivise fra i consumatori di `Map/`.
 *
 * **Perche' un header e non una copia per file.** `Cross2D` e `SignOf` erano definiti, identici,
 * nell'anonymous namespace di `RTHexOccupancyLibrary.cpp` e di `RTGeometryBake.cpp`. Nella **unity build**
 * i due file finiscono nella stessa unita' di traduzione, i due anonymous namespace diventano lo stesso
 * namespace, e la seconda definizione e' una ridefinizione: `error C2264` + `C2440`, 16 errori.
 * `static` non avrebbe aiutato — il problema non e' il linkage, e' che le due definizioni si trovano
 * nello stesso file dopo la concatenazione.
 *
 * Il terzo consumatore che nascera' domani con lo stesso helper trova questo, invece di riaprire il buco.
 *
 * ⚠️ **Qui stanno solo le PRIMITIVE.** Le regole che le usano restano separate, ed e' deliberato:
 * `URTHexOccupancyLibrary::ComputeMask` e' conservativa — un contatto puntuale conta, perche' chiede
 * *«questa geometria invade il settore?»* — mentre `SegmentClosesEdge` non puo' esserlo, perche' chiede
 * *«si passa da questo lato?»* e un muro appoggiato a un lato tocca i due vertici, quindi murerebbe anche
 * i lati adiacenti che si limita a sfiorare. Sono due domande diverse: condividono l'aritmetica, **non** la
 * regola. Vedi il commento di `SegmentClosesEdge` in `RTGeometryBake.cpp`.
 */
namespace RTGeometry2D
{
	/** Prodotto vettoriale 2D di `OA` x `OB`: il segno dice da che parte sta `B` rispetto alla retta `OA`. */
	inline double Cross2D(const FVector2D& O, const FVector2D& A, const FVector2D& B)
	{
		return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
	}

	/** Segno con tolleranza: `0` non significa «nullo», significa «sulla retta entro l'epsilon». */
	inline int32 SignOf(double Value)
	{
		if (Value > UE_KINDA_SMALL_NUMBER) { return 1; }
		if (Value < -UE_KINDA_SMALL_NUMBER) { return -1; }
		return 0;
	}
}
