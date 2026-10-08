#pragma once

#include "CoreMinimal.h"
#include "Map/RTCellId.h"
#include "RTPlaybackTracer.generated.h"

/**
 * La forma di un tracer di playback (`#2454`, spec `2026-10-07-tracer-attacco-base`).
 *
 * ⛔ **Il canale e' la GEOMETRIA, mai il solo colore**: le due forme hanno lo stesso colore della palette e si
 * distinguono perche' il getto resta ancorato all'origine e il proiettile se ne stacca.
 */
UENUM(BlueprintType)
enum class ERTTracerStyle : uint8
{
	/** Nessun tracer: colpo non idoneo, oppure chi guarda non conosceva uno dei due estremi. */
	None,
	/** Un segmento corto che si sposta dall'origine all'impatto (`ERTAbilityShape::Single`). */
	Projectile,
	/** Un segmento che si allunga dall'origine, ancorato a essa (`ERTAbilityShape::Line`). */
	Jet,
	/**
	 * ➕ #3578 (spec «il profilo FX» §2.1). Un getto SPEZZATO: ancorato all'origine come `Jet`, con i vertici funzione
	 * di (From, To) soltanto — nessun `Rand`, nessun orologio — quindi a `α` minore e' un PREFISSO di quello a `α`
	 * maggiore (`URTPlaybackLibrary::TracerPolyline`). Un solo consumatore oggi: `Hero.Aevik.LinearDischarge` (D5).
	 */
	Zigzag,
};

/**
 * Un tracer in volo nel fotogramma corrente: estremi in CELLE, avanzamento in [0,1].
 *
 * 🔑 Le celle e non le posizioni degli attori: la posizione visiva di un'unita' poco osservata puo' essere
 * stantia, e un estremo letto dall'attore sarebbe presentazione che si fa passare per fatto (spec §2.2).
 */
USTRUCT()
struct FRTPlaybackTracer
{
	GENERATED_BODY()

	UPROPERTY()
	FRTCellId From;

	UPROPERTY()
	FRTCellId To;

	UPROPERTY()
	float Alpha = 0.f;

	UPROPERTY()
	ERTTracerStyle Style = ERTTracerStyle::None;
};

/** Il segnale di attivazione sulla sorgente (`#3578`, spec §2.1, D1, D3). */
UENUM(BlueprintType)
enum class ERTActivationFxStyle : uint8
{
	None,
	/** Un esagono che si allarga. Il default di ogni forma. */
	Ring,
	/** Due esagoni concentrici che si stringono: sostegno, guardia. */
	Pulse,
	/** Sei raggi inclinati di 45° verso l'alto e l'esterno: un lampo, nessun verso (F21). */
	Flash,
};

/** La cue di OGNI `Attack`, all'arrivo, sulla sua vittima (spec §2.1, F5, F6). */
UENUM(BlueprintType)
enum class ERTImpactFxStyle : uint8
{
	None,
	/** Quattro raggi a X che crescono dal centro della vittima. */
	Marker,
};

/** La cue dell'IMPRONTA dell'atto: una per `AttackFootprint`, all'arrivo del primo colpo che la segue (spec §2.4). */
UENUM(BlueprintType)
enum class ERTFootprintFxStyle : uint8
{
	None,
	/** Un esagono a raggi che si allarga dal centro dell'area (`AimCell` dell'impronta). */
	AreaPulse,
	/** Un braccio che ruota da −60° a +60° attorno all'asse `Origin → AimCell` dell'impronta. */
	ConeSweep,
};

/**
 * Il profilo FX di un'azione (`#3578`, spec §2.1, D2). Presentazione pura: non entra in snapshot, TurnLog, StateHash,
 * replay.
 *
 * 🔑 **Default TUTTI `None`** (F17): una voce si costruisce con `URTPresentationBindingLibrary::MakeFxProfile`,
 * quattro argomenti obbligatori, cosi' una riga che dimentica un campo non eredita in silenzio.
 * ⛔ Le durate NON sono qui: sono ritmo (R2, D-287 punto 7), manopole di `ARTTurnManager`.
 */
USTRUCT()
struct FRTAbilityFxProfile
{
	GENERATED_BODY()

	UPROPERTY()
	ERTActivationFxStyle Activation = ERTActivationFxStyle::None;

	UPROPERTY()
	ERTTracerStyle Tracer = ERTTracerStyle::None;

	UPROPERTY()
	ERTImpactFxStyle Impact = ERTImpactFxStyle::None;

	UPROPERTY()
	ERTFootprintFxStyle Footprint = ERTFootprintFxStyle::None;

	bool operator==(const FRTAbilityFxProfile& Other) const
	{
		return Activation == Other.Activation && Tracer == Other.Tracer && Impact == Other.Impact
			&& Footprint == Other.Footprint;
	}
};

/** Il tipo di una cue di playback (spec §2.4, R10): UN campo di tipo, non un'unione con un invariante. */
UENUM()
enum class ERTPlaybackCueKind : uint8
{
	Ring, Pulse, Flash, Marker, AreaPulse, ConeSweep,
};

/**
 * Una cue nel fotogramma corrente: celle, non posizioni di attori (la stessa ragione di `FRTPlaybackTracer`).
 * La consegna `ARTTurnManager::PushPlaybackCues` e la SOSTITUISCE in blocco: e' funzione dell'orologio.
 */
USTRUCT()
struct FRTPlaybackCue
{
	GENERATED_BODY()

	UPROPERTY()
	ERTPlaybackCueKind Kind = ERTPlaybackCueKind::Ring;

	/** Sorgente (attivazione) | vittima (`Marker`) | centro (`AreaPulse`) | origine del ventaglio (`ConeSweep`). */
	UPROPERTY()
	FRTCellId At;

	/** Solo `ConeSweep`: l'`AimCell` dell'impronta. Per gli altri tipi vale `At`. */
	UPROPERTY()
	FRTCellId Toward;

	UPROPERTY()
	float Alpha = 0.f;
};
