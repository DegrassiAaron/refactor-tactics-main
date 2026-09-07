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
#include "Components/SizeBox.h"
#include "HAL/IConsoleManager.h"
#include "Components/TextBlock.h"
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

// ------------------------------------------------------------------------------------------------
// Il binding, provato sull'asset vero e non su un widget fabbricato dal test.
// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileBindingFillsTheCardTest,
	"RefactorTactics.HeroProfile.BindingFillsTheCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileBindingFillsTheCardTest::RunTest(const FString&)
{
	using namespace RTHeroProfileAssetTestNames;

	UWidgetBlueprintGeneratedClass* Class = RTWidgetAssetTest::LoadWidgetClass(ProfilePath);
	if (!Class)
	{
		AddError(TEXT("WBP_RT_HeroProfile non si carica"));
		return false;
	}

	URTHeroProfileWidget* Card = Cast<URTHeroProfileWidget>(NewObject<UUserWidget>(GetTransientPackage(), Class));
	if (!Card)
	{
		AddError(TEXT("la classe non produce un URTHeroProfileWidget"));
		return false;
	}

	// `Initialize` duplica il widget tree e risolve i `BindWidget`: senza, i puntatori restano nulli e
	// il test proverebbe soltanto che scrivere su `nullptr` non esplode.
	Card->Initialize();

	// Legge il testo dal widget REALE dell'albero, per nome. ⚠️ Non da un accessor C++: la domanda e'
	// «il dato e' arrivato al `UTextBlock` che il giocatore vede», non «il codice ha una variabile».
	auto TextOf = [Card](const TCHAR* WidgetName) -> FString
	{
		const UTextBlock* Block = Cast<UTextBlock>(Card->GetWidgetFromName(FName(WidgetName)));
		return Block ? Block->GetText().ToString() : FString(TEXT("<assente>"));
	};

	// ---- profilo popolato ----------------------------------------------------------------------
	FRTHeroProfileView View;
	View.HeroId = FName(TEXT("Hero.TestFixture"));
	View.DisplayName = FText::FromString(TEXT("Fixture"));
	View.PrimaryRoleId = FName(TEXT("Role.TestPrimary"));
	View.PrimaryRoleLabel = FText::FromString(TEXT("Primario"));
	View.SecondaryRoleId = FName(TEXT("Role.TestSecondary"));
	View.SecondaryRoleLabel = FText::FromString(TEXT("Secondario"));
	View.StyleTags = { FText::FromString(TEXT("TagUno")), FText::FromString(TEXT("TagDue")) };
	View.AffinityId = FName(TEXT("Affinity.TestFixture"));
	View.AffinityLabel = FText::FromString(TEXT("AffinitaDiProva"));
	View.CombatIdentity = FText::FromString(TEXT("Una frase di prova."));
	View.Strengths = { FText::FromString(TEXT("PregioUno")) };

	FRTProfileRadarAxisView Axis;
	Axis.AxisId = FName(TEXT("Radar.Profile.Offense"));
	Axis.Label = FText::FromString(TEXT("Offesa"));
	Axis.Value = 7;
	Axis.MaxValue = 10;
	View.RadarAxes = { Axis };

	Card->SetProfileView(View);

	TestEqual(TEXT("il nome arriva al TextBlock"), TextOf(TEXT("HeroName")), FString(TEXT("Fixture")));
	TestEqual(TEXT("i due ruoli stanno su una riga"), TextOf(TEXT("RoleLine")), FString(TEXT("Primario · Secondario")));
	TestEqual(TEXT("gli style tag sono uniti"), TextOf(TEXT("StyleTagLine")), FString(TEXT("TagUno · TagDue")));
	TestEqual(TEXT("l'affinita' mostra la label"), TextOf(TEXT("AffinityText")), FString(TEXT("AffinitaDiProva")));
	TestEqual(TEXT("l'identita' di combattimento arriva"), TextOf(TEXT("CombatIdentity")), FString(TEXT("Una frase di prova.")));

	// 🔴 Il caso che conta: la debolezza NON e' stata dichiarata, quindi la scheda deve dire «assente»
	// — non lasciare il testo di default del widget, e non inventarla dall'affinita'.
	TestEqual(TEXT("la debolezza assente mostra il segnaposto"),
		TextOf(TEXT("WeaknessText")), URTHeroProfileWidget::GetAbsentValueText().ToString());
	TestEqual(TEXT("portata e difficolta' assenti mostrano il segnaposto"),
		TextOf(TEXT("RangeAndDifficulty")), URTHeroProfileWidget::GetAbsentValueText().ToString());

	// ⚠️ Nessun `TextBlock` deve essere rimasto col testo di fabbrica: e' il sintomo esatto di un
	// binding che non c'e', ed e' quello che si vedrebbe a schermo.
	for (const TCHAR* const Name : { TEXT("HeroName"), TEXT("RoleLine"), TEXT("StyleTagLine"),
		TEXT("AffinityText"), TEXT("WeaknessText"), TEXT("RangeAndDifficulty"), TEXT("CombatIdentity"),
		TEXT("StrengthsText"), TEXT("TradeoffsText") })
	{
		TestNotEqual(*FString::Printf(TEXT("%s non e' rimasto al testo di default"), Name),
			TextOf(Name), FString(TEXT("Text Block")));
	}

	// Il radar ha ricevuto gli assi dalla scheda.
	const URTHeroRadarWidget* Radar = Cast<URTHeroRadarWidget>(Card->GetWidgetFromName(FName(TEXT("HeroRadar"))));
	if (TestNotNull(TEXT("la scheda trova il proprio radar"), Radar))
	{
		TestEqual(TEXT("il radar ha ricevuto l'asse"), Radar->GetRadarAxes().Num(), 1);
		TestTrue(TEXT("e lo considera disegnabile"), Radar->HasDrawableAxes());
	}

	// ---- profilo vuoto: tutto segnaposto, niente dedotto ----------------------------------------
	Card->SetProfileView(FRTHeroProfileView());

	const FString Absent = URTHeroProfileWidget::GetAbsentValueText().ToString();
	TestEqual(TEXT("una view vuota svuota il nome"), TextOf(TEXT("HeroName")), Absent);
	TestEqual(TEXT("e i ruoli"), TextOf(TEXT("RoleLine")), Absent);
	TestEqual(TEXT("e l'affinita'"), TextOf(TEXT("AffinityText")), Absent);

	const URTHeroRadarWidget* EmptyRadar = Cast<URTHeroRadarWidget>(Card->GetWidgetFromName(FName(TEXT("HeroRadar"))));
	if (EmptyRadar)
	{
		TestEqual(TEXT("il radar resta senza assi"), EmptyRadar->GetRadarAxes().Num(), 0);
		TestFalse(TEXT("e non si dichiara disegnabile"), EmptyRadar->HasDrawableAxes());
	}

	return true;
}

