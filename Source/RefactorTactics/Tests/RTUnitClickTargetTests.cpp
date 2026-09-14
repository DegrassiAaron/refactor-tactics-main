// Il click raggiunge l'unita'? — la catena che `ARTPlayerController::OnSelect` percorre, misurata senza
// aprire l'Editor (`#3105`).
//
// 🔴 **La domanda nasce da un sintomo osservato, non da una revisione del codice.** Nella seduta PIE del
// 2026-09-14 `rt.Debug.Refusal` dichiarava il caso in campo e nominava le due unita', ma il click sul
// bersaglio **non arrivava mai**: zero rifiuti nel log, e nemmeno un «troppo lontano» — cioe' il raycast
// non stava colpendo un `ARTUnit` affatto. `OnSelect` fa
//
//     GetHitResultUnderCursor(ECC_Visibility, ...)  ->  Cast<ARTUnit>(Hit.GetActor())
//
// e un cast che fallisce diventa **movimento**, che e' esattamente il sintomo riportato: *«se click su
// abilita' e click su esagono, mi prende il movimento»*.
//
// ⛔ **Cio' che questi test NON possono dire**: che il click funzioni *a schermo*. La proiezione dal
// cursore al mondo dipende dal viewport, e un test headless non ne ha uno. Qui si misura l'anello che
// **puo'** essere misurato — la collisione su `ECC_Visibility` — e si dichiara il resto.

#include "Misc/AutomationTest.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTClickTarget
{
	// Nome distinto da ogni altro file: la unity build condivide la translation unit.
	UWorld* MakeClickWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyClickWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/**
	 * Il trace che `OnSelect` esegue, ridotto a cio' che un test headless puo' riprodurre: dall'alto verso
	 * il centro dell'unita', sul canale `ECC_Visibility`.
	 *
	 * ⚠️ **Non e' `GetHitResultUnderCursor`**, che richiede un viewport. E' l'anello successivo — il canale
	 * e la collisione — e vale la pena dirlo invece di lasciar credere che sia lo stesso gesto.
	 */
	bool TraceHitsUnit(UWorld* World, const ARTUnit* Unit, AActor*& OutHitActor)
	{
		OutHitActor = nullptr;
		if (!World || !Unit) { return false; }

		const FVector Centro = Unit->GetActorLocation();
		const FVector Dall_alto = Centro + FVector(0.f, 0.f, 500.f);
		const FVector Al_basso = Centro - FVector(0.f, 0.f, 500.f);

		FHitResult Hit;
		FCollisionQueryParams Params;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Dall_alto, Al_basso, ECC_Visibility, Params);
		OutHitActor = bHit ? Hit.GetActor() : nullptr;
		return bHit;
	}

	ARTUnit* SpawnClickUnit(UWorld* World)
	{
		if (!World) { return nullptr; }
		ARTUnit* Unit = World->SpawnActor<ARTUnit>();
		if (!Unit) { return nullptr; }

		// Un eroe qualunque del roster: serve a far passare `ConfigureFromHeroData`, non a scegliere una
		// sagoma — il cilindro segnaposto e' in C++ e non dipende dall'eroe.
		for (const URTHeroData* H : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (H) { Unit->ConfigureFromHeroData(H); break; }
		}
		return Unit;
	}
}

