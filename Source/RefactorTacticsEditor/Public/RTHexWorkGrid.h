#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "Map/RTMapVisuals.h"

/**
 * LA GRIGLIA DI LAVORO — il piano di lavoro **dove le celle non esistono ancora** (#622).
 *
 * 🔑 **Il difetto che chiude**: nell'Hex Map mode si vede cio' che *esiste gia'*. Sul bordo della mappa si
 * disegna al buio, e la cella nuova appare dove capita. Qui si decide **quali** coordinate vuote marcare;
 * la resa la posa `ARTHexWorkGridActor`, e le due cose sono separate apposta.
 *
 * ## ⛔ Perche' questo namespace non vede ne' un `AActor` ne' un `UWorld`
 *
 * La issue vieta esplicitamente di *«riusare `DemoRadius` come sorgente della griglia»* — il campo
 * dell'actor che, quando l'asset manca, fa disegnare alla board 61 esagoni che il dato non contiene. E'
 * l'episodio che `docs/technical/tooling/brief-editor-map-viz.md` §1 registra: *«per un'ora la mappa
 * mostrava 61 esagoni mentre `rt.Arena.Check` diceva "non ha celle"»*, e che `RTHexEditorMode.cpp:192-194`
 * cita nominando questa issue.
 *
 * 🔑 **Qui quel divieto e' garantito dal TIPO, non da una riga di prosa**: `BuildPlan` riceve delle
 * coordinate e dei numeri. Non puo' leggere `DemoRadius` perche' non ha da dove.
 *
 * ## ⚠️ Il canale che rende la griglia non confondibile con una cella vera
 *
 * Le costanti qui sotto sono meta' della risposta al secondo criterio del DoD, e vanno lette insieme a
 * `ARTHexWorkGridActor`. In breve: **le celle vere TASSELLANO** — i loro anelli di bordo
 * (`ARTHexMapActor::CellBorders`, acceso per default, `RTHexMapActor.h:965`) condividono i lati e formano
 * un alveare continuo — mentre la griglia di lavoro e' fatta di **isole staccate**, separate da fondo
 * scoperto largo piu' di mezza cella. E' un canale **topologico**, non una tinta: sopravvive alla luce,
 * allo zoom e alla propria taratura.
 */
namespace RTHexWorkGrid
{
	/**
	 * Il circumraggio dell'anello fantasma, **in frazioni di `HexSize`**.
	 *
	 * ⚠️ **In `HexSize` e non in unita' di mesh, ed e' una scelta.** `RTMapVisuals.h:116-121` avverte che
	 * confrontare le due scale di mesh *«direbbe il falso»*: il glifo porta il proprio `0.95` **dentro** la
	 * mesh, il prisma lo porta **fuori** in `PlanarScale`. Quel `0.95` e' oggi un letterale ripetuto in tre
	 * punti di `RTHexMapActor.cpp` (`:616`, `:1298`, `:1884`) e non e' esportato da nessun header: copiarlo
	 * qui sarebbe il difetto di #983 al quarto giro. Espresso in `HexSize` questo numero non ne dipende — e
	 * l'unico fatto che serve e' geometrico e non si puo' muovere: **due celle adiacenti distano `√3·HexSize`**.
	 *
	 * 🔑 **`0.52` non e' scelto a occhio, e' scelto per stare in una banda libera.** Gli anelli del glifo di
	 * costo (`D-183`) occupano da `0.95` verso l'interno a passo `0.0947` (`RTHexMapActor.cpp:82-85`): il
	 * piu' interno dei quattro arriva a `0.6133` di mesh, cioe' `0.583·HexSize`. Sotto quella soglia non c'e'
	 * altro inchiostro esagonale nella board.
	 */
	constexpr float OuterScaleInHexSize = 0.52f;

	/**
	 * Lo spessore del tratto, **derivato dal bordo cella e non ricopiato**.
	 *
	 * 🔑 **Stesso peso d'inchiostro del confine vero, e di proposito.** Se il fantasma fosse anche piu'
	 * sottile, il canale distintivo diventerebbe «una linea piu' fine», cioe' una *proporzione* — ed e'
	 * esattamente il canale che la seduta `U18` ha misurato NON leggibile a picco (`PIE-HEX-VIZ-BLOCCHI` ❌,
	 * *«la differenza e' una proporzione che la vista di lavoro azzera»*, `RTHexLibrary.cpp:276-279`).
	 * Tenendolo uguale, l'unica differenza che resta e' quella che si legge davvero: **dove** sta il tratto e
	 * **se tocca i vicini**.
	 */
	constexpr float Thickness = RTCellBorderThickness;

