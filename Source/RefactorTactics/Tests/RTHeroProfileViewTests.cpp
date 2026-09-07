// Il Profilo Tattico / Hero Profile — contratti della view, geometria del radar, e i due confini che la
// scheda non deve attraversare.
//
// ⚠️ **Questi test provano il C++, non l'aspetto.** Un radar con la geometria giusta puo' essere illeggibile,
// e un widget largo zero pixel passerebbe ogni asserzione di questo file: colori, leggibilita' e layout
// restano verifica Editor/PIE. Cio' che si prova qui e' la meta' che a occhio non si vede — che l'ordine
// degli assi sopravviva, che lo zero finisca al centro, e che un dato rotto venga rifiutato invece di
// essere disegnato «come meglio si puo'».
//
// 🔴 Due test valgono piu' degli altri sei, perche' pinnano decisioni che si perdono in un refactor:
//
//  * `NoGameplayPointersInView` guarda la view **per riflessione**. Un `TObjectPtr<ARTUnit>` aggiunto per
//    comodita' compilerebbe, funzionerebbe, e aprirebbe ai Blueprint la porta per ricalcolare — che e'
//    esattamente il confine di §4.1 di `progettazione-hud.md`.
//  * `AffinityDoesNotImplyProficiency` prova un'ASSENZA. E' l'unico modo di difendere `#995` da una
//    scorciatoia futura: `Affinity` e proficiency si assomigliano abbastanza da sembrare la stessa cosa.

#include "Misc/AutomationTest.h"
#include "Ability/RTHeroData.h"
#include "UI/RTHeroProfileLibrary.h"
#include "UI/RTHeroProfileView.h"
#include "UI/RTHeroProfileWidget.h"
#include "UI/RTHeroRadarWidget.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Tolleranza dei confronti geometrici: sono pixel, non simulazione. */
	constexpr float RadarTolerance = 0.01f;

	FRTProfileRadarAxisView MakeAxis(const TCHAR* AxisId, int32 Value, int32 MaxValue = 10)
	{
		FRTProfileRadarAxisView Axis;
		Axis.AxisId = FName(AxisId);
		Axis.Label = FText::FromString(AxisId);
		Axis.Value = Value;
		Axis.MaxValue = MaxValue;
		return Axis;
	}

	/**
	 * I sei assi del Profile Radar nell'ordine normativo di D-107, con valori **inventati per il test**.
	 *
	 * ⚠️ I numeri non sono di nessun eroe: servono solo a essere tutti diversi, cosi' che uno scambio di
	 * posizione produca una figura diversa e il test se ne accorga. Il dataset canonico vive nei cataloghi.
	 */
	TArray<FRTProfileRadarAxisView> MakeSixAxisProfile()
	{
		return {
			MakeAxis(TEXT("Radar.Profile.Offense"), 8),
			MakeAxis(TEXT("Radar.Profile.Durability"), 3),
			MakeAxis(TEXT("Radar.Profile.Mobility"), 7),
			MakeAxis(TEXT("Radar.Profile.Control"), 2),
			MakeAxis(TEXT("Radar.Profile.Support"), 5),
			MakeAxis(TEXT("Radar.Profile.Information"), 6)
		};
	}

	/** Il punto atteso per un asse, calcolato in modo indipendente dal codice sotto test. */
	FVector2D ExpectedPoint(int32 Index, int32 AxisCount, float Ratio, FVector2D Center, float Radius)
	{
		const float AngleRadians = FMath::DegreesToRadians(-90.f + (static_cast<float>(Index) * 360.f) / static_cast<float>(AxisCount));
		return FVector2D(
			Center.X + Radius * Ratio * FMath::Cos(AngleRadians),
			Center.Y + Radius * Ratio * FMath::Sin(AngleRadians));
	}

	/**
	 * Cerca ricorsivamente un puntatore a oggetto dentro una `UStruct`, scendendo in struct annidate e
	 * dentro gli elementi degli array.
	 *
	 * ⚠️ `FObjectPropertyBase` copre in un colpo solo `TObjectPtr`, puntatore nudo, `TWeakObjectPtr`,
	 * `TSoftObjectPtr` e `TLazyObjectPtr`: elencarli a mano lascerebbe fuori proprio la forma che qualcuno
	 * userebbe per aggirare la regola.
	 */
	bool FindGameplayPointer(const UStruct* Struct, FString& OutPath)
	{
		if (!Struct)
		{
			return false;
		}

		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			const FProperty* Property = *It;
			const FString Name = Property->GetName();

			auto Inspect = [&OutPath, &Name](const FProperty* Inner, auto&& Recurse) -> bool
			{
				if (Inner->IsA<FObjectPropertyBase>() || Inner->IsA<FInterfaceProperty>())
				{
					OutPath = Name;
					return true;
				}

				if (const FStructProperty* AsStruct = CastField<FStructProperty>(Inner))
				{
					FString NestedPath;
					if (FindGameplayPointer(AsStruct->Struct, NestedPath))
					{
						OutPath = Name + TEXT(".") + NestedPath;
						return true;
					}
				}

				if (const FArrayProperty* AsArray = CastField<FArrayProperty>(Inner))
				{
					return Recurse(AsArray->Inner, Recurse);
				}

				return false;
			};

			if (Inspect(Property, Inspect))
			{
				return true;
			}
		}

		return false;
	}
}

