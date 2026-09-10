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
#include "Engine/HitResult.h"
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
	// ── Cilindro segnaposto: c'e' dove non c'e' un CORPO, e una mesh senza posa non e' un corpo (#2545)
	//
	// 🔑 **Le otto combinazioni, tutte.** Il predicato e' una congiunzione di tre booleani: coprirne solo
	// i casi «che interessano» lascia verde meta' delle congiunzioni sbagliate. Sono otto righe e si
	// scrivono una volta.
	//
	//                                                                    bRender  bHasHeroMesh  bHasPose
	TestFalse(TEXT("non renderizzata: mai, comunque"),   ARTUnit::ShouldShowPlaceholderMesh(false, false, false));
	TestFalse(TEXT("non renderizzata, con posa: mai"),   ARTUnit::ShouldShowPlaceholderMesh(false, false, true));
	TestFalse(TEXT("non renderizzata, con mesh: mai"),   ARTUnit::ShouldShowPlaceholderMesh(false, true,  false));
	TestFalse(TEXT("non renderizzata, mesh e posa"),     ARTUnit::ShouldShowPlaceholderMesh(false, true,  true));
	TestTrue (TEXT("nessuna skeletal: si vede"),         ARTUnit::ShouldShowPlaceholderMesh(true,  false, false));
	TestTrue (TEXT("nessuna skeletal, posa a catalogo: si vede lo stesso"),
		ARTUnit::ShouldShowPlaceholderMesh(true,  false, true));
	// 🔴 **La riga del difetto**: c'e' la mesh, non c'e' la clip. Prima il segnaposto spariva e restava
	// la T-pose, che si legge come «animazione rotta» invece che come «ruolo non legato».
	TestTrue (TEXT("mesh SENZA posa: il segnaposto torna, invece della T-pose"),
		ARTUnit::ShouldShowPlaceholderMesh(true,  true,  false));
	// Il controllo positivo che rende non vacue le sette righe sopra: col corpo completo sparisce davvero.
	TestFalse(TEXT("mesh E posa: il segnaposto sparisce"),
		ARTUnit::ShouldShowPlaceholderMesh(true,  true,  true));

	// ── Skeletal dell'eroe: l'altra meta' della stessa decisione (#2545)
	//
	// Senza queste quattro righe il cilindro potrebbe accendersi SOPRA la T-pose invece che al suo posto,
	// e i test del predicato di sopra resterebbero tutti verdi.
	TestTrue (TEXT("vista, con posa: lo skeletal si mostra"),  ARTUnit::ShouldShowHeroSkeletal(true,  true));
	TestFalse(TEXT("vista, SENZA posa: lo skeletal si nasconde e lascia il posto al segnaposto"),
		ARTUnit::ShouldShowHeroSkeletal(true,  false));
	TestFalse(TEXT("non vista, con posa: nascosto"),           ARTUnit::ShouldShowHeroSkeletal(false, true));
	TestFalse(TEXT("non vista, senza posa: nascosto"),         ARTUnit::ShouldShowHeroSkeletal(false, false));

	// 🔑 **I due predicati non si contraddicono mai**: su un'unita' visibile con la mesh, esattamente uno
	// dei due componenti e' acceso. E' l'invariante che «cilindro e T-pose non si sovrappongono» significa,
	// e nessuna delle righe sopra la copre da sola.
	for (int32 Caso = 0; Caso < 4; ++Caso)
	{
		const bool bRender = (Caso & 1) != 0;
		const bool bPose   = (Caso & 2) != 0;
		const bool bSegnaposto = ARTUnit::ShouldShowPlaceholderMesh(bRender, /*bHasHeroMesh*/ true, bPose);
		const bool bSkeletal   = ARTUnit::ShouldShowHeroSkeletal(bRender, bPose);
		TestFalse(*FString::Printf(TEXT("caso %d: mai entrambi accesi"), Caso), bSegnaposto && bSkeletal);
		if (bRender)
		{
			TestTrue(*FString::Printf(TEXT("caso %d: se si renderizza, uno dei due c'e'"), Caso),
				bSegnaposto || bSkeletal);
		}
	}

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

