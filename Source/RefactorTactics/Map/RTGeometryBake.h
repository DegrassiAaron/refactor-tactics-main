#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTCellId.h"
#include "RTGeometryBake.generated.h"

class URTHexMapAsset;

/**
 * LA COTTURA — `#621`: la geometria disegnata diventa dato tattico, e poi e' arte.
 *
 * ```text
 * segmento APERTO        bordi attraversati        dato autorevole
 * (quantizzato)     ──►  della cella          ──►  FRTHexCover{Edge, Type, Integrity}
 * ```
 *
 * Dopo il bake il runtime non sa che la geometria sia esistita: legge coperture, come ha sempre fatto.
 *
 * ⚠️ **Solo i BORDI** (`D-129`). Il volume — `bBlocksMovement` — non si cuoce: resta scritto dal dato
 * d'autore, un produttore solo. La ragione e' in `D-125`: cio' che ingombra una cella sono **entita'** —
 * unita', props, elementi — cioe' cose che si piazzano, non che si disegnano. Coerentemente la grammatica
 * di `#620` esprime solo segmenti aperti.
 *
 * Vive nel modulo RUNTIME ed e' PURA rispetto al viewport: l'editor la chiama, non la contiene.
 */
UCLASS()
class REFACTORTACTICS_API URTGeometryBakeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * I bordi della cella che questo segmento muraglia: quelli che **attraversa**, e quello su cui
	 * eventualmente **giace**.
	 *
	 * Il lato `k` dell'esagono va dal vertice `2k` al vertice `2k+2` dei dodici punti di confine, e la sua
	 * direzione e' `ERTHexDirection(k)` — il suo punto medio e' il confine `2k+1`, a `60k` gradi, che e'
	 * esattamente dove `E` sta per `k = 0`.
	 *
	 * ⚠️ **La regola d'intersezione NON e' quella dell'occupancy, ed e' deliberato.** `ComputeMask` e'
	 * conservativa — un contatto in un solo punto conta — perche' la' la domanda e' *«questa geometria invade
	 * il settore?»*. Qui la domanda e' *«si passa da questo lato?»*, e la regola conservativa sbaglierebbe: un
	 * muro appoggiato al lato `E` ha gli estremi **sui due vertici**, ogni vertice appartiene a DUE lati, e
	 * murerebbe anche i due adiacenti che si limita a sfiorare. Un bordo e' chiuso solo se il muro lo
	 * **attraversa** o vi **giace sopra con lunghezza non nulla**. E' `MSE-4` applicata ai bordi.
	 *
	 * Deterministica e in ordine di bordo crescente, indipendente da come arriva il segmento.
	 */
	static void EdgesTouchedBy(const FRTGeometrySegment& Segment, float HexSize, TArray<ERTHexDirection>& OutEdges);

	/**
	 * Cuoce i segmenti di UNA cella nelle sue coperture, e restituisce quante ne ha generate.
	 *
	 * IDEMPOTENTE (`D-131`), ed e' la proprieta' che il campo `bGenerated` esiste per dare:
	 *
	 * ```text
	 * 1. rimuove le coperture con bGenerated = true
	 * 2. riscrive quelle derivate dai segmenti correnti
	 * 3. non tocca MAI quelle a false — dipinte a mano
	 * ```
	 *
	 * Ne segue che rieseguirla sulla stessa geometria non cambia nulla, e che TOGLIERE un segmento rimuove
	 * la sua copertura — cosa che senza provenienza sarebbe impossibile, perche' il bake non saprebbe quali
	 * coperture fossero sue.
	 *
	 * ⚠️ **Una copertura dipinta a mano vince sempre**: se il bordo ne ha gia' una a `bGenerated = false`, il
	 * segmento non la sostituisce. Un bordo ha al massimo una copertura — invariante di `ValidateMap` — e fra
	 * le due la mano dell'autore e' quella che il rebake non deve poter cancellare.
	 *
	 * Se due segmenti murano lo stesso bordo con tipi diversi vince `High`: il muro pieno prevale sul
	 * muretto, e la regola e' deterministica invece di dipendere dall'ordine.
	 *
	 * I segmenti fuori grammatica sono **ignorati** — `ValidateSegment` decide, non questa funzione.
	 */
	static int32 BakeCell(URTHexMapAsset* Map, const FRTCellId& CellId,
		const TArray<FRTGeometrySegment>& Segments, float HexSize);

	/**
	 * COME `BakeCell`, MA NON CANCELLA LE COPERTURE GENERATE GIA' PRESENTI.
	 *
	 * ⚠️ Serve al **gesto di disegno**, che e' un chiamante di natura diversa da quella per cui `BakeCell`
	 * e' stata scritta. `BakeCell` ha un contratto di *rebake*: «questi segmenti sono lo stato generato
	 * completo della cella», e per onorarlo deve prima buttare via il proprio. E' corretto per chi possiede
	 * l'elenco dei segmenti — un rigeneratore — ed e' cio' che rende TOGLIERE un segmento un'operazione
	 * possibile.
	 *
	 * Il tool d'editor non e' quel chiamante: vede **un gesto per volta** e non ha nessun elenco. Usando
	 * `BakeCell` cancellava il muro precedente a ogni tratto — trovato in seduta `U22` disegnando due muri
	 * che condividono un vertice, dove il secondo faceva sparire il primo.
	 *
	 * Le altre regole restano identiche, perche' sono le stesse:
	 *
	 * ```text
	 * copertura a mano sul bordo   vince sempre, non viene sostituita
	 * copertura generata sul bordo High prevale su Low, altrimenti resta
	 * ordine di scrittura          per bordo crescente, non per ordine dei segmenti
	 * ```
	 *
	 * ⚠️ **Non e' l'opposto di idempotente**: ripassare lo stesso segmento non accumula nulla, perche' un
	 * bordo ha al massimo una copertura. Cio' che perde rispetto a `BakeCell` e' la capacita' di far
	 * sparire una copertura togliendo il segmento — che e' esattamente il potere che al disegno non serve
	 * e che gli faceva danno.
	 *
	 * Restituisce quante coperture ha aggiunto **questo** gesto, non quante ne ha la cella.
	 */
	static int32 AddSegmentsToCell(URTHexMapAsset* Map, const FRTCellId& CellId,
		const TArray<FRTGeometrySegment>& Segments, float HexSize);

	/** Quante coperture generate porta una cella. Serve ai test e al conteggio della regione investita. */
	static int32 CountGeneratedCovers(const URTHexMapAsset* Map, const FRTCellId& CellId);
};
