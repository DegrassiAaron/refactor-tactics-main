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

/**
 * Un carattere POSATO nel mondo: dove sta il suo quadrato unitario e quanto e' grande.
 *
 * `Right` e `Up` sono gia' scalati, quindi il consumatore ottiene il punto mondo di uno stroke con
 * `Origin + Right * S.X + Up * S.Y` — una moltiplicazione, nessuna trigonometria a valle. E' cio' che
 * permette a un secondo disegnatore (mesh, font atlas) di consumare le stesse pose.
 */
struct FRTLabelGlyph
{
	TCHAR   Character = TEXT(' ');
	FVector Origin    = FVector::ZeroVector;
	FVector Right     = FVector::ZeroVector;
	FVector Up        = FVector::ZeroVector;
};

/** Le tre run di una cella, tutte insieme: l'ordine e' run per run, carattere per carattere. */
struct FRTCellLabel
{
	TArray<FRTLabelGlyph> Glyphs;
};
