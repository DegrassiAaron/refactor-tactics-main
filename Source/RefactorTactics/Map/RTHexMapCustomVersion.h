#pragma once

#include "CoreMinimal.h"

/**
 * Versione di formato di `URTHexMapAsset`, nel regime che la regge davvero (**D-137**, #687).
 *
 * ## Perche' esiste questo file
 *
 * `URTHexMapAsset` aveva un numero di versione (`FormatVersion`), una funzione di migrazione
 * (`MigrateToCurrentFormat`) e una storia documentata v1-v8 — cioe' il **vocabolario** di un formato
 * versionato — ma non il meccanismo che lo regge. La causa precisa era un default MOBILE:
 *
 * ```cpp
 * int32 FormatVersion = CurrentFormatVersion;   // <- il difetto
 * ```
 *
 * La serializzazione delta di UE salta le property uguali al CDO, e al salvataggio il valore coincide
 * SEMPRE col default di allora. Quindi `FormatVersion` non finiva nei byte, ogni asset si ricaricava
 * gia' «aggiornato» e la migrazione non partiva mai. Misurato sui byte di `DA_HexMap_Arena` (#687).
 *
 * ## Perche' un custom version e non una property serializzata
 *
 * E' l'unico meccanismo che possiede **entrambi i lati**: scrive GUID e versione nel *summary* del
 * package — fuori dal delta, quindi non confrontabili col CDO — e in lettura distingue nativamente
 * *«asset piu' vecchio del meccanismo»* (`FArchive::CustomVer` restituisce `-1`) da *«asset a versione
 * N»*. E' esattamente la distinzione che serve a un asset committato prima di questo file.
 *
 * ⚠️ `SaveGame` era la direzione apparentemente ovvia ed **e' stata esclusa prima del voto**: e' un flag
 * che `FArchive` consulta quando `ArIsSaveGame` e' vero, ortogonale al confronto col CDO che produce il
 * delta di package. Non e' una questione di configurazione: non tocca il meccanismo giusto.
 *
 * ⚠️ E il «default fisso» (`= 1`) **peggiora le cose**: senza un lato-scrittura che timbri la versione
 * corrente, la migrazione riparte a OGNI caricamento — innocuo finche' i passi sono no-op, e il giorno
 * di un passo trasformativo lo riapplica ogni volta.
 *
 * ## Aggiungere una versione
 *
 * Si aggiunge un valore **in coda** (prima di `VersionPlusOne`) e si alza `CurrentFormatVersion` nello
 * stesso commit: lo `static_assert` in `RTHexMapAsset.cpp` non lascia scelta, ed e' deliberato — i due
 * numeri che divergono in silenzio sono la classe di difetto che questo file esiste per chiudere.
 */
struct FRTHexMapCustomVersion
{
	enum Type
	{
		/**
		 * Prima che questo meccanismo esistesse.
		 *
		 * ⚠️ **Non significa «versione 1»: significa «non lo sappiamo».** Un asset senza voce nel registro
		 * puo' essere stato scritto a qualunque versione fra 1 e 8, perche' il numero non viaggiava. Si
		 * riparte quindi dall'inizio della catena, e **oggi costa zero**: tutti e sette i passi v1->v8 sono
		 * DICHIARATIVI (il campo nuovo nasce vuoto), quindi migrare da 0 o da 6 da' lo stesso risultato.
		 *
		 * ⛔ Dal primo passo TRASFORMATIVO in poi questo non e' piu' vero, ed e' il motivo per cui D-137
		 * vieta di scriverne uno prima che #687 sia chiusa.
		 */
		BeforeCustomVersionWasAdded = 0,

		/**
		 * v1-v8 non hanno voce qui, e non e' una svista: nessun asset e' mai stato scritto CON questo
		 * meccanismo a quelle versioni, quindi una voce per ognuna descriverebbe byte che non esistono.
		 * La storia di quei passi resta dov'e' sempre stata — il commento di `CurrentFormatVersion` — e
		 * questo valore e' il primo che un asset puo' davvero portare nei propri byte.
		 */
		CoverProvenance = 8,

		/**
		 * #832 (CP 23.3): porte e archi guadagnano un nome pubblico stabile (`StableId`).
		 *
		 * Passo DICHIARATIVO come tutti i precedenti: il campo nasce `NAME_None`, e una mappa scritta prima
		 * non nominava le proprie strutture — quindi «anonima» e' cio' che gia' era, non una perdita.
		 */
		StructureIdentity = 9,

		/**
		 * #712 (seduta `U22`): la mappa guadagna i **muri interni**, la geometria che non giace su nessun
		 * bordo e che quindi nessuna copertura puo' rappresentare.
		 *
		 * ⚠️ Nasce da una misura, non da un desiderio di completezza: una retta che taglia l'esagono
		 * passando per due vertici opposti attraversa **una cella su tre** per il centro, e nelle altre due
		 * giace sul confine. Tracciata sulla griglia, quindi, e' per due terzi un bordo — esprimibile — e
		 * per un terzo una corda che non lo e'. Prima di questo campo quel terzo spariva in silenzio.
		 *
		 * Passo DICHIARATIVO come tutti i precedenti: l'elenco nasce vuoto, e una mappa scritta prima non
		 * aveva muri interni perche' non si potevano disegnare.
		 */
		InteriorWalls = 10,

		/**
		 * #75 (CP 10.2): la cella guadagna il flag di OBIETTIVO contendibile (`FRTHexCellData::bIsObjective`).
		 *
		 * Passo DICHIARATIVO come tutti i precedenti: il flag nasce `false`, e una mappa scritta prima non
		 * aveva obiettivi perche' non si potevano dichiarare — si vinceva per eliminazione o al limite di
		 * round, che e' esattamente cio' che quelle mappe gia' facevano.
		 */
		ObjectiveCell = 11,

		/**
		 * #1864: il muro interno guadagna un nome stabile (`FRTHexInteriorWall::StableId`).
		 *
		 * ⚠️ Nasce da un'operazione, non dalla simmetria con le porte: un muro interno **si sposta**, e la
		 * sua chiave naturale `(Cell, Segment)` cambia proprio nel move. Senza un nome, l'handle con cui un
		 * tool lo tiene selezionato si romperebbe durante il gesto a cui deve sopravvivere.
		 *
		 * Passo DICHIARATIVO come tutti i precedenti: il campo nasce `NAME_None`, e un muro scritto prima
		 * non aveva un nome perche' non c'era niente che lo spostasse.
		 */
		InteriorWallIdentity = 12,

		// -----<le versioni nuove si aggiungono SOPRA questa riga>------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	/** Chiave unica di questo formato nel registro globale. Non si cambia: identifica i byte gia' scritti. */
	static const FGuid GUID;

private:
	FRTHexMapCustomVersion() {}
};
