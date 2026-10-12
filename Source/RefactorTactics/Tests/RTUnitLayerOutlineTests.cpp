// Il piano di un'unita' si legge dal contorno del modello (#3705, [D-508]).
//
// 🔑 **Questi test misurano la CATENA che porta lo stencil al corpo dell'unita', non cio' che si vede.** Il contorno
// lo disegna `M_UnitLayerOutline`, un post-process: che a schermo si capisca a che piano sta ogni unita' e' della
// voce PIE di #3705. Qui si misura che la relazione sia quella dei bordi delle celle, che lo stencil lo decida la
// VISTA di chi guarda, che arrivi ai soli corpi mostrati, e che la camera porti il post-process coi colori della
// regola.
//
// ⚠️ **«Lo stencil arriva» non vuol dire «il materiale lo legge»**: e' la lezione di `M_HexCell` (#3704). L'altra
// meta' sta nel modulo Editor, `UnitOutline.MaterialMatchesTheRecipe`, che guarda l'asset.

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Camera/CameraComponent.h"
#include "Camera/RTCameraPawn.h"
#include "Components/PrimitiveComponent.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTLayerPresentation.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Perception/RTKnowledgeView.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "UI/RTHUD.h"
#include "Unit/RTUnit.h"
#include "Unit/RTUnitLayerOutline.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	UPrimitiveComponent* UloComponentNamed(const AActor* Actor, const TCHAR* Name)
	{
		TArray<UPrimitiveComponent*> Comps;
		Actor->GetComponents<UPrimitiveComponent>(Comps);
		for (UPrimitiveComponent* C : Comps)
		{
			if (C && C->GetName() == Name) { return C; }
		}
		return nullptr;
	}

	/** Quanti componenti dell'attore entrano nel custom depth, e se fra loro ce n'e' uno che non e' un corpo. */
	int32 UloCustomDepthComponents(const AActor* Actor, bool& bOutSomethingElse)
	{
		bOutSomethingElse = false;
		int32 Quanti = 0;
		TArray<UPrimitiveComponent*> Comps;
		Actor->GetComponents<UPrimitiveComponent>(Comps);
		for (const UPrimitiveComponent* C : Comps)
		{
			if (C == nullptr || !C->bRenderCustomDepth) { continue; }
			++Quanti;
			const FString Nome = C->GetName();
			if (Nome != TEXT("Mesh") && Nome != TEXT("LeftArm") && Nome != TEXT("RightArm"))
			{
				bOutSomethingElse = true; // un anello, la freccia, il ricordo: il contorno e' del modello
			}
		}
		return Quanti;
	}

	FRTKnowledgeEntry UloEntry(ERTKnowledgeVisibility Visibility, int32 Layer)
	{
		FRTKnowledgeEntry Entry;
		Entry.StableUnitId = 7;
		Entry.TeamId = 1;
		Entry.Visibility = Visibility;
		Entry.Cell = FRTCellId(0, 0, Layer);
		return Entry;
	}
}

