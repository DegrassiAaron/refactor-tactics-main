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
