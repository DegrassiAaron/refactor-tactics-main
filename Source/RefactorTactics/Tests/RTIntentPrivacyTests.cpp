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

namespace
{
	/** Piano SPOGLIO: un'unita' ferma che non fa niente. Base su cui accendere una condizione per volta. */
	FRTPlannedIntent MakeIdleIntent(int32 TeamId)
	{
		FRTPlannedIntent I;
		I.OwnerCell = FRTCellId(0, 0);
		I.TeamId = TeamId;
		I.bAlive = true;
		return I;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIntentCertaintyClassificationTest,
	"RefactorTactics.UI.IntentCertaintyClassification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIntentCertaintyClassificationTest::RunTest(const FString&)
{
	// CP 11.2. Un livello per volta, accendendo una sola condizione sul piano spoglio: cosi' il test dice
	// QUALE campo decide, non solo che la terna esiste.

	// CONFERMATO — niente puo' cambiarlo: l'unita' sta ferma e non punta niente.
	{
		const TArray<FRTIntentView> V = URTIntentPrivacyLibrary::FilterForTeam(0, { MakeIdleIntent(0) });
		if (!TestEqual(TEXT("l'alleato fermo produce una vista"), V.Num(), 1)) { return false; }
		TestEqual(TEXT("fermo e senza bersaglio: confermato"), V[0].Certainty, ERTIntentCertainty::Confirmed);
	}

	// PREVISTO — c'e' un bersaglio, ma l'unita' non si sposta: vale nello snapshot corrente.
	{
		FRTPlannedIntent Aiming = MakeIdleIntent(0);
		Aiming.bHasTarget = true;
		Aiming.TargetCell = FRTCellId(3, 0);
		const TArray<FRTIntentView> V = URTIntentPrivacyLibrary::FilterForTeam(0, { Aiming });
		if (!TestEqual(TEXT("una vista"), V.Num(), 1)) { return false; }
		TestEqual(TEXT("bersaglio senza movimento: previsto"), V[0].Certainty, ERTIntentCertainty::Predicted);
	}

	// INCERTO — muoversi basta: le celle del percorso sono contendibili e il resolver puo' troncare.
	{
		FRTPlannedIntent Moving = MakeIdleIntent(0);
		Moving.bMoving = true;
		Moving.PlannedCell = FRTCellId(2, 0);
		const TArray<FRTIntentView> V = URTIntentPrivacyLibrary::FilterForTeam(0, { Moving });
		if (!TestEqual(TEXT("una vista"), V.Num(), 1)) { return false; }
		TestEqual(TEXT("in movimento: incerto"), V[0].Certainty, ERTIntentCertainty::Uncertain);
	}

	// Lo SCATTO conta come movimento anche senza `bMoving`: e' una rotta, e ha le stesse celle contendibili.
	// Sono due flag distinti in `FRTPlannedIntent` e guardarne uno solo lascerebbe lo scatto «confermato».
	{
		FRTPlannedIntent Dashing = MakeIdleIntent(0);
		Dashing.bDashing = true;
		Dashing.DashCell = FRTCellId(1, 1);
		const TArray<FRTIntentView> V = URTIntentPrivacyLibrary::FilterForTeam(0, { Dashing });
		if (!TestEqual(TEXT("una vista"), V.Num(), 1)) { return false; }
		TestEqual(TEXT("in scatto: incerto"), V[0].Certainty, ERTIntentCertainty::Uncertain);
	}

	// La REAZIONE ha il suo livello, e DIVERGE dal piano: e' il caso che rende `ReactionCertainty` un dato
	// invece di una costante. Il piano di un'unita' ferma e' confermato; la reazione che tiene pronta
	// aspetta un trigger che decide l'avversario, quindi no.
	{
		FRTPlannedIntent Armed = MakeIdleIntent(0);
		Armed.ReactionName = FText::FromString(TEXT("Contrattacco"));
		const TArray<FRTIntentView> V = URTIntentPrivacyLibrary::FilterForTeam(0, { Armed });
		if (!TestEqual(TEXT("una vista"), V.Num(), 1)) { return false; }
		TestEqual(TEXT("il piano resta confermato"), V[0].Certainty, ERTIntentCertainty::Confirmed);
		TestEqual(TEXT("ma la reazione armata e' incerta"), V[0].ReactionCertainty,
			ERTIntentCertainty::Uncertain);
	}

	// Senza reazione il campo non afferma niente: resta al default, e il widget non ha una riga da disegnare
	// perche' `ReactionName` e' vuota.
	{
		const TArray<FRTIntentView> V = URTIntentPrivacyLibrary::FilterForTeam(0, { MakeIdleIntent(0) });
		if (!TestEqual(TEXT("una vista"), V.Num(), 1)) { return false; }
		TestTrue(TEXT("nessuna reazione da qualificare"), V[0].ReactionName.IsEmpty());
		TestEqual(TEXT("il livello della reazione resta al default"), V[0].ReactionCertainty,
			ERTIntentCertainty::Confirmed);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTNoEnemyIntentExposedTest,
	"RefactorTactics.UI.NoEnemyIntentExposed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTNoEnemyIntentExposedTest::RunTest(const FString&)
{
	// 🔴 Il test che pinna la DECISIONE DI DESIGN, non l'implementazione.
	//
	// Derivare la certezza dai piani avversari sarebbe il calcolo piu' naturale — `FilterForTeam` li riceve
	// tutti — e produrrebbe un CANALE LATERALE: un tratteggio che compare solo quando un nemico incrocia la
	// mia rotta gli dice dove si trova. L'invariante #6 vieta di esporre l'intento avversario, e una
	// deduzione affidabile e' un'esposizione: il DoD scrive che «l'occultamento non e' grafico».
	//
	// ⚠️ L'alleato di questa scena sta FERMO, ed e' una scelta senza la quale il test non morderebbe.
	// La prima stesura lo faceva muovere: sarebbe partito gia' `Uncertain`, cioe' dal livello massimo, e un
	// canale laterale che ALZA il livello non avrebbe avuto niente da alzare. Il test sarebbe rimasto verde
	// su un leak reale. Partendo da `Confirmed` ogni contaminazione si vede.
	// Trovato con la verifica di mutazione, non ragionando sul codice.
	FRTPlannedIntent Ally = MakeIdleIntent(0);

	// Un nemico che gli marcia addosso, lo punta e tiene pronta un'intercettazione: e' la scena piu'
	// "incerta" immaginabile, ed e' esattamente cio' che NON deve trasparire dal livello dell'alleato.
	FRTPlannedIntent Enemy = MakeIdleIntent(1);
	Enemy.bMoving = true;
	Enemy.PlannedCell = FRTCellId(0, 0); // la cella su cui l'alleato sta fermo: collisione
	Enemy.PlannedPath = { FRTCellId(2, 0), FRTCellId(1, 0), FRTCellId(0, 0) };
	Enemy.bHasTarget = true;
	Enemy.TargetCell = FRTCellId(0, 0);
	Enemy.ReactionName = FText::FromString(TEXT("Intercetta"));

	const TArray<FRTIntentView> Alone = URTIntentPrivacyLibrary::FilterForTeam(0, { Ally });
	const TArray<FRTIntentView> Contested = URTIntentPrivacyLibrary::FilterForTeam(0, { Ally, Enemy });

	if (!TestEqual(TEXT("scena senza nemici: una vista"), Alone.Num(), 1)) { return false; }
	if (!TestEqual(TEXT("il nemico non rivelato non aggiunge righe"), Contested.Num(), 1)) { return false; }

	// Il valore di partenza e' ancorato: se un giorno `MakeIdleIntent` cambiasse e l'alleato nascesse gia'
	// `Uncertain`, i due confronti qui sotto resterebbero verdi confrontando due massimi. Questa riga fa
	// fallire il test invece di lasciarlo smettere di verificare in silenzio.
	if (!TestEqual(TEXT("l'alleato fermo parte da confermato, o il test non morde"),
		Alone[0].Certainty, ERTIntentCertainty::Confirmed)) { return false; }

	// Il cuore: la classificazione non si muove di un livello.
	TestEqual(TEXT("la certezza del piano e' cieca ai piani nemici"),
		Contested[0].Certainty, Alone[0].Certainty);
	TestEqual(TEXT("e quella della reazione pure"),
		Contested[0].ReactionCertainty, Alone[0].ReactionCertainty);

	// E il nemico RIVELATO non porta comunque il proprio livello di reazione: `Reveal` mostra cosa un'unita'
	// sta per fare, non cosa e' pronta a parare — quindi non c'e' una reazione da qualificare.
	{
		FRTPlannedIntent RevealedEnemy = Enemy;
		RevealedEnemy.bRevealed = true;
		const TArray<FRTIntentView> V = URTIntentPrivacyLibrary::FilterForTeam(0, { RevealedEnemy });
		if (!TestEqual(TEXT("il nemico rivelato produce una vista"), V.Num(), 1)) { return false; }
		TestTrue(TEXT("senza la sua reazione"), V[0].ReactionName.IsEmpty());
		TestEqual(TEXT("e senza un livello che la qualifichi"), V[0].ReactionCertainty,
			ERTIntentCertainty::Confirmed);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
