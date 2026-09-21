#include "Misc/AutomationTest.h"

#include "Core/RTServerOnlyGuard.h"
#include "Tests/RTServerOnlyGuardFixturesForTest.h"
#include "Turn/RTIntentPrivacyLibrary.h"       // FRTPlannedIntent: il piano autorevole
#include "Turn/RTReactionOpportunityTypes.h"   // FRTReactionOpportunity: AllowedResponses, D-021
#include "Unit/RTUnit.h"                       // ARTUnit: dove il piano VIVO abita davvero — [D-429]

#if WITH_DEV_AUTOMATION_TESTS

/**
 * LA GUARDIA STRUTTURALE DELLA PRIVACY — invariante #6, PDR-04 §9 passo 6, `#589`.
 *
 * 🔑 **Tre test, e il primo da solo non varrebbe.** Lo sweep gira oggi su una superficie di replica
 * **vuota** — misurato: zero `UPROPERTY(Replicated)` **di produzione** — quindi il suo verde e'
 * indistinguibile da quello di una guardia che non guarda niente. Sono gli altri due a dare significato al
 * primo: piantano un leak vero e pretendono che venga trovato, diretto **e** annidato.
 *
 * ⌫ **Questa riga diceva *«in tutto `Source/`»*, e il file accanto la smentiva: corretto il 2026-09-20.**
 * `RTServerOnlyGuardFixturesForTest.h` — incluso da questo stesso file — dichiara le
 * `UPROPERTY(Replicated)` e le `DOREPLIFETIME` che sono l'**oracolo** delle fixture.
 *
 * 🔴 **E non e' invecchiata: e' nata gia' falsa, il che e' peggio e va detto.** Le fixture e la frase che
 * le smentisce arrivano nello **stesso commit**, `7a046fc4` — prima di quel commit
 * `git grep "UPROPERTY(Replicated)" 7a046fc4^ -- Source/` non trova nulla, dopo trova entrambe. Chi
 * rilanciasse il `grep` nudo troverebbe occorrenze e concluderebbe che il commento menta, invece di
 * capire che sono le sue.
 *
 * La forma che regge non porta un conteggio, perche' ogni nota che ne parla ne aggiunge uno: e' lo
 * **zero misurato di produzione**, col comando che lo dimostra —
 * `grep -rn "DOREPLIFETIME" Source/ | grep -v /Tests/` → nessuna riga.
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

// =========================================================================================================
// IL PIANO VIVO NON ABITA IN UN TIPO MARCATO — la guardia PER CAMPO, `#1805` / [D-429]
// =========================================================================================================
//
// 🔴 **La guardia per TIPO qui sopra e' forte, e non tocca il soggetto dell'AC di `#1805`.**
// `ServerOnlyTypesAreNotReplicated` pinna `FRTPlannedIntent` e `FRTReactionOpportunity`: due `USTRUCT`
// marcate `RTServerOnly`. Ma `FRTPlannedIntent` e' il DTO di **costruzione della vista** — HUD e ViewModel —
// non il contenitore dello stato di pianificazione durante il turno.
//
// In partita il piano vive su `ARTUnit`, come **campi nudi di tipi ordinari**: `FRTCellId`,
// `TArray<FRTCellId>`, `FName`, `bool`, `ERTHexDirection`, `TObjectPtr<ARTUnit>`. Nessuno di quei tipi e'
// marcato — e **marcarli sarebbe un falso positivo di massa**: `FRTCellId` e' ovunque, e finisce nel TurnLog
// **pubblico**. E' la stessa ragione per cui `FRTReactionOpportunityKey` non e' marcata.
//
// ∴ **il giorno in cui uno di quei campi diventasse `UPROPERTY(Replicated)`, lo sweep per tipo resterebbe
// VERDE.** Il repository lo sa gia' e lo scrive — `Unit/RTUnit.h`, sul campo `PlannedMovementProfileId`:
// *«⛔ Non e' replicato, come `PlannedWaypoints` e `PlannedCell` qui sopra, e non e' un dettaglio. La
// dichiarazione e' intento di pianificazione: dire a un avversario "questo sguscia" prima del lock-in gli
// anticipa la distanza che quell'unita' puo' coprire»* — ma nessun gate lo misurava: la regola viveva in un
// commento, che e' la forma che [D-371] ha appena ritirato altrove.
//
// Questo test e' quel gate. Non duplica la guardia per tipo: risponde alla domanda che quella non pone.
//
// ⚠️ **Cosa NON copre, e va detto invece che dedotto.** Non dice cosa accadrebbe **dentro** uno strato di
// replica che oggi non esiste: quando esistera', a decidere cosa esce sara' `URTIntentPrivacyLibrary::
// FilterForTeam`, e sono i suoi test a possedere quella domanda. Qui si dice una cosa sola, e strutturale:
// **non c'e' una via**. Il giorno in cui qualcuno ne apre una, questo test diventa rosso e la decisione si
// prende **prima** che il byte parta, non dopo.

namespace
{
	/**
	 * I campi di `ARTUnit` che portano il PIANO DEL TURNO, pinnati **per nome**.
	 *
	 * 🔑 **Per nome e non per conteggio**, la stessa disciplina con cui `ServerOnlyTypesAreNotReplicated`
	 * pinna i due tipi marcati: rinominare un campo senza aggiornare questo elenco deve costare un **rosso**,
	 * non passare inosservato perche' il totale resta positivo grazie agli altri.
	 *
	 * ⚠️ **L'elenco per nome da solo non basta, ed e' il motivo per cui il test fa anche una scansione.** Un
	 * campo di pianificazione **nuovo** non comparirebbe qui, e nessuno se ne accorgerebbe. La scansione
	 * copre per **prefisso** cio' che l'elenco copre per nome, e le due cose si sorvegliano a vicenda.
	 */
	const TCHAR* const PlanningFieldNames[] = {
		TEXT("PlannedCell"),
		TEXT("PlannedPath"),
		TEXT("PlannedWaypoints"),
		TEXT("PlannedMovementProfileId"),
		TEXT("PlannedAbilityIndex"),
		TEXT("PlannedAttackTarget"),
		TEXT("PlannedAttackCell"),
		TEXT("bAttackTargetsCell"),
		TEXT("PlannedDashAbility"),
		TEXT("PlannedDashCell"),
		TEXT("PlannedFacing"),
		TEXT("bDeclaresPlannedFacing"),
		TEXT("PlannedCoverEdge"),
		TEXT("bHasPlannedCoverEdge"),
		TEXT("PlannedReactionAbility"),
		TEXT("PlannedReactionCondition"),
		TEXT("PlannedCleansePriority"),
		TEXT("bTurnPlanDeclared"),
	};
}

