#include "RTHexTransitionGlyph.h"

#include "RTHexEditorClick.h"

namespace RTHexTransition
{
	int32 TicksFor(ERTHexTransitionKind Kind)
	{
		// ⛔ **Sei valori distinti, ed è l'unica proprietà che conta**: due tipi con lo stesso numero di
		// tacche tornerebbero a distinguersi per la sola tinta, cioè al difetto. `KindHasTwoChannels` lo
		// misura contando gli usciti distinti, non confrontandoli con questa tabella — un test che
		// ripetesse questi numeri direbbe solo che so copiare.
		switch (Kind)
		{
		case ERTHexTransitionKind::Stair:    return 1;
		case ERTHexTransitionKind::Ramp:     return 2;
		case ERTHexTransitionKind::Bridge:   return 3;
		case ERTHexTransitionKind::Tunnel:   return 4;
		case ERTHexTransitionKind::Elevator: return 5;
		case ERTHexTransitionKind::Jump:     return 6;
		default:                             return 0;
		}
	}

	FGlyph Describe(const FRTHexEdge& Edge)
	{
		FGlyph G;

		// ⛔ Gli estremi si COPIANO. Il verso di un arco è un dato — `From` e `To` non sono
		// intercambiabili — e una resa che li scambiasse racconterebbe una scala che sale al contrario.
		G.From = Edge.From;
		G.To = Edge.To;

		// ⛔ La tinta non si conia qui: è quella di `TransitionKindColor`, che il tool Arch usa dal 2026-08.
		// Una seconda tabella di sei colori sarebbe una seconda risposta alla stessa domanda, e prima o poi
		// le due si separerebbero.
		G.Tint = RTHexEditor::TransitionKindColor(Edge.Kind);
		G.Ticks = TicksFor(Edge.Kind);

		// ⚠️ **`Inactive` e `Destroyed` NON sono due gradazioni.** Entrambi dicono «non si passa» — ed è
		// perché il tratteggio li accomuna — ma `Destroyed` è **terminale** e `Inactive` si riattiva. La
		// barra dice quella differenza, che è l'unica che chi costruisce deve poter agire.
		switch (Edge.State)
		{
		case ERTHexArcState::Active:
			G.Stroke = EStroke::Solid;
			G.bCrossed = false;
			break;
		case ERTHexArcState::Inactive:
			G.Stroke = EStroke::Dashed;
			G.bCrossed = false;
			break;
		case ERTHexArcState::Destroyed:
			G.Stroke = EStroke::Dashed;
			G.bCrossed = true;
			break;
		default:
			break;
		}

		return G;
	}

	float Luminance(const FColor& C)
	{
		// Rec.709, la stessa che una conversione a scala di grigi applica.
		return 0.2126f * C.R + 0.7152f * C.G + 0.0722f * C.B;
	}
}
