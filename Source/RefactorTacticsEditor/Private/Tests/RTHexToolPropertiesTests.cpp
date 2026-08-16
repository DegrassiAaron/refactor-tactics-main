#include "Misc/AutomationTest.h"

// `UInteractiveToolPropertySet` vive in `InteractiveTool.h`, non in un header col proprio nome: quello non
// esiste, e il primo tentativo l'ha inventato. Verificato con `grep -l` sui Public/ del modulo engine.
#include "InteractiveTool.h"
#include "UObject/UObjectIterator.h"

#include "Map/RTHexCellData.h" // ERTHexSurface
#include "Terrain/RTTerrainLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * I PRIMI test del modulo editor, e la ragione per cui esistono.
 *
 * Tre issue di fila (#871, #921, #931) hanno dichiarato «`RefactorTacticsEditor/` non ha test, quindi la
 * verifica e' manuale» trattandolo come un dato di fatto. Non lo era: il `Build.cs` ha gia' `Core` — dove
 * vive `Misc/AutomationTest.h` — e il modulo runtime, e `WITH_DEV_AUTOMATION_TESTS` e' definito sui target
 * Editor. I test non erano impossibili: non erano stati scritti.
 *
 * Quello che segue e' il difetto #871 promosso a invariante. Il pennello Fill scriveva una regione intera
 * con un `MoveCost` che non corrispondeva alla superficie, in silenzio, perche' gli mancava il
 * `PostEditChangeProperty` che il Paint aveva. E' stato trovato davanti all'editor, per caso, durante una
 * verifica che riguardava altro.
 */
namespace
{
	/** `Rough` costa 2 nel catalogo: e' il valore che un pennello corretto deve scrivere da solo. */
	constexpr ERTHexSurface ProbeSurface = ERTHexSurface::Rough;

	/**
	 * Un property set e' un PENNELLO se ha `Surface` e `MoveCost` **entrambi editabili**.
	 *
	 * La distinzione non e' cosmetica: `URTHexSelectToolProperties` ha gli stessi due campi ma marcati
	 * `VisibleAnywhere` — sono un readout della cella selezionata, non un pennello, e pretendere che si
	 * auto-aggiornino sarebbe sbagliato.
	 *
	 * 🔴 **`CPF_Edit` da solo NON separa i due casi**, ed e' l'errore che la prima stesura di questo test ha
	 * commesso: entrambe le macro lo impostano. Quella che discrimina e' `CPF_EditConst`, che
	 * `VisibleAnywhere` aggiunge e `EditAnywhere` no:
	 *
	 *     EditAnywhere      CPF_Edit
	 *     VisibleAnywhere   CPF_Edit | CPF_EditConst
	 *
	 * Il test e' nato rosso proprio qui, su tutti e due i casi, e la semantica del flag era stata dedotta
	 * dal nome della macro invece che verificata.
	 */
	bool EditabileDavvero(const FProperty* P)
	{
		return P->HasAnyPropertyFlags(CPF_Edit) && !P->HasAnyPropertyFlags(CPF_EditConst);
	}

	bool IsBrushLike(const UClass* Class, FProperty*& OutSurface, FProperty*& OutMoveCost)
	{
		OutSurface = Class->FindPropertyByName(TEXT("Surface"));
		OutMoveCost = Class->FindPropertyByName(TEXT("MoveCost"));
		if (!OutSurface || !OutMoveCost)
		{
			return false;
		}
		return EditabileDavvero(OutSurface) && EditabileDavvero(OutMoveCost);
	}
}

/**
 * Ogni pennello del mode Hex Map allinea `MoveCost` al catalogo quando cambia `Surface`.
 *
 * L'elenco dei pennelli NON e' trascritto: si itera la reflection su tutte le sottoclassi di
 * `UInteractiveToolPropertySet`, cosi' un tool aggiunto domani entra nel test da solo. Una lista scritta a
 * mano resterebbe verde mentre il tool nuovo nasce col difetto — che e' esattamente come #871 e' arrivato
 * fino all'editor.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBrushMoveCostFollowsSurfaceTest,
	"RefactorTactics.HexEditor.BrushMoveCostFollowsSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBrushMoveCostFollowsSurfaceTest::RunTest(const FString&)
{
	const FRTTerrainDef Def = URTTerrainLibrary::FindTerrainDef(ProbeSurface);
	TestTrue(TEXT("il catalogo da' a Rough un costo diverso dal default 1"), Def.MoveCost != 1);

	int32 Esaminati = 0;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class->IsChildOf(UInteractiveToolPropertySet::StaticClass())
			|| Class->HasAnyClassFlags(CLASS_Abstract)
			|| !Class->GetName().StartsWith(TEXT("RTHex")))
		{
			continue;
		}

		FProperty* SurfaceProp = nullptr;
		FProperty* MoveCostProp = nullptr;
		if (!IsBrushLike(Class, SurfaceProp, MoveCostProp))
		{
			continue; // readout o tool senza pennello: non deve auto-aggiornare nulla
		}

		++Esaminati;
		const FString Nome = Class->GetName();

		UObject* Props = NewObject<UObject>(GetTransientPackage(), Class);
		if (!TestNotNull(*FString::Printf(TEXT("%s: istanza creata"), *Nome), Props))
		{
			continue;
		}

		// Si scrive la superficie e si notifica come farebbe il pannello dei dettagli. Il costo di partenza
		// e' forzato a un valore SBAGLIATO e diverso dal default, cosi' un property set che non fa nulla
		// non puo' passare per coincidenza.
		MoveCostProp->ContainerPtrToValuePtr<int32>(Props)[0] = 99;
		SurfaceProp->ContainerPtrToValuePtr<uint8>(Props)[0] = static_cast<uint8>(ProbeSurface);

		FPropertyChangedEvent Evento(SurfaceProp);
		Props->PostEditChangeProperty(Evento);

		const int32 Ottenuto = MoveCostProp->ContainerPtrToValuePtr<int32>(Props)[0];
		TestEqual(*FString::Printf(
			TEXT("%s: cambiando Surface a Rough, MoveCost segue il catalogo"), *Nome),
			Ottenuto, Def.MoveCost);
	}

	// Se il filtro smettesse di riconoscere i pennelli, il ciclo girerebbe a vuoto e il test passerebbe
	// senza aver verificato niente: e' il modo in cui un test smette di dire qualcosa in silenzio.
	TestTrue(TEXT("almeno due pennelli esaminati (Paint e Fill)"), Esaminati >= 2);
	return true;
}

/**
 * I readout NON si auto-aggiornano, ed e' il rovescio della stessa invariante.
 *
 * Senza questo, la correzione di #871 potrebbe essere applicata «per sicurezza» anche a
 * `URTHexSelectToolProperties`, dove sovrascriverebbe il dato letto dalla cella selezionata con un valore
 * di catalogo — un readout che mente invece di uno che tace.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexReadoutDoesNotAutoUpdateTest,
	"RefactorTactics.HexEditor.ReadoutDoesNotAutoUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexReadoutDoesNotAutoUpdateTest::RunTest(const FString&)
{
	UClass* Select = FindObject<UClass>(nullptr, TEXT("/Script/RefactorTacticsEditor.RTHexSelectToolProperties"));
	if (!TestNotNull(TEXT("URTHexSelectToolProperties e' riflessa"), Select))
	{
		return false;
	}

	FProperty* SurfaceProp = Select->FindPropertyByName(TEXT("Surface"));
	FProperty* MoveCostProp = Select->FindPropertyByName(TEXT("MoveCost"));
	if (!TestNotNull(TEXT("Surface esiste"), SurfaceProp) || !TestNotNull(TEXT("MoveCost esiste"), MoveCostProp))
	{
		return false;
	}

	// E' questa la riga che rende il readout un readout, e il test la pinna: se qualcuno le rendesse
	// editabili, il tool di selezione diventerebbe un pennello per sbaglio.
	//
	// ⚠️ Si asserisce `CPF_EditConst`, non l'assenza di `CPF_Edit`: `VisibleAnywhere` imposta ENTRAMBI, e
	// la prima stesura di questo test chiedeva che `CPF_Edit` fosse falso — fallendo su un codice corretto.
	TestTrue(TEXT("Surface del readout e' esposta nei dettagli"), SurfaceProp->HasAnyPropertyFlags(CPF_Edit));
	TestTrue(TEXT("Surface del readout e' di sola lettura"), SurfaceProp->HasAnyPropertyFlags(CPF_EditConst));
	TestTrue(TEXT("MoveCost del readout e' di sola lettura"), MoveCostProp->HasAnyPropertyFlags(CPF_EditConst));
	TestFalse(TEXT("il readout non e' un pennello"), EditabileDavvero(SurfaceProp));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
