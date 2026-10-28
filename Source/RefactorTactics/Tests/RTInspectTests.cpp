// La scheda dell'ispezione (`#3748`, F3): il soggetto pubblico e il compositore.

#include "Misc/AutomationTest.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchStateHash.h"
#include "UI/RTHUD.h"
#include "UI/RTInspect.h"
#include "Unit/RTUnit.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	ARTUnit* InspectSpawn(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, 250.f);
		return U;
	}

	const URTHeroData* InspectHero(int32 Which)
	{
		// Non `static`: gli eroi sono `NewObject` non rootati, e un puntatore tenuto fra due test sopravvive a un GC.
		const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
		return Roster.IsValidIndex(Which) ? Roster[Which] : nullptr;
	}

	FString InspectText(const FRTInspectView& View) { return FString::Join(View.Lines, TEXT("\n")); }

	TArray<FString> InspectForbiddenFields(const UStruct* Type, const TSet<FName>& AllowedStructs, int32& Examined)
	{
		TArray<FString> Out;
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			++Examined;
			const FProperty* Element = *It;
			if (const FArrayProperty* Array = CastField<FArrayProperty>(Element)) { Element = Array->Inner; }
			if (CastField<FObjectPropertyBase>(Element) || CastField<FSoftObjectProperty>(Element))
			{
				Out.Add(FString::Printf(TEXT("%s (oggetto)"), *It->GetName()));
			}
			else if (const FStructProperty* AsStruct = CastField<FStructProperty>(Element))
			{
				if (!AllowedStructs.Contains(AsStruct->Struct->GetFName()))
				{
					Out.Add(FString::Printf(TEXT("%s (%s)"), *It->GetName(), *AsStruct->Struct->GetName()));
				}
			}
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectSubjectCarriesNoRawTypesTest,
	"RefactorTactics.UI.InspectSubjectCarriesNoRawTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectSubjectCarriesNoRawTypesTest::RunTest(const FString&)
{
	const TSet<FName> Allowed = { FName(TEXT("RTUnitCardView")), FName(TEXT("RTStatusBadgeView")),
		FName(TEXT("RTActionTooltipView")), FName(TEXT("RTActionTooltipLine")), FName(TEXT("RTInspectEquipmentLine")),
		FName(TEXT("RTInspectDeducedCooldown")) };

	int32 Examined = 0;
	const TArray<const UStruct*> Types = { FRTInspectSubject::StaticStruct(), FRTInspectEquipmentLine::StaticStruct(),
		FRTInspectDeducedCooldown::StaticStruct(), FRTInspectView::StaticStruct() };
	for (const UStruct* Type : Types)
	{
		for (const FString& Field : InspectForbiddenFields(Type, Allowed, Examined))
		{
			AddError(FString::Printf(TEXT("`%s` porta un tipo grezzo: %s"), *Type->GetName(), *Field));
		}
	}
	TestTrue(TEXT("il controllo esamina dei campi"), Examined > 0);

	// Anti-vacuita': lo stesso controllo, su un tipo con un `FGameplayTag`, lo vede.
	int32 Dummy = 0;
	TestTrue(TEXT("il controllo prende un FGameplayTag grezzo"),
		InspectForbiddenFields(FGameplayTagContainer::StaticStruct(), Allowed, Dummy).Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectTwinsReadTheSameTest,
	"RefactorTactics.UI.InspectTwoEnemiesDifferingOnlyInThePlanReadTheSame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectTwinsReadTheSameTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	const URTHeroData* Hero = InspectHero(0);
	const URTHeroData* Other = InspectHero(1);
	FRTCellId C1; C1.X = 0; C1.Y = 0;
	FRTCellId C2; C2.X = 1; C2.Y = 0;
	ARTUnit* A = InspectSpawn(World, 1, Hero, C1);
	ARTUnit* B = InspectSpawn(World, 1, Hero, C2);
	ARTUnit* D = InspectSpawn(World, 1, Other, C1);
	if (!TestTrue(TEXT("le unita' esistono"), A && B && D && Hero && Other))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// B differisce da A nel piano, nelle ricariche vere e nella variante attiva.
	for (int32 I = 0; I < B->Abilities.Num(); ++I) { B->ConsumeAbility(I); } // ricariche vere
	B->ActiveVariantId = FName(TEXT("VarianteDiProva"));
	B->PlannedAbilityIndex = 1;
	B->PlannedReactionAbility = 2;
	B->PlannedDashAbility = 3;
	B->bAttackTargetsCell = true;

	const FString TextA = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(A, 0, true)));
	const FString TextB = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(B, 0, true)));
	TestEqual(TEXT("due nemici che differiscono nel piano danno la stessa scheda"), TextA, TextB);
	TestTrue(TEXT("e la scheda ha un kit (non e' vuota per costruzione)"), TextA.Contains(TEXT("Kit")));

	// `Compose` scarta `State` e `Reason`: il testo non vede un canale di partita che il soggetto portasse. Si guarda
	// quindi il SOGGETTO, dove una ricarica vera o un piano illegale scriverebbero stato e motivo.
	const FRTInspectSubject SoggettoB = ARTHUD::BuildInspectSubject(B, 0, true);
	TestTrue(TEXT("il kit del soggetto non e' vuoto"), SoggettoB.Kit.Num() > 0);
	for (const FRTActionTooltipView& Voce : SoggettoB.Kit)
	{
		TestTrue(TEXT("nessun motivo di partita nel kit"), Voce.Reason.IsEmpty());
		TestTrue(TEXT("nessuno stato di partita nel kit"), Voce.State == ERTActionSlotState::Empty);
	}

	// Controllo positivo: un altro eroe cambia la scheda.
	const FString TextD = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(D, 0, true)));
	TestNotEqual(TEXT("un altro eroe da' un'altra scheda"), TextA, TextD);

	// Il verso segue la targa: spento, la riga non c'e'.
	TestFalse(TEXT("verso spento: nessuna riga"),
		InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(A, 0, false))).Contains(TEXT("Guarda")));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectEquipmentTest,
	"RefactorTactics.UI.InspectNamesTheMountedEquipment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectEquipmentTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	// Ogni eroe con un loadout di default non vuoto: i pezzi realmente applicati sono tutti e soli quelli registrati,
	// e ognuno e' nominato col nome del catalogo. Anti-vacuita': fra gli eroi ci sono tutti gli slot.
	int32 Eroi = 0;
	TSet<int32> SlotVisti;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		const TArray<FName> Loadout = URTCatalogLibrary::DefaultLoadoutFor(Hero->HeroId);
		if (Loadout.Num() == 0) { continue; }
		++Eroi;

		FRTCellId C; C.X = 0; C.Y = 0;
		ARTUnit* Bare = InspectSpawn(World, 1, Hero, C);
		ARTUnit* Armed = InspectSpawn(World, 1, Hero, C);
		Armed->EquipLoadout(Loadout);

		TestEqual(TEXT("senza EquipLoadout nessun pezzo registrato"), Bare->EquippedItemIds.Num(), 0);
		TestTrue(TEXT("i pezzi registrati sono tutti e soli quelli del loadout applicato"), Armed->EquippedItemIds == Loadout);

		const FString TextBare = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Bare, 0, true)));
		const FString TextArmed = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Armed, 0, true)));
		TestFalse(TEXT("senza equipaggiamento la sezione non c'e'"), TextBare.Contains(TEXT("Equipaggiamento")));
		TestTrue(TEXT("con equipaggiamento la sezione c'e'"), TextArmed.Contains(TEXT("Equipaggiamento")));
		for (const FName& Id : Armed->EquippedItemIds)
		{
			const URTEquipmentData* Piece = URTCatalogLibrary::FindEquipment(Id);
			if (!TestNotNull(TEXT("il pezzo registrato esiste nel catalogo"), Piece)) { continue; }
			SlotVisti.Add(static_cast<int32>(Piece->Slot));
			TestTrue(TEXT("ogni pezzo e' nominato col nome del catalogo"),
				!Piece->DisplayName.IsEmpty() && TextArmed.Contains(Piece->DisplayName.ToString()));
		}

		// Un id inesistente non viene registrato.
		ARTUnit* Ghost = InspectSpawn(World, 1, Hero, C);
		Ghost->EquipLoadout({ FName(TEXT("Equip.Inesistente")) });
		TestEqual(TEXT("un id inesistente non viene registrato"), Ghost->EquippedItemIds.Num(), 0);
	}
	TestTrue(TEXT("ha esaminato degli eroi con equipaggiamento"), Eroi > 0);
	TestTrue(TEXT("e il gadget e' fra i pezzi esaminati (la via che concede un'azione)"),
		SlotVisti.Contains(static_cast<int32>(ERTEquipmentSlot::Gadget)) || SlotVisti.Contains(static_cast<int32>(ERTEquipmentSlot::ReactionModule)));
	TestTrue(TEXT("e la variante d'arma"), SlotVisti.Contains(static_cast<int32>(ERTEquipmentSlot::WeaponVariant)));

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectHashTest,
	"RefactorTactics.Inspect.EquippedItemIdsDoNotEnterTheMatchHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectHashTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	const URTHeroData* Hero = InspectHero(0);
	FRTCellId C; C.X = 0; C.Y = 0;
	ARTUnit* U = InspectSpawn(World, 1, Hero, C);
	if (!TestTrue(TEXT("l'unita' esiste"), U != nullptr))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const URTHexMapAsset* Map = NewObject<URTHexMapAsset>();
	const TArray<int32> Scores = { 0, 0 };
	auto Hash = [&]()
	{
		return URTMatchStateHashLibrary::HashMatchState(Map, URTMatchStateHashLibrary::BuildUnitDigests({ U }), Scores);
	};

	// Tutto il resto fisso (abilita' comprese): varia SOLO il campo nuovo.
	const uint32 Before = Hash();
	U->EquippedItemIds = { FName(TEXT("Equip.Uno")), FName(TEXT("Equip.Due")) };
	const uint32 After = Hash();
	TestEqual(TEXT("l'hash non cambia se cambia solo EquippedItemIds"), After, Before);

	// Controllo positivo: l'hash e' sensibile a uno stato vero (la vita), quindi l'uguaglianza non e' banale.
	U->Health = FMath::Max(0, U->Health - 1);
	TestNotEqual(TEXT("l'hash cambia se cambia la vita"), Hash(), Before);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectGateTest,
	"RefactorTactics.UI.InspectUnobservedEnemyReadsAsAnEmptyCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectGateTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	FRTCellId C; C.X = 0; C.Y = 0;
	ARTUnit* Enemy = InspectSpawn(World, 1, InspectHero(0), C);
	if (!TestTrue(TEXT("l'unita' esiste"), Enemy != nullptr))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	Enemy->StableUnitId = 7;
	const FRTInspectSubject Subject = ARTHUD::BuildInspectSubject(Enemy, 0, true);
	const int32 Id = Enemy->StableUnitId;

	// La finestra di nascita: `IsKnownToObserver()` e' ancora vero, e la vista non ha nessuna voce.
	TestTrue(TEXT("premessa: il flag del velo nasce vero"), Enemy->IsKnownToObserver());

	FRTKnowledgeView Vuota;
	FRTKnowledgeView Ricordata;
	{
		FRTKnowledgeEntry E; E.StableUnitId = Id; E.Visibility = ERTKnowledgeVisibility::Remembered;
		Ricordata.Entries.Add(E);
	}
	FRTKnowledgeView Viva;
	{
		FRTKnowledgeEntry E; E.StableUnitId = Id; E.Visibility = ERTKnowledgeVisibility::Live;
		Viva.Entries.Add(E);
	}

	// Una voce Live di UN'ALTRA unita' non apre la scheda di questa.
	FRTKnowledgeView VivaAltrui;
	{
		FRTKnowledgeEntry E; E.StableUnitId = Id + 1; E.Visibility = ERTKnowledgeVisibility::Live;
		VivaAltrui.Entries.Add(E);
	}

	const FRTInspectView SenzaNemico = FRTInspectView();
	TestEqual(TEXT("nessuna voce (anche appena nata): come una cella vuota"),
		InspectText(URTInspectLibrary::ResolveInspect(Vuota, Id, Subject)), InspectText(SenzaNemico));
	TestEqual(TEXT("solo ricordata: come una cella vuota"),
		InspectText(URTInspectLibrary::ResolveInspect(Ricordata, Id, Subject)), InspectText(SenzaNemico));
	TestEqual(TEXT("la voce Live di un'altra unita': come una cella vuota"),
		InspectText(URTInspectLibrary::ResolveInspect(VivaAltrui, Id, Subject)), InspectText(SenzaNemico));
	TestEqual(TEXT("e la cella vuota non ha righe"), SenzaNemico.Lines.Num(), 0);

	// Controllo positivo: la stessa unita', `Live`, ha la scheda.
	TestTrue(TEXT("con una voce Live ha la scheda"), URTInspectLibrary::ResolveInspect(Viva, Id, Subject).Lines.Num() > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectReleaseTest,
	"RefactorTactics.PlayerInput.InspectionIsReleasedWhenTheSubjectIsNotLive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectReleaseTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	FRTCellId C; C.X = 0; C.Y = 0;
	ARTUnit* Enemy = InspectSpawn(World, 1, InspectHero(0), C);
	if (!TestTrue(TEXT("controller e unita' esistono"), PC && Enemy))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	Enemy->StableUnitId = 7;
	FRTKnowledgeView Viva;
	{
		FRTKnowledgeEntry E; E.StableUnitId = Enemy->StableUnitId; E.Visibility = ERTKnowledgeVisibility::Live;
		Viva.Entries.Add(E);
	}
	FRTKnowledgeView VivaAltrui;
	{
		FRTKnowledgeEntry E; E.StableUnitId = Enemy->StableUnitId + 1; E.Visibility = ERTKnowledgeVisibility::Live;
		VivaAltrui.Entries.Add(E);
	}

	PC->DispatchUnitClickForTest(Enemy, nullptr);
	TestTrue(TEXT("premessa: l'ispezione e' fissata"), PC->GetInspectedUnit() == Enemy && PC->IsInspectorPinned());

	TestFalse(TEXT("con la voce Live non rilascia"), PC->ReleaseInspectionUnlessLive(Viva));
	TestTrue(TEXT("e il soggetto resta"), PC->GetInspectedUnit() == Enemy && PC->IsInspectorPinned());

	TestTrue(TEXT("senza la voce Live rilascia"), PC->ReleaseInspectionUnlessLive(FRTKnowledgeView()));
	TestTrue(TEXT("soggetto azzerato"), PC->GetInspectedUnit() == nullptr);
	TestFalse(TEXT("pin spento"), PC->IsInspectorPinned());

	// Una voce Live di un'altra unita' non tiene in piedi la scheda di questa.
	PC->DispatchUnitClickForTest(Enemy, nullptr);
	TestTrue(TEXT("la voce Live di un'altra unita' rilascia"), PC->ReleaseInspectionUnlessLive(VivaAltrui));
	TestTrue(TEXT("soggetto azzerato con la voce altrui"), PC->GetInspectedUnit() == nullptr);

	// Un nemico caduto (KO, attore ancora in vita) rilascia anche con la voce Live.
	PC->DispatchUnitClickForTest(Enemy, nullptr);
	Enemy->Health = 0;
	TestTrue(TEXT("un KO rilascia"), PC->ReleaseInspectionUnlessLive(Viva));
	TestTrue(TEXT("soggetto azzerato dopo il KO"), PC->GetInspectedUnit() == nullptr);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * LA RIGA DI CATALOGO COINCIDE CON QUELLA DEL DOCK (`#3748`). `BuildAbilityCooldowns` parte da `BuildCatalogActionRow`:
 * il tooltip che ne esce, senza lo stato di partita, e' quello che la scheda mostra per la stessa azione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCatalogRowMatchesDockRowTest,
	"RefactorTactics.UI.InspectCatalogRowMatchesTheDockRow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCatalogRowMatchesDockRowTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	FRTCellId C; C.X = 0; C.Y = 0;
	ARTUnit* Unit = InspectSpawn(World, 1, InspectHero(0), C);
	if (!TestTrue(TEXT("l'unita' esiste"), Unit != nullptr))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const TArray<FRTAbilityCooldownView> Dock = URTHudViewModel::BuildAbilityCooldowns(Unit);
	TestTrue(TEXT("il dock ha delle azioni"), Dock.Num() > 0);
	int32 Confrontate = 0;
	for (int32 I = 0; I < Dock.Num(); ++I)
	{
		const URTActionData* Azione = Unit->GetAbility(I);
		if (!Azione) { continue; }
		const FRTAbilityCooldownView Riga = URTHudViewModel::BuildCatalogActionRow(Azione, I, Unit->ActiveVariantId);
		const FRTActionTooltipView DalCatalogo = URTHudViewModel::BuildCatalogTooltip(Riga);
		const FRTActionTooltipView DalDock = URTHudViewModel::BuildCatalogTooltip(Dock[I]);
		++Confrontate;
		TestTrue(TEXT("stesso tooltip di catalogo del dock"), URTHudViewModel::SameTooltip(DalCatalogo, DalDock));
		TestEqual(TEXT("stesso id"), Riga.ActionId, Dock[I].ActionId);
	}
	TestTrue(TEXT("ne ha confrontate"), Confrontate > 0);
	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
