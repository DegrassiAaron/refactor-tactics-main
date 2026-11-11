#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTWorkbenchVariant.generated.h"

class URTActionData;

/**
 * La VARIANTE SPERIMENTALE dello Skill Workbench: *«questa abilita', ma con questo numero»*.
 *
 * ## Dove vive, e perche' non nel formato scenario
 *
 * Vive nel **contesto d'esecuzione** e non nel JSON ([`#1982`], strada C). `FRTScenarioVariant` sembrava il
 * posto ovvio e non lo e': quella struct e' un **canary di equita'** — `bExpectSameAcrossVariants` chiede
 * che le varianti producano *lo stesso TurnLog, byte per byte*, per dimostrare che un ingresso **non** ha
 * avuto effetto. La variante del Workbench serve l'opposto: dimostra che un ingresso **ha** effetto. Due
 * intenti contrari sotto lo stesso nome renderebbero il canary incapace di dire il vero.
 *
 * Il costo evitato non e' solo concettuale: il formato scenario e' **versionato**, con loader, writer,
 * validator e corpus golden appesi. Uno strumento `out_of_release_scope` non lo tocca per un dato che non
 * ha bisogno di sopravvivere alla run.
 *
 * ⚠️ E il requisito *«non modifica il dato di produzione»* diventa vero **per costruzione**: cio' che non
 * viene mai serializzato non puo' raggiungere la produzione nemmeno per errore.
 *
 * ## Perche' NON si chiama `FRTAbilityVariant`
 *
 * Quel nome e' occupato, in questo stesso dominio: `Ability/RTActionData.h` lo usa per la variante di
 * **loadout** — *«un compromesso ORIZZONTALE: cambia COME si usa l'abilita', non QUANTO e' forte»* — scelta
 * prima della partita e sostitutiva degli effetti. Un terzo significato su un nome che ne ha gia' due (il
 * terzo, contando `FRTScenarioVariant`) lo renderebbe illeggibile: e' la stessa misura con cui [D-154] ha
 * escluso `Candidate`.
 */

/** Esito dell'applicazione. Il fallimento e' esplicito e **non scrive niente**. */
UENUM(BlueprintType)
enum class ERTVariantApplyResult : uint8
{
	Ok,
	/** La variante non ha un `VariantId`: non sarebbe nominabile in un report, quindi non e' applicabile. */
	MissingVariantId,
	/** Un `ActionId` che il kit passato non contiene. */
	UnknownAction,
	/** Una `ParameterKey` che il modello non conosce. */
	UnknownParameter,
	/** `EffectIndex` fuori range, o che non punta a un effetto `Damage`. */
	InvalidEffectIndex
};

/** Un solo parametro sovrascritto. Solo interi: invariante #4 del catalogo. */
USTRUCT(BlueprintType)
struct FRTAbilityParameterOverride
{
	GENERATED_BODY()

	/** Quale azione. Deve esistere nel kit a cui la variante viene applicata. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Workbench")
	FName ActionId;

	/** Una delle chiavi di `RTActionParameterKeys` — le stesse che il readout di `#1953` espone. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Workbench")
	FName ParameterKey;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Workbench")
	int32 Value = 0;

	/**
	 * Quale effetto `Damage`, per `ParameterKey == Damage`. `INDEX_NONE` vale «il primo».
	 *
	 * Esiste perche' un'azione puo' dichiarare piu' di un effetto `Damage` e lo specchio ne proietta uno
	 * solo: senza l'indice, sovrascrivere «il danno» di un'azione a due colpi sarebbe ambiguo.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Workbench")
	int32 EffectIndex = INDEX_NONE;

	FRTAbilityParameterOverride() = default;
};

/** L'esperimento di UNA esecuzione: un id stabile e i numeri che cambia. */
USTRUCT(BlueprintType)
struct FRTWorkbenchVariant
{
	GENERATED_BODY()

	/**
	 * ID stabile. **Obbligatorio**: una variante che non si puo' nominare non si puo' leggere in un report,
	 * e un confronto baseline/variante in cui non si sa QUALE variante era non e' un confronto.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Workbench")
	FName VariantId;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Workbench")
	TArray<FRTAbilityParameterOverride> Overrides;

	/** Vuota = baseline. E' uno stato legittimo, non un errore: e' il lato «senza variante» del confronto. */
	bool IsBaseline() const { return Overrides.Num() == 0; }

	FRTWorkbenchVariant() = default;
};

UCLASS()
class REFACTORTACTICS_API URTWorkbenchVariantLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Applica la variante alle istanze passate, scrivendo in **entrambe le case**.
	 *
	 * ⚠️ **Entrambe, e non e' simmetria: e' la misura di `#1953`.** Lo stesso parametro e' letto da
	 * consumatori diversi in case diverse — il bot prende `Ability->RangeCells` e `Ability->Power` senza
	 * ternario, `ARTUnit::ConsumeAbility` prende `URTActionData::CooldownTurns`, e chi applica il ternario
	 * prende il `Def`. Scriverne una sola riprodurrebbe il *pulsante finto* di [D-090]: un attacco che il
	 * bot pianifica e che il resolver poi rifiuta.
	 *
	 * ⚠️ **Valida tutto PRIMA di scrivere qualsiasi cosa.** Un'applicazione parziale lascerebbe una mezza
	 * variante — alcuni numeri nuovi e altri vecchi — che e' peggio di nessuna: il designer leggerebbe un
	 * risultato attribuendolo a un esperimento che non e' quello che ha configurato.
	 *
	 * @param OutRestore  riempito con la variante INVERSA: applicarla annulla questa. E' il `Reset`, e vive
	 *                    qui invece che in uno snapshot separato perche' i valori precedenti li conosce solo
	 *                    chi li sta sovrascrivendo.
	 */
	static ERTVariantApplyResult Apply(const FRTWorkbenchVariant& Variant,
		const TArray<URTActionData*>& Abilities, FRTWorkbenchVariant& OutRestore);

	/** Come `Apply`, ma non scrive: risponde solo se scriverebbe. */
	static ERTVariantApplyResult Validate(const FRTWorkbenchVariant& Variant,
		const TArray<URTActionData*>& Abilities);
};
