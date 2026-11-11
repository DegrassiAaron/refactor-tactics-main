#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTAddDamageTokenCommandlet.generated.h"

/**
 * Aggiunge a `WBP_RT_UnitOverlay` il `UTextBlock` che il token di danno cerca (`#2455`).
 *
 * 🔴 **Il difetto che chiude e' un'assenza che non fa rumore.** `URTUnitOverlayWidget::DamageTokenText` e'
 * un `BindWidgetOptional`, e l'opzionalita' e' deliberata: un `WBP` a cui manca un elemento **degrada** —
 * mostra il resto — invece di rifiutarsi di compilare. La contropartita e' che finche' l'elemento non
 * esiste, il C++ compone il token, lo posa su un puntatore nullo e **non succede niente**: nessun errore,
 * nessun test rosso, nessuna cifra a schermo. E' esattamente il caso che il doc-comment di quel campo
 * dichiara, e questo commandlet e' la sua chiusura.
 *
 * ⚠️ **Perche' un commandlet e non due clic nell'Editor** — sono le parole di `RTSetObjectiveCell`, e
 * valgono identiche qui: *«un `.uasset` non e' diffabile, quindi il diff di una PR non puo' mostrare cosa
 * e' cambiato dentro. Qui il cambiamento e' un comando scritto — si rilegge, si ripete su un altro asset,
 * e se qualcuno lo disfa per sbaglio si riapplica identico.»* Un `.uasset` toccato a mano lascia solo
 * `+51 byte` nel diff, e chi rivede la PR deve fidarsi.
 *
 * 🔑 **Ripetibile.** Alla seconda esecuzione trova l'elemento e non fa nulla: e' la stessa forma di
 * `RTRemoveEnergyBar`, e serve perche' riapplicarlo dopo un `-Force` altrui non debba essere un gesto
 * delicato.
 *
 * ⛔ **Nessuna regola di gioco.** Aggiunge un elemento di presentazione e risalva. Cio' che il token
 * mostra — la cifra, il segno, il caso `Amount <= 0` — vive in `URTHudViewModel::BuildDamageToken`, in C++
 * e sotto test. Qui c'e' solo il **posto** dove finisce.
 *
 * Uso:
 *
 *     UnrealEditor-Cmd RefactorTactics.uproject -run=RTAddDamageToken [-DryRun]
 *
 * `-DryRun` non scrive nulla e dice cosa farebbe: e' il primo comando da lanciare, sempre.
 *
 * 🔎 **Il guardiano non e' questo commandlet.** Che l'elemento ci sia lo verifica
 * `RefactorTactics.Overlay.UnitOverlayCarriesTheDamageTokenBinding` — un Automation Test che carica il
 * `.uasset` e ne interroga l'albero. Senza, «l'ho cablato» resterebbe una frase.
 */
UCLASS()
class URTAddDamageTokenCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URTAddDamageTokenCommandlet();

	virtual int32 Main(const FString& Params) override;
};