// ------------------------------------------------------------------------------------------------
// L'area del radar: il difetto che nessun test vedeva, e che si e' visto solo aprendo l'Editor.
// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileRadarHasDrawableAreaTest,
	"RefactorTactics.HeroProfile.RadarHasDrawableArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileRadarHasDrawableAreaTest::RunTest(const FString&)
{
	using namespace RTHeroProfileAssetTestNames;

	UWidgetBlueprintGeneratedClass* Class = RTWidgetAssetTest::LoadWidgetClass(RadarPath);
	if (!Class)
	{
		AddError(TEXT("WBP_RT_HeroRadar non si carica"));
		return false;
	}

	URTHeroRadarWidget* Radar = Cast<URTHeroRadarWidget>(NewObject<UUserWidget>(GetTransientPackage(), Class));
	if (!Radar)
	{
		AddError(TEXT("la classe non produce un URTHeroRadarWidget"));
		return false;
	}

	Radar->Initialize();

	USizeBox* Box = Cast<USizeBox>(Radar->GetWidgetFromName(TEXT("RadarBox")));
	if (!TestNotNull(TEXT("il WBP ha il SizeBox di root"), Box))
	{
		return false;
	}

	// 🔴 Lo STATO DI PARTENZA e' il difetto: l'asset non dichiara nessuna misura. Il test lo afferma
	// invece di darlo per scontato, perche' se un giorno il `.uasset` le dichiarasse, il ramo che
	// stiamo pinnando non verrebbe piu' esercitato e questo test diventerebbe verde a vuoto.
	AddInfo(FString::Printf(TEXT("SizeBox prima: width=%.0f height=%.0f"),
		Box->GetWidthOverride(), Box->GetHeightOverride()));

	Radar->EnsureDrawableArea();

	TestTrue(TEXT("dopo la garanzia il SizeBox ha una larghezza"), Box->GetWidthOverride() > 0.f);
	TestTrue(TEXT("dopo la garanzia il SizeBox ha un'altezza"), Box->GetHeightOverride() > 0.f);

	// La domanda vera non e' «il box ha una misura» ma «il radar puo' disegnare»: e' il raggio a
	// decidere, ed e' zero che fa uscire `NativePaint` prima di ogni linea.
	const float Radius = Radar->ComputeRadiusForSize(FVector2D(Box->GetWidthOverride(), Box->GetHeightOverride()));
	TestTrue(FString::Printf(TEXT("il raggio e' disegnabile (%.1f px)"), Radius), Radius > 0.f);

	// ⚠️ E il verso opposto: una misura gia' authorata NON viene sovrascritta. Senza questo, la
	// garanzia sarebbe un'imposizione, e ogni scelta di layout nel `.uasset` verrebbe cancellata
	// all'apertura.
	URTHeroRadarWidget* Authored = Cast<URTHeroRadarWidget>(NewObject<UUserWidget>(GetTransientPackage(), Class));
	Authored->Initialize();
	USizeBox* AuthoredBox = Cast<USizeBox>(Authored->GetWidgetFromName(TEXT("RadarBox")));
	if (AuthoredBox)
	{
		AuthoredBox->SetWidthOverride(120.f);
		AuthoredBox->SetHeightOverride(90.f);
		Authored->EnsureDrawableArea();

		TestEqual(TEXT("una larghezza authorata resta"), AuthoredBox->GetWidthOverride(), 120.f);
		TestEqual(TEXT("un'altezza authorata resta"), AuthoredBox->GetHeightOverride(), 90.f);
	}

	return true;
}

