#pragma once

#include "CoreMinimal.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnRules.h"
#include "RTTurnLog.generated.h"

/** Categoria dell'esito registrato nel TurnLog. */
UENUM(BlueprintType)
enum class ERTLogCategory : uint8 { Move, Combat };

/** Esito del movimento di un'unita' nel turno (dal resolver ResolvePaths). */
UENUM(BlueprintType)
enum class ERTMoveOutcome : uint8
{
	Stayed,           // non pianificava movimento (path < 2 celle)
	Moved,            // raggiunta la destinazione pianificata (scambio incluso)
	BlockedContested, // fermata (o parziale) per destinazione contesa
	BlockedByUnit     // fermata (o parziale) per cella occupata da un'unita' ferma
};

/** Esito di un attacco nel turno. Priorita': Lethal > ShieldAbsorbed > TerrainBonus > Hit. */
UENUM(BlueprintType)
enum class ERTCombatOutcome : uint8
{
	Hit,            // danno inflitto agli HP
	ShieldAbsorbed, // danno assorbito interamente dallo scudo (HP invariati)
	Lethal,         // bersaglio portato a HP <= 0
	NoLineOfSight,  // attacco pianificato scartato per LOS bloccata
	TerrainBonus    // colpo a segno con bonus altura (+danno), non letale
};

/**
 * Voce del TurnLog: un esito autoritativo del turno con il suo reason code. Osservabilita' separata
 * dalla presentazione (non e' FRTResolvedEvent). Deterministica: la chiave dell'unita' e' la sua cella
 * di partenza del turno (max 1 unita'/cella), mai un pointer.
 */
USTRUCT(BlueprintType)
struct FRTTurnLogEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	ERTMatchPhase Phase = ERTMatchPhase::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	ERTLogCategory Category = ERTLogCategory::Move;

	/** Valore dell'enum di categoria (ERTMoveOutcome se Move, ERTCombatOutcome se Combat). Intero: no float (#4). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	uint8 Outcome = 0;

	/** Chiave stabile: cella di partenza dell'unita' nel turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FRTGridCoord SrcCell;

	/** Bersaglio (Combat) o destinazione (Move); = SrcCell se non applicabile. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FRTGridCoord TgtCell;

	/** Danno effettivo (Combat) o numero di celle percorse (Move). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 Amount = 0;

	FRTTurnLogEntry() = default;
};

/**
 * Versione del formato di serializzazione binaria del TurnLog. Ogni formato serializzato e' versionato
 * (invariante #4): il loader rifiuta versioni sconosciute invece di interpretare byte arbitrari.
 * Non e' UENUM (uint16 esce dai vincoli UHT del BlueprintType uint8): e' una costante di formato interna.
 */
enum class ERTTurnLogFormatVersion : uint16
{
	Initial      = 1, // header + voci, senza checksum (mai persistito su file)
	WithChecksum = 2  // + checksum FNV del payload in coda: rileva la corruzione del contenuto
};
