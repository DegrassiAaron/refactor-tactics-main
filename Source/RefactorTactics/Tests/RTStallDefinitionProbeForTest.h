#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"

/**
 * **Le definizioni di «stallo», messe una accanto all'altra sulla stessa partita** (`BOT-STALL-1`, `#1551`).
 *
 * Il repository ne implementa gia' DUE, ciascuna con la propria verifica di mutazione, e rispondono in modo
 * opposto alla stessa domanda:
 *
 *   `NobodyParksOnTheAuthoredMap`        un turno fermo conta solo se l'unita' e' INERTE — ferma *e* senza
 *                                        aver inflitto danno. E' l'uscita **(a)**
 *   `EngagesOnTheGeneratedTestArena`     un turno fermo conta, punto. E' l'uscita **(b)**
 *
 * `BOT-STALL-1` ne raccomanda una terza — **(c)**, esenzione condizionata all'AVANZAMENTO: un turno fermo
 * non conta se l'unita' ha inflitto danno **e** nella finestra lo stato e' avanzato. E' l'unica delle
 * quattro uscite **per cui non esiste nessuna misura**, ed e' cio' che questa sonda produce.
 *
 * ⛔ **Questa sonda NON decide.** Non e' un oracolo, non porta soglie e non modifica i due esistenti: mette
 * dei numeri accanto a una voce che resta APERTA, e il cui owner e' `PDR-00`. Chi decide sceglie con la
 * misura in mano invece che senza.
 *
 * ## «Avanzamento» reso operativo — e la scelta e' dichiarata, perche' non se ne deduce una sola
 *
 * `OPEN_DECISIONS.md` scrive *«HP nemici calati, o qualcuno caduto»*. Sono **tre** scelte distinte
 * nascoste in una riga, e danno numeri diversi:
 *
 *   `SaluteNetta`           gli `Health` nemici sono calati rispetto all'inizio della finestra
 *   `PoolNetto`             `Health + Shield` nemici sono calati rispetto all'inizio della finestra
 *   `Eliminazione`          un nemico e' caduto durante la finestra
 *   `SaluteOEliminazione`   `SaluteNetta` oppure `Eliminazione` — la lettura piu' generosa della riga
 *
 * 🔴 **Perche' `Health` e `Health + Shield` non sono la stessa domanda, ed e' misurabile e non teorico.**
 * Da `D-224` lo scudo BASE **si ricarica nel Cleanup**. Un'unita' che eroda solo scudo infligge danno a ogni
 * turno — `IsDamageInflictedByActor` risponde **vero** — mentre gli `Health` nemici non calano mai: e'
 * hold-and-shoot che non avanza, ed e' precisamente il caso che la (c) esiste per distinguere dalla (a).
 * Col `PoolNetto` invece la ricarica rientra nella misura e la finestra puo' risultare «avanzata» per un
 * danno che il Cleanup ha gia' restituito.
 *
 * ⛔ **Una quarta lettura e' stata SCARTATA, e resta scritta col motivo**: *«HP calati in QUESTO turno»*,
 * cioe' l'avanzamento misurato turno per turno invece che sulla finestra. Con `Health + Shield` collassa
 * sulla **(a)** per costruzione — danno inflitto implica pool calato nello stesso turno — quindi non
 * sarebbe una terza uscita ma un secondo nome della prima. La (c) si separa dalla (a) **solo** se la
 * finestra e' piu' lunga di un turno: e' questo che la rende una decisione e non una riscrittura.
 *
 * ## La finestra
 *
 * ⚠️ **La finestra e' la sequenza ferma stessa** — i turni consecutivi in cui l'unita' non cambia cella,
 * **indipendentemente dall'essere armata**, cioe' la sequenza della definizione **(b)**. La scelta va
 * dichiarata perche' non e' l'unica possibile e non e' innocua: prendere come finestra la sequenza gia'
 * esentata sarebbe circolare — l'esenzione azzera la sequenza che la definisce.
 *
 * Un turno fermo e' ESENTE sotto una definizione `(c*)` se l'unita' e' armata **e** l'avanzamento e'
 * avvenuto fra l'inizio della finestra e adesso. Un turno esente azzera il contatore, come fa la **(a)**:
 * l'alternativa — non incrementare senza azzerare — cucirebbe insieme due parcheggi separati da un colpo,
 * ed e' una terza semantica ancora, che nessuna delle quattro uscite propone.
 *
 * ⚠️ **La chiave dell'unita' e' del chiamante.** `NobodyParksOnTheAuthoredMap` usa `GetUniqueID()`,
 * `EngagesOnTheGeneratedTestArena` usa `StableUnitId`. Questa sonda non sceglie per loro, come non lo fa
 * `FRTOrbitProbe`.
 *
 * ⚠️ **LIMITE DICHIARATO: lo stato nemico lo calcola il CHIAMANTE.** La sonda non conosce le squadre e non
 * legge il mondo: riceve i tre totali gia' fatti. Cosi' resta pura e verificabile senza un `UWorld`, ma
 * significa anche che un chiamante che sbagli a sommare produce numeri plausibili e falsi — ed e' il motivo
 * per cui i test che la usano portano una guardia di non-vacuita' sui totali.
 */
