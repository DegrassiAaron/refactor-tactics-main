#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Map/RTHexLibrary.h"
#include "RTPlaygroundLayout.h"
#include "Camera/RTCameraPawn.h"
#include "RTPlaygroundPanelLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_CallFunction.h"
#include "World/RTGrayboxUnitFacingFixture.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * **Il modello del Playground Panel** (#1993, Epic #1990, `D-304`): sette test, uno per voce `D007`.
 *
 * 🔑 **Perche' il modello esiste, e perche' i test stanno qui e non nel grafo.** Un `EditorUtilityWidget`
 * e' un Blueprint: non si diffa, non si esercita headless, e cio' che vive dentro il suo grafo non ha
 * oracolo. La sezione Automation della issue chiede tre cose provabili senza viewport — le otto station,
 * le sei direzioni, il verdetto sulla mappa — e possono esserlo solo se vivono in C++.
 *
 * ⚠️ **Cio' che questi test NON provano**: che il widget le chiami. Quel legame resta authoring, ed e'
 * dichiarato nella `D010` invece di essere lasciato credere qui.
 */

namespace
{
	/** Stesso idioma dei test runtime: un mondo di prova, creato e distrutto dal test. */
	UWorld* MakePanelWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyPanelWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}
}

/**
 * D007.1 — le station del pannello sono **quelle della planimetria**, non una seconda copia.
 *
 * ⚠️ Il confronto e' voce per voce, non sul conteggio: otto contro otto passerebbe anche con otto
 * rettangoli sbagliati. E' la differenza fra contare e verificare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelStationsTest,
	"RefactorTactics.Playground.PanelStationsMatchTheLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelStationsTest::RunTest(const FString&)
{
	const TArray<RTPlayground::FStation> Source = RTPlayground::Stations();
	const TArray<FRTPlaygroundStationInfo> View = URTPlaygroundPanelLibrary::GetStations();

	if (!TestEqual(TEXT("il pannello vede tutte le station della planimetria"), View.Num(), Source.Num()))
	{
		return false;
	}

	for (int32 I = 0; I < Source.Num(); ++I)
	{
		TestEqual(TEXT("numero"), View[I].Number, Source[I].Number);
		TestEqual(TEXT("nome"),   View[I].Name,   FString(Source[I].Name));
		TestEqual(TEXT("vivo"),   View[I].bLive,  Source[I].bLive);

		// I bounds sono gli stessi, convertiti UNA volta: se qualcuno riscrivesse la conversione nel grafo
		// col fattore sbagliato, la camera andrebbe a un centesimo o a cento volte la distanza giusta.
		TestTrue(*FString::Printf(TEXT("station %d: bounds in unita'"), Source[I].Number),
			FMath::IsNearlyEqual(View[I].MinWorld.X, RTPlayground::WorldFromMetres(Source[I].Bounds.Min.X), 0.01)
			&& FMath::IsNearlyEqual(View[I].MaxWorld.Y, RTPlayground::WorldFromMetres(Source[I].Bounds.Max.Y), 0.01));
	}

	// ⛔ Fuori da `1..8` non si ottiene una station vuota che sembra valida: un pad a `(0,0)` di lato zero
	// manderebbe `Focus` da qualche parte senza che niente segnali l'errore.
	FRTPlaygroundStationInfo Missing;
	TestFalse(TEXT("la station 0 non esiste"), URTPlaygroundPanelLibrary::FindStation(0, Missing));
	TestFalse(TEXT("la station 9 non esiste"), URTPlaygroundPanelLibrary::FindStation(9, Missing));

	// E la 01 e' l'unica viva in `GKP 0.1`.
	FRTPlaygroundStationInfo One;
	if (TestTrue(TEXT("la station 1 esiste"), URTPlaygroundPanelLibrary::FindStation(1, One)))
	{
		TestTrue(TEXT("la station 1 e' LIVE"), One.bLive);
	}
	return true;
}

/**
 * D007.2 — le sei voci del dropdown vengono **dall'enum**.
 *
 * ⚠️ Un elenco inciso a mano e' il difetto di `#1459`: *«tre posti che elencavano le fixture e nessuno dei
 * tre coincideva col codice»*. Il test asserisce **sei**, cosi' una lista che ne dimenticasse una — o che
 * includesse il `_MAX` che UHT aggiunge — diventerebbe rossa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelFacingOptionsTest,
	"RefactorTactics.Playground.PanelFacingOptionsComeFromTheEnum",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelFacingOptionsTest::RunTest(const FString&)
{
	const TArray<FString> Options = URTPlaygroundPanelLibrary::GetFacingOptions();
	TestEqual(TEXT("sei voci, non cinque e non sette"), Options.Num(), 6);

	// Ogni voce si ri-traduce nella direzione giusta: il dropdown e' un round-trip, non due elenchi.
	for (int32 I = 0; I < Options.Num(); ++I)
	{
		ERTHexDirection Parsed = ERTHexDirection::E;
		if (TestTrue(*FString::Printf(TEXT("'%s' si traduce"), *Options[I]),
			URTPlaygroundPanelLibrary::ParseFacingOption(Options[I], Parsed)))
		{
			TestEqual(*FString::Printf(TEXT("'%s' e' la direzione %d"), *Options[I], I),
				static_cast<int32>(Parsed), I);
		}
	}

	// ⛔ Una stringa fuori set non diventa `E` per ripiego: sarebbe un dato inventato che il pannello
	// mostrerebbe come una scelta dell'utente.
	ERTHexDirection Ignored = ERTHexDirection::E;
	TestFalse(TEXT("una voce inventata non si traduce"),
		URTPlaygroundPanelLibrary::ParseFacingOption(TEXT("NORTH"), Ignored));
	TestFalse(TEXT("il _MAX non e' una direzione"),
		URTPlaygroundPanelLibrary::ParseFacingOption(TEXT("ERTHexDirection_MAX"), Ignored));
	return true;
}

/**
 * 🔴 D007.3 — **applicare il `Facing` muove davvero il marker.**
 *
 * ⚠️ **E' il test che chiude la trappola per cui la issue esiste.** Il *Why* di #1993 racconta che
 * scrivere una property senza forzare il ridisegno lasciava guardare la geometria vecchia. Il fixture
 * posiziona il marker in `OnConstruction`, e `RerunConstructionScripts` **non e' una `UFUNCTION`**: un
 * Blueprint puo' scrivere i cinque parametri ma non puo' ricostruire. Qui si verifica che il modello lo
 * faccia — spawnando l'attore e confrontando col valore della libreria, non fidandosi del ritorno `true`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelApplyFacingTest,
	"RefactorTactics.Playground.PanelApplyFacingMovesTheMarker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelApplyFacingTest::RunTest(const FString&)
{
	UWorld* World = MakePanelWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTGrayboxUnitFacingFixture* Fixture =
		World->SpawnActor<ARTGrayboxUnitFacingFixture>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("il fixture si posa"), Fixture)
		|| !TestNotNull(TEXT("ha il marker"), Fixture->FacingMarker.Get()))
	{
		DestroyPanelWorld(World);
		return false;
	}

	for (int32 D = 0; D < 6; ++D)
	{
		const ERTHexDirection Dir = static_cast<ERTHexDirection>(D);
		TestTrue(*FString::Printf(TEXT("direzione %d: applicata"), D),
			URTPlaygroundPanelLibrary::ApplyFixtureFacing(Fixture, Dir));

		const FVector  Origin   = URTHexLibrary::FacingMarkerOrigin(Dir, FVector::ZeroVector,
			Fixture->BodyRadius, Fixture->FaceHeight);
		const FRotator Rotation = URTHexLibrary::FacingRotation(Dir);
		const FVector  Expected = Origin + Rotation.Vector() * (static_cast<double>(Fixture->MarkerLength) * 0.5);

		TestTrue(*FString::Printf(TEXT("direzione %d: il MARKER si e' mosso, non solo il dato"), D),
			Fixture->FacingMarker->GetRelativeLocation().Equals(Expected, 0.01));
	}

	// ⛔ Attore nullo: rifiuto, non crash. Un pannello puo' avere una selezione svanita sotto le dita.
	TestFalse(TEXT("un attore nullo viene rifiutato"),
		URTPlaygroundPanelLibrary::ApplyFixtureFacing(nullptr, ERTHexDirection::E));

	DestroyPanelWorld(World);
	return true;
}

/**
 * D007.4 — le tre righe di `DIAGNOSTICS`, **alla lettera**.
 *
 * Non sono decorazione: dichiarano cio' che il pannello non e' autorizzato a fare. Un refuso — `NONE.` con
 * un punto, o `MINIMAL` al posto di `NONE` — cambierebbe quella dichiarazione senza che nulla suoni.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelDiagnosticsTest,
	"RefactorTactics.Playground.PanelDiagnosticsLinesAreExact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelDiagnosticsTest::RunTest(const FString&)
{
	const TArray<FString> Lines = URTPlaygroundPanelLibrary::DiagnosticsLines();
	if (!TestEqual(TEXT("le righe sono tre"), Lines.Num(), 3))
	{
		return false;
	}
	TestEqual(TEXT("riga 1"), Lines[0], FString(TEXT("Mode: PRESENTATION ONLY")));
	TestEqual(TEXT("riga 2"), Lines[1], FString(TEXT("Gameplay authority: NONE")));
	TestEqual(TEXT("riga 3"), Lines[2], FString(TEXT("Runtime state mutation: NONE")));
	return true;
}

/**
 * D007.5 — i tre preset di camera sono **numeri dichiarati**, e ordinati.
 *
 * ⛔ **Il legame con `U25` e' una citazione, non un controllo**, ed e' dichiarato: quei valori vivono in
 * prosa e nessun test puo' leggerli da li'. Cio' che si verifica e' l'invariante vera — tre preset,
 * strettamente crescenti — perche' `Close` piu' lontano di `Overview` sarebbe un difetto che il conteggio
 * non vedrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelCameraPresetsTest,
	"RefactorTactics.Playground.PanelCameraPresetsAreDeclared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelCameraPresetsTest::RunTest(const FString&)
{
	const TArray<float> Arms = URTPlaygroundPanelLibrary::CameraPresetArmLengths();
	if (!TestEqual(TEXT("i preset sono tre: Close, Tactical, Overview"), Arms.Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("Close e' piu' vicino di Tactical"),  Arms[0] < Arms[1]);
	TestTrue(TEXT("Tactical e' piu' vicino di Overview"), Arms[1] < Arms[2]);
	TestTrue(TEXT("nessun preset e' a distanza zero o negativa"), Arms[0] > 0.f);

	// 🔑 **Da citazione a CONTROLLO.** La nota di `U41` diceva che il legame con `U25` era «una citazione,
	// non un controllo»: i tre numeri vivevano in prosa e nessun test poteva leggerli da li'. Ora i preset
	// **vengono** dal CDO della camera del gioco, e questo confronto li lega. Se domani la camera cambia
	// le sue distanze, il pannello le segue invece di mostrarne tre ferme.
	const ARTCameraPawn* Camera = GetDefault<ARTCameraPawn>();
	if (TestNotNull(TEXT("il CDO della camera esiste"), Camera))
	{
		TestEqual(TEXT("Close e' la distanza minima della camera"),      Arms[0], Camera->GetMinArmLength());
		TestEqual(TEXT("Tactical e' la distanza d'inizio partita"),      Arms[1], Camera->GetMatchStartArmLength());
		TestEqual(TEXT("Overview e' la distanza massima della camera"),  Arms[2], Camera->GetMaxArmLength());
	}
	return true;
}

/**
 * D007.6 — `Reset Fixture` rimette i **default della classe**, non dei letterali.
 *
 * ⚠️ Incidere `60 / 180 / 120 / 70` nel pannello significherebbe che il giorno in cui il fixture cambia
 * default il `Reset` riporta a valori che non lo sono piu' — e nessuno se ne accorge, perche' il pannello
 * resta coerente con se stesso. Il test confronta col **CDO**, quindi segue il fixture.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelResetTest,
	"RefactorTactics.Playground.PanelResetRestoresClassDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelResetTest::RunTest(const FString&)
{
	UWorld* World = MakePanelWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTGrayboxUnitFacingFixture* Fixture =
		World->SpawnActor<ARTGrayboxUnitFacingFixture>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("il fixture si posa"), Fixture))
	{
		DestroyPanelWorld(World);
		return false;
	}

	// Si sporca TUTTO, altrimenti il reset potrebbe non fare niente e il test passerebbe lo stesso.
	URTPlaygroundPanelLibrary::ApplyFixtureParameters(Fixture, 11.f, 22.f, 33.f, 44.f);
	URTPlaygroundPanelLibrary::ApplyFixtureFacing(Fixture, ERTHexDirection::SW);

	TestTrue(TEXT("il reset accetta"), URTPlaygroundPanelLibrary::ResetFixture(Fixture));

	const ARTGrayboxUnitFacingFixture* Defaults = GetDefault<ARTGrayboxUnitFacingFixture>();
	TestEqual(TEXT("BodyRadius torna al default della CLASSE"),   Fixture->BodyRadius,   Defaults->BodyRadius);
	TestEqual(TEXT("BodyHeight torna al default"),                Fixture->BodyHeight,   Defaults->BodyHeight);
	TestEqual(TEXT("FaceHeight torna al default"),                Fixture->FaceHeight,   Defaults->FaceHeight);
	TestEqual(TEXT("MarkerLength torna al default"),              Fixture->MarkerLength, Defaults->MarkerLength);
	TestTrue(TEXT("anche il Facing torna al default"),            Fixture->Facing == Defaults->Facing);

	// ⚠️ E il marker segue: un reset che scrivesse i valori senza ricostruire lascerebbe a schermo la
	// posa sporca — la stessa trappola di `ApplyFixtureFacing`.
	if (Fixture->FacingMarker)
	{
		const FVector Expected = URTHexLibrary::FacingMarkerOrigin(Defaults->Facing, FVector::ZeroVector,
			Defaults->BodyRadius, Defaults->FaceHeight)
			+ URTHexLibrary::FacingRotation(Defaults->Facing).Vector()
			  * (static_cast<double>(Defaults->MarkerLength) * 0.5);
		TestTrue(TEXT("il marker e' tornato dov'era"),
			Fixture->FacingMarker->GetRelativeLocation().Equals(Expected, 0.01));
	}

	DestroyPanelWorld(World);
	return true;
}

/**
 * D007.7 — `Ready` sulla mappa giusta, `Error` **con una ragione** su qualunque altra.
 *
 * ⚠️ La DoD lo chiede per esteso: *«`Error` con una ragione leggibile, e non un pannello vuoto che sembra
 * rotto»*. Un pannello muto e un pannello guasto hanno lo stesso aspetto, e il secondo manda a cercare un
 * difetto che non c'e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelMapStateTest,
	"RefactorTactics.Playground.PanelMapStateNamesTheReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelMapStateTest::RunTest(const FString&)
{
	// Il nome nudo e il percorso lungo sono la stessa mappa: distinguerli non e' una decisione del pannello.
	for (const TCHAR* Name : {
		TEXT("L_GrayKitPlayground"),
		TEXT("/Game/RT/Maps/Dev/L_GrayKitPlayground/L_GrayKitPlayground"),
		TEXT("/Game/RT/Maps/Dev/L_GrayKitPlayground/L_GrayKitPlayground.L_GrayKitPlayground") })
	{
		const FRTPlaygroundMapState State = URTPlaygroundPanelLibrary::EvaluateMapState(Name);
		TestTrue(*FString::Printf(TEXT("'%s' e' Ready"), Name), State.State == ERTPlaygroundReadiness::Ready);
		TestEqual(TEXT("il nome mostrato e' quello nudo"), State.MapName, FString(TEXT("L_GrayKitPlayground")));
	}

	// Un'altra mappa, e la mappa vuota: entrambe `Error`, ed entrambe con una ragione NON VUOTA.
	for (const TCHAR* Name : { TEXT("L_DevSandbox"), TEXT("") })
	{
		const FRTPlaygroundMapState State = URTPlaygroundPanelLibrary::EvaluateMapState(Name);
		TestTrue(*FString::Printf(TEXT("'%s' e' Error"), Name), State.State == ERTPlaygroundReadiness::Error);
		TestFalse(*FString::Printf(TEXT("'%s': la ragione non e' vuota"), Name), State.Reason.IsEmpty());
	}

	// ⚠️ E la ragione NOMINA la mappa aperta: «errore» da solo manda a cercare un guasto dove c'e'
	// soltanto la mappa sbagliata.
	const FRTPlaygroundMapState Wrong = URTPlaygroundPanelLibrary::EvaluateMapState(TEXT("L_DevSandbox"));
	TestTrue(TEXT("la ragione nomina la mappa aperta"), Wrong.Reason.Contains(TEXT("L_DevSandbox")));
	TestTrue(TEXT("la ragione nomina quella attesa"),   Wrong.Reason.Contains(TEXT("L_GrayKitPlayground")));
	return true;
}

namespace
{
	/**
	 * Le voci **come sono salvate nel `.uasset`**, lette per riflessione.
	 *
	 * 🔑 **Perche' non `GetOptionCount()`/`GetOptionAtIndex()`**: quelle leggono `Options`, un
	 * `TArray<TSharedPtr<FString>>` **transiente** che `PostLoad` ricostruisce da `DefaultOptions`.
	 * Interrogare il derivato invece del persistito e' esattamente come il commandlet si era ingannato
	 * da solo — scriveva `Options`, rileggeva `Options`, e dichiarava «8 voci» su un asset vuoto.
	 * Qui si guarda il campo che finisce su disco.
	 */
	TArray<FString> PersistedComboOptions(const UComboBoxString* Combo)
	{
		TArray<FString> Out;
		static const FName DefaultOptionsName(TEXT("DefaultOptions"));
		const FArrayProperty* ArrayProp =
			FindFProperty<FArrayProperty>(UComboBoxString::StaticClass(), DefaultOptionsName);
		const FStrProperty* InnerProp = ArrayProp ? CastField<FStrProperty>(ArrayProp->Inner) : nullptr;
		if (!Combo || !ArrayProp || !InnerProp)
		{
			return Out;
		}
		FScriptArrayHelper_InContainer Helper(ArrayProp, Combo);
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			Out.Add(InnerProp->GetPropertyValue(Helper.GetElementPtr(Index)));
		}
		return Out;
	}
}

