#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "RTHexCellData.generated.h"

/** Tipi di superficie iniziali (prepara i dati senza sovraestendere: la simulazione ambientale arriva dopo). */
UENUM(BlueprintType)
enum class ERTHexSurface : uint8
{
	Floor,
	ShallowWater,
	Rough,
	Fire,
	Conductive,
	Ice,
	Void,
	Smoke,
	HighGround
};

/**
 * Tipo di copertura su UN bordo di cella. `High` (CP 9.2) si aggiungera' in CODA: estendere un enum non e'
 * una migrazione di formato, inventare oggi un valore che nessuna regola sa applicare lo sarebbe.
 */
UENUM(BlueprintType)
enum class ERTHexCoverType : uint8
{
	None,  // nessuna copertura (una voce con questo tipo e' un dato incoerente: la valida ValidateMap)
	Low,   // copertura bassa: ripara dai colpi diretti che attraversano il bordo, non blocca ne' vista ne' passo
	High   // copertura alta (CP 9.2): NEGA l'attraversamento del bordo a vista, passo e proiettili
};

/**
 * Copertura su UNO dei sei bordi di una cella. L'array della cella e' SPARSO: solo i bordi riparati vengono
 * serializzati, quindi una mappa senza coperture pesa e hasha esattamente come prima.
 *
 * La direzionalita' e' del BORDO, non dell'unita': girarsi non sposta un muretto. Il facing (ADR-0005, epic
 * #175) e' una direzionalita' ORTOGONALE — un colpo fuori dall'arco frontale ANNULLA questa riduzione
 * (CP 16.2), ma e' una regola additiva, non la stessa regola.
 */
USTRUCT(BlueprintType)
struct FRTHexCover
{
	GENERATED_BODY()

	/** Bordo riparato, visto DALLA cella: `W` protegge dai colpi che entrano dal vicino a ovest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexDirection Edge = ERTHexDirection::E;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexCoverType Type = ERTHexCoverType::Low;

	/**
	 * Punti struttura del riparo (catalogo v0.1: 30 per la copertura bassa). Il consumatore di questo turno
	 * e' `ValidateMap` (un riparo a 0 non e' un riparo); la DISTRUZIONE arriva con CP 9.2, che lo scalera'.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Integrity = 30;

	/**
	 * PROVENIENZA: questa copertura l'ha prodotta la cottura della geometria (`#621`), non la mano di un
	 * designer. Default `false` = dipinta a mano — che e' anche cio' che ogni copertura scritta prima di
	 * questo campo diventa rileggendosi, ed e' esattamente cio' che quelle coperture gia' erano.
	 *
	 * ⚠️ **Il rebake tocca SOLO le proprie** (`D-131`): rimuove le `bGenerated`, riscrive quelle derivate dai
	 * segmenti correnti, e non tocca mai le altre. Senza questo campo un rebake non saprebbe distinguere «la
	 * copertura che avevo prodotto io e ora va tolta» da «quella dipinta a mano che va preservata», quindi
	 * TOGLIERE un segmento non potrebbe togliere la sua copertura. E' cio' che rende il bake idempotente.
	 *
	 * 🔑 **NON entra in `ComputeHash`, ed e' la parte da non sbagliare.** Le coperture ci entrano perche' sono
	 * dato autorevole — cambiano il danno subito — ma la PROVENIENZA non cambia una partita: se entrasse, due
	 * mappe che si giocano in modo identico avrebbero hash diversi solo perche' una copertura e' stata
	 * disegnata invece che dipinta, cioe' un falso positivo contro il KPI `replay divergence = 0`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bGenerated = false;

	/**
	 * Integrita' di catalogo per tipo: 30 la bassa, 50 l'alta (v0.1). Sta qui e non in `URTCombatLibrary`
	 * perche' `Map/` non puo' dipendere da `Combat/`; la RIDUZIONE del danno, che e' della stessa famiglia dei
	 * numeri di `Guard`/`Deflect`/`Brace`, resta invece di la'.
	 */
	static constexpr int32 DefaultIntegrity(ERTHexCoverType Type)
	{
		return Type == ERTHexCoverType::High ? 50 : 30;
	}

	FRTHexCover() = default;
	explicit FRTHexCover(ERTHexDirection InEdge, ERTHexCoverType InType = ERTHexCoverType::Low,
		int32 InIntegrity = 30)
		: Edge(InEdge), Type(InType), Integrity(InIntegrity) {}
};

/**
 * Stato di una porta su un bordo (CP 9.3). `Closed` e `Locked` bloccano allo stesso modo passo e vista: la
 * differenza e' CHI puo' riaprirle — `SetDoorState` non apre una `Locked`, serve l'apertura autorizzata di
 * CP 10.1. `Destroyed` e' TERMINALE: una porta sfondata non si richiude.
 */
UENUM(BlueprintType)
enum class ERTHexDoorState : uint8
{
	Open,      // si passa e si vede
	Closed,    // nega passo e vista; riapribile
	Locked,    // come Closed, ma non si apre da sola
	Destroyed  // aperta per sempre (stato terminale)
};

