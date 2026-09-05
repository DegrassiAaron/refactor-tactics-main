#include "Misc/AutomationTest.h"
#include "Turn/RTIntentPrivacyLibrary.h"
// `#2331`: la partizione dei campi si legge dalla REFLECTION, non da un elenco scritto a mano.
#include "UObject/UnrealType.h"
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
// #2331 — la privacy dei CAMPI, non dei valori: il difetto e' il QUINTO campo
//
// Le quattro righe dentro `if (bIsAlly)` hanno gia' quattro guardie, e le hanno tutte: `ReactionName` e
// `PlannedWaypoints` qui sopra (`:78-80`), `bDeclaresRotation` e `DeclaredFacing` in
// `RefactorTactics.Facing.IntentIsTeamFiltered` (`RTFacingTests.cpp:525-526`, dal 2026-08-09) — che
// `adr-0005` §Verifica elenca gia' come il test normativo della rotazione dichiarata.
//
// 🔴 **Ma sono quattro asserzioni puntuali, e nessuna di esse e' il meccanismo che genera la quinta.** Un
// `View.NuovoCampo = Intent.NuovoCampo;` scritto domani FUORI dal ramo passa l'intero corpus in verde: le
// quattro guardie esistono per fortuna, non per costruzione. E' la stessa domanda che `#1805` ha posto al
// TurnLog — *«un campo audit-only aggiunto al modello non deve poter finire nell'export pubblico senza far
// diventare rosso un test»* — e la risposta ha la stessa forma: non si guardano dei valori, si guardano dei
// CAMPI, **obbligando a classificare**. Il precedente e' `RTReplayPrivacyTests.cpp:87-126`.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/**
	 * Da dove viene un campo di `FRTIntentView`.
	 *
	 * ⚠️ **Questa tabella vive nel TEST, e non deve migrare in produzione.** `RTReplayPrivacyTests` legge la
	 * propria da `URTReplayPrivacyLibrary::FieldVisibility()` perche' li' la classificazione GUIDA l'export:
	 * e' il produttore. Qui il produttore e' il corpo di `FilterForTeam`, e una seconda tabella che dicesse
	 * la stessa regola sarebbe una copia libera di divergere — il difetto di `#507`, dove una regola
	 * riscritta inline restava verde su una funzione che nessuno chiamava. Qui e' un'asserzione ESTERNA su
	 * un produttore unico; li' sarebbe un secondo produttore.
	 */
	enum class ERTIntentViewFieldClass : uint8
	{
		/** Copiato dall'intento per chiunque riceva una vista, avversario rivelato incluso. */
		Public,
		/** Copiato solo dentro `if (bIsAlly)`. Per un avversario resta al proprio default. */
		AllyOnly,
		/**
		 * Non copiato da `FRTPlannedIntent`: calcolato mentre la vista si costruisce.
		 *
		 * ⚠️ Sono due, ed e' la ragione per cui le classi sono tre e non due. `bIsAlly` nasce dal confronto
		 * fra le squadre e vale `false` — cioe' il proprio default — in **ogni** vista avversaria, del tutto
		 * correttamente: classificarlo `Public` renderebbe falso il terzo blocco del test qui sotto.
		 * `Certainty` viene da `ClassifyPlan`, e la sua cecita' ai piani altrui e' proprieta' di
		 * `NoEnemyIntentExposed`, non di una tabella di copia. Le loro guardie puntuali esistono gia'
		 * (`:53, :71` e `IntentCertaintyClassification`).
		 */
		Derived
	};

	struct FRTIntentFieldRow
	{
		FName Field;
		ERTIntentViewFieldClass Class;
	};

	/**
	 * La classe di OGNI campo di `FRTIntentView`. Un campo aggiunto al DTO e non aggiunto qui fa rosso
	 * `IntentViewFieldsAreClassified`: e' l'unico scopo di questo elenco.
	 *
	 * 🔴 **In un array e non direttamente in una `TMap`, per la ragione che `RTReplayPrivacyLibrary.cpp:16-18`
	 * ha gia' pagato**: una `TMap` costruita da initializer list INGOIA una chiave duplicata, e l'ultima riga
	 * vince in silenzio. Un campo elencato due volte in due classi diverse passerebbe ogni gate — `Contains`
	 * e' vero, non ci sono fantasmi, i conteggi tornano — mentre chi legge il file lo vede nella classe
	 * sbagliata. Con l'array la duplicazione e' misurabile, ed e' misurata.
	 *
	 * `GET_MEMBER_NAME_CHECKED` e non un `FName` letterale: un campo RINOMINATO deve rompere la
	 * COMPILAZIONE, non lasciare qui un nome che non esiste piu'.
	 */
	const TArray<FRTIntentFieldRow>& IntentViewFieldRows()
	{
		static const TArray<FRTIntentFieldRow> Rows = {
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, OwnerCell),         ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, bIsAlly),           ERTIntentViewFieldClass::Derived },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, bMoving),           ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, PlannedCell),       ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, ActionName),        ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, bHasTarget),        ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, TargetCell),        ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, ReactionName),      ERTIntentViewFieldClass::AllyOnly },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, PlannedPath),       ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, PlannedWaypoints),  ERTIntentViewFieldClass::AllyOnly },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, bDashing),          ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, DashCell),          ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, DashStyle),         ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, Facing),            ERTIntentViewFieldClass::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, bDeclaresRotation), ERTIntentViewFieldClass::AllyOnly },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, DeclaredFacing),    ERTIntentViewFieldClass::AllyOnly },
			{ GET_MEMBER_NAME_CHECKED(FRTIntentView, Certainty),         ERTIntentViewFieldClass::Derived }
		};
		return Rows;
	}

	/** Le stesse righe indicizzate per nome. Un duplicato si perde QUI, ed e' per questo che si contano. */
	const TMap<FName, ERTIntentViewFieldClass>& IntentViewFieldClasses()
	{
		static const TMap<FName, ERTIntentViewFieldClass> Table = []
		{
			TMap<FName, ERTIntentViewFieldClass> Built;
			for (const FRTIntentFieldRow& Row : IntentViewFieldRows())
			{
				Built.Add(Row.Field, Row.Class);
			}
			return Built;
		}();
		return Table;
	}

	/** Nomi ordinati, per un messaggio di fallimento che dica QUALE campo e non solo che ce n'e' uno. */
	FString ListedIntentFields(const TSet<FName>& Names)
	{
		TArray<FString> As;
		for (const FName& N : Names) { As.Add(N.ToString()); }
		As.Sort();
		return FString::Join(As, TEXT(", "));
	}

	/** I campi di `FRTIntentView` in una data classe. */
	TSet<FName> IntentViewFieldsOfClass(ERTIntentViewFieldClass Wanted)
	{
		TSet<FName> Out;
		for (const TPair<FName, ERTIntentViewFieldClass>& Row : IntentViewFieldClasses())
		{
			if (Row.Value == Wanted) { Out.Add(Row.Key); }
		}
		return Out;
	}

	/** Vero se il campo `P` della vista `V` vale ancora il default della struttura: non e' arrivato. */
	bool IntentViewFieldIsDefault(const FProperty* P, const FRTIntentView& V)
	{
		static const FRTIntentView Default;
		return P->Identical(P->ContainerPtrToValuePtr<void>(&V), P->ContainerPtrToValuePtr<void>(&Default));
	}

	/**
	 * Un intento con un valore DIVERSO dal proprio default in ogni campo che raggiunge il DTO.
	 *
	 * 🔴 **La saturazione e' il test, non un dettaglio della fixture.** Su un campo lasciato al default,
	 * *«non e' arrivato»* e *«e' arrivato e valeva il default»* sono indistinguibili, e l'asserzione di
	 * privacy e' verde per la ragione sbagliata. E' la lezione di `SaturatedEntry()`
	 * (`RTReplayPrivacyTests.cpp:44-49`) e, in questo stesso file, quella dell'ancora di
	 * `NoEnemyIntentExposed` (`:288-289`).
	 *
	 * ⚠️ Non sostituisce `MakeFullIntent`, che NON e' saturo (`:21-39`: niente `Facing`, `DashStyle`,
	 * `bDeclaresRotation`, `DeclaredFacing`) e su cui poggiano i test sopra.
	 *
	 * ⚠️ `DeclaredFacing` (`SE`) e' scelto diverso da `Facing` (`NW`) **e** dal default (`E`): se coincidesse
	 * con la posa pubblica, un `DeclaredFacing` copiato per sbaglio fuori dal ramo si confonderebbe con il
	 * valore lecito. E' la stessa cura di `RTFacingTests.cpp:502-504`.
	 */
	FRTPlannedIntent SaturatedIntentForPrivacy(int32 TeamId, bool bRevealed)
	{
		FRTPlannedIntent I;
		I.OwnerCell = FRTCellId(3, 1, 0);
		I.TeamId = TeamId;
		I.bAlive = true;
		I.bRevealed = bRevealed;
		I.bMoving = true;
		I.PlannedCell = FRTCellId(2, 0);
		I.ActionName = FText::FromString(TEXT("Tiro"));
		I.bHasTarget = true;
		I.TargetCell = FRTCellId(4, 0);
		I.ReactionName = FText::FromString(TEXT("Contrattacco"));
		I.PlannedPath = { FRTCellId(3, 1, 0), FRTCellId(2, 1), FRTCellId(2, 0) };
		I.PlannedWaypoints = { FRTCellId(2, 0) };
		I.bDashing = true;
		I.DashCell = FRTCellId(1, 1);
		I.DashStyle = ERTMovementStyle::LinearDash;
		I.Facing = ERTHexDirection::NW;
		I.bDeclaresRotation = true;
		I.DeclaredFacing = ERTHexDirection::SE;
		return I;
	}
}

