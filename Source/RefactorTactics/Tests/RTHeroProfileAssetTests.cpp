// I `WBP_RT_*` del Profilo Tattico come ASSET: esistono, derivano dalla classe giusta, e la scheda
// contiene davvero il radar.
//
// ⚠️ **Questi test provano il `.uasset`, non il C++.** `RTHeroProfileViewTests.cpp` prova gia' che la
// geometria e i contratti siano corretti; questo file prova la meta' che il codice non puo' garantire —
// che gli asset esistano, che il loro parent sia la classe C++ prevista, e che il profilo COMPONGA il
// radar invece di ridisegnarlo. Un `.uasset` non si diffa e non si grep-pa: senza un test, la sola
// evidenza che quel legame esista sarebbe un ricordo.
//
// 🔴 **Il caso che difendono e' concreto: un asset svuotato passa inosservato.** Un salvataggio con un
// riferimento che non risolve puo' lasciare un `.uasset` valido ma privo di parent e di widget tree; il
// gioco non crasha, il pannello e' solo... vuoto. `ParentIsTheCppBase` fallirebbe subito.
//
// ⛔ Cio' che questi test **non** coprono: l'aspetto. Colori, leggibilita', dimensioni a schermo e la
// sagoma effettiva del radar restano verifica Editor/PIE umana — un widget largo zero pixel supererebbe
// ogni asserzione di questo file.

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Tests/RTWidgetAssetTestHelpers.h"
#include "UI/RTHeroProfileWidget.h"
#include "UI/RTHeroRadarWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTHeroProfileAssetTestNames
{
	// ⚠️ Namespace NOMINATO e non anonimo: la unity build concatena questo file con gli altri
	// `*AssetTests.cpp`, e un nome generico in un namespace anonimo e' la collisione che #2397 ha gia'
	// pagato una volta.
	const TCHAR* const ProfilePath = TEXT("/Game/RT/UI/Profile/WBP_RT_HeroProfile.WBP_RT_HeroProfile_C");
	const TCHAR* const RadarPath = TEXT("/Game/RT/UI/Profile/WBP_RT_HeroRadar.WBP_RT_HeroRadar_C");
	const TCHAR* const ElementalPath = TEXT("/Game/RT/UI/Profile/WBP_RT_ElementalProfile.WBP_RT_ElementalProfile_C");
	const TCHAR* const RoleChipPath = TEXT("/Game/RT/UI/Profile/WBP_RT_HeroRoleChip.WBP_RT_HeroRoleChip_C");
	const TCHAR* const TagChipPath = TEXT("/Game/RT/UI/Profile/WBP_RT_HeroTagChip.WBP_RT_HeroTagChip_C");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileAssetsExistTest,
	"RefactorTactics.HeroProfile.ProfileAssetsExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileAssetsExistTest::RunTest(const FString&)
{
	using namespace RTHeroProfileAssetTestNames;

	const TCHAR* const All[] = { ProfilePath, RadarPath, ElementalPath, RoleChipPath, TagChipPath };

	for (const TCHAR* const Path : All)
	{
		UWidgetBlueprintGeneratedClass* Class = RTWidgetAssetTest::LoadWidgetClass(Path);
		// Il messaggio nomina il path: «cast fallito» non direbbe QUALE asset manca.
		TestNotNull(*FString::Printf(TEXT("l'asset %s si carica"), Path), Class);

		if (Class)
		{
			const UWidgetTree* Tree = Class->GetWidgetTreeArchetype();
			TestNotNull(*FString::Printf(TEXT("%s ha un widget tree"), Path), Tree);

			if (Tree)
			{
				int32 Count = 0;
				Tree->ForEachWidget([&Count](UWidget*) { ++Count; });
				// Zero widget e' il sintomo dell'asset svuotato: valido da caricare, vuoto da mostrare.
				TestTrue(*FString::Printf(TEXT("%s non e' vuoto (%d widget)"), Path, Count), Count > 0);
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileParentIsTheCppBaseTest,
	"RefactorTactics.HeroProfile.ParentIsTheCppBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileParentIsTheCppBaseTest::RunTest(const FString&)
{
	using namespace RTHeroProfileAssetTestNames;

	// 🔴 E' il legame che rende gli asset utili: senza il parent C++, il WBP e' un `UUserWidget`
	// qualunque, `SetProfileView` non esiste piu' e ogni binding della scheda punta al vuoto.
	UWidgetBlueprintGeneratedClass* ProfileClass = RTWidgetAssetTest::LoadWidgetClass(ProfilePath);
	if (TestNotNull(TEXT("WBP_RT_HeroProfile si carica"), ProfileClass))
	{
		TestTrue(TEXT("WBP_RT_HeroProfile deriva da URTHeroProfileWidget"),
			ProfileClass->IsChildOf(URTHeroProfileWidget::StaticClass()));
	}

	UWidgetBlueprintGeneratedClass* RadarClass = RTWidgetAssetTest::LoadWidgetClass(RadarPath);
	if (TestNotNull(TEXT("WBP_RT_HeroRadar si carica"), RadarClass))
	{
		TestTrue(TEXT("WBP_RT_HeroRadar deriva da URTHeroRadarWidget"),
			RadarClass->IsChildOf(URTHeroRadarWidget::StaticClass()));
	}

	// ⚠️ Il contrappunto: i tre asset Blueprint-only NON devono ereditare una base di gameplay. Sono
	// presentazione di un DTO, e dargli una base C++ significherebbe dargli una porta per calcolare.
	for (const TCHAR* const Path : { ElementalPath, RoleChipPath, TagChipPath })
	{
		UWidgetBlueprintGeneratedClass* Class = RTWidgetAssetTest::LoadWidgetClass(Path);
		if (Class)
		{
			TestFalse(*FString::Printf(TEXT("%s resta Blueprint-only"), Path),
				Class->IsChildOf(URTHeroProfileWidget::StaticClass()) || Class->IsChildOf(URTHeroRadarWidget::StaticClass()));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileComposesTheRadarTest,
	"RefactorTactics.HeroProfile.ProfileComposesTheRadar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileComposesTheRadarTest::RunTest(const FString&)
{
	using namespace RTHeroProfileAssetTestNames;

	UWidgetBlueprintGeneratedClass* ProfileClass = RTWidgetAssetTest::LoadWidgetClass(ProfilePath);
	if (!ProfileClass)
	{
		AddError(TEXT("WBP_RT_HeroProfile non si carica"));
		return false;
	}

	const UWidgetTree* Tree = ProfileClass->GetWidgetTreeArchetype();
	if (!Tree)
	{
		AddError(TEXT("WBP_RT_HeroProfile non ha widget tree"));
		return false;
	}

	TArray<FString> Names;
	bool bHasRadarInstance = false;
	Tree->ForEachWidget([&Names, &bHasRadarInstance](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}
		Names.Add(Widget->GetName());

		// 🔵 La domanda giusta e' sulla CLASSE, non sul nome: rinominare il widget nell'editor e' lecito,
		// sostituirlo con qualcosa che non e' un radar no.
		if (Widget->IsA(URTHeroRadarWidget::StaticClass()))
		{
			bHasRadarInstance = true;
		}
	});

	// Il radar e' un asset separato ISTANZIATO dentro la scheda: e' la ragione per cui esiste come WBP
	// proprio, e non come un riquadro disegnato dentro il profilo.
	TestTrue(TEXT("la scheda contiene un'istanza di radar"), bHasRadarInstance);

	// I contenitori del layout §9 che la scheda promette. Non e' l'aspetto: e' la presenza delle sedi.
	for (const TCHAR* const Expected : { TEXT("RootBox"), TEXT("HeaderRow"), TEXT("HeroName"), TEXT("HeroRadar"), TEXT("ElementRow") })
	{
		TestTrue(*FString::Printf(TEXT("il layout dichiara %s"), Expected), Names.Contains(Expected));
	}

	AddInfo(FString::Printf(TEXT("widget nella scheda (%d): %s"), Names.Num(), *FString::Join(Names, TEXT(", "))));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