/**
 * Porta su UNO dei sei bordi di una cella. Come le coperture, l'array della cella e' SPARSO e la direzionalita'
 * e' del BORDO: una mappa senza porte pesa e hasha esattamente come prima.
 *
 * Una porta e' un bordo e non un arco (`FRTHexEdge`) perche' e' SOTTRATTIVA — nega un'adiacenza che esiste —
 * mentre l'arco e' additivo: `GraphNeighbors` aggiunge i sei vicini planari PRIMA e indipendentemente dagli
 * archi, quindi togliere un arco fra celle adiacenti non chiuderebbe nulla. Vedi
 * docs/gameplay/spec-porte-cp93.md §1.
 */
USTRUCT(BlueprintType)
struct FRTHexDoor
{
	GENERATED_BODY()

	/** Bordo occupato dalla porta, visto DALLA cella (stessa convenzione di `FRTHexCover`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexDirection Edge = ERTHexDirection::E;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexDoorState State = ERTHexDoorState::Closed;

	/**
	 * Gruppo di appartenenza: i bordi che lo condividono formano UNA porta larga e si commutano insieme, con
	 * un solo incremento di revisione (un portone e' un evento, non tre). `INDEX_NONE` = porta singola.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 DoorId = INDEX_NONE;

	/**
	 * Nome PUBBLICO della struttura (CP 23.3, #832), stabile attraverso salvataggio, ricarica e cottura.
	 *
	 * `DoorId` sopra resta l'indice interno del gruppo e non cambia mestiere: distingue i gruppi dentro
	 * l'asset, e chi rieditasse la mappa potrebbe riassegnarlo. Questo invece e' il nome che uno scenario
	 * puo' citare e che un replay puo' risolvere — l'anello che a `DoorId` manca.
	 *
	 * I bordi che lo condividono sono la STESSA struttura, con la stessa forma con cui `DoorId` gia'
	 * definisce il gruppo. `NAME_None` = struttura senza nome pubblico, che e' cio' che ogni porta scritta
	 * prima di questo campo diventa rileggendosi — ed e' esattamente cio' che quelle porte gia' erano.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	FName StableId;

	FRTHexDoor() = default;
	explicit FRTHexDoor(ERTHexDirection InEdge, ERTHexDoorState InState = ERTHexDoorState::Closed,
		int32 InDoorId = INDEX_NONE)
		: Edge(InEdge), State(InState), DoorId(InDoorId) {}
};

/**
 * Dato compatto e AUTOREVOLE di una cella esagonale (serializzato nell'asset mappa). Nessun Actor per cella.
 * Estendibile in milestone successive (hazard, Gameplay Tags, interazioni) senza rompere il formato.
 */
USTRUCT(BlueprintType)
struct FRTHexCellData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	FRTCellId Id;

	/** Quota/offset verticale della cella (per il rendering; la logica usa Layer + archi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Height = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexSurface Surface = ERTHexSurface::Floor;

	/** Costo di movimento base della cella (intero: no float nel pathfinding). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 MoveCost = 1;

	/**
	 * Sovrapprezzo di traversata dovuto alla GEOMETRIA che invade la cella (#619, formato v7): quanto costa in
	 * piu' passare da una cella stretta. Si somma a `MoveCost` — vedi `TotalMoveCost()`, che e' l'unico punto
	 * in cui i due si sommano.
	 *
	 * ⚠️ **Perche' non e' dentro `MoveCost`, che sarebbe stato piu' semplice**: quel campo ha gia' un
	 * produttore che lo RICALCOLA dalla sola `Surface` a ogni turno — `ARTTurnManager::ApplyDynamicSurface`
	 * quando una superficie cambia, `TickDynamicSurfaces` quando scade. Un corridoio stretto su cui
	 * un'abilita' mettesse dell'acqua tornerebbe, al ripristino, al costo del pavimento: il sovrapprezzo
	 * sparirebbe per il resto della partita e nessun test se ne accorgerebbe. Con due campi i due produttori
	 * non si toccano, ed e' la stessa forma di risposta che `MSE-1` cerca per i bordi.
	 *
	 * Non e' «un terzo booleano»: e' un intero, e i booleani accanto dicono un'altra cosa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 OccupancySurcharge = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bBlocksMovement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bBlocksLineOfSight = false;

	/**
	 * Coperture per bordo (0..6 voci, al massimo una per bordo). Sparso: le celle scoperte — la quasi
	 * totalita' di una mappa — non pagano nulla. Formato v3.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	TArray<FRTHexCover> Covers;

	/**
	 * Porte per bordo (0..6 voci, al massimo una per bordo). Sparso come `Covers`. Formato v4.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	TArray<FRTHexDoor> Doors;

	/**
	 * Costo TOTALE di entrare in questa cella: il terreno piu' il sovrapprezzo della geometria.
	 *
	 * Esiste come accessore, e non come somma ripetuta nei chiamanti, perche' i lettori del costo sono
	 * CINQUE — il costruttore di vicini del pathfinding, i due accumuli della simulazione e i due
	 * `MaxCellCost`. Cinque copie della stessa somma sono cinque posti da cui puo' sparire.
	 */
	int32 TotalMoveCost() const
	{
		return FMath::Max(0, MoveCost) + FMath::Max(0, OccupancySurcharge);
	}

