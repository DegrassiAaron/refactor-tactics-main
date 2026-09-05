#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Core/RTTypes.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "Unit/RTUnitAnimInstance.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il canale della presentazione discreta (#2448): la clip la sceglie il **C++**, non il Blueprint.
 *
 * 🔴 **Che cosa questi test possono e NON possono dimostrare, e va detto prima.**
 * Negli scenari headless nessun `AnimInstance` viene mai istanziato — `ApplyUnitAnimClass()` esce senza
 * fare nulla quando l'unita' non ha una skeletal, e `grep -n "SkeletalMesh" ScenarioHarness/*.cpp` da'
 * **0**. Quindi qui **non si osserva nessuna animazione**: si osserva la **risoluzione**, cioe' il PATH
 * che `PlayPresentationRole` suonerebbe. Che poi si veda a schermo e' `PIE-AS4b`, e il suo oracolo e' una
 * persona.
 *
 * ⚠️ E' la ragione per cui `ResolvedClipPathFor` esiste separata dal suo uso: i pack Paragon non sono
 * versionati, quindi `LoadSynchronous` da' `nullptr` su ogni clone appena creato e un test non
 * distinguerebbe «la clip giusta non si carica» da «punto alla clip sbagliata». Il **path** c'e' sempre.
 */
namespace
{
	/** Id sintetico: non tocca le quattro voci del roster nel CDO, cosi' nessun altro test ne risente. */
	const FName IdDiProva(TEXT("Hero.CanaleDiProva"));

	UWorld* MakeChannelWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyChannelWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTUnit* SpawnChannelUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/**
	 * Scrive nel CDO una voce per `IdDiProva` con **tre varianti** di `Attack`, e attiva la n-esima.
	 *
	 * 🔑 **E' il meccanismo del controllo positivo**: senza poter cambiare *cosa* si risolve, l'asserzione
	 * di invarianza sul TurnLog sarebbe vera per costruzione — tre configurazioni che non cambiano niente
	 * danno lo stesso risultato sempre.
	 */
	void ConfiguraVariante(int32 Indice)
	{
		URTUnitAnimInstance* Cdo = GetMutableDefault<URTUnitAnimInstance>();
		FRTHeroPresentationClips Voce;
		FRTAnimRoleClips Ruolo;

		static const TCHAR* Path[] = {
			TEXT("/Game/Prova/AnimA.AnimA"),
			TEXT("/Game/Prova/AnimB.AnimB"),
			TEXT("/Game/Prova/AnimC.AnimC")
		};
		static const TCHAR* Ident[] = { TEXT("AV_ProvaA"), TEXT("AV_ProvaB"), TEXT("AV_ProvaC") };

		for (int32 I = 0; I < 3; ++I)
		{
			Ruolo.AddVariant(FName(Ident[I]), FName(Ident[I]),
				TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path[I])));
		}
		Ruolo.MakeActive(FName(Ident[Indice]));
		Voce.PerRole.Add(ERTPresentationRole::Attack, Ruolo);
		Cdo->ClipsPerHero.Add(IdDiProva, Voce);
	}

	/** Toglie la voce sintetica: il CDO e' stato globale, e un test che lo sporca avvelena i vicini. */
	void PulisciVariante()
	{
		GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero.Remove(IdDiProva);
	}
}

