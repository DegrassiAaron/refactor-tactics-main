#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Bot/RTHexBotLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "UObject/StrongObjectPtr.h" // tiene viva l'arena del test attraverso un eventuale GC

#if WITH_DEV_AUTOMATION_TESTS

/**
 * La selezione della reazione del bot: punteggio tattico, kit-first solo come spareggio ([D-268], `#1802`).
 *
 * ⚠️ **Il punteggio si misura su cio' che la SQUADRA conosce**, e la prova che lo fa davvero non e' il
 * canary di equita' — quello passa anche con un punteggio costante. E' la sua coppia: un nemico che *entra*
 * nella conoscenza deve **cambiare** il punteggio. Senza l'anti-vacuita' accanto, il canary dimostra solo
 * che la funzione non guarda niente.
 *
 * ⚠️ **`Map` e' `nullptr` in questi test, e non e' una scorciatoia**: `DescribeLineOfSight` dichiara
 * *«nessun dato di mappa: nessun ostacolo noto»*, quindi qui la linea di vista e' sempre libera e cio' che
 * si misura e' il TERMINE, isolato. Che la LOS entri davvero nel conto lo pinna
 * `ReactionScoreRespectsLineOfSight`, che una mappa ce l'ha.
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
 * atterrabile**: con tutti i punteggi a zero decide lo spareggio, cioe' il kit, cioe' il comportamento di
 * oggi. [D-268] cambia le scelte solo dove la conoscenza le separa davvero.
 *
 * ⚠️ **Si enumerano TUTTI e sei i trigger**, e non i due che il commento nomina: l'header e la spec
 * dichiarano quali valgono zero e perche', e una dichiarazione senza test e' una promessa. Se domani
 * qualcuno scrivesse un termine speculativo per `AboutToReceiveControl` — l'onniscienza che quel commento
 * esiste per vietare — questo test lo direbbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionZeroWithoutThreatTest,
	"RefactorTactics.HexBot.ReactionScoreIsZeroWithoutKnownThreat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionZeroWithoutThreatTest::RunTest(const FString&)
{
	const FRTHexBotContext Ctx = EmptyContext();

	TestEqual(TEXT("contrattacco senza nemici conosciuti"),
		URTHexBotLibrary::ScoreReaction(nullptr, Reaction(ERTReactionTrigger::HitByDirectAttack), Ctx), 0);
	TestEqual(TEXT("interposizione senza alleati"),
		URTHexBotLibrary::ScoreReaction(nullptr, Reaction(ERTReactionTrigger::AllyHitByDirectAttack), Ctx), 0);

	// I tre zeri DICHIARATI, con un contesto pieno: non valgono zero per mancanza di soggetto, valgono zero
	// perche' la conoscenza autorizzata non ha un termine per loro.
	FRTHexBotContext Pieno = EmptyContext();
	AddKnownEnemy(Pieno, FRTCellId(1, 0, 0), /*Reach*/ 5);
	Pieno.Allies.Add(FRTCellId(0, 1, 0));
	Pieno.AllyHealth.Add(90);

	TestEqual(TEXT("AboutToBeDisplaced vale zero anche con nemici e alleati conosciuti"),
		URTHexBotLibrary::ScoreReaction(nullptr, Reaction(ERTReactionTrigger::AboutToBeDisplaced), Pieno), 0);
	TestEqual(TEXT("AboutToReceiveControl vale zero anche con nemici e alleati conosciuti"),
		URTHexBotLibrary::ScoreReaction(nullptr, Reaction(ERTReactionTrigger::AboutToReceiveControl), Pieno), 0);
	TestEqual(TEXT("CellBecameHazardous vale zero: il contesto del bot non porta gli hazard"),
		URTHexBotLibrary::ScoreReaction(nullptr, Reaction(ERTReactionTrigger::CellBecameHazardous), Pieno), 0);
	TestEqual(TEXT("cio' che non e' una reazione vale zero"),
		URTHexBotLibrary::ScoreReaction(nullptr, Reaction(ERTReactionTrigger::None), Pieno), 0);

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
	const int32 Blind = URTHexBotLibrary::ScoreReaction(nullptr, Counter, Ctx);

	// Un nemico conosciuto a due celle, con portata 3: la mia cella e' dentro la sua minaccia.
	AddKnownEnemy(Ctx, FRTCellId(2, 0, 0), /*Reach*/ 3);
	const int32 WithThreat = URTHexBotLibrary::ScoreReaction(nullptr, Counter, Ctx);

	TestEqual(TEXT("cieco vale zero"), Blind, 0);
	TestTrue(TEXT("un nemico che entra nella conoscenza ALZA il punteggio"), WithThreat > Blind);
	TestEqual(TEXT("e lo alza di WThreat, non di un numero nuovo"), WithThreat, Ctx.WThreat);

	// Un secondo nemico conosciuto ma FUORI portata non aggiunge minaccia: il termine misura chi puo'
	// colpirmi, non quanti ne conosco.
	AddKnownEnemy(Ctx, FRTCellId(9, 0, 0), /*Reach*/ 1);
	TestEqual(TEXT("un nemico conosciuto ma fuori portata non conta"),
		URTHexBotLibrary::ScoreReaction(nullptr, Counter, Ctx), Ctx.WThreat);

	return true;
}

