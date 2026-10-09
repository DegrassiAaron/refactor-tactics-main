#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "Unit/RTUnit.h"

// La guardia: senza, questi test finiscono nel binario Shipping. Vedi `#923`.
#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/**
	 * I quattro eroi della v0.1. I path si scrivono qui e non si deducono: `RTGameMode` li risolve con
	 * `FClassFinder`, che vive in un costruttore e non e' interrogabile da un test.
	 */
	const TCHAR* const PathEroi[] = {
		TEXT("/Game/RT/Characters/Aevik/Blueprints/BP_Unit_Aevik.BP_Unit_Aevik_C"),
		TEXT("/Game/RT/Characters/Branth/Blueprints/BP_Unit_Branth.BP_Unit_Branth_C"),
		TEXT("/Game/RT/Characters/Ivrin/Blueprints/BP_Unit_Ivrin.BP_Unit_Ivrin_C"),
		TEXT("/Game/RT/Characters/Muiren/Blueprints/BP_Unit_Muiren.BP_Unit_Muiren_C"),
	};

	/**
	 * Le proprieta' in virgola mobile AMMESSE sulla superficie Blueprint, ciascuna con la sua ragione.
	 *
	 * 🔑 **E' la stessa disciplina di `RTG7Perimetro::EccezioniVirgolaMobile` per i tipi nativi**, e non
	 * una regola nuova: un float non e' vietato, e' **da dichiarare**. Cosi' il presidio non diventa un
	 * divieto che chi aggiunge una variabile di presentazione aggira spegnendo il test.
	 *
	 * ⚠️ **Oggi e' VUOTO perche' la misura e' zero**, non perche' nessuno l'abbia guardato: il conteggio
	 * sta nel log di questo test, col controllo positivo accanto.
	 */
	const TCHAR* const EccezioniVirgolaMobile[] = { nullptr };

	/** Srotola i contenitori fino alle foglie: un `TArray<float>` nasconde un float. */
	void Foglie(FProperty* Prop, TArray<FProperty*>& Out)
	{
		if (Prop == nullptr)
		{
			return;
		}
		if (FArrayProperty* AsArray = CastField<FArrayProperty>(Prop))
		{
			Foglie(AsArray->Inner, Out);
			return;
		}
		if (FSetProperty* AsSet = CastField<FSetProperty>(Prop))
		{
			Foglie(AsSet->ElementProp, Out);
			return;
		}
		if (FMapProperty* AsMap = CastField<FMapProperty>(Prop))
		{
			Foglie(AsMap->KeyProp, Out);
			Foglie(AsMap->ValueProp, Out);
			return;
		}
		Out.Add(Prop);
	}
}