/**
 * **Lo stencil del contorno si ricava da (piano dell'unita', piano attivo, mappa a piu' piani)** — la funzione pura
 * della DoD di #3705.
 *
 * 🔑 La relazione NON e' riscritta: e' `RTLayerPresentation::Relation`, e i colori sono `BorderColorFor`. Il test lo
 * asserisce confrontando con quelle, non con numeri: se un giorno i bordi delle celle cambiassero colore, il
 * contorno delle unita' li segue senza che nessuno tocchi questo file.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOutlineStencilPureTest,
	"RefactorTactics.UnitOutline.StencilFollowsTheLayerRelation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTUnitOutlineStencilPureTest::RunTest(const FString&)
{
	using namespace RTUnitLayerOutline;

	TestEqual(TEXT("mappa a un piano: il contorno e' quello dell'attivo, per tutti (scope 4)"),
		StencilForLayer(3, 0, /*bMultiLayer=*/ false), StencilActive);
	TestEqual(TEXT("sul piano attivo"), StencilForLayer(1, 1, true), StencilActive);
	TestEqual(TEXT("un piano sopra"), StencilForLayer(2, 1, true), StencilAbove);
	TestEqual(TEXT("due piani sopra: lo stesso colore a ogni distanza"), StencilForLayer(3, 1, true), StencilAbove);
	TestEqual(TEXT("un piano sotto"), StencilForLayer(0, 1, true), StencilBelow);
	TestEqual(TEXT("due piani sotto: lo stesso"), StencilForLayer(0, 2, true), StencilBelow);

	// I tre stencil sono distinti e nessuno e' «nessun contorno»: un'unita' che si vede ha sempre il suo.
	const uint8 Tutti[] = { StencilActive, StencilAbove, StencilBelow };
	for (int32 I = 0; I < UE_ARRAY_COUNT(Tutti); ++I)
	{
		TestTrue(*FString::Printf(TEXT("lo stencil %d non e' «nessun contorno»"), Tutti[I]), Tutti[I] != StencilNone);
		for (int32 J = I + 1; J < UE_ARRAY_COUNT(Tutti); ++J)
		{
			TestTrue(*FString::Printf(TEXT("gli stencil %d e %d sono distinti"), Tutti[I], Tutti[J]), Tutti[I] != Tutti[J]);
		}
	}

	// I colori sono quelli dei bordi delle celle, dalla stessa funzione.
	const TPair<uint8, ERTLayerRelation> Coppie[] = {
		{ StencilActive, ERTLayerRelation::Active }, { StencilAbove, ERTLayerRelation::Above },
		{ StencilBelow, ERTLayerRelation::Below } };
	for (const TPair<uint8, ERTLayerRelation>& Coppia : Coppie)
	{
		FColor Colore(1, 2, 3);
		TestTrue(*FString::Printf(TEXT("lo stencil %d ha un colore"), Coppia.Key), ColorFor(Coppia.Key, Colore));
		TestTrue(*FString::Printf(TEXT("lo stencil %d ha il colore del bordo della sua relazione"), Coppia.Key),
			Colore == RTLayerPresentation::BorderColorFor(Coppia.Value));
		TestEqual(*FString::Printf(TEXT("lo stencil %d e' quello della sua relazione"), Coppia.Key),
			StencilFor(Coppia.Value), Coppia.Key);
		TestTrue(*FString::Printf(TEXT("lo stencil %d ha un parametro di colore"), Coppia.Key),
			!ColorParamFor(Coppia.Key).IsNone());
	}
	FColor Nessuno(9, 9, 9);
	TestFalse(TEXT("«nessun contorno» non ha colore"), ColorFor(StencilNone, Nessuno));
	TestFalse(TEXT("uno stencil non nostro non ha colore"), ColorFor(200, Nessuno));
	TestTrue(TEXT("e non ha parametro"), ColorParamFor(StencilNone).IsNone() && ColorParamFor(200).IsNone());
	TestTrue(TEXT("i tre parametri di colore sono distinti"),
		ParamColorActive != ParamColorAbove && ParamColorAbove != ParamColorBelow && ParamColorActive != ParamColorBelow);

	// #3705, scope 5: SOLO chi sta sotto il piano attivo si disegna attraverso la geometria.
	TestTrue(TEXT("sotto: il contorno passa attraverso la geometria"), ShowsThroughGeometry(StencilBelow));
	TestFalse(TEXT("sull'attivo: no"), ShowsThroughGeometry(StencilActive));
	TestFalse(TEXT("sopra: no"), ShowsThroughGeometry(StencilAbove));
	TestFalse(TEXT("nessun contorno: no"), ShowsThroughGeometry(StencilNone));

	// Le leve nascono accese e con lo spessore dichiarato: provvisorie, le giudica la voce PIE.
	const FRTUnitLayerOutlineTuning T;
	TestTrue(TEXT("default: il contorno e' acceso"), T.bEnabled);
	TestTrue(TEXT("default: lo spessore e' quello dichiarato, sotto il tetto"),
		FMath::IsNearlyEqual(T.ThicknessPx, DefaultThicknessPx) && DefaultThicknessPx > 0.f && DefaultThicknessPx < MaxThicknessPx);
	return true;
}

