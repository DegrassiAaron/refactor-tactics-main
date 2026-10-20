#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h"
#include "Tests/RTAbilityFixtures.h"
#include "Tests/RTReactionWindowFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWorld* MakeSuspWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroySuspWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTHexMapActor* SpawnSuspMap(UWorld* World, int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		if (Actor) { Actor->MapAsset = M; }
		return Actor;
	}

	/** Eroe dal CATALOGO, fermo sulla sua cella e non del bot: i piani li scrive il test. */
	ARTUnit* SpawnSuspHero(UWorld* World, FName HeroId, int32 TeamId, const FRTCellId& Cell)
	{
		const URTHeroData* Hero = nullptr;
		for (const URTHeroData* Candidate : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (Candidate && Candidate->HeroId == HeroId) { Hero = Candidate; break; }
		}
		if (!World || !Hero) { return nullptr; }

		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay(); // senza, i cooldown non esistono e ogni abilita' risulta sempre pronta
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		U->PlannedCell = Cell; // fermo salvo che il caso non lo cambi
		return U;
	}

	int32 FindSuspAbilityById(const ARTUnit* Unit, const TCHAR* ActionId)
	{
		if (!Unit) { return INDEX_NONE; }
		for (int32 I = 0; I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* A = Unit->GetAbility(I);
			if (A && A->Def.ActionId == FName(ActionId)) { return I; }
		}
		return INDEX_NONE;
	}
}

