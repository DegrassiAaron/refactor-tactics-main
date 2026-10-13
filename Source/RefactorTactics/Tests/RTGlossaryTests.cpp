// Il glossario di stati, superfici e ripari (`#3744`): ogni cosa che il gioco puo' nominare ha un nome e un effetto,
// e ogni voce nomina qualcosa che esiste.

#include "Misc/AutomationTest.h"
#include "Core/RTGameplayTags.h"
#include "GameplayTagsManager.h"
#include "Map/RTHexCellData.h"
#include "UI/RTGlossary.h"
#include "UObject/Class.h" // UEnum: il file deve compilare anche fuori dal blob unity

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */

	/** Le chiavi degli stati: ogni tag registrato sotto `Status.`, come li enumera `URTIconLibrary`. */
	TArray<FName> GlossaryStatusKeys()
	{
		TArray<FName> Out;
		const FGameplayTag StatusRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Status")), /*ErrorIfNotFound*/ false);
		if (StatusRoot.IsValid())
		{
			TArray<FGameplayTag> Tags;
			UGameplayTagsManager::Get().RequestGameplayTagChildren(StatusRoot).GetGameplayTagArray(Tags);
			for (const FGameplayTag& Tag : Tags)
			{
				Out.Add(RTGlossary::KeyForStatus(Tag));
			}
		}
		return Out;
	}

	/** Le chiavi delle superfici: ogni valore di `ERTHexSurface`. */
	TArray<FName> GlossarySurfaceKeys()
	{
		TArray<FName> Out;
		const UEnum* Enum = StaticEnum<ERTHexSurface>();
		// `NumEnums() - 1`: l'ultimo e' il `_MAX` sintetico che UHT aggiunge.
		for (int32 I = 0; I < Enum->NumEnums() - 1; ++I)
		{
			Out.Add(RTGlossary::KeyForSurface(static_cast<ERTHexSurface>(Enum->GetValueByIndex(I))));
		}
		return Out;
	}

	/** Le chiavi dei ripari: ogni valore di `ERTHexCoverType` tranne `None`, che non e' un riparo. */
	TArray<FName> GlossaryCoverKeys()
	{
		TArray<FName> Out;
		const UEnum* Enum = StaticEnum<ERTHexCoverType>();
		for (int32 I = 0; I < Enum->NumEnums() - 1; ++I)
		{
			const ERTHexCoverType Type = static_cast<ERTHexCoverType>(Enum->GetValueByIndex(I));
			if (Type != ERTHexCoverType::None)
			{
				Out.Add(RTGlossary::KeyForCover(Type));
			}
		}
		return Out;
	}

	/** Le regole di un testo: non vuoto, solo ASCII stampabile, nessuna cifra, nessun nome di tag. */
	void GlossaryCheckText(FAutomationTestBase& Test, const FString& Key, const TCHAR* Quale, const FString& Testo)
	{
		Test.TestFalse(*FString::Printf(TEXT("%s: il %s non e' vuoto"), *Key, Quale), Testo.TrimStartAndEnd().IsEmpty());
		bool bSoloAscii = true;
		bool bSenzaCifre = true;
		// Per indice e non con un `for` a intervallo: qui conta ogni carattere della stringa e nessun terminatore.
		for (int32 I = 0; I < Testo.Len(); ++I)
		{
			const TCHAR C = Testo[I];
			bSoloAscii = bSoloAscii && C >= 0x20 && C <= 0x7E;
			bSenzaCifre = bSenzaCifre && !FChar::IsDigit(C);
		}
		Test.TestTrue(*FString::Printf(TEXT("%s: il %s e' ASCII, con l'apostrofo al posto dell'accento"), *Key, Quale), bSoloAscii);
		Test.TestTrue(*FString::Printf(TEXT("%s: il %s non porta cifre"), *Key, Quale), bSenzaCifre);
		for (const TCHAR* Prefisso : { TEXT("Status."), TEXT("Terrain."), TEXT("Structure.") })
		{
			Test.TestFalse(*FString::Printf(TEXT("%s: il %s non nomina un tag (%s)"), *Key, Quale, Prefisso),
				Testo.Contains(Prefisso, ESearchCase::IgnoreCase));
		}
	}
}

