// `M_HexVolume` (#3706, [D-508]): l'asset e' l'output di una ricetta (`D-229`), e questi test tengono i due insieme.
//
// 🔑 **Che cosa proteggono.** Il materiale legge i dati per istanza a indici che il runtime decide
// (`RTLayerPresentation`): se la disposizione dei canali cambiasse e l'`.uasset` restasse quello di prima, un muro
// leggerebbe la trasparenza dove la board scrive la tinta, senza un errore.
//
// ⚠️ **Si misura l'asset, non lo shader**: la suite gira in `-NullRHI`. Che la tinta si VEDA resta della voce PIE.

#include "Misc/AutomationTest.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPerInstanceCustomData.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialFunctionInterface.h"
#include "UObject/Package.h"

#include "Content/RTMaterialRecipe.h"
#include "Content/RTHexVolumeMaterial.h"
#include "Map/RTLayerPresentation.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	UMaterialExpressionCustom* HvmFindCustom(UMaterial* Material, int32& OutCount)
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

	const UMaterialExpression* HvmPin(const UMaterialExpressionCustom* Custom, const TCHAR* Name)
	{
		const FCustomInput* Pin = Custom->Inputs.FindByPredicate([Name](const FCustomInput& I) { return I.InputName == FName(Name); });
		return Pin ? Pin->Input.Expression : nullptr;
	}

	/** Le asserzioni che valgono sia per l'asset salvato sia per un grafo appena costruito dalla ricetta. */
	void HvmCheckGraph(FAutomationTestBase& Test, UMaterial* Material, const TCHAR* Chi)
	{
		Test.TestTrue(*FString::Printf(TEXT("%s: e' un materiale di superficie"), Chi), Material->MaterialDomain == MD_Surface);
		Test.TestTrue(*FString::Printf(TEXT("%s: e' Masked, perche' e' la maschera che fa vedere sotto un volume di un piano sopra"), Chi),
			Material->GetBlendMode() == BLEND_Masked);
		Test.TestTrue(*FString::Printf(TEXT("%s: e' usabile sulle mesh istanziate (senza, il pacchetto disegna il materiale di default)"), Chi),
			Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));

		// 🔴 Si contano TUTTE le espressioni: una ricostruzione che lasciasse indietro dei nodi li lascerebbe nel binario.
		Test.TestEqual(*FString::Printf(TEXT("%s: il grafo ha le espressioni della ricetta, e nessun nodo orfano"), Chi),
			RTMaterialRecipe::ExpressionCount(Material), RTHexVolumeMaterial::ExpressionCount);

		int32 Quanti = 0;
		UMaterialExpressionCustom* Custom = HvmFindCustom(Material, Quanti);
		if (!Test.TestEqual(*FString::Printf(TEXT("%s: un solo nodo Custom"), Chi), Quanti, 1) || Custom == nullptr)
		{
			return;
		}
		Test.TestTrue(*FString::Printf(TEXT("%s: il codice del nodo e' quello generato dalla ricetta (rilancia RTBuildHexVolumeMaterial)"), Chi),
			Custom->Code.Equals(RTHexVolumeMaterial::CustomCode(), ESearchCase::CaseSensitive));

		// Il colore proprio: un parametro, col default dei prismi.
		const UMaterialExpressionVectorParameter* Own =
			Cast<UMaterialExpressionVectorParameter>(HvmPin(Custom, RTHexVolumeMaterial::PinOwn));
		if (Test.TestNotNull(*FString::Printf(TEXT("%s: il colore proprio e' un parametro"), Chi), Own))
		{
			Test.TestTrue(*FString::Printf(TEXT("%s: e si chiama come il runtime lo scrive"), Chi),
				Own->ParameterName == RTLayerPresentation::VolumeParamOwnColor);
			Test.TestTrue(*FString::Printf(TEXT("%s: il suo default e' il colore proprio dei prismi"), Chi),
				Own->DefaultValue.Equals(RTLayerPresentation::VolumeOwnColor(/*bEdgePanel=*/ false), 1.e-4f));
		}

		// 🔴 Gli indici dei dati per istanza: sono il contratto fra la board e il materiale.
		const UMaterialExpressionPerInstanceCustomData3Vector* Tint =
			Cast<UMaterialExpressionPerInstanceCustomData3Vector>(HvmPin(Custom, RTHexVolumeMaterial::PinTint));
		if (Test.TestNotNull(*FString::Printf(TEXT("%s: la tinta sono tre dati per istanza"), Chi), Tint))
		{
			Test.TestEqual(*FString::Printf(TEXT("%s: la tinta parte dal canale 0"), Chi), static_cast<int32>(Tint->DataIndex), 0);
		}
		const TPair<const TCHAR*, int32> Scalari[] = {
			{ RTHexVolumeMaterial::PinDesaturation, RTLayerPresentation::FirstChannel + 1 },
			{ RTHexVolumeMaterial::PinExposureStops, RTLayerPresentation::FirstChannel + 2 },
			{ RTHexVolumeMaterial::PinTintStrength, RTLayerPresentation::VolumeTintStrengthChannel },
		};
		for (const TPair<const TCHAR*, int32>& Atteso : Scalari)
		{
			const UMaterialExpressionPerInstanceCustomData* Nodo =
				Cast<UMaterialExpressionPerInstanceCustomData>(HvmPin(Custom, Atteso.Key));
			if (Test.TestNotNull(*FString::Printf(TEXT("%s: il pin %s e' un dato per istanza"), Chi, Atteso.Key), Nodo))
			{
				Test.TestEqual(*FString::Printf(TEXT("%s: il pin %s legge il canale che la board scrive"), Chi, Atteso.Key),
					static_cast<int32>(Nodo->DataIndex), Atteso.Value);
				Test.TestEqual(*FString::Printf(TEXT("%s: il pin %s e' neutro a zero"), Chi, Atteso.Key), Nodo->ConstDefaultValue, 0.f);
			}
		}

		// Le quattro uscite: collegate, e con la costante in linea SPENTA (la lezione di `M_HexCell`, #3704).
		const EMaterialProperty Uscite[] = { MP_BaseColor, MP_Specular, MP_Roughness, MP_OpacityMask };
		for (const EMaterialProperty Uscita : Uscite)
		{
			FMaterialInputDescription Ingresso;
			const bool bHa = Material->GetExpressionInputDescription(Uscita, Ingresso);
			const FString Nome = UEnum::GetValueAsString(Uscita);
			if (Test.TestTrue(*FString::Printf(TEXT("%s: %s ha un nodo collegato"), Chi, *Nome),
				bHa && Ingresso.Input != nullptr && Ingresso.Input->Expression != nullptr))
			{
				Test.TestFalse(*FString::Printf(TEXT("%s: la costante in linea di %s e' spenta (altrimenti il nodo non si compila)"), Chi, *Nome),
					Ingresso.bUseConstant);
			}
		}

		FMaterialInputDescription Colore;
		if (Material->GetExpressionInputDescription(MP_BaseColor, Colore) && Colore.Input != nullptr)
		{
			Test.TestTrue(*FString::Printf(TEXT("%s: al colore arriva il nodo Custom"), Chi), Colore.Input->Expression == Custom);
			Test.TestEqual(*FString::Printf(TEXT("%s: dalla sua uscita principale"), Chi), Colore.Input->OutputIndex, 0);
		}

		// 🔴 Il riflesso: dall'uscita in piu' dello stesso nodo. Se arrivasse dall'uscita principale il riflesso sarebbe
		// il colore, e nessun errore lo direbbe.
		if (Test.TestEqual(*FString::Printf(TEXT("%s: il nodo Custom ha una sola uscita in piu'"), Chi), Custom->AdditionalOutputs.Num(), 1))
		{
			Test.TestTrue(*FString::Printf(TEXT("%s: e si chiama come la variabile che il codice scrive"), Chi),
				Custom->AdditionalOutputs[0].OutputName == FName(RTHexVolumeMaterial::OutputSpecular));
			Test.TestTrue(*FString::Printf(TEXT("%s: ed e' uno scalare"), Chi), Custom->AdditionalOutputs[0].OutputType == CMOT_Float1);
		}
		FMaterialInputDescription Riflesso;
		if (Material->GetExpressionInputDescription(MP_Specular, Riflesso) && Riflesso.Input != nullptr)
		{
			Test.TestTrue(*FString::Printf(TEXT("%s: al riflesso arriva lo stesso nodo Custom"), Chi), Riflesso.Input->Expression == Custom);
			Test.TestEqual(*FString::Printf(TEXT("%s: dalla sua uscita in piu', non da quella del colore"), Chi), Riflesso.Input->OutputIndex, 1);
		}

		// 🔴 La maschera: retino( 1 - trasparenza ), con la trasparenza letta dal canale che la board scrive.
		FMaterialInputDescription Maschera;
		const UMaterialExpressionMaterialFunctionCall* Retino = nullptr;
		if (Material->GetExpressionInputDescription(MP_OpacityMask, Maschera) && Maschera.Input != nullptr)
		{
			Retino = Cast<UMaterialExpressionMaterialFunctionCall>(Maschera.Input->Expression);
		}
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: alla maschera arriva la funzione del retino"), Chi), Retino))
		{
			return;
		}
		Test.TestTrue(*FString::Printf(TEXT("%s: la funzione e' quella del motore (%s)"), Chi, RTHexVolumeMaterial::DitherFunctionPath),
			Retino->MaterialFunction != nullptr
			&& Retino->MaterialFunction->GetPathName().Equals(RTHexVolumeMaterial::DitherFunctionPath, ESearchCase::IgnoreCase));
		const FFunctionExpressionInput* Soglia = Retino->FunctionInputs.FindByPredicate(
			[](const FFunctionExpressionInput& In) { return In.Input.InputName == FName(RTHexVolumeMaterial::DitherInputName); });
		const UMaterialExpressionOneMinus* Opacita = Soglia ? Cast<UMaterialExpressionOneMinus>(Soglia->Input.Expression) : nullptr;
		if (Test.TestNotNull(*FString::Printf(TEXT("%s: la soglia del retino e' 1 meno qualcosa"), Chi), Opacita))
		{
			const UMaterialExpressionPerInstanceCustomData* Trasparenza =
				Cast<UMaterialExpressionPerInstanceCustomData>(Opacita->Input.Expression);
			if (Test.TestNotNull(*FString::Printf(TEXT("%s: e quel qualcosa e' un dato per istanza"), Chi), Trasparenza))
			{
				Test.TestEqual(*FString::Printf(TEXT("%s: la trasparenza si legge dal canale che la board scrive"), Chi),
					static_cast<int32>(Trasparenza->DataIndex), RTLayerPresentation::FirstChannel);
			}
		}
	}
}

