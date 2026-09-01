#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTCombatResolver.generated.h"

/** Stato di combattimento di un'unita' (HP e scudo). */
USTRUCT(BlueprintType)
struct FRTUnitCombatState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 Health = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 Shield = 0;

	/**
	 * Quota di `Shield` che scade nel Cleanup ([D-224]). Serve al resolver per sapere quanta protezione e'
	 * BASE, cioe' quanta ne deve saltare quando il danno viene dall'ambiente.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 TemporaryShield = 0;

	FRTUnitCombatState() = default;
	FRTUnitCombatState(int32 InHealth, int32 InShield, int32 InTemporaryShield = 0)
		: Health(InHealth), Shield(InShield), TemporaryShield(InTemporaryShield) {}
};

/**
 * GLI STADI che un valore d'abilita' attraversa fra il catalogo e gli HP — `#1951`.
 *
 * 🔑 **Non e' un ordine nuovo: e' il nome di quello che gia' esiste.** La catena e' orchestrata in un
 * punto solo (`ARTTurnManager`, la composizione di `ApplyAbsorptionPool(ApplyDamageDelta(ApplyFirstHitDelta(...)))`)
 * e ogni stadio e' motivato da una decisione accettata. Questo enum li rende **nominabili** perche' il
 * TurnLog possa registrarli; non decide niente.
 *
 * ⚠️ L'ordine dei valori E' l'ordine di applicazione, ed e' cio' che rende ordinabile un elenco
 * raccolto da punti diversi del codice.
 */
UENUM(BlueprintType)
enum class ERTDamageStage : uint8
{
	/** Il valore dichiarato dal catalogo. Intero, invariante #4. */
	Catalog,
	/** Bonus della cella di CHI TIRA (`EffectiveAttackPower`). */
	AttackerCell,
	/** Bonus condizionale di catalogo, p.es. `Wet` × `Hero.Gadget.LinearDischarge` (CP 8.2). */
	ConditionalBonus,
	/** Copertura, per-colpo e direzionale: dipende da dove sta chi SUBISCE (`D-206`). */
	Cover,
	/** Delta di primo colpo per bersaglio: `Status.Exposed` `+5`, `Action.Deflect` `-20`. Vale una volta. */
	FirstHitDelta,
	/** Delta su OGNI colpo: `Action.Brace` `-10`, senza il gate «una volta sola» (CP 5.2). */
	EveryHitDelta,
	/** Pool con eleggibilita' frontale: `Status.Guarded`, 15 (`D-292` + `D-206`). */
	AbsorptionPool,
	/** Somma per bersaglio sullo stato iniziale (invariante #3). */
	TargetSum,
	/** Temporaneo → base → HP: la base ferma SOLO il danno diretto (`D-224`). */
	ShieldAbsorption
};

/**
 * LE CINQUE OPERAZIONI REALI della catena, e non ce ne sono altre — `#1951`.
 *
 * ⛔ **Non esiste un solo moltiplicatore**, e i cinque tipi `Flat`/`Percent`/`Multiplier`/`Override`/`Clamp`
 * che un sistema di modificatori generico userebbe **non descrivono questo sistema**: la issue li mette
 * fuori scope per questo. Chi aggiunge un'operazione qui sta cambiando la catena, non il registro.
 */
UENUM(BlueprintType)
enum class ERTDamageOp : uint8
{
	/** Somma. */
	Add,
	/** Sottrazione con clamp a zero: la riduzione che avanza si perde. */
	SubtractClamped,
	/** Pool con avanzo: assorbe `Min(budget, colpo)` e il budget scala. */
	Pool,
	/** Somma per bersaglio. */
	Sum,
	/** Assorbimento a due strati (temporaneo, poi base). */
	TwoLayerAbsorb
};

/**
 * UNO STADIO REGISTRATO: cosa ha tolto chi, e quanto restava prima e dopo — `#1951`.
 *
 * 🔑 **`SourceId` non e' un identificatore nuovo**: e' la decisione o il tag che GIA' governa lo
 * stadio — `D-292`, `Status.Guarded`, `Action.Brace`, `D-224`. Inventarne uno proprio avrebbe creato un
 * secondo vocabolario da tenere allineato al primo.
 *
 * ⚠️ **`Before` e `After` sono interi come tutta la catena**, e la loro differenza non e' sempre
 * `Operand`: nello stadio con clamp la riduzione che avanza si perde, ed e' esattamente cio' che il
 * registro deve poter mostrare invece di far dedurre.
 */
