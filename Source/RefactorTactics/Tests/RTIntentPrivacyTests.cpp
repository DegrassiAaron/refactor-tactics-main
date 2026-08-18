#include "Misc/AutomationTest.h"
#include "Turn/RTIntentPrivacyLibrary.h"
// La RESA dei tre livelli (CP 11.2, passo 4) vive su `ARTHUD` come statica pura: si verifica senza montare
// una partita, che e' cio' che il DoD intende con «test headless su `ARTHUD`».
#include "UI/RTHUD.h"

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

	// La REAZIONE non ha piu' un livello proprio, e la coppia di casi qui sotto pinna cio' che resta vero:
	// il piano di un'unita' ferma e' `Confirmed` **anche quando tiene pronto un contrattacco**, e l'unica
	// cosa che distingue «reazione armata» da «nessuna reazione» e' `ReactionName`.
	//
	// 🔴 Qui c'erano due assert su `ReactionCertainty`, e insieme misuravano una costante: il campo valeva
	// `Uncertain` con la reazione e — dopo il primo fix — `Uncertain` anche senza, cioe' i due blocchi
	// raggiungevano lo stesso valore per strade diverse e l'armamento della reazione era causalmente
	// irrilevante. Il campo e' uscito dal DTO; questi due casi restano perche' la loro domanda vera —
	// «armare una reazione cambia la certezza del PIANO?» — ha ancora una risposta, ed e' no.
	{
		FRTPlannedIntent Armed = MakeIdleIntent(0);
		Armed.ReactionName = FText::FromString(TEXT("Contrattacco"));
		const TArray<FRTIntentView> V = URTIntentPrivacyLibrary::FilterForTeam(0, { Armed });
		if (!TestEqual(TEXT("una vista"), V.Num(), 1)) { return false; }
		TestEqual(TEXT("il piano resta confermato"), V[0].Certainty, ERTIntentCertainty::Confirmed);
		TestFalse(TEXT("e l'alleato riceve la reazione"), V[0].ReactionName.IsEmpty());
	}
	{
		const TArray<FRTIntentView> V = URTIntentPrivacyLibrary::FilterForTeam(0, { MakeIdleIntent(0) });
		if (!TestEqual(TEXT("una vista"), V.Num(), 1)) { return false; }
		TestEqual(TEXT("stesso livello di piano senza reazione"), V[0].Certainty,
			ERTIntentCertainty::Confirmed);
		TestTrue(TEXT("e nessuna reazione da mostrare"), V[0].ReactionName.IsEmpty());
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// CP 11.2: il DTO non popolato deve promettere il MENO possibile, non il piu'
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIntentViewSafeDefaultsTest,
	"RefactorTactics.UI.IntentViewDefaultsToUnknown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIntentViewSafeDefaultsTest::RunTest(const FString&)
{
	// Il default di `Certainty` era `Confirmed`: un campo mai popolato affermava la garanzia PIU' FORTE del
	// dominio. Ogni percorso vivo lo assegna (`FilterForTeam` chiama `ClassifyPlan`), quindi il difetto non
	// e' osservabile in partita — lo diventa in rete (M10) o al primo widget Blueprint.
	//
	// 🔴 **Un initializer non bastava, e questo test lo dimostra su DUE strade.** La prima correzione mise
	// `= Uncertain` sul membro: difendeva la costruzione C++, che non era mai a rischio, e lasciava intatto
	// il caso vero — `Confirmed` era ancora l'enumeratore **zero**, quindi qualunque memoria azzerata lo
	// rileggeva. Serviva un valore che *significhi* «mai calcolato» **e** valga zero.
	const FRTIntentView Fresh;
	TestEqual(TEXT("il DTO costruito non afferma nessun livello"),
		Fresh.Certainty, ERTIntentCertainty::Unknown);

	// ⚠️ La seconda strada e' quella che l'initializer NON copre: memoria azzerata, come la produce una
	// variabile Blueprint, un `Memzero` o un `SetNumZeroed`. Qui il membro non viene inizializzato dal
	// costruttore C++, quindi il test misura il valore dell'ENUM e non quello della struct.
	ERTIntentCertainty Zeroed;
	FMemory::Memzero(&Zeroed, sizeof(Zeroed));
	TestEqual(TEXT("e la memoria azzerata nemmeno"), Zeroed, ERTIntentCertainty::Unknown);

	// La garanzia in una riga: lo zero del tipo non e' un livello disegnabile.
	TestNotEqual(TEXT("lo zero del tipo non e' «confermato»"),
		Zeroed, ERTIntentCertainty::Confirmed);
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

	// 🔴 Qui c'era un secondo confronto, su `ReactionCertainty`, e la code review ha mostrato che stava per
	// smettere di mordere: col default portato a `Uncertain` i due lati partivano entrambi dal livello
	// MASSIMO, e un canale laterale puo' solo alzare — non restava niente da alzare. E' lo stesso difetto
	// che l'ancora tre righe sopra previene per il piano, e per la reazione non c'era. Il campo e' poi
	// uscito dal DTO; se un giorno torna, torna anche il confronto **con la sua ancora**.

	// E il nemico RIVELATO non porta comunque la propria reazione: `Reveal` mostra cosa un'unita' sta per
	// FARE, non cosa e' pronta a PARARE.
	{
		FRTPlannedIntent RevealedEnemy = Enemy;
		RevealedEnemy.bRevealed = true;
		const TArray<FRTIntentView> V = URTIntentPrivacyLibrary::FilterForTeam(0, { RevealedEnemy });
		if (!TestEqual(TEXT("il nemico rivelato produce una vista"), V.Num(), 1)) { return false; }
		TestTrue(TEXT("senza la sua reazione"), V[0].ReactionName.IsEmpty());
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// CP 11.2, passo 4: la RESA. I tre livelli devono arrivare a schermo distinguibili.
//
// ⚠️ **Perche' questi test stanno qui e non in `RTHUDMarksTests.cpp`**, che sarebbe la casa naturale di una
// statica di `ARTHUD`: quel file non e' nel `writable` di nessuna track del batch corrente, e per `D-139` un
// path non assegnato e' STOP. Questo file e' di `client_tools`, che possiede `#78`, e ospita gia' i tre test
// di certezza — quindi la scelta e' anche coerente, non solo permessa. Precedente identico e dichiarato:
// `overwatch_lifecycle` ha messo il pin di `#166` in `RTReactionOpportunityTests.cpp` per non toccare un file
// di `simulation`. Se un giorno `RTHUDMarksTests.cpp` viene assegnato a questa track, questi due si spostano.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIntentCertaintyRenderingTest,
	"RefactorTactics.UI.IntentCertaintyRendering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIntentCertaintyRenderingTest::RunTest(const FString&)
{
	// Il DoD chiede che «tre viste con `Certainty` diversa producano un output osservabilmente diverso». Il
	// verbo che conta e' **diverso**, non «corretto»: ecco perche' il cuore del test sono i confronti a
	// COPPIE e non tre confronti con valori attesi.
	//
	// 🔴 **Senza i confronti a coppie questo test sarebbe VACUO, e per una ragione precisa.** I valori di
	// costruzione di `FRTIntentCertaintyStyle` — opacita' `1`, nessun tratteggio, nessun `?` — coincidono
	// esattamente con lo stile di `Confirmed`. Una `ComposeIntentCertaintyStyle` che ignorasse l'input e
	// restituisse `FRTIntentCertaintyStyle{}` passerebbe qualunque assert scritto come «Confirmed vale 1,
	// niente tratteggio, niente `?`». I confronti a coppie la fanno fallire su due righe.
	auto ViewWith = [](ERTIntentCertainty Level)
	{
		FRTIntentView V;
		V.bIsAlly = true;
		V.Certainty = Level;
		return V;
	};

	const FRTIntentCertaintyStyle Confirmed = ARTHUD::ComposeIntentCertaintyStyle(ViewWith(ERTIntentCertainty::Confirmed));
	const FRTIntentCertaintyStyle Predicted = ARTHUD::ComposeIntentCertaintyStyle(ViewWith(ERTIntentCertainty::Predicted));
	const FRTIntentCertaintyStyle Uncertain = ARTHUD::ComposeIntentCertaintyStyle(ViewWith(ERTIntentCertainty::Uncertain));

	// 1. I tre livelli sono distinguibili A DUE A DUE. Una scala a tre valori che ne rendesse due allo stesso
	//    modo sarebbe una scala a due, e il giocatore non potrebbe leggere la differenza che la issue esiste
	//    per mostrargli.
	TestNotEqual(TEXT("confermato e previsto non si disegnano uguali"),
		Confirmed.GhostOpacity, Predicted.GhostOpacity);
	TestNotEqual(TEXT("previsto e incerto non si disegnano uguali"),
		Predicted.GhostOpacity, Uncertain.GhostOpacity);
	TestNotEqual(TEXT("confermato e incerto non si disegnano uguali"),
		Confirmed.GhostOpacity, Uncertain.GhostOpacity);

	// 2. E la grammatica del 2026-08-07 nei suoi tre elementi, uno per livello.
	TestFalse(TEXT("confermato: linea piena"), Confirmed.bDashedLine);
	TestTrue(TEXT("previsto: linea tratteggiata"), Predicted.bDashedLine);
	TestTrue(TEXT("confermato e' il ghost pienamente leggibile"), Confirmed.GhostOpacity >= 1.f);
	TestTrue(TEXT("previsto e' attenuato, non dissolto"),
		Predicted.GhostOpacity < Confirmed.GhostOpacity && Predicted.GhostOpacity > Uncertain.GhostOpacity);

	// 3. Il `?` appartiene al SOLO livello incerto. E' l'elemento piu' facile da spargere ovunque «per
	//    prudenza», e un `?` su tutto non dice piu' niente.
	TestTrue(TEXT("confermato: nessun ?"), Confirmed.Mark.IsEmpty());
	TestTrue(TEXT("previsto: nessun ?"), Predicted.Mark.IsEmpty());
	TestEqual(TEXT("incerto: il punto interrogativo"), Uncertain.Mark, FString(TEXT("?")));

	// 4. 🔴 **La UI NON ricalcola**, ed e' l'invariante #1 della issue. Questa vista si CONTRADDICE apposta:
	//    porta `bMoving`, `bDashing` e un bersaglio — cioe' tutto cio' che farebbe dire «incerto» a chiunque
	//    riclassificasse — ma il livello calcolato dal simulatore dice `Confirmed`. Una `ComposeIntentCertaintyStyle`
	//    che guardasse i flag invece del campo produrrebbe qui lo stile incerto, e questo assert cadrebbe.
	//    Senza questo caso, una copia della regola dentro la HUD resterebbe verde: e' il precedente
	//    `IsIntentVisibleTo` (#507), dove la divergenza non la vide nessuno.
	{
		FRTIntentView Contradictory = ViewWith(ERTIntentCertainty::Confirmed);
		Contradictory.bMoving = true;
		Contradictory.bDashing = true;
		Contradictory.bHasTarget = true;
		Contradictory.PlannedCell = FRTCellId(5, 0);
		const FRTIntentCertaintyStyle S = ARTHUD::ComposeIntentCertaintyStyle(Contradictory);
		TestEqual(TEXT("la resa segue il livello, non i flag del piano"), S.GhostOpacity, Confirmed.GhostOpacity);
		TestTrue(TEXT("e non inventa un ? che il livello non chiede"), S.Mark.IsEmpty());
	}

	// 5. `Unknown` non deve MAI ricevere la resa di `Confirmed`: e' il difetto per cui vale zero. Un campo mai
	//    calcolato che ereditasse il ghost pieno affermerebbe a schermo la garanzia piu' forte del dominio.
	{
		const FRTIntentCertaintyStyle S = ARTHUD::ComposeIntentCertaintyStyle(ViewWith(ERTIntentCertainty::Unknown));
		TestNotEqual(TEXT("il livello mai calcolato non si disegna come confermato"),
			S.GhostOpacity, Confirmed.GhostOpacity);
		TestTrue(TEXT("e non promette una linea piena"), S.bDashedLine);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArmedReactionRenderingTest,
	"RefactorTactics.UI.ArmedReactionRendering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArmedReactionRenderingTest::RunTest(const FString&)
{
	// Il DoD: «la presenza di una reazione armata e' resa con la grammatica incerto, e la resa si abilita su
	// `ReactionName`, MAI su `ReactionCertainty`». La seconda meta' e' oggi indimostrabile per costruzione —
	// quel campo non esiste piu' — quindi cio' che resta verificabile e' la prima: il segnale e' il NOME.
	FRTIntentView Armed;
	Armed.bIsAlly = true;
	Armed.Certainty = ERTIntentCertainty::Confirmed;
	Armed.ReactionName = FText::FromString(TEXT("Contrattacco"));

	FRTIntentView Bare;
	Bare.bIsAlly = true;
	Bare.Certainty = ERTIntentCertainty::Confirmed;

	const FRTIntentCertaintyStyle WithReaction = ARTHUD::ComposeIntentCertaintyStyle(Armed);
	const FRTIntentCertaintyStyle NoReaction = ARTHUD::ComposeIntentCertaintyStyle(Bare);

	TestTrue(TEXT("il nome pieno arma la resa della reazione"), WithReaction.bReactionArmed);
	TestFalse(TEXT("il nome vuoto no"), NoReaction.bReactionArmed);

	// ⚠️ **I due assert qui sopra da soli non basterebbero**, ed e' lo stesso difetto che fece togliere
	// `ReactionCertainty` dal DTO: misurerebbero che un booleano copia un `IsEmpty()`. La domanda che conta e'
	// se armare una reazione CAMBIA qualcosa nella resa — cioe' se il segnale e' causalmente vivo.
	TestNotEqual(TEXT("armare una reazione cambia cio' che si disegna"),
		WithReaction.bReactionArmed, NoReaction.bReactionArmed);

	// E la reazione e' un SECONDO asse: non contamina il livello del piano, che resta quello che il simulatore
	// ha calcolato. Lo pinna gia' `IntentCertaintyClassification` sul DTO; qui si pinna sulla resa.
	TestEqual(TEXT("e non tocca il ghost del piano"), WithReaction.GhostOpacity, NoReaction.GhostOpacity);
	TestEqual(TEXT("ne' la sua linea"), WithReaction.bDashedLine, NoReaction.bDashedLine);

	// Una reazione armata su un piano INCERTO resta armata: i due assi sono indipendenti in entrambi i versi.
	{
		FRTIntentView MovingArmed = Armed;
		MovingArmed.Certainty = ERTIntentCertainty::Uncertain;
		const FRTIntentCertaintyStyle S = ARTHUD::ComposeIntentCertaintyStyle(MovingArmed);
		TestTrue(TEXT("reazione armata anche su piano incerto"), S.bReactionArmed);
		TestEqual(TEXT("col ghost del proprio livello"), S.Mark, FString(TEXT("?")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