	/**
	 * Quota dell'anello fantasma sopra il piano del layer.
	 *
	 * ⚠️ **Derivata, mai un letterale.** `RTMapVisuals.h:145-148` registra che un numero riscritto a mano
	 * qui e' gia' finito dentro il prisma due volte, e che dal 2026-08-28 `RTCellTopZ` e' passato da `2,5` a
	 * `7,5` — quindi oggi una copia sbaglierebbe **di piu'** di allora. Il fantasma sta sul piano che le
	 * facce delle celle definiscono: cosi' la griglia di lavoro e la mappa si leggono complanari.
	 */
	constexpr float LiftZ = RTCellTopZ;

	/**
	 * ⚠️ **L'anello fantasma non raggiunge mai il perimetro della cella che marca.** Il perimetro sta a
	 * `HexSize` per definizione della griglia: sotto `0.6` il fantasma resta un segno *dentro* l'esagono, mai
	 * un contorno *dell'*esagono — che e' cio' che `CellBorders` gia' dice.
	 */
	static_assert(OuterScaleInHexSize > 0.f && OuterScaleInHexSize < 0.6f,
		"L'anello della griglia deve restare ben dentro l'esagono che marca, o diventa un secondo bordo cella (#622).");

	/**
	 * 🔴 **L'asserzione che porta il secondo criterio del DoD**, e vale la pena leggerla per esteso.
	 *
	 * Due celle adiacenti distano `√3·HexSize`. Un esagono di circumraggio `k·HexSize` ha apotema
	 * `(√3/2)·k·HexSize`, quindi fra due fantasmi vicini resta `√3·(1-k)·HexSize` di fondo scoperto. Con
	 * `k = 0.52` sono `0,83·HexSize`: **piu' di mezza cella**. Le celle vere, disegnate a `0,95·HexSize`,
	 * lasciano invece una cucitura di `0,087·HexSize`.
	 *
	 * ∴ tessere che si toccano contro isole staccate, con un rapporto di **9,6 a 1**. Se qualcuno alzasse
	 * `k` fin dove i fantasmi si sfiorano, la griglia diventerebbe l'alveare che questa issue esiste per non
	 * produrre — e la build si ferma prima.
	 */
	static_assert(1.7320508f * (1.f - OuterScaleInHexSize) >= 0.5f,
		"Fra due esagoni della griglia deve restare piu' di mezza cella di fondo scoperto: sotto questa "
		"soglia la griglia di lavoro TASSELLA come le celle vere, ed e' la vista che mente di #622.");

	/** ⚠️ Il bordo vero resta la cima della pila di lettura: la griglia di lavoro non ci sale sopra. */
	static_assert(LiftZ < RTLiftCellBorder,
		"La griglia di lavoro deve stare SOTTO l'anello di bordo delle celle vere (RTMapVisuals.h:150-151).");

	/**
	 * Da dove viene l'insieme restituito. Esiste perche' **un insieme vuoto ha piu' di una causa**, e senza
	 * questo campo chi legge il log non puo' distinguerle.
	 */
	enum class ESource : uint8
	{
		/** Nessun asset mappa: non c'e' un dato da cui derivare una griglia, e non se ne inventa una. */
		None,
		/** Le celle del layer attivo, dilatate di `AppliedReach` anelli. E' il caso normale. */
		Dilated,
		/** Il layer attivo e' vuoto: un esagono di raggio `AppliedReach` attorno all'origine, per cominciare. */
		Seeded,
	};

	/** Gli ingressi della decisione. Niente `AActor`, niente `UWorld`, niente asset: vedi il docstring sopra. */
	struct FInput
	{
		/** Se l'actor bersaglio ha davvero un `URTHexMapAsset`. `false` ⇒ nessuna griglia. */
		bool bHasAsset = false;

		/** Le celle che **esistono gia'** sul layer di lavoro. Sono cio' da cui la griglia si sottrae. */
		TArray<FRTCellId> ExistingOnLayer;

		/** Il layer di lavoro. La griglia vive solo qui: e' il piano su cui i pennelli scrivono. */
		int32 Layer = 0;

		/** Di quanti anelli dilatare le celle esistenti. */
		int32 Margin = 2;

		/** Raggio del seme quando il layer attivo e' vuoto. Distinto da `Margin`: significa un'altra cosa. */
		int32 SeedRadius = 3;

		/** Tetto sul numero di esagoni. Passato come PARAMETRO perche' un test possa stringerlo. */
		int32 MaxCells = 4096;
	};

	/** L'esito della decisione. */
	struct FPlan
	{
		/** Le coordinate da marcare, in ordine stabile (`URTHexLibrary::StableLess`). */
		TArray<FRTCellId> Cells;

		/** Quale ramo ha risposto. Vedi `ESource`. */
		ESource Source = ESource::None;

