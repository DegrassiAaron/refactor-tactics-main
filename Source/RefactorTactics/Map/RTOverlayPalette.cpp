#include "Map/RTOverlayPalette.h"

// ⛔ I valori sono quelli GIA' IN PRODUZIONE, ricopiati qui una volta sola dal corpo di
// `ARTHexMapActor::DrawPlanningPreview` e cancellati di la'. Non e' la palette della spec v0.2: quella
// richiede la decisione di palette che #1941 tiene aperta. Vedi il commento della classe.

FColor URTOverlayPalette::ColorFor(const ERTOverlayMeaning Meaning)
{
	switch (Meaning)
	{
	case ERTOverlayMeaning::Movement:        return FColor(53, 199, 89);   // #35C759 [D-368]
	case ERTOverlayMeaning::PathTrace:       return FColor(40, 220, 220);
	case ERTOverlayMeaning::AttackOriginAim: return FColor(220, 220, 255);
	case ERTOverlayMeaning::Attack:          return FColor(255, 69, 58);   // #FF453A [D-368]
	case ERTOverlayMeaning::FriendlyFire:    return FColor(250, 155, 10);  // #FA9B0A [D-368]
	case ERTOverlayMeaning::Hover:           return FColor(255, 214, 10);  // #FFD60A [D-368]
	}
	// Nessun default nello switch: cosi' un significato nuovo rompe la COMPILAZIONE invece di cadere in
	// silenzio su un colore di ripiego che a schermo sembra una scelta.
	checkNoEntry();
	return FColor::Black;
}

uint8 URTOverlayPalette::AlphaFor(const ERTOverlayCertainty Certainty)
{
	switch (Certainty)
	{
	case ERTOverlayCertainty::Confirmed: return 255;
	case ERTOverlayCertainty::Predicted: return 160;
	case ERTOverlayCertainty::Uncertain: return 90;
	}
	checkNoEntry();
	return 255;
}

FColor URTOverlayPalette::ColorForArea(const ERTOverlayMeaning Meaning, const ERTOverlayCertainty Certainty)
{
	FColor Color = ColorFor(Meaning);
	Color.A = AlphaFor(Certainty);
	return Color;
}

int32 URTOverlayPalette::PriorityFor(const ERTOverlayMeaning Meaning)
{
	// «Dal meno al piu' urgente»: 1) dove POSSO andare 2) dove VADO 3) da dove sparo 4) chi COLPISCO
	// 5) cosa sto indicando. E' l'ordine che il commento di `DrawPlanningPreview` descriveva a parole.
	switch (Meaning)
	{
	case ERTOverlayMeaning::Movement:        return 10;
	case ERTOverlayMeaning::PathTrace:       return 20;
	case ERTOverlayMeaning::AttackOriginAim: return 30;
	case ERTOverlayMeaning::Attack:          return 40;
	case ERTOverlayMeaning::FriendlyFire:    return 45;
	case ERTOverlayMeaning::Hover:           return 50;
	}
	checkNoEntry();
	return 0;
}

float URTOverlayPalette::ScaleFor(const ERTOverlayMeaning Meaning)
{
	switch (Meaning)
	{
	case ERTOverlayMeaning::Movement:        return 0.52f;
	case ERTOverlayMeaning::PathTrace:       return 0.72f;
	case ERTOverlayMeaning::AttackOriginAim: return 0.58f;
	case ERTOverlayMeaning::Attack:          return 0.68f;
	case ERTOverlayMeaning::FriendlyFire:    return 0.80f;
	case ERTOverlayMeaning::Hover:           return 0.88f;
	}
	checkNoEntry();
	return 1.f;
}

bool URTOverlayPalette::DrawsThroughUnits(const ERTOverlayMeaning Meaning)
{
	switch (Meaning)
	{
	case ERTOverlayMeaning::Movement:        return false;
	case ERTOverlayMeaning::PathTrace:       return false;
	case ERTOverlayMeaning::AttackOriginAim: return true;
	case ERTOverlayMeaning::Attack:          return true;
	case ERTOverlayMeaning::FriendlyFire:    return true;
	case ERTOverlayMeaning::Hover:           return true;
	}
	checkNoEntry();
	return false;
}

TArray<ERTOverlayMeaning> URTOverlayPalette::AllMeanings()
{
	// Elenco esplicito, come gia' fa `SurfaceColorsAreDistinguishable`: l'enum non dichiara `TEnumRange`, e
	// aggiungerlo per comodita' di test cambierebbe un tipo di dominio. Che l'elenco resti allineato all'enum
	// lo verifica `PaletteCoversEveryMeaning`, che legge `StaticEnum`.
	return {
		ERTOverlayMeaning::Movement,
		ERTOverlayMeaning::PathTrace,
		ERTOverlayMeaning::AttackOriginAim,
		ERTOverlayMeaning::Attack,
		ERTOverlayMeaning::FriendlyFire,
		ERTOverlayMeaning::Hover
	};
}

TArray<ERTOverlayCertainty> URTOverlayPalette::AllCertainties()
{
	return {
		ERTOverlayCertainty::Confirmed,
		ERTOverlayCertainty::Predicted,
		ERTOverlayCertainty::Uncertain
	};
}