/**
 * 🔴 **A RISOLUZIONE SOSPESA SU UNA FINESTRA DI REAZIONE IL MONDO E' IN SOLA LETTURA, ANCHE SENZA PLAYBACK** — `#3837`.
 *
 * D-468 e la spec del puntatore dicono che con una risoluzione in corso nessun input cambia il piano. Il controller
 * lo deriva dal contesto del puntatore, e il contesto guardava `bIsResolving`: la risoluzione SOSPESA su una finestra
 * (l'Overwatch nel Move, qui) lo accende solo se parte il playback parziale, cioe' non con `bEnablePlayback` falso —
 * una proprieta' `EditAnywhere`, la risoluzione istantanea — ne' con la timeline vuota. In quel tratto `Phase` e'
 * `Move`, le azioni di Prep, Dash e Blast sono GIA' consumate e il Cleanup non e' arrivato: un'unita' viva, armata e
 * in ricarica, a mondo scrivibile. Dalla #3835 nessuna guardia di ricarica sull'azione armata la ferma piu'.
 *
 * Cosa succede qui, in ordine:
 *
 *  - **A.** Prima del lock-in l'armo e il click sul bersaglio PIANIFICANO. E' il controllo positivo della sonda: senza,
 *    "il click non pianifica" potrebbe valere per un click che non pianifica mai.
 *  - **B.** Lock-in con un Overwatch che sospende nel Move. Premesse: la finestra e' aperta, la fase e' `Move`,
 *    l'attaccante e' vivo, ancora armato e con la ricarica scritta, e `IsResolving()` dice in quale delle due
 *    configurazioni siamo.
 *  - **C.** Contesto `ResolutionPlayback`, `IsWorldReadOnly()` vero, e un click su un ALTRO nemico (il `Decoy`, cosi'
 *    un piano scritto si distingue da quello consumato) non pianifica niente.
 *
 * ⚠️ **Le due configurazioni girano SEMPRE entrambe.** Con il playback acceso il mondo era gia' in sola lettura (il
 * playback parziale accende `bIsResolving`): e' il controllo positivo che separa il caso del difetto dal caso
 * normale, e resta verde prima e dopo la correzione.
 *
 * ⛔ **Verifica di mutazione**: togliere `IsResolutionSuspended()` da `GetPointerContext()` fa cadere C a playback
 * spento, e solo li'.
 *
 * ⚠️ **Cosa NON copre**: la timeline vuota con il playback acceso. Non ho trovato una sequenza che la produca
 * (`BeginPartialPlayback` la tratta come lo schermo nero che D-355 vuole evitare), e la dichiaro invece di
 * costruirla a mano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSuspendedResolutionIsReadOnlyTest,
	"RefactorTactics.Reactions.ASuspendedResolutionIsReadOnlyForInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSuspendedResolutionIsReadOnlyTest::RunTest(const FString&)
{
	const auto Prova = [this](bool bPlayback) -> bool
	{
		// La configurazione in ogni messaggio: un rosso deve dire QUALE delle due lo ha prodotto.
		const auto E = [bPlayback](const TCHAR* Testo)
		{
			return FString::Printf(TEXT("playback %s - %s"), bPlayback ? TEXT("acceso") : TEXT("spento"), Testo);
		};

		UWorld* World = MakeSuspWorld();
		if (!TestNotNull(*E(TEXT("mondo di prova")), World)) { return false; }
		SpawnSuspMap(World, /*Radius=*/ 10);

		// Squadra 0: chi attacca (Blast) e chi cammina nella zona (Move). Squadra 1: il bersaglio, un altro nemico per
		// la sonda, e chi tiene l'Overwatch. Il Mover e il Watcher hanno le distanze della fixture di
		// `Reactions.WindowSuspendsResolution`, traslate a est perche' non tocchino la linea dell'attacco.
		ARTUnit* Attacker = SpawnSuspHero(World, TEXT("Hero.Aevik"),  0, FRTCellId(-1, 0, 0));
		ARTUnit* Target   = SpawnSuspHero(World, TEXT("Hero.Ivrin"),  1, FRTCellId( 1, 0, 0));
		ARTUnit* Decoy    = SpawnSuspHero(World, TEXT("Hero.Muiren"), 1, FRTCellId(-3, 0, 0));
		ARTUnit* Mover    = SpawnSuspHero(World, TEXT("Hero.Branth"), 0, FRTCellId( 5, 0, 0));
		ARTUnit* Watcher  = SpawnSuspHero(World, TEXT("Hero.Ivrin"),  1, FRTCellId( 8, 0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		if (!TestNotNull(*E(TEXT("attaccante")), Attacker) || !TestNotNull(*E(TEXT("bersaglio")), Target)
			|| !TestNotNull(*E(TEXT("secondo nemico")), Decoy) || !TestNotNull(*E(TEXT("chi cammina")), Mover)
			|| !TestNotNull(*E(TEXT("chi tiene l'Overwatch")), Watcher)
			|| !TestNotNull(*E(TEXT("turn manager")), TM) || !TestNotNull(*E(TEXT("controller")), PC))
		{
			DestroySuspWorld(World);
			return false;
		}
		TM->bEnablePlayback = bPlayback;

		// Un'azione ATTIVA con ricarica di almeno due turni, che il click su un'unita' sa pianificare.
		const int32 ConRicarica = FindSuspAbilityById(Attacker, TEXT("Hero.Aevik.LinearDischarge"));
		if (!TestNotEqual(*E(TEXT("premessa: Aevik ha la scarica lineare")), ConRicarica, static_cast<int32>(INDEX_NONE))
			|| !TestTrue(*E(TEXT("premessa: la sua ricarica e' di almeno due turni")),
				Attacker->GetAbility(ConRicarica)->CooldownTurns >= 2))
		{
			DestroySuspWorld(World);
			return false;
		}

		// L'Overwatch e' l'azione PRINCIPALE di chi lo tiene, e il cono E' il facing.
		Watcher->PlannedAbilityIndex = RTAbilityFixtures::AddCoreAbilityInSlot(Watcher, TEXT("Action.Overwatch"), 3);
		Watcher->Facing = ERTHexDirection::W;
		Mover->PlannedCell = FRTCellId(7, 0, 0);

		// --- A. prima del lock-in l'armo e il click PIANIFICANO: la sonda e' viva ---------------------------
		PC->SelectActorForTest(Attacker);
		PC->SelectAbilityForCurrentForTest(ConRicarica);
		PC->HandleClickOnUnitForTest(Target);
		if (!TestEqual(*E(TEXT("A: pronta, l'azione si arma")), Attacker->SelectedAbilityIndex, ConRicarica)
			|| !TestEqual(*E(TEXT("A: il click sul bersaglio pianifica l'azione")), Attacker->PlannedAbilityIndex, ConRicarica)
			|| !TestTrue(*E(TEXT("A: e il bersaglio e' quello cliccato")), Attacker->PlannedAttackTarget == Target))
		{
			DestroySuspWorld(World);
			return false;
		}

		// --- B. lock-in: l'Overwatch sospende nel Move, dopo che il Blast ha consumato l'azione -------------
		int32 FinestreAperte = 0;
		TM->OnReactionWindowOpened.BindLambda(
			[&FinestreAperte](const FRTReactionWindowView&, int32) { ++FinestreAperte; });
		TM->LockInAndResolve();
		RTReactionWindowFixtures::PompaFinoAllaPresentazione(TM); // #3666: la finestra si presenta al trattenimento

		const bool bPremesse =
			TestTrue(*E(TEXT("premessa: una finestra di reazione si e' aperta")), FinestreAperte > 0)
			&& TestTrue(*E(TEXT("premessa: la risoluzione e' sospesa")), TM->IsResolutionSuspended())
			&& TestEqual(*E(TEXT("premessa: la fase che si e' fermata e' il Move")), TM->GetPhase(), ERTMatchPhase::Move)
			&& TestEqual(*E(TEXT("premessa: IsResolving() dice la configurazione")), TM->IsResolving(), bPlayback)
			&& TestTrue(*E(TEXT("premessa ANTI-VACUITA': l'azione e' stata consumata, quindi e' in ricarica")),
				Attacker->GetAbilityCooldown(ConRicarica) > 0)
			&& TestTrue(*E(TEXT("premessa: l'attaccante e' vivo")), Attacker->IsAlive())
			&& TestEqual(*E(TEXT("premessa: e il Cleanup non e' arrivato, quindi e' ancora armata")),
				Attacker->SelectedAbilityIndex, ConRicarica);
		if (!bPremesse)
		{
			TM->OnReactionWindowOpened.Unbind();
			DestroySuspWorld(World);
			return false;
		}

		// --- C. il mondo e' in sola lettura, e un click non scrive un piano ------------------------------
		TestEqual(*E(TEXT("C: il contesto del puntatore e' la risoluzione in corso")),
			PC->GetPointerContext(), ERTPointerContext::ResolutionPlayback);
		TestTrue(*E(TEXT("C: il mondo e' in sola lettura per l'input")), PC->IsWorldReadOnly());

		PC->HandleClickOnUnitForTest(Decoy);
		TestFalse(*E(TEXT("C: il click su un altro nemico NON pianifica un'azione gia' spesa")),
			Attacker->PlannedAttackTarget == Decoy);

		TM->OnReactionWindowOpened.Unbind();
		DestroySuspWorld(World);
		return true;
	};

	// Le due configurazioni si provano SEMPRE entrambe: una premessa che salta per la prima non deve nascondere l'altra.
	const bool bSenzaPlayback = Prova(false);
	const bool bConPlayback = Prova(true);
	return bSenzaPlayback && bConPlayback;
}

#endif // WITH_DEV_AUTOMATION_TESTS
