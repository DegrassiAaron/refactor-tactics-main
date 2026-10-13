// Il master dei materiali graybox, `M_Graybox_Master` (#1714): l'asset e' l'output di una ricetta (`D-229`), e questi
// test tengono i due insieme (#3725).
//
// 🔑 **Che cosa proteggono.** La ricetta svuota il grafo prima di ricostruirlo. La funzione del motore che lo faceva
// (`UMaterialEditingLibrary`, UE 5.8) scorre le espressioni mentre le toglie, e a ogni rilancio ne lascia indietro una
// parte: niente a schermo, e un nodo orfano nel binario. Con #3706 e' successo a `M_UnitLayerOutline`. Qui si
// contano TUTTE le espressioni: dopo una passata e dopo due, su un grafo sporco, e sull'asset salvato.
//
// 🔴 **Il test che distingue le due funzioni e' quello sul grafo sporco**, `MasterRecipeClearsWhatItFinds`. Su un
// grafo di DUE espressioni, come quello del master, la funzione del motore finisce per toglierle entrambe: scorre una
// vista lunga quanto l'elenco all'inizio, e dopo la prima cancellazione il secondo posto contiene ancora la seconda
// espressione. Misurato il 2026-10-13: rimessa la funzione del motore, la seconda passata restava verde. Da tre
// espressioni in su ne lascia indietro.
//
// ⚠️ **Si misura l'asset, non lo shader**: la suite gira in `-NullRHI`.

#include "Misc/AutomationTest.h"

#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UObject/Package.h"

#include "Content/RTGrayboxMaterials.h"
#include "Content/RTMaterialRecipe.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	void GbmCheckGraph(FAutomationTestBase& Test, UMaterial* Material, const TCHAR* Quando)
	{
		Test.TestEqual(*FString::Printf(TEXT("%s: il grafo ha le sole espressioni della ricetta"), Quando),
			RTMaterialRecipe::ExpressionCount(Material), RTGraybox::MasterExpressionCount);

		UMaterialExpression* Colore = nullptr;
		UMaterialExpression* Ruvidita = nullptr;
		int32 Colori = 0;
		int32 Ruvidi = 0;
		for (UMaterialExpression* Expression : Material->GetExpressions())
		{
			if (UMaterialExpressionVectorParameter* Vettore = Cast<UMaterialExpressionVectorParameter>(Expression))
			{
				if (Vettore->ParameterName == RTGraybox::ParamBaseColor) { ++Colori; Colore = Vettore; }
			}
			else if (UMaterialExpressionScalarParameter* Scalare = Cast<UMaterialExpressionScalarParameter>(Expression))
			{
				if (Scalare->ParameterName == RTGraybox::ParamRoughness) { ++Ruvidi; Ruvidita = Scalare; }
			}
		}
		Test.TestEqual(*FString::Printf(TEXT("%s: un solo parametro del colore di base"), Quando), Colori, 1);
		Test.TestEqual(*FString::Printf(TEXT("%s: un solo parametro della ruvidita'"), Quando), Ruvidi, 1);

		// Un parametro che c'e' ma non e' collegato non veste niente: l'istanza lo scriverebbe a vuoto.
		const UMaterialEditorOnlyData* Dati = Material->GetEditorOnlyData();
		if (Test.TestNotNull(*FString::Printf(TEXT("%s: i dati di Editor del materiale"), Quando), Dati))
		{
			Test.TestTrue(*FString::Printf(TEXT("%s: il colore di base e' collegato al suo parametro"), Quando),
				Colore != nullptr && Dati->BaseColor.Expression == Colore);
			Test.TestTrue(*FString::Printf(TEXT("%s: la ruvidita' e' collegata al suo parametro"), Quando),
				Ruvidita != nullptr && Dati->Roughness.Expression == Ruvidita);
		}
	}
}

/**
 * **La ricetta costruisce il grafo che dichiara, e si puo' rilanciare**, su un materiale transiente: non dipende
 * dall'asset salvato. La seconda passata e' il caso di #3725: su un grafo che esiste gia', lo svuotamento non deve
 * lasciare indietro niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayboxMasterRecipeBuildsTest,
	"RefactorTactics.Graybox.MasterRecipeBuildsTheGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxMasterRecipeBuildsTest::RunTest(const FString&)
{
	UMaterial* Materiale = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("il materiale transiente"), Materiale)) { return false; }
	if (!TestTrue(TEXT("la ricetta costruisce il grafo"), RTGraybox::BuildMasterGraph(Materiale))) { return false; }
	GbmCheckGraph(*this, Materiale, TEXT("ricetta"));

	TestTrue(TEXT("la ricetta si puo' rilanciare"), RTGraybox::BuildMasterGraph(Materiale));
	GbmCheckGraph(*this, Materiale, TEXT("seconda passata"));
	return true;
}

/**
 * 🔴 **La ricetta svuota il grafo qualunque cosa ci trovi**, anche nodi che non sono suoi: e' cio' che la ricetta
 * dichiara («un nodo autorato a mano lo perderebbe qui»), ed e' il caso in cui la funzione del motore lascia
 * indietro qualcosa. Il grafo di partenza ha cinque espressioni: le due della ricetta e tre costanti estranee.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayboxMasterRecipeClearsTest,
	"RefactorTactics.Graybox.MasterRecipeClearsWhatItFinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxMasterRecipeClearsTest::RunTest(const FString&)
{
	UMaterial* Materiale = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("il materiale transiente"), Materiale)) { return false; }
	if (!TestTrue(TEXT("la ricetta costruisce il grafo"), RTGraybox::BuildMasterGraph(Materiale))) { return false; }

	const int32 Estranee = 3;
	for (int32 Indice = 0; Indice < Estranee; ++Indice)
	{
		TestNotNull(TEXT("una costante estranea nasce"), UMaterialEditingLibrary::CreateMaterialExpression(
			Materiale, UMaterialExpressionConstant::StaticClass(), 200, 100 * Indice));
	}
	if (!TestEqual(TEXT("premessa: il grafo sporco ha le espressioni della ricetta piu' le estranee"),
		RTMaterialRecipe::ExpressionCount(Materiale), RTGraybox::MasterExpressionCount + Estranee))
	{
		return false;
	}

	TestTrue(TEXT("la ricetta si rilancia sul grafo sporco"), RTGraybox::BuildMasterGraph(Materiale));
	GbmCheckGraph(*this, Materiale, TEXT("dopo il grafo sporco"));
	return true;
}

/**
 * 🔴 **L'asset salvato e' quello che la ricetta scrive oggi.** Se cade per un'espressione in piu', l'asset porta un
 * nodo orfano: si rilancia `RTBuildGrayboxMeshes` dal clone principale e si committa il binario.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGrayboxMasterAssetMatchesRecipeTest,
	"RefactorTactics.Graybox.MasterMatchesTheRecipe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGrayboxMasterAssetMatchesRecipeTest::RunTest(const FString&)
{
	// La radice e' quella di default del commandlet (`RTBuildGrayboxMeshesCommandlet`), la stessa dei test delle mesh.
	const FString Percorso = FString::Printf(TEXT("/Game/RT/World/Graybox/%s/%s.%s"),
		RTGraybox::MaterialsFolder, RTGraybox::MasterAssetName, RTGraybox::MasterAssetName);
	UMaterial* Materiale = LoadObject<UMaterial>(nullptr, *Percorso);
	if (!TestNotNull(*FString::Printf(TEXT("l'asset %s esiste (lo scrive il commandlet del kit graybox)"), *Percorso), Materiale))
	{
		return false;
	}
	GbmCheckGraph(*this, Materiale, TEXT("asset"));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
