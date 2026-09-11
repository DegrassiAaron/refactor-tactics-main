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

/**
 * La conoscenza MANCANTE non rende il bot onnisciente.
 *
 * 🔴 **Il difetto che questo test presidia e' stato reale, e la suite intera era verde** (#3025, trovato
 * in code review su #3020). `KnowledgeByTeam.FindRef(TeamId)` su una chiave assente restituisce un
 * `FRTTeamKnowledge` default-costruito, il cui `TeamId` vale **0**; e `ClassifyTarget` corto-circuita su
 * `TargetTeamId == Knowledge.TeamId` restituendo `Allowed`, perche' *«un alleato non passa dalla
 * conoscenza»*. Un bot di squadra diversa da 0 vedeva quindi **ogni** nemico della squadra 0 con cella
 * vera, salute vera e `Unbalanced` vero: la fuga che CP 13.5 esiste per chiudere, a favore del bot.
 *
 * ⚠️ **Perche' la suite restava verde**: `ARTTurnManager::PlanBots` popola la mappa per ogni squadra
 * presente, quindi dal chiamante di produzione il caso degradato non si presenta. Era irraggiungibile per
 * una proprieta' del CHIAMANTE, non per una difesa del codice — e `PlanTurn` e' una porta **pubblica**.
 *
 * 🔑 **La squadra del bot e' 1 e quella del nemico e' 0, e l'asimmetria e' il test**: col difetto la
 * conoscenza vuota si spaccia per «squadra 0», e il nemico di squadra 0 diventa un alleato da vedere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotPlanningMissingKnowledgeIsNotOmniscienceTest,
	"RefactorTactics.Bot.PlannerMissingKnowledgeIsNotOmniscience",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotPlanningMissingKnowledgeIsNotOmniscienceTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeFlatMap(4);

	// Adiacenti: col difetto il bot LO VEDE, e a portata 1 dichiara l'attacco. Senza, non lo vede affatto.
	TArray<FRTHexSimUnit> SimUnits;
	SimUnits.Add(FRTHexSimUnit(0, FRTCellId(0, 0, 0), /*budget*/ 2));
	SimUnits.Add(FRTHexSimUnit(1, FRTCellId(1, 0, 0), /*budget*/ 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, SimUnits);

	TArray<FRTBotUnitFacts> Facts;
	Facts.Add(MakeFacts(0, /*Team*/ 1, FRTCellId(0, 0, 0), /*bBot*/ true));   // il bot NON e' di squadra 0
	Facts.Add(MakeFacts(1, /*Team*/ 0, FRTCellId(1, 0, 0), /*bBot*/ false));  // il nemico SI'

	// 🔴 **Il bot deve avere un'abilita' d'attacco, altrimenti il test e' VACUO** — e la prima stesura lo
	// era: `AddCandidates` costruisce le candidate d'attacco solo da un'abilita' (`Ability->Power`), quindi
	// un'unita' senza abilita' non dichiara MAI un bersaglio e l'asserzione passava anche col difetto in
	// piedi. Misurato con la mutazione: il test restava verde. Un test che non cade non e' un gate.
	URTActionData* Colpo = NewObject<URTActionData>();
	Colpo->RangeCells = 1;
	Colpo->Power = 40;
	Facts[0].Abilities.Add(Colpo);
	Facts[0].bAbilityUsable.Add(true);

	FRTBotWeights Pesi;
	Pesi.WKill = 100;
	Pesi.WDamage = 50;
	Pesi.WApproach = 5;

	// ⛔ **VUOTA, ed e' il punto**: e' il caso degradato che dal chiamante di produzione non si presenta.
	TMap<int32, FRTTeamKnowledge> Conoscenza;
	TMap<int32, int32> Inattivita;
	TMap<int32, int32> UltimoRound;

	const FRTBotPlanningOutcome Esito = URTBotPlanningLibrary::PlanTurn(
		Snap, Facts, Pesi, Conoscenza, Inattivita, UltimoRound, /*TurnNumber*/ 1, /*bRecordAudit*/ false);

	if (!TestEqual(TEXT("un piano per il bot"), Esito.Decisions.Num(), 1))
	{
		return false;
	}

	// 🔴 Il bot non sa che quel nemico esiste: non puo' dichiarargli un attacco.
	TestEqual(TEXT("nessun bersaglio dichiarato: la squadra non lo conosce"),
		Esito.Decisions[0].PlannedAttackTargetIndex, static_cast<int32>(INDEX_NONE));

	// --- IL CONTROLLO POSITIVO, senza il quale l'asserzione sopra puo' marcire -------------------------
	//
	// 🔴 **Senza questa seconda meta' il test tornerebbe vacuo in silenzio**, ed e' un rilievo di code
	// review. L'asserzione sopra e' un `INDEX_NONE` atteso: passa anche se il bot smette di dichiarare
	// attacchi per una ragione che non c'entra — un cambio nel punteggio di `AddCandidates`, un altro
	// equilibrio fra `WDamage` e `WApproach`, un significato diverso di `RangeCells`. Sarebbe di nuovo un
	// verde che non puo' diventare rosso, che e' il difetto che questa issue esiste per chiudere.
	//
	// 🔑 **Qui la fixture dimostra da se' di poter produrre un attacco**: stessa scena, stessi pesi, e la
	// sola differenza e' che la squadra 1 VEDE la cella del nemico. Se questa meta' cade, l'altra non sta
	// piu' misurando la conoscenza — sta misurando un bot che non attacca mai.
	FRTTeamKnowledge Vista;
	Vista.TeamId = 1;
	Vista.VisibleCells.Add(FRTCellId(1, 0, 0)); // dove il nemico sta adesso -> `Detected`
	TMap<int32, FRTTeamKnowledge> ConoscenzaPiena;
	ConoscenzaPiena.Add(1, Vista);

	TMap<int32, int32> InattivitaB;
	TMap<int32, int32> UltimoRoundB;
	const FRTBotPlanningOutcome ConVista = URTBotPlanningLibrary::PlanTurn(
		Snap, Facts, Pesi, ConoscenzaPiena, InattivitaB, UltimoRoundB, /*TurnNumber*/ 1, /*bRecordAudit*/ false);

	if (TestEqual(TEXT("controllo positivo: un piano per il bot"), ConVista.Decisions.Num(), 1))
	{
		TestEqual(TEXT("controllo positivo: vedendolo, il bot LO dichiara bersaglio"),
			ConVista.Decisions[0].PlannedAttackTargetIndex, 1);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
