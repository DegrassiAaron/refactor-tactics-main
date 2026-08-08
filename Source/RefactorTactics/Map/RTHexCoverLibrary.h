#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "RTHexCoverLibrary.generated.h"

class URTHexMapAsset;

/**
 * Danno RACCOLTO contro la struttura su un bordo, gia' sommato per bordo (invariante #3: l'ordine dei colpi
 * non cambia l'esito). La coppia di celle identifica il bordo senza bisogno di una direzione: e' la stessa
 * convenzione con cui il TurnLog lo scrivera' in `SrcCell`/`TgtCell`.
 */
USTRUCT(BlueprintType)
struct FRTStructureHit
{
	GENERATED_BODY()

	/** Cella dal lato di chi colpisce. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId From;

	/** Cella oltre il bordo colpito. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId To;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 Amount = 0;

	/** Chi ha colpito per primo in ordine canonico: serve al TurnLog, non al calcolo. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 AttackerId = INDEX_NONE;

	FRTStructureHit() = default;
	FRTStructureHit(const FRTCellId& InFrom, const FRTCellId& InTo, int32 InAmount, int32 InAttackerId)
		: From(InFrom), To(InTo), Amount(InAmount), AttackerId(InAttackerId) {}
};

/** Cosa e' successo a una struttura dopo il danno: quanto le resta, e se e' caduta. */
USTRUCT(BlueprintType)
struct FRTCoverDamageResult
{
	GENERATED_BODY()

	/** Cella che PORTA la voce di copertura (puo' essere una delle due facce del bordo). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId Cell;

	/** Cella oltre il bordo: con `Cell` identifica la barriera in modo leggibile nel log. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId Toward;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTHexCoverType Type = ERTHexCoverType::None;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 RemainingIntegrity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	bool bDestroyed = false;
};

/**
 * Semantica del BORDO fra due celle adiacenti: che copertura c'e', e se nega l'attraversamento.
 *
 * Esiste come libreria propria perche' la stessa domanda la fanno tre aree che non devono dipendere l'una
 * dall'altra — la vista (`URTHexVisionLibrary`), il grafo di traversata (`URTHexPathLibrary`) e il combat
 * (`URTHexCombatLibrary`). Con la regola in un solo posto non possono divergere: una copertura che blocca la
 * vista ma non il passo sarebbe un difetto invisibile finche' qualcuno non ci cammina attraverso.
 */
UCLASS()
class REFACTORTACTICS_API URTHexCoverLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Direzione esagonale da `From` a `To` se le due celle sono adiacenti sullo STESSO layer.
	 * Falso altrimenti (celle lontane, uguali, o su layer diversi: quelle si collegano con archi espliciti).
	 */
	static bool EdgeDirection(const FRTCellId& From, const FRTCellId& To, ERTHexDirection& OutDirection);

	/**
	 * Copertura sul bordo fra due celle ADIACENTI, vista dai due lati: vale la PIU' ALTA delle due voci.
	 *
	 * Il dato sta su un bordo di una cella sola, ma una barriera e' fisica: chi la disegna ne mette una, e la
	 * regola non deve dipendere da quale delle due celle l'ha ricevuta (decisione sulla issue #70, 2026-08-07).
	 * `None` se le celle non sono adiacenti, se una manca dalla mappa o se `Map` e' nullo.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static ERTHexCoverType CoverBetween(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);

	/**
	 * Vero se il bordo fra due celle adiacenti NEGA l'attraversamento — vista, passo e proiettili.
	 * Lo fanno la copertura ALTA (la bassa ripara chi ci sta dietro, non chiude il passaggio) e una PORTA
	 * chiusa o bloccata (CP 9.3). E' l'unico punto che risponde alla domanda: cambiarlo aggiorna insieme
	 * `URTHexVisionLibrary`, `URTHexPathLibrary` e il combat, che non hanno una risposta propria.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool BlocksTraversal(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);

	/**
	 * Applica alla mappa il danno raccolto contro le strutture: scala l'integrita' e RIMUOVE la copertura che
	 * arriva a zero. Ritorna solo cio' che e' cambiato davvero, in ordine canonico, cosi' il chiamante ha
	 * esattamente le voci da scrivere nel TurnLog.
	 *
	 * Muta la mappa (la copia di lavoro della partita, non l'asset su disco: la duplica `ARTGameMode`), quindi
	 * va chiamata a FASE CONCLUSA — non colpo per colpo. Ogni modifica passa da `AddOrUpdateCell`, che
	 * incrementa la revisione: e' il segnale che invalida cache di lookup e percorsi.
	 *
	 * Un bordo su cui non c'e' nessuna copertura non produce nulla e non tocca ne' hash ne' revisione.
	 */
	static TArray<FRTCoverDamageResult> ApplyStructureDamage(URTHexMapAsset* Map,
		const TArray<FRTStructureHit>& Hits);
};
