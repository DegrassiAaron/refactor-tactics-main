#include "Misc/AutomationTest.h"
#include "Ability/RTActionData.h"
#include "Ability/RTActionDef.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Combat/RTCombatLibrary.h"
#include "Core/RTGameplayTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWorld* MakeDeadWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyDeadWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/** Eroe dal CATALOGO, fermo sulla sua cella e non del bot: i piani li scrive il test. */
	ARTUnit* SpawnDeadHero(UWorld* World, FName HeroId, int32 TeamId, const FRTCellId& Cell)
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

	int32 FindDeadAbilityById(const ARTUnit* Unit, const TCHAR* ActionId)
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
 * 🔴 **UN'UNITA' UCCISA NEL CLEANUP RESTA ARMATA E IN RICARICA FINO ALLA DISTRUZIONE, E L'INPUT NON LA RAGGIUNGE** — `#3838`.
 *
 * La revisione di `#3833` l'ha misurata e le note di `D-531` e di `SelectAbilityForCurrent` ci si appoggiano: il
 * ciclo del Cleanup salta chi e' gia' morto e fa `continue` per chi muore proprio li' (il Burning), quindi non
 * fa scorrere la ricarica e non disarma. E' la ragione per cui `MostraLaPortata` conserva `IsAlive()`. Nessun test
 * la pinnava: se il Cleanup cambiasse, o `ConcludeTurn` si spostasse oltre un punto con l'input aperto, la nota
 * diventerebbe falsa senza un rosso.
 *
 * Cosa dimostra, in ordine:
 *
 *  - **A.** All'istante in cui il Cleanup si chiude (`OnPhaseClosed`) l'unita' e' morta, ancora armata e con la
 *    ricarica scritta. Premessa anti-vacuita': senza, "l'input non la raggiunge" varrebbe per un'unita' qualunque.
 *  - **B.** **Con il playback** l'intervallo e' lungo (tutto il playback): il mondo e' in sola lettura, tasto,
 *    slot del dock e click non cambiano niente, e a playback finito l'unita' e' distrutta e la selezione vuota.
 *  - **C.** **Senza playback** non c'e' nessun intervallo: tornato `LockInAndResolve` l'unita' e' gia' distrutta, e
 *    tutto e' avvenuto in un solo passaggio sincrono.
 *
 * ⚠️ **Le due configurazioni girano SEMPRE entrambe.** `bEnablePlayback` e' `EditAnywhere`, e le note dicevano
 * "nello stesso passaggio sincrono" di entrambe: era vero di una sola.
 *
 * ⛔ **Verifica di mutazione**: togliere `DestroyDefeatedUnits` da `ConcludeTurn` fa cadere B e C; togliere da
 * `GetPointerContext()` la risposta `ResolutionPlayback` fa cadere B.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitKilledInTheCleanupTest,
	"RefactorTactics.Turn.AUnitKilledInTheCleanupStaysArmedUntilDestroyedOutOfReachOfInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitKilledInTheCleanupTest::RunTest(const FString&)
{
	const auto Prova = [this](bool bPlayback) -> bool
	{
		// La configurazione in ogni messaggio: un rosso deve dire QUALE delle due lo ha prodotto.
		const auto E = [bPlayback](const TCHAR* Testo)
		{
			return FString::Printf(TEXT("playback %s - %s"), bPlayback ? TEXT("acceso") : TEXT("spento"), Testo);
		};

		UWorld* World = MakeDeadWorld();
		if (!TestNotNull(*E(TEXT("mondo di prova")), World)) { return false; }
		URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 6);
		ARTHexMapActor* MapActor = World->SpawnActor<ARTHexMapActor>();
		if (MapActor) { MapActor->MapAsset = Arena; }

		ARTUnit* Doomed = SpawnDeadHero(World, TEXT("Hero.Aevik"),  0, FRTCellId(-1, 0, 0));
		ARTUnit* Target = SpawnDeadHero(World, TEXT("Hero.Ivrin"),  1, FRTCellId( 1, 0, 0));
		ARTUnit* Decoy  = SpawnDeadHero(World, TEXT("Hero.Muiren"), 1, FRTCellId(-3, 0, 0));
		// Un alleato lontano e fermo: senza, la morte di chi brucia chiuderebbe la partita (squadra 0 azzerata) e il turno
		// concluderebbe per un'altra strada.
		ARTUnit* Ally   = SpawnDeadHero(World, TEXT("Hero.Branth"), 0, FRTCellId( 5, 0, 0));
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
		if (!TestNotNull(*E(TEXT("chi muore")), Doomed) || !TestNotNull(*E(TEXT("bersaglio")), Target)
			|| !TestNotNull(*E(TEXT("secondo nemico")), Decoy) || !TestNotNull(*E(TEXT("alleato")), Ally)
			|| !TestNotNull(*E(TEXT("turn manager")), TM) || !TestNotNull(*E(TEXT("controller")), PC))
		{
			DestroyDeadWorld(World);
			return false;
		}
		TM->bEnablePlayback = bPlayback;

		const int32 ConRicarica = FindDeadAbilityById(Doomed, TEXT("Hero.Aevik.LinearDischarge"));
		if (!TestNotEqual(*E(TEXT("premessa: Aevik ha la scarica lineare")), ConRicarica, static_cast<int32>(INDEX_NONE))
			|| !TestTrue(*E(TEXT("premessa: la sua ricarica e' di almeno due turni")),
				Doomed->GetAbility(ConRicarica)->CooldownTurns >= 2))
		{
			DestroyDeadWorld(World);
			return false;
		}

		// Muore del Burning del Cleanup e di niente altro: tanti PV quanti ne toglie, senza scudo. Il danno ambientale
		// non ha attaccante, quindi il Blast non lo tocca e l'unita' arriva viva al Cleanup, dopo aver agito.
		Doomed->Shield = 0;
		Doomed->Health = URTCombatLibrary::BurningCleanupDamage;
		Doomed->ApplyStatus(TAG_Status_Burning, /*Turns=*/ 2);

		PC->SelectActorForTest(Doomed);
		PC->SelectAbilityForCurrentForTest(ConRicarica);
		Doomed->PlannedAbilityIndex = ConRicarica;
		Doomed->PlannedAttackTarget = Target;
		if (!TestEqual(*E(TEXT("premessa: armata e pianificata come farebbe il giocatore")),
				Doomed->SelectedAbilityIndex, ConRicarica))
		{
			DestroyDeadWorld(World);
			return false;
		}

		// --- A. all'istante in cui il Cleanup si chiude: morta, armata, in ricarica -----------------------
		struct FStato { bool bRilevato = false; bool bViva = true; bool bValida = false; int32 Armata = INDEX_NONE; int32 Ricarica = 0; };
		FStato AllaChiusura;
		const FDelegateHandle Ascolto = TM->OnPhaseClosed.AddLambda([&](ERTMatchPhase Chiusa)
		{
			if (Chiusa == ERTMatchPhase::Cleanup)
			{
				AllaChiusura.bRilevato = true;
				AllaChiusura.bValida = IsValid(Doomed);
				AllaChiusura.bViva = Doomed->IsAlive();
				AllaChiusura.Armata = Doomed->SelectedAbilityIndex;
				AllaChiusura.Ricarica = Doomed->GetAbilityCooldown(ConRicarica);
			}
		});

		TM->LockInAndResolve();
		TM->OnPhaseClosed.Remove(Ascolto);

		const bool bPremesse =
			TestTrue(*E(TEXT("A: il Cleanup si e' chiuso")), AllaChiusura.bRilevato)
			&& TestTrue(*E(TEXT("A: l'attore c'e' ancora")), AllaChiusura.bValida)
			&& TestFalse(*E(TEXT("A: ed e' morto del Burning")), AllaChiusura.bViva)
			&& TestEqual(*E(TEXT("A: il Cleanup che lo salta non l'ha disarmato")), AllaChiusura.Armata, ConRicarica)
			&& TestTrue(*E(TEXT("A: e ha l'azione in ricarica")), AllaChiusura.Ricarica > 0);
		if (!bPremesse)
		{
			DestroyDeadWorld(World);
			return false;
		}

		if (bPlayback)
		{
			// --- B. con il playback l'intervallo dura quanto il playback, e il mondo e' in sola lettura -----
			if (!TestTrue(*E(TEXT("B: premessa: il playback e' in corso, quindi l'unita' non e' ancora distrutta")),
					TM->IsResolving() && IsValid(Doomed)))
			{
				DestroyDeadWorld(World);
				return false;
			}
			TestEqual(*E(TEXT("B: il contesto del puntatore e' la risoluzione in corso")),
				PC->GetPointerContext(), ERTPointerContext::ResolutionPlayback);
			TestTrue(*E(TEXT("B: il mondo e' in sola lettura per l'input")), PC->IsWorldReadOnly());

			// Se il mondo fosse scrivibile lo slot gia' armato si DISARMEREBBE (toggle) e il click pianificherebbe.
			PC->ArmKitAbility(ConRicarica);
			TestEqual(*E(TEXT("B: lo slot del dock non cambia l'armo")), Doomed->SelectedAbilityIndex, ConRicarica);
			PC->SelectAbilityForCurrentForTest(INDEX_NONE);
			TestEqual(*E(TEXT("B: il tasto non cambia l'armo")), Doomed->SelectedAbilityIndex, ConRicarica);
			PC->HandleClickOnUnitForTest(Decoy);
			TestFalse(*E(TEXT("B: il click su un altro nemico non pianifica")), Doomed->PlannedAttackTarget == Decoy);

			for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
			{
				TM->Tick(0.05f);
			}
			TestFalse(*E(TEXT("B: finito il playback, il turno e' concluso")), TM->IsResolving());
		}

		// --- C. in fondo, con o senza playback: distrutta, e la selezione non le sopravvive ---------------
		TestFalse(*E(TEXT("C: l'unita' e' distrutta da ConcludeTurn")), IsValid(Doomed));
		TestTrue(*E(TEXT("C: e la selezione del controller e' vuota")), PC->GetSelectedUnit() == nullptr);

		DestroyDeadWorld(World);
		return true;
	};

	// Le due configurazioni si provano SEMPRE entrambe: una premessa che salta per la prima non deve nascondere l'altra.
	const bool bSenzaPlayback = Prova(false);
	const bool bConPlayback = Prova(true);
	return bSenzaPlayback && bConPlayback;
}

#endif // WITH_DEV_AUTOMATION_TESTS