/**
 * ⛔ **Lo stencil lo decide la VISTA di chi guarda** — e' la privacy di #3705: «il contorno non deve rivelare ne' un
 * nemico ne' la sua quota».
 *
 * 🔴 La riga che porta il peso e' quella del nemico `Live`: gli si passa come `OwnLayer` un piano DIVERSO da quello
 * della sua voce. Se la funzione leggesse il piano dell'attore invece di quello della vista, il contorno sarebbe
 * dell'altro colore. Verifica di mutazione: togliere il ramo `bIsOwnTeam` e usare sempre `OwnLayer` fa cadere
 * quella riga.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOutlineObserverStencilTest,
	"RefactorTactics.UnitOutline.ObserverStencilComesFromTheView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTUnitOutlineObserverStencilTest::RunTest(const FString&)
{
	using namespace RTUnitLayerOutline;
	const int32 Attivo = 1;

	// La propria squadra: sempre, dal piano della propria cella.
	TestEqual(TEXT("propria unita' sotto il piano attivo: viola"),
		ARTHUD::UnitOutlineStencilForUnit(nullptr, /*bIsOwnTeam=*/ true, /*OwnLayer=*/ 0, Attivo, true, true), StencilBelow);
	TestEqual(TEXT("propria unita' sopra: bianco"),
		ARTHUD::UnitOutlineStencilForUnit(nullptr, true, 2, Attivo, true, true), StencilAbove);
	TestEqual(TEXT("propria unita' sull'attivo: nero"),
		ARTHUD::UnitOutlineStencilForUnit(nullptr, true, 1, Attivo, true, true), StencilActive);
	TestEqual(TEXT("mappa a un piano: nero, qualunque piano"),
		ARTHUD::UnitOutlineStencilForUnit(nullptr, true, 5, 0, /*bMultiLayer=*/ false, true), StencilActive);

	// Un nemico visto: il piano e' quello della VOCE (2, sopra), non quello passato come proprio (0, sotto).
	const FRTKnowledgeEntry Visto = UloEntry(ERTKnowledgeVisibility::Live, /*Layer=*/ 2);
	TestEqual(TEXT("nemico visto: il piano viene dalla vista, non dall'attore"),
		ARTHUD::UnitOutlineStencilForUnit(&Visto, /*bIsOwnTeam=*/ false, /*OwnLayer=*/ 0, Attivo, true, true), StencilAbove);

	// Chi non si disegna non ha contorno: ne' il ricordo, ne' chi non ha voce.
	const FRTKnowledgeEntry Ricordato = UloEntry(ERTKnowledgeVisibility::Remembered, /*Layer=*/ 2);
	TestEqual(TEXT("nemico ricordato: nessun contorno"),
		ARTHUD::UnitOutlineStencilForUnit(&Ricordato, false, 0, Attivo, true, true), StencilNone);
	TestEqual(TEXT("nemico senza voce: nessun contorno"),
		ARTHUD::UnitOutlineStencilForUnit(nullptr, false, 0, Attivo, true, true), StencilNone);

	// La leva spenta spegne tutti, anche la propria squadra.
	TestEqual(TEXT("leva spenta: nessun contorno sulla propria unita'"),
		ARTHUD::UnitOutlineStencilForUnit(nullptr, true, 0, Attivo, true, /*bEnabled=*/ false), StencilNone);
	TestEqual(TEXT("leva spenta: nessun contorno sul nemico visto"),
		ARTHUD::UnitOutlineStencilForUnit(&Visto, false, 0, Attivo, true, false), StencilNone);
	return true;
}