/**
 * 🔴 **La minaccia e' gittata E linea di vista, come in `ScorePlan`.**
 *
 * Senza questo test il punteggio conterebbe i nemici dietro un muro, e sulla mappa d'autore — quella con
 * l'ostacolo centrale che blocca vista e passo — il bot armerebbe un contrattacco contro chi non puo'
 * sparargli: la «reazione che non sarebbe scattata» di [D-268], col segno rovesciato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionLineOfSightTest,
	"RefactorTactics.HexBot.ReactionScoreRespectsLineOfSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionLineOfSightTest::RunTest(const FString&)
{
	// Il bot in (0,0), il nemico in (2,0) con portata 3: in linea d'aria lo raggiunge.
	FRTHexBotContext Ctx = EmptyContext();
	AddKnownEnemy(Ctx, FRTCellId(2, 0, 0), /*Reach*/ 3);
	const FRTActionDef Counter = Reaction(ERTReactionTrigger::HitByDirectAttack);

	// Stessa arena dei test del bot: `MakeFlatArena` invece di una mappa costruita a mano, cosi' la
	// geometria e' quella che il resto della suite usa gia'.
	TStrongObjectPtr<URTHexMapAsset> Map(URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), 3));

	TestEqual(TEXT("senza ostacoli la minaccia si conta"),
		URTHexBotLibrary::ScoreReaction(Map.Get(), Counter, Ctx), Ctx.WThreat);

	// La cella in mezzo blocca la vista: la stessa geometria, lo stesso nemico, nessuna minaccia.
	{
		const FRTCellId Between(1, 0, 0);
		FRTHexCellData Data = Map->FindCell(Between) ? *Map->FindCell(Between) : FRTHexCellData(Between);
		Data.Id = Between;
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	TestEqual(TEXT("un muro in mezzo azzera la minaccia, come per ScorePlan"),
		URTHexBotLibrary::ScoreReaction(Map.Get(), Counter, Ctx), 0);

	return true;
}