// ------------------------------------------------------------------------------------------------
// 1. Il travaso dal catalogo copia, e non inventa.
// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileBaseProfileReadsCanonicalIdentityTest,
	"RefactorTactics.HeroProfile.BaseProfileReadsCanonicalIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileBaseProfileReadsCanonicalIdentityTest::RunTest(const FString&)
{
	URTHeroData* Hero = NewObject<URTHeroData>(GetTransientPackage());
	if (!Hero)
	{
		AddError(TEXT("impossibile creare il dato d'eroe di prova"));
		return false;
	}

	// ⚠️ Un eroe inventato per il test: nessun `HeroId` del catalogo entra in un test della UI, o il
	// rename di `#2491` romperebbe questo file senza che la UI sia cambiata.
	Hero->HeroId = FName(TEXT("Hero.TestFixture"));
	Hero->DisplayName = FText::FromString(TEXT("Fixture"));
	Hero->Affinity = FName(TEXT("Affinity.TestFixture"));
	Hero->Weakness = FName(TEXT("Weakness.TestFixture"));

	const FRTHeroProfileView View = URTHeroProfileLibrary::BuildBaseProfile(Hero);

	TestEqual(TEXT("HeroId copiato"), View.HeroId, Hero->HeroId);
	TestEqual(TEXT("DisplayName copiato"), View.DisplayName.ToString(), Hero->DisplayName.ToString());
	TestEqual(TEXT("AffinityId copiato"), View.AffinityId, Hero->Affinity);
	TestEqual(TEXT("WeaknessId copiato"), View.WeaknessId, Hero->Weakness);

	// La meta' che conta: cio' che il catalogo NON possiede resta vuoto.
	TestEqual(TEXT("nessun asse radar inventato"), View.RadarAxes.Num(), 0);
	TestEqual(TEXT("nessun ruolo inventato"), View.PrimaryRoleId, FName());
	TestEqual(TEXT("nessuna relazione elementale inventata"), View.ElementRelations.Num(), 0);
	TestEqual(TEXT("nessuna proficiency inventata"), View.ElementalProficiencies.Num(), 0);
	TestTrue(TEXT("nessuna etichetta di affinita' sintetizzata dall'ID"), View.AffinityLabel.IsEmpty());
	TestTrue(TEXT("nessuna etichetta di debolezza sintetizzata dall'ID"), View.WeaknessLabel.IsEmpty());

	// Un eroe assente non e' un crash e non e' un profilo finto.
	const FRTHeroProfileView Empty = URTHeroProfileLibrary::BuildBaseProfile(nullptr);
	TestEqual(TEXT("nullptr produce una view vuota"), Empty.HeroId, FName());

	return true;
}

