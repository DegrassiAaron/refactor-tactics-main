#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTRemoveEnergyBarCommandlet.generated.h"

/**
 * Toglie dai `WBP_RT_*` la barra dell'energia che nessun C++ aggiorna piu' (`#2372`).
 *
 * 🔴 **Il difetto e' che i due asset hanno smesso di essere alimentati senza smettere di disegnare.**
 * [`D-324`](../../../../docs/decisions/RT_PDR_00_Decision_Log.md) ha tolto `Energy` dal gameplay e
 * [#610](https://github.com/DegrassiAaron/refactor-tactics-main/issues/610) ha rimosso il C++ che ne
 * riempiva la UI: `URTUnitOverlayWidget::EnergyBar` e i campi `Energy`/`MaxEnergy` di `FRTUnitCardView`
 * non esistono piu'. ⚠️ Ma `EnergyBar` era un `BindWidgetOptional`, e il binding e' un requisito **dal
 * C++ verso il Blueprint**, non viceversa: tolta la proprieta', il `WBP` non fallisce il bind — la
 * `ProgressBar` resta nell'albero come elemento normale, e nessuno chiama piu' `SetPercent` su di lei.
 * Si disegna sopra ogni unita', al valore che il designer le ha dato, per sempre.
 *
 * ✅ **«Compila» e «e' corretto» sono due domande.** `#610` ha verificato la prima con
 * `-run=CompileAllBlueprints`, e la risposta era si' per entrambi gli asset. Questo commandlet risponde
 * alla seconda.
 *
 * Perche' un commandlet e non quattro clic nell'Editor — sono le parole di `RTSetObjectiveCell`, e valgono
 * identiche qui: *«un `.uasset` non e' diffabile, quindi il diff di una PR non puo' mostrare cosa e'
 * cambiato dentro. Qui il cambiamento e' un comando scritto — si rilegge, si ripete su un altro asset, e se
 * qualcuno lo disfa per sbaglio si riapplica identico.»*
 *
 * ⛔ **Non contiene nessuna regola di gioco**: rimuove un widget di presentazione e risalva. La decisione
 * di toglierlo e' `D-324` piu' la misura di `#2372`; qui c'e' il gesto.
 *
 * ⚠️ **`WBP_RT_UnitCard` non ha un widget da togliere, ma pin da far cadere.** Il suo grafo leggeva
 * `Card_Energy` e `Card_MaxEnergy` da `FRTUnitCardView`; i membri non ci sono piu', e i pin orfani
 * spariscono alla **ricompilazione**. Restano pero' scritti nel `.uasset` finche' qualcuno non lo risalva:
 * il commandlet lo ricompila e lo risalva per quello, senza toccarne l'albero.
 *
 * Uso:
 *
 *     UnrealEditor-Cmd RefactorTactics.uproject -run=RTRemoveEnergyBar [-DryRun]
 *
 * `-DryRun` non scrive nulla e dice cosa farebbe: e' il primo comando da lanciare, sempre.
 */
UCLASS()
class URTRemoveEnergyBarCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URTRemoveEnergyBarCommandlet();

	virtual int32 Main(const FString& Params) override;
};
