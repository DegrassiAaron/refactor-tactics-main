// La scheda dell'ispezione (`#3748`, F3): il soggetto pubblico e il compositore.

#include "Misc/AutomationTest.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Kismet/GameplayStatics.h"
#include "Core/RTGameplayTags.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchStateHash.h"
#include "UI/RTGlossary.h"
#include "UI/RTHUD.h"
#include "UI/RTInspect.h"
#include "UI/RTInspectCardWidget.h" // `MaxRighe`: la finestra della scheda, letta dal default invece che riscritta qui
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

	/** Un'unita' di prova con il loadout di default del suo eroe, se ne ha uno (`EquippedItemIds` resta vuoto senza). */
	ARTUnit* InspectEquippedUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		ARTUnit* U = InspectSpawn(World, TeamId, Hero, Cell);
		if (U != nullptr && Hero != nullptr)
		{
			const TArray<FName> Loadout = URTCatalogLibrary::DefaultLoadoutFor(Hero->HeroId);
			if (Loadout.Num() > 0) { U->EquipLoadout(Loadout); }
		}
		return U;
	}

	/** Le righe di `Lines` che iniziano con `Prefix`, in ordine. */
	TArray<int32> InspectLinesStartingWith(const TArray<FString>& Lines, const FString& Prefix)
	{
		TArray<int32> Out;
		for (int32 I = 0; I < Lines.Num(); ++I)
		{
			if (Lines[I].StartsWith(Prefix, ESearchCase::CaseSensitive)) { Out.Add(I); }
		}
		return Out;
	}

	/** I campi booleani e numerici sciolti di un tipo (un `bUsableNow`, un `TurnsRemaining`): per un dettaglio di skill non ce ne sono. */
	TArray<FString> InspectLooseScalars(const UStruct* Type)
	{
		TArray<FString> Out;
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			if (CastField<FBoolProperty>(*It) != nullptr || CastField<FNumericProperty>(*It) != nullptr) { Out.Add(It->GetName()); }
		}
		return Out;
	}

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
	const TSet<FName> Allowed = { FName(TEXT("RTUnitCardView")), FName(TEXT("RTStatusBadgeView")), FName(TEXT("RTInspectEquipmentLine")),
		FName(TEXT("RTInspectDeducedCooldown")), FName(TEXT("RTInspectSkillDetail")) };

	int32 Examined = 0;
	const TArray<const UStruct*> Types = { FRTInspectSubject::StaticStruct(), FRTInspectEquipmentLine::StaticStruct(),
		FRTInspectDeducedCooldown::StaticStruct(), FRTInspectSkillDetail::StaticStruct(), FRTInspectView::StaticStruct() };
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

	// 🔑 IL DETTAGLIO DI UNA SKILL (#3748, 2026-10-31) ha un insieme CHIUSO di campi e nessuno e' stato vero. Il controllo dei tipi sopra vede un
	// puntatore o una struct di gioco, ma non un `bUsableNow` o un `TurnsRemaining`: quelli si fermano qui, per nome e per natura. Un campo nuovo
	// si dichiara in questo insieme, e chi lo fa deve dire perche' non e' una ricarica vera, un piano, un tasto o un'azione armata.
	const UStruct* Detail = FRTInspectSkillDetail::StaticStruct();
	TSet<FString> DetailNames;
	for (TFieldIterator<FProperty> It(Detail); It; ++It) { DetailNames.Add(It->GetName()); }
	const TSet<FString> ExpectedDetailNames = { TEXT("ActionId"), TEXT("Title"), TEXT("Lines"), TEXT("Equipment"), TEXT("Deduced") };
	TestTrue(*FString::Printf(TEXT("il dettaglio porta solo i campi dichiarati (trovati: %s)"), *FString::Join(DetailNames.Array(), TEXT(", "))),
		DetailNames.Num() == ExpectedDetailNames.Num() && DetailNames.Includes(ExpectedDetailNames));
	TestEqual(TEXT("e nessun booleano o numero sciolto: sarebbe uno stato (ricarica residua, «usabile ora»)"),
		InspectLooseScalars(Detail).Num(), 0);
	// La stima e' un array di `FRTInspectDeducedCooldown` e non un `TOptional` che la reflection non vede: di che tipo e' si legge.
	const FArrayProperty* DeducedArray = CastField<FArrayProperty>(Detail->FindPropertyByName(TEXT("Deduced")));
	const FStructProperty* DeducedInner = DeducedArray != nullptr ? CastField<FStructProperty>(DeducedArray->Inner) : nullptr;
	TestTrue(TEXT("`Deduced` e' un array di FRTInspectDeducedCooldown (la stima, non la ricarica vera)"),
		DeducedInner != nullptr && DeducedInner->Struct == FRTInspectDeducedCooldown::StaticStruct());
	// Il soggetto lo porta, come struct pubblica, e non ha acquistato nessun altro scalare sciolto.
	const FStructProperty* HoveredProp = CastField<FStructProperty>(FRTInspectSubject::StaticStruct()->FindPropertyByName(TEXT("HoveredSkill")));
	TestTrue(TEXT("il soggetto porta `HoveredSkill`"), HoveredProp != nullptr && HoveredProp->Struct == Detail);

	// Controllo positivo: lo stesso conteggio, sul tipo che porta la ricarica VERA di uno slot, trova i suoi scalari.
	TestTrue(TEXT("anti-vacuita': la scansione trova `TurnsRemaining` e `bUsableNow` su FRTAbilityCooldownView"),
		InspectLooseScalars(FRTAbilityCooldownView::StaticStruct()).Num() >= 2);
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

	// La premessa si MISURA: B ha davvero una ricarica vera in corso, e A no. Senza, «stessa scheda» potrebbe voler dire «nessuna differenza».
	int32 TrueCooldownsA = 0;
	int32 TrueCooldownsB = 0;
	for (int32 I = 0; I < A->NumAbilities(); ++I) { TrueCooldownsA += A->GetAbilityCooldown(I); }
	for (int32 I = 0; I < B->NumAbilities(); ++I) { TrueCooldownsB += B->GetAbilityCooldown(I); }
	TestTrue(TEXT("premessa: B ha una ricarica vera in corso e A no"), TrueCooldownsB > 0 && TrueCooldownsA == 0);

	const FRTInspectView ViewA = URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(A, 0, true));
	const FString TextA = InspectText(ViewA);
	const FString TextB = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(B, 0, true)));
	TestEqual(TEXT("due nemici che differiscono nel piano danno la stessa scheda"), TextA, TextB);
	// La scheda non e' vuota per costruzione: nome, PV e verso ci sono (il kit non c'e' piu': lo mostra la barra, `BuildPublicBar`).
	TestTrue(TEXT("e la scheda non e' vuota: nome, PV e verso"),
		ViewA.Lines.Num() >= 3 && ViewA.Lines[1].StartsWith(TEXT("PV ")) && ViewA.Lines[2].StartsWith(TEXT("Guarda verso")));

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
 * il tooltip che ne esce, senza lo stato di partita, e' quello che la barra in sola lettura (`BuildPublicBar`) mostra per la stessa azione.
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
 * LA SCHEDA NON HA RIGHE DI KIT (`#3748`, decisione d'autore del 2026-10-30): il kit di chi e' a fuoco lo mostra la barra delle skill, in
 * sola lettura, e la scheda non lo ripete. Restano nome, PV, verso, stati ed equipaggiamento; e, col cursore su uno slot (decisione d'autore
 * del 2026-10-31), i dettagli di QUELLA skill.
 *
 * L'oracolo e' il kit dell'unita' (`GetAbility`, titolo dalla riga di catalogo), non il testo della scheda: una riga di kit era il titolo da
 * solo o «Titolo - frase», e la sezione apriva con la riga «Kit». Due unita' per eroe, con lo stesso soggetto di produzione
 * (`BuildInspectSubject`): una nuda, in cui, SENZA cursore, NESSUNA riga contiene un titolo, e CON il cursore su una posizione compare il titolo
 * di quella skill e di nessun'altra; una armata, in cui l'equipaggiamento c'e' e nessuna riga e' di kit. Il rivelatore ha il suo controllo
 * positivo, su una vista scritta a mano.
 *
 * ⚠️ Non c'e' un registro di «usate viste»: la stima delle ricariche dedotte (`#3751`) nomina un'azione solo se la squadra l'ha vista
 * usare, e vive nei suoi test (`InspectSubjectDeducesCooldownsOnlyForAnEnemy`, `InspectCardHoveredSkillShowsTheDeducedCooldownOnlyIfSeen`). Qui
 * il soggetto e' costruito senza registro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectCardHasNoKitRowsTest,
	"RefactorTactics.UI.InspectCardHasNoKitRows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectCardHasNoKitRowsTest::RunTest(const FString&)
{
	// La riga di kit com'era composta: il titolo da solo, o «Titolo - frase».
	const auto KitLineOf = [](const FRTInspectView& View, const FString& Title) -> int32
	{
		for (int32 Line = 0; Line < View.Lines.Num(); ++Line)
		{
			if (View.Lines[Line] == Title || View.Lines[Line].StartsWith(Title + TEXT(" - "))) { return Line; }
		}
		return INDEX_NONE;
	};
	const auto TitleOf = [](const ARTUnit* Unit, int32 Position) -> FString
	{
		const URTActionData* Action = Unit->GetAbility(Position);
		return Action != nullptr
			? URTHudViewModel::BuildCatalogTooltip(URTHudViewModel::BuildCatalogActionRow(Action, Position, NAME_None)).Title.ToString()
			: FString();
	};

	// Controllo positivo del rivelatore: su una vista col kit, scritta a mano, vede il titolo e la riga «Kit»; e non inventa un titolo.
	{
		FRTInspectView WithKit;
		WithKit.Lines = { TEXT("Nome"), TEXT("PV 10/10"), TEXT("Kit"), TEXT("Colpo - una frase"), TEXT("  Portata 4"), TEXT("Scatto") };
		TestEqual(TEXT("controllo positivo: il rivelatore trova «Titolo - frase»"), KitLineOf(WithKit, TEXT("Colpo")), 3);
		TestEqual(TEXT("controllo positivo: e il titolo da solo"), KitLineOf(WithKit, TEXT("Scatto")), 5);
		TestEqual(TEXT("controllo positivo: e non un titolo che non c'e'"), KitLineOf(WithKit, TEXT("Altro")), static_cast<int32>(INDEX_NONE));
		TestTrue(TEXT("controllo positivo: la riga «Kit» si riconosce per uguaglianza"), WithKit.Lines.Contains(TEXT("Kit")));
	}

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	int32 Eroi = 0;
	int32 Armati = 0;
	int32 Titoli = 0;
	int32 Sotto = 0;
	int32 Pezzi = 0;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		++Eroi;
		FRTCellId C; C.X = 0; C.Y = 0;
		ARTUnit* Bare = InspectSpawn(World, 1, Hero, C);
		if (!TestNotNull(TEXT("l'unita' esiste"), Bare)) { continue; }

		// La unita' nuda: nessuna riga contiene il titolo di una sua skill, e non c'e' la riga «Kit».
		const FRTInspectView BareView = URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Bare, 0, true));
		TestFalse(*FString::Printf(TEXT("%s: nessuna riga «Kit»"), *Hero->HeroId.ToString()), BareView.Lines.Contains(TEXT("Kit")));
		TestEqual(*FString::Printf(TEXT("%s: la prima riga e' il nome"), *Hero->HeroId.ToString()),
			BareView.Lines.Num() > 0 ? BareView.Lines[0] : FString(), ARTUnit::DisplayLabel(Bare->HeroDisplayName, Bare->HeroId, Bare->GetName()));
		for (int32 Position = 0; Position < Bare->NumAbilities(); ++Position)
		{
			const FString Title = TitleOf(Bare, Position);
			if (Title.IsEmpty()) { continue; }
			++Titoli;
			for (const FString& Line : BareView.Lines)
			{
				TestFalse(*FString::Printf(TEXT("%s: la scheda non nomina `%s` (riga `%s`)"), *Hero->HeroId.ToString(), *Title, *Line),
					Line.Contains(Title));
			}

			// COL CURSORE su questa posizione compare il suo titolo, una volta, e nessun altro titolo del kit: la scheda mostra UNA skill, non il kit.
			// Un titolo uguale a quello sotto il cursore non e' «un altro».
			const FRTInspectView HoverView = URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Bare, 0, true, nullptr, Position));
			++Sotto;
			int32 TitleLines = 0;
			for (const FString& Line : HoverView.Lines) { TitleLines += Line == Title ? 1 : 0; }
			TestEqual(*FString::Printf(TEXT("%s: col cursore sulla posizione %d compare il titolo `%s`, una volta"), *Hero->HeroId.ToString(), Position, *Title),
				TitleLines, 1);
			for (int32 Other = 0; Other < Bare->NumAbilities(); ++Other)
			{
				const FString OtherTitle = TitleOf(Bare, Other);
				if (Other == Position || OtherTitle.IsEmpty() || OtherTitle == Title) { continue; }
				TestEqual(*FString::Printf(TEXT("%s: col cursore su `%s` non compare il titolo di `%s`"), *Hero->HeroId.ToString(), *Title, *OtherTitle),
					KitLineOf(HoverView, OtherTitle), static_cast<int32>(INDEX_NONE));
			}
		}

		// La unita' armata: l'equipaggiamento c'e', e nessuna riga e' una riga di kit.
		const TArray<FName> Loadout = URTCatalogLibrary::DefaultLoadoutFor(Hero->HeroId);
		if (Loadout.Num() == 0) { continue; }
		ARTUnit* Armed = InspectSpawn(World, 1, Hero, C);
		if (!TestNotNull(TEXT("l'unita' armata esiste"), Armed)) { continue; }
		Armed->EquipLoadout(Loadout);
		++Armati;
		const FRTInspectView ArmedView = URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Armed, 0, true));
		const FString ArmedText = InspectText(ArmedView);
		// L'equipaggiamento e' UNA riga coi nomi (la testata compatta, 2026-10-31): non piu' una sezione con le righe `+`/`-` dei pezzi.
		TestEqual(*FString::Printf(TEXT("%s: la riga dell'equipaggiamento c'e', una sola"), *Hero->HeroId.ToString()),
			InspectLinesStartingWith(ArmedView.Lines, TEXT("Equipaggiamento: ")).Num(), 1);
		for (const FName& Id : Armed->EquippedItemIds)
		{
			const URTEquipmentData* Piece = URTCatalogLibrary::FindEquipment(Id);
			if (!TestNotNull(TEXT("il pezzo registrato esiste nel catalogo"), Piece)) { continue; }
			++Pezzi;
			TestTrue(*FString::Printf(TEXT("%s: `%s` e' nominato"), *Hero->HeroId.ToString(), *Piece->DisplayName.ToString()),
				ArmedText.Contains(Piece->DisplayName.ToString()));
		}
		TestFalse(*FString::Printf(TEXT("%s armata: nessuna riga «Kit»"), *Hero->HeroId.ToString()), ArmedView.Lines.Contains(TEXT("Kit")));
		for (int32 Position = 0; Position < Armed->NumAbilities(); ++Position)
		{
			const FString Title = TitleOf(Armed, Position);
			if (Title.IsEmpty()) { continue; }
			TestEqual(*FString::Printf(TEXT("%s armata: nessuna riga di kit per `%s`"), *Hero->HeroId.ToString(), *Title),
				KitLineOf(ArmedView, Title), static_cast<int32>(INDEX_NONE));
		}
	}
	TestTrue(TEXT("anti-vacuita': ha esaminato degli eroi, dei titoli di skill, dei cursori su una posizione, e degli eroi armati con dei pezzi"),
		Eroi > 0 && Titoli > 0 && Sotto > 0 && Armati > 0 && Pezzi > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * LA TESTATA E' COMPATTA (`#3748`, decisione d'autore del 2026-10-31): senza il cursore su una skill, il box ha nome, PV, verso, stati, UNA
 * riga coi NOMI dell'equipaggiamento e la riga di invito. Niente righe `+`/`-` dei pezzi, niente sezione «Ricariche», nessun titolo di skill.
 *
 * Due livelli. Un soggetto scritto a mano con TUTTO acceso (tre pezzi con vantaggio e svantaggio, una ricarica dedotta, due stati, il verso): se
 * `Compose` lasciasse una riga in testata la vedrebbe, e il controllo positivo mostra che quelle righe esistono e che il rivelatore le trova
 * (con un cursore su una skill, le stesse righe `+`/`-` compaiono). E le unita' vere di ogni eroe, col loro loadout di default, dal soggetto di
 * produzione: il conto delle righe e' esatto, non un «non ce ne sono troppe».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectCardHeaderIsCompactTest,
	"RefactorTactics.UI.InspectCardHeaderIsCompact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectCardHeaderIsCompactTest::RunTest(const FString&)
{
	const FString Invitation = TEXT("Passa su una skill");

	// --- 1. il soggetto scritto a mano, con tutto acceso -------------------------------------------------------
	FRTInspectSubject Subject;
	Subject.DisplayName = FText::FromString(TEXT("Nemico di prova"));
	Subject.Card.Health = 17;
	Subject.Card.MaxHealth = 30;
	Subject.Card.Shield = 5;
	Subject.Facing = ERTHexDirection::E;
	Subject.bShowFacing = true;
	FRTStatusBadgeView Burning; Burning.Tag = TAG_Status_Burning.GetTag().GetTagName();
	Subject.Statuses.Add(Burning);
	const FString BurningName = RTGlossary::NameFor(Burning.Tag).ToString();
	if (!TestFalse(TEXT("premessa: lo stato di prova ha un nome nel glossario (se no la riga dello stato non esiste e il controllo e' vacuo)"),
		BurningName.IsEmpty()))
	{
		return false;
	}

	const TCHAR* PieceNames[] = { TEXT("Pezzo Alfa"), TEXT("Pezzo Beta"), TEXT("Pezzo Gamma") };
	const TCHAR* Advantages[] = { TEXT("vantaggio di alfa"), TEXT("vantaggio di beta"), TEXT("vantaggio di gamma") };
	const TCHAR* Drawbacks[] = { TEXT("svantaggio di alfa"), TEXT("svantaggio di beta"), TEXT("svantaggio di gamma") };
	const ERTEquipmentSlot Slots[] = { ERTEquipmentSlot::WeaponVariant, ERTEquipmentSlot::Gadget, ERTEquipmentSlot::ReactionModule };
	for (int32 I = 0; I < 3; ++I)
	{
		FRTInspectEquipmentLine Line;
		Line.Name = FText::FromString(PieceNames[I]);
		Line.Slot = Slots[I];
		Line.Advantage = FText::FromString(Advantages[I]);
		Line.Drawback = FText::FromString(Drawbacks[I]);
		Subject.Equipment.Add(Line);
	}
	FRTInspectDeducedCooldown Deduced;
	if (!TestTrue(TEXT("premessa: la ricarica dedotta di prova esiste"),
		URTInspectLibrary::MakeDeducedCooldown(TEXT("Hero.Test.Shot"), FText::FromString(TEXT("Colpo di prova")), 3, 4, Deduced)))
	{
		return false;
	}
	Subject.DeducedCooldowns.Add(Deduced);

	const FRTInspectView Header = URTInspectLibrary::Compose(Subject);
	const FString HeaderText = InspectText(Header);
	TestEqual(TEXT("la prima riga e' il nome"), Header.Lines.Num() > 0 ? Header.Lines[0] : FString(), FString(TEXT("Nemico di prova")));
	TestEqual(TEXT("poi PV e scudo"), Header.Lines.Num() > 1 ? Header.Lines[1] : FString(), FString(TEXT("PV 17/30, scudo 5")));
	TestEqual(TEXT("poi il verso"), Header.Lines.Num() > 2 ? Header.Lines[2] : FString(), FString(TEXT("Guarda verso E.")));
	TestEqual(TEXT("lo stato c'e', col suo nome"), InspectLinesStartingWith(Header.Lines, BurningName).Num(), 1);
	const TArray<int32> EquipmentLines = InspectLinesStartingWith(Header.Lines, TEXT("Equipaggiamento: "));
	if (TestEqual(TEXT("l'equipaggiamento e' UNA riga"), EquipmentLines.Num(), 1))
	{
		TestEqual(TEXT("coi soli nomi, separati da un punto mediano"), Header.Lines[EquipmentLines[0]],
			FString(TEXT("Equipaggiamento: Pezzo Alfa · Pezzo Beta · Pezzo Gamma")));
	}
	for (int32 I = 0; I < 3; ++I)
	{
		TestFalse(*FString::Printf(TEXT("il vantaggio di `%s` non e' in testata"), PieceNames[I]), HeaderText.Contains(Advantages[I]));
		TestFalse(*FString::Printf(TEXT("lo svantaggio di `%s` non e' in testata"), PieceNames[I]), HeaderText.Contains(Drawbacks[I]));
		TestEqual(*FString::Printf(TEXT("`%s` non ha una riga sua (nome e slot) in testata"), PieceNames[I]),
			InspectLinesStartingWith(Header.Lines, FString(PieceNames[I]) + TEXT(" (")).Num(), 0);
	}
	TestEqual(TEXT("nessuna riga `+` in testata"), InspectLinesStartingWith(Header.Lines, TEXT("  + ")).Num(), 0);
	TestEqual(TEXT("nessuna riga `-` in testata"), InspectLinesStartingWith(Header.Lines, TEXT("  - ")).Num(), 0);
	TestFalse(TEXT("nessuna sezione «Ricariche», benche' il soggetto abbia una ricarica dedotta"), HeaderText.Contains(TEXT("Ricariche")));
	TestFalse(TEXT("e la stima non si disegna da sola, senza una skill sotto il cursore"), HeaderText.Contains(TEXT("Stima")));
	TestFalse(TEXT("ne' l'azione vista usare"), HeaderText.Contains(TEXT("Colpo di prova")));
	TestEqual(TEXT("l'ultima riga e' l'invito"), Header.Lines.Num() > 0 ? Header.Lines.Last().Left(Invitation.Len()) : FString(), Invitation);
	// La testata e' compatta: nome, PV, verso, uno stato, l'equipaggiamento, l'invito.
	TestEqual(TEXT("e non ha altre righe"), Header.Lines.Num(), 6);

	// Controllo positivo: con una skill sotto il cursore le righe `+`/`-` del pezzo che la riguarda ci sono, e il rivelatore le trova.
	FRTInspectSubject WithHover = Subject;
	WithHover.HoveredSkill.ActionId = TEXT("Hero.Test.Shot");
	WithHover.HoveredSkill.Title = FText::FromString(TEXT("Colpo di prova"));
	WithHover.HoveredSkill.Equipment.Add(Subject.Equipment[1]);
	WithHover.HoveredSkill.Deduced.Add(Deduced);
	const FRTInspectView Detailed = URTInspectLibrary::Compose(WithHover);
	TestEqual(TEXT("controllo positivo: coi dettagli c'e' la riga `+` del pezzo"), InspectLinesStartingWith(Detailed.Lines, TEXT("  + ")).Num(), 1);
	TestEqual(TEXT("controllo positivo: e la riga `-`"), InspectLinesStartingWith(Detailed.Lines, TEXT("  - ")).Num(), 1);
	TestEqual(TEXT("controllo positivo: e la stima"), InspectLinesStartingWith(Detailed.Lines, TEXT("Stima")).Num(), 1);
	TestEqual(TEXT("controllo positivo: e non c'e' piu' l'invito"), InspectLinesStartingWith(Detailed.Lines, Invitation).Num(), 0);

	// --- 2. le unita' vere, un eroe dopo l'altro --------------------------------------------------------------------
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }
	int32 Eroi = 0;
	int32 Armati = 0;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		++Eroi;
		FRTCellId C; C.X = 0; C.Y = 0;
		ARTUnit* Unit = InspectEquippedUnit(World, 1, Hero, C);
		if (!TestNotNull(TEXT("l'unita' esiste"), Unit)) { continue; }
		const FString Who = Hero->HeroId.ToString();

		const FRTInspectView View = URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Unit, 0, true));
		const FString Text = InspectText(View);
		const bool bArmed = Unit->EquippedItemIds.Num() > 0;
		Armati += bArmed ? 1 : 0;

		// Nome, PV, verso, [equipaggiamento], invito: l'unita' vera non ha stati. Il conto e' esatto.
		TestEqual(*FString::Printf(TEXT("%s: la testata ha le sole righe attese"), *Who), View.Lines.Num(), 3 + (bArmed ? 1 : 0) + 1);
		TestEqual(*FString::Printf(TEXT("%s: la riga dell'equipaggiamento c'e' se e solo se ci sono pezzi"), *Who),
			InspectLinesStartingWith(View.Lines, TEXT("Equipaggiamento: ")).Num(), bArmed ? 1 : 0);
		for (const FName& Id : Unit->EquippedItemIds)
		{
			const URTEquipmentData* Piece = URTCatalogLibrary::FindEquipment(Id);
			if (!TestNotNull(TEXT("il pezzo registrato esiste"), Piece)) { continue; }
			TestTrue(*FString::Printf(TEXT("%s: `%s` e' nominato"), *Who, *Piece->DisplayName.ToString()), Text.Contains(Piece->DisplayName.ToString()));
			TestFalse(*FString::Printf(TEXT("%s: il vantaggio di `%s` non e' in testata"), *Who, *Piece->DisplayName.ToString()),
				!Piece->Advantage.IsEmpty() && Text.Contains(Piece->Advantage.ToString()));
			TestFalse(*FString::Printf(TEXT("%s: lo svantaggio di `%s` non e' in testata"), *Who, *Piece->DisplayName.ToString()),
				!Piece->Drawback.IsEmpty() && Text.Contains(Piece->Drawback.ToString()));
		}
		TestEqual(*FString::Printf(TEXT("%s: nessuna riga `+`/`-` in testata"), *Who),
			InspectLinesStartingWith(View.Lines, TEXT("  + ")).Num() + InspectLinesStartingWith(View.Lines, TEXT("  - ")).Num(), 0);
		TestFalse(*FString::Printf(TEXT("%s: nessuna sezione «Ricariche»"), *Who), Text.Contains(TEXT("Ricariche")));
		for (int32 Position = 0; Position < Unit->NumAbilities(); ++Position)
		{
			const URTActionData* Action = Unit->GetAbility(Position);
			if (Action == nullptr) { continue; }
			const FString Title = URTHudViewModel::BuildCatalogTooltip(URTHudViewModel::BuildCatalogActionRow(Action, Position, NAME_None)).Title.ToString();
			// Una RIGA col titolo, o «Titolo - frase»: il titolo di una skill concessa da un pezzo e' anche il NOME del pezzo, che sta nella riga
			// dell'equipaggiamento, ed e' legittimo. Quella riga non e' una riga di skill.
			TestEqual(*FString::Printf(TEXT("%s: nessuna riga di skill in testata (`%s`)"), *Who, *Title),
				View.Lines.ContainsByPredicate([&Title](const FString& Line) { return Line == Title || Line.StartsWith(Title + TEXT(" - ")); }) && !Title.IsEmpty(), false);
		}
		TestEqual(*FString::Printf(TEXT("%s: l'ultima riga e' l'invito"), *Who), View.Lines.Last().Left(Invitation.Len()), Invitation);
	}
	TestTrue(TEXT("anti-vacuita': ha esaminato degli eroi, e alcuni con l'equipaggiamento"), Eroi > 0 && Armati > 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * COL CURSORE SU UNA SKILL IL BOX NE MOSTRA I DETTAGLI (`#3748`, decisione d'autore del 2026-10-31): titolo e righe come il tooltip dello
 * STESSO slot, le righe `+`/`-` dei pezzi che la riguardano, e niente di piu'. Un indice vuoto o fuori kit lascia la testata e l'invito.
 *
 * Per ogni eroe col suo loadout e per OGNI posizione di kit, dal soggetto di produzione:
 *  - la testata non cambia: le righe fino all'invito (escluso) sono quelle dell'unita' senza cursore, e al posto dell'invito c'e' il TITOLO della
 *    skill di QUELLA posizione (non di un'altra);
 *  - le righe dopo il titolo sono il tooltip della stessa riga di catalogo (`BuildCatalogTooltip`, via `ComposeTooltipText`): oracolo scritto da
 *    fuori, non riletto dal compositore. Ogni numero del tooltip (etichetta e valore) e' in una riga;
 *  - i pezzi sono quelli che l'ORACOLO POSIZIONALE dice: la variante d'arma sulla posizione 0 (l'attacco base e' sempre l'indice 0), e per una
 *    posizione dopo il kit di base il pezzo che `EquipLoadout` ha accodato proprio li' (i pezzi che concedono un'azione sono accodati nell'ordine
 *    del loadout). Nessun altro pezzo, e nessuna riga `+`/`-` per una skill senza pezzo.
 * I controlli positivi sono i contatori: ci sono posizioni col pezzo d'arma, con un pezzo concesso, e senza pezzo; l'oracolo posizionale ha
 * trovato abbastanza pezzi da non essere vacuo. E il confronto con una unita' gemella, che differisce nella partita e non nel catalogo.
 *
 * ⚠️ **La regola della consegna diceva `GrantedActionId == ActionId`, e non si puo' applicare**: l'azione di un gadget porta l'id del PEZZO
 * (`MakeEquipmentAction`), mentre `GrantedActionId` e' l'azione core, condivisa da piu' pezzi (`Action.Counter`: Contrattacco e Scudo reattivo).
 * L'oracolo posizionale non dipende da nessuna delle due letture: e' la posizione in cui `EquipLoadout` ha messo l'azione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectCardShowsTheHoveredSkillTest,
	"RefactorTactics.UI.InspectCardShowsTheHoveredSkill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectCardShowsTheHoveredSkillTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	int32 Positions = 0;
	int32 WithWeaponPiece = 0;
	int32 WithGrantedPiece = 0;
	int32 WithoutPiece = 0;
	int32 WeaponPiecesNotShownElsewhere = 0;
	int32 TooltipRows = 0;
	int32 LongestCard = 0;     // la scheda piu' lunga (testata + dettagli) fra tutte le posizioni, a titolo informativo
	FString LongestWhere;
	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		FRTCellId C; C.X = 0; C.Y = 0;
		ARTUnit* Unit = InspectSpawn(World, 1, Hero, C);
		if (!TestNotNull(TEXT("l'unita' esiste"), Unit)) { continue; }
		const FString Who = Hero->HeroId.ToString();

		// L'oracolo posizionale: il kit prima del loadout, e i pezzi che concedono un'azione, accodati nell'ordine.
		const int32 BaseKit = Unit->NumAbilities();
		const TArray<FName> Loadout = URTCatalogLibrary::DefaultLoadoutFor(Hero->HeroId);
		if (Loadout.Num() > 0) { Unit->EquipLoadout(Loadout); }
		TArray<const URTEquipmentData*> WeaponPieces;
		TArray<const URTEquipmentData*> GrantingPieces;
		for (const FName& Id : Unit->EquippedItemIds)
		{
			const URTEquipmentData* Piece = URTCatalogLibrary::FindEquipment(Id);
			if (!TestNotNull(TEXT("il pezzo registrato esiste"), Piece)) { continue; }
			(Piece->Slot == ERTEquipmentSlot::WeaponVariant ? WeaponPieces : GrantingPieces).Add(Piece);
		}
		if (!TestEqual(*FString::Printf(TEXT("%s: premessa: l'unita' ha tanti pezzi concessi quante azioni accodate"), *Who),
			GrantingPieces.Num(), Unit->NumAbilities() - BaseKit))
		{
			continue;
		}

		const FRTInspectView Plain = URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Unit, 0, true));
		if (!TestTrue(TEXT("premessa: senza cursore l'ultima riga e' l'invito"), Plain.Lines.Num() > 0 && Plain.Lines.Last().StartsWith(TEXT("Passa su una skill"))))
		{
			continue;
		}
		const int32 HeaderLines = Plain.Lines.Num() - 1;

		for (int32 Position = 0; Position < Unit->NumAbilities(); ++Position)
		{
			const URTActionData* Action = Unit->GetAbility(Position);
			if (Action == nullptr) { continue; }
			++Positions;
			const FString Where = FString::Printf(TEXT("%s, posizione %d"), *Who, Position);

			const FRTInspectSubject Hovered = ARTHUD::BuildInspectSubject(Unit, 0, true, nullptr, Position);
			const FRTInspectView View = URTInspectLibrary::Compose(Hovered);
			if (View.Lines.Num() > LongestCard) { LongestCard = View.Lines.Num(); LongestWhere = Where; }
			if (!TestEqual(*FString::Printf(TEXT("%s: il dettaglio e' dell'azione di quella posizione"), *Where),
				Hovered.HoveredSkill.ActionId, Action->Def.ActionId))
			{
				continue;
			}

			// La testata e' quella di prima, e al posto dell'invito c'e' il titolo.
			if (!TestTrue(*FString::Printf(TEXT("%s: la testata ha le righe di prima, e poi i dettagli"), *Where), View.Lines.Num() > HeaderLines))
			{
				continue;
			}
			for (int32 Line = 0; Line < HeaderLines; ++Line)
			{
				TestEqual(*FString::Printf(TEXT("%s: la riga %d della testata e' quella senza cursore"), *Where, Line), View.Lines[Line], Plain.Lines[Line]);
			}

			// I dettagli: titolo e righe del tooltip della stessa riga di catalogo, scritti dall'oracolo e non dal compositore.
			const FRTAbilityCooldownView CatalogRow = URTHudViewModel::BuildCatalogActionRow(Action, Position, NAME_None);
			const FRTActionTooltipView Tooltip = URTHudViewModel::BuildCatalogTooltip(CatalogRow);
			TArray<FString> ExpectedBlock;
			URTHudViewModel::ComposeTooltipText(Tooltip).ToString().ParseIntoArrayLines(ExpectedBlock, /*bCullEmpty=*/ false);
			TestEqual(*FString::Printf(TEXT("%s: il titolo e' quello del tooltip"), *Where), View.Lines[HeaderLines], Tooltip.Title.ToString());
			for (int32 I = 0; I < ExpectedBlock.Num(); ++I)
			{
				const int32 At = HeaderLines + I;
				TestEqual(*FString::Printf(TEXT("%s: la riga %d dei dettagli e' quella del tooltip"), *Where, I),
					View.Lines.IsValidIndex(At) ? View.Lines[At] : FString(TEXT("<manca>")), ExpectedBlock[I]);
			}
			const FString DetailText = FString::Join(TArray<FString>(View.Lines.GetData() + HeaderLines, View.Lines.Num() - HeaderLines), TEXT("\n"));
			for (const FRTActionTooltipLine& Row : Tooltip.Lines)
			{
				++TooltipRows;
				TestTrue(*FString::Printf(TEXT("%s: la riga «%s» del tooltip ha il suo valore nei dettagli"), *Where, *Row.Label.ToString()),
					DetailText.Contains(Row.Label.ToString()) && DetailText.Contains(Row.Value.ToString()));
			}
			if (!Tooltip.Description.IsEmpty())
			{
				TestTrue(*FString::Printf(TEXT("%s: la frase d'autore e' nei dettagli"), *Where), DetailText.Contains(Tooltip.Description.ToString()));
			}

			// I pezzi che riguardano la skill, dall'oracolo posizionale.
			// Per `EquipmentId`: `FindEquipment` rifa il catalogo a ogni chiamata, e due letture dello stesso pezzo non sono lo stesso puntatore.
			TArray<const URTEquipmentData*> Expected;
			if (Position == 0) { Expected.Append(WeaponPieces); }
			if (Position >= BaseKit && GrantingPieces.IsValidIndex(Position - BaseKit)) { Expected.Add(GrantingPieces[Position - BaseKit]); }
			TSet<FName> ExpectedIds;
			for (const URTEquipmentData* Piece : Expected) { ExpectedIds.Add(Piece->EquipmentId); }
			int32 ExpectedAdvantages = 0;
			int32 ExpectedDrawbacks = 0;
			for (const URTEquipmentData* Piece : Expected)
			{
				ExpectedAdvantages += Piece->Advantage.IsEmpty() ? 0 : 1;
				ExpectedDrawbacks += Piece->Drawback.IsEmpty() ? 0 : 1;
			}
			TestEqual(*FString::Printf(TEXT("%s: tanti pezzi nei dettagli quanti ne dice l'oracolo"), *Where), Hovered.HoveredSkill.Equipment.Num(), Expected.Num());
			TestEqual(*FString::Printf(TEXT("%s: righe `+` quanti i vantaggi dei pezzi attesi"), *Where),
				InspectLinesStartingWith(View.Lines, TEXT("  + ")).Num(), ExpectedAdvantages);
			TestEqual(*FString::Printf(TEXT("%s: righe `-` quanti gli svantaggi dei pezzi attesi"), *Where),
				InspectLinesStartingWith(View.Lines, TEXT("  - ")).Num(), ExpectedDrawbacks);
			for (const FName& Id : Unit->EquippedItemIds)
			{
				const URTEquipmentData* Piece = URTCatalogLibrary::FindEquipment(Id);
				if (Piece == nullptr) { continue; }
				// «Nome (Slot)»: la riga che il pezzo ha solo nei dettagli (in testata c'e' fra i nomi, dopo «Equipaggiamento: »).
				const int32 OwnLines = InspectLinesStartingWith(View.Lines, Piece->DisplayName.ToString() + TEXT(" (")).Num();
				const bool bIsExpected = ExpectedIds.Contains(Piece->EquipmentId);
				TestEqual(*FString::Printf(TEXT("%s: `%s` ha una riga sua se e solo se riguarda questa skill"), *Where, *Piece->DisplayName.ToString()),
					OwnLines, bIsExpected ? 1 : 0);
				if (bIsExpected)
				{
					TestTrue(*FString::Printf(TEXT("%s: `%s` porta il suo vantaggio e il suo svantaggio"), *Where, *Piece->DisplayName.ToString()),
						(Piece->Advantage.IsEmpty() || DetailText.Contains(Piece->Advantage.ToString()))
						&& (Piece->Drawback.IsEmpty() || DetailText.Contains(Piece->Drawback.ToString())));
				}
				else if (Piece->Slot == ERTEquipmentSlot::WeaponVariant)
				{
					++WeaponPiecesNotShownElsewhere;
				}
			}
			if (Expected.Num() == 0)
			{
				++WithoutPiece;
				TestEqual(*FString::Printf(TEXT("%s: una skill senza pezzo non ha righe di equipaggiamento"), *Where),
					InspectLinesStartingWith(View.Lines, TEXT("  + ")).Num() + InspectLinesStartingWith(View.Lines, TEXT("  - ")).Num(), 0);
			}
			WithWeaponPiece += (Position == 0 && WeaponPieces.Num() > 0) ? 1 : 0;
			WithGrantedPiece += (Position >= BaseKit && Expected.Num() > 0) ? 1 : 0;
		}

		// Un indice che non porta un'azione lascia la testata e l'invito: negativo, fuori kit, e una posizione di kit vuota.
		const int32 Hole = Unit->NumAbilities();
		Unit->Abilities.Add(nullptr);
		for (const int32 Bad : { static_cast<int32>(INDEX_NONE), -7, Hole, Hole + 1, Hole + 100 })
		{
			const FRTInspectSubject BadSubject = ARTHUD::BuildInspectSubject(Unit, 0, true, nullptr, Bad);
			TestFalse(*FString::Printf(TEXT("%s: l'indice %d non da' dettagli"), *Who, Bad), BadSubject.HoveredSkill.IsSet());
			TestEqual(*FString::Printf(TEXT("%s: con l'indice %d la scheda e' quella senza cursore, invito compreso"), *Who, Bad),
				InspectText(URTInspectLibrary::Compose(BadSubject)), InspectText(Plain));
		}
	}
	TestTrue(TEXT("anti-vacuita': posizioni di kit, con la variante d'arma, con un pezzo concesso, senza pezzo, e righe di tooltip esaminate"),
		Positions > 0 && WithWeaponPiece > 0 && WithGrantedPiece > 0 && WithoutPiece > 0 && TooltipRows > 0);
	TestTrue(TEXT("anti-vacuita': la variante d'arma e' stata cercata, e NON trovata, fuori dall'attacco base"), WeaponPiecesNotShownElsewhere > 0);
	// Informativo (senza una stima: la stima aggiunge una riga). La finestra della scheda e' di `MaxRighe` righe e, lasciando lo slot, l'hover cade:
	// i dettagli oltre la finestra non si scorrono con la rotella, e il marcatore dice solo quante righe restano. Questo e' quanto arriva a essere lungo il box.
	AddInfo(FString::Printf(TEXT("la scheda piu' lunga (testata + dettagli, senza stima): %d righe, a %s"), LongestCard, *LongestWhere));

	// --- Il gemello che differisce nella PARTITA e non nel catalogo: stessi dettagli ------------------------------------
	// Muiren ha un'azione con una variante (nome e compromesso non vuoti). Il gemello B ha una ricarica vera, un piano, un'azione armata e quella
	// variante attiva; il dock lo dice (premessa), i dettagli no: sono gli stessi di A, riga per riga.
	ARTUnit* A = InspectSpawn(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 0, 0));
	ARTUnit* B = InspectSpawn(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(1, 0, 0));
	if (TestTrue(TEXT("i due gemelli esistono"), A != nullptr && B != nullptr))
	{
		int32 WithVariant = INDEX_NONE;
		for (int32 I = 0; I < B->NumAbilities() && WithVariant == INDEX_NONE; ++I)
		{
			const URTActionData* Candidate = B->GetAbility(I);
			if (Candidate != nullptr && Candidate->CooldownTurns > 0 && Candidate->Variants.Num() > 0
				&& !Candidate->Variants[0].DisplayName.IsEmpty() && !Candidate->Variants[0].Tradeoff.IsEmpty())
			{
				WithVariant = I;
			}
		}
		if (TestNotEqual(TEXT("premessa: un'azione del kit ha una ricarica di catalogo e una variante con nome e compromesso"), WithVariant, static_cast<int32>(INDEX_NONE)))
		{
			const FRTAbilityVariant Variant = B->GetAbility(WithVariant)->Variants[0];
			B->ActiveVariantId = Variant.VariantId;
			B->ConsumeAbility(WithVariant);
			B->PlannedAbilityIndex = WithVariant;
			B->SelectedAbilityIndex = WithVariant;

			// La partita e' DIVERSA, e il dock lo dice: senza questa premessa «stessi dettagli» non proverebbe niente.
			const TArray<FRTAbilityCooldownView> DockA = URTHudViewModel::BuildAbilityCooldowns(A);
			const TArray<FRTAbilityCooldownView> DockB = URTHudViewModel::BuildAbilityCooldowns(B);
			const FRTActionTooltipView DockTipB = URTHudViewModel::BuildActionTooltip(DockB[WithVariant], /*bArmed=*/ true);
			TestTrue(TEXT("premessa: il gemello B ha la ricarica vera in corso, A no"), B->GetAbilityCooldown(WithVariant) > 0 && A->GetAbilityCooldown(WithVariant) == 0);
			TestTrue(TEXT("premessa: il tooltip del dock di B dice il compromesso della variante, e che e' in ricarica"),
				!DockTipB.Variant.IsEmpty() && !DockTipB.Reason.IsEmpty());
			TestTrue(TEXT("premessa: e quello di A no"), URTHudViewModel::BuildActionTooltip(DockA[WithVariant], false).Variant.IsEmpty());

			const FString TextA = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(A, 0, true, nullptr, WithVariant)));
			const FString TextB = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(B, 0, true, nullptr, WithVariant)));
			TestEqual(TEXT("due unita' che differiscono nella partita danno gli stessi dettagli"), TextA, TextB);
			TestFalse(TEXT("il compromesso della variante attiva non e' nei dettagli"), TextB.Contains(Variant.Tradeoff.ToString()));
			TestFalse(TEXT("ne' che l'azione e' in ricarica"), TextB.Contains(DockTipB.Reason.ToString()));
		}
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * LA STIMA NON E' LA PRIMA RIGA CHE LA FINESTRA TAGLIA (`#3748`, review finale del 2026-10-31). La finestra della scheda e' di `MaxRighe` righe
 * e taglia la CODA; uscendo dallo slot l'hover cade, quindi cio' che sta oltre la finestra non si raggiunge con la rotella. La stima della ricarica
 * e' l'informazione che viene dall'osservazione, e le righe `+`/`-` dei pezzi sono catalogo: se uno dei due deve cadere, cadono i pezzi.
 *
 * Il caso vero piu' lungo: Muiren, posizione 0 (l'attacco base, con la variante d'arma e il suo vantaggio e svantaggio), con DUE stati attivi e la
 * stima della ricarica di quell'azione. Prima di questo test l'ordine era titolo, righe del tooltip, pezzi, stima, e quella scheda arrivava a una
 * riga in piu' della finestra: la riga tagliata era la stima.
 *
 * `MaxRighe` si legge dal default del widget e non si riscrive: se qualcuno lo alza, le premesse dicono che il caso non e' piu' al limite, invece di
 * lasciar passare un test che non misura niente. Controlli positivi: la stessa vista senza la stima ha una riga in meno, e la finestra E' tagliata
 * (ha il marcatore e non mostra l'ultima riga), quindi il test non puo' passare per una scheda che entra tutta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTInspectCardKeepsTheEstimateInTheVisibleWindowTest,
	"RefactorTactics.UI.InspectCardKeepsTheEstimateInTheVisibleWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTInspectCardKeepsTheEstimateInTheVisibleWindowTest::RunTest(const FString&)
{
	const int32 MaxLines = GetDefault<URTInspectCardWidget>()->MaxRighe;
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Unit = InspectEquippedUnit(World, 1, URTHeroCatalogLibrary::MakeMuiren(), FRTCellId(0, 0, 0));
	if (!TestNotNull(TEXT("l'unita' esiste"), Unit)) { RTWorldFixtures::DestroyWorld(World); return false; }

	// La scheda piu' lunga: l'attacco base (posizione 0) di un eroe armato, osservato da un'altra squadra, con DUE stati.
	FRTInspectSubject Subject = ARTHUD::BuildInspectSubject(Unit, 0, true, nullptr, /*HoverAbilityIndex=*/ 0);
	if (!TestTrue(TEXT("premessa: la posizione 0 ha i suoi dettagli, e un pezzo che la riguarda (la variante d'arma)"),
		Subject.HoveredSkill.IsSet() && Subject.HoveredSkill.Equipment.Num() > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	for (const FName& Tag : { TAG_Status_Burning.GetTag().GetTagName(), TAG_Status_Slow.GetTag().GetTagName() })
	{
		FRTStatusBadgeView Badge;
		Badge.Tag = Tag;
		if (!TestFalse(TEXT("premessa: lo stato di prova ha un nome nel glossario (se no la sua riga non esiste)"), RTGlossary::NameFor(Badge.Tag).IsEmpty()))
		{
			RTWorldFixtures::DestroyWorld(World);
			return false;
		}
		Subject.Statuses.Add(Badge);
	}

	// La stima: l'azione sotto il cursore, vista usare al turno 2, con una ricarica di catalogo dichiarata a 3.
	FRTInspectDeducedCooldown Estimate;
	if (!TestTrue(TEXT("premessa: la stima di prova esiste"),
		URTInspectLibrary::MakeDeducedCooldown(Subject.HoveredSkill.ActionId, Subject.HoveredSkill.Title, 3, 2, Estimate)))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const FRTInspectView Without = URTInspectLibrary::Compose(Subject);
	Subject.HoveredSkill.Deduced.Add(Estimate);
	const FRTInspectView With = URTInspectLibrary::Compose(Subject);

	// Controlli positivi: la stima e' UNA riga in piu', e la scheda non entra in una finestra.
	TestEqual(TEXT("controllo positivo: senza la stima la vista non ha nessuna riga di stima"), InspectLinesStartingWith(Without.Lines, TEXT("Stima")).Num(), 0);
	TestEqual(TEXT("controllo positivo: con la stima la vista ne ha UNA"), InspectLinesStartingWith(With.Lines, TEXT("Stima")).Num(), 1);
	TestEqual(TEXT("controllo positivo: la stessa vista senza la stima ha una riga in meno"), Without.Lines.Num() + 1, With.Lines.Num());
	if (!TestTrue(*FString::Printf(TEXT("premessa: la scheda eccede la finestra di %d righe (ne ha %d): se no il test non misura il taglio"), MaxLines, With.Lines.Num()),
		With.Lines.Num() >= MaxLines + 1))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const FRTInspectView Window = URTInspectLibrary::VisibleWindow(With, 0, MaxLines);
	TestEqual(TEXT("la finestra e' piena"), Window.Lines.Num(), MaxLines);
	TestFalse(TEXT("controllo positivo: la finestra e' tagliata, l'ultima riga della scheda non c'e'"), Window.Lines.Contains(With.Lines.Last()));
	TestEqual(TEXT("la riga della stima e' nella finestra"), InspectLinesStartingWith(Window.Lines, TEXT("Stima")).Num(), 1);

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
			}
			else
			{
				const int32 Source = DigitsOf(Text);
				Body.Add(Source);
				// La riga del corpo e' la riga di scheda di quel numero: la finestra non riscrive il testo.
				TestEqual(*FString::Printf(TEXT("%s: la riga %d e' quella della scheda"), *Where, Source),
					Text, View.Lines.IsValidIndex(Source) ? View.Lines[Source] : FString());
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
