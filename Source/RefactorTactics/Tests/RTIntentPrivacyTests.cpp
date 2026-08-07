#include "Misc/AutomationTest.h"
#include "Turn/RTIntentPrivacyLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Privacy dell'intento (invariante #6) estesa alle REAZIONI: CP 5.4, che chiude l'epic E5.
 *
 * Questi test guardano il DTO, non lo schermo. E' il punto: la privacy non e' "non disegnare", e' "non
 * costruire la vista". Un test che verificasse cosa la UI disegna resterebbe verde anche con lo stato completo
 * spedito al client e nascosto graficamente — cioe' proprio la violazione che l'invariante vieta.
 */
namespace
{
	/** Piano completo di prova: un'unita' con movimento, azione, bersaglio e reazione pronta. */
	FRTPlannedIntent MakeFullIntent(int32 TeamId, bool bRevealed)
	{
		FRTPlannedIntent I;
		I.OwnerCell = FRTCellId(0, 0);
		I.TeamId = TeamId;
		I.bAlive = true;
		I.bRevealed = bRevealed;
		I.bMoving = true;
		I.PlannedCell = FRTCellId(2, 0);
		I.ActionName = FText::FromString(TEXT("Tiro"));
		I.bHasTarget = true;
		I.TargetCell = FRTCellId(4, 0);
		I.ReactionName = FText::FromString(TEXT("Contrattacco"));
		I.PlannedPath = { FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0) };
		I.PlannedWaypoints = { FRTCellId(2, 0) };
		I.bDashing = true;
		I.DashCell = FRTCellId(1, 1);
		return I;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReactionIntentNotVisibleToEnemyTest,
	"RefactorTactics.Reactions.IntentNotVisibleToEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReactionIntentNotVisibleToEnemyTest::RunTest(const FString&)
{
	// Nome vincolante della DoD. La reazione di un'unita' non raggiunge MAI un avversario — nemmeno quando
	// l'unita' e' rivelata, e nemmeno come campo vuoto in una vista che comunque arriva.
	const TArray<FRTPlannedIntent> Intents = { MakeFullIntent(/*TeamId*/ 0, /*bRevealed*/ false) };

	// L'ALLEATO vede tutto, reazione inclusa: senza, non potrebbe coordinarsi (pilastro di prodotto).
	{
		const TArray<FRTIntentView> Ally = URTIntentPrivacyLibrary::FilterForTeam(/*Observer*/ 0, Intents);
		if (!TestEqual(TEXT("l'alleato riceve la vista"), Ally.Num(), 1)) { return false; }
		TestTrue(TEXT("riconosciuta come alleata"), Ally[0].bIsAlly);
		TestFalse(TEXT("l'alleato vede la reazione pronta"), Ally[0].ReactionName.IsEmpty());
		TestEqual(TEXT("e vede i waypoint del piano"), Ally[0].PlannedWaypoints.Num(), 1);
	}

	// L'AVVERSARIO non rivelato non riceve NIENTE: non una vista con i campi vuoti, proprio nessuna riga.
	// Non deve nemmeno sapere che un piano esiste.
	{
		const TArray<FRTIntentView> Enemy = URTIntentPrivacyLibrary::FilterForTeam(/*Observer*/ 1, Intents);
		TestEqual(TEXT("nessuna vista per l'avversario non rivelato"), Enemy.Num(), 0);
	}

	// L'AVVERSARIO su un'unita' RIVELATA riceve l'intento, ma la reazione resta fuori: `Reveal` mostra cosa
	// l'unita' sta per FARE, non cosa e' pronta a PARARE.
	{
		const TArray<FRTPlannedIntent> Revealed = { MakeFullIntent(/*TeamId*/ 0, /*bRevealed*/ true) };
		const TArray<FRTIntentView> Enemy = URTIntentPrivacyLibrary::FilterForTeam(/*Observer*/ 1, Revealed);
		if (!TestEqual(TEXT("l'avversario riceve la vista di un'unita' rivelata"), Enemy.Num(), 1)) { return false; }
		TestFalse(TEXT("non e' un'alleata"), Enemy[0].bIsAlly);
		TestFalse(TEXT("l'intento c'e': l'azione e' visibile"), Enemy[0].ActionName.IsEmpty());
		TestTrue(TEXT("la destinazione e' visibile"), Enemy[0].PlannedCell == FRTCellId(2, 0));

		// Il cuore del checkpoint.
		TestTrue(TEXT("la REAZIONE non raggiunge l'avversario, nemmeno se rivelato"),
			Enemy[0].ReactionName.IsEmpty());
		TestEqual(TEXT("nemmeno i waypoint, che sono autoria del piano"), Enemy[0].PlannedWaypoints.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIntentPrivacyDeadAndOrderTest,
	"RefactorTactics.Reactions.IntentViewSkipsDeadAndKeepsOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIntentPrivacyDeadAndOrderTest::RunTest(const FString&)
{
	// Un'unita' eliminata non ha un piano da mostrare, nemmeno ai suoi.
	{
		FRTPlannedIntent Dead = MakeFullIntent(0, false);
		Dead.bAlive = false;
		const TArray<FRTIntentView> Views = URTIntentPrivacyLibrary::FilterForTeam(0, { Dead });
		TestEqual(TEXT("nessuna vista per un'unita' eliminata"), Views.Num(), 0);
	}

	// L'ordine dell'input si conserva: il filtro seleziona, non riordina — cosi' due osservatori diversi
	// vedono le stesse unita' nello stesso ordine relativo.
	{
		FRTPlannedIntent A = MakeFullIntent(0, false); A.OwnerCell = FRTCellId(1, 0);
		FRTPlannedIntent B = MakeFullIntent(1, true);  B.OwnerCell = FRTCellId(2, 0);
		FRTPlannedIntent C = MakeFullIntent(0, false); C.OwnerCell = FRTCellId(3, 0);

		const TArray<FRTIntentView> Views = URTIntentPrivacyLibrary::FilterForTeam(0, { A, B, C });
		if (!TestEqual(TEXT("due alleate piu' un nemico rivelato"), Views.Num(), 3)) { return false; }
		TestTrue(TEXT("ordine conservato (1)"), Views[0].OwnerCell == FRTCellId(1, 0));
		TestTrue(TEXT("ordine conservato (2)"), Views[1].OwnerCell == FRTCellId(2, 0));
		TestTrue(TEXT("ordine conservato (3)"), Views[2].OwnerCell == FRTCellId(3, 0));
		TestFalse(TEXT("il nemico rivelato in mezzo non porta la sua reazione"), !Views[1].ReactionName.IsEmpty());
	}

	// Simmetria: la stessa scena vista dall'altra squadra da' l'esito speculare. La privacy non e' una
	// proprieta' del team 0.
	{
		const TArray<FRTPlannedIntent> Both = {
			MakeFullIntent(/*TeamId*/ 0, /*bRevealed*/ false),
			MakeFullIntent(/*TeamId*/ 1, /*bRevealed*/ false)
		};
		const TArray<FRTIntentView> AsTeam0 = URTIntentPrivacyLibrary::FilterForTeam(0, Both);
		const TArray<FRTIntentView> AsTeam1 = URTIntentPrivacyLibrary::FilterForTeam(1, Both);
		TestEqual(TEXT("il team 0 vede solo i suoi"), AsTeam0.Num(), 1);
		TestEqual(TEXT("il team 1 vede solo i suoi"), AsTeam1.Num(), 1);
		TestTrue(TEXT("ciascuno vede la propria reazione"),
			!AsTeam0[0].ReactionName.IsEmpty() && !AsTeam1[0].ReactionName.IsEmpty());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
