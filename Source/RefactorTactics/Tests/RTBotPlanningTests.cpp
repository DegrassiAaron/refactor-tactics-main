// Copyright RefactorTactics. All Rights Reserved.
//
// La decisione del bot, provata SENZA UN MONDO (#3013, fetta di #1818).
//
// 🔴 **E' la capacita' che questa fetta esiste per produrre, e prima non c'era.** Ogni test di
// pianificazione del repository passa da `ARTTurnManager::PlanBotsForTest()`, che vuole un
// `ARTTurnManager` spawnato in un mondo: xx file lo fanno — `grep -rln "PlanBotsForTest" Source/RefactorTactics | wc -l`.
// Qui non si spawna niente: si costruiscono i fatti, si chiama `URTBotPlanningLibrary::PlanTurn`, si
// guarda cosa ha deciso.
//
// ⚠️ **Questi test NON sostituiscono quelli col mondo, e non e' un ripiego**: #3013 dichiara gameplay
// invariato, quindi i test esistenti devono restare verdi esattamente come sono — sono loro l'oracolo che
// la decisione non e' cambiata. Questi provano che la decisione si possa raggiungere **anche** da qui.

#include "Misc/AutomationTest.h"

#include "Ability/RTActionData.h"
#include "Bot/RTBotPlanningLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTHexSimLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

// ⚠️ **Namespace NOMINATO, non anonimo**: sotto unity build i file dello stesso modulo finiscono in una
// traduzione sola, e due `MakeFlatMap` in namespace anonimi diversi collidono. Il repository ha un gate che
// lo misura — `Meta.AnonymousHelpersDoNotCollideUnderUnity` — ed e' caduto su questo file alla prima stesura,
// perche' `RTHexOccupancyTests.cpp` ne definisce gia' uno con lo stesso nome.
namespace RTBotPlanningTestsInternal
{
	/** Una mappa piatta NxN senza ostacoli: la geometria non e' l'oggetto di questi test. */
	URTHexMapAsset* MakeFlatMap(int32 Raggio)
	{
		URTHexMapAsset* M = NewObject<URTHexMapAsset>();
		for (int32 X = -Raggio; X <= Raggio; ++X)
		{
			for (int32 Y = -Raggio; Y <= Raggio; ++Y)
			{
				FRTHexCellData Cella;
				Cella.Id = FRTCellId(X, Y, 0);
				M->Cells.Add(Cella);
			}
		}
		return M;
	}

	/** I fatti di un'unita' senza abilita': il minimo perche' il planner la consideri. */
	FRTBotUnitFacts MakeFacts(int32 Index, int32 TeamId, const FRTCellId& Cella, bool bBot)
	{
		FRTBotUnitFacts F;
		F.Index = Index;
		F.StableUnitId = 100 + Index;
		F.TeamId = TeamId;
		F.bIsBotControlled = bBot;
		F.bAlive = true;
		F.DisplayName = FString::Printf(TEXT("Unita%d"), Index);
		F.Cell = Cella;
		F.Health = 100;
		F.MaxHealth = 100;
		F.AttackRange = 1;
		return F;
	}
}

using namespace RTBotPlanningTestsInternal;

/**
 * Il caso minimo: un bot decide, e la decisione esce dalla funzione invece che dall'Actor.
 *
 * 🔑 **Non asserisce QUALE cella scelga** — quello lo presidiano i test col mondo, ed e' giusto che sia
 * cosi' finche' #3013 dichiara gameplay invariato. Asserisce che la decisione **esista e sia
 * indirizzata**: un piano per il bot, nessuno per l'unita' che bot non e'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotPlanningDecidesWithoutWorldTest,
	"RefactorTactics.Bot.PlannerDecidesWithoutAWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotPlanningDecidesWithoutWorldTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeFlatMap(4);

	TArray<FRTHexSimUnit> SimUnits;
	SimUnits.Add(FRTHexSimUnit(0, FRTCellId(0, 0, 0), /*budget*/ 3));
	SimUnits.Add(FRTHexSimUnit(1, FRTCellId(3, 0, 0), /*budget*/ 3));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, SimUnits);

	TArray<FRTBotUnitFacts> Facts;
	Facts.Add(MakeFacts(0, /*Team*/ 0, FRTCellId(0, 0, 0), /*bBot*/ true));
	Facts.Add(MakeFacts(1, /*Team*/ 1, FRTCellId(3, 0, 0), /*bBot*/ false));

	FRTBotWeights Pesi;
	Pesi.WKill = 100;
	Pesi.WDamage = 10;
	Pesi.WApproach = 5;

	TMap<int32, FRTTeamKnowledge> Conoscenza;
	TMap<int32, int32> Inattivita;
	TMap<int32, int32> UltimoRound;

	const FRTBotPlanningOutcome Esito = URTBotPlanningLibrary::PlanTurn(
		Snap, Facts, Pesi, Conoscenza, Inattivita, UltimoRound, /*TurnNumber*/ 1, /*bRecordAudit*/ false);

	// ⛔ **Un piano per ogni BOT, e per nessun altro.** L'unita' del team 1 non e' guidata dal bot: se
	// comparisse fra le decisioni, l'orchestratore ne sovrascriverebbe il piano — che per un giocatore
	// umano significa perdere l'intento appena dichiarato.
	TestEqual(TEXT("un piano, e uno solo: il bot"), Esito.Decisions.Num(), 1);
	if (Esito.Decisions.Num() == 1)
	{
		TestEqual(TEXT("il piano e' indirizzato al bot, per indice di snapshot"),
			Esito.Decisions[0].UnitIndex, 0);
	}

	return true;
}

/**
 * L'audit costa, e si paga solo quando lo si chiede.
 *
 * ⚠️ Era `bRecordReplay` letto dall'orchestratore; ora e' un parametro, e questo test e' l'unico posto in
 * cui la differenza fra i due modi si osserva direttamente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotPlanningAuditIsOptInTest,
	"RefactorTactics.Bot.PlannerAuditIsOptIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotPlanningAuditIsOptInTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeFlatMap(4);

	TArray<FRTHexSimUnit> SimUnits;
	SimUnits.Add(FRTHexSimUnit(0, FRTCellId(0, 0, 0), /*budget*/ 3));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, SimUnits);

	TArray<FRTBotUnitFacts> Facts;
	Facts.Add(MakeFacts(0, /*Team*/ 0, FRTCellId(0, 0, 0), /*bBot*/ true));

	FRTBotWeights Pesi;
	TMap<int32, FRTTeamKnowledge> Conoscenza;
	TMap<int32, int32> Inattivita;
	TMap<int32, int32> UltimoRound;

	const FRTBotPlanningOutcome Spento = URTBotPlanningLibrary::PlanTurn(
		Snap, Facts, Pesi, Conoscenza, Inattivita, UltimoRound, 1, /*bRecordAudit*/ false);
	TestEqual(TEXT("senza audit non si registra nulla"), Spento.AuditDecisions.Num(), 0);

	const FRTBotPlanningOutcome Acceso = URTBotPlanningLibrary::PlanTurn(
		Snap, Facts, Pesi, Conoscenza, Inattivita, UltimoRound, 1, /*bRecordAudit*/ true);
	TestEqual(TEXT("con l'audit si registra una voce per bot"), Acceso.AuditDecisions.Num(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