/**
 * 🔴 **Un'unita' velata non e' cliccabile, e questo e' il test che lo MISURA invece di dedurlo.**
 *
 * ## La premessa falsa che ha aperto #2755
 *
 * La issue affermava il contrario: *«il collider di un'unita' velata resta attivo — la mesh e'
 * `QueryOnly` + `ECR_Block` (`RTUnit.cpp:62-63`) e `RefreshComponentVisibility` spegne la visibilita' ma
 * non la collisione»*. Quella lettura si era fermata al **costruttore**. La stessa funzione, in coda,
 * chiama `SetActorEnableCollision(bRender)` — dal 2026-08-27, commit `678cc8fc`, il cui titolo dice
 * esattamente *«un'unita' ignota alla squadra non si vede e non si clicca»*.
 *
 * ∴ La via che #2755 proponeva era gia' quella implementata. Quello che mancava non era il codice: era
 * **la prova**. Nessun test copriva `SetActorEnableCollision`, e una riga che nessun test tiene ferma e'
 * una riga che il prossimo refactor puo' togliere senza che nulla diventi rosso.
 *
 * ## Perche' il trace, e non `GetActorEnableCollision()`
 *
 * L'oracolo e' il **percorso reale**: `RTPlayerController.cpp:1174` risolve il bersaglio con un trace su
 * `ECC_Visibility`. Asserire il flag proverebbe che il flag e' `false`; asserire il trace prova che il
 * click non trova l'unita' — che e' l'affermazione che ci interessa. Fra le due c'e' tutto cio' che puo'
 * ancora bloccare il raggio: un secondo componente, una skeletal del Blueprint, un canale diverso.
 *
 * ## Anti-vacuita', su due fronti
 *
 *   - ⚠️ **Controllo positivo** — prima del velo il nemico DEVE essere colpito. Senza questa asserzione un
 *     mondo di prova senza scena fisica non colpirebbe mai nulla, e il test sarebbe verde per assenza di
 *     geometria invece che per il velo;
 *   - ⚠️ **La propria squadra resta cliccabile** — un'implementazione che spegnesse la collisione a
 *     *tutti* soddisfa la prima meta' e rompe il gioco. E' la stessa coppia che gli altri test di questo
 *     file asseriscono sulla visibilita'.
 *
 * ⛔ **Verifica di mutazione**: sostituire `SetActorEnableCollision(bRender)` con
 * `SetActorEnableCollision(true)` deve far cadere l'asserzione centrale, e SOLO quella.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTVeilHiddenEnemyIsNotPickableTest,
	"RefactorTactics.Veil.HiddenEnemyIsNotPickable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTVeilHiddenEnemyIsNotPickableTest::RunTest(const FString&)
{
	FUvVeilFixture F = UvMakeVeilFixture();
	if (!TestNotNull(TEXT("mondo di prova"), F.World)) { return false; }
	if (!TestNotNull(TEXT("HUD"), F.Hud) || !TestNotNull(TEXT("avversario"), F.Enemy)
		|| !TestNotNull(TEXT("alleato"), F.Ally))
	{
		UvDestroyWorld(F.World);
		return false;
	}

	// La fixture spawna entrambe le unita' all'ORIGINE: sovrapposte, un trace verticale non saprebbe dire
	// quale delle due ha colpito. Si separano prima di misurare.
	const float EnemyX = 0.f;
	const float AllyX  = 1000.f;
	F.Enemy->SetActorLocation(FVector(EnemyX, 0.f, 0.f));
	F.Ally->SetActorLocation(FVector(AllyX, 0.f, 0.f));

	// Lo stesso canale del picking (`ECC_Visibility`), dall'alto verso il basso lungo l'asse dell'unita'.
	auto PickAt = [&F](float X) -> AActor*
	{
		FHitResult Hit;
		const bool bHit = F.World->LineTraceSingleByChannel(
			Hit, FVector(X, 0.f, 500.f), FVector(X, 0.f, -500.f), ECC_Visibility);
		return bHit ? Hit.GetActor() : nullptr;
	};

	// 1. CONTROLLO POSITIVO. Senza, tutto il resto sarebbe verde anche in un mondo senza collisione.
	TestTrue(TEXT("premessa: finche' e' noto, il trace del picking COLPISCE l'avversario"),
		PickAt(EnemyX) == F.Enemy);

	F.Hud->UpdateObserverVeil();
	TestFalse(TEXT("premessa: il velo ha reso ignoto l'avversario"), F.Enemy->IsKnownToObserver());

	// 2. 🔴 Il cuore: velato, il trace non lo restituisce piu'.
	TestNull(TEXT("l'avversario velato non viene restituito dal trace del picking"), PickAt(EnemyX));

	// 3. E la propria squadra resta selezionabile: e' l'altra meta', e cade se qualcuno spegne tutto.
	TestTrue(TEXT("la propria unita' resta cliccabile"), PickAt(AllyX) == F.Ally);

	UvDestroyWorld(F.World);
	return true;
}


// ---------------------------------------------------------------------------------------------------------
// I bracci graykit (#2880) seguono lo STESSO predicato del cilindro segnaposto.
//
// 🔴 E' l'unica proprieta' che conta, ed e' la ragione per cui questo test sta QUI e non in un file suo:
// la cosa da provare non e' «i bracci esistono», e' «i bracci sono parte del segnaposto». Su un eroe
// skeletal con una posa legata il cilindro sparisce, e due bastoncini che non seguissero quella regola
// resterebbero a orbitare attorno al personaggio.
// ---------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGraykitLimbsFollowPlaceholderTest,
	"RefactorTactics.Graykit.BracciSeguonoIlSegnaposto", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGraykitLimbsFollowPlaceholderTest::RunTest(const FString& Parameters)
{
	// (a) La tabella del predicato, nei quattro casi che contano. E' la stessa che governa il cilindro:
	// qui si dichiara che i bracci NON ne hanno una propria.
	TestTrue(TEXT("unita' visibile senza eroe: segnaposto mostrato"),
		ARTUnit::ShouldShowPlaceholderMesh(/*bRender*/ true, /*bHasHeroMesh*/ false, /*bHasPose*/ false));
	TestFalse(TEXT("eroe skeletal CON posa: segnaposto nascosto"),
		ARTUnit::ShouldShowPlaceholderMesh(true, true, true));
	TestTrue(TEXT("eroe skeletal SENZA posa (#2545): segnaposto mostrato, non una T-pose"),
		ARTUnit::ShouldShowPlaceholderMesh(true, true, false));
	TestFalse(TEXT("unita' non renderizzata: segnaposto nascosto comunque"),
		ARTUnit::ShouldShowPlaceholderMesh(false, false, false));

	// (b) E i componenti veri lo seguono. Un'unita' senza skeletal: cilindro e bracci visibili insieme.
	UWorld* World = UvMakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	UStaticMeshComponent* Placeholder = UvComponentNamed(Unit, TEXT("Mesh"));
	UStaticMeshComponent* Left = UvComponentNamed(Unit, TEXT("LeftArm"));
	UStaticMeshComponent* Right = UvComponentNamed(Unit, TEXT("RightArm"));
	if (!TestNotNull(TEXT("unita'"), Unit) || !TestNotNull(TEXT("cilindro segnaposto"), Placeholder)
		|| !TestNotNull(TEXT("braccio sinistro"), Left) || !TestNotNull(TEXT("braccio destro"), Right))
	{
		UvDestroyWorld(World);
		return false;
	}

	// ⚠️ La transizione, non l'assegnazione: `SetKnownToObserver` ha un guard di idempotenza e un
	// `SetKnownToObserver(true)` su un'unita' gia' nota non ricalcolerebbe nulla. E' la stessa ragione per
	// cui i test sopra passano da `false` prima di tornare a `true`.
	Unit->SetKnownToObserver(false);
	Unit->SetKnownToObserver(true);
	TestEqual(TEXT("nota: il braccio sinistro segue il cilindro"), Left->IsVisible(), Placeholder->IsVisible());
	TestEqual(TEXT("nota: il braccio destro segue il cilindro"), Right->IsVisible(), Placeholder->IsVisible());

	// (c) Velata: i bracci si spengono insieme al cilindro.
	Unit->SetKnownToObserver(false);
	TestFalse(TEXT("velata: il braccio sinistro e' nascosto"), Left->IsVisible());
	TestFalse(TEXT("velata: il braccio destro e' nascosto"), Right->IsVisible());
	TestEqual(TEXT("velata: segue ancora esattamente il cilindro"), Left->IsVisible(), Placeholder->IsVisible());

	// (d) ⛔ I bracci NON somigliano agli anelli: `TeamRing` porta la squadra, cioe' informazione che il
	// personaggio vero non dice; un braccio grigio no. Il test lo dichiara sul predicato, dove vive.
	TestNotEqual(TEXT("sull'eroe con posa, segnaposto e anello di squadra divergono"),
		ARTUnit::ShouldShowPlaceholderMesh(true, true, true),
		ARTUnit::ShouldShowTeamRing(true, /*bHasTeamRingMaterial*/ true));

	UvDestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