// ------------------------------------------------------------------------------------------------
// 2. L'ordine degli assi e' quello del DTO, e il dataset normativo e' rappresentabile.
// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileRadarPreservesAxisOrderTest,
	"RefactorTactics.HeroProfile.RadarPreservesAxisOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileRadarPreservesAxisOrderTest::RunTest(const FString&)
{
	const TArray<FRTProfileRadarAxisView> Axes = MakeSixAxisProfile();
	const FVector2D Center(100.f, 100.f);
	const float Radius = 50.f;

	TArray<FVector2D> Points;
	TestTrue(TEXT("il dataset a sei assi e' disegnabile"), URTHeroRadarWidget::ComputeRadarPoints(Axes, Center, Radius, Points));
	TestEqual(TEXT("un vertice per asse"), Points.Num(), Axes.Num());

	if (Points.Num() != Axes.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < Axes.Num(); ++Index)
	{
		const float Ratio = static_cast<float>(Axes[Index].Value) / static_cast<float>(Axes[Index].MaxValue);
		const FVector2D Expected = ExpectedPoint(Index, Axes.Num(), Ratio, Center, Radius);

		TestTrue(FString::Printf(TEXT("il vertice %d (%s) sta sul proprio asse"), Index, *Axes[Index].AxisId.ToString()),
			FVector2D::Distance(Points[Index], Expected) < RadarTolerance);
	}

	// Il primo asse punta in alto: e' la convenzione del radar pubblicato, e senza questo controllo una
	// rotazione dell'intera figura passerebbe inosservata.
	TestTrue(TEXT("il primo asse punta verso l'alto"), Points[0].X - Center.X < RadarTolerance && Points[0].Y < Center.Y);

	// ⚠️ La prova che l'ordine e' DATO e non ricostruito: gli stessi assi mescolati producono una figura
	// diversa. Se il widget ordinasse per `FName` o per etichetta, le due sarebbero identiche.
	TArray<FRTProfileRadarAxisView> Shuffled = Axes;
	Shuffled.Swap(0, 3);

	TArray<FVector2D> ShuffledPoints;
	URTHeroRadarWidget::ComputeRadarPoints(Shuffled, Center, Radius, ShuffledPoints);
	TestTrue(TEXT("scambiare due assi cambia la sagoma"),
		FVector2D::Distance(Points[0], ShuffledPoints[0]) > RadarTolerance);

	return true;
}