USTRUCT(BlueprintType)
struct FRTDamageStageEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	ERTDamageStage Stage = ERTDamageStage::Catalog;

	/** La decisione o il tag che governa lo stadio, p.es. `D-292 · Status.Guarded`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	FName SourceId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	ERTDamageOp Operation = ERTDamageOp::Add;

	/** Quanto lo stadio ha mosso: sempre non negativo, il verso lo dice `Operation`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Operand = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 Before = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	int32 After = 0;

	FRTDamageStageEntry() = default;
	FRTDamageStageEntry(ERTDamageStage InStage, FName InSourceId, ERTDamageOp InOp,
		int32 InOperand, int32 InBefore, int32 InAfter)
		: Stage(InStage), SourceId(InSourceId), Operation(InOp)
		, Operand(InOperand), Before(InBefore), After(InAfter) {}
};

/** Gli stadi di UN bersaglio, in ordine di applicazione. `TMap` non puo' contenere un `TArray` nudo. */
USTRUCT(BlueprintType)
struct FRTDamageBreakdown
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	TArray<FRTDamageStageEntry> Stages;
};

/** Un attacco pianificato: colpisce l'unita' TargetIndex con Power danni. */
USTRUCT(BlueprintType)
struct FRTAttack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 TargetIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 Power = 0;

	/**
	 * CHI ha tirato ([D-212]). In coda, e con un default che vale «non lo so»: i costruttori esistenti
	 * continuano a compilare e chi non riempie il campo non finge di saperlo.
	 *
	 * Serve perche' una mitigazione DIREZIONALE non e' esprimibile senza: `ApplyDamageDelta` e la vecchia
	 * `ApplyFirstHitDelta` prendono un delta **per bersaglio**, e «questo colpo arriva dall'arco frontale» e'
	 * una proprieta' del COLPO. Senza questo campo il resolver puo' solo guardare il primo colpo dell'array e
	 * decidere per tutti — che e' il ripiego che [D-292] rimuove.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Combat")
	int32 AttackerIndex = INDEX_NONE;

	/**
	 * GLI STADI che hanno trasformato QUESTO colpo, in ordine di applicazione — `#1951`.
	 *
	 * 🔑 **Viaggia col colpo invece di stare in una mappa a parte**, e non e' comodita': gli stadi
	 * nascono in punti diversi — la copertura in `CollectHexAttacks`, il pool nel resolver — e una
	 * correlazione per indice si romperebbe alla prima riga che riordina o filtra gli attacchi.
	 *
	 * ⚠️ **Non entra in nessun hash e in nessuna serializzazione**, come il resto di `FRTAttack`:
	 * misurato, zero occorrenze della struttura in codice di hash, salvataggio o replay. E' un registro di
	 * lettura, e un registro che cambiasse un esito sarebbe un secondo calcolo.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Combat")
	TArray<FRTDamageStageEntry> Breakdown;

	FRTAttack() = default;
	FRTAttack(int32 InTarget, int32 InPower) : TargetIndex(InTarget), Power(InPower) {}
	FRTAttack(int32 InTarget, int32 InPower, int32 InAttacker)
		: TargetIndex(InTarget), Power(InPower), AttackerIndex(InAttacker) {}
};

/**
 * Risoluzione simultanea degli attacchi della fase Blast.
 * "Raccogli poi applica": i danni sono calcolati sullo stato iniziale (snapshot),
 * sommati per bersaglio e applicati insieme. Un'unita' colpita a morte infligge comunque
 * il proprio danno (il risultato non dipende dall'ordine degli attacchi).
 */
