#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RTSyncUnitAnimRefsCommandlet.generated.h"

/**
 * Scrive sui `BP_Unit_*` la classe d'animazione autorata e le clip che il cook deve portare (#3562).
 *
 * 🔑 **Due campi per blueprint, e tutti e due vengono dal CDO della classe autorata `ABP_RTUnitAuthored`**, che
 * `RTBuildAnimBindings` genera dal default C++ e dal catalogo:
 *  - `UnitAnimClass` = la classe autorata, cosi' cio' che suona in gioco e' cio' che il catalogo ha deciso;
 *  - `CookDeps_Anim` (variabile del Blueprint) = **esattamente** le clip attive dell'eroe, ruolo piu' azione, come
 *    riferimenti DURI. Il cook segue solo quelli (D-262), e `Packaging.RequiredActionClipsAreCooked` li misura.
 *
 * ⛔ **Tre fasi, e la prima non scrive niente.** Un `.uasset` salvato dove una sua dipendenza dura non si carica
 * perde quel riferimento senza errore (`CLAUDE.md` §10, la trappola del worktree: `Content/FabAsset/` e' ignorato).
 *  1. **Validare tutto**: la classe autorata si carica; ogni eroe del default C++ ha il suo blueprint in
 *     `ARTGameMode::HeroUnitClasses`; `CookDeps_Anim` esiste ed e' un array di riferimenti DURI (`FObjectProperty`,
 *     non `FSoftObjectProperty`); ogni dipendenza dura del package esiste; ogni clip si carica. Un errore ferma tutto.
 *  2. **Scrivere**: i due campi, la compilazione (`BS_Error` -> niente salvataggio), il salvataggio.
 *  3. **Rileggere**: le dipendenze DURE di ogni package, secondo l'Asset Registry, contengono le clip e la classe.
 *
 * Uso — dal clone principale, l'unico che ha i pack Paragon:
 *
 *     UnrealEditor-Cmd RefactorTactics.uproject -run=RTSyncUnitAnimRefs [-DryRun]
 *
 * `-DryRun` esegue la fase 1 e scrive nel log, per ogni blueprint, le clip che entrano e quelle che escono, e una riga
 * `TOTALE blueprint=… entrano=… escono=… classi_da_assegnare=… errori=…`. `-ExpectNoChanges` fa lo stesso ed esce
 * con errore se qualcosa entrerebbe o uscirebbe ancora: dopo un salvataggio e' la prova che i valori sono
 * sopravvissuti alla compilazione.
 *
 * ⚠️ Quando una variante diventa attiva, l'ordine e': `-run=RTBuildAnimBindings`, poi questo.
 */
UCLASS()
class URTSyncUnitAnimRefsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URTSyncUnitAnimRefsCommandlet();

	virtual int32 Main(const FString& Params) override;
};
