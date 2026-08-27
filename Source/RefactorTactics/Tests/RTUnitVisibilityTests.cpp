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

#endif // WITH_DEV_AUTOMATION_TESTS
