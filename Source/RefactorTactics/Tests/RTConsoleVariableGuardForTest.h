// Guardia RAII parametrica per le console variable dei test (#2235).
//
// 🔴 **Esiste perche' i guard scritti a mano perdevano in silenzio.** Ogni copia faceva
// `CVar->Set(Valore, ECVF_SetByCode)`, e `ECVF_SetByCode` (`0x0E000000`) sta **sotto**
// `ECVF_SetByConsole` (`0x10000000`): se la variabile era stata impostata digitandola nella console
// dell'editor, ogni `Set` del guard veniva **rifiutato senza un errore e senza una riga di log**. Il
// guard non riusciva nemmeno a RIPRISTINARE, quindi il valore digitato colava nei test successivi — e
// li' il rosso non si collega piu' a chi l'ha causato.
//
// 🔑 **La scrittura usa `SetWithCurrentPriority`, non una priorita' piu' alta.** Alzare a
// `ECVF_SetByConsole` avrebbe fatto vincere il guard, ma avrebbe anche alzato **permanentemente** il
// pavimento della variabile: da quel momento un `Set(..., ECVF_SetByCode)` del codice di produzione
// sarebbe stato ignorato, cioe' lo stesso difetto spostato di un passo. `SetWithCurrentPriority`
// riscrive il valore **conservando la priorita' corrente**: vince su chiunque l'abbia impostata, e non
// cambia chi la tiene.
//
// ⚠️ **E si RILEGGE comunque.** `SetWithCurrentPriority` non copre tutto: una variabile `ECVF_ReadOnly`,
// o un valore che il parser rifiuta, restano possibili. L'idioma non e' nuovo in questo repository —
// `RTHexMapActorTests.cpp` lo applica gia' a mano su `r.InstancedStaticMeshes.ForceRemoveAtSwap`, e la
// ragione scritta li' vale identica qui: *«senza questo controllo le due meta' del test costruirebbero
// la STESSA board e l'asserzione finale passerebbe a vuoto»*. Un guard che non prende e non lo dice
// trasforma un test in un rituale.
//
// ⛔ **Namespace NOMINATO e funzioni `inline`**, come `RTWidgetAssetTestHelpers.h` e per le stesse due
// ragioni: una copia per unita' di traduzione e' codice duplicato nel binario, e un helper anonimo in un
// header e' una trappola ODR. Una funzione **non** `inline` qui sarebbe invece la collisione vera, ed e'
// il limite 3 dell'oracolo di `RTTestGuardTests.cpp` — gli header non sono guardati da quel gate.
//
// ⚠️ **Nessuna guardia `WITH_DEV_AUTOMATION_TESTS` attorno a questo file**, ed e' deliberato: i `.cpp` lo
// includono FUORI dalla propria, quindi deve compilare in ogni target. Regge perche' sia
// `FAutomationTestFramework` sia `FAutomationTestBase` sono dichiarati **prima** dell'unico
// `#if WITH_AUTOMATION_TESTS` di `Misc/AutomationTest.h` (rispettivamente :947 e :1594, contro :4086 —
// UE 5.8). E' una proprieta' dell'Engine, non una scelta di questo repository: se un aggiornamento la
// spostasse, questo header romperebbe la Shipping.

#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"

namespace RTTestConsoleVariable
{
	inline void Applica(IConsoleVariable& Var, int32 Valore) { Var.SetWithCurrentPriority(Valore); }
	inline void Applica(IConsoleVariable& Var, float Valore) { Var.SetWithCurrentPriority(Valore); }
	inline void Applica(IConsoleVariable& Var, const TCHAR* Valore) { Var.SetWithCurrentPriority(Valore); }
	inline void Applica(IConsoleVariable& Var, const FString& Valore) { Var.SetWithCurrentPriority(*Valore); }

	inline bool Coincide(const IConsoleVariable& Var, int32 Atteso) { return Var.GetInt() == Atteso; }
	inline bool Coincide(const IConsoleVariable& Var, const TCHAR* Atteso) { return Var.GetString() == Atteso; }
	inline bool Coincide(const IConsoleVariable& Var, const FString& Atteso) { return Var.GetString() == Atteso; }

