#pragma once

#include "CoreMinimal.h"
#include "Turn/RTHexSim.h" // FRTMovementResolutionState e FRTHexSnapshot: stanno entrambe qui
#include "Turn/RTReactionOpportunityTypes.h" // FRTReactionOpportunity: cio' che una finestra offre

class ARTUnit;


/**
 * Un trigger dell'Overwatch gia' appaiato al suo armamento, in attesa di essere risolto (`#2679` fetta 2).
 *
 * 🔑 **Perche' l'`ArmedIndex` viaggia con l'opportunity invece di essere ricercato al momento.** Il lookup
 * originale confronta l'opportunity con la lista dei `Watchers`, che e' costruita dallo stato **corrente**.
 * Fra un trigger e il successivo la resolution puo' sospendersi per una finestra, e al rientro quella lista
 * sarebbe diversa — una charge spesa, un'unita' ferma. Appaiare una volta sola, prima di cominciare a
 * consumare, e' cio' che tiene l'ordine totale di ADR-0004 §4 uguale a se stesso attraverso una sospensione.
 */
struct FRTPendingReactionTrigger
{
	/** Cio' che viene offerto a chi decide. */
	FRTReactionOpportunity Opportunity;

	/** L'armamento da cui nasce, indice in `ARTTurnManager::ArmedOverwatches`. */
	int32 ArmedIndex = INDEX_NONE;
};

/**
 * Il contesto di una risoluzione di movimento che puo' SOSPENDERSI fra due micro-step (`#2679`, [D-355]).
 *
 * 🔑 **Non introduce la sospendibilita': la rende RAGGIUNGIBILE.** `FRTMovementResolutionState`
 * (`Turn/RTHexSim.h:201`) e' sospendibile dal CP 14.2 e lo dichiara nel proprio docstring — *«gli input sono
 * COPIATI, non referenziati … un resolver che restituisce il controllo non puo' dipendere dalla vita di
 * variabili locali del chiamante, che fra un microstep e il successivo puo' essere uscito dal proprio scope —
 * o aver aperto una finestra di reazione durata un turno di orologio»*. Il livello difficile era gia'
 * costruito: cio' che mancava era che `ARTTurnManager::ResolveMovement` smettesse di tenere quello stato sul
 * proprio stack, insieme alle altre locali che il ciclo attraversa.
 *
 * 🔴 **I campi sono MISURATI, non scelti.** Sono esattamente le locali di `ResolveMovement` che il codice
 * dopo il ciclo rilegge, contate su `RTTurnManager.cpp:7137-7660`. Due locali di quella funzione NON sono
 * qui, e l'assenza e' un fatto verificato invece che una dimenticanza: `PlannedMoves` e' consumata da
 * `BeginHexMovement` e mai riletta, e il ciclo non la tocca.
 *
 * ⚠️ **`Units` sono `TWeakObjectPtr` e non puntatori nudi.** Fra due micro-step passa, in prospettiva, una
 * finestra di reazione: un'unita' distrutta nel frattempo lascerebbe un dangling che il compilatore non vede
 * e che si manifesterebbe come un crash a valle, lontano dalla causa. Col modello sincrono di oggi non puo'
 * accadere — ed e' precisamente per il modello in cui potra' accadere che questa struct esiste.
 *
 * ⛔ **Non e' una USTRUCT**, per la stessa ragione di `FRTMovementResolutionState`: `TArray<TArray<>>` non e'
 * esponibile a UPROPERTY, e questo e' stato interno di un calcolo — non un dato di gioco che qualcuno debba
 * ispezionare da Blueprint.
 *
 * ⛔ **Non e' un secondo stato canonico.** E' il MEZZO con cui una risoluzione attraversa piu' frame. Lo
 * stato canonico resta quello delle unita' e dello snapshot; questo muore a `FinishMovementResolution`.
 */
struct FRTMovementResolutionContext
{
	/** Lo stato del resolver puro. Gia' sospendibile: e' il cuore che questa struct trasporta. */
	FRTMovementResolutionState State;

	/**
	 * Lo snapshot su cui il turno e' stato deciso.
	 *
	 * ⚠️ Va trasportato e non riletto dal mondo: `MakeCurrentSnapshot` a meta' risoluzione darebbe uno stato
	 * PIU' RECENTE di quello su cui i percorsi sono stati calcolati, e i due divergerebbero esattamente nei
	 * turni in cui qualcosa si e' mosso — cioe' sempre.
	 */
	FRTHexSnapshot Snapshot;