UCLASS()
class REFACTORTACTICS_API URTCombatResolver : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Ritorna il nuovo stato di ogni unita' (stesso ordine dell'input). */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Combat")
	static TArray<FRTUnitCombatState> ResolveAttacks(const TArray<FRTUnitCombatState>& Units, const TArray<FRTAttack>& Attacks);

	/**
	 * LO STESSO CALCOLO, che in piu' consegna il REGISTRO degli stadi per bersaglio — `#1951`.
	 *
	 * 🔑 **E' un overload e non una seconda funzione**, ed e' il vincolo che la issue pone: un
	 * breakdown ricostruito a parte sarebbe un secondo calcolo, e il giorno che divergesse dal primo
	 * spiegherebbe un danno che non e' avvenuto. Qui il registro esce dal codice che il danno lo fa.
	 *
	 * L'elenco per bersaglio e' la concatenazione degli stadi dei suoi colpi — nell'ordine in cui i colpi
	 * stanno in `Attacks` — seguita da `TargetSum` e `ShieldAbsorption`. L'ultimo `After` e' il danno
	 * effettivo, cioe' l'`Amount` che il TurnLog registra.
	 *
	 * ⚠️ **L'ordine non passa da una `TMap`**: l'iterazione di una mappa non e' garantita, e un elenco
	 * che cambia ordine fra due esecuzioni non e' verificabile — lo dice il vincolo tecnico della issue.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Combat")
	static TArray<FRTUnitCombatState> ResolveAttacksWithBreakdown(const TArray<FRTUnitCombatState>& Units,
		const TArray<FRTAttack>& Attacks, TMap<int32, FRTDamageBreakdown>& OutByTarget);

	/**
	 * Applica i modificatori che valgono UNA VOLTA SOLA per bersaglio: oggi `Status.Exposed` (+5 al primo
	 * danno diretto ricevuto), domani `Guard` (-15, CP 4.4). `DeltaByTarget[i]` e' il delta dell'unita' `i`
	 * (0 = nessuno); un indice fuori dall'array vale 0.
	 *
	 * Il delta va al PRIMO colpo che quel bersaglio riceve nell'ordine dato. Poiche' `ResolveAttacks` somma i
	 * danni per bersaglio, il totale non dipende da quale colpo se lo prenda: la regola «primo danno» resta
	 * ordine-indipendente (invariante #3) invece di diventare una corsa fra attaccanti.
	 *
	 * Il danno di un colpo non scende sotto 0: un delta negativo puo' annullarlo, non curare.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static TArray<FRTAttack> ApplyFirstHitDelta(const TArray<FRTAttack>& Attacks, const TArray<int32>& DeltaByTarget);

	/**
	 * Applica i modificatori che valgono su OGNI colpo ricevuto da un bersaglio: oggi `Action.Brace` (-10 a
	 * tutti i danni diretti fino al Cleanup, CP 5.2). Stessa forma di `ApplyFirstHitDelta`, senza il gate
	 * "una volta sola" — ed e' proprio quel gate a rendere le due funzioni non intercambiabili: `Brace`
	 * applicato con `ApplyFirstHitDelta` proteggerebbe da un colpo e lascerebbe passare gli altri interi.
	 *
	 * Il danno di un colpo non scende sotto 0: un delta negativo puo' annullarlo, non curare. Un colpo
	 * azzerato resta comunque una voce nell'array — cioe' un attacco AVVENUTO, non cancellato.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static TArray<FRTAttack> ApplyDamageDelta(const TArray<FRTAttack>& Attacks, const TArray<int32>& DeltaByTarget);

	/**
	 * Assorbimento a POOL ([D-292]): ogni bersaglio ha un budget di danno che i colpi consumano finche' dura,
	 * e cio' che avanza NON si perde.
	 *
	 * E' la differenza che conta rispetto a `ApplyFirstHitDelta`, e non e' di sfumatura: quella sceglie **un**
	 * colpo e clampa a zero, quindi con un delta negativo piu' grande del colpo che lo riceve la riduzione che
	 * avanza sparisce — e quanta ne sparisca dipende da QUALE colpo era primo. Misurato: un bersaglio in
	 * Guardia colpito da 10 e da 30 incassava **30** o **25** a seconda dell'ordine dell'array
	 * (`Combat.NegativeFirstHitDeltaIsPermutationInvariant`). Il pool consuma sempre lo stesso totale, quindi
	 * la somma torna **commutativa per costruzione**: non serve nessuna regola su chi viene prima.
	 *
	 * `bEligible` e' PARALLELO a `Attacks` e dice quali colpi possono attingere al pool. E' il canale della
	 * direzionalita' di [D-206] — la Guardia copre il davanti, l'emisfero posteriore resta scoperto — e sta
	 * qui invece che dentro questa funzione perche' la geometria ha un owner solo
	 * (`URTHexCombatLibrary::IsInFrontalArc`) e il resolver resta puro: nessun Actor, nessuna mappa.
	 *
	 * Un colpo non eleggibile passa INTERO e non consuma nulla: il budget resta per chi arriva dal davanti.
	 *
	 * @param PoolByTarget  budget per bersaglio, indicizzato come `TargetIndex`. Valori <= 0 non assorbono.
	 * @param bEligible     parallelo ad `Attacks`. Se piu' corto, i colpi oltre la fine NON sono eleggibili.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Combat")
	static TArray<FRTAttack> ApplyAbsorptionPool(const TArray<FRTAttack>& Attacks,
		const TArray<int32>& PoolByTarget, const TArray<bool>& bEligible);
};