	/** Tipo di copertura sul bordo indicato (`None` se il bordo e' scoperto). */
	ERTHexCoverType CoverOn(ERTHexDirection Edge) const
	{
		for (const FRTHexCover& Cover : Covers)
		{
			if (Cover.Edge == Edge) { return Cover.Type; }
		}
		return ERTHexCoverType::None;
	}

	/**
	 * Voce di copertura su quel bordo, o nullptr se il bordo e' scoperto. `CoverOn` risponde «che tipo», questa
	 * serve a chi deve leggere anche l'INTEGRITA' — spostare una copertura conservandola (CP 9.5) o scalarla.
	 */
	const FRTHexCover* CoverEntryOn(ERTHexDirection Edge) const
	{
		for (const FRTHexCover& Cover : Covers)
		{
			if (Cover.Edge == Edge) { return &Cover; }
		}
		return nullptr;
	}

	/** Porta dichiarata su quel bordo, o nullptr se il bordo non ne ha. */
	const FRTHexDoor* DoorOn(ERTHexDirection Edge) const
	{
		for (const FRTHexDoor& Door : Doors)
		{
			if (Door.Edge == Edge) { return &Door; }
		}
		return nullptr;
	}

	FRTHexCellData() = default;
	explicit FRTHexCellData(const FRTCellId& InId) : Id(InId) {}
};

/** Tipo semantico di una transizione verticale/speciale (per rendering, validazione e futuri costi di profilo). */
UENUM(BlueprintType)
enum class ERTHexTransitionKind : uint8
{
	Stair,     // scala
	Ramp,      // rampa
	Bridge,    // ponte
	Tunnel,    // tunnel
	Elevator,  // ascensore
	Jump       // salto (predisposizione; teletrasporto resta futuro)
};

/**
 * Stato di un arco (CP 9.4). `Inactive` e `Destroyed` sono indistinguibili per il grafo — da nessuno dei due
 * si passa — e differiscono per la REVERSIBILITA': un ponte disattivato si riattiva, uno distrutto no. E' la
 * stessa distinzione che le porte fanno fra `Closed` e `Destroyed`.
 */
UENUM(BlueprintType)
enum class ERTHexArcState : uint8
{
	Active,    // il collegamento esiste e si percorre
	Inactive,  // spento: non si passa, ma si puo' riattivare
	Destroyed  // abbattuto: stato TERMINALE
};

/**
 * Arco di transizione ESPLICITA tra due celle esagonali (scale, rampe, ponti, tunnel, ascensori). Direzionale:
 * il bidirezionale richiede due archi. Le celle su layer diversi NON sono adiacenti senza un arco.
 *
 * L'arco e' ADDITIVO: crea un collegamento dove non c'era. E' la ragione per cui le PORTE non stanno qui ma
 * sui bordi (CP 9.3) — negare un'adiacenza planare richiede un oggetto sottrattivo, e questo non lo e'.
 */
USTRUCT(BlueprintType)
struct FRTHexEdge
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	FRTCellId From;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	FRTCellId To;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Cost = 1;

	/** Tipo semantico dell'arco (informativo: non altera il pathfinding, che usa solo Cost). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexTransitionKind Kind = ERTHexTransitionKind::Stair;

	/** Stato del collegamento (CP 9.4). Solo `Active` e' percorribile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	ERTHexArcState State = ERTHexArcState::Active;

	/** Punti struttura del ponte (catalogo v0.1: 40). A zero l'arco passa a `Destroyed`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	int32 Integrity = DefaultIntegrity;

	/**
	 * L'elettricita' RISALE questo collegamento (CP 9.4). Senza, la propagazione resta planare: il BFS di
	 * `URTTerrainLibrary::CollectElectricPropagation` cammina sui sei vicini e non sale mai di layer. Un ponte
	 * conduttivo e' quindi un rischio oltre che una scorciatoia.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	bool bConductsElectricity = false;

	/**
	 * Nome PUBBLICO della struttura (CP 23.3, #832), come per `FRTHexDoor::StableId`.
	 *
	 * ⚠️ Qui non c'e' un `DoorId` su cui appoggiare il gruppo: un arco e' identificato dalla coppia
	 * `(From, To)` e nient'altro. Il gruppo pero' la semantica del repository ce l'ha gia' — un ponte
	 * bidirezionale sono **due archi reciproci ma un evento solo** (`UpdateTransitions`, con lo stesso
	 * argomento del portone di CP 9.3) — ed e' l'unica condivisione di nome ammessa fra archi.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Hex")
	FName StableId;

	/** Integrita' di catalogo di un ponte (v0.1). Sta qui per la stessa ragione di `FRTHexCover`. */
	static constexpr int32 DefaultIntegrity = 40;

	FRTHexEdge() = default;
	FRTHexEdge(const FRTCellId& InFrom, const FRTCellId& InTo, int32 InCost,
		ERTHexTransitionKind InKind = ERTHexTransitionKind::Stair)
		: From(InFrom), To(InTo), Cost(InCost), Kind(InKind) {}
};
