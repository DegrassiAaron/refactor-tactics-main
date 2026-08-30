#pragma once

#include "CoreMinimal.h"
#include "RTMapSource.generated.h"

/**
 * Sorgente della mappa su cui allestire la partita. Le voci generate non richiedono asset: `Content/**` non e'
 * versionato, quindi una mappa generata da codice e' l'unica che sopravvive a un clone.
 *
 * ⚠️ **L'ORDINE DELLE VOCI NON SI TOCCA.** `ARTGameMode::MapSource` e' una `UPROPERTY` salvata in
 * `BP_GameMode`, cioe' un byte in un `.uasset`: riordinare qui cambierebbe in silenzio la mappa su cui una
 * configurazione salvata dichiara di giocare. Le voci nuove si aggiungono **in coda**.
 *
 * ⚠️ **Vive qui e non in `RTGameMode.h`** (era li' fino a `E-SOLID` fetta 3): l'allestimento della partita e'
 * uscito dal GameMode, e un `Match/` che dovesse includere l'header del GameMode per leggere un enum di
 * mappa avrebbe una dipendenza al contrario. Il nome del tipo non cambia, quindi nessun `.uasset` se ne
 * accorge — i `UENUM` sono registrati per nome nel package, non per file.
 */
UENUM(BlueprintType)
enum class ERTMapSource : uint8
{
	/** La mappa d'autore assegnata all'`ARTHexMapActor` del livello. Se manca o e' vuota si ripiega sull'arena demo. */
	LevelAsset,

	/** Arena di ripiego generata: esagono pieno di `DemoArenaRadius`, pavimento liscio. Un fondo di scena giocabile. */
	GeneratedDemoArena,

	/** Mappa di PROVA generata: ostacoli, muri che bloccano la vista, terreno costoso e piattaforma su un secondo layer. */
	GeneratedTestArena
};
