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
#include "Combat/RTCombatLibrary.h" // LowCoverDamageReduction: il canale si confronta col catalogo, non con 10
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

	/**
	 * Una copertura BASSA su un bordo di una cella gia' nella mappa.
	 *
	 * Scritta sul dato invece che via `URTHexCoverLibrary::AddCover`, perche' `MakeFlatMap` qui sopra
	 * riempie `Cells` a mano e non ordina: la via di produzione passa da `FindCell`, che su un array non
	 * ordinato non e' la stessa cosa. E' lo stesso idioma di `SetBotLowCover` in `RTHexBotTests.cpp`.
	 */
	void PosaCoperturaBassa(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		for (FRTHexCellData& Cella : Map->Cells)
		{
			if (Cella.Id == Id)
			{
				Cella.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::Low,
					FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
				return;
			}
		}
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
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshotOmniscient(M, SimUnits);

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
 * 🔴 DUE COMPAGNE NON SCELGONO LA STESSA CELLA — `#1088`, e da [D-446] va DIFESO invece che
 * ereditato.
 *
 * 🔑 **La proprieta' arrivava gratis da un rifiuto che non c'e' piu'.** Prima della scommessa,
 * `FindPathForUnit` rispondeva `NoPath` su una meta occupata, quindi alla seconda compagna quella cella
 * non si offriva. [D-446] fa riuscire quel percorso, e due bot che seguono lo stesso cammino troncano
 * alla **stessa** cella.
 *
 * 🔴 **E il pezzo che la difende NON e' quello che credevo, l'ha detto la mutazione.** Avevo scritto
 * qui che a tenerla fosse `CandidateCells` — il ventaglio meno le celle altrui — e rimettendo
 * `ReachableCells` nei suoi due siti del planner la suite **intera** restava verde, questo banco compreso.
 * Il pezzo che porta il peso e' il **ritorno all'ultimo passo LIBERO** nel ramo di ricerca: disabilitato
 * quello, le due scelgono entrambe `(q=0,r=0,L=0)` e il banco va rosso. Le due misure insieme dicono che
 * `CandidateCells` qui e' ridondante rispetto al ritorno — resta perche' restringe prima, e perche' e'
 * difeso per conto proprio da `Bot.ReservedDestinationBlocksTeammatesOnly`.
 *
 * ⚠️ **Senza questo banco la proprieta' era indifesa, ed e' cosi' che si e' scoperto**: una
 * mutazione che non uccide nessuno non dice *«il codice e' ridondante»*, dice *«nessuno sta guardando»*.
 *
 * ⛔ **Non asserisce QUALE cella scelgano**, come il banco qui sopra: asserisce che siano **due**.
 * Il contenuto della decisione appartiene allo scorer e ai banchi col mondo; cio' che si pinna qui e' che
 * la seconda compagna veda la prima.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotPlanningTeammatesPickDistinctCellsTest,
	"RefactorTactics.Bot.TeammatesDoNotPickTheSameCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotPlanningTeammatesPickDistinctCellsTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeFlatMap(5);

	// Due bot affiancati e NESSUN avversario: e' il ramo di RICERCA, dove entrambe seguono lo stesso
	// cammino verso il centro e troncano sulla stessa cella. E' il caso misurato in
	// `RTBotPlanningLibrary.cpp` — *«entrambe su (q=0,r=0,L=0), il centro»*.
	TArray<FRTHexSimUnit> SimUnits;
	SimUnits.Add(FRTHexSimUnit(0, FRTCellId(-3, 0, 0), /*budget*/ 3));
	SimUnits.Add(FRTHexSimUnit(1, FRTCellId(-4, 1, 0), /*budget*/ 4));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshotOmniscient(M, SimUnits);

	TArray<FRTBotUnitFacts> Facts;
	Facts.Add(MakeFacts(0, /*Team*/ 0, FRTCellId(-3, 0, 0), /*bBot*/ true));
	Facts.Add(MakeFacts(1, /*Team*/ 0, FRTCellId(-4, 1, 0), /*bBot*/ true));

	FRTBotWeights Pesi;
	Pesi.WKill = 100;
	Pesi.WDamage = 10;
	Pesi.WApproach = 5;

	TMap<int32, FRTTeamKnowledge> Conoscenza;
	TMap<int32, int32> Inattivita;
	TMap<int32, int32> UltimoRound;

	const FRTBotPlanningOutcome Esito = URTBotPlanningLibrary::PlanTurn(
		Snap, Facts, Pesi, Conoscenza, Inattivita, UltimoRound, /*TurnNumber*/ 1, /*bRecordAudit*/ false);

	if (!TestEqual(TEXT("due piani: uno per bot"), Esito.Decisions.Num(), 2)) { return false; }
	// ➕ **Le due destinazioni finiscono nel referto anche quando il banco e' verde.** Se un giorno
	// cade, cio' che serve sapere e' **su quale cella** sono finite insieme: un rosso che dice solo
	// «non sono distinte» manda a rileggere lo scorer, e il colpevole e' quasi sempre il troncamento.
	for (const FRTBotPlanDecision& D : Esito.Decisions)
	{
		AddInfo(FString::Printf(TEXT("u%d sceglie %s"), D.UnitIndex, *D.PlannedCell.ToString()));
	}

	// 🔑 **La premessa che rende il banco non vacuo: si muovono entrambe.** Due unita' ferme hanno
	// destinazioni distinte per costruzione, e l'asserzione sotto passerebbe senza misurare niente.
	const FRTCellId PartenzaA(-3, 0, 0);
	const FRTCellId PartenzaB(-4, 1, 0);
	if (!TestTrue(TEXT("premessa: la prima si muove"), Esito.Decisions[0].PlannedCell != PartenzaA)
		|| !TestTrue(TEXT("premessa: e anche la seconda"), Esito.Decisions[1].PlannedCell != PartenzaB))
	{
		return false;
	}

	TestNotEqual(TEXT("e le due destinazioni sono DISTINTE"),
		Esito.Decisions[0].PlannedCell, Esito.Decisions[1].PlannedCell);
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
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshotOmniscient(M, SimUnits);

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
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshotOmniscient(M, SimUnits);

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

/**
 * Il CANALE fra la stima del bot e chi la misura: la pianificazione riporta i punti di copertura che i piani
 * scelti si aspettano di scavalcare (`#649`).
 *
 * 🔴 **Esiste perche' senza di lui il cablaggio puo' staccarsi restando verde, e il referto pubblica allora
 * una diagnosi FALSA SUL BOT.** Il misuratore
 * (`Bot.FacingBypassRealizationOnACoveredArena`) somma questo valore e non lo asserisce mai — non puo': il
 * suo mestiere e' produrre un numero, non difenderne uno. Se la somma di `PlanTurn` o il travaso
 * nell'orchestratore sparissero, la suite resterebbe verde e il referto stamperebbe *«le coperture c'erano,
 * ma nessun piano SCELTO ha mai contato un punto da scavalcare»* — cioe' attribuirebbe al bot un silenzio
 * che e' del canale. E' un quarto zero, diverso dai tre che quel file dichiara di saper distinguere.
 *
 * 🔑 **Sta QUI e non nel misuratore perche' `PlanTurn` e' puro**: niente mondo, niente Actor, niente motore.
 * Il gate che serve e' sul cablaggio, e il cablaggio si vede da qui.
 *
 * ⛔ **Non e' una soglia sul comportamento del bot**, che `D-102` vieta di fissare su partite bot-vs-bot: e'
 * un'uguaglianza fra il valore riportato e la costante del catalogo di combattimento.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotPlanningCarriesPlannedBypassTest,
	"RefactorTactics.Bot.PlannerCarriesThePlannedCoverBypass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotPlanningCarriesPlannedBypassTest::RunTest(const FString&)
{
	// L'allestimento, una volta sola: cambia il solo orientamento del bersaglio fra le due meta'.
	auto Pianifica = [](ERTHexDirection FacingBersaglio, FRTBotPlanningOutcome& OutEsito)
	{
		URTHexMapAsset* M = MakeFlatMap(4);
		// Il bordo che il colpo attraversa: il bot sta a OVEST del bersaglio, quindi entra dal suo lato W.
		PosaCoperturaBassa(M, FRTCellId(1, 0, 0), ERTHexDirection::W);

		// Budget ZERO su entrambe: fermi. La geometria del colpo — e quindi il bordo attraversato — non
		// puo' cambiare per un movimento, che renderebbe il numero atteso una congettura.
		TArray<FRTHexSimUnit> SimUnits;
		SimUnits.Add(FRTHexSimUnit(0, FRTCellId(0, 0, 0), /*budget*/ 0));
		SimUnits.Add(FRTHexSimUnit(1, FRTCellId(1, 0, 0), /*budget*/ 0));
		const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshotOmniscient(M, SimUnits);

		TArray<FRTBotUnitFacts> Facts;
		Facts.Add(MakeFacts(0, /*Team*/ 1, FRTCellId(0, 0, 0), /*bBot*/ true));
		Facts.Add(MakeFacts(1, /*Team*/ 0, FRTCellId(1, 0, 0), /*bBot*/ false));
		Facts[1].Facing = FacingBersaglio;

		// Senza un'abilita' d'attacco il bot non dichiara nessun bersaglio e il test sarebbe vacuo: e' la
		// lezione gia' pagata da `PlannerMissingKnowledgeIsNotOmniscience` qui sopra.
		URTActionData* Colpo = NewObject<URTActionData>();
		Colpo->RangeCells = 1;
		Colpo->Power = 40;
		Facts[0].Abilities.Add(Colpo);
		Facts[0].bAbilityUsable.Add(true);

		FRTBotWeights Pesi;
		Pesi.WKill = 100;
		Pesi.WDamage = 50;
		Pesi.WApproach = 5;

		// La squadra del bot VEDE la cella del bersaglio: senza, non lo conosce e non lo attacca.
		FRTTeamKnowledge Vista;
		Vista.TeamId = 1;
		Vista.VisibleCells.Add(FRTCellId(1, 0, 0));
		TMap<int32, FRTTeamKnowledge> Conoscenza;
		Conoscenza.Add(1, Vista);

		TMap<int32, int32> Inattivita;
		TMap<int32, int32> UltimoRound;
		OutEsito = URTBotPlanningLibrary::PlanTurn(
			Snap, Facts, Pesi, Conoscenza, Inattivita, UltimoRound, /*TurnNumber*/ 1, /*bRecordAudit*/ false);
	};

	// --- IL BERSAGLIO VOLTA LE SPALLE: la copertura non lo protegge, e il piano lo conta ----------------
	FRTBotPlanningOutcome Scoperto;
	Pianifica(ERTHexDirection::E, Scoperto); // guarda a est, il bot e' a ovest: colpo posteriore

	if (!TestEqual(TEXT("premessa: un piano per il bot"), Scoperto.Decisions.Num(), 1))
	{
		return false;
	}
	// 🔴 **La premessa che rende leggibile il numero**: se il bot non attaccasse, lo zero dell'altra meta'
	// non direbbe niente sulla copertura.
	TestEqual(TEXT("premessa: il bot dichiara il bersaglio"),
		Scoperto.Decisions[0].PlannedAttackTargetIndex, 1);

	TestEqual(TEXT("la pianificazione riporta i punti che il piano si aspetta di scavalcare"),
		Scoperto.PlannedCoverBypassedByFacing, URTCombatLibrary::LowCoverDamageReduction);

	// --- LA META' FALSIFICANTE: stessa scena, il bersaglio guarda il bot -------------------------------
	//
	// ⛔ Senza di lei passerebbe un canale che riporta la riduzione NOMINALE invece di quella ANNULLATA:
	// sono lo stesso numero quando la direzione la scavalca, e divergono solo qui.
	FRTBotPlanningOutcome Coperto;
	Pianifica(ERTHexDirection::W, Coperto); // guarda il bot: colpo frontale, la copertura tiene

	if (TestEqual(TEXT("controllo positivo: il bot attacca comunque"), Coperto.Decisions.Num(), 1))
	{
		TestEqual(TEXT("controllo positivo: e dichiara lo stesso bersaglio"),
			Coperto.Decisions[0].PlannedAttackTargetIndex, 1);
	}
	TestEqual(TEXT("dove la copertura TIENE non c'e' niente di scavalcato da riportare"),
		Coperto.PlannedCoverBypassedByFacing, 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