		/** Quanti anelli sono stati CHIESTI (`Margin`, o `SeedRadius` nel ramo `Seeded`). */
		int32 RequestedReach = 0;

		/** Quanti ne sono stati davvero applicati. Minore del richiesto ⇔ `bClamped`. */
		int32 AppliedReach = 0;

		/** Il tetto ha morso: la griglia e' piu' piccola di quanto chiesto, e chi guarda deve saperlo. */
		bool bClamped = false;
	};

	/**
	 * Decide quali coordinate vuote marcare.
	 *
	 * ⚠️ **Il troncamento avviene per ANELLI INTERI, non a meta'.** Una griglia tagliata dove capita
	 * suggerirebbe che la mappa finisce li'; una ridotta di un anello dice soltanto «piu' in la' non si
	 * mostra». `AppliedReach` porta la riduzione fuori invece di lasciarla in un log: e' l'unica forma in cui
	 * un test puo' distinguere «troncato per anelli» da «troncato a caso».
	 */
	FPlan BuildPlan(const FInput& In);

	/** Il nome del ramo, per il log. Non localizzato: e' diagnostica, non interfaccia. */
	const TCHAR* SourceName(ESource Source);

	/**
	 * La tinta dell'anello fantasma.
	 *
	 * 🔑 **Neutra di proposito, e non e' un gusto.** Ogni tinta della tavolozza *dichiara una superficie*
	 * (`URTHexLibrary::SurfaceColor`): un fantasma verde direbbe «rampa», uno giallo «altura». Dove la
	 * griglia di lavoro si posa non c'e' ancora niente, e il segno non deve promettere cosa ci sara'.
	 *
	 * ⚠️ **Sta qui e non in un namespace anonimo del `.cpp` perche' e' verificata invece che dichiarata** —
	 * `RefactorTactics.HexEditor.WorkGridColourIsNotASurfaceColour` la fa passare per i due cancelli di
	 * casa: distanza Manhattan da ogni superficie e dal `(25,25,25)` dell'anello di bordo, e luminanza
	 * Rec.601 a distanza da ciascuna. Una tinta chiusa in un anonimo sarebbe una scelta senza oracolo.
	 */
	FColor GhostColour();

	/**
	 * Tutto cio' da cui la griglia dipende, raccolto in una chiave confrontabile.
	 *
	 * 🔑 **Perche' una chiave e non degli hook.** La griglia va rifatta quando cambia: il dato, il layer
	 * attivo, la scala, la posizione dell'actor, l'actor bersaglio, o un'impostazione. Agganciare un hook per
	 * ciascuno sembra piu' economico e **lascia buchi**, e il buco non si vede: resta una griglia che non
	 * corrisponde piu' a niente, cioe' la vista che mente di #622.
	 *
	 * ⚠️ **Il buco piu' facile da prendere e' il pannello Details.** `RTHexEditorClick.cpp:150` dichiara che
	 * scrivere `ActiveLayer` *da codice* non passa da `PostEditChangeProperty` — il che implica l'opposto per
	 * la via dal pannello, ed e' `ARTHexMapActor::PostEditChangeProperty` (`RTHexMapActor.cpp:935-942`) a
	 * scriverlo: *«Cambiare MapAsset, ActiveLayer, LayerView, DemoRadius, HexSize/LayerHeight o CellMesh
	 * cambia cosa si deve vedere: si ricostruisce sempre»*. Un aggancio al solo `RTHexEditor::SetActiveLayer`
	 * si sarebbe perso ogni cambio di piano fatto dal pannello.
	 *
	 * ⚠️ **Il confronto e' O(1) e gira in `ModeTick`**, che in `UEdMode` e' un no-op virtuale
	 * (`UEdMode.h:160`): l'override non cambia il comportamento di nient'altro. Il lavoro vero — dilatare e
	 * posare — avviene **solo** quando la chiave cambia, cioe' a ogni modifica, non a ogni fotogramma.
	 */
	struct FWatch
	{
		/** `GetUniqueID` dell'actor mappa bersaglio, `0` se non ce n'e' uno. Copre anche il cambio di selezione. */
		uint32 MapActorId = 0;

		/** `URTHexMapAsset::Revision`, che si muove a ogni modifica strutturale (otto siti in `RTHexMapAsset.cpp`). */
		int32 Revision = -1;

		/** Quante celle ha l'asset: ridondante con `Revision`, e costa un `int32`. */
		int32 NumCells = -1;

		int32 ActiveLayer = 0;
		FVector Origin = FVector::ZeroVector;
		float HexSize = 0.f;
		float LayerHeight = 0.f;

		bool bShow = false;
		int32 Margin = -1;
		int32 SeedRadius = -1;

		bool operator==(const FWatch& Other) const;
		bool operator!=(const FWatch& Other) const { return !(*this == Other); }
	};
}
