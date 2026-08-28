#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"                   // UEnum::GetNameStringByValue
#include "UObject/ReflectedTypeAccessors.h" // StaticEnum: e' qui che vive, non in Class.h

/**
 * IL NOME DI UN VALORE D'ENUM, dichiarato una volta invece che due.
 *
 * ## Il duplicato che questo header chiude
 *
 * `EnumName` esisteva in `Turn/RTTurnLogLibrary.cpp` e in `Debug/RTDebugReportLibrary.cpp`, entrambe come
 * `template <typename TEnum> FString EnumName(TEnum)` in un namespace ANONIMO. UE compila piu' `.cpp` in
 * un'unica translation unit, e i namespace anonimi di quei file diventano lo stesso namespace: due omonimi
 * sono una ridefinizione, e la compilazione muore appena il raggruppamento li mette insieme (#1548, stessa
 * famiglia di #1530). Un `inline`/`template` in un namespace NOMINATO scioglie il vincolo, perche' e' la
 * stessa entita' e non due omonime.
 *
 * ## Le due versioni NON erano equivalenti, e questa e' quella giusta
 *
 * Quella di `RTTurnLogLibrary` ripiega sul numero quando il **nome** e' vuoto; quella di
 * `RTDebugReportLibrary` solo quando `StaticEnum` e' **nullo**. La differenza si vede con un valore fuori
 * enum — un cast di un intero che nessun enumeratore copre: `GetNameStringByValue` torna stringa vuota, e la
 * seconda versione la restituiva tale e quale. Un campo che si svuota invece di mostrare cio' che contiene e'
 * la peggiore delle due risposte: sparisce dal report senza dire che c'era qualcosa.
 *
 * Vince quindi la semantica gia' documentata in `RTTurnLogLibrary`, e per la sua stessa ragione:
 *
 *     «NON uno switch scritto a mano: sarebbe un secondo elenco degli stessi valori [...] Con la reflection
 *      un valore fuori enum si mostra GREZZO, che e' l'unica risposta onesta: stesso criterio, e stesse
 *      parole, di `URTScenarioLoader::DescribeLogEvent`.»
 *
 * ⚠️ **Serve un `UENUM()`.** Senza reflection `StaticEnum<TEnum>()` e' nullo e si ottiene il numero: e'
 * degradazione voluta, non un errore da diagnosticare.
 */
namespace RTReflection
{
	/**
	 * Il nome dell'enumeratore, o il valore grezzo se l'enum non e' riflesso o il valore non e' coperto.
	 *
	 * ⚠️ Il nome torna **senza** il prefisso del tipo (`GetNameStringByValue` da' `Move`, non
	 * `ERTResolutionPhase::Move`): chi lo scrive in un report o in un log lo qualifica se serve.
	 */
	template <typename TEnum>
	FString EnumName(TEnum Value)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		const FString Name = Enum ? Enum->GetNameStringByValue(static_cast<int64>(Value)) : FString();
		return Name.IsEmpty() ? FString::FromInt(static_cast<int32>(Value)) : Name;
	}
}
