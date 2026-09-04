#include "Misc/AutomationTest.h"

#include "Core/RTServerOnlyGuard.h"
#include "Tests/RTServerOnlyGuardFixturesForTest.h"
#include "Turn/RTIntentPrivacyLibrary.h"       // FRTPlannedIntent: il piano autorevole
#include "Turn/RTReactionOpportunityTypes.h"   // FRTReactionOpportunity: AllowedResponses, D-021

#if WITH_DEV_AUTOMATION_TESTS

/**
 * LA GUARDIA STRUTTURALE DELLA PRIVACY — invariante #6, PDR-04 §9 passo 6, `#589`.
 *
 * 🔑 **Tre test, e il primo da solo non varrebbe.** Lo sweep gira oggi su una superficie di replica
 * **vuota** — misurato: zero `UPROPERTY(Replicated)` in tutto `Source/` — quindi il suo verde e'
 * indistinguibile da quello di una guardia che non guarda niente. Sono gli altri due a dare significato al
 * primo: piantano un leak vero e pretendono che venga trovato, diretto **e** annidato.
 *
 * ⚠️ **Non sostituiscono i quattro test di privacy logica** (`Reactions.IntentNotVisibleToEnemy`,
 * `Facing.IntentIsTeamFiltered`, `Combat.IntentVisibleToAlliesAlwaysEnemiesOnlyIfRevealed`,
 * `Overwatch.OpportunityLeaksNoFuture`): quelli chiedono se il filtro consegna la cosa giusta, questi se
 * esiste una via che aggira il filtro. Un `FilterForTeam` corretto non impedisce a un tipo server-only di
 * acquisire una proprieta' replicata in un refactor futuro.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTServerOnlyTypesAreNotReplicatedTest,
	"RefactorTactics.Privacy.ServerOnlyTypesAreNotReplicated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTServerOnlyTypesAreNotReplicatedTest::RunTest(const FString&)
{
	// ⛔ FAIL-LOUD, non fail-silent. Senza metadata la guardia non puo' leggere nessuna dichiarazione, e un
	// gate che tace quando non puo' misurare produce un verde: e' peggio di un gate assente.
	if (!TestTrue(TEXT("i metadata di reflection sono leggibili (WITH_METADATA)"),
		RTServerOnlyGuard::IsMetadataAvailable()))
	{
		return false;
	}

	const TArray<UStruct*> Production = RTServerOnlyGuard::CollectServerOnlyTypes(/*bIncludeFixtures*/ false);

	// ⛔ Il guardiano contro la vacuita': se il marcatore sparisse da tutti i tipi, lo sweep girerebbe su un
	// insieme vuoto e resterebbe verde. Zero tipi marcati e' un FALLIMENTO, non un caso banale.
	if (!TestTrue(TEXT("esiste almeno un tipo dichiarato server-only"), Production.Num() > 0))
	{
		return false;
	}

	// I due tipi che oggi lo sono, pinnati per NOME e non per conteggio: togliere il marcatore a uno dei due
	// e' ridecidere la privacy per sottrazione, e deve costare un test rosso — non passare inosservato
	// perche' il totale resta positivo grazie all'altro.
	TestTrue(TEXT("FRTPlannedIntent e' dichiarato server-only (invariante #6)"),
		Production.Contains(FRTPlannedIntent::StaticStruct()));
	TestTrue(TEXT("FRTReactionOpportunity e' dichiarato server-only (D-021)"),
		Production.Contains(FRTReactionOpportunity::StaticStruct()));

	// LA MISURA. Nessuna via di rete raggiunge un tipo server-only.
	const TArray<FRTReplicationLeak> Leaks = RTServerOnlyGuard::FindLeaks(Production);
	for (const FRTReplicationLeak& Leak : Leaks)
	{
		AddError(FString::Printf(TEXT("leak di privacy strutturale: %s"), *Leak.Describe()));
	}
	TestEqual(TEXT("nessun tipo server-only e' raggiungibile da rete"), Leaks.Num(), 0);

	// L'ESCLUSIONE E' CONTATA. Le fixture del controllo positivo violano apposta, quindi lo sweep le salta —
	// ma un'esclusione che cresce in silenzio e' il modo in cui un gate smette di coprire senza diventare
	// rosso. Oggi l'unica esclusa e' `FRTServerOnlyGuardPlantedSecret`.
	const TArray<UStruct*> WithFixtures = RTServerOnlyGuard::CollectServerOnlyTypes(/*bIncludeFixtures*/ true);
	TestEqual(TEXT("le esclusioni dallo sweep sono esattamente una"),
		WithFixtures.Num() - Production.Num(), 1);
	TestTrue(TEXT("e l'esclusa e' la fixture del controllo positivo"),
		WithFixtures.Contains(FRTServerOnlyGuardPlantedSecret::StaticStruct()));
	TestFalse(TEXT("che infatti non entra nello sweep di produzione"),
		Production.Contains(FRTServerOnlyGuardPlantedSecret::StaticStruct()));

	return true;
}

