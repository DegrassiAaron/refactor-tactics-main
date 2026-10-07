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
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Misc/ScopeExit.h"

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
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeIvrin());
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

	// --- Le voci per azione (#3563, spec «la clip per abilita'» §2.1-§2.2) -------------------------------------

	/** Due eroi sintetici: non toccano il roster nel CDO, e `GenericActionClipIsSharedAcrossHeroes` ne vuole DUE. */
	const FName IdAzioneDiProva(TEXT("Hero.AzioneDiProva"));
	const FName IdAzioneDiProvaBis(TEXT("Hero.AzioneDiProvaBis"));

	const TCHAR* PathRuoloCast    = TEXT("/Game/Prova/RuoloCast.RuoloCast");
	const TCHAR* PathRuoloAttacco = TEXT("/Game/Prova/RuoloAttacco.RuoloAttacco");
	const TCHAR* PathProfilo      = TEXT("/Game/Prova/Profilo.Profilo");
	const TCHAR* PathGenerica     = TEXT("/Game/Prova/Generica.Generica");

	/** Un pool con una sola variante, attiva: la forma di `MakeRuolo` del default, con un path sintetico. */
	FRTAnimRoleClips PoolAzioneDiProva(const TCHAR* Path)
	{
		FRTAnimRoleClips Pool;
		Pool.AddVariant(FName(TEXT("AV_ProvaPool")), FName(TEXT("A")),
			TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path)));
		Pool.MakeActive(FName(TEXT("AV_ProvaPool")));
		return Pool;
	}

	/**
	 * Scrive nel CDO la voce di `Eroe` con i ruoli `Cast` e `Attack` popolati e NESSUNA azione — la stessa
	 * disciplina di `ConfiguraVariante` (`:75-96`): il ripiego sul ruolo e' il controllo positivo di ogni
	 * asserto sotto, e senza un path di ruolo «e' tornato il ruolo» e «e' tornato nulla» sarebbero lo stesso.
	 *
	 * ⚠️ Restituisce un riferimento dentro `ClipsPerHero`: lo si usa SUBITO, prima di aggiungere un altro eroe
	 * (un `Add` successivo puo' riallocare la mappa).
	 */
	FRTHeroPresentationClips& ConfiguraVoceAzione(const FName& Eroe)
	{
		FRTHeroPresentationClips Voce;
		Voce.PerRole.Add(ERTPresentationRole::Cast, PoolAzioneDiProva(PathRuoloCast));
		Voce.PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathRuoloAttacco));
		return GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero.Add(Eroe, Voce);
	}

	/** Come `PulisciVariante` (`:99-102`): il CDO e' stato globale. */
	void PulisciVoceAzione()
	{
		URTUnitAnimInstance* Cdo = GetMutableDefault<URTUnitAnimInstance>();
		Cdo->ClipsPerHero.Remove(IdAzioneDiProva);
		Cdo->ClipsPerHero.Remove(IdAzioneDiProvaBis);
	}

	FString PathDi(const TSoftObjectPtr<UAnimSequenceBase>& Clip) { return Clip.ToSoftObjectPath().ToString(); }

	// --- La mappa di default (spec §2.6) — la SECONDA copia dichiarata ---------------------------------------
	//
	// ⚠️ Ogni ritocco a schermo tocca DUE righe: questa e quella di `MakeActionClips` (spec §6). E' il prezzo di un
	// test che non legge il default per confrontarlo con se stesso.

	/** Una riga di §2.6. `nullptr` = nessuna voce per quel beat: resta la clip di ruolo. */
	struct FRTClipAttesaDefault
	{
		const TCHAR* ActionId;
		const TCHAR* Pack;
		const TCHAR* Cast;
		const TCHAR* Attack;
	};

	const FRTClipAttesaDefault ClipAtteseDefault[] = {
		// Aevik (Gadget)
		{ TEXT("Hero.Aevik.ArcPulse"),          TEXT("Gadget"), nullptr,                          TEXT("LMB_Fire_A") },
		{ TEXT("Hero.Aevik.LinearDischarge"),   TEXT("Gadget"), TEXT("Ability_Q_Target"),         TEXT("LMB_Fire_B") },
		{ TEXT("Hero.Aevik.Overload"),          TEXT("Gadget"), TEXT("Throw_Ready"),              TEXT("LMB_Fire_C") },
		{ TEXT("Hero.Aevik.ConductiveNode"),    TEXT("Gadget"), nullptr,                          nullptr },  // Environment: nessun beat in v0.1
		{ TEXT("Hero.Aevik.ReactiveCapacitor"), TEXT("Gadget"), nullptr,                          nullptr },  // reazione: non si attiva
		// Muiren (Phase)
		{ TEXT("Hero.Muiren.PressureJet"),      TEXT("Phase"),  nullptr,                          TEXT("Primary_Attack_A_Medium") },
		{ TEXT("Hero.Muiren.CircularTide"),     TEXT("Phase"),  TEXT("R_Ability_Intro"),          nullptr },
		{ TEXT("Hero.Muiren.FluidTrail"),       TEXT("Phase"),  TEXT("Ability_E"),                nullptr },
		{ TEXT("Hero.Muiren.TideGuard"),        TEXT("Phase"),  TEXT("Ability_R_Alt"),            nullptr },
		{ TEXT("Hero.Muiren.FlowReaction"),     TEXT("Phase"),  nullptr,                          nullptr },  // inerte: resta il ruolo
		{ TEXT("Hero.Muiren.MistVeil"),         TEXT("Phase"),  nullptr,                          nullptr },  // Environment
		// Branth (Riktor)
		{ TEXT("Hero.Branth.ImpactShot"),       TEXT("Riktor"), nullptr,                          TEXT("PrimaryAttack_A_Slow") },
		{ TEXT("Hero.Branth.KineticPanel"),     TEXT("Riktor"), TEXT("Ability_Lockdown"),         nullptr },
		{ TEXT("Hero.Branth.Reconfigure"),      TEXT("Riktor"), TEXT("Ability_Hook_Pull"),        nullptr },
		{ TEXT("Hero.Branth.Ram"),              TEXT("Riktor"), TEXT("Ability_Hook_Start"),       TEXT("Ability_ShockingPunch") },
		{ TEXT("Hero.Branth.MortarShot"),       TEXT("Riktor"), TEXT("Ability_Hook_Cast"),        TEXT("PrimaryAttack_B_Slow") },
		{ TEXT("Hero.Branth.Interposition"),    TEXT("Riktor"), nullptr,                          nullptr },  // reazione
		// Ivrin (Wraith)
		{ TEXT("Hero.Ivrin.PulseShot"),         TEXT("Wraith"), nullptr,                          TEXT("Fire_A_Fast_V1") },
		{ TEXT("Hero.Ivrin.InterceptShot"),     TEXT("Wraith"), TEXT("Ability_E_Targeting_Start"), nullptr }, // nessun evento Attack: RTTurnManager.cpp:7133-7136
		{ TEXT("Hero.Ivrin.PassingBlade"),      TEXT("Wraith"), TEXT("Ability_R_InMotion"),       TEXT("Ability_Q_Fire_Fwd") },
		{ TEXT("Hero.Ivrin.Feint"),             TEXT("Wraith"), TEXT("Ability_E"),                nullptr },
		{ TEXT("Hero.Ivrin.PhaseGuard"),        TEXT("Wraith"), TEXT("Ability_RMB_Start"),        nullptr },
		{ TEXT("Hero.Ivrin.Deflection"),        TEXT("Wraith"), nullptr,                          nullptr },  // reazione
	};

	/** Un beat di una riga: la voce c'e', e' attiva, punta al path atteso e NON e' quello di ruolo — o non c'e'. */
	void ControllaBeatDiDefault(FAutomationTestBase& Test, const URTUnitAnimInstance& Cdo,
		const FRTHeroPresentationClips& Voce, const FName& HeroId, const FName& ActionId,
		ERTPresentationRole Ruolo, const TCHAR* Pack, const TCHAR* ClipAttesa)
	{
		const FString Chi = FString::Printf(TEXT("%s / %s"), *ActionId.ToString(), *UEnum::GetValueAsString(Ruolo));
		const FRTActionPresentationClips* Azione = Voce.PerAction.Find(ActionId);
		const FRTAnimRoleClips* Pool = Azione ? Azione->PerRole.Find(Ruolo) : nullptr;
		if (ClipAttesa == nullptr)
		{
			Test.TestNull(*FString::Printf(TEXT("%s: nessuna voce di default, resta la clip di ruolo"), *Chi),
				(const void*)Pool);
			return;
		}
		const FRTAnimVariant* Attiva = Pool ? Pool->FindActive() : nullptr;
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s: la voce di default c'e' ed e' attiva"), *Chi), (const void*)Attiva))
		{
			return;
		}
		const FString Atteso = FString::Printf(
			TEXT("/Game/FabAsset/Paragon/Paragon%s/Characters/Heroes/%s/Animations/%s.%s"), Pack, Pack, ClipAttesa, ClipAttesa);
		Test.TestEqual(*FString::Printf(TEXT("%s: il path e' quello della mappa"), *Chi),
			Attiva->Clip.ToSoftObjectPath().ToString(), Atteso);
		Test.TestNotEqual(*FString::Printf(TEXT("%s: ed e' DIVERSO dalla clip di ruolo"), *Chi),
			Attiva->Clip.ToSoftObjectPath().ToString(), Cdo.ActiveClipFor(HeroId, Ruolo).ToSoftObjectPath().ToString());
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

/**
 * Il ruolo `Cast` risolve una clip per ogni eroe del roster — spec «il momento» §2.3.
 *
 * 🔑 **Il PATH, senza caricare**: i pack Paragon non sono versionati, e headless `LoadSynchronous` darebbe
 * `nullptr` su ogni clone appena creato. E' la stessa ragione di `ResolvedAnimationReachesTheUnit`.
 *
 * ⚠️ **In v0.1 la clip e' la STESSA del ruolo `Attack`**, ed e' una decisione (D2): cast e colpo si
 * distinguono per MOMENTO, non per forma. La seconda asserzione lo pinna, cosi' che chi la cambia lo faccia
 * nel catalogo ANIM CORE e non per sbaglio qui.
 *
 * ➕ #3563: le clip per AZIONE stanno sopra il ruolo (`Unit.DefaultActionClipsResolveForEveryKitAbility`); qui si
 * pinna che il ruolo resti il ripiego quando l'azione non ha voce — anche con una generica dichiarata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitCastRoleResolvesAClipForEveryHeroTest,
	"RefactorTactics.Unit.CastRoleResolvesAClipForEveryHero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitCastRoleResolvesAClipForEveryHeroTest::RunTest(const FString&)
{
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	if (!TestNotNull(TEXT("CDO di URTUnitAnimInstance"), Cdo)) { return false; }

	static const TCHAR* Eroi[] = { TEXT("Hero.Aevik"), TEXT("Hero.Muiren"), TEXT("Hero.Branth"), TEXT("Hero.Ivrin") };
	for (const TCHAR* Eroe : Eroi)
	{
		const TSoftObjectPtr<UAnimSequenceBase> Cast = Cdo->ActiveClipFor(FName(Eroe), ERTPresentationRole::Cast);
		const TSoftObjectPtr<UAnimSequenceBase> Attacco = Cdo->ActiveClipFor(FName(Eroe), ERTPresentationRole::Attack);
		TestFalse(*FString::Printf(TEXT("%s: il ruolo Cast ha una clip attiva"), Eroe), Cast.IsNull());
		TestEqual(*FString::Printf(TEXT("%s: in v0.1 e' la stessa del ruolo Attack"), Eroe),
			Cast.ToSoftObjectPath().ToString(), Attacco.ToSoftObjectPath().ToString());
		// ➕ #3563: un'azione senza voce — ne' profilo ne' generica — ripiega sul ruolo, non su nulla.
		TestEqual(*FString::Printf(TEXT("%s: un'azione senza voce ripiega sul ruolo Cast"), Eroe),
			Cdo->ActiveClipFor(FName(Eroe), ERTPresentationRole::Cast, FName(TEXT("Hero.SenzaVoce.Prova")),
				FName(TEXT("Action.BasicAttack"))).ToSoftObjectPath().ToString(),
			Cast.ToSoftObjectPath().ToString());
	}
	return true;
}


/**
 * La clip d'AZIONE vince su quella di ruolo, e senza voce si torna al ruolo — spec «la clip per abilita'» §2.2, D2.
 *
 * 🔑 **Controllo positivo e ripiego nello stesso test**: con la sola voce di ruolo il risultato e' il path di ruolo;
 * aggiunta la voce d'azione, e' il path d'azione. Lo stesso eroe, la stessa chiamata: cambia solo il dato, e il
 * risultato deve cambiare con lui.
 * ✅ Validato per mutazione (1): il ruolo consultato PRIMA delle azioni fa cadere «l'azione vince».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitActionClipWinsOverRoleClipTest,
	"RefactorTactics.Unit.ActionClipWinsOverRoleClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitActionClipWinsOverRoleClipTest::RunTest(const FString&)
{
	ON_SCOPE_EXIT{ PulisciVoceAzione(); };
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	const FName Profilo(TEXT("Hero.AzioneDiProva.Colpo"));
	const FName Generica(TEXT("Action.BasicAttack"));

	ConfiguraVoceAzione(IdAzioneDiProva);
	TestEqual(TEXT("senza voce d'azione: il ripiego e' il ruolo"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Cast, Profilo, Generica)), FString(PathRuoloCast));

	GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero[IdAzioneDiProva]
		.PerAction.FindOrAdd(Profilo).PerRole.Add(ERTPresentationRole::Cast, PoolAzioneDiProva(PathProfilo));
	TestEqual(TEXT("🔴 con la voce d'azione attiva: l'azione vince sul ruolo"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Cast, Profilo, Generica)), FString(PathProfilo));
	TestEqual(TEXT("l'overload a due argomenti resta il ruolo"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Cast)), FString(PathRuoloCast));

	// Una voce d'azione con la variante NON attiva e' «non popolata»: si torna al ruolo (spec §4).
	GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero[IdAzioneDiProva]
		.PerAction[Profilo].PerRole[ERTPresentationRole::Cast].ActiveClipVariant = NAME_None;
	TestEqual(TEXT("voce d'azione senza variante attiva: ripiego sul ruolo"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Cast, Profilo, Generica)), FString(PathRuoloCast));
	return true;
}

/**
 * La generica (`BaseActionId`) e' condivisa fra eroi, e il profilo la batte — spec §2.2, D2.
 * ✅ Validato per mutazione (P2): i due livelli d'azione in ordine inverso fanno cadere «il profilo batte la generica».
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitGenericActionClipIsSharedAcrossHeroesTest,
	"RefactorTactics.Unit.GenericActionClipIsSharedAcrossHeroes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitGenericActionClipIsSharedAcrossHeroesTest::RunTest(const FString&)
{
	ON_SCOPE_EXIT{ PulisciVoceAzione(); };
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	const FName Generica(TEXT("Action.BasicAttack"));
	const FName ProfiloUno(TEXT("Hero.AzioneDiProva.Colpo"));
	const FName ProfiloDue(TEXT("Hero.AzioneDiProvaBis.Colpo"));

	// ⚠️ Un eroe per volta: il riferimento di `ConfiguraVoceAzione` non sopravvive all'`Add` del secondo.
	ConfiguraVoceAzione(IdAzioneDiProva).PerAction.FindOrAdd(Generica)
		.PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathGenerica));
	ConfiguraVoceAzione(IdAzioneDiProvaBis).PerAction.FindOrAdd(Generica)
		.PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathGenerica));

	TestEqual(TEXT("primo eroe, profilo senza voce: la generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, ProfiloUno, Generica)), FString(PathGenerica));
	TestEqual(TEXT("secondo eroe, profilo senza voce: la STESSA generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProvaBis, ERTPresentationRole::Attack, ProfiloDue, Generica)), FString(PathGenerica));

	// 🔴 Il profilo batte la generica: e' il primo livello, la generica il secondo.
	GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero[IdAzioneDiProva]
		.PerAction.FindOrAdd(ProfiloUno).PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathProfilo));
	TestEqual(TEXT("🔴 con entrambe popolate il profilo batte la generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, ProfiloUno, Generica)), FString(PathProfilo));
	TestEqual(TEXT("e il secondo eroe, senza profilo, resta sulla generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProvaBis, ERTPresentationRole::Attack, ProfiloDue, Generica)), FString(PathGenerica));
	return true;
}

/**
 * `BaseActionId` si legge dall'EVENTO e non si indovina — `Ruling` di §2.2, `Turn/RTResolvedEvent.h:393-397`.
 *
 * 🔑 Con la generica popolata e nessuna voce per il profilo: passando `Action.BasicAttack` si risolve la generica
 * (controllo positivo); passando `NAME_None` si risolve il RUOLO. Un `ActiveClipFor` che derivasse la generica dal
 * profilo darebbe la generica anche nel secondo caso.
 * ✅ Validato per mutazione (P4): la generica derivata quando manca fa cadere il secondo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitBaseActionIdIsNeverDerivedTest,
	"RefactorTactics.Unit.BaseActionIdIsNeverDerived",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitBaseActionIdIsNeverDerivedTest::RunTest(const FString&)
{
	ON_SCOPE_EXIT{ PulisciVoceAzione(); };
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	const FName Generica(TEXT("Action.BasicAttack"));
	const FName Profilo(TEXT("Hero.AzioneDiProva.Colpo"));

	ConfiguraVoceAzione(IdAzioneDiProva).PerAction.FindOrAdd(Generica)
		.PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathGenerica));

	TestEqual(TEXT("controllo positivo: con BaseActionId dichiarato si risolve la generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, Profilo, Generica)), FString(PathGenerica));
	TestEqual(TEXT("🔴 con BaseActionId vuoto si salta il livello: il RUOLO, non la generica indovinata"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, Profilo, NAME_None)), FString(PathRuoloAttacco));
	TestEqual(TEXT("e con entrambi vuoti, il ruolo"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, NAME_None, NAME_None)), FString(PathRuoloAttacco));
	return true;
}

/**
 * Review Focus (a): una voce per l'azione che NON ha il ruolo richiesto non ferma la ricerca.
 *
 * 🔴 Il caso reale: `Hero.Muiren.TideGuard` ha solo un beat `Cast` (spec §2.6). Un `Attack` con quella chiave — o
 * qualunque ruolo assente dalla voce — deve passare alla generica, poi al ruolo; mai restituire nulla perche' «la
 * voce dell'azione c'era».
 * ✅ Validato per mutazione (P1): `return` incondizionato quando la voce dell'azione esiste → cade il primo asserto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitActionEntryWithoutTheRoleFallsBackTest,
	"RefactorTactics.Unit.ActionEntryWithoutTheRoleFallsBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitActionEntryWithoutTheRoleFallsBackTest::RunTest(const FString&)
{
	ON_SCOPE_EXIT{ PulisciVoceAzione(); };
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	const FName Profilo(TEXT("Hero.AzioneDiProva.Scudo"));
	const FName Generica(TEXT("Action.BasicAttack"));

	// Il profilo ha SOLO `Cast`.
	ConfiguraVoceAzione(IdAzioneDiProva).PerAction.FindOrAdd(Profilo)
		.PerRole.Add(ERTPresentationRole::Cast, PoolAzioneDiProva(PathProfilo));

	const TSoftObjectPtr<UAnimSequenceBase> Attacco =
		Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, Profilo, NAME_None);
	TestFalse(TEXT("🔴 voce solo Cast, richiesta Attack: NON nulla"), Attacco.IsNull());
	TestEqual(TEXT("e' la clip di ruolo Attack"), PathDi(Attacco), FString(PathRuoloAttacco));

	// Con la generica popolata per Attack, il ripiego si ferma li', prima del ruolo.
	GetMutableDefault<URTUnitAnimInstance>()->ClipsPerHero[IdAzioneDiProva]
		.PerAction.FindOrAdd(Generica).PerRole.Add(ERTPresentationRole::Attack, PoolAzioneDiProva(PathGenerica));
	TestEqual(TEXT("voce solo Cast, richiesta Attack, generica popolata: la generica"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Attack, Profilo, Generica)), FString(PathGenerica));
	TestEqual(TEXT("controllo positivo: il Cast dello stesso profilo e' la voce d'azione"),
		PathDi(Cdo->ActiveClipFor(IdAzioneDiProva, ERTPresentationRole::Cast, Profilo, Generica)), FString(PathProfilo));
	return true;
}

/**
 * Il default C++ porta la mappa abilita'→clip della spec §2.6 per OGNI abilita' dei kit — e nient'altro.
 *
 * 🔑 **La lista attesa e' una FUNZIONE del catalogo eroi**, non un elenco letterale: si parte da
 * `GetHeroRoster()` e ogni abilita' d'eroe (profilo `Hero.<Eroe>.*`, D-033) deve avere una riga nella tabella
 * qui sopra — con una clip o dichiarata senza voce. Un'abilita' nuova senza riga e' ROSSO: la sua clip e' una
 * decisione dell'autore, non un silenzio.
 * 🔴 **E nel verso opposto** (Review Focus (d)): ogni chiave di `PerAction` nel CDO deve essere un'abilita' di
 * QUELL'eroe nel catalogo, e ogni riga della tabella deve essere stata visitata. Un'abilita' tolta dal catalogo
 * mentre resta nella mappa di `MakeActionClips` cade qui, col suo nome.
 * ✅ Validato per mutazione (2) — una riga tolta da `MakeActionClips` — e (P3) — una riga per un'abilita'
 * inesistente aggiunta a `MakeActionClips`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitDefaultActionClipsResolveForEveryKitAbilityTest,
	"RefactorTactics.Unit.DefaultActionClipsResolveForEveryKitAbility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitDefaultActionClipsResolveForEveryKitAbilityTest::RunTest(const FString&)
{
	const URTUnitAnimInstance* Cdo = GetDefault<URTUnitAnimInstance>();
	if (!TestNotNull(TEXT("CDO di URTUnitAnimInstance"), Cdo)) { return false; }

	TSet<FString> RigheVisitate;   // solo Contains/Add: nessun asserto dipende dall'ordine
	int32 AbilitaConfrontate = 0;
	for (const URTHeroData* Eroe : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!TestNotNull(TEXT("eroe del roster"), Eroe)) { continue; }
		const FString NomeEroe = Eroe->HeroId.ToString();
		const FString Prefisso = NomeEroe + TEXT(".");
		const FRTHeroPresentationClips* Voce = Cdo->FindClipsFor(Eroe->HeroId);
		if (!TestNotNull(*FString::Printf(TEXT("%s ha una voce nel CDO"), *NomeEroe), (const void*)Voce)) { continue; }

		TSet<FName> AzioniDellEroe;
		for (const URTActionData* Azione : Eroe->Actions)
		{
			if (Azione == nullptr) { continue; }
			const FName Id = Azione->Def.ActionId;
			AzioniDellEroe.Add(Id);
			if (!Id.ToString().StartsWith(Prefisso)) { continue; }   // solo i profili d'eroe
			++AbilitaConfrontate;

			const FRTClipAttesaDefault* Riga = nullptr;
			for (const FRTClipAttesaDefault& R : ClipAtteseDefault)
			{
				if (Id == FName(R.ActionId)) { Riga = &R; break; }
			}
			if (!TestNotNull(*FString::Printf(
					TEXT("%s: l'abilita' del catalogo ha una riga nella mappa di §2.6 (decidi la clip, o dichiarala senza voce)"),
					*Id.ToString()), (const void*)Riga))
			{
				continue;
			}
			RigheVisitate.Add(Riga->ActionId);
			ControllaBeatDiDefault(*this, *Cdo, *Voce, Eroe->HeroId, Id, ERTPresentationRole::Cast, Riga->Pack, Riga->Cast);
			ControllaBeatDiDefault(*this, *Cdo, *Voce, Eroe->HeroId, Id, ERTPresentationRole::Attack, Riga->Pack, Riga->Attack);
		}

		// 🔴 Il verso opposto: il default non nomina abilita' che il catalogo non ha, e solo i ruoli di D3.
		for (const TPair<FName, FRTActionPresentationClips>& Azione : Voce->PerAction)
		{
			TestTrue(*FString::Printf(TEXT("%s: la voce di default '%s' e' un'abilita' che il catalogo eroi ha"),
				*NomeEroe, *Azione.Key.ToString()), AzioniDellEroe.Contains(Azione.Key));
			for (const TPair<ERTPresentationRole, FRTAnimRoleClips>& Ruolo : Azione.Value.PerRole)
			{
				TestTrue(*FString::Printf(TEXT("%s / %s: solo Cast e Attack conoscono l'azione (D3)"),
					*Azione.Key.ToString(), *UEnum::GetValueAsString(Ruolo.Key)),
					Ruolo.Key == ERTPresentationRole::Cast || Ruolo.Key == ERTPresentationRole::Attack);
			}
		}
	}

	TestTrue(TEXT("⛔ premessa: il catalogo eroi ha abilita' da confrontare"), AbilitaConfrontate > 0);
	for (const FRTClipAttesaDefault& R : ClipAtteseDefault)
	{
		TestTrue(*FString::Printf(TEXT("%s: la riga della mappa nomina un'abilita' che il catalogo eroi ha ancora"),
			R.ActionId), RigheVisitate.Contains(FString(R.ActionId)));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
