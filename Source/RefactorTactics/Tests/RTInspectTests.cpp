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

/**
 * Ogni riga di una skill porta la sua posizione di kit (`#3748`): e' cio' che l'hover sulla scheda chiede al controller.
 *
 * L'oracolo e' l'ordine del kit dell'unita', non `Subject.KitAbilityIndices`: la posizione attesa di ogni titolo e' quella in cui
 * `GetAbility` lo restituisce. Un compositore che perdesse le posizioni, o un soggetto che le scrivesse spostate, cade qui.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectLinesCarryAbilityIndexTest,
	"RefactorTactics.UI.InspectLinesCarryTheirAbilityIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectLinesCarryAbilityIndexTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	const URTHeroData* Hero = InspectHero(0);
	FRTCellId Cell; Cell.X = 0; Cell.Y = 0;
	ARTUnit* Unit = InspectSpawn(World, 1, Hero, Cell);
	if (!TestTrue(TEXT("l'unita' esiste"), Unit != nullptr && Hero != nullptr))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTInspectView View = URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Unit, 0, true));
	TestEqual(TEXT("un indice per riga"), View.LineAbilityIndices.Num(), View.Lines.Num());

	int32 SkillsChecked = 0;
	for (int32 Position = 0; Position < Unit->NumAbilities(); ++Position)
	{
		const URTActionData* Action = Unit->GetAbility(Position);
		if (Action == nullptr) { continue; }
		const FRTActionTooltipView Tooltip = URTHudViewModel::BuildCatalogTooltip(
			URTHudViewModel::BuildCatalogActionRow(Action, Position, NAME_None));
		if (!Tooltip.IsValid()) { continue; }

		// Il titolo e' la riga intera o la riga fino a « - »: un titolo che ne e' il prefisso non la scambia con un'altra skill.
		const FString Title = Tooltip.Title.ToString();
		int32 TitleLine = INDEX_NONE;
		for (int32 Line = 0; Line < View.Lines.Num(); ++Line)
		{
			if (View.Lines[Line] == Title || View.Lines[Line].StartsWith(Title + TEXT(" - ")))
			{
				TitleLine = Line;
				break;
			}
		}
		if (!TestTrue(*FString::Printf(TEXT("la riga di `%s` c'e'"), *Title), TitleLine != INDEX_NONE)) { continue; }
		++SkillsChecked;
		TestEqual(*FString::Printf(TEXT("`%s` porta la sua posizione di kit"), *Title), View.LineAbilityIndices[TitleLine], Position);

		// La riga dei numeri, se c'e', e' della stessa skill: passarci sopra mostra la stessa portata.
		const int32 NumbersLine = TitleLine + 1;
		if (View.Lines.IsValidIndex(NumbersLine) && View.Lines[NumbersLine].StartsWith(TEXT("  ")))
		{
			TestEqual(*FString::Printf(TEXT("i numeri di `%s` portano la stessa posizione"), *Title),
				View.LineAbilityIndices[NumbersLine], Position);
		}
	}
	TestTrue(TEXT("anti-vacuita': almeno una skill con la sua riga"), SkillsChecked > 0);

	// Cio' che non e' di una skill non indica nessuna skill: il nome e i PV in cima, e ogni riga di equipaggiamento e di ricarica.
	TestEqual(TEXT("il nome non e' di una skill"), View.LineAbilityIndices[0], static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("i PV non sono di una skill"), View.LineAbilityIndices[1], static_cast<int32>(INDEX_NONE));
	int32 NotSkillLines = 0;
	for (int32 Line = 0; Line < View.Lines.Num(); ++Line)
	{
		if (View.LineAbilityIndices[Line] == INDEX_NONE) { ++NotSkillLines; }
	}
	TestTrue(TEXT("anti-vacuita': ci sono righe che non sono di una skill"), NotSkillLines > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * La scheda si nasconde in una finestra di reazione e in un modale, e mai senza una fissazione (`#3748`, spec §3 regola 11).
 *
 * Tabella scritta a mano, contesto per contesto: la regola e' un elenco chiuso, e una funzione che confrontasse un insieme
 * calcolato con se stessa non direbbe niente. Questo e' il lato puro; che il widget resti fissato mentre e' nascosto lo prova il
 * test del widget.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectShowCardOutsideDialogsTest,
	"RefactorTactics.UI.InspectShouldShowCardOnlyOutsideDialogs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectShowCardOutsideDialogsTest::RunTest(const FString&)
{
	struct FRiga { ERTPointerContext Contesto; const TCHAR* Nome; bool bVisibileSeFissata; };
	const FRiga Tabella[] = {
		{ ERTPointerContext::IdleSelection,     TEXT("IdleSelection"),     true },
		{ ERTPointerContext::Planning,          TEXT("Planning"),          true },
		{ ERTPointerContext::Pathing,           TEXT("Pathing"),           true },
		{ ERTPointerContext::Targeting,         TEXT("Targeting"),         true },
		{ ERTPointerContext::Facing,            TEXT("Facing"),            true },
		{ ERTPointerContext::ResolutionPlayback, TEXT("ResolutionPlayback"), true },
		{ ERTPointerContext::ReactionWindow,    TEXT("ReactionWindow"),    false },
		{ ERTPointerContext::Modal,             TEXT("Modal"),             false },
	};

	int32 Visible = 0;
	int32 Hidden = 0;
	for (const FRiga& Riga : Tabella)
	{
		TestEqual(*FString::Printf(TEXT("%s, fissata"), Riga.Nome),
			URTInspectLibrary::ShouldShowCard(Riga.Contesto, /*bPinned=*/ true), Riga.bVisibileSeFissata);
		TestFalse(*FString::Printf(TEXT("%s, senza fissazione non c'e' niente da mostrare"), Riga.Nome),
			URTInspectLibrary::ShouldShowCard(Riga.Contesto, /*bPinned=*/ false));
		(Riga.bVisibileSeFissata ? Visible : Hidden) += 1;
	}
	// Il conto viene dall'enum e non da `Modal`: un contesto aggiunto dopo di lui deve far cadere la tabella, non restarne fuori.
	const UEnum* ContextEnum = StaticEnum<ERTPointerContext>();
	if (!TestNotNull(TEXT("l'enum dei contesti e' riflesso"), ContextEnum)) { return false; }
	TestEqual(TEXT("la tabella copre ogni contesto dell'enum"), Visible + Hidden, ContextEnum->NumEnums() - 1);
	TestTrue(TEXT("anti-vacuita': la tabella ha sia contesti che mostrano sia contesti che nascondono"), Visible > 0 && Hidden > 0);
	return true;
}

