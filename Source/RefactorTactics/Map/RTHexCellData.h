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

	/** Tipo di copertura sul bordo indicato (`None` se il bordo e' scoperto). */
	ERTHexCoverType CoverOn(ERTHexDirection Edge) const
	{
		for (const FRTHexCover& Cover : Covers)
		{
			if (Cover.Edge == Edge) { return Cover.Type; }
		}
		return ERTHexCoverType::None;
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
 * Arco di transizione ESPLICITA tra due celle esagonali (scale, rampe, ponti, tunnel, ascensori). Direzionale:
 * il bidirezionale richiede due archi. Le celle su layer diversi NON sono adiacenti senza un arco.
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

	FRTHexEdge() = default;
	FRTHexEdge(const FRTCellId& InFrom, const FRTCellId& InTo, int32 InCost,
		ERTHexTransitionKind InKind = ERTHexTransitionKind::Stair)
		: From(InFrom), To(InTo), Cost(InCost), Kind(InKind) {}
};
