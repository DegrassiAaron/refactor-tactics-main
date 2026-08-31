// La porta Blueprint dello Scenario Harness: il contratto di ADR-0010.
//
// Due domande, e sono diverse.
//
// 1. **Il draft si comporta?** Aprire, creare, validare, salvare — verificato headless su `FRTScenarioDraft`,
//    che e' C++ puro e non richiede un `UObject`.
// 2. **Il contratto e' davvero esposto?** Questa e' la domanda che un test di comportamento non fa. Una
//    `UFUNCTION` dimenticata, un `BlueprintType` non messo, e il codice compila, i test passano, e l'Editor
//    scopre il buco quando prova a costruire il widget. Qui si interroga la RIFLESSIONE: se un giorno
//    qualcuno togliesse `BlueprintType` da un DTO, questo file diventa rosso invece che silenzioso.
//
// La seconda meta' e' la ragione per cui questo file esiste separato da `RTScenarioWriterTests.cpp`.

#include "Misc/AutomationTest.h"
#include "ScenarioHarness/RTScenarioAuthoring.h"
#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioIndex.h"
#include "ScenarioHarness/RTScenarioLoader.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h" // TObjectIterator: le struct del formato si interrogano, non si elencano

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	const TCHAR* AuthoringSampleJson = TEXT(R"JSON(
	{
	  "scenarioId": "Movement.AuthoringProbe",
	  "version": 1,
	  "tags": ["movement", "authoring"],
	  "mapRadius": 3,
	  "units": [
	    { "id": "A1", "hero": "Hero.Gadget", "team": 0, "cell": [-2, 0, 0], "facing": "SW" },
	    { "id": "B1", "hero": "Hero.Riktor", "team": 1, "cell": [2, 0, 0] }
	  ],
	  "turns": [ { "intents": [ { "unit": "A1", "move": [[-1, 0, 0]] } ] } ],
	  "expect": [ { "type": "UnitAtCell", "unit": "A1", "cell": [-1, 0, 0] } ]
	}
	)JSON");

	FString AuthoringTempDir()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tests"), TEXT("ScenarioAuthoring"));
	}

	/** Scrive lo scenario di prova su disco e restituisce il percorso: serve un file per provare ad aprirlo. */
	bool WriteSampleScenario(const FString& Path, FString& OutError)
	{
		FRTTestScenario Scenario;
		if (!URTScenarioLoader::LoadFromString(AuthoringSampleJson, Scenario, OutError))
		{
			return false;
		}
		return URTScenarioLoader::SaveToFile(Scenario, Path, OutError);
	}
}

