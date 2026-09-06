#pragma once

#include "CoreMinimal.h"
#include "RTPresentationRole.generated.h"

/**
 * Il ruolo che una clip copre nella presentazione.
 *
 * ⚠️ **E' vocabolario, non un elenco di cose che il runtime gia' suona.** Il grafo di
 * `URTUnitAnimInstance` ha DUE sequence player e consuma `Idle` e `Move` e nient'altro. `Attack`, `Hit`
 * e `Death` passano dai tre `BlueprintImplementableEvent` di `ARTUnit` (`PlayAttackMontage`,
 * `PlayHitMontage`, `PlayDefeatMontage`) — vedi #2448; `Cast`, `Dash`, `Defend` e `Fall` non hanno
 * ancora nessun consumatore.
 *
 * L'enum li nomina lo stesso perche' servono all'authoring: il pannello di #2443 li elenca, il catalogo
 * ci lega le clip, e l'Action possiede il proprio `PresentationRole`. Il DATO pero' non nasce inerte —
 * `FRTHeroPresentationClips::PerRole` e' una `TMap`, e un ruolo che nessuno popola **non esiste**.
 *
 * 🔑 **Vive in un header suo, e la ragione e' misurata** (#2443): stava dentro `RTUnitAnimInstance.h`,
 * che include `AnimNodes/AnimNode_Slot.h` e `Animation/AnimNode_SequencePlayer.h` — cioe' il modulo
 * `AnimGraphRuntime`. Chiunque volesse solo *nominare un ruolo* — il catalogo, il modulo Editor, un
 * validator — si portava dietro il grafo d'animazione, e il modulo Editor non compilava affatto.
 * Un vocabolario deve costare quanto un vocabolario.
 */
UENUM(BlueprintType)
enum class ERTPresentationRole : uint8
{
	Idle,
	Move,
	Attack,
	Cast,
	Dash,
	Defend,
	Hit,
	Death,
	Fall
};
