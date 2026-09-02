#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Bot/RTHexBotLibrary.h"
#include "Map/RTCellId.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La selezione della reazione del bot: punteggio tattico, kit-first solo come tie-break ([D-268], `#1802`).
 *
 * ⚠️ **Il punteggio si misura su cio' che la SQUADRA conosce**, e la prova che lo fa davvero non e' il
 * canary di equita' — quello passa anche con un punteggio costante. E' la sua coppia: un nemico che *entra*
 * nella conoscenza deve **cambiare** il punteggio. Senza l'anti-vacuita' accanto, il canary dimostra solo
 * che la funzione non guarda niente.
 */
namespace
{
	FRTActionDef Reaction(ERTReactionTrigger Trigger, int32 RangeCells = 1)
	{
		FRTActionDef Def;
		Def.Slot = ERTActionSlot::Reaction;
		Def.ReactionTrigger = Trigger;
		Def.RangeCells = RangeCells;
		return Def;
	}

	/** Un contesto senza nessuno: nessun nemico conosciuto, nessun alleato. */
	FRTHexBotContext EmptyContext()
	{
		FRTHexBotContext Ctx;
		Ctx.Origin = FRTCellId(0, 0, 0);
		return Ctx;
	}

	void AddKnownEnemy(FRTHexBotContext& Ctx, const FRTCellId& Cell, int32 Reach)
	{
		Ctx.Enemies.Add(Cell);
		Ctx.EnemyRanges.Add(Reach);
		Ctx.EnemyHealth.Add(100);
		Ctx.EnemyFacings.Add(ERTHexDirection::E);
	}

	FRTReactionCandidate Candidate(int32 AbilityIndex, int32 Score, bool bFromKit)
	{
		FRTReactionCandidate C;
		C.AbilityIndex = AbilityIndex;
		C.Score = Score;
		C.bFromKit = bFromKit;
		return C;
	}
}

/**
 * Senza minaccia conosciuta nessuna reazione vale niente — **e' la proprieta' che rende il cambio
 * atterrabile**: con tutti i punteggi a zero decide il tie-break, cioe' il kit, cioe' il comportamento di
 * oggi. `D-268` cambia le scelte solo dove la conoscenza le separa davvero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionZeroWithoutThreatTest,
	"RefactorTactics.HexBot.ReactionScoreIsZeroWithoutKnownThreat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionZeroWithoutThreatTest::RunTest(const FString&)
{
	const FRTHexBotContext Ctx = EmptyContext();

	TestEqual(TEXT("contrattacco senza nemici conosciuti"),
		URTHexBotLibrary::ScoreReaction(Reaction(ERTReactionTrigger::HitByDirectAttack), Ctx), 0);
	TestEqual(TEXT("interposizione senza alleati"),
		URTHexBotLibrary::ScoreReaction(Reaction(ERTReactionTrigger::AllyHitByDirectAttack), Ctx), 0);
	TestEqual(TEXT("un trigger senza termine di conoscenza vale zero, non un numero inventato"),
		URTHexBotLibrary::ScoreReaction(Reaction(ERTReactionTrigger::AboutToBeDisplaced), Ctx), 0);
	TestEqual(TEXT("cio' che non e' una reazione vale zero"),
		URTHexBotLibrary::ScoreReaction(Reaction(ERTReactionTrigger::None), Ctx), 0);

	return true;
}

/**
 * 🔑 **Anti-vacuita' del canary di equita'.** Se questo test non esistesse, un punteggio costante
 * soddisferebbe «un nemico nascosto non cambia la scelta» senza guardare nessuna conoscenza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionKnownThreatRaisesScoreTest,
	"RefactorTactics.HexBot.ReactionKnownThreatRaisesTheCounterScore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionKnownThreatRaisesScoreTest::RunTest(const FString&)
{
	const FRTActionDef Counter = Reaction(ERTReactionTrigger::HitByDirectAttack);

	FRTHexBotContext Ctx = EmptyContext();
	const int32 Blind = URTHexBotLibrary::ScoreReaction(Counter, Ctx);

	// Un nemico conosciuto a due celle, con portata 3: la mia cella e' dentro la sua minaccia.
	AddKnownEnemy(Ctx, FRTCellId(2, 0, 0), /*Reach*/ 3);
	const int32 WithThreat = URTHexBotLibrary::ScoreReaction(Counter, Ctx);

	TestEqual(TEXT("cieco vale zero"), Blind, 0);
	TestTrue(TEXT("un nemico che entra nella conoscenza ALZA il punteggio"), WithThreat > Blind);
	TestEqual(TEXT("e lo alza di WThreat, non di un numero nuovo"), WithThreat, Ctx.WThreat);

	// Un secondo nemico conosciuto ma FUORI portata non aggiunge minaccia: il termine misura chi puo'
	// colpirmi, non quanti ne conosco.
	AddKnownEnemy(Ctx, FRTCellId(9, 0, 0), /*Reach*/ 1);
	TestEqual(TEXT("un nemico conosciuto ma fuori portata non conta"),
		URTHexBotLibrary::ScoreReaction(Counter, Ctx), Ctx.WThreat);

	return true;
}

