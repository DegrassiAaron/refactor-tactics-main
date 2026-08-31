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
