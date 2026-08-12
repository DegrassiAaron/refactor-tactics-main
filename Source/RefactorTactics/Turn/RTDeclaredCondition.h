#pragma once

#include "CoreMinimal.h"
#include "RTDeclaredCondition.generated.h"

/**
 * Una condizione dichiarata in PIANIFICAZIONE, valutata al trigger come funzione pura ([D-012], [D-109]).
 *
 * Non e' un regime a parte: **riduce** le risposte legali di una opportunity. Se dopo la riduzione ne resta
 * una sola, il commit e' immediato e nessuna finestra si apre — ed e' cosi' che il regime *Conditional*
 * emerge dai dati invece che da un enum di policy parallelo.
 *
 * `Id` vuoto (`NAME_None`) significa **nessuna condizione**: si risponde a ogni trigger, che e' il
 * comportamento di sempre. La v0.1 ammette un solo `Id`, e l'elenco vive nel codice —
 * `URTReactionOpportunityLibrary::IsDeclaredConditionAllowed` e' il validator.
 *
 * **Sta in un header proprio, e non accanto al modello dell'opportunity, per una ragione misurata**: il campo
 * vive su `ARTUnit`, che e' il tipo piu' incluso del progetto. Tenere il tipo in
 * `RTReactionOpportunityTypes.h` costringeva `RTUnit.h` a trascinarsi dietro la geometria di soppressione e
 * la percezione — due sottosistemi che un'unita' non ha bisogno di conoscere per portare due campi.
 */
USTRUCT()
struct FRTDeclaredCondition
{
	GENERATED_BODY()

	/** Quale condizione. `NAME_None` = nessuna dichiarata. */
	UPROPERTY()
	FName Id;

	/**
	 * Il suo parametro. Per la soglia di salute e' una percentuale **intera**: il confronto si fa in
	 * aritmetica intera (`Health * 100 <= MaxHealth * Param`), perche' il gate `G7` della v0.1 vieta i float
	 * in costi, priorita' e danni e una soglia in virgola mobile li farebbe rientrare dalla finestra.
	 */
	UPROPERTY()
	int32 Param = 0;

	FRTDeclaredCondition() = default;
	FRTDeclaredCondition(FName InId, int32 InParam) : Id(InId), Param(InParam) {}

	bool IsDeclared() const { return !Id.IsNone(); }
};