/**
 * L'interposizione vale se c'e' qualcuno da proteggere: un alleato **entro la portata dell'azione** e
 * raggiungibile da un nemico conosciuto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionInterposeTest,
	"RefactorTactics.HexBot.ReactionInterposeNeedsAKnownThreatOnAnAlly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionInterposeTest::RunTest(const FString&)
{
	const FRTActionDef Intercept = Reaction(ERTReactionTrigger::AllyHitByDirectAttack, /*RangeCells*/ 1);

	FRTHexBotContext Ctx = EmptyContext();
	Ctx.Allies.Add(FRTCellId(1, 0, 0)); // adiacente: dentro la portata dell'interposizione
	Ctx.AllyHealth.Add(90);

	TestEqual(TEXT("un alleato che nessun nemico conosciuto minaccia non vale l'interposizione"),
		URTHexBotLibrary::ScoreReaction(Intercept, Ctx), 0);

	AddKnownEnemy(Ctx, FRTCellId(3, 0, 0), /*Reach*/ 2); // arriva sull'alleato, non su di me
	TestEqual(TEXT("un alleato minacciato da un nemico conosciuto la vale"),
		URTHexBotLibrary::ScoreReaction(Intercept, Ctx), Ctx.WAllyDamage);

	// Un alleato FUORI dalla portata dell'interposizione non si puo' coprire, quindi non conta.
	FRTHexBotContext Lontano = EmptyContext();
	Lontano.Allies.Add(FRTCellId(5, 0, 0));
	Lontano.AllyHealth.Add(90);
	AddKnownEnemy(Lontano, FRTCellId(6, 0, 0), /*Reach*/ 2);
	TestEqual(TEXT("un alleato fuori dalla portata dell'azione non si copre"),
		URTHexBotLibrary::ScoreReaction(Intercept, Lontano), 0);

	return true;
}

/** A parita' ESATTA vince il kit: e' il tie-break di `D-268`, e retrocede `D-220` da politica a spareggio. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionTieGoesToKitTest,
	"RefactorTactics.HexBot.ReactionExactTieGoesToTheKit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionTieGoesToKitTest::RunTest(const FString&)
{
	// Il modulo arriva PRIMA nell'array: se vincesse, avrebbe vinto per ordine e non per regola.
	const TArray<FRTReactionCandidate> Candidates = {
		Candidate(/*Index*/ 7, /*Score*/ 100, /*bFromKit*/ false),
		Candidate(/*Index*/ 2, /*Score*/ 100, /*bFromKit*/ true),
	};
	TestEqual(TEXT("a parita' esatta vince quella di kit"),
		URTHexBotLibrary::SelectReaction(Candidates), 2);

	// Due di kit a parita': vince l'indice piu' basso, che e' deterministico.
	const TArray<FRTReactionCandidate> DueDiKit = {
		Candidate(5, 100, true),
		Candidate(3, 100, true),
	};
	TestEqual(TEXT("fra due di kit a parita' vince l'indice piu' basso"),
		URTHexBotLibrary::SelectReaction(DueDiKit), 3);

	TestEqual(TEXT("nessuna candidata da' INDEX_NONE"),
		URTHexBotLibrary::SelectReaction({}), (int32)INDEX_NONE);

	return true;
}