/**
 * OGNI STATO, OGNI SUPERFICIE E OGNI RIPARO HA UNA VOCE - `#3744`.
 *
 * 🔑 **Le tre sorgenti sono quelle del gioco, non un elenco scritto qui**: i tag registrati sotto `Status.` e i
 * valori dei due enum. Uno stato nuovo senza testo fa cadere questo test il giorno in cui viene registrato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlossaryCoversEverythingTest,
	"RefactorTactics.UI.GlossaryCoversEveryStatusSurfaceAndCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlossaryCoversEverythingTest::RunTest(const FString&)
{
	const TArray<FName> Stati = GlossaryStatusKeys();
	const TArray<FName> Superfici = GlossarySurfaceKeys();
	const TArray<FName> Ripari = GlossaryCoverKeys();

	// ⚠️ Le premesse: un insieme vuoto renderebbe i cicli qui sotto un verde vuoto. Tre variabili e non un
	// `&&`: si valutano tutte, e il log le nomina tutte.
	const bool bStati = TestTrue(TEXT("premessa: ci sono stati registrati sotto `Status.`"), Stati.Num() > 0);
	const bool bSuperfici = TestTrue(TEXT("premessa: `ERTHexSurface` ha dei valori"), Superfici.Num() > 0);
	const bool bRipari = TestTrue(TEXT("premessa: c'e' almeno un tipo di riparo diverso da `None`"), Ripari.Num() > 0);
	if (!bStati || !bSuperfici || !bRipari)
	{
		return false;
	}
	// Controllo positivo: l'enumerazione vede uno stato che il gioco applica di sicuro.
	TestTrue(TEXT("premessa: fra gli stati c'e' quello del fuoco"),
		Stati.Contains(TAG_Status_Burning.GetTag().GetTagName()));

	TArray<FName> Tutte;
	Tutte.Append(Stati);
	Tutte.Append(Superfici);
	Tutte.Append(Ripari);
	for (const FName& Key : Tutte)
	{
		TestFalse(*FString::Printf(TEXT("%s ha un nome"), *Key.ToString()),
			RTGlossary::NameFor(Key).IsEmptyOrWhitespace());
		TestFalse(*FString::Printf(TEXT("%s ha un effetto"), *Key.ToString()),
			RTGlossary::EffectFor(Key).IsEmptyOrWhitespace());
	}
	return true;
}

/**
 * OGNI VOCE NOMINA QUALCOSA CHE ESISTE, UNA VOLTA SOLA, E RISPETTA LE REGOLE DEI TESTI - `#3744`.
 *
 * 🔑 **E' il verso opposto del test qui sopra.** Senza di lui una voce resta nella tabella dopo che il suo stato
 * e' stato rinominato, e il documento la ripete come se valesse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlossaryEntriesExistTest,
	"RefactorTactics.UI.GlossaryEntriesNameThingsThatExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlossaryEntriesExistTest::RunTest(const FString&)
{
	const TArray<TPair<FName, FString>>& Nomi = RTGlossary::Names();
	const TArray<TPair<FName, FString>>& Effetti = RTGlossary::Effects();
	if (!TestTrue(TEXT("premessa: il glossario ha delle voci"), Nomi.Num() > 0))
	{
		return false;
	}
	TestEqual(TEXT("nomi ed effetti hanno lo stesso numero di voci"), Nomi.Num(), Effetti.Num());

	TSet<FName> Attese;
	Attese.Append(GlossaryStatusKeys());
	Attese.Append(GlossarySurfaceKeys());
	Attese.Append(GlossaryCoverKeys());

	TSet<FName> Viste;
	for (int32 I = 0; I < Nomi.Num(); ++I)
	{
		const FName Key = Nomi[I].Key;
		const FString K = Key.ToString();
		TestTrue(*FString::Printf(TEXT("%s nomina uno stato, una superficie o un riparo che esiste"), *K),
			Attese.Contains(Key));
		TestFalse(*FString::Printf(TEXT("%s compare una volta sola"), *K), Viste.Contains(Key));
		Viste.Add(Key);

		GlossaryCheckText(*this, K, TEXT("nome"), Nomi[I].Value);
		TestFalse(*FString::Printf(TEXT("%s: il nome non finisce col punto"), *K), Nomi[I].Value.EndsWith(TEXT(".")));

		if (!Effetti.IsValidIndex(I))
		{
			continue;
		}
		// Le due tabelle hanno le stesse chiavi nello stesso ordine: e' cio' che le rende rileggibili affiancate.
		TestEqual(*FString::Printf(TEXT("la voce %d ha la stessa chiave nelle due tabelle"), I), Effetti[I].Key, Key);
		GlossaryCheckText(*this, K, TEXT("effetto"), Effetti[I].Value);
		TestTrue(*FString::Printf(TEXT("%s: l'effetto finisce col punto"), *K), Effetti[I].Value.EndsWith(TEXT(".")));
	}
	return true;
}

/**
 * LE CHIAVI SONO GLI ID CHE IL GIOCO SCRIVE GIA', E UNA CHIAVE CHE NON C'E' DA' UN TESTO VUOTO - `#3744`.
 *
 * 🔑 **Valori letterali, e non una seconda composizione**: se `KeyForSurface` cambiasse forma, il test qui sopra
 * continuerebbe a concordare con lei. `Status.Burning` e `Terrain.Fire` sono cio' che il TurnLog scrive come causa
 * di un danno; gli altri sono gli id del catalogo dei terreni, dove una superficie che non fa danno non arriva
 * mai al TurnLog.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlossaryKeysTest,
	"RefactorTactics.UI.GlossaryKeysAreTheIdsTheGameWrites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlossaryKeysTest::RunTest(const FString&)
{
	TestEqual(TEXT("uno stato ha per chiave il nome del suo tag"),
		RTGlossary::KeyForStatus(TAG_Status_Burning), FName(TEXT("Status.Burning")));
	TestEqual(TEXT("una superficie ha per chiave la causa che il TurnLog scrive"),
		RTGlossary::KeyForSurface(ERTHexSurface::Fire), FName(TEXT("Terrain.Fire")));
	TestEqual(TEXT("il nome del valore entra intero nella chiave"),
		RTGlossary::KeyForSurface(ERTHexSurface::ShallowWater), FName(TEXT("Terrain.ShallowWater")));
	TestEqual(TEXT("il riparo basso ha l'id del catalogo dei terreni"),
		RTGlossary::KeyForCover(ERTHexCoverType::Low), FName(TEXT("Structure.LowCover")));
	TestEqual(TEXT("il riparo alto ha l'id del catalogo dei terreni"),
		RTGlossary::KeyForCover(ERTHexCoverType::High), FName(TEXT("Structure.HighCover")));

	// Cio' che non ha una voce: nessuna chiave, e un testo vuoto invece di un crash.
	TestTrue(TEXT("`None` non e' un riparo: nessuna chiave"), RTGlossary::KeyForCover(ERTHexCoverType::None).IsNone());
	TestTrue(TEXT("un tag non valido non ha chiave"), RTGlossary::KeyForStatus(FGameplayTag()).IsNone());
	TestTrue(TEXT("una chiave vuota non ha nome"), RTGlossary::NameFor(NAME_None).IsEmpty());
	TestTrue(TEXT("una chiave sconosciuta non ha nome"), RTGlossary::NameFor(TEXT("Status.Inesistente")).IsEmpty());
	TestTrue(TEXT("una chiave sconosciuta non ha effetto"), RTGlossary::EffectFor(TEXT("Status.Inesistente")).IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
