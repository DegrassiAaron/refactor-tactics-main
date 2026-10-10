#include "UI/RTUIPalette.h"

FLinearColor URTUIPalette::ColorFor(const ERTUIToken Token)
{
	return FLinearColor::FromSRGBColor(SRGBFor(Token));
}

FColor URTUIPalette::SRGBFor(const ERTUIToken Token)
{
	// Gli esadecimali sono quelli di §32. Il test `RefactorTactics.UI.Palette.MatchesStyleGuide` li rilegge
	// da la' e li confronta con questi: un valore cambiato da una parte sola non passa.
	switch (Token)
	{
	case ERTUIToken::BG_Deep:          return FColor::FromHex(TEXT("080F14"));
	case ERTUIToken::BG_Panel:         return FColor::FromHex(TEXT("151A23"));
	case ERTUIToken::BG_Raised:        return FColor::FromHex(TEXT("212733"));
	case ERTUIToken::Frame_Deep:       return FColor::FromHex(TEXT("203542"));
	case ERTUIToken::Frame_Mid:        return FColor::FromHex(TEXT("4A5568"));
	case ERTUIToken::Cyan:             return FColor::FromHex(TEXT("00E0FF"));
	case ERTUIToken::Violet:           return FColor::FromHex(TEXT("7C5CFF"));
	case ERTUIToken::Amber:            return FColor::FromHex(TEXT("FFD456"));
	case ERTUIToken::Red:              return FColor::FromHex(TEXT("FF4D4D"));
	case ERTUIToken::White:            return FColor::FromHex(TEXT("FFFFFF"));
	case ERTUIToken::Text_Primary:     return FColor::FromHex(TEXT("E6EBF2"));
	case ERTUIToken::Text_Secondary:   return FColor::FromHex(TEXT("A9B4C2"));
	case ERTUIToken::Text_Disabled:    return FColor::FromHex(TEXT("6B7684"));
	case ERTUIToken::Frame_Off:        return FColor::FromHex(TEXT("2E3746"));
	case ERTUIToken::BG_Selected:      return FColor::FromHex(TEXT("2B2918"));
	case ERTUIToken::BG_Reaction:      return FColor::FromHex(TEXT("221E3A"));
	case ERTUIToken::BG_Invalid:       return FColor::FromHex(TEXT("2A1719"));
	case ERTUIToken::BG_ProfileActive: return FColor::FromHex(TEXT("0E2A33"));
	case ERTUIToken::Icon_Cooldown:    return FColor::FromHex(TEXT("3A4454"));
	case ERTUIToken::Violet_Light:     return FColor::FromHex(TEXT("B9A8FF"));
	case ERTUIToken::Phase_Prep:       return FColor::FromHex(TEXT("56B4E9"));
	case ERTUIToken::Phase_Dash:       return FColor::FromHex(TEXT("009E73"));
	case ERTUIToken::Phase_Blast:      return FColor::FromHex(TEXT("D55E00"));
	case ERTUIToken::Phase_Move:       return FColor::FromHex(TEXT("0072B2"));
	}
	// Nessun default: un token senza colore non deve ricadere su una tinta di ripiego che a schermo sembra
	// una scelta. `MatchesStyleGuide` scorre l'enum per intero, quindi un caso mancante ferma il test qui.
	checkNoEntry();
	return FColor::Magenta;
}

FString URTUIPalette::TokenName(const ERTUIToken Token)
{
	return TEXT("RT_UI_") + StaticEnum<ERTUIToken>()->GetNameStringByValue(static_cast<int64>(Token));
}
