#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/RTUIPalette.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTUIPaletteTests
{
	FString StyleGuidePath()
	{
		return FPaths::Combine(FPaths::ProjectDir(), TEXT("docs/technical/systems/progettazione-hud.md"));
	}

	/** Il testo fra due backtick di una cella: `` `RT_UI_BG_Deep` `` -> `RT_UI_BG_Deep`. Vuoto se non ce n'e'. */
	FString FraBacktick(const FString& Cella)
	{
		int32 Apre = INDEX_NONE;
		int32 Chiude = INDEX_NONE;
		if (!Cella.FindChar(TEXT('`'), Apre) || !Cella.FindLastChar(TEXT('`'), Chiude) || Chiude <= Apre + 1)
		{
			return FString();
		}
		return Cella.Mid(Apre + 1, Chiude - Apre - 1);
	}

	/**
	 * Le righe `| `RT_UI_…` | `#RRGGBB` | uso |` di §32, dal titolo `# 32.` al titolo `# 33.`.
	 *
	 * ⚠️ Solo §32: `RT_UI_Panel_*` di §34 e' un nome di asset, non un token colore, e porta lo stesso prefisso.
	 * Leggere l'intero documento renderebbe questo test rosso su cio' che e' corretto.
	 */
	TArray<TPair<FString, FString>> RigheDiPalette(const FString& Documento)
	{
		TArray<TPair<FString, FString>> Righe;
		TArray<FString> Linee;
		Documento.ParseIntoArrayLines(Linee, /*CullEmpty=*/ false);
		bool bDentro = false;
		for (const FString& Linea : Linee)
		{
			if (Linea.StartsWith(TEXT("# 32."))) { bDentro = true; continue; }
			if (!bDentro) { continue; }
			if (Linea.StartsWith(TEXT("# 33."))) { break; }
			if (!Linea.StartsWith(TEXT("| `RT_UI_"))) { continue; }

			TArray<FString> Celle;
			Linea.ParseIntoArray(Celle, TEXT("|"), /*CullEmpty=*/ true);
			if (Celle.Num() < 2) { continue; }
			Righe.Emplace(FraBacktick(Celle[0]), FraBacktick(Celle[1]));
		}
		return Righe;
	}
}

/**
 * **La palette a runtime e §32 dicono la stessa cosa** ([D-489], #3610).
 *
 * §32 e' la decisione, `URTUIPalette` la sede a runtime. Gli esadecimali attesi **non** sono ricopiati qui:
 * si rileggono dal documento, cosi' un colore cambiato da una parte sola fa fallire il test invece di
 * aggiungere una terza copia da tenere allineata.
 *
 * Il confronto va nei due versi:
 * - ogni valore di `ERTUIToken` ha la sua riga in §32, con lo stesso esadecimale. Scorrere l'enum per intero
 *   e' anche cio' che passa per ogni caso dello `switch` di `SRGBFor`;
 * - ogni riga `RT_UI_…` di §32 ha il suo valore nell'enum. Un token deciso e non portato a runtime fallisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUIPaletteMatchesStyleGuideTest,
	"RefactorTactics.UI.Palette.MatchesStyleGuide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUIPaletteMatchesStyleGuideTest::RunTest(const FString&)
{
	using namespace RTUIPaletteTests;

	FString Documento;
	if (!TestTrue(TEXT("progettazione-hud.md e' leggibile"), FFileHelper::LoadFileToString(Documento, *StyleGuidePath())))
	{
		return false;
	}
	const TArray<TPair<FString, FString>> Righe = RigheDiPalette(Documento);
	if (!TestTrue(TEXT("premessa: §32 ha righe di token"), Righe.Num() > 0))
	{
		return false;
	}

	TMap<FString, FString> HexDiToken;
	for (const TPair<FString, FString>& Riga : Righe)
	{
		TestFalse(*FString::Printf(TEXT("§32 elenca `%s` una volta sola"), *Riga.Key), HexDiToken.Contains(Riga.Key));
		TestTrue(*FString::Printf(TEXT("§32: `%s` ha un esadecimale `#RRGGBB`, non `%s`"), *Riga.Key, *Riga.Value),
			Riga.Value.Len() == 7 && Riga.Value.StartsWith(TEXT("#")));
		HexDiToken.Add(Riga.Key, Riga.Value);
	}

	const UEnum* Enum = StaticEnum<ERTUIToken>();
	TSet<FString> NomiDellEnum;
	for (int32 i = 0; i < Enum->NumEnums() - 1; ++i) // l'ultimo e' `_MAX`, aggiunto da UHT
	{
		const ERTUIToken Token = static_cast<ERTUIToken>(Enum->GetValueByIndex(i));
		const FString Nome = URTUIPalette::TokenName(Token);
		NomiDellEnum.Add(Nome);

		const FString* Hex = HexDiToken.Find(Nome);
		if (!TestNotNull(*FString::Printf(TEXT("`%s` e' nell'enum ma non in §32"), *Nome), Hex))
		{
			continue;
		}
		const FColor Atteso = FColor::FromHex(*Hex);
		const FColor Reale = URTUIPalette::SRGBFor(Token);
		TestEqual(*FString::Printf(TEXT("`%s`: §32 dice %s, la palette #%s"), *Nome, **Hex,
			*Reale.ToHex().Left(6)), Reale, Atteso);
	}

	for (const TPair<FString, FString>& Riga : HexDiToken)
	{
		TestTrue(*FString::Printf(TEXT("`%s` e' in §32 ma non nell'enum"), *Riga.Key), NomiDellEnum.Contains(Riga.Key));
	}
	return true;
}

/**
 * **`ColorFor` restituisce il colore LINEARE che corrisponde all'sRGB di §32.**
 *
 * 🔑 Il difetto che conta e' plausibile e silenzioso: `FLinearColor(R / 255.f, …)` tratta l'sRGB come lineare
 * e da' una tinta slavata, che a schermo sembra soltanto un altro grigio. Il ritorno a sRGB deve ridare
 * l'esadecimale esatto, e il valore lineare deve essere diverso da quello ingenuo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUIPaletteDecodesSRGBTest,
	"RefactorTactics.UI.Palette.ColorIsDecodedFromSRGB",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUIPaletteDecodesSRGBTest::RunTest(const FString&)
{
	const UEnum* Enum = StaticEnum<ERTUIToken>();
	for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
	{
		const ERTUIToken Token = static_cast<ERTUIToken>(Enum->GetValueByIndex(i));
		const FColor SRGB = URTUIPalette::SRGBFor(Token);
		TestEqual(*FString::Printf(TEXT("`%s`: torna all'esadecimale di partenza"), *URTUIPalette::TokenName(Token)),
			URTUIPalette::ColorFor(Token).ToFColorSRGB(), SRGB);
		TestEqual(*FString::Printf(TEXT("`%s`: e' opaco"), *URTUIPalette::TokenName(Token)), (int32)SRGB.A, 255);
	}

	const FLinearColor Testo = URTUIPalette::ColorFor(ERTUIToken::Text_Primary);
	const FLinearColor Ingenuo(0xE6 / 255.f, 0xEB / 255.f, 0xF2 / 255.f);
	TestFalse(TEXT("`RT_UI_Text_Primary` non e' l'sRGB letto come lineare"), Testo.Equals(Ingenuo, 0.01f));
	TestEqual(TEXT("il nome del token porta il prefisso di §32"),
		URTUIPalette::TokenName(ERTUIToken::Text_Primary), FString(TEXT("RT_UI_Text_Primary")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
