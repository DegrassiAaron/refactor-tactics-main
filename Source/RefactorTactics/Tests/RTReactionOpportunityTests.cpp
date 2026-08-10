// L'identita' di una `ReactionOpportunity` (CP 14.3).
//
// Il punto di questo file e' UNO: l'id di una opportunity si DERIVA dai sei campi che la individuano, e non
// si genera. Un GUID runtime sarebbe piu' comodo e romperebbe il replay in silenzio — due esecuzioni dello
// stesso scenario darebbero id diversi, quindi hash diversi, e la divergenza si presenterebbe come un difetto
// del resolver invece che come cio' che e': un identificatore che non e' una funzione del suo stato.

#include "Misc/AutomationTest.h"
#include "Turn/RTTurnRules.h"
#include "Turn/RTReactionOpportunityTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	FRTReactionOpportunityKey MakeOpportunityKeyForIdTest()
	{
		FRTReactionOpportunityKey Key;
		Key.TurnNumber = 3;
		Key.MacroPhase = ERTMatchPhase::Blast;
		Key.MicroStepIndex = 2;
		Key.OwnerId = 7;
		Key.ReactionDefId = TEXT("Action.Counter");
		Key.Seq = 0;
		return Key;
	}
}

/**
 * Stessi sei campi -> stesso id, e SEMPRE lo stesso: due derivazioni successive non possono divergere.
 *
 * E' la meta' del contratto che un GUID fallirebbe subito. L'altra meta' — campi diversi, id diversi — sta
 * nel test accanto: separate perche' falliscono per ragioni diverse, e un test che le mescola non dice quale
 * delle due proprieta' e' saltata.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpportunityIdIsDerivedNotRandomTest,
	"RefactorTactics.Reactions.OpportunityIdIsDerivedNotRandom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpportunityIdIsDerivedNotRandomTest::RunTest(const FString&)
{
	const FRTReactionOpportunityKey Key = MakeOpportunityKeyForIdTest();

	const FString First = URTReactionOpportunityLibrary::DeriveOpportunityId(Key);
	const FString Second = URTReactionOpportunityLibrary::DeriveOpportunityId(Key);

	// Un id vuoto passerebbe l'uguaglianza qui sotto senza identificare niente: e' il modo piu' facile in cui
	// questo test potrebbe diventare verde e inutile.
	TestFalse(TEXT("l'id derivato non e' vuoto"), First.IsEmpty());
	TestEqual(TEXT("due derivazioni dalla stessa chiave danno lo stesso id"), First, Second);

	// La chiave ricostruita da zero, non riusata: un id che dipendesse dall'ISTANZA invece che dai VALORI
	// passerebbe il confronto qui sopra e fallirebbe questo.
	const FString FromEquivalentKey = URTReactionOpportunityLibrary::DeriveOpportunityId(MakeOpportunityKeyForIdTest());
	TestEqual(TEXT("una chiave equivalente ricostruita da zero da' lo stesso id"), First, FromEquivalentKey);

	return true;
}

/**
 * Ognuno dei sei campi partecipa all'id: cambiarne uno solo lo cambia.
 *
 * Senza questo, `DeriveOpportunityId` potrebbe ignorare meta' della chiave e nessuno se ne accorgerebbe
 * finche' due opportunity distinte non collidessero — cioe' finche' il replay non attribuisse a una la
 * decisione dell'altra. Il campo che si dimentica piu' facilmente e' `Seq`, che esiste proprio per
 * distinguere due opportunity identiche in tutto il resto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpportunityIdUsesEveryFieldTest,
	"RefactorTactics.Reactions.OpportunityIdUsesEveryField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpportunityIdUsesEveryFieldTest::RunTest(const FString&)
{
	const FRTReactionOpportunityKey Base = MakeOpportunityKeyForIdTest();
	const FString BaseId = URTReactionOpportunityLibrary::DeriveOpportunityId(Base);

	auto DiffersFromBase = [this, &BaseId](const TCHAR* What, const FRTReactionOpportunityKey& Changed)
	{
		const FString ChangedId = URTReactionOpportunityLibrary::DeriveOpportunityId(Changed);
		TestNotEqual(FString::Printf(TEXT("cambiare %s cambia l'id"), What), ChangedId, BaseId);
	};

	{
		FRTReactionOpportunityKey K = Base; K.TurnNumber = 4;
		DiffersFromBase(TEXT("TurnNumber"), K);
	}
	{
		FRTReactionOpportunityKey K = Base; K.MacroPhase = ERTMatchPhase::Move;
		DiffersFromBase(TEXT("MacroPhase"), K);
	}
	{
		FRTReactionOpportunityKey K = Base; K.MicroStepIndex = 3;
		DiffersFromBase(TEXT("MicroStepIndex"), K);
	}
	{
		FRTReactionOpportunityKey K = Base; K.OwnerId = 8;
		DiffersFromBase(TEXT("OwnerId"), K);
	}
	{
		FRTReactionOpportunityKey K = Base; K.ReactionDefId = TEXT("Action.Deflect");
		DiffersFromBase(TEXT("ReactionDefId"), K);
	}
	{
		FRTReactionOpportunityKey K = Base; K.Seq = 1;
		DiffersFromBase(TEXT("Seq"), K);
	}

	return true;
}

/**
 * Due `FName` che l'engine considera la stessa azione danno lo stesso id, qualunque sia la loro casing.
 *
 * `FName` confronta senza distinguere maiuscole e minuscole, ma `ToString()` restituisce la casing
 * dell'ISTANZA. Senza normalizzazione, un chiamante che scrive `Action.Counter` e uno che scrive
 * `action.counter` — per l'engine la stessa azione, `==` vero — produrrebbero due opportunity con id diversi,
 * e il replay attribuirebbe a due opportunity distinte cio' che e' successo una volta sola.
 *
 * E' il difetto che il tipo `FName` risolve a meta': toglie l'ambiguita' dal CONFRONTO e la lascia nella
 * SERIALIZZAZIONE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpportunityIdIgnoresActionIdCasingTest,
	"RefactorTactics.Reactions.OpportunityIdIgnoresActionIdCasing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpportunityIdIgnoresActionIdCasingTest::RunTest(const FString&)
{
	FRTReactionOpportunityKey Lower = MakeOpportunityKeyForIdTest();
	Lower.ReactionDefId = FName(TEXT("action.counter"));

	FRTReactionOpportunityKey Upper = MakeOpportunityKeyForIdTest();
	Upper.ReactionDefId = FName(TEXT("ACTION.COUNTER"));

	// La premessa del test: per l'engine sono la stessa azione. Se un giorno non lo fossero piu', questo
	// assert cade per primo e dice perche', invece di lasciar fallire quello sotto senza spiegazione.
	if (!TestTrue(TEXT("i due FName sono uguali per l'engine"), Lower.ReactionDefId == Upper.ReactionDefId))
	{
		return false;
	}

	TestEqual(TEXT("e quindi danno lo stesso OpportunityId"),
		URTReactionOpportunityLibrary::DeriveOpportunityId(Lower),
		URTReactionOpportunityLibrary::DeriveOpportunityId(Upper));

	return true;
}

/**
 * La cardinalita' delle risposte legali decide il regime, e nient'altro (ADR-0004 §2).
 *
 * Le tre cardinalita' stanno in un test solo perche' sono **una** proprieta' — la soglia — e verificarne una
 * meta' non direbbe niente: con il solo caso `≤ 1` passerebbe un'implementazione che non apre MAI il
 * boundary, con il solo `≥ 2` una che lo apre SEMPRE. E' lo stesso motivo per cui
 * `Combat.BastionImpactShotSlows` ha un gemello di controllo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSingleResponseCommitsWithoutWindowTest,
	"RefactorTactics.Reactions.SingleResponseCommitsWithoutWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTSingleResponseCommitsWithoutWindowTest::RunTest(const FString&)
{
	FRTReactionOpportunity Opportunity;
	Opportunity.Key = MakeOpportunityKeyForIdTest();

	// Nessuna risposta legale: non c'e' niente da scegliere, quindi niente da chiedere.
	TestFalse(TEXT("zero risposte non aprono un boundary"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity));

	// Una sola risposta: il CASO DEGENERE, cioe' le reazioni E5 di oggi. `Counter`, `Deflect`, `Shield`,
	// `Cleanse` e il profilo base di `Brace` scattano o non scattano, e restano deterministiche.
	Opportunity.AllowedResponses = { TEXT("Hold Ground") };
	TestFalse(TEXT("una sola risposta si committa senza finestra"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity));

	// Due risposte: qui e solo qui nasce una scelta, e quindi una finestra.
	Opportunity.AllowedResponses = { TEXT("Hold Ground"), TEXT("Fire") };
	TestTrue(TEXT("due risposte aprono il decision boundary"),
		URTReactionOpportunityLibrary::RequiresDecisionBoundary(Opportunity));

	return true;
}

/**
 * Il DTO non porta futuro: nessuna posizione, nessun trigger, nessuna opportunity altrui, nessun intento
 * avversario (ADR-0004 §7, invariante #6).
 *
 * E' un test D'ARCHITETTURA e non di comportamento: interroga la reflection invece di eseguire il resolver,
 * perche' cio' che va impedito non e' un valore sbagliato ma un CAMPO IN PIU'. Un test che leggesse i valori
 * passerebbe felicemente accanto a un `FutureCells` lasciato vuoto quel giro.
 *
 * Il rosso di questo test si ottiene per MUTAZIONE — aggiungere un campo fuori elenco — e non scrivendolo
 * prima dell'implementazione: sul DTO conforme passa per costruzione. Verificato cosi': aggiungendo un
 * `UPROPERTY() FRTCellId PredictedCell` cade nominandolo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTOpportunityLeaksNoFutureTest,
	"RefactorTactics.Overwatch.OpportunityLeaksNoFuture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTOpportunityLeaksNoFutureTest::RunTest(const FString&)
{
	// Ogni campo che il DTO puo' avere, ed e' un ELENCO CHIUSO: chi ne aggiunge uno deve passare di qui e
	// dichiarare perche' non e' informazione futura. La lista corta e' il punto — non un dettaglio da tenere
	// aggiornato.
	auto CheckClosedFieldSet = [this](UScriptStruct* Struct, const TCHAR* What, const TSet<FString>& Allowed)
	{
		if (!Struct)
		{
			AddError(FString::Printf(TEXT("%s: struct non risolta dalla reflection"), What));
			return;
		}

		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			const FString Name = It->GetName();
			if (!Allowed.Contains(Name))
			{
				AddError(FString::Printf(
					TEXT("%s espone il campo '%s', che non e' nell'elenco chiuso: se e' informazione futura ")
					TEXT("(posizione, trigger, opportunity altrui, intento avversario) non puo' stare nel DTO; ")
					TEXT("se non lo e', va aggiunto qui con la ragione"),
					What, *Name));
			}
		}
	};

	CheckClosedFieldSet(FRTReactionOpportunity::StaticStruct(), TEXT("FRTReactionOpportunity"),
		{ TEXT("Key"), TEXT("AllowedResponses") });

	// La chiave viaggia dentro il DTO, quindi il suo contenuto e' altrettanto esposto. `MicroStepIndex` e'
	// passato: dice DOVE nella risoluzione ci si trova, non dove si andra'.
	CheckClosedFieldSet(FRTReactionOpportunityKey::StaticStruct(), TEXT("FRTReactionOpportunityKey"),
		{ TEXT("TurnNumber"), TEXT("MacroPhase"), TEXT("MicroStepIndex"),
		  TEXT("OwnerId"), TEXT("ReactionDefId"), TEXT("Seq") });

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
