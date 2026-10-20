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
		static const TArray<URTHeroData*> Roster = URTHeroCatalogLibrary::GetHeroRoster();
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
		FName(TEXT("RTActionTooltipView")), FName(TEXT("RTActionTooltipLine")), FName(TEXT("RTInspectEquipmentLine")) };

	int32 Examined = 0;
	const TArray<const UStruct*> Types = { FRTInspectSubject::StaticStruct(), FRTInspectEquipmentLine::StaticStruct(),
		FRTInspectView::StaticStruct() };
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

	// B differisce da A solo nel piano e nelle ricariche vere.
	B->PlannedAbilityIndex = 1;
	B->PlannedReactionAbility = 2;
	B->PlannedDashAbility = 3;
	B->bAttackTargetsCell = true;

	const FString TextA = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(A, 0, true)));
	const FString TextB = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(B, 0, true)));
	TestEqual(TEXT("due nemici che differiscono nel piano danno la stessa scheda"), TextA, TextB);
	TestTrue(TEXT("e la scheda ha un kit (non e' vuota per costruzione)"), TextA.Contains(TEXT("Kit")));

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

	// Un eroe con un loadout di default non vuoto (Aevik e Ivrin ne sono privi).
	const URTHeroData* Hero = nullptr;
	TArray<FName> Loadout;
	for (const URTHeroData* Candidate : URTHeroCatalogLibrary::GetHeroRoster())
	{
		Loadout = URTCatalogLibrary::DefaultLoadoutFor(Candidate->HeroId);
		if (Loadout.Num() > 0) { Hero = Candidate; break; }
	}
	if (!TestNotNull(TEXT("esiste un eroe con equipaggiamento"), Hero))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	FRTCellId C; C.X = 0; C.Y = 0;
	ARTUnit* Bare = InspectSpawn(World, 1, Hero, C);
	ARTUnit* Armed = InspectSpawn(World, 1, Hero, C);
	Armed->EquipLoadout(Loadout);

	TestEqual(TEXT("senza EquipLoadout nessun pezzo registrato"), Bare->EquippedItemIds.Num(), 0);
	TestTrue(TEXT("dopo EquipLoadout i pezzi applicati sono registrati"), Armed->EquippedItemIds.Num() > 0);
	TestFalse(TEXT("un id inesistente non viene registrato"), [&]()
		{
			ARTUnit* U = InspectSpawn(World, 1, Hero, C);
			U->EquipLoadout({ FName(TEXT("Equip.Inesistente")) });
			return U->EquippedItemIds.Num() > 0;
		}());

	const FString TextBare = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Bare, 0, true)));
	const FString TextArmed = InspectText(URTInspectLibrary::Compose(ARTHUD::BuildInspectSubject(Armed, 0, true)));
	TestFalse(TEXT("senza equipaggiamento la sezione non c'e'"), TextBare.Contains(TEXT("Equipaggiamento")));
	TestTrue(TEXT("con equipaggiamento la sezione c'e'"), TextArmed.Contains(TEXT("Equipaggiamento")));
	for (const FName& Id : Armed->EquippedItemIds)
	{
		const URTEquipmentData* Piece = URTCatalogLibrary::FindEquipment(Id);
		TestTrue(TEXT("ogni pezzo e' nominato col nome del catalogo"),
			Piece && !Piece->DisplayName.IsEmpty() && TextArmed.Contains(Piece->DisplayName.ToString()));
	}

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

#endif // WITH_DEV_AUTOMATION_TESTS