/**
 * Un'unita' appena spawnata E' colpita dal trace su `ECC_Visibility`.
 *
 * E' la premessa di tutto il resto: se cadesse qui, il click non potrebbe funzionare in nessuna
 * condizione, e la seduta del 2026-09-14 avrebbe una spiegazione immediata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitIsHitByVisibilityTraceTest,
	"RefactorTactics.PlayerInput.UnitIsHitByVisibilityTrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitIsHitByVisibilityTraceTest::RunTest(const FString&)
{
	UWorld* World = RTClickTarget::MakeClickWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Unit = RTClickTarget::SpawnClickUnit(World);
	if (!TestNotNull(TEXT("l'unita' si spawna"), Unit))
	{
		RTClickTarget::DestroyClickWorld(World);
		return false;
	}

	AActor* Colpito = nullptr;
	const bool bHit = RTClickTarget::TraceHitsUnit(World, Unit, Colpito);

	TestTrue(TEXT("il trace su ECC_Visibility colpisce qualcosa"), bHit);
	// 🔑 L'asserzione che conta: `OnSelect` fa `Cast<ARTUnit>`, quindi non basta colpire — bisogna colpire
	// UN'UNITA'. Un hit su un componente il cui owner non e' `ARTUnit` diventerebbe movimento.
	TestEqual(TEXT("e cio' che colpisce E' l'unita'"), Colpito, static_cast<AActor*>(Unit));

	RTClickTarget::DestroyClickWorld(World);
	return true;
}

/**
 * 🔴 **L'ipotesi della seduta: il cilindro INVISIBILE resta cliccabile.**
 *
 * `ARTUnit::RefreshComponentVisibility` nasconde il cilindro segnaposto quando il Blueprint porta una hero
 * mesh con posa (`RTUnit.cpp:562`, `ShouldShowPlaceholderMesh`). Ma `SetVisibility(false)` **non** tocca la
 * collisione: il componente sparisce dallo schermo e continua a rispondere al trace.
 *
 * ∴ in partita il bersaglio del click e' una forma che **non si vede**, mentre cio' che si vede — la
 * skeletal del `BP_Unit_*` — puo' avere ingombro diverso. Cliccare «sul personaggio» puo' quindi mancare
 * il cilindro, e il click diventa movimento.
 *
 * ⚠️ **Questo test non dimostra che sia ANDATA cosi' nella seduta**: dimostra che il meccanismo esiste. La
 * distinzione e' quella fra una causa possibile e una causa misurata, e conflonderle qui sarebbe il
 * difetto che questa stessa famiglia di voci ha gia' pagato quattro volte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHiddenPlaceholderStillTakesTheClickTest,
	"RefactorTactics.PlayerInput.HiddenPlaceholderStillTakesTheClick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHiddenPlaceholderStillTakesTheClickTest::RunTest(const FString&)
{
	UWorld* World = RTClickTarget::MakeClickWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Unit = RTClickTarget::SpawnClickUnit(World);
	if (!TestNotNull(TEXT("l'unita' si spawna"), Unit))
	{
		RTClickTarget::DestroyClickWorld(World);
		return false;
	}

	UStaticMeshComponent* Cilindro = Unit->FindComponentByClass<UStaticMeshComponent>();
	if (!TestNotNull(TEXT("il cilindro segnaposto esiste"), Cilindro))
	{
		RTClickTarget::DestroyClickWorld(World);
		return false;
	}

	// --- controllo positivo: visibile, il trace colpisce ---
	{
		AActor* Colpito = nullptr;
		const bool bHit = RTClickTarget::TraceHitsUnit(World, Unit, Colpito);
		TestTrue(TEXT("visibile: il trace colpisce"), bHit);
	}

	// --- il fatto che spiega il sintomo: nascosto, il trace colpisce ANCORA ---
	{
		Cilindro->SetVisibility(false, /*bPropagateToChildren*/ false);

		AActor* Colpito = nullptr;
		const bool bHit = RTClickTarget::TraceHitsUnit(World, Unit, Colpito);

		TestTrue(TEXT("NASCOSTO: il trace colpisce lo stesso — la collisione non segue la visibilita'"), bHit);
		TestEqual(TEXT("e colpisce ancora l'unita'"), Colpito, static_cast<AActor*>(Unit));
	}

	// --- e il contrario: tolta la COLLISIONE, il trace non colpisce piu' ---
	{
		// ⛔ Senza questa meta' il test sarebbe verde anche se il trace colpisse il pavimento o un altro
		// attore: prova che a rispondere e' proprio il cilindro, non qualcos'altro nella scena.
		Cilindro->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AActor* Colpito = nullptr;
		const bool bHit = RTClickTarget::TraceHitsUnit(World, Unit, Colpito);

		TestFalse(TEXT("senza collisione il trace NON colpisce piu': era il cilindro a rispondere"), bHit);
	}

	RTClickTarget::DestroyClickWorld(World);
	return true;
}

/**
 * **Quanto e' grande il bersaglio invisibile del click** — `#3105`.
 *
 * Non asserisce una dimensione «giusta»: nessuno l'ha dichiarata, e fissarne una qui sarebbe inventare un
 * contratto. Misura l'ingombro e lo **stampa**, perche' e' il numero che serve a decidere se cliccare «sul
 * personaggio» possa mancare il cilindro.
 *
 * 🔑 **Il confronto che conta non e' fattibile qui**, e va detto invece di lasciarlo intendere: la sagoma
 * VISIBILE in partita e' la skeletal del `BP_Unit_*`, che vive nei pack `Content/FabAsset/` — gitignorati,
 * quindi assenti in un clone pulito. Questo test misura meta' del confronto; l'altra meta' richiede i pack.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTClickTargetExtentIsMeasuredTest,
	"RefactorTactics.PlayerInput.ClickTargetExtentIsMeasured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTClickTargetExtentIsMeasuredTest::RunTest(const FString&)
{
	UWorld* World = RTClickTarget::MakeClickWorld();
	if (!TestNotNull(TEXT("il mondo di prova esiste"), World)) { return false; }

	ARTUnit* Unit = RTClickTarget::SpawnClickUnit(World);
	UStaticMeshComponent* Cilindro = Unit ? Unit->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!TestNotNull(TEXT("il cilindro segnaposto esiste"), Cilindro))
	{
		RTClickTarget::DestroyClickWorld(World);
		return false;
	}

	const FBoxSphereBounds B = Cilindro->Bounds;
	AddInfo(FString::Printf(TEXT("bersaglio cliccabile: semi-estensione X=%.1f Y=%.1f Z=%.1f (raggio %.1f)"),
		B.BoxExtent.X, B.BoxExtent.Y, B.BoxExtent.Z, B.SphereRadius));

	// Le due controprove del metodo: senza, questo test stamperebbe numeri anche su un componente degenere
	// — e un ingombro nullo si leggerebbe come «misurato», non come «non c'e' niente da cliccare».
	TestTrue(TEXT("l'ingombro orizzontale non e' nullo"), B.BoxExtent.X > 1.f && B.BoxExtent.Y > 1.f);
	TestTrue(TEXT("l'ingombro verticale non e' nullo"), B.BoxExtent.Z > 1.f);

	RTClickTarget::DestroyClickWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