	/**
	 * Le unita' del turno, nell'ordine dello snapshot.
	 *
	 * 🔑 **Gli indici di `State` sono indici QUI.** Non e' una comodita': `FRTHexMoveResult` non porta
	 * l'identita' dell'unita', e l'unico legame fra un risultato e chi lo ha prodotto e' la posizione in
	 * questo array. Riordinarlo, filtrarlo o ricostruirlo da `GetAllActorsOfClass` — che non e' ordinato —
	 * scambierebbe gli esiti fra unita' senza che nulla fallisca.
	 */
	TArray<TWeakObjectPtr<ARTUnit>> Units;

	/** I percorsi come il turno li ha pianificati, per `BuildMoveLog`. */
	TArray<TArray<FRTCellId>> Paths;

	/** Chi e' stato accorciato dalla TOPOLOGIA: il resolver non puo' saperlo, questo ciclo si'. */
	TArray<bool> bStoppedByTopology;

	/** Chi ha DICHIARATO una destinazione e se l'e' vista negare perche' occupata (`#79`). */
	TArray<bool> bDeniedByOccupant;

	/** La destinazione richiesta e negata, per indice. Viaggia accanto al flag, mai dentro `Paths`. */
	TArray<FRTCellId> DeniedDestination;

	/**
	 * Quante celle ogni unita' aveva gia' percorso al micro-step precedente.
	 *
	 * 🔑 E' cio' che distingue **«ha fatto un passo»** da **«era ferma»**: l'Overwatch scatta su chi ENTRA
	 * nella cella controllata, non su chi ci sta. Vive qui e non nel ciclo perche' il ciclo, da `#2679` in
	 * poi, puo' uscire e rientrare.
	 */
	TArray<int32> EnteredBefore;

	/**
	 * Il contesto esagonale, congelato all'inizio.
	 *
	 * ⚠️ `GetHexContext` legge l'attore mappa, che fra due frame puo' cambiare. Le coordinate mondo calcolate
	 * a meta' risoluzione con un'origine diversa collocherebbero i modelli altrove — presentazione, non
	 * simulazione, ma visibile e ingiustificabile.
	 */
	FVector Origin = FVector::ZeroVector;
	float HexSize = 0.f;
	float LayerHeight = 0.f;

	/** Vero fra `Begin` e `Finish`. Un contesto non attivo non ha campi da leggere. */
	bool bActive = false;

	/**
	 * I trigger del micro-step corrente non ancora risolti, e a che punto e' arrivato il consumo
	 * (`#2679` fetta 2).
	 *
	 * 🔴 **Costruiti UNA volta e poi consumati, mai ricostruiti a meta'.** Ricalcolarli dopo che una
	 * decisione e' stata applicata darebbe una lista diversa — `ApplyReactionDecision` spende una charge e
	 * puo' fermare un mover — e i trigger gia' risolti si presenterebbero una seconda volta, o quelli non
	 * ancora visti sparirebbero. E' la stessa ragione per cui `RecordedDecisions` e' una mappa e non un
	 * array: cio' che identifica una finestra e' la sua IDENTITA', non la sua posizione in una lista che
	 * cambia sotto.
	 *
	 * ⚠️ `NextTrigger` e' un indice in `PendingTriggers`, non un contatore di micro-step: si azzera quando
	 * la lista si svuota, cioe' a ogni boundary nuovo.
	 */
	TArray<FRTPendingReactionTrigger> PendingTriggers;
	int32 NextTrigger = 0;

	/**
	 * La finestra aperta, se ce n'e' una. Vuoto significa **nessuna attesa in corso** (`#2679` fetta 2).
	 *
	 * 🔑 **E' l'`OpportunityId` e non un indice**, per la stessa ragione per cui `RecordedDecisions` e' una
	 * mappa: una risposta che arriva da fuori nomina la finestra a cui risponde, e una risposta in ritardo
	 * — arrivata dopo che la finestra e' scaduta e un'altra si e' aperta — deve poter essere **riconosciuta
	 * come tale** invece di essere applicata a quella sbagliata. Un indice non lo consente.
	 *
	 * ⚠️ Il trigger a cui appartiene e' `PendingTriggers[NextTrigger - 1]`: il pump incrementa l'indice
	 * **prima** di decidere, quindi la finestra aperta e' sempre quella dell'elemento gia' consumato.
	 */
	FString OpenWindowOpportunityId;

	/** Da quanti secondi la finestra aperta e' in attesa. Senza finestra non significa nulla. */
	float OpenWindowElapsed = 0.f;
};
