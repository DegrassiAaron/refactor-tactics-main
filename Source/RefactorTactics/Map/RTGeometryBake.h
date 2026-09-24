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

	/**
	 * Questo segmento e' un MURO INTERNO — cioe' nessuna copertura di bordo puo' rappresentarlo?
	 *
	 * 🔴 **Esiste perche' la regola era scritta in DUE posti e i due divergevano** (misurato il 2026-09-24).
	 * `#2085` ha stabilito che `Offset == 0` — «il segmento passa per il centro» — e' una proprieta' della
	 * GIACITURA e non l'esito di una domanda sui bordi, e ha corretto la cottura; ma la correzione si e'
	 * fermata al suo primo chiamante, e `URTMapEditLibrary::MoveInteriorWall` ha continuato a definire
	 * «interno» per negazione, chiedendolo alla sola `EdgesTouchedBy`:
	 *
	 * ```text
	 * diametro Deg0 (lato -> lato)   la cottura lo scrive in InteriorWalls
	 *                                il move lo rifiuta RefusedWouldCloseEdge
	 * ```
	 *
	 * ⚠️ **Il danno non era un rifiuto in piu', era un rifiuto con una diagnosi FALSA**: quel valore
	 * significa *«e' una copertura, non un muro interno»*, e manda a correggere la cosa sbagliata — il
	 * difetto per cui `RefusedNoNeighbour` era gia' stato separato da `RefusedNoSuchCell`.
	 *
	 * ⛔ **`EdgesTouchedBy` non cambia, ed e' deliberato**: risponde correttamente alla propria domanda —
	 * *«si passa da questo lato?»* — e ha altri chiamanti di produzione a cui questa distinzione non
	 * appartiene. Cio' che cambia e' che «e' interno» ha ora UNA sede invece di due stesure.
	 */
	static bool IsInteriorSegment(const FRTGeometrySegment& Segment, float HexSize);

	/**
	 * Rideriva la CALPESTABILITA' di una cella dai muri interni che porta adesso, e basta.
	 *
	 * 🔑 **E' la coda di `BakeCell` senza il suo corpo, e la distinzione e' tutto il punto.** Dopo un move o
	 * una cancellazione l'insieme dei muri interni di una cella e' cambiato, quindi `bBlocksMovement`
	 * derivato va rifatto — ma coperture e muri sono gia' quelli giusti, e non vanno toccati.
	 *
	 * ⛔ **`BakeCell` NON e' lo strumento per questo, e usarla sarebbe distruttivo.** Il suo contratto e' di
	 * *rebake*: «questi segmenti sono lo stato generato completo della cella», e per onorarlo comincia
	 * buttando via **tutte le coperture generate e tutti i muri interni** della cella per riscriverli dai
	 * segmenti ricevuti. Chi la chiamasse passandole i soli `InteriorWalls` — gli unici che un'operazione di
	 * authoring ha sottomano — le farebbe cancellare le coperture cotte dal disegno, che derivano da
	 * segmenti che chiudono bordi e che in `InteriorWalls` per invariante non ci sono.
	 *
	 * ⚠️ **L'autore vince, e questa funzione non lo contraddice**: un `bBlocksMovement` dipinto a mano
	 * (`bMovementBlockGenerated == false`) resta dov'e'. E' la regola che `DeriveStandability` applica gia'
	 * e che `ValidateMap` REGOLA 4 rispetta non segnalandola — qui si eredita, non si riscrive.
	 *
	 * `false` se la mappa o la cella non esistono: una cella cancellata non ha piu' niente da ricuocere, e
	 * per un chiamante e' un esito normale, non un errore.
	 */
	static bool RederiveStandability(URTHexMapAsset* Map, const FRTCellId& CellId, float HexSize);
};
