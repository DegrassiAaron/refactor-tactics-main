#pragma once

// Eseguire un tasto come lo esegue il giocatore, senza un viewport (#4065).
//
// Un test che vuole provare un handler di `ARTPlayerController` non ha bisogno di un gancio `*ForTest`: lega i binding
// a un componente suo (`ARTPlayerController::BindInputActions`) ed esegue quello dell'azione, attraverso il delegate
// vero. Se il binding manca, o porta a un altro handler, il test lo vede.
//
// ⚠️ **Cosa NON copre**: il passo tasto → azione, che e' del mapping context e dei suoi test (`PlayerInput.*`), e
// tutto cio' che l'handler chiede al viewport — in headless non c'e', e un handler che lo interroga (la rotella, l'edge
// pan) risponde come col cursore fuori dalla scena.

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Player/RTPlayerController.h"

namespace RTInputBindingFixtures
{
	/**
	 * Un'istanza d'azione con l'evento e il valore scelti dal test.
	 *
	 * `FInputActionInstance` li tiene `protected`, e a riempirli in partita e' `UEnhancedPlayerInput`. Un tipo derivato e'
	 * l'accesso che l'header lascia, e il binding lo riceve com'e': `Execute` passa `GetValue()` al delegate, e
	 * `GetValue()` restituisce il valore vero (`EnhancedInput.bAlwaysGetRealValueFromActionInstanceData` vale 1 di
	 * default).
	 */
	struct FActionInstance : public FInputActionInstance
	{
		FActionInstance(const UInputAction* Action, ETriggerEvent Event, const FInputActionValue& InValue)
			: FInputActionInstance(Action)
		{
			TriggerEvent = Event;
			Value = InValue;
		}
	};

	/** Un componente d'input del controller, con i binding di produzione legati. */
	inline UEnhancedInputComponent* BindInput(ARTPlayerController& PC)
	{
		UEnhancedInputComponent* Input = NewObject<UEnhancedInputComponent>(&PC);
		PC.BindInputActions(*Input);
		return Input;
	}

	inline const UInputAction* ActionNamed(const UEnhancedInputComponent& Component, const TCHAR* Name)
	{
		for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : Component.GetActionEventBindings())
		{
			if (Binding && Binding->GetAction() && Binding->GetAction()->GetName() == Name)
			{
				return Binding->GetAction();
			}
		}
		return nullptr;
	}

	/** Esegue i binding dell'azione col nome dato su quell'evento. Restituisce quanti ne ha eseguiti: 0 vuol dire nessun binding. */
	inline int32 Fire(const UEnhancedInputComponent& Component, const TCHAR* Name, ETriggerEvent Event,
		const FInputActionValue& Value = FInputActionValue())
	{
		const UInputAction* Action = ActionNamed(Component, Name);
		if (!Action)
		{
			return 0;
		}
		int32 Eseguiti = 0;
		for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : Component.GetActionEventBindings())
		{
			if (Binding && Binding->GetAction() == Action && Binding->GetTriggerEvent() == Event)
			{
				Binding->Execute(FActionInstance(Action, Event, Value));
				++Eseguiti;
			}
		}
		return Eseguiti;
	}
}
