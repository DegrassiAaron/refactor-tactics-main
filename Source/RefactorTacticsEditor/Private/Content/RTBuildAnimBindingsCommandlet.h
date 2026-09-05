#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "Unit/RTUnitAnimInstance.h"

#include "RTBuildAnimBindingsCommandlet.generated.h"

/**
 * Genera `ABP_RTUnitAuthored` dai binding di `Data/Anim/AnimCatalog.json` (#2443).
 *
 * 🔑 **E' il ponte fra il testo e il runtime, e chiude la domanda che l'innesto di #2445 poneva.**
 * Il pannello scrive il giudizio umano in un file **diffabile**; il gioco legge
 * `URTUnitAnimInstance::ClipsPerHero`, che e' C++. Le tre vie per collegarli avevano costi diversi:
 *
 *  - **scrivere il sorgente C++** obbliga l'autore a ricompilare per vedere l'effetto della propria scelta;
 *  - **autorare un `.uasset` a mano** aggiunge il byte binario che `D-248` e #2441 esistono per evitare,
 *    e `CLAUDE.md` §5 lo vieta comunque;
 *  - **testo versionato + generazione** non paga nessuno dei due, ed e' il pattern che il repository ha
 *    gia' pagato tre volte (`RTBuildGrayboxMeshes`, `RTBuildIconCatalog`, `RTBuildPlaygroundPanel`).
 *    `D-229`: la sorgente e' il codice, l'asset e' il suo **output**.
 *
 * ⚠️ **Cio' che rende utile la generazione, e non cerimoniale**: `ClipsPerHero` e' `EditDefaultsOnly`, e
 * `ARTUnit::ApplyUnitAnimClass` dichiara in chiaro che *«una `Anim Class` gia' scelta in Blueprint
 * VINCE»*. Il Blueprint generato scavalca quindi il default C++ **senza ricompilare**.
 *
 * ⛔ **Non promuove niente e non lega niente.** Legge i binding che una persona ha scritto e li traduce.
 * Se il catalogo non ha binding, l'asset generato e' vuoto e il roster resta quello del default C++.
 *
 * ⛔ **Rifiuta un catalogo non valido.** Due varianti attive sullo stesso `(eroe, ruolo)` sono
 * rappresentabili nel testo ma non a runtime: generare comunque significherebbe sceglierne una per
 * posizione nell'array, cioe' far dipendere la clip che suona dall'ordine delle righe di un file.
 *
 * Uso — si avvia ed esce da solo, senza tenere aperto un Editor:
 *
 * ```
 * UnrealEditor-Cmd.exe <progetto> -run=RTBuildAnimBindings [-DryRun]
 * ```
 */
UCLASS()
class URTBuildAnimBindingsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;

	/**
	 * La traduzione catalogo → CDO, estratta perche' sia misurabile.
	 *
	 * 🔑 **E' l'unica parte di questo commandlet che si puo' provare headless**, ed e' anche l'unica che
	 * contiene decisioni: il resto e' aprire un file, creare un package e salvarlo. Lasciata dentro
	 * `Main` sarebbe stata verificabile solo eseguendo l'Editor su un catalogo con dei legami — e i
	 * legami richiedono una clip `Promoted`, che **solo una persona puo' scrivere**. Il test sarebbe
	 * quindi rimasto impossibile per costruzione.
	 *
	 * @param OutLegami quanti binding sono stati tradotti.
	 */
	static TMap<FName, FRTHeroPresentationClips> BuildClipsPerHero(
		const struct FRTAnimCatalog& Catalog, int32& OutLegami);
};