/**
 * **Lo stencil arriva ai soli corpi MOSTRATI** — «un'unita' nemica non vista non ha contorno» (DoD di #3705), sul
 * componente e non sul flag.
 *
 * 🔴 Lo stato (`GetLayerOutlineStencil`) e cio' che si disegna (`GetRenderedOutlineStencilForTest`) sono due cose:
 * un'unita' persa di vista CONSERVA lo stato e perde il contorno. Un test che leggesse solo lo stato passerebbe
 * anche se il corpo spento restasse nel custom depth.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOutlineBodyTest,
	"RefactorTactics.UnitOutline.BodyCarriesTheStencilOnlyWhenShown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTUnitOutlineBodyTest::RunTest(const FString&)
{
	using namespace RTUnitLayerOutline;
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	if (!TestNotNull(TEXT("l'unita'"), Unit)) { return false; }
	UPrimitiveComponent* Corpo = UloComponentNamed(Unit, TEXT("Mesh"));
	if (!TestNotNull(TEXT("il segnaposto"), Corpo)) { return false; }

	bool bAltro = false;
	TestEqual(TEXT("premessa: nasce senza contorno"), Unit->GetRenderedOutlineStencilForTest(), StencilNone);
	TestEqual(TEXT("premessa: nessun componente nel custom depth"), UloCustomDepthComponents(Unit, bAltro), 0);
	TestTrue(TEXT("premessa: l'unita' nasce nota e col segnaposto acceso"), Unit->IsKnownToObserver() && Corpo->GetVisibleFlag());

	Unit->SetLayerOutlineStencil(StencilBelow);
	TestEqual(TEXT("vista, sotto il piano attivo: il corpo porta lo stencil di «sotto»"),
		Unit->GetRenderedOutlineStencilForTest(), StencilBelow);
	TestTrue(TEXT("e il segnaposto entra nel custom depth con quel valore"),
		Corpo->bRenderCustomDepth && Corpo->CustomDepthStencilValue == StencilBelow);
	TestTrue(TEXT("solo i corpi: ne' anelli, ne' freccia, ne' il ricordo"),
		UloCustomDepthComponents(Unit, bAltro) > 0 && !bAltro);

	Unit->SetLayerOutlineStencil(StencilAbove);
	TestEqual(TEXT("cambia il piano: il corpo cambia stencil"), Unit->GetRenderedOutlineStencilForTest(), StencilAbove);

	// 🔴 Persa di vista: lo stato resta, il contorno no.
	Unit->SetKnownToObserver(false);
	TestEqual(TEXT("non vista: lo stato dichiarato resta"), Unit->GetLayerOutlineStencil(), StencilAbove);
	TestEqual(TEXT("non vista: nessun corpo porta il contorno"), Unit->GetRenderedOutlineStencilForTest(), StencilNone);
	TestFalse(TEXT("non vista: il segnaposto esce dal custom depth"), Corpo->bRenderCustomDepth);
	TestEqual(TEXT("non vista: e il suo stencil torna a zero"), static_cast<int32>(Corpo->CustomDepthStencilValue), 0);
	TestEqual(TEXT("non vista: nessun componente nel custom depth"), UloCustomDepthComponents(Unit, bAltro), 0);

	Unit->SetKnownToObserver(true);
	TestEqual(TEXT("rivista: il contorno torna, dello stato"), Unit->GetRenderedOutlineStencilForTest(), StencilAbove);

	// In strategica il segnalino e' il cilindro senza bracci: i bracci escono dal custom depth insieme alla vista.
	Unit->SetStrategicPresentation(true);
	TestEqual(TEXT("strategica: il segnalino porta lo stesso contorno (scope 3)"),
		Unit->GetRenderedOutlineStencilForTest(), StencilAbove);
	if (const UPrimitiveComponent* Braccio = UloComponentNamed(Unit, TEXT("LeftArm")))
	{
		TestFalse(TEXT("strategica: un braccio spento non resta nel custom depth"), Braccio->bRenderCustomDepth);
	}

	Unit->SetLayerOutlineStencil(StencilNone);
	TestEqual(TEXT("spento: nessun corpo nel custom depth"), UloCustomDepthComponents(Unit, bAltro), 0);
	return true;
}

/**
 * **Il contorno segue il cambio di piano attivo e il cambio di piano dell'unita'**, dal giro vero dell'HUD (DoD di
 * #3705).
 *
 * La board e' quella a tre piani di `Visual.Map.LayerStack`; il piano attivo e' quello che la board PRESENTA, lo
 * stesso che colora i bordi delle celle. Senza `ARTTurnManager` la vista di conoscenza e' vuota: l'avversario non ha
 * voce, e deve restare senza contorno mentre l'alleato lo cambia — e' il controllo che il test non sia verde
 * perche' «tutti hanno un contorno».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOutlineFollowsLayersTest,
	"RefactorTactics.UnitOutline.FollowsTheActiveLayerAndTheUnitLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTUnitOutlineFollowsLayersTest::RunTest(const FString&)
{
	using namespace RTUnitLayerOutline;
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTHexMapActor* Board = World->SpawnActorDeferred<ARTHexMapActor>(ARTHexMapActor::StaticClass(), FTransform::Identity);
	if (Board)
	{
		Board->MapAsset = URTMatchSetupLibrary::MakeLayerStackArena(World);
		Board->FinishSpawning(FTransform::Identity);
	}
	ARTHUD* Hud = World->SpawnActor<ARTHUD>();
	ARTUnit* Alleato = World->SpawnActor<ARTUnit>();
	ARTUnit* Nemico = World->SpawnActor<ARTUnit>();
	if (!TestNotNull(TEXT("la board"), Board) || !TestNotNull(TEXT("l'HUD"), Hud)
		|| !TestNotNull(TEXT("l'alleato"), Alleato) || !TestNotNull(TEXT("il nemico"), Nemico))
	{
		return false;
	}
	Alleato->TeamId = 0; Alleato->StableUnitId = 1; Alleato->Cell = FRTCellId(0, 0, 1);
	Nemico->TeamId = 1;  Nemico->StableUnitId = 2;  Nemico->Cell = FRTCellId(1, 0, 2);

	// Prima che qualcuno dica il piano la board non presenta niente: il contorno e' quello di una mappa a un piano.
	Hud->UpdateObserverVeil();
	TestEqual(TEXT("premessa: nessun piano presentato, contorno dell'attivo"),
		Alleato->GetRenderedOutlineStencilForTest(), StencilActive);

	TestTrue(TEXT("la board presenta il piano 1"), Board->SyncActiveLayerPresentation(1));
	TestTrue(TEXT("premessa: la presentazione e' a piu' piani"), Board->IsPresentationMultiLayer());
	Hud->UpdateObserverVeil();
	TestEqual(TEXT("alleato sul piano attivo: nero"), Alleato->GetRenderedOutlineStencilForTest(), StencilActive);

	// Cambia il piano ATTIVO: l'alleato, fermo sul piano 1, passa sotto e poi sopra.
	Board->SyncActiveLayerPresentation(2);
	Hud->UpdateObserverVeil();
	TestEqual(TEXT("piano attivo 2: l'alleato sul piano 1 e' sotto"), Alleato->GetRenderedOutlineStencilForTest(), StencilBelow);
	TestTrue(TEXT("e sotto vuol dire attraverso la geometria: e' il caso del mezzanino che copre (scope 5)"),
		ShowsThroughGeometry(Alleato->GetRenderedOutlineStencilForTest()));
	Board->SyncActiveLayerPresentation(0);
	Hud->UpdateObserverVeil();
	TestEqual(TEXT("piano attivo 0: l'alleato sul piano 1 e' sopra"), Alleato->GetRenderedOutlineStencilForTest(), StencilAbove);

	// Cambia il piano dell'UNITA': scende al piano attivo.
	Alleato->Cell = FRTCellId(0, 0, 0);
	Hud->UpdateObserverVeil();
	TestEqual(TEXT("l'alleato scende al piano 0, che e' l'attivo: nero"), Alleato->GetRenderedOutlineStencilForTest(), StencilActive);

	// ⛔ Il nemico non ha voce nella vista: in tutto questo non ha mai avuto un contorno.
	TestFalse(TEXT("il nemico senza voce non e' noto"), Nemico->IsKnownToObserver());
	TestEqual(TEXT("e non ha contorno, su nessun corpo"), Nemico->GetRenderedOutlineStencilForTest(), StencilNone);
	TestEqual(TEXT("ne' uno stato da cui ricavarne la quota"), Nemico->GetLayerOutlineStencil(), StencilNone);
	return true;
}

/**
 * **La camera di chi guarda porta il post-process, coi colori della regola dei bordi.**
 *
 * ⚠️ Richiede l'asset: `M_UnitLayerOutline` lo scrive il commandlet `RTBuildUnitOutlineMaterial`. Se manca, in
 * partita non c'e' contorno e nessun errore — per questo qui la premessa e' un asserto, non un salto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitOutlineCameraTest,
	"RefactorTactics.UnitOutline.CameraCarriesThePostProcess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTUnitOutlineCameraTest::RunTest(const FString&)
{
	using namespace RTUnitLayerOutline;
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ON_SCOPE_EXIT{ RTWorldFixtures::DestroyWorld(World); };

	ARTCameraPawn* Camera = World->SpawnActor<ARTCameraPawn>();
	if (!TestNotNull(TEXT("la camera"), Camera)) { return false; }
	TestTrue(TEXT("premessa: prima della sincronizzazione non c'e' nessun blendable"),
		Camera->GetUnitLayerOutlineMaterialForTest() == nullptr && Camera->GetUnitLayerOutlineWeightForTest() < 0.f);

	FRTUnitLayerOutlineTuning Leve;
	TestTrue(TEXT("la prima sincronizzazione scrive"), Camera->SyncUnitLayerOutline(Leve));
	UMaterialInstanceDynamic* Mid = Camera->GetUnitLayerOutlineMaterialForTest();
	if (!TestNotNull(TEXT("il materiale del contorno si carica (lo scrive il commandlet RTBuildUnitOutlineMaterial)"), Mid))
	{
		return false;
	}
	TestTrue(TEXT("e' un post-process"), Mid->GetMaterial() != nullptr && Mid->GetMaterial()->MaterialDomain == MD_PostProcess);
	TestEqual(TEXT("sta fra i blendable della camera, a peso pieno"), Camera->GetUnitLayerOutlineWeightForTest(), 1.f);

	// I parametri si cercano per NOME nel materiale: un `Set` su un nome che il materiale non ha non fallisce, e
	// rileggere il valore appena scritto direbbe «c'e'» anche quando l'asset non lo usa.
	TArray<FMaterialParameterInfo> Vettori;
	TArray<FMaterialParameterInfo> Scalari;
	TArray<FGuid> Ids;
	Mid->GetAllVectorParameterInfo(Vettori, Ids);
	Mid->GetAllScalarParameterInfo(Scalari, Ids);
	auto Dichiara = [](const TArray<FMaterialParameterInfo>& Elenco, const FName Nome)
	{
		return Elenco.ContainsByPredicate([Nome](const FMaterialParameterInfo& I) { return I.Name == Nome; });
	};

	// I tre colori arrivano dalla regola dei bordi, in lineare.
	const uint8 Tutti[] = { StencilActive, StencilAbove, StencilBelow };
	for (const uint8 Stencil : Tutti)
	{
		FColor Atteso;
		ColorFor(Stencil, Atteso);
		TestTrue(*FString::Printf(TEXT("il parametro di colore dello stencil %d esiste nel materiale"), Stencil),
			Dichiara(Vettori, ColorParamFor(Stencil)));
		TestTrue(*FString::Printf(TEXT("ed e' il colore del bordo, stencil %d"), Stencil),
			Mid->K2_GetVectorParameterValue(ColorParamFor(Stencil)).Equals(FLinearColor::FromSRGBColor(Atteso), 1.e-4f));
	}
	TestTrue(TEXT("il parametro dello spessore esiste nel materiale"), Dichiara(Scalari, ParamThickness));
	TestTrue(TEXT("e vale quello delle leve"),
		FMath::IsNearlyEqual(Mid->K2_GetScalarParameterValue(ParamThickness), Leve.ThicknessPx));

	TestFalse(TEXT("le stesse leve non riscrivono: e' il caso di ogni fotogramma"), Camera->SyncUnitLayerOutline(Leve));

	Leve.ThicknessPx = 4.f;
	TestTrue(TEXT("uno spessore diverso riscrive"), Camera->SyncUnitLayerOutline(Leve));
	TestTrue(TEXT("e arriva al materiale"), FMath::IsNearlyEqual(Mid->K2_GetScalarParameterValue(ParamThickness), 4.f));
	TestTrue(TEXT("senza creare una seconda istanza"), Camera->GetUnitLayerOutlineMaterialForTest() == Mid);

	Leve.bEnabled = false;
	TestTrue(TEXT("spegnere riscrive"), Camera->SyncUnitLayerOutline(Leve));
	TestEqual(TEXT("spento: il blendable resta, a peso zero"), Camera->GetUnitLayerOutlineWeightForTest(), 0.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
