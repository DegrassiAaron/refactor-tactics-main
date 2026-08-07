#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionData.h"
#include "Combat/RTCombatResolver.h"
#include "Map/RTCellId.h"
#include "RTHexCombatLibrary.generated.h"

class URTHexMapAsset;

/**
 * Stato minimo di un'unita' per la risoluzione del Blast esagonale. Come FRTHexSimUnit per il movimento,
 * l'identita' e' un INTERO STABILE (UnitId = indice nell'array), mai un pointer: e' la disciplina del
 * TurnLog (determinismo/replay) e permette di testare il combat senza Actor ne' UWorld.
 */
USTRUCT(BlueprintType)
struct FRTHexCombatUnit
{
	GENERATED_BODY()

	/** Identita' stabile nell'array delle unita' del turno. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	int32 UnitId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	int32 TeamId = 0;

	/** Posizione autorevole nello snapshot del Blast (pre-movimento, post-Dash). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	FRTCellId Cell;

	/** Le unita' non vive non attaccano e non vengono colpite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	bool bAlive = true;

	FRTHexCombatUnit() = default;
};

/**
 * Intento d'attacco gia' validato dal chiamante per quanto riguarda l'abilita' (esiste, non e' uno scatto,
 * e' utilizzabile). La libreria valida solo la GEOMETRIA: portata, linea di tiro, celle colpite.
 * Power e' il danno EFFETTIVO (bonus della cella dell'attaccante gia' applicato dal chiamante).
 */
USTRUCT(BlueprintType)
struct FRTHexAttackIntent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	int32 AttackerId = INDEX_NONE;

	/**
	 * Unita' bersaglio, oppure `INDEX_NONE` per mirare a una CELLA: e' il caso di `Fallback.AttackCell`, dove
	 * il bersaglio e' sparito ma l'area parte lo stesso dove era stata puntata. Con `INDEX_NONE` il chiamante
	 * DEVE valorizzare `TargetCell`: portata, linea di tiro e forma si calcolano su quella.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	int32 TargetId = INDEX_NONE;

	/** Cella mirata quando non c'e' un'unita' bersaglio (`TargetId == INDEX_NONE`); ignorata altrimenti. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	FRTCellId TargetCell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	ERTAbilityShape Shape = ERTAbilityShape::Single;

	/** Portata in celle (distanza esagonale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	int32 RangeCells = 0;

	/** Raggio dell'area attorno al bersaglio (solo con Shape = Area). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	int32 AreaRadius = 0;

	/** Danno effettivo del colpo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	int32 Power = 0;

	/**
	 * L'area colpisce anche gli ALLEATI dell'attaccante, se stanno nelle celle interessate. Si COPIA da
	 * `FRTActionDef::bFriendlyFire`, che e' dove l'azione lo dichiara: qui e' il parametro dell'intento, non
	 * la sede della decisione.
	 *
	 * Non colpisce mai chi la usa: il catalogo non lo prevede, e un'area che uccide chi la lancia sarebbe una
	 * regola nuova, non un caso di questa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexCombat")
	bool bFriendlyFire = false;

	FRTHexAttackIntent() = default;
};

/** Un colpo a segno: chi colpisce chi, con quanta potenza, generato da quale intento. */
USTRUCT(BlueprintType)
struct FRTHexAttackHit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexCombat")
	int32 AttackerId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexCombat")
	int32 TargetId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexCombat")
	int32 Power = 0;

	/** Indice dell'intento che ha prodotto il colpo (per risalire ad abilita'/status/knockback). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexCombat")
	int32 IntentIndex = INDEX_NONE;

	FRTHexAttackHit() = default;
	FRTHexAttackHit(int32 InAttacker, int32 InTarget, int32 InPower, int32 InIntent)
		: AttackerId(InAttacker), TargetId(InTarget), Power(InPower), IntentIndex(InIntent) {}
};

/**
 * Esito della RACCOLTA del Blast (prima meta' di "raccogli poi applica"): i colpi a segno in ordine
 * CANONICO (attaccante, bersaglio) e gli intenti scartati per linea di tiro. Non applica nulla:
 * l'applicazione e' di URTCombatResolver::ResolveAttacks, che somma per bersaglio sullo stato iniziale.
 */
USTRUCT(BlueprintType)
struct FRTHexBlastPlan
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexCombat")
	TArray<FRTHexAttackHit> Hits;

