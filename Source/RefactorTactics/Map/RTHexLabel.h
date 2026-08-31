#pragma once

#include "CoreMinimal.h"

/**
 * Un segmento di un carattere, in coordinate NORMALIZZATE dentro il quadrato unitario.
 *
 * 🔑 Normalizzate e non in centimetri: la forma del carattere non sa quanto sara' grande, e chi la posa
 * non sa che forma ha. E' il confine che permette di cambiare il disegnatore senza toccare la geometria.
 */
struct FRTLabelStroke
{
	FVector2D From = FVector2D::ZeroVector;
	FVector2D To   = FVector2D::ZeroVector;
};
