#pragma once

#include "CoreMinimal.h"
#include "Map/RTHexCellData.h"

/**
 * COME SI DISEGNA UN ARCO DI TRANSIZIONE (#1768) — funzione pura, nessun viewport.
 *
 * 🔑 **Il difetto che questo file chiude è misurato, non dedotto.** `TransitionKindColor` distingue i sei
 * `Kind` con la **sola tinta**, e due di quelle tinte sono lo stesso grigio: `Tunnel` (200,120,255) e
 * `Elevator` (255,120,120) hanno luminanza Rec.709 **146.8** e **148.7** — `1.9` su `255`. In scala di
 * grigi non si distinguono, ed è precisamente ciò che `D-146` vieta: *«l'encoding è ridondante: mai solo
 * il colore»*. Il test `KindSurvivesGrayscale` lo misura invece di ripeterlo.
 *
 * ⛔ **E `ERTHexArcState` non era mostrato affatto**: un ponte abbattuto si disegnava identico a uno
 * percorribile. `Inactive` e `Destroyed` sono indistinguibili per il grafo ma **differiscono per la
 * reversibilità** — uno si riaccende, l'altro è terminale — quindi non possono essere due gradazioni
 * della stessa cosa.
 *
 * ⚠️ **Perché una struct pura invece di disegnare e basta.** L'oracolo di *«si distingue»* non esiste
 * nell'harness e la issue vieta di simularlo: la leggibilità resta voce di seduta. Ciò che un test **può**
 * vedere è il confine — che i canali siano due, che siano distinti, e che nessuno stato si distingua per
 * sola opacità. È lo stesso pattern di `RTHexAnchorReadout` (#1895), `RTHexProbeReadout` (#711) e
 * `RTHexLos` (#1755): una struct che **riceve** il dato e lo traduce, testabile senza aprire un viewport.
 */
namespace RTHexTransition
{
	/** Il tratto con cui l'arco si disegna. Primo canale per lo **stato**. */
	enum class EStroke : uint8
	{
		/** `Active`: si passa. */
		Solid,
		/** `Inactive` e `Destroyed`: non si passa. Che siano due cose diverse lo dice `bCrossed`. */
		Dashed
	};

	/**
	 * Il modello di presentazione di un arco: estremi, tipo, stato.
	 *
	 * ⛔ **Non c'è un campo «opacità», e l'assenza è il punto.** Il DoD chiede che `Active`, `Inactive` e
	 * `Destroyed` si distinguano *«e la distinzione **non** sia la sola opacità»*. Un canale che non esiste
	 * non può diventare l'unico, e `StateDoesNotRelyOnOpacity` lo presidia.
	 */
	struct FGlyph
	{
		/** Gli estremi, copiati dal dato. Il disegno li converte in mondo: qui non si calcola niente. */
		FRTCellId From;
		FRTCellId To;

		/** Primo canale per il **tipo**: la tinta storica di `TransitionKindColor`, invariata. */
		FColor Tint = FColor::White;

		/**
		 * Secondo canale per il **tipo**: quante tacche perpendicolari si posano lungo l'arco, `1`…`6`.
		 *
		 * 🔑 È ciò che sopravvive alla scala di grigi, ed è la ragione per cui non è un'altra tinta.
		 */
		int32 Ticks = 0;

		/** Primo canale per lo **stato**. */
		EStroke Stroke = EStroke::Solid;

		/** Secondo canale per lo stato: **solo** `Destroyed`, che è terminale e non si riattiva. */
		bool bCrossed = false;

		bool operator==(const FGlyph& Other) const = default;
	};

	/** Le tacche di un tipo: `1`…`6`, una per valore dell'enum, nell'ordine in cui è dichiarato. */
	int32 TicksFor(ERTHexTransitionKind Kind);

	/** Come si legge l'arco. Pura: stesso `FRTHexEdge` → stesso `FGlyph`. */
	FGlyph Describe(const FRTHexEdge& Edge);

	/**
	 * Luminanza Rec.709 di una tinta, `0`…`255`.
	 *
	 * ⚠️ **Esiste per il test, e lo dichiara.** È il modo in cui *«verifica in scala di grigi»* — le parole
	 * del DoD — diventa un'asserzione invece di un'impressione. Non la chiama nessun percorso di disegno.
	 */
	float Luminance(const FColor& C);
}
