#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Turn/Actions/RTDeclaredCondition.h"
#include "RTPlanEntry.generated.h"

class ARTUnit;

/**
 * La classe di una voce del piano: i campi a casella unica che il piano aveva prima della lista (E38 CP 38.12, #4113).
 * Le classi CHIUSE (scatto, principale, reazione, movimento) portano al piu' una voce, finche' il loro checkpoint non le
 * apre (CP 38.14–38.16); `Prep` e' aperta e ne porta quante il budget consente (`FRTUnitPlan::AddEntry`).
 */
UENUM()
enum class ERTPlanEntryClass : uint8
{
	Dash,
	Main,
	Reaction,
	Movement,
	/**
	 * Un'azione di Prep IN PIU' della principale (E38 CP 38.13, #4114; [D-584] (1): «due azioni di Prep sono legali se gli
	 * AP bastano»). La principale resta la voce `Main` e puo' essere a sua volta di Prep: tiene la prima azione armata,
	 * come prima della lista, cosi' ogni lettore della principale vede cio' che vedeva. Le voci `Prep` sono le altre.
	 *
	 * 🔑 **Solo azioni di Prep senza bersaglio**: niente strutture di bordo, niente colpi predittivi, che chiedono una cella o
	 * un bordo e oggi li dichiara solo la principale (`ARTUnit::DeclarePlannedAction`). In coda, come ogni valore: il
	 * piano e' serializzato con l'unita'.
	 */
	Prep,
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

	/**
	 * L'identita' della voce nel piano (E38 CP 38.14, #4115): assegnata quando la voce nasce, non cambia quando altre voci
	 * escono, e la copia al lock-in la conserva. Lega una voce del piano vivo alla sua copia, dove la posizione puo'
	 * essere diversa perche' il Prep ha tolto le sue voci. Non e' un dato dell'intento: `IsEmpty` non la guarda.
	 */
	UPROPERTY()
	int32 Id = INDEX_NONE;

	/**
	 * La mira congelata al lock-in di QUESTA voce principale ([D-415], [D-608] (4); E38 CP 38.14): la cella che il suo
	 * bersaglio-unita' aveva al lock-in. La scrive solo `ARTUnit::LockPlan`, nella copia; nel piano vivo resta al default.
	 * Con due attacchi su due bersagli, ognuno mira alla cella del proprio.
	 */
	UPROPERTY()
	FRTCellId FrozenAimCell;

	UPROPERTY()
	bool bHasFrozenAim = false;

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
