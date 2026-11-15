#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Turn/Actions/RTDeclaredCondition.h"
#include "RTPlanEntry.generated.h"

class ARTUnit;

/**
 * La classe di una voce del piano: i campi a casella unica che il piano aveva prima della lista (E38 CP 38.12, #4113).
 * Finche' il resolver legge una voce per classe, il piano ne porta al piu' una per classe (`FRTUnitPlan::AddEntry`).
 */
UENUM()
enum class ERTPlanEntryClass : uint8
{
	Dash,
	Main,
	Reaction,
	Movement,
};

/**
 * UNA VOCE DEL PIANO: l'abilita' e i parametri della sua classe (E38 CP 38.12, #4113; [D-608] (4) per l'elenco
 * dell'intento). La posizione nella lista di `FRTUnitPlan` e' il suo ordine di dichiarazione.
 *
 * I campi sono quelli che stavano in `FRTUnitPlan` a casella unica, con la stessa forma e gli stessi default: la loro
 * documentazione sta sugli accessori di `FRTUnitPlan`, che restano i punti in cui il gioco li legge. Ogni voce porta
 * solo i campi della sua classe; gli altri restano al default.
 *
 * ⛔ La mira congelata al lock-in non e' un parametro dell'intento: sta nella copia al lock-in, `FRTLockedPlan`
 * ([D-608] (4), tappa 4).
 */
USTRUCT(meta = (RTServerOnly))
struct REFACTORTACTICS_API FRTPlanEntry
{
	GENERATED_BODY()

	UPROPERTY()
	ERTPlanEntryClass Class = ERTPlanEntryClass::Main;

	/** L'abilita' nel kit (scatto, principale, reazione); `INDEX_NONE` per il movimento o se non e' armata. */
	UPROPERTY()
	int32 AbilityIndex = INDEX_NONE;

	// Principale.
	UPROPERTY()
	TObjectPtr<ARTUnit> AttackTarget = nullptr;
	UPROPERTY()
	FRTCellId AttackCell;
	UPROPERTY()
	bool bAttackTargetsCell = false;
	UPROPERTY()
	ERTHexDirection CoverEdge = ERTHexDirection::E;
	UPROPERTY()
	bool bHasCoverEdge = false;

	// Scatto.
	UPROPERTY()
	FRTCellId DashCell;

	// Reazione.
	UPROPERTY()
	FRTDeclaredCondition ReactionCondition;

	// Movimento.
	UPROPERTY()
	TArray<FRTCellId> Path;
	UPROPERTY()
	TArray<FRTCellId> Waypoints;
	UPROPERTY()
	FName MovementProfileId;

	/** Vero se ogni campo e' al suo default: una voce cosi' non dichiara niente, e il piano non la tiene. */
	bool IsEmpty() const
	{
		return AbilityIndex == INDEX_NONE && AttackTarget == nullptr && AttackCell == FRTCellId() && !bAttackTargetsCell
			&& CoverEdge == ERTHexDirection::E && !bHasCoverEdge && DashCell == FRTCellId()
			&& !ReactionCondition.IsDeclared() && ReactionCondition.Param == 0
			&& Path.IsEmpty() && Waypoints.IsEmpty() && MovementProfileId.IsNone();
	}
};