	/** Indici degli intenti scartati per linea di tiro bloccata (reason code NoLineOfSight nel TurnLog). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexCombat")
	TArray<int32> BlockedIntents;

	/**
	 * Indici degli intenti che non e' stato possibile VALUTARE (mappa autorevole assente). Tenuti separati da
	 * `BlockedIntents` perche' non sono un esito di gioco ma un difetto di configurazione del livello: finirci
	 * dentro farebbe scrivere «nessuna linea di tiro» nel TurnLog dove non c'e' nessuna copertura.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HexCombat")
	TArray<int32> UnverifiableIntents;
};

/**
 * Raccolta degli attacchi della fase Blast su griglia ESAGONALE: pura, deterministica, senza Actor.
 * Le forme si risolvono sulle primitive di URTHexLibrary (HexLine/HexCone/HexArea) e la linea di tiro
 * su URTHexVisionLibrary; il danno resta a URTCombatLibrary/URTCombatResolver (combat math invariata).
 */
UCLASS()
class REFACTORTACTICS_API URTHexCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Celle colpite da un'abilita' secondo la sua forma, su griglia esagonale:
	 * - `Single` -> la sola cella del bersaglio;
	 * - `Area`   -> esagono pieno di raggio AreaRadius attorno al bersaglio (HexArea);
	 * - `Line`   -> celle attraversate da From a Target, **cella dell'attaccante esclusa** (HexLine);
	 * - `Cone`   -> ventaglio di 120 gradi verso il bersaglio, profondo RangeCells (HexCone).
	 * Output senza duplicati e ordinato (URTHexLibrary::StableLess): l'ordine non dipende dall'input.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexCombat")
	static TArray<FRTCellId> HexHitCells(ERTAbilityShape Shape, const FRTCellId& From, const FRTCellId& Target,
		int32 RangeCells, int32 AreaRadius);

	/**
	 * Raccoglie i colpi a segno degli intenti del turno.
	 *
	 * Scarta l'intento se: id non validi, attaccante o bersaglio non vivi, stessa squadra, bersaglio oltre
	 * `RangeCells` (distanza esagonale). Se il bersaglio e' in portata ma la linea di tiro e' bloccata,
	 * l'indice dell'intento finisce in `BlockedIntents` (il chiamante ne fa un reason code NoLineOfSight).
	 *
	 * **FAIL-CLOSED**: `Map == nullptr` -> nessun colpo e intento in `BlockedIntents`. Senza mappa autorevole
	 * la linea di tiro non e' valutabile, quindi non si colpisce (stessa scelta di CanTargetHexCell).
	 *
	 * Ordine-indipendente: permutare `Intents` non cambia il piano prodotto (ordine canonico in uscita).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexCombat")
	static FRTHexBlastPlan CollectHexAttacks(const TArray<FRTHexCombatUnit>& Units,
		const TArray<FRTHexAttackIntent>& Intents, const URTHexMapAsset* Map);

	/** Converte il piano nella forma attesa da URTCombatResolver::ResolveAttacks (indice bersaglio + danno). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexCombat")
	static TArray<FRTAttack> ToAttacks(const FRTHexBlastPlan& Plan);

	/**
	 * Cella finale del bersaglio RESPINTO di `Distance` celle, su griglia esagonale.
	 *
	 * La direzione e' quella dell'ULTIMO passo della linea attaccante -> bersaglio: una delle sei direzioni
	 * esagonali, sempre una cella adiacente (il quadrato sceglieva l'asse col delta maggiore, che su esagoni
	 * non esiste). La spinta si ferma sulla cella libera prima di: cella assente dalla mappa (bordo), cella
	 * che blocca il movimento, cella occupata (`Occupied`). Il Layer del bersaglio e' preservato: i piani si
	 * collegano con archi espliciti, non con la spinta.
	 *
	 * Nessuna spinta se `Distance <= 0`, se attaccante e bersaglio condividono la cella assiale (nessuna
	 * direzione) o se `Map == nullptr` (**fail-closed**: senza mappa autorevole non si sposta nessuno).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexCombat")
	static FRTCellId HexKnockbackDestination(const FRTCellId& Attacker, const FRTCellId& Target, int32 Distance,
		const URTHexMapAsset* Map, const TArray<FRTCellId>& Occupied);

	/**
	 * Cella finale del bersaglio TIRATO verso chi lo tira, di `Distance` celle (`Action.Pull`, catalogo v0.1
	 * §5). Simmetrica a `HexKnockbackDestination`: stessa direzione (la linea Puller->Target), ma INVERTITA —
	 * il bersaglio si avvicina invece di allontanarsi. Si ferma sulla cella libera prima di: bordo mappa,
	 * cella che blocca il movimento, cella occupata — la cella di chi tira e' SEMPRE fra le occupate (e' li'
	 * un'unita' viva), quindi la trazione non finisce mai addosso a lui.
	 *
	 * Nessuna trazione se `Distance <= 0`, se le due unita' condividono la cella assiale, o se `Map ==
	 * nullptr` (fail-closed).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HexCombat")
	static FRTCellId HexPullDestination(const FRTCellId& Puller, const FRTCellId& Target, int32 Distance,
		const URTHexMapAsset* Map, const TArray<FRTCellId>& Occupied);
};