// ------------------------------------------------------------------------------------------------
// 3-4. I due estremi della scala: centro e bordo.
// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileRadarMapsZeroToCenterTest,
	"RefactorTactics.HeroProfile.RadarMapsZeroToCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileRadarMapsZeroToCenterTest::RunTest(const FString&)
{
	const FVector2D Center(64.f, 48.f);
	const float Radius = 30.f;

	// ⚠️ `0` deve funzionare **senza casi speciali**, anche se i rating pubblicati oggi partono da 1: e'
	// il valore che distingue «misurato al minimo» da «non rappresentabile».
	TArray<FRTProfileRadarAxisView> Axes = {
		MakeAxis(TEXT("Axis.Zero"), 0),
		MakeAxis(TEXT("Axis.Half"), 5),
		MakeAxis(TEXT("Axis.AlsoZero"), 0)
	};

	TArray<FVector2D> Points;
	TestTrue(TEXT("zero e' disegnabile"), URTHeroRadarWidget::ComputeRadarPoints(Axes, Center, Radius, Points));

	if (Points.Num() != 3)
	{
		AddError(TEXT("attesi tre vertici"));
		return false;
	}

	TestTrue(TEXT("Value=0 finisce nel centro"), FVector2D::Distance(Points[0], Center) < RadarTolerance);
	TestTrue(TEXT("anche il secondo zero finisce nel centro"), FVector2D::Distance(Points[2], Center) < RadarTolerance);
	TestTrue(TEXT("meta' scala sta a meta' raggio"),
		FMath::Abs(FVector2D::Distance(Points[1], Center) - Radius * 0.5f) < RadarTolerance);

	TestEqual(TEXT("il rapporto di zero e' zero"), URTHeroRadarWidget::ComputeAxisRatio(Axes[0]), 0.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileRadarMapsMaxToBoundaryTest,
	"RefactorTactics.HeroProfile.RadarMapsMaxToBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileRadarMapsMaxToBoundaryTest::RunTest(const FString&)
{
	const FVector2D Center(0.f, 0.f);
	const float Radius = 120.f;

	// Fondoscala diversi sulla stessa figura: il bordo e' `Value == MaxValue`, non «10».
	TArray<FRTProfileRadarAxisView> Axes = {
		MakeAxis(TEXT("Axis.MaxOfTen"), 10, 10),
		MakeAxis(TEXT("Axis.MaxOfFive"), 5, 5),
		MakeAxis(TEXT("Axis.MaxOfHundred"), 100, 100)
	};

	TArray<FVector2D> Points;
	TestTrue(TEXT("il fondoscala e' disegnabile"), URTHeroRadarWidget::ComputeRadarPoints(Axes, Center, Radius, Points));

	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		TestTrue(FString::Printf(TEXT("il vertice %d sta sul bordo"), Index),
			FMath::Abs(FVector2D::Distance(Points[Index], Center) - Radius) < RadarTolerance);
	}

	// I punti del bordo calcolati per gli anelli devono coincidere con i vertici a fondoscala, o griglia
	// e valori si disallineerebbero di qualche pixel proprio dove il confronto e' piu' visibile.
	TArray<FVector2D> RingPoints;
	URTHeroRadarWidget::ComputeRingPoints(Axes.Num(), Center, Radius, RingPoints);
	TestEqual(TEXT("l'anello ha un vertice per asse"), RingPoints.Num(), Axes.Num());

	for (int32 Index = 0; Index < RingPoints.Num(); ++Index)
	{
		TestTrue(FString::Printf(TEXT("anello e valore coincidono sull'asse %d"), Index),
			FVector2D::Distance(RingPoints[Index], Points[Index]) < RadarTolerance);
	}

	// Un valore oltre il fondoscala non sfonda la figura: viene clampato al bordo.
	TArray<FRTProfileRadarAxisView> Overflow = { MakeAxis(TEXT("Axis.Over"), 30, 10) };
	TestEqual(TEXT("oltre il fondoscala il rapporto resta 1"), URTHeroRadarWidget::ComputeAxisRatio(Overflow[0]), 1.f);

	return true;
}

// ------------------------------------------------------------------------------------------------
// 5. Il widget non e' un esagono.
// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileRadarSupportsVariableAxisCountTest,
	"RefactorTactics.HeroProfile.RadarSupportsVariableAxisCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileRadarSupportsVariableAxisCountTest::RunTest(const FString&)
{
	const FVector2D Center(10.f, 10.f);
	const float Radius = 40.f;

	for (const int32 AxisCount : { 3, 5, 6, 8 })
	{
		TArray<FRTProfileRadarAxisView> Axes;
		for (int32 Index = 0; Index < AxisCount; ++Index)
		{
			Axes.Add(MakeAxis(*FString::Printf(TEXT("Axis.%d"), Index), Index % 11));
		}

		TArray<FVector2D> Points;
		const bool bOk = URTHeroRadarWidget::ComputeRadarPoints(Axes, Center, Radius, Points);

		TestTrue(FString::Printf(TEXT("%d assi sono disegnabili"), AxisCount), bOk);
		TestEqual(FString::Printf(TEXT("%d assi producono %d vertici"), AxisCount, AxisCount), Points.Num(), AxisCount);

		for (const FVector2D& Point : Points)
		{
			// Nessun NaN e nessun infinito: sono i due modi in cui una divisione mal guardata arriva fino
			// a Slate, dove non produce un errore ma un widget che sparisce.
			TestTrue(TEXT("il vertice e' un numero finito"), FMath::IsFinite(Point.X) && FMath::IsFinite(Point.Y));
			TestTrue(TEXT("il vertice sta dentro il raggio"), FVector2D::Distance(Point, Center) <= Radius + RadarTolerance);
		}

		// Gli angoli coprono il giro intero a passi uguali, qualunque sia il numero di assi.
		const float Step = URTHeroRadarWidget::ComputeAxisAngleDegrees(1, AxisCount) - URTHeroRadarWidget::ComputeAxisAngleDegrees(0, AxisCount);
		TestTrue(FString::Printf(TEXT("il passo angolare di %d assi e' 360/%d"), AxisCount, AxisCount),
			FMath::Abs(Step - 360.f / static_cast<float>(AxisCount)) < RadarTolerance);
	}

	return true;
}

// ------------------------------------------------------------------------------------------------
// 6. Un dato rotto viene rifiutato, non disegnato.
// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileInvalidAxisFailsClosedTest,
	"RefactorTactics.HeroProfile.InvalidAxisFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileInvalidAxisFailsClosedTest::RunTest(const FString&)
{
	TArray<FString> Diagnostics;

	// (a) fondoscala non positivo
	{
		FRTHeroProfileView View;
		View.RadarAxes = { MakeAxis(TEXT("Axis.Broken"), 5, 0) };
		TestFalse(TEXT("MaxValue=0 non passa la validazione"), URTHeroProfileLibrary::ValidateProfileView(View, Diagnostics));
		TestTrue(TEXT("la diagnosi dice quale asse"), Diagnostics.Num() > 0);

		// E soprattutto: la geometria si rifiuta di produrre una figura parziale.
		TArray<FVector2D> Points;
		TestFalse(TEXT("un asse rotto non e' disegnabile"),
			URTHeroRadarWidget::ComputeRadarPoints(View.RadarAxes, FVector2D::ZeroVector, 10.f, Points));
		TestEqual(TEXT("e non lascia vertici a meta'"), Points.Num(), 0);
	}

	// (b) un asse rotto in mezzo ad assi buoni invalida la figura intera
	{
		TArray<FRTProfileRadarAxisView> Axes = MakeSixAxisProfile();
		Axes[2].MaxValue = -1;

		TArray<FVector2D> Points;
		TestFalse(TEXT("un solo asse rotto basta a fermare il disegno"),
			URTHeroRadarWidget::ComputeRadarPoints(Axes, FVector2D::ZeroVector, 10.f, Points));
		TestEqual(TEXT("nessun vertice sopravvive"), Points.Num(), 0);
	}

	// (c) AxisId duplicato
	{
		FRTHeroProfileView View;
		View.RadarAxes = { MakeAxis(TEXT("Axis.Same"), 3), MakeAxis(TEXT("Axis.Same"), 7) };
		TestFalse(TEXT("un AxisId duplicato non passa"), URTHeroProfileLibrary::ValidateProfileView(View, Diagnostics));
	}

	// (d) valore fuori dall'intervallo
	{
		FRTHeroProfileView View;
		View.RadarAxes = { MakeAxis(TEXT("Axis.TooHigh"), 12, 10) };
		TestFalse(TEXT("Value oltre MaxValue non passa"), URTHeroProfileLibrary::ValidateProfileView(View, Diagnostics));

		View.RadarAxes = { MakeAxis(TEXT("Axis.Negative"), -1, 10) };
		TestFalse(TEXT("Value negativo non passa"), URTHeroProfileLibrary::ValidateProfileView(View, Diagnostics));
	}

	// (e) elemento duplicato fra le proficiency
	{
		FRTHeroProfileView View;
		FRTElementProficiencyView First;
		First.ElementId = FName(TEXT("Element.Test"));
		First.GradeId = FName(TEXT("Grade.A"));
		FRTElementProficiencyView Second;
		Second.ElementId = FName(TEXT("Element.Test"));
		Second.GradeId = FName(TEXT("Grade.B"));
		View.ElementalProficiencies = { First, Second };

		TestFalse(TEXT("lo stesso elemento con due gradi non passa"), URTHeroProfileLibrary::ValidateProfileView(View, Diagnostics));
	}

	// (f) etichetta senza identita' — il modo in cui una seconda fonte di verita' entra in una UI
	{
		FRTHeroProfileView View;
		View.WeaknessLabel = FText::FromString(TEXT("scritta a mano"));
		TestFalse(TEXT("una label senza ID non passa"), URTHeroProfileLibrary::ValidateProfileView(View, Diagnostics));
	}

	// (g) il caso valido: il dataset normativo passa, altrimenti la validazione sarebbe solo severa
	{
		FRTHeroProfileView View;
		View.RadarAxes = MakeSixAxisProfile();
		TestTrue(TEXT("il dataset a sei assi e' valido"), URTHeroProfileLibrary::ValidateProfileView(View, Diagnostics));
		TestEqual(TEXT("e non produce diagnosi"), Diagnostics.Num(), 0);
	}

	// (h) una view completamente vuota e' valida: e' lo stato di partenza, non un errore
	{
		const FRTHeroProfileView Empty;
		TestTrue(TEXT("una view vuota e' valida"), URTHeroProfileLibrary::ValidateProfileView(Empty, Diagnostics));
	}

	return true;
}

// ------------------------------------------------------------------------------------------------
// 7. Affinity != Elemental Proficiency (`#995`).
// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileAffinityDoesNotImplyProficiencyTest,
	"RefactorTactics.HeroProfile.AffinityDoesNotImplyProficiency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileAffinityDoesNotImplyProficiencyTest::RunTest(const FString&)
{
	URTHeroData* Hero = NewObject<URTHeroData>(GetTransientPackage());
	if (!Hero)
	{
		AddError(TEXT("impossibile creare il dato d'eroe di prova"));
		return false;
	}

	Hero->HeroId = FName(TEXT("Hero.TestFixture"));
	Hero->Affinity = FName(TEXT("Element.TestFixture"));
	Hero->Weakness = FName(TEXT("Weakness.TestFixture"));

	const FRTHeroProfileView View = URTHeroProfileLibrary::BuildBaseProfile(Hero);

	TestEqual(TEXT("l'affinita' e' stata letta"), View.AffinityId, Hero->Affinity);
	TestEqual(TEXT("ma non produce nessuna proficiency"), View.ElementalProficiencies.Num(), 0);
	TestEqual(TEXT("e nessuna relazione elementale"), View.ElementRelations.Num(), 0);

	// La stessa asimmetria vista dal widget: la sezione affinita' si mostra, quella proficiency no.
	URTHeroProfileWidget* Widget = NewObject<URTHeroProfileWidget>(GetTransientPackage());
	if (!Widget)
	{
		AddError(TEXT("impossibile creare il widget di prova"));
		return false;
	}

	Widget->SetProfileView(View);
	TestTrue(TEXT("il widget mostra l'affinita'"), Widget->HasAffinity());
	TestFalse(TEXT("il widget NON mostra proficiency"), Widget->HasElementalProficiencies());
	TestFalse(TEXT("ne' relazioni elementali"), Widget->HasElementRelations());

	// ⚠️ E l'affinita' non deve essere per forza un elemento: il catalogo ammette strutture e movimento.
	// Un profilo con affinita' non elementale resta valido e mostrabile.
	FRTHeroProfileView Structural = View;
	Structural.AffinityId = FName(TEXT("Affinity.Structures"));
	Widget->SetProfileView(Structural);
	TestTrue(TEXT("un'affinita' non elementale e' comunque mostrabile"), Widget->HasAffinity());
	TestFalse(TEXT("e continua a non implicare proficiency"), Widget->HasElementalProficiencies());

	// Il verso opposto: una proficiency dichiarata si mostra anche senza affinita'. Le due sezioni sono
	// indipendenti in entrambe le direzioni, non solo in quella comoda.
	FRTHeroProfileView ProficiencyOnly;
	FRTElementProficiencyView Proficiency;
	Proficiency.ElementId = FName(TEXT("Element.TestFixture"));
	Proficiency.GradeId = FName(TEXT("Grade.TestFixture"));
	Proficiency.Capabilities = { FName(TEXT("Apply")), FName(TEXT("Propagate")) };
	ProficiencyOnly.ElementalProficiencies = { Proficiency };

	Widget->SetProfileView(ProficiencyOnly);
	TestFalse(TEXT("nessuna affinita' dichiarata"), Widget->HasAffinity());
	TestTrue(TEXT("ma la proficiency si mostra lo stesso"), Widget->HasElementalProficiencies());

	return true;
}

// ------------------------------------------------------------------------------------------------
// 8. La view non espone gameplay, e lo si controlla per riflessione.
// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileNoGameplayPointersInViewTest,
	"RefactorTactics.HeroProfile.NoGameplayPointersInView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileNoGameplayPointersInViewTest::RunTest(const FString&)
{
	// ⚠️ Il controllo e' per riflessione e non per grep: un `TObjectPtr<ARTUnit>` aggiunto domani in una
	// delle sub-view non comparirebbe in nessuna ricerca fatta su questo file, ma comparirebbe qui.
	const TArray<UScriptStruct*> Structs = {
		FRTHeroProfileView::StaticStruct(),
		FRTProfileRadarAxisView::StaticStruct(),
		FRTElementRelationView::StaticStruct(),
		FRTElementProficiencyView::StaticStruct()
	};

	for (const UScriptStruct* Struct : Structs)
	{
		FString OffendingPath;
		const bool bFound = FindGameplayPointer(Struct, OffendingPath);

		TestFalse(FString::Printf(TEXT("%s non espone puntatori a oggetto (%s)"),
			*Struct->GetName(), bFound ? *OffendingPath : TEXT("nessuno")), bFound);
	}

	// Il contrappunto: il rilevatore funziona davvero. Senza questo, un `FindGameplayPointer` rotto
	// direbbe «nessun puntatore» su qualunque cosa, e i quattro controlli sopra sarebbero verdi a vuoto.
	FString SanityPath;
	TestTrue(TEXT("il rilevatore trova un puntatore quando c'e' (controprova su URTHeroData)"),
		FindGameplayPointer(URTHeroData::StaticClass(), SanityPath));

	return true;
}

// ------------------------------------------------------------------------------------------------
// Le etichette: dove si ancorano, e perche' fuori dal poligono.
// ------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHeroProfileRadarLabelsSitOutsideTest,
	"RefactorTactics.HeroProfile.RadarLabelsSitOutside",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHeroProfileRadarLabelsSitOutsideTest::RunTest(const FString&)
{
	const FVector2D Center(200.f, 200.f);
	const float Radius = 80.f;
	const float Padding = 12.f;
	const int32 AxisCount = 6;

	for (int32 Index = 0; Index < AxisCount; ++Index)
	{
		const FVector2D Anchor = URTHeroRadarWidget::ComputeAxisLabelAnchor(Index, AxisCount, Center, Radius, Padding);

		// 🔴 Fuori dal poligono, sempre: un'etichetta dentro la figura la copre proprio dove il giocatore
		// guarda per leggerne la forma.
		const float Distance = FVector2D::Distance(Anchor, Center);
		TestTrue(FString::Printf(TEXT("l'etichetta %d sta fuori dal raggio (%.1f > %.1f)"), Index, Distance, Radius),
			Distance > Radius);
		TestTrue(FString::Printf(TEXT("e alla distanza attesa (%.1f)"), Distance),
			FMath::Abs(Distance - (Radius + Padding)) < RadarTolerance);

		// Sulla direzione del proprio asse: il vertice a fondoscala e l'etichetta devono essere allineati
		// col centro, o la legenda indicherebbe l'asse sbagliato.
		TArray<FVector2D> Ring;
		URTHeroRadarWidget::ComputeRingPoints(AxisCount, Center, Radius, Ring);
		const FVector2D ToVertex = (Ring[Index] - Center).GetSafeNormal();
		const FVector2D ToLabel = (Anchor - Center).GetSafeNormal();
		TestTrue(FString::Printf(TEXT("l'etichetta %d e' sulla direzione del suo asse"), Index),
			FVector2D::DotProduct(ToVertex, ToLabel) > 0.999f);
	}

	// Il primo asse punta in alto, quindi la sua etichetta sta SOPRA il centro.
	const FVector2D First = URTHeroRadarWidget::ComputeAxisLabelAnchor(0, AxisCount, Center, Radius, Padding);
	TestTrue(TEXT("la prima etichetta sta sopra il centro"), First.Y < Center.Y);

	// ⚠️ Padding a zero non e' un errore: l'etichetta tocca il bordo. E' una scelta di stile, non un dato
	// rotto, quindi non ha senso rifiutarla.
	const FVector2D NoPad = URTHeroRadarWidget::ComputeAxisLabelAnchor(0, AxisCount, Center, Radius, 0.f);
	TestTrue(TEXT("senza padding l'etichetta sta sul bordo"),
		FMath::Abs(FVector2D::Distance(NoPad, Center) - Radius) < RadarTolerance);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