/**
 * Un taglio della scheda non e' mai muto (`#3748`, spec §7): mostrate piu' nascoste sopra e sotto fanno sempre il totale, e
 * l'ultima riga visibile dice quante ne restano. La lezione e' quella di `URTContextInspectorLibrary::VisibleLines` (#3320).
 *
 * Il corpo si riconosce dal testo («riga NN»), non dal calcolo che lo produce: i marcatori si leggono contando le righe vere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectWindowNeverTruncatesSilentlyTest,
	"RefactorTactics.UI.InspectWindowNeverTruncatesSilently",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectWindowNeverTruncatesSilentlyTest::RunTest(const FString&)
{
	constexpr int32 Total = 12;
	constexpr int32 Max = 5;

	FRTInspectView View;
	for (int32 I = 0; I < Total; ++I)
	{
		View.Lines.Add(FString::Printf(TEXT("riga %02d"), I));
		View.LineAbilityIndices.Add(I % 3 == 0 ? I : INDEX_NONE);
	}

	auto DigitsOf = [](const FString& Text)
	{
		FString Digits;
		for (const TCHAR C : Text) { if (FChar::IsDigit(C)) { Digits.AppendChar(C); } }
		return Digits.IsEmpty() ? -1 : FCString::Atoi(*Digits);
	};

	TSet<int32> EverShown;
	bool bSawTop = false;
	bool bSawBottom = false;
	for (int32 First = -3; First <= Total + 3; ++First)
	{
		const FRTInspectView Window = URTInspectLibrary::VisibleWindow(View, First, Max);
		const FString Where = FString::Printf(TEXT("da %d"), First);
		TestEqual(*FString::Printf(TEXT("%s: la finestra riempie le righe che ha"), *Where), Window.Lines.Num(), Max);
		TestEqual(*FString::Printf(TEXT("%s: un indice per riga"), *Where), Window.LineAbilityIndices.Num(), Window.Lines.Num());

		TArray<int32> Body;
		int32 TopMarker = INDEX_NONE;
		int32 BottomMarker = INDEX_NONE;
		for (int32 Row = 0; Row < Window.Lines.Num(); ++Row)
		{
			const FString& Text = Window.Lines[Row];
			if (Text.StartsWith(TEXT("…")))
			{
				if (Text.EndsWith(TEXT("righe sopra"))) { TopMarker = DigitsOf(Text); }
				else { BottomMarker = DigitsOf(Text); }
				TestEqual(*FString::Printf(TEXT("%s: un marcatore non e' di una skill"), *Where),
					Window.LineAbilityIndices[Row], static_cast<int32>(INDEX_NONE));
			}
			else
			{
				const int32 Source = DigitsOf(Text);
				Body.Add(Source);
				TestEqual(*FString::Printf(TEXT("%s: la riga %d porta il suo indice"), *Where, Source),
					Window.LineAbilityIndices[Row], View.LineAbilityIndices[Source]);
			}
		}
		if (!TestTrue(*FString::Printf(TEXT("%s: c'e' del corpo"), *Where), Body.Num() > 0)) { continue; }

		for (int32 I = 1; I < Body.Num(); ++I)
		{
			TestEqual(*FString::Printf(TEXT("%s: il corpo e' contiguo"), *Where), Body[I], Body[I - 1] + 1);
		}
		const int32 HiddenAbove = Body[0];
		const int32 HiddenBelow = Total - (Body.Last() + 1);
		TestEqual(*FString::Printf(TEXT("%s: il marcatore in alto dice quante restano sopra"), *Where),
			TopMarker, HiddenAbove > 0 ? HiddenAbove : static_cast<int32>(INDEX_NONE));
		TestEqual(*FString::Printf(TEXT("%s: il marcatore in basso dice quante restano sotto"), *Where),
			BottomMarker, HiddenBelow > 0 ? HiddenBelow : static_cast<int32>(INDEX_NONE));
		TestEqual(*FString::Printf(TEXT("%s: mostrate e nascoste fanno il totale"), *Where),
			Body.Num() + HiddenAbove + HiddenBelow, Total);

		for (const int32 Source : Body) { EverShown.Add(Source); }
		bSawTop |= HiddenAbove > 0;
		bSawBottom |= HiddenBelow > 0;
	}
	TestEqual(TEXT("scorrendo si arriva a ogni riga"), EverShown.Num(), Total);
	TestTrue(TEXT("anti-vacuita': si e' visto un taglio in alto e uno in basso"), bSawTop && bSawBottom);
	TestEqual(TEXT("l'ultima finestra parte da Total - (Max - 1)"), URTInspectLibrary::ClampFirstLine(Total, Max, 1000), Total - (Max - 1));

	// Che entri tutto: nessun marcatore, e le righe sono le stesse.
	const FRTInspectView Whole = URTInspectLibrary::VisibleWindow(View, 4, Total);
	TestEqual(TEXT("se entra tutto, non si taglia"), Whole.Lines.Num(), Total);
	TestEqual(TEXT("se entra tutto, la prima riga e' la prima"), Whole.Lines[0], View.Lines[0]);

	// Sotto le tre righe i marcatori non entrano: taglio secco dalle prime, e vuoto per un budget nullo.
	const FRTInspectView Tiny = URTInspectLibrary::VisibleWindow(View, 6, 2);
	TestEqual(TEXT("con due righe: due righe"), Tiny.Lines.Num(), 2);
	TestEqual(TEXT("con due righe: dalle prime"), Tiny.Lines[0], View.Lines[0]);
	TestEqual(TEXT("con un budget nullo: niente"), URTInspectLibrary::VisibleWindow(View, 0, 0).Lines.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
