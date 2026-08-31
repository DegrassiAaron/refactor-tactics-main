#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTActionReadout.generated.h"

class URTActionData;

/**
 * Il READOUT numerico di un'azione: quali parametri ha, quanto valgono, e **da quale casa vengono**.
 *
 * ## Perche' non basta "il valore"
 *
 * Un parametro d'azione puo' vivere in **tre** posti, e la scoperta e' di `#1953`:
 *
 *   1. `FRTActionDef` — il catalogo;
 *   2. i campi SPECCHIO di `URTActionData` (`RangeCells`, `Power`, `CooldownTurns`, `bSelfTarget`), eredita'
 *      dell'MVP quadrato, che `ARTTurnManager` e il bot leggono ancora;
 *   3. `Def.Effects[]` — dove vive il DANNO, che nel `Def` **non ha un campo**.
 *
 * E la regola che decide quale vince — `Def.ActionId.IsNone() ? specchio : Def` — e' ricopiata a mano in tre
 * punti (`RTPlayerController.cpp:1376`, `RTTurnManager.cpp:1124`, `:3725`) e **non e' applicata da tutti**:
 * il bot pianifica su `Ability->RangeCells` e `Ability->Power` senza ternario (`RTTurnManager.cpp:1271`,
 * `:1329`), e `ConsumeAbility` legge `URTActionData::CooldownTurns`.
 *
 * ∴ **non esiste "il valore effettivo"**: esiste cio' che il catalogo dichiara e cio' che un dato consumatore
 * legge. Questo readout espone entrambi invece di sceglierne uno, perche' uno strumento che scegliesse
 * mostrerebbe al designer un numero che il gioco potrebbe non usare — ed e' precisamente la divergenza
 * editor/runtime che lo Skill Workbench esiste per impedire (`#1950`, ADR-0010).
 *
 * ## Cosa questo readout NON fa
 *
 * Non calcola, non risolve, non sceglie. Non conosce le varianti di loadout (`FRTAbilityVariant`), i cui
 * `Effects` sostituiscono per intero quelli del `Def` quando una e' selezionata: quello e' un readout
 * DIVERSO e appartiene al contract della variante.
 */

/** Dove il parametro e' MEMORIZZATO. Risponde a «da dove viene», non a «chi decide». */
UENUM(BlueprintType)
enum class ERTParameterStorageHome : uint8
{
	/** `FRTActionDef`: il catalogo. */
	CatalogDef,
	/** I campi legacy di `URTActionData`, che alcuni consumatori leggono ancora. */
	LegacyMirror,
	/** `FRTActionDef::Effects[]`: e' la casa del danno, che nel `Def` non ha un campo proprio. */
	EffectSpec
};

/**
 * Chi DECIDE il valore. E' una domanda diversa da `ERTParameterStorageHome`, e tenerle separate e' il punto:
 * per `RangeCells` la risposta onesta e' «dipende da chi legge», e un campo solo l'avrebbe nascosta.
 */
UENUM(BlueprintType)
enum class ERTParameterAuthority : uint8
{
	/** Il catalogo vince per ogni consumatore. */
	CatalogWins,
	/** Lo specchio vince per ogni consumatore: e' il caso della ricarica, che `ConsumeAbility` legge di li'. */
	MirrorWins,
	/**
	 * Dipende dal consumatore, e non e' un'ambiguita' da risolvere qui: e' un FATTO del runtime corrente.
	 * Chi applica il ternario legge il `Def`, il bot legge lo specchio. Finche' le due case concordano la
	 * differenza non si vede — ed e' il guardiano `Actions.HeroKitsMatchTheirCatalogDef` (`#1963`) a rendere
	 * quella concordanza **misurata** invece che assunta.
	 */
	DependsOnConsumer
};

/** Esito della lettura. Un elenco vuoto NON e' un segnale: e' indistinguibile da «azione senza parametri». */
UENUM(BlueprintType)
enum class ERTActionReadoutResult : uint8
{
	/** Letto. L'elenco puo' comunque essere corto: un'azione senza danno non ha voci di danno. */
	Ok,
	/** Riferimento nullo, o azione che il catalogo non conosce. E' un difetto dello STRUMENTO, non del dato. */
	UnknownAction
};

/**
 * Un parametro numerico di un'azione, con la sua provenienza.
 *
 * `BlueprintType` perche' e' un DTO destinato al tooling — e per la stessa ragione **non** vive in
 * `RTTestScenario.h`: dopo `#1631` il gate `Scenario.AuthoringContractIsReachableFromBlueprint` enumera per
 * riflessione le struct di quell'header e pretende che non siano esposte. Un DTO li' dentro sarebbe rosso.
 */
USTRUCT(BlueprintType)
struct FRTActionParameterView
{
	GENERATED_BODY()

	/** Chiave stabile (`Action.RangeCells`, `Action.CooldownTurns`, `Action.Damage`). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Ability")
	FName ParameterKey;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Ability")
	FText DisplayName;

	/** Cio' che il CATALOGO dichiara. Solo interi: invariante #4, verificata da `Catalog.NoFloatInIntegerFields`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 DeclaredValue = 0;

	/** Cio' che il consumatore reale LEGGE oggi. Coincide con `DeclaredValue` quando le case concordano. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 ConsumedValue = 0;

	/**
	 * Se le due case rappresentano lo stesso valore.
	 *
	 * ⚠️ **Falso non significa "difetto"**: per il secondo effetto `Damage` di un'azione lo specchio non ha
	 * proprio una rappresentazione — `URTActionData::Power` proietta il PRIMO e si ferma (`break`). Li'
	 * `false` dice «lo specchio non porta questo numero», che e' esattamente cio' che il designer deve sapere.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Ability")
	bool bHomesAgree = true;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Ability")
	ERTParameterStorageHome StorageHome = ERTParameterStorageHome::CatalogDef;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Ability")
	ERTParameterAuthority AuthorityRule = ERTParameterAuthority::CatalogWins;

	/**
	 * Indice dentro `Def.Effects` per i parametri con `StorageHome == EffectSpec`; `INDEX_NONE` altrimenti.
	 *
	 * Serve perche' un'azione puo' dichiarare **piu' di un** effetto `Damage` — `Hero.Gadget.LinearDischarge`
	 * nella variante ramificata ne ha due da 18 — e senza l'indice due voci identiche sarebbero
	 * indistinguibili.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Ability")
	int32 EffectIndex = INDEX_NONE;

	FRTActionParameterView() = default;
};

UCLASS()
class REFACTORTACTICS_API URTActionReadoutLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * I parametri numerici di `Action`, in ordine stabile: portata, ricarica, poi un'entrata per **ogni**
	 * effetto `Damage` dichiarato, nell'ordine in cui il catalogo li elenca.
	 *
	 * L'ordine e' quello, e non alfabetico: e' l'ordine in cui il catalogo li scrive, quindi due letture
	 * della stessa azione danno la stessa sequenza senza dipendere da un `Sort` su `FName`, che varia con
	 * l'ordine di creazione dei nomi.
	 *
	 * `OutParameters` viene SVUOTATO all'ingresso: un elenco che si accoda a quello di prima farebbe passare
	 * per parametri di questa azione quelli della precedente.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Ability")
	static ERTActionReadoutResult DescribeActionParameters(const URTActionData* Action,
		TArray<FRTActionParameterView>& OutParameters);
};