/**
 * L'interposizione vale se c'e' qualcuno da proteggere: un alleato **entro la portata dell'azione** e
 * raggiungibile da un nemico conosciuto.
 *
 * ⚠️ **Lo stesso peso del contrattacco, e non e' pigrizia.** In `ScorePlan` `WAllyDamage` e' per PUNTO di
 * danno e `WThreat` e' per NEMICO: usarli qui come se fossero la stessa unita' avrebbe reso
 * l'interposizione (10) sempre perdente contro un contrattacco (100), cioe' il difetto di [D-220]
 * rovesciato. Il punteggio misura «quanta minaccia questa reazione risponde», e la minaccia si conta allo
 * stesso modo sulla mia cella e su quella di un alleato.
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
		URTHexBotLibrary::ScoreReaction(nullptr, Intercept, Ctx), 0);

	AddKnownEnemy(Ctx, FRTCellId(3, 0, 0), /*Reach*/ 2); // arriva sull'alleato, non su di me
	TestEqual(TEXT("un alleato minacciato da un nemico conosciuto la vale, e vale WThreat"),
		URTHexBotLibrary::ScoreReaction(nullptr, Intercept, Ctx), Ctx.WThreat);

	// Due nemici sullo stesso alleato restano UN alleato: si contano i protetti, non le traiettorie.
	AddKnownEnemy(Ctx, FRTCellId(2, 1, 0), /*Reach*/ 3);
	TestEqual(TEXT("due minacce sullo stesso alleato non raddoppiano il valore"),
		URTHexBotLibrary::ScoreReaction(nullptr, Intercept, Ctx), Ctx.WThreat);

	// Un alleato FUORI dalla portata dell'interposizione non si puo' coprire, quindi non conta.
	FRTHexBotContext Lontano = EmptyContext();
	Lontano.Allies.Add(FRTCellId(5, 0, 0));
	Lontano.AllyHealth.Add(90);
	AddKnownEnemy(Lontano, FRTCellId(6, 0, 0), /*Reach*/ 2);
	TestEqual(TEXT("un alleato fuori dalla portata dell'azione non si copre"),
		URTHexBotLibrary::ScoreReaction(nullptr, Intercept, Lontano), 0);

	return true;
}

/**
 * A parita' ESATTA vince il kit: e' lo spareggio di [D-268], e retrocede [D-220] da politica a spareggio.
 *
 * E la RAGIONE e' un dato: «kit» e «indice» sono due spareggi diversi, e [D-245] chiede che si distinguano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionTieGoesToKitTest,
	"RefactorTactics.HexBot.ReactionExactTieGoesToTheKit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionTieGoesToKitTest::RunTest(const FString&)
{
	// Il modulo arriva PRIMA nell'array: se vincesse, avrebbe vinto per ordine e non per regola.
	const FRTReactionChoice Kit = URTHexBotLibrary::SelectReaction({
		Candidate(/*Index*/ 7, /*Score*/ 100, /*bFromKit*/ false),
		Candidate(/*Index*/ 2, /*Score*/ 100, /*bFromKit*/ true),
	});
	TestEqual(TEXT("a parita' esatta vince quella di kit"), Kit.AbilityIndex, 2);
	TestTrue(TEXT("e la ragione dichiarata e' lo spareggio di kit"),
		Kit.DecidedBy == ERTReactionTieBreak::Kit);

	// Due di kit a parita': vince l'indice piu' basso — che e' uno spareggio DIVERSO, e si deve leggere.
	const FRTReactionChoice PerIndice = URTHexBotLibrary::SelectReaction({
		Candidate(5, 100, true),
		Candidate(3, 100, true),
	});
	TestEqual(TEXT("fra due di kit a parita' vince l'indice piu' basso"), PerIndice.AbilityIndex, 3);
	TestTrue(TEXT("e la ragione e' l'indice, non il kit"),
		PerIndice.DecidedBy == ERTReactionTieBreak::Index);

	const FRTReactionChoice Nessuna = URTHexBotLibrary::SelectReaction({});
	TestEqual(TEXT("nessuna candidata da' INDEX_NONE"), Nessuna.AbilityIndex, (int32)INDEX_NONE);
	TestTrue(TEXT("e nessuna ragione"), Nessuna.DecidedBy == ERTReactionTieBreak::None);

	return true;
}

