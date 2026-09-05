// La visibilita' dei componenti di `ARTUnit` e' una FUNZIONE dello stato, non un'assegnazione.
//
// ## Il difetto che questi test chiudono
//
// `ApplyObserverVisibility` accendeva `Mesh` e `SelectionRing` incondizionatamente su `bRender`, e quel
// «true» sovrascriveva due decisioni prese da altri:
//
//   - l'anello di SELEZIONE lo accende `OnSelected`. Cammino rotto: nemico ignoto -> viene avvistato ->
//     `SetKnownToObserver(true)` -> l'anello si accende su un nemico che nessuno ha selezionato, e ci resta;
//   - il cilindro SEGNAPOSTO e' nascosto sugli eroi skeletal. Riaccenderlo rimette un cilindro dentro il
//     personaggio.
//
// Il guard di idempotenza di `SetKnownToObserver` non risincronizzava mai, perche' la transizione che
// clobbera e' proprio il ritorno a `true`.
//
// ## La forma della riparazione
//
// I flag sono lo STATO (`bKnownToObserver`, `bSelected`, i materiali, la vita); la visibilita' e' la sua
// funzione, e la calcola `RefreshComponentVisibility` — l'unico posto che sa quali componenti esistono.
// Con W scrittori e F flag l'alternativa sono W×F congiunzioni sparse, e ognuna e' una che qualcuno
// dimentichera'.

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UI/RTHUD.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: la unity build condivide la translation unit. */
	UWorld* UvMakeWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void UvDestroyWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	/**
	 * I componenti dell'unita' sono `protected`: si raggiungono per NOME di sottooggetto, che e' il nome
	 * con cui il costruttore li crea (`Mesh`, `TeamRing`, `SelectionRing`).
	 */
	UStaticMeshComponent* UvComponentNamed(AActor* Actor, const TCHAR* Name)
	{
		if (!Actor) { return nullptr; }
		TArray<UStaticMeshComponent*> Comps;
		Actor->GetComponents<UStaticMeshComponent>(Comps);
		for (UStaticMeshComponent* C : Comps)
		{
			if (C && C->GetName() == Name) { return C; }
		}
		return nullptr;
	}
}