/**
 * Nessun campo di pianificazione di `ARTUnit` e' raggiungibile da una via di replica — `#1805`, [D-429].
 *
 * Given lo stato di pianificazione vivo, che abita `ARTUnit` come campi di tipi ordinari
 * When si interroga la reflection su ciascuno di essi
 * Then nessuno porta `CPF_Net`, e il rilevatore lo dimostra vedendo un `Replicated` vero altrove
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanningFieldsOnTheUnitAreNotReplicatedTest,
	"RefactorTactics.Privacy.PlanningFieldsOnTheUnitAreNotReplicated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlanningFieldsOnTheUnitAreNotReplicatedTest::RunTest(const FString&)
{
	// ⛔ **IL CONTROLLO POSITIVO, e non e' opzionale.** Oggi `CPF_Net` non compare su nessun campo di
	// produzione: un test che cercasse solo la sua assenza sarebbe verde anche a rilevatore rotto — la stessa
	// ragione per cui `RTServerOnlyGuardFixturesForTest.h` esiste. Qui si chiede al rilevatore di **vedere**
	// una replica vera prima di credergli quando dice di non vederne.
	const FProperty* Planted =
		URTServerOnlyGuardLeakyCarrierForTest::StaticClass()->FindPropertyByName(TEXT("Direct"));
	if (!TestNotNull(TEXT("il controllo positivo esiste: la fixture porta ancora la proprieta' piantata"), Planted))
	{
		return false;
	}
	if (!TestTrue(TEXT("e il rilevatore la VEDE replicata: CPF_Net e' leggibile su questo binario"),
		Planted->HasAnyPropertyFlags(CPF_Net)))
	{
		return false;
	}

	UClass* Unit = ARTUnit::StaticClass();
	if (!TestNotNull(TEXT("premessa: ARTUnit e' riflessa"), Unit))
	{
		return false;
	}

	// (1) L'elenco per NOME: ogni campo dichiarato deve ESISTERE — una rinomina silenziosa e' rossa — e non
	//     deve portare `CPF_Net`.
	for (const TCHAR* const Name : PlanningFieldNames)
	{
		const FProperty* Field = Unit->FindPropertyByName(FName(Name));
		if (!TestNotNull(*FString::Printf(
				TEXT("il campo di pianificazione '%s' esiste ancora su ARTUnit (se e' stato rinominato, aggiorna l'elenco)"), Name),
			Field))
		{
			continue;
		}
		TestFalse(*FString::Printf(TEXT("'%s' non e' replicato: il piano privato non ha una via di rete"), Name),
			Field->HasAnyPropertyFlags(CPF_Net));
	}

	// (2) La SCANSIONE per prefisso: copre i campi di pianificazione che qualcuno aggiungera' domani e che
	//     l'elenco sopra non conterrebbe. ⚠️ E ha il proprio controllo di non-vacuita': se la scansione non
	//     trovasse nessun campo, girerebbe sull'insieme vuoto e sarebbe verde per costruzione.
	int32 Scansionati = 0;
	TArray<FString> Replicati;
	for (TFieldIterator<FProperty> It(Unit); It; ++It)
	{
		const FString FieldName = It->GetName();
		if (!FieldName.Contains(TEXT("Planned")) && !FieldName.Contains(TEXT("PlanDeclared")))
		{
			continue;
		}
		++Scansionati;
		if (It->HasAnyPropertyFlags(CPF_Net))
		{
			Replicati.Add(FieldName);
		}
	}
	AddInfo(FString::Printf(TEXT("campi di ARTUnit scanditi per prefisso di pianificazione: %d"), Scansionati));
	if (!TestTrue(TEXT("anti-vacuita': la scansione TROVA dei campi di pianificazione, non gira a vuoto"),
		Scansionati >= UE_ARRAY_COUNT(PlanningFieldNames) - 2))
	{
		return false;
	}
	for (const FString& Leaked : Replicati)
	{
		AddError(FString::Printf(
			TEXT("leak di privacy strutturale: ARTUnit::%s e' replicato, e porta il piano del turno"), *Leaked));
	}
	TestEqual(TEXT("nessun campo di pianificazione di ARTUnit e' replicato"), Replicati.Num(), 0);

	// (3) Il secondo lucchetto, che vale anche se un campo sfuggisse ai due sopra: l'attore **non replica**.
	//     `AActor::bReplicates` e' la quarta rotta che `RTServerOnlyGuard` dichiara fuori copertura per
	//     assenza di soggetto (`Core/RTServerOnlyGuard.h`). Qui il soggetto c'e', ed e' `ARTUnit`.
	const AActor* Cdo = Unit->GetDefaultObject<AActor>();
	if (TestNotNull(TEXT("premessa: il CDO di ARTUnit e' istanziabile"), Cdo))
	{
		TestFalse(TEXT("ARTUnit non replica: nessun campo parte, qualunque flag porti"), Cdo->GetIsReplicated());
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