/**
 * LA SUPERFICIE BLUEPRINT DEL PERIMETRO DI `G7`, MISURATA INVECE CHE ASSUNTA.
 *
 * 🔴 **La cella `G7` del DoD dichiara che questa superficie e' «misurata vuota» e non dice COME**, e la
 * misura non e' riproducibile dall'esterno: un `grep` di `FloatProperty` sul `.uasset` risponde `1` sia su
 * un `BP_Unit_*` sia su una UMG qualunque, perche' conta la presenza della stringa nella name table e non
 * le proprieta'. Lo stesso valore per due ipotesi opposte non e' una misura.
 *
 * 🔑 **Il buco che questo test chiude ha UNA riga.** I tre validator di `G7` filtrano i tipi con
 * `GetPathName().StartsWith(TEXT("/Script/RefactorTactics"))`; una classe generata da Blueprint ha path
 * `/Game/...`, quindi e' esclusa **per costruzione**. E `Catalog.PlacedUnitGameplayNumbersAreIntegers` gira
 * su `ARTUnit::StaticClass()`, cioe' sul tipo **nativo**: una sottoclasse Blueprint non viene ispezionata
 * da nessuno dei tre.
 *
 * ⚠️ **Si guardano solo le proprieta' che la classe Blueprint AGGIUNGE**, non quelle ereditate: le
 * ereditate sono il tipo nativo, che i tre validator coprono gia'. Includerle raddoppierebbe la copertura e
 * farebbe fallire questo test per difetti che appartengono a un'altra riga.
 *
 * ⛔ **Non asserisce «zero float» e la ragione e' che il DoD non lo chiede.** `G7` vuole *«nessun float in
 * costi, priorita', danni»*: una variabile Blueprint che non entra nella simulazione non e' un difetto —
 * l'autorita' e' C++ ([`CLAUDE.md`] §5). Cio' che questo test garantisce e' che la superficie sia
 * **enumerata e visibile**, e stampa cio' che trova perche' la decisione su `G7` poggi su un dato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTG7BlueprintSurfaceTest,
	"RefactorTactics.Catalog.BlueprintSurfaceOfTheG7PerimeterIsEnumerated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTG7BlueprintSurfaceTest::RunTest(const FString& /*Parameters*/)
{
	int32 ClassiCaricate = 0;
	int32 TotaleAggiunte = 0;
	int32 TotaleVirgolaMobile = 0;

	for (const TCHAR* Path : PathEroi)
	{
		UClass* Generata = LoadClass<ARTUnit>(nullptr, Path);
		if (!TestNotNull(*FString::Printf(TEXT("la classe Blueprint si carica: %s"), Path), Generata))
		{
			continue;
		}
		++ClassiCaricate;

		// ⚠️ `EFieldIterationFlags::None` esclude le ereditate: e' precisamente la distinzione che serve.
		int32 Aggiunte = 0;
		TArray<FString> VirgolaMobile;
		for (TFieldIterator<FProperty> It(Generata, EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			TArray<FProperty*> Leaves;
			Foglie(*It, Leaves);
			for (FProperty* Leaf : Leaves)
			{
				++Aggiunte;
				if (Leaf->IsA<FFloatProperty>() || Leaf->IsA<FDoubleProperty>())
				{
					VirgolaMobile.Add(FString::Printf(TEXT("%s::%s"),
						*Generata->GetName(), *It->GetName()));
				}
			}
		}
		TotaleAggiunte += Aggiunte;
		TotaleVirgolaMobile += VirgolaMobile.Num();

		UE_LOG(LogTemp, Display,
			TEXT("[G7] %s: %d proprieta' AGGIUNTE dalla Blueprint, di cui %d in virgola mobile%s%s"),
			*Generata->GetName(), Aggiunte, VirgolaMobile.Num(),
			VirgolaMobile.Num() > 0 ? TEXT(" -> ") : TEXT(""),
			*FString::Join(VirgolaMobile, TEXT(", ")));
	}

	// 🔑 **Il controllo positivo del metodo, e senza di esso questo test e' verde per assenza.** Se
	// `ExcludeSuper` o l'iteratore fossero sbagliati, l'enumerazione risponderebbe zero su tutto e il test
	// passerebbe dicendo «superficie vuota» quando in realta' non ha guardato. Il tipo NATIVO ha proprieta'
	// proprie per costruzione: se il conteggio su di esso e' zero, lo strumento e' rotto.
	// `{ nullptr }` e' un elenco vuoto: un array C++ di dimensione zero non e' legale, quindi la
	// sentinella c'e' e non si conta.
	int32 NumEccezioni = 0;
	for (const TCHAR* E : EccezioniVirgolaMobile)
	{
		if (E != nullptr) { ++NumEccezioni; }
	}

	int32 ProprieteNative = 0;
	for (TFieldIterator<FProperty> It(ARTUnit::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		++ProprieteNative;
	}
	TestTrue(*FString::Printf(
		TEXT("controllo positivo: l'enumerazione vede le proprieta' proprie di ARTUnit (%d)"), ProprieteNative),
		ProprieteNative > 0);

	TestEqual(TEXT("tutte e quattro le classi d'eroe si caricano"), ClassiCaricate, (int32)UE_ARRAY_COUNT(PathEroi));

	// 🔴 **Il presidio: ogni float sulla superficie Blueprint va DICHIARATO.** Senza questa riga il test
	// enumera e non presidia — stamperebbe un float nuovo nel log e passerebbe comunque, cioe' sarebbe
	// esattamente il *«misurata vuota, ma non presidiata»* che la cella `G7` dichiarava.
	TestEqual(*FString::Printf(
		TEXT("ogni proprieta' in virgola mobile aggiunta da una Blueprint e' dichiarata fra le eccezioni "
			 TEXT("(trovate %d, dichiarate %d)")),
		TotaleVirgolaMobile, NumEccezioni), TotaleVirgolaMobile, NumEccezioni);

	UE_LOG(LogTemp, Display,
		TEXT("[G7] SUPERFICIE BLUEPRINT: %d classi, %d proprieta' aggiunte in totale, %d in virgola mobile. "
			 "Controllo positivo su ARTUnit nativo: %d proprieta' proprie."),
		ClassiCaricate, TotaleAggiunte, TotaleVirgolaMobile, ProprieteNative);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