/**
 * **L'ordine nel codice e' la regola: desaturazione, tinta, luce.** Una tinta messa prima della desaturazione
 * perderebbe il suo viola su un piano sotto; messa dopo la luce dominerebbe un corpo scurito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVolumeRecipeCodeTest,
	"RefactorTactics.HexVolume.RecipeCodeTintsBetweenDesaturationAndLight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHexVolumeRecipeCodeTest::RunTest(const FString&)
{
	const FString Code = RTHexVolumeMaterial::CustomCode();
	const int32 Desaturazione = Code.Find(TEXT("saturate(Desaturation)"), ESearchCase::CaseSensitive);
	const int32 Luce = Code.Find(TEXT("exp2(ExposureStops)"), ESearchCase::CaseSensitive);
	const int32 Tinta = Code.Find(TEXT("Tint.rgb"), ESearchCase::CaseSensitive);
	TestTrue(TEXT("il codice desatura, schiarisce o scurisce, e tinge"), Desaturazione >= 0 && Luce >= 0 && Tinta >= 0);
	TestTrue(TEXT("la tinta viene DOPO la desaturazione: altrimenti un piano sotto perderebbe il suo viola"), Tinta > Desaturazione);
	TestTrue(TEXT("e PRIMA della luce: lo scurimento vale per tutto il volume, tinta compresa"), Tinta < Luce);
	TestTrue(TEXT("la forza della tinta e' l'alfa della sua interpolazione"),
		Code.Contains(TEXT("lerp(Body, Tint.rgb, saturate(TintStrength))"), ESearchCase::CaseSensitive));

	// 🔴 Il riflesso segue la luce di un piano sotto, e non supera quello di base su un piano sopra.
	TestTrue(TEXT("il riflesso e' quello di base per la luce, limitata a uno: sotto scurisce, sopra resta com'e'"),
		Code.Contains(FString::Printf(TEXT("%s = %s * saturate(Light);"), RTHexVolumeMaterial::OutputSpecular,
			*FString::SanitizeFloat(RTHexVolumeMaterial::BaseSpecular)), ESearchCase::CaseSensitive));
	TestTrue(TEXT("e la luce e' una sola, la stessa del colore"),
		Code.Contains(TEXT("float Light = exp2(ExposureStops);"), ESearchCase::CaseSensitive)
		&& Code.Contains(TEXT("return saturate(Body * Light);"), ESearchCase::CaseSensitive));
	return true;
}

/** **La ricetta costruisce il grafo che dichiara**, su un materiale transiente: non dipende dall'asset salvato. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVolumeRecipeBuildsTest,
	"RefactorTactics.HexVolume.RecipeBuildsTheGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHexVolumeRecipeBuildsTest::RunTest(const FString&)
{
	UMaterial* Materiale = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("il materiale transiente"), Materiale)) { return false; }
	if (!TestTrue(TEXT("la ricetta costruisce il grafo"), RTHexVolumeMaterial::BuildGraph(Materiale))) { return false; }
	HvmCheckGraph(*this, Materiale, TEXT("ricetta"));

	TestTrue(TEXT("la ricetta si puo' rilanciare"), RTHexVolumeMaterial::BuildGraph(Materiale));
	TestEqual(TEXT("e non lascia indietro nessun nodo della passata precedente"),
		RTMaterialRecipe::ExpressionCount(Materiale), RTHexVolumeMaterial::ExpressionCount);
	HvmCheckGraph(*this, Materiale, TEXT("seconda passata"));
	return true;
}

/**
 * 🔴 **L'asset salvato e' quello che la ricetta scrive oggi.** Se cade dopo un cambio in `RTLayerPresentation` o
 * nella ricetta, l'asset e' rimasto indietro: si rilancia `RTBuildHexVolumeMaterial` dal clone principale e si
 * committa il binario.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexVolumeAssetMatchesRecipeTest,
	"RefactorTactics.HexVolume.MaterialMatchesTheRecipe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTHexVolumeAssetMatchesRecipeTest::RunTest(const FString&)
{
	const FString Percorso = FString::Printf(TEXT("%s.%s"), RTLayerPresentation::VolumeMaterialPackage, RTLayerPresentation::VolumeMaterialName);
	UMaterial* Materiale = LoadObject<UMaterial>(nullptr, *Percorso);
	if (!TestNotNull(*FString::Printf(TEXT("l'asset %s esiste (lo scrive RTBuildHexVolumeMaterial)"), *Percorso), Materiale))
	{
		return false;
	}
	HvmCheckGraph(*this, Materiale, TEXT("asset"));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
