#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTGeometryGrammar.h"
#include "Map/RTCellId.h"
#include "Map/RTHexMapAsset.h"              // URTHexMapAsset + FRTNoWalkArea + ERTStandabilityBlock
#include "Map/RTHexOccupancyLibrary.h"      // FRTOccupancyMask
#include "Map/RTHexCoverPlacementLibrary.h" // FRTFootprintProfile
//
// ⚠️ Tre include nell'header, e non e' un ciclo: nessuno dei tre include questo file (misurato). Li chiede
// la firma di `WhyNotStandable`, che e' la sede unica del predicato di calpestabilita' — il prezzo di
// averne UNA invece di due.
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
	 * (`bMovementBlockGenerated == false`) resta suo — anche quando la geometria chiude la cella. E' la
	 * regola che `DeriveStandability` applica, e che `ValidateMap` REGOLA 4 rispetta non segnalandola.
	 *
	 * ⏱️ **La regola c'era e il ramo che BLOCCA la violava**, scrivendo `bMovementBlockGenerated = true`
	 * senza guardare il valore precedente: la marcatura d'autore veniva etichettata come derivata, e il
	 * gesto successivo — tolta la geometria — la spegneva. Invisibile finche' la cottura la chiamava il
	 * solo disegno; raggiungibile in due mosse da quando la chiamano anche il move e la cancellazione.
	 * Corretto insieme a questa funzione, e pinnato da
	 * `RefactorTactics.Map.Edit.AnAuthoredBlockSurvivesEvenWhenGeometryClosesTheCell`.
	 *
	 * `false` se la mappa o la cella non esistono: una cella cancellata non ha piu' niente da ricuocere, e
	 * per un chiamante e' un esito normale, non un errore.
	 */
	static bool RederiveStandability(URTHexMapAsset* Map, const FRTCellId& CellId, float HexSize);

	/**
	 * QUALI CELLE COPRE una regione No-Walk — o meglio: questa cella e' coperta? (`#1868`, [D-439])
	 *
	 * 🔴 **Il `Layer` si confronta PRIMA della geometria, e non e' una ottimizzazione.** `AxialToWorld`
	 * mette il piano interamente nella `Z` — `Wx` e `Wy` dipendono solo da `q` e `r` — quindi un test di
	 * appartenenza 2D **non distingue i piani**: senza questo confronto una regione al piano terra
	 * chiuderebbe le celle impilate sopra. ⚠️ Non e' un caso di laboratorio: l'arena committata ha tre
	 * celle sul layer 1 che stanno sopra celle del layer 0.
	 *
	 * ⚠️ **Sotto i tre vertici risponde `false`**: un poligono di due vertici non ha un «dentro», e
	 * interrogare l'appartenenza su una degenerazione darebbe una risposta arbitraria invece di nessuna.
	 *
	 * 🔴 **Decide sul RETICOLO INTERO** (`URTGeometryGrammarLibrary::RingContainsPoint`), e non in
	 * `FVector2D`: fino al 2026-09-25 passava da `PointInPolygon`, il cui confronto sul bordo non e'
	 * simmetrico — e **invertire il verso dell'anello cambiava quali celle la regione chiude**. I centri di
	 * cella stanno sul bordo per costruzione, quindi non era un caso limite; e `bBlocksMovement` entra in
	 * `ComputeHash`, quindi il difetto arrivava fino al digest.
	 *
	 * La regola di appartenenza e' il **centro** della cella dentro il poligono — la stessa che
	 * `URTHexOccupancyLibrary::ComputeMask` usa per `bCoreBlocked`, e non una seconda.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool AreaCoversCell(const FRTNoWalkArea& Area, const FRTCellId& CellId, float HexSize);

	/**
	 * PERCHE' QUESTA CELLA NON E' CALPESTABILE — **l'unica sede del predicato** (`#1868`, [D-439]).
	 *
	 * 🔴 **Esiste perche' le sedi erano DUE, e stavano per diventare due verita'.** `DeriveStandability` lo
	 * calcolava per cuocere; `URTHexMapAsset::ValidateMapDetailed` lo **ricalcolava** per giudicare, e le
	 * due erano tenute insieme solo dal commento che sta sopra la seconda — *«se questa misurasse
	 * diversamente da `DeriveStandability`, il validator segnalerebbe celle che il bake considera sane»* —
	 * senza nessun test che legasse le due. Misurato prima di scrivere: nessun test le confronta.
	 *
	 * ⛔ **E il difetto sarebbe stato silenzioso.** Aggiungendo la consultazione delle regioni alla sola
	 * cottura, una cella coperta ne usciva con `bBlocksMovement` e `bMovementBlockGenerated` accesi mentre
	 * il validator, che guarda la sola geometria, la vedeva sana: **REGOLA 4** sarebbe scattata su ogni
	 * cella coperta dicendo *«Ricuoci la mappa: il prossimo rebake lo toglierebbe»* — e il rebake invece lo
	 * **rimette**. Un avviso falso, con un rimedio che non fa niente.
	 *
	 * 🔑 **Restituisce la RAGIONE e non un `bool`** perche' il messaggio di REGOLA 1 nomina una causa:
	 * finche' l'unica era la geometria, *«la geometria non lascia alcuna posa legale»* era vera per
	 * costruzione; con le regioni quella frase manderebbe chi legge a cercare un muro che non c'e'.
	 *
	 * ⚠️ **La regione vince sulla posa, non la calcola**: e' il *«veto esplicito d'autore sopra quel
	 * calcolo»* di `#1868`, quindi si guarda per prima e chiude anche dove la geometria lascerebbe passare.
	 */
	static ERTStandabilityBlock WhyNotStandable(const URTHexMapAsset* Map, const FRTCellId& CellId,
		const FRTOccupancyMask& Mask, const FRTFootprintProfile& Footprint, float HexSize);
};