struct FRTStallDefinitionProbe
{
	/** Le sei letture messe a confronto. Le prime due sono gia' nel repository, le altre quattro no. */
	enum class EDefinizione : uint8
	{
		/** (b) — un turno fermo conta sempre. `EngagesOnTheGeneratedTestArena`. */
		Immobilita = 0,
		/** (a) — conta solo se l'unita' non ha inflitto danno. `NobodyParksOnTheAuthoredMap`. */
		Sterile,
		/** (c) — esente se armata e gli `Health` nemici sono calati nella finestra. */
		SaluteNetta,
		/** (c) — esente se armata e `Health + Shield` nemici sono calati nella finestra. */
		PoolNetto,
		/** (c) — esente se armata e un nemico e' caduto nella finestra. */
		Eliminazione,
		/** (c) — esente se armata e (`SaluteNetta` oppure `Eliminazione`). */
		SaluteOEliminazione,

		Count
	};

	static constexpr int32 NumDefinizioni = static_cast<int32>(EDefinizione::Count);

	static const TCHAR* NomeDi(EDefinizione D)
	{
		switch (D)
		{
		case EDefinizione::Immobilita:          return TEXT("(b) immobilita'");
		case EDefinizione::Sterile:             return TEXT("(a) immobilita' sterile");
		case EDefinizione::SaluteNetta:         return TEXT("(c) salute netta");
		case EDefinizione::PoolNetto:           return TEXT("(c) pool netto (salute+scudo)");
		case EDefinizione::Eliminazione:        return TEXT("(c) eliminazione");
		case EDefinizione::SaluteOEliminazione: return TEXT("(c) salute o eliminazione");
		default:                                return TEXT("(sconosciuta)");
		}
	}

	/** I totali della squadra AVVERSARIA all'unita' osservata, gia' sommati dal chiamante. */
	struct FStatoNemico
	{
		int32 Salute = 0;
		int32 Pool = 0;
		int32 Vivi = 0;
	};

	/**
	 * Un'osservazione: una unita', a fine di un turno risolto.
	 *
	 * Va chiamata **una volta per unita' viva per turno**, e nell'ordine dei turni: la sonda tiene la cella
	 * precedente e i totali d'inizio finestra, e non ha modo di accorgersi di una chiamata saltata.
	 */
	void Observe(int32 UnitKey, const FRTCellId& Cell, bool bArmato, const FStatoNemico& Nemici)
	{
		FPerUnita& U = Unita.FindOrAdd(UnitKey);
		const bool bFerma = U.bHaPrecedente && U.Precedente == Cell;

		if (!bFerma)
		{
			// Nuova finestra: si riparte, e i totali di adesso diventano il riferimento.
			U.InizioSalute = Nemici.Salute;
			U.InizioPool = Nemici.Pool;
			U.InizioVivi = Nemici.Vivi;
			for (int32 I = 0; I < NumDefinizioni; ++I) { U.Sequenza[I] = 0; }
		}
		else
		{
			const bool bSaluteCalata = Nemici.Salute < U.InizioSalute;
			const bool bPoolCalato = Nemici.Pool < U.InizioPool;
			const bool bCaduto = Nemici.Vivi < U.InizioVivi;

			Avanza(U, EDefinizione::Immobilita,          false);
			Avanza(U, EDefinizione::Sterile,             bArmato);
			Avanza(U, EDefinizione::SaluteNetta,         bArmato && bSaluteCalata);
			Avanza(U, EDefinizione::PoolNetto,           bArmato && bPoolCalato);
			Avanza(U, EDefinizione::Eliminazione,        bArmato && bCaduto);
			Avanza(U, EDefinizione::SaluteOEliminazione, bArmato && (bSaluteCalata || bCaduto));
		}

		U.Precedente = Cell;
		U.bHaPrecedente = true;

		for (int32 I = 0; I < NumDefinizioni; ++I)
		{
			Record[I] = FMath::Max(Record[I], U.Sequenza[I]);
		}
	}

	/** La sequenza piu' lunga vista su una qualsiasi unita', sotto la definizione data. */
	int32 Peggiore(EDefinizione D) const { return Record[static_cast<int32>(D)]; }

	/** Quante unita' sono state osservate: una guardia contro il verde su zero osservazioni. */
	int32 UnitaOsservate() const { return Unita.Num(); }

private:
	struct FPerUnita
	{
		FRTCellId Precedente;
		bool bHaPrecedente = false;
		int32 InizioSalute = 0;
		int32 InizioPool = 0;
		int32 InizioVivi = 0;
		int32 Sequenza[NumDefinizioni] = {};
	};

	static void Avanza(FPerUnita& U, EDefinizione D, bool bEsente)
	{
		int32& S = U.Sequenza[static_cast<int32>(D)];
		S = bEsente ? 0 : S + 1;
	}

	TMap<int32, FPerUnita> Unita;
	int32 Record[NumDefinizioni] = {};
};