/**
 * I tre predicati, puri e statici: enumera i casi che contano, compresi quelli che la forma precedente
 * sbagliava — dove `bRender` da solo accendeva il componente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitComponentVisibilityIsDerivedTest,
	"RefactorTactics.Unit.ComponentVisibilityIsDerived",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitComponentVisibilityIsDerivedTest::RunTest(const FString&)
{
	// Cilindro segnaposto: c'e' solo dove non c'e' l'eroe.
	TestTrue (TEXT("nessuna skeletal: il segnaposto si vede"),  ARTUnit::ShouldShowPlaceholderMesh(true,  false));
	TestFalse(TEXT("eroe skeletal: il segnaposto sparisce"),    ARTUnit::ShouldShowPlaceholderMesh(true,  true));
	TestFalse(TEXT("unita' non renderizzata: mai"),             ARTUnit::ShouldShowPlaceholderMesh(false, false));

	// Anello di selezione: servono tutte e tre le condizioni.
	TestTrue (TEXT("vista, selezionata, col materiale: si vede"),
		ARTUnit::ShouldShowSelectionRing(true, true, true));
	// 🔴 La riga del difetto: un'unita' che torna visibile NON e' un'unita' selezionata.
	TestFalse(TEXT("vista ma NON selezionata: resta spento"),
		ARTUnit::ShouldShowSelectionRing(true, false, true));
	TestFalse(TEXT("selezionata ma non vista: spento"),
		ARTUnit::ShouldShowSelectionRing(false, true, true));
	TestFalse(TEXT("selezionata e vista ma senza materiale: spento"),
		ARTUnit::ShouldShowSelectionRing(true, true, false));

	// Anello di squadra: senza materiale il ripiego e' il colore sul cilindro, non un disco grigio.
	TestTrue (TEXT("col materiale e visibile: si vede"),  ARTUnit::ShouldShowTeamRing(true,  true));
	TestFalse(TEXT("senza materiale: non si accende"),    ARTUnit::ShouldShowTeamRing(true,  false));
	TestFalse(TEXT("non renderizzata: mai"),              ARTUnit::ShouldShowTeamRing(false, true));
	return true;
}

/**
 * 🔴 **Riavvistare un nemico non lo seleziona.**
 *
 * ⚠️ **Anti-vacuita'**: il cilindro segnaposto — che in questa unita' esiste, perche' un `ARTUnit` C++ non
 * ha la skeletal che il Blueprint aggiunge — DEVE tornare visibile nello stesso giro. Senza questa
 * asserzione il test passerebbe anche se la refresh non facesse piu' nulla.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTUnitRediscoveryDoesNotSelectTest,
	"RefactorTactics.Unit.RediscoveryDoesNotSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTUnitRediscoveryDoesNotSelectTest::RunTest(const FString&)
{
	UWorld* World = UvMakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	UStaticMeshComponent* Ring = UvComponentNamed(Unit, TEXT("SelectionRing"));
	UStaticMeshComponent* Placeholder = UvComponentNamed(Unit, TEXT("Mesh"));
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("anello di selezione"), Ring)
		|| !TestNotNull(TEXT("cilindro segnaposto"), Placeholder))
	{
		UvDestroyWorld(World);
		return false;
	}

	// Premessa: nessuno l'ha selezionata, e l'anello nasce spento.
	TestFalse(TEXT("premessa: l'anello nasce spento"), Ring->GetVisibleFlag());

	// Si perde di vista...
	Unit->SetKnownToObserver(false);
	TestFalse(TEXT("nascosta: l'anello resta spento"),  Ring->GetVisibleFlag());
	TestFalse(TEXT("nascosta: il segnaposto sparisce"), Placeholder->GetVisibleFlag());

	// ...e la si riavvista.
	Unit->SetKnownToObserver(true);

	// 🔴 Il cuore.
	TestFalse(TEXT("riavvistata: l'anello di selezione NON si accende"), Ring->GetVisibleFlag());
	// Anti-vacuita': la refresh ha davvero riacceso cio' che doveva.
	TestTrue(TEXT("riavvistata: il segnaposto torna"), Placeholder->GetVisibleFlag());

	UvDestroyWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------
// Il DRIVER del velo (`#2246`).
//
// ## Il difetto che questi due test chiudono
//
// `bKnownToObserver` nasce `true` — *«un'unita' nasce nota»* — e fino a `#2246` l'unico posto che lo
// smentiva era il ciclo di `ARTHUD::DrawHUD`, che comincia con `if (!Canvas) { return; }`.
//
// Ne segue che **«il driver non ha girato» e «tutto e' noto» producono lo stesso schermo**, e nessun test
// li distingueva: si poteva spostare la sovrapposizione altrove — la migrazione a `WidgetComponent` che
// `#613` prepara — portarsi via il velo, e restare verdi con ogni nemico visibile.
//
// Servono ENTRAMBI, e nessuno dei due basta:
//   - `EnemyWithoutViewIsHidden` prova che il driver DECIDE bene;
//   - `DriverRunsOnTick` prova che il driver VIENE ESEGUITO.
// ---------------------------------------------------------------------------------------------------

namespace
{
	/**
	 * Un HUD e due unita' nello stesso mondo: una avversaria e una della squadra dell'osservatore.
	 *
	 * ⚠️ **Senza `PlayerController` il team dell'osservatore e' `0`** — e' il ripiego dichiarato da
	 * `ARTPlayerState::TeamIdOf`, non un caso non gestito: e' cio' che rende `TeamId = 1` un avversario e
	 * `TeamId = 0` un alleato senza dover montare un player state.
	 *
	 * ⚠️ **Senza `ARTTurnManager` la vista di conoscenza resta VUOTA**, quindi l'avversario non ha voce,
	 * quindi non e' `Live`: e' esattamente lo stato in cui il velo deve spegnerlo.
	 */
	struct FUvVeilFixture
	{
		UWorld* World = nullptr;
		ARTHUD* Hud = nullptr;
		ARTUnit* Enemy = nullptr;
		ARTUnit* Ally = nullptr;
	};

	FUvVeilFixture UvMakeVeilFixture()
	{
		FUvVeilFixture F;
		F.World = UvMakeWorld();
		if (!F.World) { return F; }

		F.Hud = F.World->SpawnActor<ARTHUD>();

		F.Enemy = F.World->SpawnActor<ARTUnit>();
		if (F.Enemy) { F.Enemy->TeamId = 1; F.Enemy->StableUnitId = 1; }

		F.Ally = F.World->SpawnActor<ARTUnit>();
		if (F.Ally) { F.Ally->TeamId = 0; F.Ally->StableUnitId = 2; }

		return F;
	}
}