	/**
	 * ⚠️ **Il confronto sul float e' a tolleranza, e non e' pigrizia**: `IConsoleVariable::Set` formatta il
	 * valore con `%g` prima di riparsarlo (`IConsoleManager.h`, *«inefficient but no common code path»*),
	 * quindi il giro d'andata e ritorno conserva sei cifre significative e non i bit.
	 */
	inline bool Coincide(const IConsoleVariable& Var, float Atteso)
	{
		return FMath::IsNearlyEqual(Var.GetFloat(), Atteso, UE_KINDA_SMALL_NUMBER);
	}

	inline FString Mostra(int32 Valore) { return FString::FromInt(Valore); }
	inline FString Mostra(float Valore) { return FString::SanitizeFloat(Valore); }
	inline FString Mostra(const TCHAR* Valore) { return FString::Printf(TEXT("\"%s\""), Valore); }
	inline FString Mostra(const FString& Valore) { return FString::Printf(TEXT("\"%s\""), *Valore); }

	template <typename T> T Leggi(const IConsoleVariable& Var);
	template <> inline int32 Leggi<int32>(const IConsoleVariable& Var) { return Var.GetInt(); }
	template <> inline float Leggi<float>(const IConsoleVariable& Var) { return Var.GetFloat(); }
	template <> inline FString Leggi<FString>(const IConsoleVariable& Var) { return Var.GetString(); }

	/**
	 * Scrive, **rilegge** e riporta al test in corso se la scrittura non ha preso.
	 *
	 * 🔑 **Riporta senza che il chiamante passi il test**: `FAutomationTestFramework::Get().GetCurrentTest()`
	 * restituisce il test attivo, ed e' lo stesso canale che l'Engine usa per i propri errori differiti
	 * (`AutomationTest.h:4838`). Serve perche' i guard di questo repository vivono dentro fixture con
	 * costruttore di default — `FRTScopedAutobattleCVars`, `FScopedCVars` — dove un `FAutomationTestBase&`
	 * andrebbe filato attraverso ogni chiamante. Un fallimento silenzioso e' esattamente cio' che #2235
	 * toglie: non va reintrodotto per comodita' di firma.
	 */
	template <typename TValore>
	bool Forza(IConsoleVariable& Var, const TValore& Valore)
	{
		Applica(Var, Valore);
		if (Coincide(Var, Valore))
		{
			return true;
		}

		const FString Messaggio = FString::Printf(
			TEXT("la console variable %s non ha accettato %s (vale \"%s\"): la scrittura e' stata rifiutata ")
			TEXT("anche a priorita' corrente — la variabile e' ReadOnly, oppure il valore non e' parsabile. ")
			TEXT("Questo test sta misurando un allestimento che non ha ottenuto."),
			*IConsoleManager::Get().FindConsoleObjectName(&Var), *Mostra(Valore), *Var.GetString());

		if (FAutomationTestBase* InCorso = FAutomationTestFramework::Get().GetCurrentTest())
		{
			InCorso->AddError(Messaggio);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s"), *Messaggio);
		}
		return false;
	}

	/**
	 * Imposta una console variable per la durata di uno scope e la RIPRISTINA uscendo.
	 *
	 * Una console variable dura quanto il processo: lasciarla impostata scavalcherebbe la proprieta' in
	 * ogni test successivo, e il rosso comparirebbe altrove — dove nessuno lo collega al file che l'ha
	 * causato.
	 *
	 * ⛔ **Non copiabile**: due guard che salvano lo stesso «precedente» e ripristinano in sequenza
	 * rimetterebbero un valore intermedio.
	 */
	template <typename TValore>
	struct TGuardia
	{
		explicit TGuardia(IConsoleVariable& InVar)
			: Var(InVar), Precedente(Leggi<TValore>(InVar)) {}

		TGuardia(IConsoleVariable& InVar, const TValore& Nuovo)
			: Var(InVar), Precedente(Leggi<TValore>(InVar))
		{
			Forza(Var, Nuovo);
		}

		~TGuardia() { Forza(Var, Precedente); }

		TGuardia(const TGuardia&) = delete;
		TGuardia& operator=(const TGuardia&) = delete;

		/** Cambia il valore dentro lo scope: il «precedente» resta quello letto alla costruzione. */
		bool Imposta(const TValore& Nuovo) { return Forza(Var, Nuovo); }

	private:
		IConsoleVariable& Var;
		TValore Precedente;
	};
}
