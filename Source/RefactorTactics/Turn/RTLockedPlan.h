#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Turn/RTUnitPlan.h"
#include "RTLockedPlan.generated.h"

/**
 * LA COPIA DEL PIANO AL LOCK-IN ([D-608] (2) e (4); E50 fetta 2, tappa 4, #4127).
 *
 * Il piano dell'unita' com'era quando il turno si e' chiuso, con le due cose che solo il lock-in sa: il bersaglio per
 * IDENTITA' (`StableUnitId`, che `EnsureMatchRoster` assegna proprio al lock-in) e la mira congelata ([D-415]). La
 * scatta `ARTUnit::LockPlan()`, e da li' non cambia: la legge chi risolve, `ARTUnit::GetLockedPlan()`.
 *
 * 🔑 **Immutabile, e il consumo e' altrove.** I resolver consumano il piano VIVO (la principale spesa, la mossa
 * ripiantata dopo lo scatto); la copia resta il piano dichiarato. Leggere le voci da qui invece che dal piano vivo e'
 * il passo di E38 CP 38.13, il primo che legge piu' voci della stessa unita'.
 *
 * 🔒 Privata della squadra fino alla risoluzione, come il piano: `RTServerOnly`, e
 * `Privacy.ServerOnlyTypesAreNotReplicated` la pinna per nome.
 */
USTRUCT(meta = (RTServerOnly))
struct REFACTORTACTICS_API FRTLockedPlan
{
	GENERATED_BODY()

	/** Il piano al lock-in: le voci nel loro ordine di dichiarazione, la cella d'arrivo, il verso, il Cleanse. */
	UPROPERTY()
	FRTUnitPlan Plan;

	/** Il bersaglio-unita' per identita' ([D-608] (2)); `INDEX_NONE` se il piano non ne dichiara uno vivo. */
	UPROPERTY()
	int32 AttackTargetStableId = INDEX_NONE;

	/**
	 * **La cella su cui l'azione principale mira, congelata al LOCK-IN** ([D-415]).
	 *
	 * 🔑 **Esiste perche' la mira non deve INSEGUIRE.** Fino al 2026-09-20 il Blast leggeva
	 * `PlannedAttackTarget->Cell` al momento di risolvere, cioe' **dopo** il Dash, le spinte e le reazioni
	 * (il movimento normale risolve dopo il Blast): un bersaglio che scattava veniva seguito, e chi sparava
	 * colpiva una posizione che al momento di decidere non esisteva.
	 * In una fase simultanea questo rende illeggibile la scelta — si mira a qualcosa che si muoverà.
	 *
	 * ⚠️ **NON e' `PlannedAttackCell`, ed e' deliberato.** Quella e' l'altra META' della scelta esclusiva di
	 * `#2884`: `bAttackTargetsCell` decide QUALE delle due il resolver legge, e scriverci la mira di un
	 * bersaglio-unita' renderebbe le due forme indistinguibili proprio nel campo che le separa.
	 *
	 * 🔑 **Il corollario di [D-415] regge da solo**: il bersaglio che si e' spostato puo' comunque essere
	 * colpito, perche' `CollectHexAttacks` sceglie chi colpire **geometricamente** — ogni unita' viva su una
	 * cella dell'area — e non dall'identita' del bersaglio dichiarato. Non c'e' niente da aggiungere per
	 * ottenerlo, solo da non rompere.
	 *
	 * ⛔ **Non vale per chi DICHIARA di agganciare** (`ERTActionFallback::AttackTarget`): quelle azioni
	 * leggono la cella viva del bersaglio, ed e' l'uscita che `D-415` nomina. La dichiarano `Action.Interrupt` e
	 * `Action.Heal` (D-493), e il perche' sta accanto alla riga di catalogo di ciascuna; chi risolve la legge da
	 * `ResolutionAimCell`.
	 *
	 * 🔒 **NON e' visibile ai Blueprint, ed e' deliberato** (CLAUDE.md §7): e' intento della squadra che lo
	 * dichiara, letto solo dal resolver. Esposto senza filtro, un grafo di presentazione leggerebbe dove mira
	 * il nemico — e `Unit.BlueprintSurfaceIsCensused` lo avrebbe dovuto censire come debito «da ritirare»,
	 * come ogni altro campo del piano. ⌫ *Fino alla suite completa del 2026-10-10 era `BlueprintReadOnly`.*
	 */
	UPROPERTY()
	FRTCellId AimCell;

	/** Se `AimCell` porta una mira (un bersaglio-unita' vivo al lock-in). `FRTCellId()` e' una cella legittima. */
	UPROPERTY()
	bool bHasAim = false;
};
