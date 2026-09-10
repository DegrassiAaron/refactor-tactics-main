#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "RTOverlayArea.generated.h"

/**
 * Cosa SIGNIFICA un'area disegnata sulla board (OVL-01, #1941).
 *
 * 🔴 **Le voci sono i significati che il gioco disegna DAVVERO oggi, non la grammatica che vorrebbe
 * disegnare.** Misurato sul corpo di `ARTHexMapActor::DrawPlanningPreview` (`RTHexMapActor.cpp`, `origin/main`
 * `072d026a`):
 *
 * ```
 * $ awk 'NR>=1138 && NR<=1264' Source/RefactorTactics/Map/RTHexMapActor.cpp \
 *     | grep -oE "FColor\([0-9]+, *[0-9]+, *[0-9]+\)|FColor::[A-Za-z]+" | sort -u | wc -l
 *   6
 * ```
 *
 * (Misurato **prima** della migrazione. Oggi la stessa riga risponde `0`, ed e' il punto di questa issue.)
 *
 * Sei tinte distinte, una per voce di questo enum. ⛔ **I valori non sono ricopiati qui**: vivono in
 * `URTOverlayPalette::ColorFor` e in nessun altro posto — e' l'invariante che #1941 chiede, *«git grep di un
 * valore della palette da' UNA occorrenza, non una per consumatore»*. Un commento che li ripetesse la
 * romperebbe esattamente come un secondo consumatore.
 *
 * ⚠️ **Sono SEI, e il body di #1941 ne contava cinque**: `AttackOriginAim` — *«da dove parte»* e *«verso
 * cosa»* — non era in tabella, ma e' un significato a se' con la sua scala e il suo canale di certezza.
 *
 * ⛔ **Le voci della spec v0.2 che qui NON compaiono — Vision/LOS, Ability Range, Hazard, Objective, Invalid —
 * non sono dimenticate: non hanno un produttore.** #1944 misura quali dati esistono e quali overlay mancano,
 * ed e' l'owner che le apre. Dichiararle qui senza produttore sarebbe un placeholder, e un enum con voci che
 * nessuno emette non e' un vocabolario: e' una promessa.
 */
UENUM(BlueprintType)
enum class ERTOverlayMeaning : uint8
{
	/** Dove POSSO andare: contesto del budget di movimento, non una decisione presa. */
	Movement,
	/** Dove VADO: la traccia del percorso pianificato. */
	PathTrace,
	/** Da dove parte l'attacco e verso cosa mira. Precede l'area: risponde a una domanda diversa. */
	AttackOriginAim,
	/** Chi COLPISCO. */
	Attack,
	/** Un alleato e' dentro l'area colpita, e va visto PRIMA del lock-in. */
	FriendlyFire,
	/** Cosa sto indicando. Interaction Context, non un'area semantica di gameplay. */
	Hover,
	/**
	 * La LINEA di tiro verso un bersaglio, e dove si interrompe — `#2742`.
	 *
	 * ⛔ **Non e' l'area di visibilita'**, cioe' *«quali celle vedo»*: quella e' di `#1944`, ha un'altra
	 * resa e un altro costo. Il confine e' stato deciso il 2026-09-10 ed e' scritto in entrambe le issue.
	 *
	 * 🔑 Il valore `#32ADE6` era **riservato da [D-368] e tenuto fuori da questo enum**, perche' un
	 * significato senza produttore nasce morto. Entra ora, insieme a `URTSightLineLibrary`.
	 */
	Vision
};

/**
 * Quanto e' certa l'informazione che l'area porta.
 *
 * 🔴 **`Certainty` muove SOLO l'opacita', mai il colore** — invariante di #1941, verificata da
 * `RefactorTactics.AreaOverlay.CertaintyChangesOnlyOpacity`. Il canale non cromatico della certezza esiste gia'
 * ed e' il **tratteggio**: `RTHexMapActor.cpp` lo usa per distinguere un'origine confermata da una prevista,
 * e il commento in loco lo dichiara *«un canale non cromatico, come chiede la grammatica della certezza»*.
 */
UENUM(BlueprintType)
enum class ERTOverlayCertainty : uint8
{
	Confirmed,
	Predicted,
	Uncertain
};

/**
 * Un'area semantica da disegnare: **cosa** significa, **chi** l'ha prodotta, **quanto** e' certa, su **quale**
 * piano vive. La priorita' non e' un campo perche' non e' un dato dell'area: e' una proprieta' del significato,
 * e vive con lui in `URTOverlayPalette::PriorityFor`.
 *
 * ⛔ **Presentation-only.** Le celle arrivano gia' calcolate dai servizi autorevoli. Questo tipo non ricalcola
 * LOS, path o targeting, e non muta `FRTMapState`, snapshot, TurnLog, hash, graph revision o path cache —
 * verificato da `RefactorTactics.AreaOverlay.DrawingMutatesNothing`.
 */
USTRUCT(BlueprintType)
struct FRTOverlayArea
{
	GENERATED_BODY()

	/** Le celle dell'area. Portano loro l'identita' spaziale: `FRTCellId{X, Y, Layer}`. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Overlay")
	TArray<FRTCellId> Cells;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Overlay")
	ERTOverlayMeaning Meaning = ERTOverlayMeaning::Movement;

	/**
	 * Unita', abilita' o sistema che ha prodotto l'area.
	 *
	 * 🔴 **`Source` non cambia il colore** — invariante di #1941, verificata da
	 * `RefactorTactics.AreaOverlay.SourceDoesNotChangeColor`. E' anche il punto in cui la privacy si puo' perdere:
	 * un'area puo' esistere solo per dati che quel client e' **gia'** autorizzato a conoscere (invariante #6,
	 * `FRTPlannedIntent → FilterForTeam → FRTIntentView`). Questo tipo non apre un canale nuovo.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Overlay")
	FName Source;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Overlay")
	ERTOverlayCertainty Certainty = ERTOverlayCertainty::Confirmed;

	/**
	 * Il piano su cui vive l'area, LETTO dalle celle invece che ricopiato accanto a loro.
	 *
	 * ⚠️ **Un campo `Layer` separato sarebbe una seconda verita' di piano** accanto a `FRTCellId::Layer`, e le
	 * due divergerebbero appena qualcuno riempie una senza l'altra. Il non-goal di #1941 e' esplicito —
	 * *«nessun secondo modello spaziale»* — quindi il piano si deriva. Che un'area stia su un piano solo lo
	 * verifica `RefactorTactics.AreaOverlay.AreaIsSingleLayer`.
	 */
	int32 Layer() const
	{
		return Cells.Num() > 0 ? Cells[0].Layer : 0;
	}

	/** Vero se tutte le celle stanno sullo stesso piano. Un'area che ne attraversa due non e' un'area. */
	bool IsSingleLayer() const
	{
		for (const FRTCellId& Cell : Cells)
		{
			if (Cell.Layer != Layer())
			{
				return false;
			}
		}
		return true;
	}
};
