#include "UI/RTHudZoneWidget.h"

FLinearColor URTHudZoneWidget::BlockoutColor(ERTHudZone Zone)
{
	const uint8 Indice = static_cast<uint8>(Zone);
	if (Indice >= static_cast<uint8>(ERTHudZone::Count))
	{
		// Magenta pieno: il colore che non appartiene a nessuna zona, per un valore che non e' una zona.
		return FLinearColor(1.f, 0.f, 1.f, 1.f);
	}

	// Otto tonalita' equispaziate sul cerchio, saturazione e valore pieni. `FLinearColor::MakeFromHSV8`
	// vuole la tinta su 0-255, non su 0-360.
	//
	// ⚠️ **L'aritmetica e' in `int32` per scelta, non per caso.** `Indice` e' un `uint8` e la promozione
	// intera farebbe comunque il lavoro, ma `Indice * 256` scritto su `uint8` sembra un overflow a chi
	// legge: reso esplicito, non c'e' niente da dedurre. Le tinte sono i multipli di `256 / Count`, e cambiano
	// tutte quando cambia il numero di zone — come il 2026-10-04, con la fascia bassa unica ([D-456]).
	const int32 Quante = static_cast<int32>(ERTHudZone::Count);
	const int32 Tinta = (static_cast<int32>(Indice) * 256) / Quante;
	return FLinearColor::MakeFromHSV8(static_cast<uint8>(Tinta), 255, 255);
}

FString URTHudZoneWidget::ZoneName(ERTHudZone Zone)
{
	switch (Zone)
	{
	case ERTHudZone::TopLeft:      return TEXT("TopLeft");
	case ERTHudZone::TopCenter:    return TEXT("TopCenter");
	case ERTHudZone::TopRight:     return TEXT("TopRight");
	case ERTHudZone::MiddleLeft:   return TEXT("MiddleLeft");
	case ERTHudZone::MiddleRight:  return TEXT("MiddleRight");
	case ERTHudZone::Bottom:       return TEXT("Bottom");
	default:                       return TEXT("<non e' una zona>");
	}
}