// --- il draft si comporta -------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioDraftLifecycleTest,
	"RefactorTactics.Scenario.AuthoringDraftOpensSavesAndCloses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioDraftLifecycleTest::RunTest(const FString&)
{
	const FString Dir = AuthoringTempDir();
	const FString Path = FPaths::Combine(Dir, TEXT("Sample.json"));
	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	FString Error;
	if (!TestTrue(TEXT("scenario di prova scritto su disco"), WriteSampleScenario(Path, Error)))
	{
		AddError(Error);
		return false;
	}

	FRTScenarioDraft Draft;

	// Un draft appena costruito non ha niente aperto, e le operazioni lo dicono invece di fingere.
	TestFalse(TEXT("un draft nuovo non ha scenari aperti"), Draft.IsOpen());
	TestEqual(TEXT("validare senza scenario risponde NoScenarioOpen"),
		Draft.Validate(Error), ERTScenarioAuthoringResult::NoScenarioOpen);
	TestEqual(TEXT("salvare senza scenario risponde NoScenarioOpen"),
		Draft.SaveToFile(Path, Error), ERTScenarioAuthoringResult::NoScenarioOpen);
	TestEqual(TEXT("il riassunto di un draft vuoto e' vuoto"), Draft.GetSummary().UnitCount, 0);
	TestEqual(TEXT("e non elenca unita'"), Draft.ListUnits().Num(), 0);

	// Apertura.
	if (!TestEqual(TEXT("apertura da file riuscita"),
		Draft.OpenFromFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}
	TestTrue(TEXT("ora c'e' uno scenario aperto"), Draft.IsOpen());

	const FRTScenarioSummary Summary = Draft.GetSummary();
	TestEqual(TEXT("identita' letta dal file"), Summary.ScenarioId, TEXT("Movement.AuthoringProbe"));
	TestEqual(TEXT("due unita'"), Summary.UnitCount, 2);
	TestEqual(TEXT("un turno"), Summary.TurnCount, 1);
	TestEqual(TEXT("una assertion"), Summary.ExpectationCount, 1);
	TestEqual(TEXT("i tag arrivano fino alla vista"), Summary.Tags.Num(), 2);

	// La vista sulle unita' parla il vocabolario del gioco, non stringhe e angoli.
	const TArray<FRTScenarioUnitView> Units = Draft.ListUnits();
	if (TestEqual(TEXT("due unita' elencate"), Units.Num(), 2))
	{
		TestEqual(TEXT("Stable Unit ID preservato"), Units[0].Id, TEXT("A1"));
		TestEqual(TEXT("eroe"), Units[0].HeroId, FName(TEXT("Hero.Gadget")));
		TestEqual(TEXT("la cella e' un FRTCellId"), Units[0].Cell, FRTCellId(-2, 0, 0));
		TestEqual(TEXT("il facing e' un ERTHexDirection"), Units[0].Facing, ERTHexDirection::SW);
		TestEqual(TEXT("l'ordine e' quello del file"), Units[1].Id, TEXT("B1"));
	}

	TestEqual(TEXT("lo scenario aperto e' valido"), Draft.Validate(Error), ERTScenarioAuthoringResult::Success);

	// Salvataggio altrove: l'identita' non dipende dal percorso.
	const FString Elsewhere = FPaths::Combine(Dir, TEXT("Altrove"), TEXT("NomeDiverso.json"));
	if (!TestEqual(TEXT("salvato in un altro percorso"),
		Draft.SaveToFile(Elsewhere, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	FRTScenarioDraft Reopened;
	if (TestEqual(TEXT("riaperto dall'altro percorso"),
		Reopened.OpenFromFile(Elsewhere, Error), ERTScenarioAuthoringResult::Success))
	{
		TestEqual(TEXT("stesso scenarioId nonostante il percorso diverso"),
			Reopened.GetSummary().ScenarioId, TEXT("Movement.AuthoringProbe"));
	}

	// Chiusura: torna indistinguibile da un draft nuovo, e lo dice.
	Draft.Close();
	TestFalse(TEXT("dopo Close non c'e' piu' niente di aperto"), Draft.IsOpen());
	TestEqual(TEXT("e le operazioni tornano a NoScenarioOpen"),
		Draft.Validate(Error), ERTScenarioAuthoringResult::NoScenarioOpen);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioDraftRejectsTest,
	"RefactorTactics.Scenario.AuthoringDraftDistinguishesItsFailures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioDraftRejectsTest::RunTest(const FString&)
{
	const FString Dir = AuthoringTempDir();
	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	FRTScenarioDraft Draft;
	FString Error;

	// (1) Un ID che l'indice non conosce e' `NotFound`, e non deve somigliare a «invalido».
	TestEqual(TEXT("ID inesistente -> NotFound"),
		Draft.OpenById(TEXT("Non.Esiste.Affatto"), Error), ERTScenarioAuthoringResult::NotFound);
	TestFalse(TEXT("e la frase spiega perche'"), Error.IsEmpty());
	TestFalse(TEXT("un'apertura fallita non lascia lo scenario aperto"), Draft.IsOpen());

	// (2) Idem per un percorso che non esiste.
	TestEqual(TEXT("percorso inesistente -> NotFound"),
		Draft.OpenFromFile(FPaths::Combine(Dir, TEXT("Fantasma.json")), Error),
		ERTScenarioAuthoringResult::NotFound);

	// (3) Uno scenario NUOVO non e' valido — non ha unita' ne' assertion — e questo e' corretto: rifiutarlo
	//     alla nascita renderebbe impossibile crearne uno. Ma non deve nemmeno essere salvabile.
	Draft.NewScenario(TEXT("Nuovo.Scenario"), 3);
	TestTrue(TEXT("uno scenario nuovo risulta aperto"), Draft.IsOpen());
	TestEqual(TEXT("ma non e' ancora valido"), Draft.Validate(Error), ERTScenarioAuthoringResult::Invalid);
	TestFalse(TEXT("e l'errore nomina cosa manca"), Error.IsEmpty());

	const FString Path = FPaths::Combine(Dir, TEXT("NonDeveEsistere.json"));
	TestEqual(TEXT("un invalido non si salva"),
		Draft.SaveToFile(Path, Error), ERTScenarioAuthoringResult::Invalid);
	TestFalse(TEXT("e il file non viene creato"), FPaths::FileExists(Path));

	// (4) `Invalid` accusa lo scenario, `NotFound` accusa l'indice: un salvataggio in-place di uno scenario
	//     mai stato su disco non deve travestirsi da problema di validita'.
	FRTScenarioDraft Valid;
	if (TestEqual(TEXT("scenario valido caricato in un draft"),
		[&]()
		{
			FRTTestScenario Loaded;
			FString Why;
			if (!URTScenarioLoader::LoadFromString(AuthoringSampleJson, Loaded, Why)) { return false; }
			Valid.NewScenario(TEXT("segnaposto"), 3);
			Valid.MutableScenario() = Loaded;
			return true;
		}(), true))
	{
		TestEqual(TEXT("valido, ma senza un posto dove vivere -> NotFound"),
			Valid.SaveInPlace(Error), ERTScenarioAuthoringResult::NotFound);
	}

	return true;
}

// --- il contratto e' davvero esposto --------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioAuthoringContractIsExposedTest,
	"RefactorTactics.Scenario.AuthoringContractIsReachableFromBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioAuthoringContractIsExposedTest::RunTest(const FString&)
{
	// Questo test NON verifica un comportamento: verifica che il contratto ESISTA per Blueprint. E' la
	// differenza fra «il codice funziona» e «l'Editor puo' chiamarlo», e senza di lui la seconda si scopre
	// solo aprendo Unreal.
	UClass* Facade = URTScenarioAuthoring::StaticClass();
	if (!TestNotNull(TEXT("la facade esiste come UClass"), Facade))
	{
		return false;
	}

	// Le funzioni che l'Editor deve poter chiamare per SC-2. Se una sparisce o perde la `UFUNCTION`, qui
	// cade — e cade prima che qualcuno costruisca un widget sopra un buco.
	const TArray<FName> RequiredFunctions = {
		TEXT("CreateScenarioDraft"),
		TEXT("NewScenario"),
		TEXT("OpenById"),
		TEXT("OpenFromFile"),
		TEXT("Close"),
		TEXT("IsOpen"),
		TEXT("Validate"),
		TEXT("SaveToFile"),
		TEXT("SaveInPlace"),
		TEXT("GetSummary"),
		TEXT("ListUnits"),
		TEXT("ListScenarioIds"),
		TEXT("ListScenarioTags"),
		TEXT("DescribeResult")
	};

	for (const FName& FunctionName : RequiredFunctions)
	{
		const UFunction* Function = Facade->FindFunctionByName(FunctionName);
		if (!TestNotNull(*FString::Printf(TEXT("'%s' e' una UFUNCTION"), *FunctionName.ToString()), Function))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("'%s' e' chiamabile da Blueprint"), *FunctionName.ToString()),
			Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure));
	}

	// I DTO devono essere `BlueprintType`, altrimenti non esistono come pin e le funzioni qui sopra non si
	// possono cablare a niente.
	const TArray<UScriptStruct*> RequiredStructs = {
		FRTScenarioSummary::StaticStruct(),
		FRTScenarioUnitView::StaticStruct(),
		FRTCellId::StaticStruct()
	};
	for (UScriptStruct* Struct : RequiredStructs)
	{
		if (!TestNotNull(TEXT("il DTO esiste come UScriptStruct"), Struct)) { continue; }
		// 🔴 **`GetBoolMetaData` e' API solo-metadata, e senza questa guardia `main` NON COMPILA in
		// Game Development.** Vive sotto `#if WITH_METADATA` (`UObject/Class.h:256`, `UObject/Field.h:916`),
		// che vale **1** in Editor e **0** in un target Game. La Shipping non se ne accorge perche' li'
		// `WITH_DEV_AUTOMATION_TESTS` e' 0 e questo file non esiste affatto: **Game Development e' l'unica
		// configurazione che ha i test accesi e i metadata spenti**, quindi l'unica che rompe. Misurato il
		// 2026-08-29 rieseguendo `G1`: rossa dal 2026-08-27, due giorni, sei asserzioni in tre file.
		//
		// ⚠️ E' la stessa CLASSE del difetto che `G1` aveva gia' trovato il 2026-08-24 — test scritti dopo
		// l'`#endif` della guardia, invisibili fuori dalla Shipping — ma un meccanismo diverso, quindi
		// `RefactorTactics.Meta.TestGuardClosesAtEndOfFile` non poteva vederlo: quell'oracolo controlla
		// DOVE chiude la guardia, non QUALE API si usa dentro.
		//
		// ✅ L'asserzione non si perde: la suite gira in Editor, dove `WITH_METADATA` e' 1.
#if WITH_METADATA
		TestTrue(*FString::Printf(TEXT("'%s' e' BlueprintType"), *Struct->GetName()),
			Struct->GetBoolMetaData(TEXT("BlueprintType")));
#endif
	}

	// L'enum degli esiti idem: senza `BlueprintType` la UI non puo' ramificare sull'esito.
	const UEnum* ResultEnum = StaticEnum<ERTScenarioAuthoringResult>();
	if (TestNotNull(TEXT("l'enum degli esiti esiste"), ResultEnum))
	{
		// Stessa ragione di sopra: `GetBoolMetaData` e' sotto `WITH_METADATA`, spento in un target Game.
#if WITH_METADATA
		TestTrue(TEXT("ERTScenarioAuthoringResult e' BlueprintType"),
			ResultEnum->GetBoolMetaData(TEXT("BlueprintType")));
#endif
		// Sei esiti distinti: un `bool` non basterebbe, ed e' il punto di ADR-0010 §3. Il sesto e'
		// `RunFailed`, arrivato con `#1117` per non far passare un guasto dello STRUMENTO per uno scenario
		// scritto male — la stessa separazione che `ERTTestOutcome` tiene fra `Error` e `Fail`.
		TestEqual(TEXT("sei esiti distinti"), ResultEnum->NumEnums() - 1, 6);
	}

	// ⚠️ E il verso opposto, che e' la meta' che conta davvero: il MODELLO non deve essere esposto. Se un
	// giorno qualcuno marcasse `FRTTestScenario` come `BlueprintType`, Blueprint potrebbe costruirne uno
	// membro per membro — cioe' uno scenario incoerente — e ADR-0010 cadrebbe senza che nulla diventi rosso.
	// Questo lo rende rosso.
	//
	// 🔴 **Questo blocco era un ELENCO SCRITTO A MANO, e ne mancava una** (`#1631`): `RTTestScenario.h`
	// dichiara **nove** struct e la lista ne conteneva **otto** — `FRTScenarioVariantUnit` restava fuori,
	// quindi marcarla `BlueprintType` non avrebbe reso rosso niente. Aggiungere la nona riga avrebbe chiuso
	// l'istanza e lasciato in piedi il meccanismo: la decima si sarebbe dimenticata allo stesso modo, e la
	// prossima struct del formato e' gia' prevista (l'override d'abilita' in una variante, `#1950`).
	//
	// Ora la lista **non esiste**: le struct del formato si interrogano per riflessione, come
	// `RTScreenHudWidgetTests.cpp` gia' fa con le classi widget. Una struct nuova in `RTTestScenario.h`
	// entra nel gate **da sola**, il giorno in cui viene dichiarata.
	//
	// ⚠️ Il criterio e' l'header di provenienza, non il prefisso del nome: `FRTScenarioSummary` e
	// `FRTScenarioUnitView` si chiamano quasi come il modello ma sono DTO, vivono altrove, e devono restare
	// `BlueprintType` — sono verificati venti righe piu' su, nel verso opposto. Un filtro sul nome li
	// avrebbe presi e il test si sarebbe contraddetto da solo.
#if WITH_METADATA
	// Stessa ragione di sopra per la guardia: `GetMetaData`/`GetBoolMetaData` sono API solo-metadata, e in
	// un target Game Development `WITH_METADATA` vale 0 mentre i test sono accesi.
	int32 StructDelFormato = 0;
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		UScriptStruct* Struct = *It;
		if (Struct == nullptr) { continue; }

		// `ModuleRelativePath` e' il percorso dell'header che dichiara la struct, scritto da UHT.
		if (!Struct->GetMetaData(TEXT("ModuleRelativePath")).EndsWith(TEXT("RTTestScenario.h")))
		{
			continue;
		}

		++StructDelFormato;
		TestFalse(
			*FString::Printf(
				TEXT("'%s' NON e' BlueprintType (ADR-0010: il modello non passa per Blueprint)"),
				*Struct->GetName()),
			Struct->GetBoolMetaData(TEXT("BlueprintType")));
	}

	// Anti-vacuita', e non e' un censimento: `>=` e non `==` perche' una struct nuova deve poter entrare nel
	// gate senza toccare questo file — che e' l'intero motivo per cui l'elenco a mano e' stato tolto. Se
	// invece il conteggio scendesse, vorrebbe dire che la riflessione non trova piu' l'header (metadata
	// spenti, header rinominato, struct spostate) e che il ciclo qui sopra sta girando a vuoto **restando
	// verde** — cioe' esattamente il modo in cui questo gate puo' smettere di misurare in silenzio.
	TestTrue(*FString::Printf(
		TEXT("la riflessione ha trovato le nove struct del formato dichiarate in RTTestScenario.h (trovate %d)"),
		StructDelFormato), StructDelFormato >= 9);
#endif

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioAuthoringFacadeDelegatesTest,
	"RefactorTactics.Scenario.AuthoringFacadeDelegatesToTheDraft",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioAuthoringFacadeDelegatesTest::RunTest(const FString&)
{
	const FString Dir = AuthoringTempDir();
	const FString Path = FPaths::Combine(Dir, TEXT("Facade.json"));
	ON_SCOPE_EXIT{ IFileManager::Get().DeleteDirectory(*Dir, false, true); };

	FString Error;
	if (!TestTrue(TEXT("scenario di prova scritto"), WriteSampleScenario(Path, Error)))
	{
		AddError(Error);
		return false;
	}

	URTScenarioAuthoring* Facade = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (!TestNotNull(TEXT("la factory produce un oggetto"), Facade))
	{
		return false;
	}
	TestFalse(TEXT("appena creata non ha scenari aperti"), Facade->IsOpen());

	if (!TestEqual(TEXT("la facade apre"),
		Facade->OpenFromFile(Path, Error), ERTScenarioAuthoringResult::Success))
	{
		AddError(Error);
		return false;
	}

	// La facade traduce e basta: cio' che risponde deve essere cio' che il draft dice.
	TestEqual(TEXT("il riassunto e' quello del draft"),
		Facade->GetSummary().ScenarioId, Facade->GetDraft().GetSummary().ScenarioId);
	TestEqual(TEXT("le unita' sono quelle del draft"),
		Facade->ListUnits().Num(), Facade->GetDraft().ListUnits().Num());
	TestEqual(TEXT("valida"), Facade->Validate(Error), ERTScenarioAuthoringResult::Success);

	// Due draft convivono senza che nulla sia globale: e' la ragione per cui non e' un subsystem.
	URTScenarioAuthoring* Second = URTScenarioAuthoring::CreateScenarioDraft(nullptr);
	if (TestNotNull(TEXT("un secondo draft si crea"), Second))
	{
		Second->NewScenario(TEXT("Altro.Scenario"), 5);
		TestEqual(TEXT("il secondo ha la sua identita'"),
			Second->GetSummary().ScenarioId, TEXT("Altro.Scenario"));
		TestEqual(TEXT("e il primo non e' cambiato"),
			Facade->GetSummary().ScenarioId, TEXT("Movement.AuthoringProbe"));
	}

	// Le frasi d'esito esistono per tutti e cinque i codici: una UI che ne trovasse una vuota mostrerebbe
	// un'etichetta bianca invece di dire cos'e' successo.
	const TArray<ERTScenarioAuthoringResult> AllResults = {
		ERTScenarioAuthoringResult::Success,
		ERTScenarioAuthoringResult::NotFound,
		ERTScenarioAuthoringResult::Invalid,
		ERTScenarioAuthoringResult::WriteFailed,
		ERTScenarioAuthoringResult::NoScenarioOpen
	};
	for (const ERTScenarioAuthoringResult Result : AllResults)
	{
		TestFalse(TEXT("ogni esito ha una frase leggibile"),
			URTScenarioAuthoring::DescribeResult(Result).IsEmpty());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTScenarioAuthoringOpensShippedScenariosTest,
	"RefactorTactics.Scenario.AuthoringOpensShippedScenariosById",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTScenarioAuthoringOpensShippedScenariosTest::RunTest(const FString&)
{
	// Aprire per ID e' il percorso che l'Editor usera' davvero: se l'indice e il draft non si parlano, qui
	// si vede sul corpus vero invece che su un file inventato per l'occasione.
	const TArray<FString> Ids = URTScenarioAuthoring::ListScenarioIds(FString(), FString());
	if (Ids.Num() == 0)
	{
		AddWarning(TEXT("l'indice non elenca scenari: l'apertura per ID non e' stata verificata"));
		return true;
	}

	int32 Opened = 0;
	for (const FString& Id : Ids)
	{
		FRTScenarioDraft Draft;
		FString Error;
		const ERTScenarioAuthoringResult Result = Draft.OpenById(Id, Error);
		if (Result != ERTScenarioAuthoringResult::Success)
		{
			// Uno scenario che non si carica lo copre gia' il test del loader sul corpus: qui interessa che
			// l'apertura PER ID non aggiunga un fallimento suo.
			continue;
		}

		if (Draft.GetSummary().ScenarioId != Id)
		{
			AddError(FString::Printf(
				TEXT("aperto per ID '%s' ma il draft dichiara '%s': l'indice e il file non concordano"),
				*Id, *Draft.GetSummary().ScenarioId));
		}
		if (Draft.ListUnits().Num() != Draft.GetScenario().Units.Num())
		{
			AddError(FString::Printf(TEXT("'%s': la vista sulle unita' non conta come il modello"), *Id));
		}
		++Opened;
	}

	AddInfo(FString::Printf(TEXT("aperti per ID %d scenari su %d elencati"), Opened, Ids.Num()));
	TestTrue(TEXT("almeno uno scenario del corpus si apre per ID"), Opened > 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
