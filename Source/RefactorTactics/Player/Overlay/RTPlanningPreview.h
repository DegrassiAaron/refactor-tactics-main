#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"

class ARTHexMapActor;
class ARTUnit;
class URTActionData;
class UWorld;
struct FRTFocusMoveAreas;
struct FRTRangeCellVerdict;

/**
 * **CHI SCRIVE L'ANTEPRIMA DI PIANIFICAZIONE SULLA MAPPA** (#4065, S5), e le tre domande sulla mira che la decidono.
 *
 * Stava nel namespace anonimo del controller. La chiama una sola sede, l'arbitro dell'overlay
 * (`FRTPlanningOverlayArbiter::Apply`): e' l'unico punto che tocca i setter delle anteprime di `ARTHexMapActor`.
 *
 * ⚠️ **Sola presentazione.** Ogni insieme viene dalle stesse funzioni che decidono l'esito (`ReachableCells`,
 * `BlastOriginCell`, `HexHitCells` via `MakeBlastPreview`): nessun calcolo parallelo, altrimenti il giocatore vedrebbe una
 * zona e ne subirebbe un'altra.
 */
namespace RTPlanningPreview
{
	/**
	 * L'azione armata chiede un BERSAGLIO, e allora in targeting la portata prende il posto del ventaglio (`#3507`).
	 *
	 * Il predicato e' quello di `GetPointerContext` — armata, e non mobilita' rapida — piu' un bersaglio da scegliere: un
	 * supporto su se stessi non ha una portata da mostrare, e uno scatto chiede una destinazione, cioe' il ventaglio.
	 * ⛔ Una reazione si arma senza bersaglio, e scatta in risoluzione: non ha un punto in cui mirare.
	 */
	REFACTORTACTICS_API bool ChiedeUnBersaglio(const URTActionData& Armata);

	/**
	 * ...e quella portata ha celle da mostrare (`#3517`). Ogni congiunto toglie un caso in cui il click rifiuterebbe ogni
	 * cella dell'area, cioe' la lettura che [D-128] vieta:
	 *
	 * - **portata positiva** — `AC-10`. Con portata `0` `TargetableRangeCells` restituisce la sola cella del tiratore, e
	 *   `Action.Wait` contornava di viola l'unita' stessa;
	 * - **tiratore vivo** — `AC-10`: un'unita' caduta, ancora selezionata a fine playback, non mira.
	 *
	 * ⏱️ **Fino a `#3833` c'era un terzo congiunto, `CanUseAbility`** — `DR-8`, decisione d'autore del 2026-10-06: *«con
	 * un'azione in ricarica armata, nessuna portata»*. Da `#3830` un'azione in ricarica non si arma e un'unita' viva, in
	 * Pianificazione, non ne ha una armata (l'armo si rifiuta in `SelectAbilityForCurrent`, la ricarica nasce solo in
	 * risoluzione, il Cleanup disarma): la condizione non poteva piu' essere vera, ed e' stata tolta. L'invariante la pinna
	 * `Turn.ACooldownActionLeavesTheTurnConsumedDisarmedAndNotRearmable`.
	 *
	 * ⚠️ **Non decide il ventaglio**: quello lo spegne un'azione che chiede un bersaglio con una portata positiva
	 * (`Refresh`).
	 */
	REFACTORTACTICS_API bool MostraLaPortata(const ARTUnit* Unit, const URTActionData& Armata);

	/**
	 * La cella da cui si mira l'azione ARMATA, e da cui si misura la portata. 🔑 **Una sola espressione per la portata e
	 * per il piano attivo** (`DR-5`, `#3517`): se divergessero, la portata starebbe su un piano e il click su un altro.
	 *
	 * Dipende dalla FASE dell'azione ([D-464], `#3509`): con uno scatto pianificato un `Attack` mira dalla cella dello
	 * scatto, un `Environment` da quella corrente. ⏱️ *Fino a #3509 era `Unit->Cell`*, e `HandleTargetCell` e il click su
	 * un'unita' la leggevano da li' per conto proprio: ora chiedono tutti `ARTUnit::AimOriginFor`.
	 */
	REFACTORTACTICS_API FRTCellId OrigineDiMira(const ARTUnit* Unit, const URTActionData& Armata);

	/**
	 * Aggiorna l'anteprima di pianificazione (SOLA PRESENTAZIONE) dallo stato dell'unita': dove puo' arrivare, da DOVE
	 * agira' e quali celle colpirebbe — segnalando gli ALLEATI che finirebbero nell'area. `Unit == nullptr` (deselezione,
	 * fine pianificazione) spegne l'anteprima.
	 *
	 * @param KnownHexMap la mappa, se il chiamante l'ha gia' cercata: una seconda ricerca itererebbe gli actor del livello.
	 * @param FocusAreas  le aree del soggetto a fuoco: se ci sono, la mappa e' del fuoco e il piano cede.
	 * @param SkillRange  la portata a tre stati di una skill in hover: se c'e', la mappa e' sua e il piano cede.
	 */
	REFACTORTACTICS_API void Refresh(const UWorld* World, const ARTUnit* Unit, ARTHexMapActor* KnownHexMap = nullptr,
		const FRTFocusMoveAreas* FocusAreas = nullptr, const TArray<FRTRangeCellVerdict>* SkillRange = nullptr);
}