/**
 * 🔴 **Il driver spegne l'avversario che la vista non conosce, e NON tocca l'alleato.**
 *
 * ⚠️ **Anti-vacuita' su due fronti.** Un test che guardasse solo l'avversario passerebbe anche se il driver
 * spegnesse *tutto* — che a schermo e' un altro difetto, non una riparazione. E il flag da solo non basta:
 * si asserisce anche il COMPONENTE, perche' `SetKnownToObserver` ha un guard di idempotenza e la
 * visibilita' e' una funzione dello stato, non un'assegnazione (vedi i test qui sopra).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilEnemyWithoutViewIsHiddenTest,
	"RefactorTactics.Veil.EnemyWithoutViewIsHidden",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilEnemyWithoutViewIsHiddenTest::RunTest(const FString&)
{
	FUvVeilFixture F = UvMakeVeilFixture();
	if (!TestNotNull(TEXT("mondo di prova"), F.World)) { return false; }
	if (!TestNotNull(TEXT("HUD"), F.Hud) || !TestNotNull(TEXT("avversario"), F.Enemy)
		|| !TestNotNull(TEXT("alleato"), F.Ally))
	{
		UvDestroyWorld(F.World);
		return false;
	}

	UStaticMeshComponent* EnemyMesh = UvComponentNamed(F.Enemy, TEXT("Mesh"));
	UStaticMeshComponent* AllyMesh  = UvComponentNamed(F.Ally,  TEXT("Mesh"));
	if (!TestNotNull(TEXT("segnaposto avversario"), EnemyMesh)
		|| !TestNotNull(TEXT("segnaposto alleato"), AllyMesh))
	{
		UvDestroyWorld(F.World);
		return false;
	}

	// Premessa: entrambe nascono note, ed e' il default che rende il difetto silenzioso.
	TestTrue(TEXT("premessa: l'avversario nasce noto"), F.Enemy->IsKnownToObserver());
	TestTrue(TEXT("premessa: l'alleato nasce noto"),    F.Ally->IsKnownToObserver());

	F.Hud->UpdateObserverVeil();

	// 🔴 Il cuore: senza voce nella vista, l'avversario non e' `Live` e sparisce.
	TestFalse(TEXT("avversario senza vista: non e' piu' noto"), F.Enemy->IsKnownToObserver());
	TestFalse(TEXT("avversario senza vista: il segnaposto sparisce"), EnemyMesh->GetVisibleFlag());

	// 🔴 L'altra meta': la propria squadra si vede SEMPRE. Nasconderla a se' stessi non e' conoscenza
	// parziale, e' un difetto — ed e' il caso che un driver troppo zelante romperebbe.
	TestTrue(TEXT("alleato: resta noto"), F.Ally->IsKnownToObserver());
	TestTrue(TEXT("alleato: il segnaposto resta"), AllyMesh->GetVisibleFlag());

	UvDestroyWorld(F.World);
	return true;
}

/**
 * 🔴 **Il driver e' CABLATO al tick, e il tick e' acceso.**
 *
 * Due asserzioni, e nessuna delle due e' sufficiente da sola:
 *
 *   - `bCanEverTick` — `AHUD` nasce con il tick spento. Senza, il motore non chiamerebbe mai `Tick`, e il
 *     velo non verrebbe applicato **mai**: ogni nemico visibile, nessun errore, nessun log;
 *   - `Tick()` produce l'effetto — cioe' `Tick` chiama davvero il driver. Se qualcuno domani ne stacca la
 *     chiamata, questa asserzione cade mentre `bCanEverTick` resterebbe verde.
 *
 * ⚠️ E' il test che distingue *«il driver ha deciso "noto"»* da *«il driver non e' mai girato»*, che e'
 * precisamente la coppia che il default `true` rende indistinguibile a schermo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilDriverRunsOnTickTest,
	"RefactorTactics.Veil.DriverRunsOnTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilDriverRunsOnTickTest::RunTest(const FString&)
{
	FUvVeilFixture F = UvMakeVeilFixture();
	if (!TestNotNull(TEXT("mondo di prova"), F.World)) { return false; }
	if (!TestNotNull(TEXT("HUD"), F.Hud) || !TestNotNull(TEXT("avversario"), F.Enemy))
	{
		UvDestroyWorld(F.World);
		return false;
	}

	// 1. Il motore deve poterlo chiamare.
	TestTrue(TEXT("il tick dell'HUD e' abilitato"), F.Hud->PrimaryActorTick.bCanEverTick);

	// 2. E chiamarlo deve applicare il velo, senza disegnare nulla.
	TestTrue(TEXT("premessa: l'avversario nasce noto"), F.Enemy->IsKnownToObserver());
	F.Hud->Tick(0.f);
	TestFalse(TEXT("dopo un tick: l'avversario non e' piu' noto"), F.Enemy->IsKnownToObserver());

	UvDestroyWorld(F.World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
