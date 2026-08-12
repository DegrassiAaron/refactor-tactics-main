#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Perception/RTTeamKnowledge.h" // FRTTeamKnowledge: la conoscenza di squadra viaggia nello snapshot
#include "Turn/RTTurnLog.h"
#include "RTHexSim.generated.h"

class URTHexMapAsset;

/**
 * Stato minimo di un'unita' per la simulazione esagonale di un turno. L'identita' e' un INTERO STABILE
 * (UnitId), mai un pointer: e' la stessa disciplina del TurnLog (determinismo/replay).
 */
USTRUCT(BlueprintType)
struct FRTHexSimUnit
{
	GENERATED_BODY()

	/** Identita' stabile dell'unita' nel turno (chiave di occupazione e di risultato). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 UnitId = 0;

	/** Posizione autorevole a inizio fase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	FRTCellId Cell;

	/** Le unita' non vive non occupano celle e non partecipano al movimento. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	bool bAlive = true;

	/** Costo massimo (intero) spendibile nel turno per il movimento; 0 = immobile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 MoveBudget = 0;

	/**
	 * Sovrapprezzo (intero, >= 0) aggiunto al costo di OGNI cella attraversata, per QUESTA unita' (`Action.Slow`,
	 * CP 4.7): 0 = nessun sovrapprezzo. E' un costo di pathfinding, non una riduzione del budget totale — la
	 * differenza conta perche' rende piu' cara la strada lunga senza rendere impossibile quella corta,
	 * mentre dimezzare il budget penalizza allo stesso modo un passo e dieci.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	int32 MoveCostModifier = 0;

	/**
	 * Orientamento AUTOREVOLE (CP 16.1): non lo yaw della mesh, ma il dato che decide da che lato si e'
	 * scoperti e dove punta un cono. Entra nello snapshot perche' e' stato di gioco, quindi entra anche
	 * nell'hash del replay attraverso le voci di TurnLog che lo registrano.
	 *
	 * Un solo campo, non sei: la timeline di D-020 e' la SEQUENZA delle scritture, e vive nel TurnLog. Tenere
	 * qui i sei momenti significherebbe conservare cinque valori che nessuno rilegge, e doverli invalidare a
	 * ogni cambio. Il valore qui e' sempre «il piu' recente», che e' esattamente cio' che D-020 chiede ai
	 * consumatori di leggere. Il facing di fine round persiste in quello dopo perche' l'unita' lo porta con se'.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexSim")
	ERTHexDirection Facing = ERTHexDirection::E;

	FRTHexSimUnit() = default;
	FRTHexSimUnit(int32 InUnitId, const FRTCellId& InCell, int32 InMoveBudget = 0, bool bInAlive = true)
		: UnitId(InUnitId), Cell(InCell), bAlive(bInAlive), MoveBudget(InMoveBudget) {}
};

/** Cella raggiungibile entro il budget, col costo cumulato dalla partenza. */
USTRUCT(BlueprintType)
struct FRTHexReachableCell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	FRTCellId Cell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	int32 Cost = 0;

	/**
	 * Cella da cui si ARRIVA su questa: il predecessore nel percorso piu' economico. Per la cella di partenza
	 * vale la cella stessa.
	 *
	 * Esiste per il FACING (CP 13.5): l'orientamento deriva dall'ultimo passo, e chi valuta una destinazione
	 * deve poter sapere da che parte ci arrivera' senza rifare il pathfinding per ogni candidata. La ricerca
	 * che produce queste celle il predecessore lo conosce gia' mentre espande — qui si smette di buttarlo via.
	 *
	 * ⚠️ E' il predecessore del percorso che la ricerca ha scelto, non «un» percorso qualsiasi: se un giorno
	 * la ricerca cambiasse criterio a parita' di costo, questo campo cambierebbe con lei. E' voluto — il facing
	 * deve seguire il percorso che il resolver poi costruira', non uno alternativo.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	FRTCellId FromCell;

	FRTHexReachableCell() = default;
	FRTHexReachableCell(const FRTCellId& InCell, int32 InCost) : Cell(InCell), Cost(InCost), FromCell(InCell) {}
	FRTHexReachableCell(const FRTCellId& InCell, int32 InCost, const FRTCellId& InFrom)
		: Cell(InCell), Cost(InCost), FromCell(InFrom) {}
};

/** Esito del movimento simultaneo di un'unita': cella finale, celle attraversate (partenza esclusa), reason code. */
USTRUCT(BlueprintType)
struct FRTHexMoveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	FRTCellId Final;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	TArray<FRTCellId> Entered;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexSim")
	ERTMoveOutcome Outcome = ERTMoveOutcome::Stayed;
};

