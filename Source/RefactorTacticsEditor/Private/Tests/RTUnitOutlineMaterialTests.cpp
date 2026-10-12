// `M_UnitLayerOutline` (#3705, [D-508]): l'asset e' l'output di una ricetta (`D-229`), e questi test tengono i due
// insieme.
//
// 🔑 **Che cosa proteggono, e da cosa.** Il materiale e' un nodo Custom il cui codice porta i numeri degli stencil del
// runtime. Il rischio non e' che qualcuno lo disegni male: e' che `RTUnitLayerOutline` cambi — uno stencil, quale
// passa attraverso la geometria — e l'`.uasset` resti quello di prima. Il repository resterebbe verde mostrando
// contorni che nessun documento descrive.
//
// ⚠️ **Si misura l'asset, non lo shader**: la suite gira in `-NullRHI`. Che il contorno si VEDA resta della voce PIE.

#include "Misc/AutomationTest.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UObject/Package.h"

#include "Content/RTMaterialRecipe.h"
#include "Content/RTUnitOutlineMaterial.h"
#include "Unit/RTUnitLayerOutline.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	UMaterialExpressionCustom* UomFindCustom(UMaterial* Material, int32& OutCount)
	{
		UMaterialExpressionCustom* Found = nullptr;
		OutCount = 0;
		for (UMaterialExpression* Expression : Material->GetExpressions())
		{
			if (UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expression))
			{
				++OutCount;
				Found = Custom;
			}
		}
		return Found;
	}

	/** Le asserzioni che valgono sia per l'asset salvato sia per un grafo appena costruito dalla ricetta. */
	void UomCheckGraph(FAutomationTestBase& Test, UMaterial* Material, const TCHAR* Chi)
	{
		Test.TestTrue(*FString::Printf(TEXT("%s: e' un post-process"), Chi), Material->MaterialDomain == MD_PostProcess);
		Test.TestTrue(*FString::Printf(TEXT("%s: gira dopo il DOF, prima dell'antialiasing temporale"), Chi),
			Material->BlendableLocation == BL_SceneColorAfterDOF);

		// 🔴 Si contano TUTTE le espressioni: una ricostruzione che lasciasse indietro dei nodi li lascerebbe nel binario.
		Test.TestEqual(*FString::Printf(TEXT("%s: il grafo ha le espressioni della ricetta, e nessun nodo orfano"), Chi),
			RTMaterialRecipe::ExpressionCount(Material), RTUnitOutlineMaterial::ExpressionCount);

		int32 Quanti = 0;
		UMaterialExpressionCustom* Custom = UomFindCustom(Material, Quanti);
		if (!Test.TestEqual(*FString::Printf(TEXT("%s: un solo nodo Custom"), Chi), Quanti, 1) || Custom == nullptr)
		{
			return;
		}
		Test.TestEqual(*FString::Printf(TEXT("%s: il nodo Custom e' quello della ricetta"), Chi),
			Custom->Description, FString(RTUnitOutlineMaterial::CustomDescription));
		Test.TestTrue(*FString::Printf(TEXT("%s: restituisce un colore"), Chi), Custom->OutputType == CMOT_Float3);
		// 🔴 Il cuore: il codice salvato e' quello che la ricetta genera OGGI dagli stencil del runtime.
		// ⚠️ `TestEqual` su `FString` ignora le maiuscole: qui si confronta con `Equals(..., CaseSensitive)`.
		Test.TestTrue(*FString::Printf(TEXT("%s: il codice del nodo e' quello generato dalla ricetta (rilancia RTBuildUnitOutlineMaterial)"), Chi),
			Custom->Code.Equals(RTUnitOutlineMaterial::CustomCode(), ESearchCase::CaseSensitive));

		// I pin, per nome, e a che cosa sono collegati. Le quattro scene texture servono tutte: il codice le legge con
		// `SceneTextureFetch`, e una non dichiarata sarebbe una texture che nessuno ha legato.
		const TPair<const TCHAR*, ESceneTextureId> SceneTextures[] = {
			{ RTUnitOutlineMaterial::PinScene, PPI_PostProcessInput0 },
			{ RTUnitOutlineMaterial::PinStencil, PPI_CustomStencil },
			{ RTUnitOutlineMaterial::PinUnitDepth, PPI_CustomDepth },
			{ RTUnitOutlineMaterial::PinSceneDepth, PPI_SceneDepth },
		};
		for (const TPair<const TCHAR*, ESceneTextureId>& Atteso : SceneTextures)
		{
			const FCustomInput* Pin = Custom->Inputs.FindByPredicate(
				[&Atteso](const FCustomInput& I) { return I.InputName == FName(Atteso.Key); });
			const UMaterialExpressionSceneTexture* Nodo =
				Pin ? Cast<UMaterialExpressionSceneTexture>(Pin->Input.Expression) : nullptr;
			if (Test.TestNotNull(*FString::Printf(TEXT("%s: il pin %s e' una scene texture"), Chi, Atteso.Key), Nodo))
			{
				Test.TestTrue(*FString::Printf(TEXT("%s: il pin %s legge la scene texture giusta"), Chi, Atteso.Key),
					Nodo->SceneTextureId == Atteso.Value);
			}
		}

		const FName Colori[] = { RTUnitLayerOutline::ParamColorActive, RTUnitLayerOutline::ParamColorAbove,
			RTUnitLayerOutline::ParamColorBelow };
		for (const FName Nome : Colori)
		{
			const FCustomInput* Pin = Custom->Inputs.FindByPredicate([Nome](const FCustomInput& I) { return I.InputName == Nome; });
			const UMaterialExpressionVectorParameter* Nodo =
				Pin ? Cast<UMaterialExpressionVectorParameter>(Pin->Input.Expression) : nullptr;
			if (Test.TestNotNull(*FString::Printf(TEXT("%s: il pin %s e' un parametro di colore"), Chi, *Nome.ToString()), Nodo))
			{
				Test.TestTrue(*FString::Printf(TEXT("%s: il parametro si chiama come il runtime lo scrive (%s)"), Chi, *Nome.ToString()),
					Nodo->ParameterName == Nome);
			}
		}
		{
			const FName Nome = RTUnitLayerOutline::ParamThickness;
			const FCustomInput* Pin = Custom->Inputs.FindByPredicate([Nome](const FCustomInput& I) { return I.InputName == Nome; });
			const UMaterialExpressionScalarParameter* Nodo =
				Pin ? Cast<UMaterialExpressionScalarParameter>(Pin->Input.Expression) : nullptr;
			if (Test.TestNotNull(*FString::Printf(TEXT("%s: il pin dello spessore e' un parametro scalare"), Chi), Nodo))
			{
				Test.TestTrue(*FString::Printf(TEXT("%s: e si chiama come il runtime lo scrive"), Chi), Nodo->ParameterName == Nome);
				Test.TestTrue(*FString::Printf(TEXT("%s: il suo default e' quello dichiarato"), Chi),
					FMath::IsNearlyEqual(Nodo->DefaultValue, RTUnitLayerOutline::DefaultThicknessPx));
			}
		}

		// 🔴 L'uscita: il nodo Custom arriva all'emissivo, E la costante in linea del pin e' spenta. Con `UseConstant`
		// acceso il traduttore compila la costante e il nodo non viene letto, senza errori: e' il difetto di
		// `M_HexCell` fra #3679 e #3704, e la ricetta lo spegne esplicitamente.
		FMaterialInputDescription Uscita;
		const bool bHa = Material->GetExpressionInputDescription(MP_EmissiveColor, Uscita);
		if (Test.TestTrue(*FString::Printf(TEXT("%s: l'emissivo ha un ingresso"), Chi), bHa && Uscita.Input != nullptr))
		{
			Test.TestTrue(*FString::Printf(TEXT("%s: all'emissivo arriva il nodo Custom"), Chi), Uscita.Input->Expression == Custom);
			Test.TestFalse(*FString::Printf(TEXT("%s: la costante in linea dell'emissivo e' spenta (altrimenti il nodo non si compila)"), Chi),
				Uscita.bUseConstant);
		}
	}
}

