#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"

/**
 * **La planimetria del Gray Kit Playground** (#1991, Epic #1990, `D-304`) come DATO, non come geometria
 * dispersa in un `.umap`.
 *
 * 🔑 **Perche' vive qui e non solo nella scena.** Otto rettangoli posati a mano dentro un binario sono
 * otto numeri che nessuno puo' diffare, rileggere o verificare: un pad spostato di mezzo metro durante una
 * seduta non produce nessun segnale. Qui invece la planimetria e' una tabella che si legge, si confronta e
 * ha i suoi test — le otto bounding box non si sovrappongono, stanno dentro il floor, e il corridoio non
 * ne tocca nessuna.
 *
 * E' anche cio' che il **pannello** (#1993) consuma per `Focus` e per la riga `station bounds` di
 * `DIAGNOSTICS`. Senza, quegli stessi numeri sarebbero trascritti a mano in un secondo posto — che e' il
 * difetto di `#1459`, dove tre elenchi della stessa cosa non coincidevano con il codice.
 *
 * ⛔ **Nessuna regola di gioco.** Questa e' presentazione d'editor: nessun `FRTCellId`, nessuna cella,
 * nessun costo, nessuna occupancy. Le coordinate sono **metri di scena**, e il loro unico consumatore e'
 * chi guarda.
 *
 * ## L'unita' di misura, e perche' non e' quella di Unreal
 *
 * La planimetria e' in **metri** perche' e' la lingua in cui la roadmap e la seduta la discutono. Unreal
 * misura in centimetri, e `WorldFromMetres` e' l'unico punto in cui le due si toccano: un solo posto da
 * leggere invece di un `* 100` sparso.
 */
namespace RTPlayground
{
	/** Una stazione tematica: il numero che la nomina, il tema, il suo rettangolo, e se e' viva oggi. */
	struct FStation
	{
		/** `1`..`8`, come compare sul signage e nelle chip del pannello. */
		int32 Number = 0;

		/** Il tema, in inglese come nel resto del vocabolario d'editor. */
		const TCHAR* Name = TEXT("");

		/** Rettangolo in **metri di scena**, `Min` in basso a sinistra. */
		FBox2D Bounds = FBox2D(ForceInit);

		/**
		 * `true` solo per la Station 01 in `GKP 0.1`.
		 *
		 * ⚠️ **Non e' decorazione**: una station `PLANNED` esiste come pad e signage e **non finanzia** il
		 * proprio sistema. Il pannello la mostra come chip non operativa, e chi apre la mappa deve poterlo
		 * capire senza chiedere.
		 */
		bool bLive = false;
	};

	/** Il floor: `40 m x 24 m`, `X = -20..+20`, `Y = -12..+12`. */
	FBox2D FloorBounds();

	/** Il corridoio centrale: `Y = -2..+2`, largo 4 m, per tutta la lunghezza del floor. */
	FBox2D CorridorBounds();

	/**
	 * Le due service strip, `Y = +2..+3` e `Y = -3..-2`.
	 *
	 * Stanno **fra** il corridoio e le station, e la loro esistenza e' la ragione per cui il corridoio non
	 * tocca nessun pad: un metro di stacco su ciascun lato.
	 */
	TArray<FBox2D> ServiceStrips();

	/** Le otto stazioni, in ordine di numero. */
	TArray<FStation> Stations();

	/** La stazione con quel numero, o `nullptr`. Numero fuori da `1..8` -> `nullptr`, mai una vuota. */
	const FStation* FindStation(int32 Number);

	/**
	 * 🔴 **Il passo della guida metrica: un metro. E' un RIGHELLO, non una griglia** (`D-304`).
	 *
	 * Non corrisponde a `FRTCellId`, non e' un nodo A\*, non e' topologia, non e' distanza di gameplay,
	 * non e' occupancy, non e' pathfinding. Serve a chi modella per sapere quanto e' grande una cosa.
	 *
	 * ⚠️ **La regola e' `D-304`, non l'aritmetica** — e la distinzione conta, perche' la prima stesura di
	 * quella decisione si appoggiava a un'aritmetica sbagliata. Vedi
	 * `RefactorTactics.Playground.MetreGuideIsNotTheHexPitch`, che misura la relazione vera fra guida e
	 * passo esagonale invece di affermarla.
	 */
	inline constexpr double GuideSpacingMetres = 1.0;

	/** Metri di planimetria -> unita' Unreal. L'unico punto in cui le due misure si toccano. */
	double WorldFromMetres(double Metres);
}