/**
 * La regressione che dimostra che `D-220` non e' piu' la politica: con punteggi diversi vince l'utilita'
 * piu' alta **anche se e' del loadout**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionLoadoutCanWinTest,
	"RefactorTactics.HexBot.ReactionHigherUtilityWinsEvenFromTheLoadout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionLoadoutCanWinTest::RunTest(const FString&)
{
	const TArray<FRTReactionCandidate> Candidates = {
		Candidate(/*kit*/ 2, /*Score*/ 10, true),
		Candidate(/*loadout*/ 7, /*Score*/ 100, false),
	};
	TestEqual(TEXT("vince il loadout quando vale di piu'"),
		URTHexBotLibrary::SelectReaction(Candidates), 7);

	// E il verso opposto, perche' «vince sempre il loadout» passerebbe il test di sopra.
	const TArray<FRTReactionCandidate> Opposto = {
		Candidate(2, 100, true),
		Candidate(7, 10, false),
	};
	TestEqual(TEXT("vince il kit quando vale di piu'"),
		URTHexBotLibrary::SelectReaction(Opposto), 2);

	return true;
}

/**
 * 🔑 La proprieta' di atterraggio: **tutti-zero riproduce il comportamento di oggi**.
 *
 * E' cio' che distingue un cambio di politica da un cambio di gioco: dove la conoscenza non separa i
 * candidati, il bot arma esattamente cio' che armava prima di questa issue.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionAllZeroKeepsTodayTest,
	"RefactorTactics.HexBot.ReactionAllZeroScoresKeepTodayBehaviour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionAllZeroKeepsTodayTest::RunTest(const FString&)
{
	const TArray<FRTReactionCandidate> Candidates = {
		Candidate(/*loadout, primo nell'array*/ 7, 0, false),
		Candidate(/*kit*/ 2, 0, true),
	};
	TestEqual(TEXT("con tutti i punteggi a zero decide il kit, come oggi"),
		URTHexBotLibrary::SelectReaction(Candidates), 2);

	// Solo loadout disponibile: si arma quello, come oggi.
	const TArray<FRTReactionCandidate> SoloLoadout = { Candidate(7, 0, false) };
	TestEqual(TEXT("senza reazione di kit si arma il modulo, come oggi"),
		URTHexBotLibrary::SelectReaction(SoloLoadout), 7);

	return true;
}

/**
 * L'ordine di enumerazione non puo' cambiare l'esito a parita' di punteggi: e' l'AC di determinismo, e la
 * si misura permutando le candidate invece di sperarci.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionOrderInvariantTest,
	"RefactorTactics.HexBot.ReactionSelectionIsIndependentOfCandidateOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionOrderInvariantTest::RunTest(const FString&)
{
	const FRTReactionCandidate Kit = Candidate(2, 100, true);
	const FRTReactionCandidate Loadout = Candidate(7, 100, false);
	const FRTReactionCandidate Altro = Candidate(9, 100, false);

	const TArray<TArray<FRTReactionCandidate>> Permutazioni = {
		{ Kit, Loadout, Altro }, { Kit, Altro, Loadout }, { Loadout, Kit, Altro },
		{ Loadout, Altro, Kit }, { Altro, Kit, Loadout }, { Altro, Loadout, Kit },
	};

	bool bSempreLoStesso = true;
	for (const TArray<FRTReactionCandidate>& P : Permutazioni)
	{
		bSempreLoStesso = bSempreLoStesso && URTHexBotLibrary::SelectReaction(P) == 2;
	}
	TestTrue(TEXT("tutte e sei le permutazioni scelgono la stessa reazione"), bSempreLoStesso);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
