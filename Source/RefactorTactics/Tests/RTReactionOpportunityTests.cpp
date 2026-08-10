// L'identita' di una `ReactionOpportunity` (CP 14.3).
//
// Il punto di questo file e' UNO: l'id di una opportunity si DERIVA dai sei campi che la individuano, e non
// si genera. Un GUID runtime sarebbe piu' comodo e romperebbe il replay in silenzio — due esecuzioni dello
// stesso scenario darebbero id diversi, quindi hash diversi, e la divergenza si presenterebbe come un difetto
// del resolver invece che come cio' che e': un identificatore che non e' una funzione del suo stato.

#include "Misc/AutomationTest.h"
#include "Ability/RTActionDef.h"
#include "Turn/RTReactionOpportunityTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	FRTReactionOpportunityKey MakeOpportunityKeyForIdTest()
	{
		FRTReactionOpportunityKey Key;
		Key.TurnNumber = 3;
		Key.MacroPhase = ERTResolutionPhase::Attack;
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
		FRTReactionOpportunityKey K = Base; K.MacroPhase = ERTResolutionPhase::Control;
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

#endif // WITH_DEV_AUTOMATION_TESTS