/**
 * La regressione che dimostra che [D-220] non e' piu' la politica: con punteggi diversi vince l'utilita'
 * piu' alta **anche se e' del loadout**.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionLoadoutCanWinTest,
	"RefactorTactics.HexBot.ReactionHigherUtilityWinsEvenFromTheLoadout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionLoadoutCanWinTest::RunTest(const FString&)
{
	const FRTReactionChoice Loadout = URTHexBotLibrary::SelectReaction({
		Candidate(/*kit*/ 2, /*Score*/ 10, true),
		Candidate(/*loadout*/ 7, /*Score*/ 100, false),
	});
	TestEqual(TEXT("vince il loadout quando vale di piu'"), Loadout.AbilityIndex, 7);
	TestTrue(TEXT("e ha vinto per punteggio, non per spareggio"),
		Loadout.DecidedBy == ERTReactionTieBreak::Utility);

	// E il verso opposto, perche' «vince sempre il loadout» passerebbe il test di sopra.
	const FRTReactionChoice Kit = URTHexBotLibrary::SelectReaction({
		Candidate(2, 100, true),
		Candidate(7, 10, false),
	});
	TestEqual(TEXT("vince il kit quando vale di piu'"), Kit.AbilityIndex, 2);
	TestTrue(TEXT("e anche qui per punteggio"), Kit.DecidedBy == ERTReactionTieBreak::Utility);

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
	const FRTReactionChoice ConKit = URTHexBotLibrary::SelectReaction({
		Candidate(/*loadout, primo nell'array*/ 7, 0, false),
		Candidate(/*kit*/ 2, 0, true),
	});
	TestEqual(TEXT("con tutti i punteggi a zero decide il kit, come oggi"), ConKit.AbilityIndex, 2);

	// Solo loadout disponibile: si arma quello, come oggi.
	const FRTReactionChoice SoloLoadout = URTHexBotLibrary::SelectReaction({ Candidate(7, 0, false) });
	TestEqual(TEXT("senza reazione di kit si arma il modulo, come oggi"), SoloLoadout.AbilityIndex, 7);
	TestTrue(TEXT("ed e' l'unica, quindi ha vinto per punteggio"),
		SoloLoadout.DecidedBy == ERTReactionTieBreak::Utility);

	return true;
}

/**
 * L'ordine di enumerazione non puo' cambiare l'esito a parita' di punteggi: e' l'AC di determinismo.
 *
 * ⚠️ **Si confrontano le permutazioni FRA LORO**, non con un numero scritto qui: la proprieta' e' «tutte
 * danno lo stesso», e un valore atteso a mano duplicherebbe il test dello spareggio senza aggiungere
 * niente. E si raccolgono tutti i sei esiti prima di giudicare: un `&&` in corto circuito smetterebbe di
 * chiamare la funzione dopo il primo scarto, e il referto non direbbe **quale** permutazione ha rotto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotReactionOrderInvariantTest,
	"RefactorTactics.HexBot.ReactionSelectionIsIndependentOfCandidateOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotReactionOrderInvariantTest::RunTest(const FString&)
{
	const FRTReactionCandidate Kit = Candidate(2, 100, true);
	const FRTReactionCandidate Loadout = Candidate(7, 100, false);
	const FRTReactionCandidate Altro = Candidate(9, 100, false);

	const TArray<TArray<FRTReactionCandidate>> Permutations = {
		{ Kit, Loadout, Altro }, { Kit, Altro, Loadout }, { Loadout, Kit, Altro },
		{ Loadout, Altro, Kit }, { Altro, Kit, Loadout }, { Altro, Loadout, Kit },
	};

	TArray<int32> Results;
	for (const TArray<FRTReactionCandidate>& P : Permutations)
	{
		Results.Add(URTHexBotLibrary::SelectReaction(P).AbilityIndex);
	}

	for (int32 i = 1; i < Results.Num(); ++i)
	{
		TestEqual(*FString::Printf(
			TEXT("la permutazione %d da' lo stesso esito della prima"), i), Results[i], Results[0]);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