/**
 * Ogni campo del DTO ha una classe DICHIARATA — il gate che #2331 chiede per nome.
 *
 * Non si ottiene guardando dei valori: si ottiene obbligando a classificare. Chi aggiunge una `UPROPERTY` a
 * `FRTIntentView` e non dice se e' pubblica, ally-only o derivata trova questo rosso, e la classificazione
 * diventa una decisione presa invece che un default subito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTIntentViewFieldsAreClassifiedTest,
	"RefactorTactics.UI.IntentViewFieldsAreClassified",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTIntentViewFieldsAreClassifiedTest::RunTest(const FString&)
{
	const TMap<FName, ERTIntentViewFieldClass>& Table = IntentViewFieldClasses();

	TSet<FName> Reflected;
	for (TFieldIterator<FProperty> It(FRTIntentView::StaticStruct()); It; ++It)
	{
		Reflected.Add(It->GetFName());
	}

	// ANTI-VACUITA': una tabella vuota, o una reflection che non vede niente, renderebbe verdi per assenza
	// di soggetto tutti i confronti qui sotto.
	TestTrue(TEXT("la tabella classifica almeno un campo"), Table.Num() > 0);
	TestTrue(TEXT("la reflection vede almeno un campo di FRTIntentView"), Reflected.Num() > 0);

	// Il cuore: nessun campo del DTO puo' restare senza classe.
	TSet<FName> Unclassified;
	for (const FName& N : Reflected)
	{
		if (!Table.Contains(N)) { Unclassified.Add(N); }
	}
	TestTrue(
		FString::Printf(TEXT("ogni campo di FRTIntentView e' classificato; non classificati: [%s]"),
			*ListedIntentFields(Unclassified)),
		Unclassified.Num() == 0);

	// Il difetto simmetrico: un nome nella tabella che non esiste piu' nella struttura. Qui
	// `GET_MEMBER_NAME_CHECKED` lo previene gia' in compilazione, ma un gate che dipende dall'aver usato la
	// macro giusta non e' un gate — e' una convenzione.
	TSet<FName> Ghosts;
	for (const TPair<FName, ERTIntentViewFieldClass>& Row : Table)
	{
		if (!Reflected.Contains(Row.Key)) { Ghosts.Add(Row.Key); }
	}
	TestTrue(
		FString::Printf(TEXT("la tabella non classifica campi inesistenti; fantasmi: [%s]"),
			*ListedIntentFields(Ghosts)),
		Ghosts.Num() == 0);

	// 🔴 Il duplicato NON si vede confrontando la mappa con la reflection: elencare `ReactionName` due volte
	// e in due classi diverse lascerebbe 17 chiavi su 17 campi, tutti i gate verdi, e la classe decisa
	// dall'ORDINE delle righe invece che da chi le ha scritte. Si vede solo contro l'array sorgente, dove il
	// duplicato esiste ancora.
	TestEqual(TEXT("nessun campo classificato due volte"), IntentViewFieldRows().Num(), Table.Num());

	// E la classificazione deve coprire la struttura esattamente: non un campo di meno, non uno di piu'.
	TestEqual(TEXT("una riga per campo del DTO"), Table.Num(), Reflected.Num());

	// E le tre classi devono essere tutte abitate, o due terzi del test qui sotto non avrebbero soggetto.
	TestTrue(TEXT("almeno un campo pubblico"),
		IntentViewFieldsOfClass(ERTIntentViewFieldClass::Public).Num() > 0);
	TestTrue(TEXT("almeno un campo ally-only"),
		IntentViewFieldsOfClass(ERTIntentViewFieldClass::AllyOnly).Num() > 0);
	TestTrue(TEXT("almeno un campo derivato"),
		IntentViewFieldsOfClass(ERTIntentViewFieldClass::Derived).Num() > 0);

	return true;
}

/**
 * Nessun campo ally-only raggiunge un avversario — misurato su TUTTI i campi ally-only, non su due.
 *
 * ⚠️ **Il terzo blocco e' la meta' che si dimentica sempre**, ed e' la stessa che `PublicFieldsKeepTheirValue`
 * aggiunge al replay (`RTReplayPrivacyTests.cpp:192-195`): un `FilterForTeam` che per eccesso di zelo
 * smettesse di copiare la rotta a un avversario rivelato passerebbe qualunque test di privacy — e non
 * sarebbe piu' un filtro, sarebbe un muro. `Status.Reveal` esiste per mostrare cosa un'unita' sta per FARE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTEnemyViewCarriesNoAllyOnlyFieldTest,
	"RefactorTactics.UI.EnemyViewCarriesNoAllyOnlyField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTEnemyViewCarriesNoAllyOnlyFieldTest::RunTest(const FString&)
{
	// Rivelato: e' il caso PIU' PERMISSIVO che la funzione conosce, quindi quello in cui un leak ha piu'
	// strade per passare. Su un avversario non rivelato non ci sarebbe nemmeno una riga da ispezionare.
	const TArray<FRTPlannedIntent> Intents = { SaturatedIntentForPrivacy(/*TeamId*/ 0, /*bRevealed*/ true) };

	const TArray<FRTIntentView> AsAlly = URTIntentPrivacyLibrary::FilterForTeam(/*Observer*/ 0, Intents);
	const TArray<FRTIntentView> AsEnemy = URTIntentPrivacyLibrary::FilterForTeam(/*Observer*/ 1, Intents);
	if (!TestEqual(TEXT("l'alleato riceve la vista"), AsAlly.Num(), 1)) { return false; }
	if (!TestEqual(TEXT("e l'avversario rivelato pure"), AsEnemy.Num(), 1)) { return false; }

	const TMap<FName, ERTIntentViewFieldClass>& Table = IntentViewFieldClasses();

	// 1. ANTI-VACUITA' — la fixture satura davvero, e il filtro riempie: sulla vista ALLEATA nessun campo,
	//    di nessuna classe, resta al proprio default. Senza questo blocco il punto 2 sarebbe verde anche su
	//    una `FilterForTeam` che non copia niente.
	TSet<FName> StillDefaultForAlly;
	for (TFieldIterator<FProperty> It(FRTIntentView::StaticStruct()); It; ++It)
	{
		if (IntentViewFieldIsDefault(*It, AsAlly[0])) { StillDefaultForAlly.Add(It->GetFName()); }
	}
	TestTrue(
		FString::Printf(TEXT("l'intento saturo non lascia nessun campo al default nella vista alleata; fermi: [%s]"),
			*ListedIntentFields(StillDefaultForAlly)),
		StillDefaultForAlly.Num() == 0);

	// 2. IL CUORE — ogni campo ally-only e' al default nella vista di un avversario: non e' spedito e
	//    nascosto, non e' proprio valorizzato (invariante #6). Sono i quattro di oggi e i quinti di domani.
	TSet<FName> LeakedToEnemy;
	TSet<FName> WronglyClearedForAlly;
	for (const FName& N : IntentViewFieldsOfClass(ERTIntentViewFieldClass::AllyOnly))
	{
		const FProperty* P = FRTIntentView::StaticStruct()->FindPropertyByName(N);
		if (!P) { continue; } // gia' rosso in `IntentViewFieldsAreClassified` come fantasma
		if (!IntentViewFieldIsDefault(P, AsEnemy[0])) { LeakedToEnemy.Add(N); }
		if (IntentViewFieldIsDefault(P, AsAlly[0])) { WronglyClearedForAlly.Add(N); }
	}
	TestTrue(
		FString::Printf(TEXT("nessun campo ally-only raggiunge un avversario rivelato; trapelati: [%s]"),
			*ListedIntentFields(LeakedToEnemy)),
		LeakedToEnemy.Num() == 0);
	TestTrue(
		FString::Printf(TEXT("e l'alleato li riceve tutti, o la privacy sarebbe un muro; mancanti: [%s]"),
			*ListedIntentFields(WronglyClearedForAlly)),
		WronglyClearedForAlly.Num() == 0);

	// 3. LA META' CHE SI DIMENTICA — un campo PUBBLICO arriva all'avversario rivelato con lo STESSO valore
	//    che ha per l'alleato. `Reveal` mostra l'intento; azzerare tutto passerebbe ogni assert di privacy.
	//    I `Derived` restano fuori, e non e' una scappatoia: `bIsAlly` vale correttamente `false` di la' e
	//    `true` di qua — sono la ragione per cui le classi sono tre.
	TSet<FName> DivergentPublic;
	for (const FName& N : IntentViewFieldsOfClass(ERTIntentViewFieldClass::Public))
	{
		const FProperty* P = FRTIntentView::StaticStruct()->FindPropertyByName(N);
		if (!P) { continue; }
		if (!P->Identical(P->ContainerPtrToValuePtr<void>(&AsEnemy[0]),
			P->ContainerPtrToValuePtr<void>(&AsAlly[0])))
		{
			DivergentPublic.Add(N);
		}
	}
	TestTrue(
		FString::Printf(TEXT("un campo pubblico vale lo stesso per l'alleato e per l'avversario rivelato; divergenti: [%s]"),
			*ListedIntentFields(DivergentPublic)),
		DivergentPublic.Num() == 0);

	// I tre cicli qui sopra iterano sulla TABELLA, non sulla struttura: un campo aggiunto al DTO e non
	// classificato non li farebbe fallire, li lascerebbe girare su un insieme piu' piccolo — cioe' questo
	// test smetterebbe di verificare in silenzio proprio nel caso che deve coprire.
	// `IntentViewFieldsAreClassified` lo misura, e questa riga fa in modo che questo test non dipenda dal
	// fatto che qualcuno non abbia cancellato quello.
	int32 ReflectedCount = 0;
	for (TFieldIterator<FProperty> It(FRTIntentView::StaticStruct()); It; ++It) { ++ReflectedCount; }
	TestEqual(TEXT("la tabella copre la struttura, o i cicli qui sopra girano su meno campi di quanti ce ne sono"),
		Table.Num(), ReflectedCount);

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
	// 🔴 **Senza i confronti a coppie questo test sarebbe VACUO se i default della struct coincidessero con
	// un livello**, ed e' esattamente com'era la prima stesura: i valori di costruzione erano quelli di
	// `Confirmed`, quindi una `ComposeIntentCertaintyStyle` che ignorasse l'input e restituisse
	// `FRTIntentCertaintyStyle{}` passava qualunque assert scritto come «Confirmed vale 1, niente
	// tratteggio, niente `?`». La code review l'ha trovato, e la correzione e' su DUE fronti: i default sono
	// diventati quelli del livello **incerto** — un valore mai calcolato non deve promettere la garanzia piu'
	// forte — e i confronti a coppie restano, perche' sono cio' che fa cadere una funzione costante
	// qualunque sia il livello su cui e' incollata.
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
	//
	// ⚠️ **Il confronto e' sullo SPESSORE, e la prima stesura lo faceva sull'opacita': sbagliato.**
	// `AHUD::DrawLine` finisce in `FBatchedElements::AddLine`, che forza `OpaqueColor.A = 1` — quindi
	// asserire l'alpha significava misurare un campo che l'engine butta via, con il test verde e lo schermo
	// invariato. Lo spessore l'engine lo rispetta.
	// 🔴 **Riscritti il 2026-08-19, dopo che la verifica PIE ha BOCCIATO una resa che questi assert
	// dichiaravano corretta.** Dicevano `TestNotEqual` sullo spessore: `2,0 != 1,25` e' vero, e il test era
	// verde mentre un occhio davanti allo schermo diceva *«si somigliano troppo»*. **«Diverso» non e'
	// «distinguibile»**, ed e' la sola cosa che un test headless non puo' misurare da solo — ma puo'
	// misurare che la differenza stia su un canale che ha superato una verifica umana, e con quale margine.
	//
	// 🔴 **E la seconda stesura sbagliava un livello piu' su, dove nessun assert arrivava.** Assegnava a ogni
	// coppia il suo canale, ma **una delle tre coppie non e' osservabile**: `Confirmed` non disegna nessuna
	// linea — non entra in `if (bMoving)`, ne' in `if (bHasTarget)`, ne' in `if (bDashing)` — quindi il
	// canale «piena contro tratteggiata» assegnato a `Confirmed` vs `Predicted` non arriva mai a schermo.
	// Restava un solo confronto grafico vero, e gli era stato dato il colore, che qui e' gia' occupato
	// dall'identita' di squadra. Trovato dalla code review leggendo le condizioni di disegno.
	//
	// Il test segue quindi la MATRICE, non i livelli:

	// · `Predicted` vs `Uncertain` → **l'unico confronto grafico che esiste**, sulla linea al bersaglio, che
	//   e' il solo elemento su cui due livelli coesistono. Lo porta il tratteggio: canale libero (nessun
	//   altro lo usa) e confermato da un occhio umano — *«le linee tratteggiate si vedevano»*.
	// ⚠️ **Entrambi tratteggiati, e li separa il PASSO del segno** — trattini contro punti, cioe' i due stili
	// distinti che `progettazione-hud.md` §16 assegna ai due livelli e che nessuna stesura aveva reso: le
	// prime due davano a tutti e due lo stesso tratteggio, poi cercavano un secondo canale per distinguerli
	// (l'opacita', inerte; lo spessore, bocciato in PIE; il colore, gia' occupato dalla squadra).
	// 🔴 Un `TestNotEqual` su `bDashedLine` **non direbbe niente qui**, ed e' la trappola in cui la stesura
	// precedente e' caduta invertendo la grammatica per creare una disuguaglianza su quel flag.
	TestTrue(TEXT("previsto: tratteggiata, come §16 prescrive"), Predicted.bDashedLine);
	TestTrue(TEXT("incerto: tratteggiata anch'essa"), Uncertain.bDashedLine);
	TestTrue(TEXT("previsto e incerto: li separa il PASSO del segno, trattini contro punti"),
		Predicted.DashPeriodPx >= Uncertain.DashPeriodPx * 2.f);

	// · `Confirmed` vs gli altri due → **non ha un confronto grafico, e il test lo dice invece di fingerlo.**
	//   Si distingue dal CONTENUTO dell'etichetta, che e' l'unico elemento presente a tutti e tre i livelli:
	//   non nomina nessun bersaglio e non porta il `?`. Lo verifica `IntentLabelGrammar`, che dal 2026-08-19
	//   costruisce anche una vista `Predicted` — prima non lo faceva, e la distinzione che questo commento
	//   delegava non era coperta da nessun assert della suite. Trovato dalla code review.
	TestFalse(TEXT("confermato: linea piena, §16 alla lettera"), Confirmed.bDashedLine);

	// Lo spessore resta e accompagna, ma non porta da solo nessun confronto: e' quello che la seduta ha
	// insegnato. Pinnato come rinforzo — spinge nella stessa direzione del canale che decide.
	TestTrue(TEXT("il tratto dell'incerto e' piu' sottile, a rinforzo"),
		Uncertain.LineThickness < Predicted.LineThickness);

	// 🔴 **`ColorSaturation` NON esiste piu', e la sua assenza e' parte della specifica.** Il colore in questa
	// HUD e' l'identita' di squadra; spenderlo per la certezza toglieva croma a ogni unita' in movimento per
	// un confronto che su quell'elemento non esiste. Se qualcuno lo reintroduce, questo commento e la matrice
	// sull'intestazione della struct sono il posto dove leggere perche' era stato tolto.

	// 2. Coerenza fra i campi del segno. ⚠️ **Un congiunto per assert**, cosi' il log dice QUALE mutazione ha
	//    colpito: la stesura precedente combinava flag e soglia in un solo `TestTrue`, e lo stesso errore
	//    produceva due righe rosse per una causa sola. Trovato dalla code review.
	TestFalse(TEXT("confermato: la linea non e' tratteggiata"), Confirmed.bDashedLine);
	TestTrue(TEXT("confermato: e il duty e' pieno, o uscirebbe spezzata"), Confirmed.DashDutyCycle >= 1.f);
	TestTrue(TEXT("previsto: il duty tiene i trattini leggibili"), Predicted.DashDutyCycle >= 0.5f);
	TestTrue(TEXT("incerto: il duty e' piu' basso, cosi' il segno legge come punto"),
		Uncertain.DashDutyCycle < Predicted.DashDutyCycle);

	// 3. Il `?` appartiene al SOLO livello incerto. E' l'elemento piu' facile da spargere ovunque «per
	//    prudenza», e un `?` su tutto non dice piu' niente.
	TestFalse(TEXT("confermato: nessun ?"), Confirmed.bUncertaintyMark);
	TestFalse(TEXT("previsto: nessun ?"), Predicted.bUncertaintyMark);
	TestTrue(TEXT("incerto: il punto interrogativo"), Uncertain.bUncertaintyMark);

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
		TestEqual(TEXT("la resa segue il livello, non i flag del piano"), S.LineThickness, Confirmed.LineThickness);
		TestEqual(TEXT("con la linea del livello, non dei flag"), S.bDashedLine, Confirmed.bDashedLine);
		TestFalse(TEXT("e non inventa un ? che il livello non chiede"), S.bUncertaintyMark);
	}

	// 5. `Unknown` non deve MAI ricevere la resa di `Confirmed`: e' il difetto per cui vale zero. Un campo mai
	//    calcolato che ereditasse il tratto pieno affermerebbe a schermo la garanzia piu' forte del dominio.
	{
		const FRTIntentCertaintyStyle S = ARTHUD::ComposeIntentCertaintyStyle(ViewWith(ERTIntentCertainty::Unknown));
		TestNotEqual(TEXT("il livello mai calcolato non si disegna come confermato"),
			S.LineThickness, Confirmed.LineThickness);
		TestTrue(TEXT("e non promette una linea piena"), S.bDashedLine);
		TestTrue(TEXT("ne' il ? di chi non sa: quello lo porta"), S.bUncertaintyMark);
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

	// 🔴 Qui c'era un `TestNotEqual(WithReaction.bReactionArmed, NoReaction.bReactionArmed)`, tolto perche'
	// **non poteva fallire da solo**: dopo `TestTrue(A)` e `TestFalse(B)` su due `bool`, `A != B` segue, e
	// l'assert riportava un errore solo nelle run in cui uno dei due precedenti ne aveva gia' riportato uno.
	// Si presentava come una terza verifica e ne valeva zero. Trovato dalla code review.
	// La domanda che voleva porre — «armare una reazione cambia cio' che si VEDE?» — non si risponde sulla
	// struct: si risponde sull'etichetta, ed e' `ArmedReactionLabel` a farlo.

	// La reazione e' un SECONDO asse: non contamina il livello del piano, che resta quello che il simulatore
	// ha calcolato. Lo pinna gia' `IntentCertaintyClassification` sul DTO; qui si pinna sulla resa.
	TestEqual(TEXT("e non tocca il peso del tratto"), WithReaction.LineThickness, NoReaction.LineThickness);
	TestEqual(TEXT("ne' la sua linea"), WithReaction.bDashedLine, NoReaction.bDashedLine);

	// Una reazione armata su un piano INCERTO resta armata: i due assi sono indipendenti in entrambi i versi.
	{
		FRTIntentView MovingArmed = Armed;
		MovingArmed.Certainty = ERTIntentCertainty::Uncertain;
		const FRTIntentCertaintyStyle S = ARTHUD::ComposeIntentCertaintyStyle(MovingArmed);
		TestTrue(TEXT("reazione armata anche su piano incerto"), S.bReactionArmed);
		TestTrue(TEXT("col marcatore del proprio livello"), S.bUncertaintyMark);
	}
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// CP 11.2: cio' che il giocatore LEGGE davvero — l'etichetta e la geometria del tratteggio.
//
// Entrambi questi test esistono perche' la code review ha mostrato che la parte di grammatica piu' visibile
// era anche l'unica senza copertura: viveva in una format string e in una lambda dentro `DrawHUD`.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTArmedReactionLabelTest,
	"RefactorTactics.UI.IntentLabelGrammar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTArmedReactionLabelTest::RunTest(const FString&)
{
	auto Label = [](const FRTIntentView& V)
	{
		return ARTHUD::ComposeIntentLabel(V, ARTHUD::ComposeIntentCertaintyStyle(V));
	};

	// Il `?` del PIANO compare sul livello incerto e su nessun altro.
	{
		FRTIntentView Still;
		Still.bIsAlly = true;
		Still.Certainty = ERTIntentCertainty::Confirmed;
		TestFalse(TEXT("un piano confermato non porta il ?"), Label(Still).Contains(TEXT("?")));

		FRTIntentView Moving = Still;
		Moving.Certainty = ERTIntentCertainty::Uncertain;
		Moving.bMoving = true;
		Moving.PlannedCell = FRTCellId(2, 0);
		TestTrue(TEXT("un piano incerto lo porta"), Label(Moving).Contains(TEXT("?")));
	}

	// 🔴 **`Confirmed` contro `Predicted`: l'unico canale che li separa, e non era coperto da NESSUN assert.**
	// I due livelli hanno la stessa struct di stile — `Confirmed` non disegna linee, quindi non c'e' niente
	// da distinguere graficamente — e la distinzione vive tutta nel CONTENUTO dell'etichetta: uno nomina un
	// bersaglio, l'altro no. `IntentCertaintyRendering` delegava qui la verifica, e qui non c'era: nessun
	// caso costruiva una vista `Predicted`. Rompendo il primo ramo di `ComposeIntentLabel` — fargli ignorare
	// `bHasTarget` — i due livelli diventavano indistinguibili su OGNI canale e la suite restava verde.
	// Trovato dalla code review.
	{
		FRTIntentView Still;
		Still.bIsAlly = true;
		Still.Certainty = ERTIntentCertainty::Confirmed;
		Still.ActionName = FText::FromString(TEXT("Guardia"));

		FRTIntentView Aiming = Still;
		Aiming.Certainty = ERTIntentCertainty::Predicted;
		Aiming.bHasTarget = true;
		Aiming.TargetCell = FRTCellId(3, -1);

		const FString Confirmed = Label(Still);
		const FString Predicted = Label(Aiming);
		TestNotEqual(TEXT("confermato e previsto NON si leggono uguali"), Confirmed, Predicted);
		TestTrue(TEXT("previsto nomina la cella del bersaglio"), Predicted.Contains(TEXT("q=3")));
		TestFalse(TEXT("confermato no"), Confirmed.Contains(TEXT("q=3")));
		// Nessuno dei due porta il `?`: quello separa l'incerto, non questi due.
		TestFalse(TEXT("e nessuno dei due porta il ?"),
			Confirmed.Contains(TEXT("?")) || Predicted.Contains(TEXT("?")));
	}

	// 🔴 **La voce di DoD «la reazione armata e' resa con la grammatica incerto» non aveva NESSUN test**: il
	// `?` viveva in un `Printf` dentro `DrawHUD`, e toglierlo lasciava la suite verde. Qui l'etichetta e' un
	// valore, quindi il glifo si asserisce.
	{
		FRTIntentView Armed;
		Armed.bIsAlly = true;
		Armed.Certainty = ERTIntentCertainty::Confirmed; // il PIANO e' certo: il ? che segue e' della reazione
		Armed.ReactionName = FText::FromString(TEXT("Contrattacco"));

		const FString WithReaction = Label(Armed);
		TestTrue(TEXT("la reazione compare"), WithReaction.Contains(TEXT("Contrattacco")));
		TestTrue(TEXT("e porta il ? anche su un piano confermato"), WithReaction.Contains(TEXT("?")));

		FRTIntentView Bare = Armed;
		Bare.ReactionName = FText::GetEmpty();
		const FString NoReaction = Label(Bare);
		TestFalse(TEXT("senza reazione non compare nulla di suo"), NoReaction.Contains(TEXT("Contrattacco")));
		// La domanda causale, posta dove si puo' rispondere: cambia cio' che il giocatore legge?
		TestNotEqual(TEXT("armare una reazione cambia l'etichetta"), WithReaction, NoReaction);
	}

	// 🔴 **Uno scatto senza movimento normale produceva «fermo ?»**, cioe' due meta' della stessa etichetta
	// che affermano cose opposte: `bMoving` e' falso, quindi la catena cadeva nel ramo «fermo», mentre
	// `ClassifyPlan` guarda `bMoving || bDashing` e appendeva il `?`. Trovato dalla code review.
	{
		FRTIntentView DashOnly;
		DashOnly.bIsAlly = true;
		DashOnly.Certainty = ERTIntentCertainty::Uncertain;
		DashOnly.bDashing = true;
		DashOnly.DashCell = FRTCellId(4, 1);

		const FString L = Label(DashOnly);
		TestFalse(TEXT("un'unita' che scatta non e' «ferma»"), L.Contains(TEXT("fermo")));
		TestTrue(TEXT("l'etichetta nomina lo scatto"), L.Contains(TEXT("scatto")));
		TestTrue(TEXT("e la cella di arrivo"), L.Contains(TEXT("q=4")));
	}
	return true;
}

// 🔴 **Qui viveva `RefactorTactics.UI.IntentCertaintyTint`, RIMOSSO il 2026-08-19 con la funzione che
// verificava.** Copriva `ApplyCertaintyTint`, che sbiadiva il colore di squadra secondo la certezza: quel
// canale non era disponibile — in questa HUD il colore E' l'identita' di squadra — e veniva speso anche
// sulle rotte, che sono `Uncertain` per costruzione. La distinzione la porta ora il tratteggio.
// ⚠️ **Due dei suoi assert erano vacui, ed e' un difetto che vale piu' del test.** Quello sull'alpha
// confrontava `Faded.A` con `Team.A` su un colore di prova con `A = 1`: un'implementazione che scrivesse
// `1.f` a mano passava. E la tolleranza sulla luminanza (`0,01`) era ~15 volte piu' larga della deviazione
// prodotta dalla mutazione che doveva catturare — i pesi percettivi sostituiti da una media aritmetica
// danno uno scarto di `0,00065`. Entrambi trovati dalla code review, non da una run rossa.
// Se un giorno il colore torna a essere un canale libero, il test torna con una sonda ad alpha != 1 e una
// tolleranza dell'ordine di `1e-4`.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTDashSegmentsTest,
	"RefactorTactics.UI.IntentDashSegments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTDashSegmentsTest::RunTest(const FString&)
{
	const FVector2D A(0.f, 0.f);
	const FVector2D B(140.f, 0.f);

	// Linea piena: un segmento solo. E' anche la garanzia che il livello «confermato» non paghi il costo del
	// tratteggio, perche' prima di CP 11.2 quella stessa geometria costava esattamente una `DrawLine`.
	{
		const TArray<TPair<FVector2D, FVector2D>> Full = ARTHUD::ComposeDashSegments(A, B, 1.f, 14.f);
		if (!TestEqual(TEXT("duty pieno: un segmento solo"), Full.Num(), 1)) { return false; }
		TestEqual(TEXT("che e' il segmento intero"), Full[0].Value, B);
	}

	// Tratteggiata: piu' segmenti, e ciascuno copre la frazione ACCESA del proprio periodo. Il duty si legge
	// dalla geometria, non da un campo: e' cio' che distingue «attenuato» da «dissolto» a schermo.
	{
		const TArray<TPair<FVector2D, FVector2D>> Half = ARTHUD::ComposeDashSegments(A, B, 0.5f, 14.f);
		const TArray<TPair<FVector2D, FVector2D>> Thin = ARTHUD::ComposeDashSegments(A, B, 0.3f, 14.f);
		if (!TestTrue(TEXT("il tratteggio spezza il segmento"), Half.Num() > 1)) { return false; }
		TestEqual(TEXT("stesso periodo, stesso numero di tratti"), Thin.Num(), Half.Num());

		const float HalfLen = FVector2D::Distance(Half[0].Key, Half[0].Value);
		const float ThinLen = FVector2D::Distance(Thin[0].Key, Thin[0].Value);
		TestTrue(TEXT("un duty piu' basso accende meno tratto"), ThinLen < HalfLen);
	}

	// 🔴 **Il tetto, che e' la ragione per cui questa funzione e' stata estratta.** `UCanvas::Project` clampa
	// `W` a `UE_KINDA_SMALL_NUMBER`: una cella pochi centimetri davanti alla camera passa `Z > 0` e proietta a
	// coordinate dell'ordine di `1e6`. Senza limite erano decine di migliaia di `DrawLine` per segmento, ogni
	// frame — una regressione che questo lavoro avrebbe introdotto. Trovato dalla code review.
	{
		const TArray<TPair<FVector2D, FVector2D>> Absurd =
			ARTHUD::ComposeDashSegments(FVector2D(-1.e6f, 0.f), FVector2D(1.e6f, 0.f), 0.5f, 14.f);
		TestTrue(TEXT("una proiezione degenere non produce centinaia di migliaia di tratti"),
			Absurd.Num() <= 512);
		TestTrue(TEXT("ma nemmeno zero"), Absurd.Num() >= 1);
	}

	// Un segmento degenere non deve dividere per zero ne' restituire una lista vuota.
	{
		const TArray<TPair<FVector2D, FVector2D>> Point = ARTHUD::ComposeDashSegments(A, A, 0.5f, 14.f);
		TestEqual(TEXT("un punto resta un tratto"), Point.Num(), 1);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