/**
 * **Il codice del nodo porta gli stencil del runtime, e solo «sotto» passa attraverso la geometria.**
 *
 * Pura: non serve un asset. E' la meta' della DoD di #3705 che dice *«un'unita' sotto il piano attivo ha il
 * contorno configurato per disegnarsi attraverso la geometria»*: il flag e' `ShowsThroughGeometry`, e qui si
 * misura che il materiale lo ESEGUA — che la condizione nel codice nomini quello stencil, e nessun altro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOutlineRecipeCodeTest,
	"RefactorTactics.UnitOutline.RecipeCodeCarriesTheRuntimeStencils",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTUnitOutlineRecipeCodeTest::RunTest(const FString&)
{
	const FString Code = RTUnitOutlineMaterial::CustomCode();

	// La riga che decide «attraverso la geometria», isolata: e' l'unica che comincia cosi'.
	FString Riga;
	TArray<FString> Righe;
	Code.ParseIntoArrayLines(Righe);
	int32 Trovate = 0;
	for (const FString& R : Righe)
	{
		if (R.TrimStart().StartsWith(TEXT("bool bShown ="), ESearchCase::CaseSensitive))
		{
			Riga = R;
			++Trovate;
		}
	}
	if (!TestEqual(TEXT("una sola riga decide chi passa attraverso la geometria"), Trovate, 1)) { return false; }

	const uint8 Tutti[] = { RTUnitLayerOutline::StencilActive, RTUnitLayerOutline::StencilAbove, RTUnitLayerOutline::StencilBelow };
	int32 Attraverso = 0;
	for (const uint8 Stencil : Tutti)
	{
		const FString Confronto = FString::Printf(TEXT("abs(S - %d.0) < 0.5"), Stencil);
		const bool bNelCodice = Riga.Contains(Confronto, ESearchCase::CaseSensitive);
		TestEqual(*FString::Printf(TEXT("lo stencil %d passa attraverso la geometria nel materiale come nel runtime"), Stencil),
			bNelCodice, RTUnitLayerOutline::ShowsThroughGeometry(Stencil));
		Attraverso += bNelCodice ? 1 : 0;

		// E ogni stencil ha il suo ramo di colore: senza, un'unita' avrebbe lo stencil e nessun contorno.
		TestTrue(*FString::Printf(TEXT("lo stencil %d ha un ramo di colore nel codice"), Stencil),
			Code.Contains(FString::Printf(TEXT("abs(Found - %d.0) < 0.5"), Stencil), ESearchCase::CaseSensitive));
	}
	TestEqual(TEXT("controllo positivo: almeno uno stencil passa attraverso la geometria, ed e' uno solo"), Attraverso, 1);

	// Gli altri si confrontano con la profondita' della scena: se sparisse, ogni contorno passerebbe i muri.
	TestTrue(TEXT("chi non passa si confronta con la profondita' della scena"),
		Code.Contains(TEXT("SceneTextureFetch(PPI_SceneDepth"), ESearchCase::CaseSensitive)
		&& Code.Contains(TEXT("SceneTextureFetch(PPI_CustomDepth"), ESearchCase::CaseSensitive));
	// Dentro la sagoma il modello resta com'e': il contorno sta FUORI.
	TestTrue(TEXT("il contorno e' esterno: dentro la sagoma il colore della scena non si tocca"),
		Code.Contains(TEXT("if (!bInside"), ESearchCase::CaseSensitive));
	return true;
}

/**
 * **La ricetta costruisce il grafo che dichiara**, su un materiale transiente: non dipende dall'asset salvato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOutlineRecipeBuildsTest,
	"RefactorTactics.UnitOutline.RecipeBuildsTheGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTUnitOutlineRecipeBuildsTest::RunTest(const FString&)
{
	UMaterial* Materiale = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("il materiale transiente"), Materiale)) { return false; }
	if (!TestTrue(TEXT("la ricetta costruisce il grafo"), RTUnitOutlineMaterial::BuildGraph(Materiale))) { return false; }
	UomCheckGraph(*this, Materiale, TEXT("ricetta"));

	// Idempotente: una seconda passata non lascia due nodi Custom.
	TestTrue(TEXT("la ricetta si puo' rilanciare"), RTUnitOutlineMaterial::BuildGraph(Materiale));
	TestEqual(TEXT("e non lascia indietro nessun nodo della passata precedente"),
		RTMaterialRecipe::ExpressionCount(Materiale), RTUnitOutlineMaterial::ExpressionCount);
	UomCheckGraph(*this, Materiale, TEXT("seconda passata"));
	return true;
}

/**
 * 🔴 **L'asset salvato e' quello che la ricetta scrive oggi.** Se questo test cade dopo un cambio in
 * `RTUnitLayerOutline` o nella ricetta, l'asset e' rimasto indietro: si rilancia `RTBuildUnitOutlineMaterial` dal
 * clone principale e si committa il binario.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOutlineAssetMatchesRecipeTest,
	"RefactorTactics.UnitOutline.MaterialMatchesTheRecipe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTUnitOutlineAssetMatchesRecipeTest::RunTest(const FString&)
{
	const FString Percorso = FString::Printf(TEXT("%s.%s"), RTUnitLayerOutline::MaterialPackage, RTUnitLayerOutline::MaterialName);
	UMaterial* Materiale = LoadObject<UMaterial>(nullptr, *Percorso);
	if (!TestNotNull(*FString::Printf(TEXT("l'asset %s esiste (lo scrive RTBuildUnitOutlineMaterial)"), *Percorso), Materiale))
	{
		return false;
	}
	UomCheckGraph(*this, Materiale, TEXT("asset"));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
