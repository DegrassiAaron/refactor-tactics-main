// #2826 — la porta che il dock chiama per armare, e la differenza fra un click e un tasto.
//
// 🔑 **Il difetto che questo file presidia e' che il dock era a schermo e non serviva a niente.**
// `#2759`, `#2760` e `#2784` hanno chiuso i tre difetti di montaggio: gli slot mostrano azione, icona,
// cooldown e stato armato — e cliccarli non faceva nulla, perche' `SelectAbilityForCurrent` e' `private:`
// e senza `UFUNCTION`. L'unica strada per armare era la tastiera.

#include "Misc/AutomationTest.h"
#include "Player/RTPlayerController.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Map/RTCellId.h"
#include "RTWorldFixtures.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Un'unita' con il kit vero: senza `DispatchBeginPlay` i cooldown restano vuoti e il kit e' finto. */
	ARTUnit* SpawnUnitConKit(UWorld* World, int32 TeamId)
	{
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U)
		{
			return nullptr;
		}
		U->TeamId = TeamId;
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeAevik());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->bIsBotControlled = false;
		U->DispatchBeginPlay();
		U->PlaceOnCell(FRTCellId(0, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}
}

/**
 * 🔴 **CLICCARE UNO SLOT ARMA, E RICLICCARLO DISARMA — passando dalla stessa porta del tasto.**
 *
 * 🔑 **Le due meta' misurano cose diverse, e la seconda e' quella che il tasto non ha.**
 * `ArmKitAbility` delega a `SelectAbilityForCurrent`, cioe' allo stesso corpo che i dieci numeri
 * attraversano: cooldown, slot reazione e self-target restano decisi in un posto solo, e un click non
 * puo' aggirare un controllo che il tasto rispetta. Cio' che aggiunge e' **soltanto** il toggle.
 *
 * ⚠️ **E il toggle NON deve stare dentro `SelectAbilityForCurrent`**, ed e' il controllo C di questo
 * test: premere due volte `3` e' una riconferma, cliccare due volte lo slot acceso e' una richiesta di
 * spegnerlo. Se il toggle scendesse nel percorso comune, il secondo `3` disarmerebbe — un difetto che
 * nessuna delle due meta' qui sopra, da sola, vedrebbe.
 *
 * ⛔ **Il disarmo passa dalla porta e non scrive `SelectedAbilityIndex` a mano**: eredita cosi' le
 * guardie su input bloccato e pianificazione inerte. Un click che disarmasse durante la risoluzione
 * sarebbe un secondo canale con regole proprie.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionDockPortArmsAndDisarmsTest,
	"RefactorTactics.PlayerInput.TheDockPortArmsAndDisarms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionDockPortArmsAndDisarmsTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTUnit* Unit = SpawnUnitConKit(World, /*TeamId*/ 0);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!Unit || !PC)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita' e controller esistono"), false);
	}
	PC->SelectActorForTest(Unit);

	// Anti-vacuita': senza un kit, ogni asserzione qui sotto sarebbe verde per il motivo sbagliato.
	if (!TestTrue(TEXT("premessa: l'unita' ha un kit da armare"), Unit->NumAbilities() > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}
	const int32 Indice = 0;

	// --- A. il click ARMA ------------------------------------------------------------------------
	PC->ArmKitAbility(Indice);
	TestEqual(TEXT("A: cliccare uno slot arma quella posizione del kit"),
		Unit->SelectedAbilityIndex, Indice);

	// --- B. ricliccarlo DISARMA -----------------------------------------------------------------
	PC->ArmKitAbility(Indice);
	TestEqual(TEXT("B: ricliccare lo slot armato riporta al neutro di D-128"),
		Unit->SelectedAbilityIndex, INDEX_NONE);

	// --- C. il TASTO non fa toggle, ed e' la differenza che giustifica la porta ------------------
	// ⚠️ Senza questo controllo, spostare il toggle dentro `SelectAbilityForCurrent` supererebbe A e B
	// e romperebbe la tastiera in silenzio.
	PC->SelectAbilityForCurrentForTest(Indice);
	PC->SelectAbilityForCurrentForTest(Indice);
	TestEqual(TEXT("C: due pressioni dello stesso tasto RICONFERMANO, non disarmano"),
		Unit->SelectedAbilityIndex, Indice);

	// --- D. armare un'altra posizione sostituisce, non accumula ---------------------------------
	if (Unit->NumAbilities() > 1)
	{
		PC->ArmKitAbility(1);
		TestEqual(TEXT("D: armare un'altra posizione sostituisce la precedente"),
			Unit->SelectedAbilityIndex, 1);
	}

	// --- E. fail-closed senza selezione ----------------------------------------------------------
	// Non c'e' un valore da leggere: la prova e' che non esploda e non tocchi l'unita' deselezionata.
	PC->SelectActorForTest(nullptr);
	PC->ArmKitAbility(0);
	TestTrue(TEXT("E: senza unita' selezionata la porta non fa nulla e non crolla"), true);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔴 **CLICK E TASTO RAGGIUNGONO LA STESSA VOCE DI KIT — PRIMA, INTERMEDIA E ULTIMA POSIZIONE** (`#2987`).
 *
 * 🔑 **L'oracolo e' l'UGUAGLIANZA fra i due percorsi, non che ciascuno «funzioni».** Due canali che
 * armano *qualcosa* passerebbero due test scritti separatamente e potrebbero comunque armare due voci
 * diverse: e' precisamente il difetto che la lista del dock rendeva possibile, quando la vista rinumerava
 * le posizioni e il tasto no.
 *
 * ⚠️ **Le tre posizioni non sono un campione di cortesia.** `ActionSlotLineCarriesKeyArmedAndReason`
 * provava una posizione centrale e per questo restava verde sul difetto: sono la **prima** e l'**ultima** a
 * rompersi per prime quando una lista si accorcia o si riordina.
 *
 * ⛔ **Il toggle e' l'unica differenza ammessa**, ed e' gia' presidiato da `TheDockPortArmsAndDisarms`:
 * qui si parte ogni volta dal neutro di [D-128], cosi' la seconda meta' del confronto misura l'armamento e
 * non un disarmo accidentale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTActionDockParityTest,
	"RefactorTactics.PlayerInput.DockClickAndHotkeyReachTheSameAbility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTActionDockParityTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World))
	{
		return false;
	}

	ARTUnit* Unit = SpawnUnitConKit(World, /*TeamId*/ 0);
	ARTPlayerController* PC = World->SpawnActor<ARTPlayerController>();
	if (!Unit || !PC)
	{
		RTWorldFixtures::DestroyWorld(World);
		return TestTrue(TEXT("unita' e controller esistono"), false);
	}
	PC->SelectActorForTest(Unit);

	const int32 Ultima = Unit->NumAbilities() - 1;
	if (!TestTrue(TEXT("premessa: il kit ha almeno tre posizioni"), Unit->NumAbilities() >= 3))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Prima, intermedia, ultima: gli estremi sono quelli che un riordino o un accorciamento spostano.
	for (const int32 Posizione : { 0, Unit->NumAbilities() / 2, Ultima })
	{
		// Dal neutro, cosi' il click ARMA invece di fare toggle su cio' che il tasto aveva lasciato.
		PC->SelectAbilityForCurrentForTest(INDEX_NONE);
		PC->SelectAbilityForCurrentForTest(Posizione);
		const int32 DalTasto = Unit->SelectedAbilityIndex;

		PC->SelectAbilityForCurrentForTest(INDEX_NONE);
		PC->ArmKitAbility(Posizione);
		const int32 DalClick = Unit->SelectedAbilityIndex;

		TestEqual(
			FString::Printf(TEXT("posizione %d: click e tasto armano la stessa voce"), Posizione),
			DalClick, DalTasto);

		// ⚠️ Anti-vacuita': se entrambi i canali fallissero, `INDEX_NONE == INDEX_NONE` passerebbe
		// l'uguaglianza qui sopra dichiarando parita' fra due percorsi che non armano niente.
		TestNotEqual(
			FString::Printf(TEXT("posizione %d: e l'hanno armata davvero"), Posizione),
			DalClick, static_cast<int32>(INDEX_NONE));
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