/**
 * **L'animazione che arriva all'unita' e' quella che il resolver ha scelto**, su tre configurazioni.
 *
 * Il ruolo e' `Attack`, e la clip che lo riempie sui pack reali si chiama `Cast`: sono due tassonomie
 * diverse, e l'enum ha **anche** un ruolo `Cast` che non ha consumatore. Qui si usano path sintetici
 * proprio per non far dipendere l'asserzione da quella coincidenza di nomi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimChannelResolvedReachesUnitTest,
	"RefactorTactics.Anim.Channel.ResolvedAnimationReachesTheUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimChannelResolvedReachesUnitTest::RunTest(const FString&)
{
	UWorld* World = MakeChannelWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* U = SpawnChannelUnit(World, 0, FRTCellId(0, 0));
	if (!U) { DestroyChannelWorld(World); PulisciVariante(); return false; }
	U->HeroId = IdDiProva;

	if (!TestNotNull(TEXT("premessa: l'unita' ha una classe di animazione da cui leggere"),
		U->UnitAnimClass.Get()))
	{
		DestroyChannelWorld(World); return false;
	}

	static const TCHAR* Atteso[] = {
		TEXT("/Game/Prova/AnimA.AnimA"),
		TEXT("/Game/Prova/AnimB.AnimB"),
		TEXT("/Game/Prova/AnimC.AnimC")
	};

	for (int32 I = 0; I < 3; ++I)
	{
		ConfiguraVariante(I);
		const FString Risolto = U->ResolvedClipPathFor(ERTPresentationRole::Attack).ToSoftObjectPath().ToString();
		TestEqual(*FString::Printf(TEXT("configurazione %d: arriva la clip attiva"), I), Risolto, FString(Atteso[I]));
	}

	PulisciVariante();
	DestroyChannelWorld(World);
	return true;
}

/**
 * **Un Blueprint che non implementa l'evento non produce ne' crash ne' cambio di esito logico.**
 *
 * ⚠️ `ARTUnit::StaticClass()` non implementa i tre `BlueprintImplementableEvent`: e' esattamente il caso
 * di questo test, e **anche lo stato reale dei quattro `BP_Unit_*`** al 2026-09-05.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimChannelMissingImplTest,
	"RefactorTactics.Anim.Channel.MissingImplementationIsHarmless",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimChannelMissingImplTest::RunTest(const FString&)
{
	UWorld* World = MakeChannelWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* U = SpawnChannelUnit(World, 0, FRTCellId(0, 0));
	if (!U) { DestroyChannelWorld(World); return false; }
	U->HeroId = IdDiProva;
	ConfiguraVariante(0);

	const int32 VitaPrima = U->Health;
	const int32 ScudoPrima = U->Shield;
	const FRTCellId CellaPrima = U->Cell;

	// Nessuna assertion di "non crasha": se crasha, il test non arriva alla riga dopo.
	U->PlayPresentationRole(ERTPresentationRole::Attack);
	U->PlayPresentationRole(ERTPresentationRole::Hit);
	U->PlayPresentationRole(ERTPresentationRole::Death);

	TestEqual(TEXT("la vita non e' cambiata"), U->Health, VitaPrima);
	TestEqual(TEXT("lo scudo non e' cambiato"), U->Shield, ScudoPrima);
	TestTrue(TEXT("la cella non e' cambiata"), U->Cell == CellaPrima);
	TestTrue(TEXT("l'unita' e' ancora viva e valida"), IsValid(U) && U->IsAlive());

	PulisciVariante();
	DestroyChannelWorld(World);
	return true;
}

/**
 * **Un'animazione che non risolve non produce ne' crash ne' cambio di esito logico.**
 *
 * Due modi di «non risolvere», e vanno provati entrambi perche' passano da rami diversi:
 * l'eroe **senza voce** nel CDO (`ActiveClipFor` esce al primo controllo) e il path che **non carica**
 * (`LoadSynchronous` da' `nullptr` — ed e' il caso normale su un checkout senza i pack Paragon).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimChannelUnresolvedTest,
	"RefactorTactics.Anim.Channel.UnresolvedAssetIsHarmless",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimChannelUnresolvedTest::RunTest(const FString&)
{
	UWorld* World = MakeChannelWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTUnit* U = SpawnChannelUnit(World, 0, FRTCellId(0, 0));
	if (!U) { DestroyChannelWorld(World); return false; }

	// (a) eroe senza voce nel CDO
	U->HeroId = FName(TEXT("Hero.NonEsiste"));
	TestTrue(TEXT("eroe fuori catalogo: nessuna clip, e non e' un errore"),
		U->ResolvedClipPathFor(ERTPresentationRole::Attack).IsNull());
	U->PlayPresentationRole(ERTPresentationRole::Attack);

	// (b) path dichiarato che non carica — il caso NORMALE senza i pack versionati
	U->HeroId = IdDiProva;
	ConfiguraVariante(0);
	const TSoftObjectPtr<UAnimSequenceBase> Path = U->ResolvedClipPathFor(ERTPresentationRole::Attack);
	if (!TestFalse(TEXT("premessa: il path c'e'"), Path.IsNull()))
	{
		PulisciVariante(); DestroyChannelWorld(World); return false;
	}
	TestNull(TEXT("premessa: e non carica (asset sintetico)"), Path.LoadSynchronous());

	const int32 VitaPrima = U->Health;
	U->PlayPresentationRole(ERTPresentationRole::Attack);
	TestEqual(TEXT("la vita non e' cambiata"), U->Health, VitaPrima);
	TestTrue(TEXT("l'unita' e' ancora valida"), IsValid(U));

	PulisciVariante();
	DestroyChannelWorld(World);
	return true;
}

/**
 * 🔴 **L'esito della simulazione non cambia fra le varianti — e il controllo positivo e' DENTRO il test.**
 *
 * ⚠️ **Senza la prima meta' questo test sarebbe vacuo.** Headless nessun `AnimInstance` esiste: tre
 * configurazioni che non cambiano niente danno lo stesso TurnLog **per costruzione**, non per correttezza.
 * E' la classe di difetto di #1763, e l'acceptance di #2448 la vieta esplicitamente.
 *
 * Quindi si dimostra, in quest'ordine:
 *   1. che le tre configurazioni **cambiano davvero** cio' che arriva alla presentazione (path diversi);
 *   2. che **non cambiano** lo stato logico ne' il combat log.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimChannelOutcomeUnchangedTest,
	"RefactorTactics.Anim.Channel.SimulationOutcomeIsUnchangedAcrossVariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimChannelOutcomeUnchangedTest::RunTest(const FString&)
{
	TArray<FString> PathPerVariante;
	TArray<int32> VitaFinale;
	TArray<int32> RigheDiLog;

	for (int32 I = 0; I < 3; ++I)
	{
		UWorld* World = MakeChannelWorld();
		if (!TestNotNull(TEXT("world di prova"), World)) { return false; }
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 6);
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		MapActor->MapAsset = M;

		ARTUnit* Attaccante = SpawnChannelUnit(World, 0, FRTCellId(0, 0));
		ARTUnit* Bersaglio = SpawnChannelUnit(World, 1, FRTCellId(2, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!TM || !Attaccante || !Bersaglio) { DestroyChannelWorld(World); PulisciVariante(); return false; }

		// L'attaccante legge la voce sintetica: e' lui che cambia configurazione fra un giro e l'altro.
		Attaccante->HeroId = IdDiProva;
		ConfiguraVariante(I);
		PathPerVariante.Add(Attaccante->ResolvedClipPathFor(ERTPresentationRole::Attack)
			.ToSoftObjectPath().ToString());

		Attaccante->PlannedAbilityIndex = 0;
		Attaccante->PlannedAttackTarget = Bersaglio;

		TM->LockInAndResolve();
		for (int32 T = 0; T < 400 && TM->IsResolving(); ++T) { TM->Tick(0.05f); }

		VitaFinale.Add(Bersaglio->Health + Bersaglio->Shield);
		RigheDiLog.Add(TM->GetRecentEventsForTeam(0).Num());

		PulisciVariante();
		DestroyChannelWorld(World);
	}

	// --- 1. CONTROLLO POSITIVO: le tre configurazioni cambiano cio' che arriva alla presentazione -----
	if (!TestNotEqual(TEXT("controllo positivo: variante A e B risolvono clip DIVERSE"),
		PathPerVariante[0], PathPerVariante[1]))
	{
		return false; // senza questo, l'invarianza qui sotto sarebbe vera per costruzione
	}
	if (!TestNotEqual(TEXT("controllo positivo: variante B e C risolvono clip DIVERSE"),
		PathPerVariante[1], PathPerVariante[2]))
	{
		return false;
	}

	// --- 2. E NON cambiano l'esito logico ------------------------------------------------------------
	TestEqual(TEXT("la vita del bersaglio e' identica fra A e B"), VitaFinale[0], VitaFinale[1]);
	TestEqual(TEXT("la vita del bersaglio e' identica fra B e C"), VitaFinale[1], VitaFinale[2]);
	TestEqual(TEXT("il combat log ha lo stesso numero di righe fra A e B"), RigheDiLog[0], RigheDiLog[1]);
	TestEqual(TEXT("il combat log ha lo stesso numero di righe fra B e C"), RigheDiLog[1], RigheDiLog[2]);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