/**
 * Stato di una risoluzione di movimento SOSPENDIBILE fra un microstep e l'altro (CP 14.2).
 *
 * Esiste per una ragione sola: un Overwatch interattivo deve poter fermare il movimento **dentro** il calcolo.
 * Se la finestra si aprisse a movimento concluso, il futuro sarebbe gia' stato calcolato con uno stato che il
 * colpo avrebbe dovuto cambiare — e il prompt mostrerebbe una scelta gia' irrilevante.
 *
 * NON e' una USTRUCT, come `FRTHexSnapshot` qui sotto e per una ragione affine: `TArray<TArray<>>` non e'
 * esponibile a UPROPERTY, e questo e' stato interno di un calcolo, non un dato di gioco che qualcuno debba
 * ispezionare da Blueprint.
 *
 * **Gli input sono COPIATI, non referenziati.** E' il punto della struttura: un resolver che restituisce il
 * controllo non puo' dipendere dalla vita di variabili locali del chiamante, che fra un microstep e il
 * successivo puo' essere uscito dal proprio scope — o aver aperto una finestra di reazione durata un turno di
 * orologio. I percorsi di una partita 2v2 sono poche decine di celle: la copia costa meno di un dangling.
 */
struct FRTMovementResolutionState
{
	/** Percorsi pianificati, uno per unita'. Indice = identita' dell'unita' per tutta la risoluzione. */
	TArray<TArray<FRTCellId>> Paths;

	/** Precedenza sulla cella contesa (CP 4.8). Vuoto = tutti a parita', com'era prima delle priorita'. */
	TArray<int32> Priorities;

	/** Chi si muove in linea (`Action.Charge` e affini): due lineari non si attraversano, si scontrano. */
	TArray<bool> bLinearMovers;

	/** Chi ATTRAVERSA le unita' ferme, tranne che sulla propria cella finale. */
	TArray<bool> bPassThrough;

	/** Posizione corrente di ogni unita'. */
	TArray<FRTCellId> Pos;

	/** Indice raggiunto dentro il proprio `Paths[i]`. */
	TArray<int32> Prog;

	/** Percorso esaurito, o nessun movimento da fare. */
	TArray<bool> Done;

	/**
	 * Motivo del PRIMO congelamento, e solo di quello: un microstep successivo bloccherebbe la stessa unita'
	 * per un motivo diverso e piu' recente, ma meno vero. La memoria vive nello stato perche' e' proprio cio'
	 * che un resolver a passi non puo' ricalcolare guardando l'ultimo passo.
	 */
	TArray<ERTMoveOutcome> BlockReason;

	/** Il motivo di `BlockReason[i]` e' stato fissato e non si sovrascrive. */
	TArray<bool> ReasonLocked;

	/** Risultato in costruzione: `Entered` cresce a ogni microstep, `Outcome` si scrive alla fine. */
	TArray<FRTHexMoveResult> Results;

	/** Quanti microstep sono stati eseguiti. Diagnostico: nessuna regola lo legge. */
	int32 MicroStepIndex = 0;

	/** Vero quando l'ultimo microstep non ha mosso nessuno: da qui in poi `Results` non cambia piu'. */
	bool bFinished = false;

	int32 Num() const { return Paths.Num(); }
};

/**
 * Stato CONGELATO a inizio fase per la risoluzione su griglia esagonale ("raccogli poi applica", invariante #3).
 *
 * NON e' una USTRUCT e NON va conservata oltre la fase che la produce: contiene un puntatore non-UPROPERTY
 * all'asset mappa (nessuna protezione dal GC). L'occupazione e' COPIATA (cambia durante la risoluzione);
 * la mappa e' solo referenziata insieme a hash/revisione, perche' e' immutabile per tutto il turno —
 * URTHexSimLibrary::IsSnapshotStale rileva la violazione di questa assunzione invece di ignorarla.
 */
struct FRTHexSnapshot
{
	/** Mappa autorevole (non posseduta): valida solo per la durata della fase. */
	const URTHexMapAsset* Map = nullptr;

	/** Hash del contenuto della mappa al momento della cattura. */
	uint32 MapHash = 0;

	/** Revisione della mappa al momento della cattura. */
	int32 Revision = 0;

	/** Unita' del turno, ordinate per UnitId (ordine stabile). */
	TArray<FRTHexSimUnit> Units;

	/** Cella -> UnitId dell'occupante (solo unita' vive). */
	TMap<FRTCellId, int32> Occupancy;

	/**
	 * Cosa sa ogni squadra (CP 13.2), ordinata per `TeamId` — un array e non una `TMap` perche' i consumatori
	 * la ITERANO, e l'ordine di una TMap non e' garantito (invariante #3).
	 *
	 * Sta nello snapshot e non in un servizio a parte perche' e' **stato del turno**: il bot che deciderà su
	 * conoscenza parziale (CP 13.5) e il resolver che rifiuta un bersaglio ignoto devono leggere la STESSA
	 * fotografia, altrimenti la partita si decide su due verita' diverse.
	 */
	TArray<FRTTeamKnowledge> TeamKnowledge;
};