// ------------------------------------------------------------------------------------------------
// Il comando che rende eseguibile la verifica visiva.
// ------------------------------------------------------------------------------------------------
/**
 * ⚠️ **Non va aggiunto agli otto di `Debug.NamespaceDeclaresAllCommands`**, come `rt.Debug.Los`: quel DoD
 * elenca cio' che deve esserci, non tutto cio' che c'e'.
 *
 * 🔵 Questo test prova che il comando ESISTA, non che disegni bene: cio' che si vede a schermo resta la
 * verifica umana per cui il comando e' stato scritto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileConsoleIsRegisteredTest,
	"RefactorTactics.HeroProfile.ConsoleIsRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileConsoleIsRegisteredTest::RunTest(const FString&)
{
	IConsoleObject* Cmd = IConsoleManager::Get().FindConsoleObject(TEXT("rt.Debug.HeroProfile"));
	if (!TestNotNull(TEXT("rt.Debug.HeroProfile e' registrato"), Cmd))
	{
		return false;
	}

	// ⛔ L'aiuto deve dire che i valori sono inventati. E' l'unico posto in cui chi lo esegue lo legge, e
	// senza quella riga le fixture di prova possono essere scambiate per dati del catalogo.
	const FString Help = Cmd->GetHelp();
	TestTrue(*FString::Printf(TEXT("l'aiuto dichiara che i valori sono inventati: %s"), *Help),
		Help.Contains(TEXT("inventati")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