/**
 * IL CONTROLLO POSITIVO. Senza questo test, il precedente non distingue «nessun leak» da «guardia cieca».
 *
 * Given una `USTRUCT` marcata `RTServerOnly` e una `UCLASS` con una `UPROPERTY(Replicated)` di quel tipo
 * When si interroga la guardia sul tipo marcato
 * Then ritorna la violazione, rotta `ReplicatedProperty`, e **nomina la proprieta' colpevole**
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardDetectsAPlantedLeakTest,
	"RefactorTactics.Privacy.GuardDetectsAPlantedLeak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardDetectsAPlantedLeakTest::RunTest(const FString&)
{
	if (!TestTrue(TEXT("i metadata di reflection sono leggibili"), RTServerOnlyGuard::IsMetadataAvailable()))
	{
		return false;
	}

	const TArray<FRTReplicationLeak> Leaks =
		RTServerOnlyGuard::FindLeaksForType(FRTServerOnlyGuardPlantedSecret::StaticStruct());

	// Due proprieta' replicate puntano al segreto: una diretta, una annidata. La guardia deve vederle
	// entrambe — trovarne una sola significherebbe che una delle due rotte non e' percorsa.
	if (!TestEqual(TEXT("la guardia trova entrambi i leak piantati"), Leaks.Num(), 2))
	{
		for (const FRTReplicationLeak& Leak : Leaks)
		{
			AddInfo(FString::Printf(TEXT("trovato: %s"), *Leak.Describe()));
		}
		return false;
	}

	bool bFoundDirect = false;
	for (const FRTReplicationLeak& Leak : Leaks)
	{
		TestEqual(TEXT("la rotta e' quella della proprieta' replicata"),
			static_cast<int32>(Leak.Route), static_cast<int32>(ERTLeakRoute::ReplicatedProperty));
		TestEqual(TEXT("il leak nomina il tipo server-only"),
			Leak.ServerOnlyType, FRTServerOnlyGuardPlantedSecret::StaticStruct()->GetFName());

		// Il messaggio deve NOMINARE il colpevole: un leak che dice solo «esiste» non e' azionabile.
		TestTrue(TEXT("il portatore e' nominato"),
			Leak.Carrier.Contains(TEXT("RTServerOnlyGuardLeakyCarrierForTest")));

		if (Leak.Carrier.Contains(TEXT("Direct")))
		{
			bFoundDirect = true;
		}
	}
	TestTrue(TEXT("fra i due c'e' la rotta diretta"), bFoundDirect);

	return true;
}

/**
 * IL LEAK A DUE SALTI. La rotta che una guardia ingenua manca — ed e' quella che conta.
 *
 * `FRTPlannedIntent` non avra' MAI un `UPROPERTY(Replicated)` fra i propri membri: la replica si dichiara
 * sulla **classe** che lo trasporta. Una guardia che guardasse solo il tipo diretto della proprieta'
 * replicata sarebbe verde su ogni leak reale, perche' nessuno replica un intento nudo — lo replica dentro
 * qualcos'altro.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGuardSeesThroughContainersAndNestingTest,
	"RefactorTactics.Privacy.GuardSeesThroughContainersAndNesting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGuardSeesThroughContainersAndNestingTest::RunTest(const FString&)
{
	if (!TestTrue(TEXT("i metadata di reflection sono leggibili"), RTServerOnlyGuard::IsMetadataAvailable()))
	{
		return false;
	}

	const TArray<FRTReplicationLeak> Leaks =
		RTServerOnlyGuard::FindLeaksForType(FRTServerOnlyGuardPlantedSecret::StaticStruct());

	const FRTReplicationLeak* Nested = Leaks.FindByPredicate(
		[](const FRTReplicationLeak& Leak) { return Leak.Carrier.Contains(TEXT("Nested")); });

	if (!TestNotNull(TEXT("il leak annidato e' stato trovato"), Nested))
	{
		return false;
	}

	// Il cammino deve MOSTRARE i salti, non solo dichiarare l'esito: chi legge il fallimento deve poter
	// risalire da solo alla riga da correggere.
	TestTrue(TEXT("il cammino attraversa il wrapper innocente"),
		Nested->Path.Contains(TEXT("RTServerOnlyGuardInnocentWrapper")));
	TestTrue(TEXT("e il TArray al suo interno"),
		Nested->Path.Contains(TEXT("Items")));
	TestTrue(TEXT("e arriva al tipo server-only"),
		Nested->Path.Contains(TEXT("RTServerOnlyGuardPlantedSecret")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