/**
 * 🔑 **Il legame ASSET -> MODELLO, che prima non era verificabile da nessun test.**
 *
 * L'intestazione di questo file dichiarava: *«cio' che questi test NON provano: che il widget le
 * chiami»*. Per le due combo **ora lo prova**: le voci non vivono piu' solo nel grafo, vivono in
 * `DefaultOptions` del `.uasset`, e un `.uasset` si puo' caricare e interrogare.
 *
 * ⚠️ Il confronto e' **voce per voce**, non sul conteggio: otto station con i nomi sbagliati sono otto,
 * e un test sulla cardinalita' le accetterebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelComboOptionsTest,
	"RefactorTactics.Playground.PanelComboOptionsComeFromTheModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelComboOptionsTest::RunTest(const FString&)
{
	UEditorUtilityWidgetBlueprint* Panel = LoadObject<UEditorUtilityWidgetBlueprint>(
		nullptr, TEXT("/Game/RT/Editor/GrayKit/UI/WBP_RT_GrayKitPlayground"));
	if (!TestNotNull(TEXT("il pannello e' caricabile dal suo percorso"), Panel))
	{
		return false;
	}
	UWidgetTree* Tree = Panel->WidgetTree;
	if (!TestNotNull(TEXT("il pannello ha un widget tree"), Tree))
	{
		return false;
	}

	UComboBoxString* StationCombo = Cast<UComboBoxString>(Tree->FindWidget(TEXT("Cmb_Station")));
	UComboBoxString* FacingCombo  = Cast<UComboBoxString>(Tree->FindWidget(TEXT("Cmb_Facing")));
	if (!TestNotNull(TEXT("Cmb_Station esiste con il suo nome stabile"), StationCombo) ||
		!TestNotNull(TEXT("Cmb_Facing esiste con il suo nome stabile"), FacingCombo))
	{
		return false;
	}

	const TArray<FString> SavedStations = PersistedComboOptions(StationCombo);
	const TArray<FRTPlaygroundStationInfo> Stations = URTPlaygroundPanelLibrary::GetStations();
	if (TestEqual(TEXT("tante voci salvate quante le station del modello"),
			SavedStations.Num(), Stations.Num()))
	{
		for (int32 Index = 0; Index < Stations.Num(); ++Index)
		{
			TestEqual(*FString::Printf(TEXT("station %d: la voce salvata e' quella del modello"), Index),
				SavedStations[Index], URTPlaygroundPanelLibrary::StationOptionLabel(Stations[Index]));
		}
	}

	const TArray<FString> SavedFacings = PersistedComboOptions(FacingCombo);
	const TArray<FString> Facings = URTPlaygroundPanelLibrary::GetFacingOptions();
	if (TestEqual(TEXT("tante voci salvate quante le direzioni del modello"),
			SavedFacings.Num(), Facings.Num()))
	{
		for (int32 Index = 0; Index < Facings.Num(); ++Index)
		{
			TestEqual(*FString::Printf(TEXT("direzione %d: la voce salvata e' quella del modello"), Index),
				SavedFacings[Index], Facings[Index]);
		}
	}

	// ⛔ Senza questo il grafo non ha maniglia: un widget non-variabile non genera il suo `Get<Nome>`.
	TestTrue(TEXT("Cmb_Facing e' una variabile, quindi il grafo la raggiunge"), FacingCombo->bIsVariable);
	const UWidget* MapState = Tree->FindWidget(TEXT("Txt_MapState"));
	if (TestNotNull(TEXT("Txt_MapState esiste"), MapState))
	{
		TestTrue(TEXT("Txt_MapState e' una variabile"), MapState->bIsVariable);
	}
	return true;
}

/**
 * 🔑 **Il pannello CHIAMA il modello — misurato, non dichiarato.**
 *
 * Il grafo vive dentro un `.uasset`, che non si diffa: il commandlet `-RefreshOptions` stampa
 * *«Grafo NON toccato»*, ma quella e' la sua parola. Questo test e' l'oracolo. Se qualcuno rigenera
 * il pannello con `-Force`, o riapplica un DSL mutilo, **qui diventa rosso**.
 *
 * ⚠️ Il confronto e' sul **nome della funzione** (`UK2Node_CallFunction::FunctionReference`), non sul
 * titolo del nodo: il titolo e' presentazione, e cambia con la lingua dell'editor.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelGraphCallsTheModelTest,
	"RefactorTactics.Playground.PanelGraphCallsTheModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelGraphCallsTheModelTest::RunTest(const FString&)
{
	UEditorUtilityWidgetBlueprint* Panel = LoadObject<UEditorUtilityWidgetBlueprint>(
		nullptr, TEXT("/Game/RT/Editor/GrayKit/UI/WBP_RT_GrayKitPlayground"));
	if (!TestNotNull(TEXT("il pannello e' caricabile dal suo percorso"), Panel))
	{
		return false;
	}

	TSet<FName> Called;
	for (const TObjectPtr<UEdGraph>& Graph : Panel->UbergraphPages)
	{
		if (!Graph)
		{
			continue;
		}
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			if (const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
			{
				Called.Add(Call->FunctionReference.GetMemberName());
			}
		}
	}

	if (!TestTrue(TEXT("l'EventGraph del pannello non e' vuoto"), Called.Num() > 0))
	{
		return false;
	}

	// Le chiamate che rendono il pannello un pannello invece di una finestra inerte.
	const FName Expected[] =
	{
		TEXT("EvaluateMapState"),        // l'HEADER dice dove sei, e perche' non ci sei
		TEXT("DiagnosticsLines"),        // le tre righe vengono dal modello, non riscritte nel widget
		TEXT("ParseFacingOption"),       // la voce della combo diventa una direzione
		TEXT("ApplyFixtureFacing"),      // 🔑 e la direzione MUOVE il marker: senza questa il pannello mente
		TEXT("ResetFixture"),            // e si torna ai default dichiarati
		TEXT("ParseStationOption"),      // la voce della combo station diventa un numero
		TEXT("FindStation"),             // il numero diventa una station, con il suo centro
		TEXT("CameraPresetArmLengths"),  // i tre bracci vengono dal modello, non scritti nel grafo
		TEXT("CameraPresetPitch"),       // e l'inclinazione e' quella del gioco, non un -90 inventato
		TEXT("SetLevelViewportCameraInfo"), // 🔑 e la camera si MUOVE davvero
	};
	for (const FName& Function : Expected)
	{
		TestTrue(*FString::Printf(TEXT("il grafo chiama %s"), *Function.ToString()), Called.Contains(Function));
	}

	// ⛔ I quattro pulsanti che inquadrano devono muovere la camera **ciascuno**: una sola chiamata
	// vorrebbe dire che tre di loro sono ancora inerti, ed e' esattamente lo stato da cui si viene.
	int32 CameraCalls = 0;
	for (const TObjectPtr<UEdGraph>& Graph : Panel->UbergraphPages)
	{
		if (!Graph) { continue; }
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
			if (Call && Call->FunctionReference.GetMemberName() == TEXT("SetLevelViewportCameraInfo"))
			{
				++CameraCalls;
			}
		}
	}
	TestEqual(TEXT("quattro pulsanti inquadrano, quindi quattro chiamate alla camera"), CameraCalls, 4);
	return true;
}

/**
 * 🔴 **Le tre righe di `DIAGNOSTICS` si vedevano DUE volte, e la prima stesura di questo test non se ne
 * accorgeva.**
 *
 * ⚠️ **La duplicazione e' una proprieta' di RUNTIME, non dell'asset.** Nel `.uasset` salvato i tre slot
 * `Txt_Diag*` portavano dei segnaposto (`Station: —`) e solo `Txt_Declared_*` portava le righe del
 * modello: contando le occorrenze nel file, ogni riga compariva **una volta**, e il test era **verde sul
 * pannello rotto**. Misurato rimettendo l'asset pre-fix, non dedotto.
 *
 * 🔑 La condizione statica che PRODUCE il doppione e' un'altra: esiste un widget che porta una riga di
 * `DiagnosticsLines()` **e non e' uno dei tre slot che il grafo riscrive**. Allora a `EventConstruct` le
 * righe finiscono in due posti. E' questo che il test asserisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelDiagnosticsOnceTest,
	"RefactorTactics.Playground.PanelDiagnosticsAppearOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelDiagnosticsOnceTest::RunTest(const FString&)
{
	UEditorUtilityWidgetBlueprint* Panel = LoadObject<UEditorUtilityWidgetBlueprint>(
		nullptr, TEXT("/Game/RT/Editor/GrayKit/UI/WBP_RT_GrayKitPlayground"));
	if (!TestNotNull(TEXT("il pannello e' caricabile"), Panel) ||
		!TestNotNull(TEXT("il pannello ha un widget tree"), Panel->WidgetTree.Get()))
	{
		return false;
	}

	// I tre slot che `EventConstruct` riscrive da `DiagnosticsLines()`. Chiunque altro porti quelle righe
	// e' una seconda copia, e a schermo si legge come un difetto.
	const TSet<FName> Slots = {
		FName(TEXT("Txt_DiagStation")), FName(TEXT("Txt_DiagBounds")), FName(TEXT("Txt_DiagActor")) };
	for (const FName& Slot : Slots)
	{
		TestNotNull(*FString::Printf(TEXT("lo slot %s esiste"), *Slot.ToString()),
			Panel->WidgetTree->FindWidget(Slot));
	}

	const TArray<FString> Lines = URTPlaygroundPanelLibrary::DiagnosticsLines();
	if (!TestTrue(TEXT("il modello dichiara almeno una riga"), Lines.Num() > 0))
	{
		return false;
	}

	TArray<FString> Intruders;
	Panel->WidgetTree->ForEachWidget([&Intruders, &Lines, &Slots](UWidget* Widget)
	{
		const UTextBlock* Block = Cast<UTextBlock>(Widget);
		if (!Block || Slots.Contains(Widget->GetFName()))
		{
			return;
		}
		const FString Text = Block->GetText().ToString();
		if (Lines.ContainsByPredicate([&Text](const FString& Line) { return Line.Equals(Text); }))
		{
			Intruders.Add(Widget->GetName());
		}
	});

	TestEqual(*FString::Printf(TEXT("nessun widget fuori dai tre slot porta una riga di DIAGNOSTICS (trovati: %s)"),
		Intruders.Num() > 0 ? *FString::Join(Intruders, TEXT(", ")) : TEXT("nessuno")),
		Intruders.Num(), 0);
	return true;
}

/**
 * 🔴 **Sei pulsanti erano barre grigie MUTE**: `UButton` senza figlio di testo non mostra niente, e il
 * verdetto di chi ha aperto il pannello e' stato *«non c'e' molto selezionabile»*. C'era — non si leggeva.
 *
 * ⚠️ Il test cerca il testo **nella discendenza**, non fra i figli diretti: un pulsante che un giorno
 * incapsulasse l'etichetta in un box resterebbe corretto, e un test troppo stretto lo chiamerebbe rotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelButtonLabelsTest,
	"RefactorTactics.Playground.PanelButtonsCarryALabel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelButtonLabelsTest::RunTest(const FString&)
{
	UEditorUtilityWidgetBlueprint* Panel = LoadObject<UEditorUtilityWidgetBlueprint>(
		nullptr, TEXT("/Game/RT/Editor/GrayKit/UI/WBP_RT_GrayKitPlayground"));
	if (!TestNotNull(TEXT("il pannello e' caricabile"), Panel) ||
		!TestNotNull(TEXT("il pannello ha un widget tree"), Panel->WidgetTree.Get()))
	{
		return false;
	}

	TArray<UButton*> Buttons;
	Panel->WidgetTree->ForEachWidget([&Buttons](UWidget* Widget)
	{
		if (UButton* Button = Cast<UButton>(Widget))
		{
			Buttons.Add(Button);
		}
	});

	// ⛔ Senza questo il ciclo sotto sarebbe vacuo: zero pulsanti = zero asserzioni = verde.
	if (!TestTrue(TEXT("il pannello ha dei pulsanti da controllare"), Buttons.Num() > 0))
	{
		return false;
	}

	for (const UButton* Button : Buttons)
	{
		bool bHasLabel = false;
		for (int32 I = 0; I < Button->GetChildrenCount(); ++I)
		{
			if (const UTextBlock* Block = Cast<UTextBlock>(Button->GetChildAt(I)))
			{
				bHasLabel |= !Block->GetText().IsEmptyOrWhitespace();
			}
		}
		TestTrue(*FString::Printf(TEXT("%s porta un'etichetta leggibile"), *Button->GetName()), bHasLabel);
	}
	return true;
}

/**
 * La gerarchia visiva esiste, e la fa la **differenza** fra le dimensioni.
 *
 * ⚠️ Il default di `UTextBlock` e' 24 per tutti: a quel punto titolo, intestazioni e corpo erano
 * indistinguibili e quattro righe riempivano il pannello. Un test sul valore assoluto invecchierebbe al
 * primo ritocco; questo asserisce la **relazione**, che e' cio' che rende leggibile una lista.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelFontHierarchyTest,
	"RefactorTactics.Playground.PanelTitleIsLargerThanBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelFontHierarchyTest::RunTest(const FString&)
{
	UEditorUtilityWidgetBlueprint* Panel = LoadObject<UEditorUtilityWidgetBlueprint>(
		nullptr, TEXT("/Game/RT/Editor/GrayKit/UI/WBP_RT_GrayKitPlayground"));
	if (!TestNotNull(TEXT("il pannello e' caricabile"), Panel) ||
		!TestNotNull(TEXT("il pannello ha un widget tree"), Panel->WidgetTree.Get()))
	{
		return false;
	}

	const UTextBlock* Title  = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("Txt_Title")));
	const UTextBlock* Header = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("Txt_StationHeader")));
	const UTextBlock* Body   = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("Txt_DiagStation")));
	if (!TestNotNull(TEXT("il titolo esiste"), Title) ||
		!TestNotNull(TEXT("un'intestazione di sezione esiste"), Header) ||
		!TestNotNull(TEXT("una riga di corpo esiste"), Body))
	{
		return false;
	}

	TestTrue(TEXT("il titolo e' piu' grande di un'intestazione"), Title->GetFont().Size > Header->GetFont().Size);
	TestTrue(TEXT("un'intestazione e' piu' grande del corpo"),    Header->GetFont().Size > Body->GetFont().Size);
	return true;
}

/**
 * 🔑 **L'etichetta e il numero fanno un giro chiuso**, su tutte le station.
 *
 * `StationOptionLabel` scrive cio' che la combo mostra; `ParseStationOption` e' la strada di ritorno che
 * il grafo percorre quando `OnSelectionChanged` gli consegna `SelectedItem`. Due funzioni che si
 * invertono sono un posto classico dove il difetto si nasconde: **testarle separatamente non basta**, e
 * il round-trip e' l'unica asserzione che le lega.
 *
 * ⛔ E il caso negativo non e' cerimonia: una stringa che non e' una station deve dare `false` **e**
 * lasciare `OutNumber` a zero. Un parser che restituisse l'ultimo valore letto porterebbe la camera su
 * una station a caso, e nessuno saprebbe perche'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelStationRoundTripTest,
	"RefactorTactics.Playground.PanelStationOptionRoundTrips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelStationRoundTripTest::RunTest(const FString&)
{
	const TArray<FRTPlaygroundStationInfo> Stations = URTPlaygroundPanelLibrary::GetStations();
	if (!TestTrue(TEXT("il modello dichiara delle station"), Stations.Num() > 0))
	{
		return false;
	}

	for (const FRTPlaygroundStationInfo& Station : Stations)
	{
		const FString Label = URTPlaygroundPanelLibrary::StationOptionLabel(Station);
		int32 Parsed = -1;
		const bool bOk = URTPlaygroundPanelLibrary::ParseStationOption(Label, Parsed);
		TestTrue(*FString::Printf(TEXT("station %d: l'etichetta si rilegge"), Station.Number), bOk);
		TestEqual(*FString::Printf(TEXT("station %d: e il numero e' lo stesso"), Station.Number),
			Parsed, Station.Number);
	}

	int32 Nonsense = 7;
	TestFalse(TEXT("una stringa che non e' una station non passa"),
		URTPlaygroundPanelLibrary::ParseStationOption(TEXT("99  Non esiste"), Nonsense));
	TestEqual(TEXT("e non lascia dietro un numero vecchio"), Nonsense, 0);

	int32 Empty = 7;
	TestFalse(TEXT("la stringa vuota non passa"), URTPlaygroundPanelLibrary::ParseStationOption(FString(), Empty));
	return true;
}

/**
 * 🔴 **Il pannello inquadrava a picco, e nessuno se ne accorgeva finche' non l'ha premuto qualcuno.**
 *
 * Il grafo usava `-90`: un numero che non veniva da nessuna parte, **fuori dal clamp `[-89, 0]`** che
 * `ARTCameraPawn::AddPitch` impone, e soprattutto non la vista del gioco. Chi premeva un preset vedeva i
 * tetti. Il pitch ora e' quello della camera tattica, e questo test lo lega.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelCameraPitchTest,
	"RefactorTactics.Playground.PanelCameraPitchIsTheGamePitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelCameraPitchTest::RunTest(const FString&)
{
	const ARTCameraPawn* Camera = GetDefault<ARTCameraPawn>();
	if (!TestNotNull(TEXT("il CDO della camera esiste"), Camera))
	{
		return false;
	}
	const float Pitch = URTPlaygroundPanelLibrary::CameraPresetPitch();
	TestEqual(TEXT("il pannello inclina come la camera del gioco"), Pitch, Camera->GetCameraPitch());

	// ⛔ Il clamp non e' decorativo: `-90` lo violava, ed e' il valore da cui si viene.
	TestTrue(TEXT("il pitch sta dentro il clamp [-89, 0]"), Pitch >= -89.f && Pitch <= 0.f);
	TestTrue(TEXT("e guarda verso il basso, non in orizzontale"), Pitch < 0.f);
	return true;
}

/**
 * 🔑 **`bLive` trova il suo primo consumatore.** Era dichiarato nella planimetria, trasportato nella vista
 * e **letto da nessuno**: la combo elencava otto station indistinguibili, mentre il `done_when` di `U41`
 * chiede che una station `PLANNED` **non sembri funzionante**.
 *
 * ⚠️ Il test non si limita a contare i `[PLANNED]`: verifica che il marchio segua **esattamente** `bLive`
 * station per station. Un'etichetta che marcasse tutte, o nessuna, passerebbe un test sul totale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPanelPlannedMarkTest,
	"RefactorTactics.Playground.PanelMarksPlannedStations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPanelPlannedMarkTest::RunTest(const FString&)
{
	const TArray<FRTPlaygroundStationInfo> Stations = URTPlaygroundPanelLibrary::GetStations();
	if (!TestTrue(TEXT("il modello dichiara delle station"), Stations.Num() > 0))
	{
		return false;
	}

	// ⛔ Senza entrambe le specie l'asserzione sotto sarebbe vacua: otto station tutte vive renderebbero
	// il marchio non verificabile, e il test verde su un'etichetta che non marca mai.
	int32 Live = 0;
	for (const FRTPlaygroundStationInfo& Station : Stations) { Live += Station.bLive ? 1 : 0; }
	TestTrue(TEXT("esistono station vive"),      Live > 0);
	TestTrue(TEXT("ed esistono station PLANNED"), Live < Stations.Num());

	for (const FRTPlaygroundStationInfo& Station : Stations)
	{
		const FString Label = URTPlaygroundPanelLibrary::StationOptionLabel(Station);
		TestEqual(*FString::Printf(TEXT("station %d: il marchio segue bLive"), Station.Number),
			Label.Contains(TEXT("[PLANNED]")), !Station.bLive);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
