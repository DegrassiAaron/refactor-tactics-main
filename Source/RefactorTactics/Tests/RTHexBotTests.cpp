#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h" // tiene vive le arene del test attraverso un eventuale GC
#include "Turn/RTMatchSetupLibrary.h"
#include "Ability/RTActionData.h"
#include "Bot/RTHexBotLibrary.h"
#include "Combat/RTCombatLibrary.h" // LowCoverDamageReduction: il bonus direzionale si confronta col catalogo
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "Map/RTHexLibrary.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTHexVisionLibrary.h" // #1300: la premessa del termine di ingaggio si asserisce, non si spera
#include "Turn/RTHexSim.h"
#include "Turn/RTTurnManager.h" // l'invariante WElevation<WApproach si misura su ENTRAMBE le sorgenti (#1088)
#include "Turn/RTHexSimLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Esagono pieno di raggio N sul layer 0. Nome distinto per file (unity build). */
	URTHexMapAsset* MakeBotMap(int32 Radius)
	{
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		return M;
	}

	/**
	 * Come `MakeBotMap`, piu' una colonna di celle REALI sui layer 1..`Layers`-1 sopra l'origine e sopra la
	 * cella (1,0), **collegate al piano di sotto da una transizione per livello**.
	 *
	 * 🔴 **Le transizioni mancavano, e senza di loro il test qui sotto misurava un'altra cosa.** La prima
	 * stesura aggiungeva le celle e basta: nessun arco fra i layer, quindi `GraphNeighbors` non ne conosce
	 * nessuno. Da quando l'avvicinamento si misura in passi sul grafo, `ApproachSteps` verso una cella in
	 * quota rispondeva `NoPath` e ripiegava su `HexDistance` — che il layer lo ignora. Il risultato:
	 * `ElevationNeverOutweighsClosingOneCell`, cioe' l'UNICO test che pinna l'invariante
	 * `WElevation * MaxLayer < WApproach`, passava esercitando il ripiego invece della metrica nuova.
	 * Trovato in code review.
	 *
	 * ⚠️ **Il costo dell'arco e' 1**, non 2 come sulla piattaforma di `ArenaV01`: qui interessa che il
	 * grafo sia CONNESSO, e un costo diverso da uno mescolerebbe due domande nello stesso test.
	 * ⚠️ Il commento precedente diceva che «nessun ramo di `ScorePlan` interroga la mappa quando
	 * `EnemyRanges[0] == 0`»: non e' piu' vero, `ApproachSteps` la interroga sempre.
	 */
	URTHexMapAsset* MakeLayeredBotMap(int32 Radius, int32 Layers)
	{
		URTHexMapAsset* M = MakeBotMap(Radius);
		for (int32 L = 1; L < Layers; ++L)
		{
			M->AddOrUpdateCell(FRTHexCellData(FRTCellId(0, 0, L)));
			M->AddOrUpdateCell(FRTHexCellData(FRTCellId(1, 0, L)));
			M->AddTransition(FRTCellId(0, 0, L - 1), FRTCellId(0, 0, L), /*Cost=*/ 1);
			M->AddTransition(FRTCellId(1, 0, L - 1), FRTCellId(1, 0, L), /*Cost=*/ 1);
		}
		M->SortCells();
		return M;
	}

	void BlockBotSight(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.Id = Id;
		Data.bBlocksLineOfSight = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Muro che non si attraversa: serve a distinguere i PASSI dalla distanza in linea d'aria (#1296). */
	void BlockBotMove(URTHexMapAsset* Map, const FRTCellId& Id)
	{
		FRTHexCellData Data = Map->FindCell(Id) ? *Map->FindCell(Id) : FRTHexCellData(Id);
		Data.Id = Id;
		Data.bBlocksMovement = true;
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Come `MakeBotMap`, con una cella marcata OBIETTIVO (`bIsObjective`, formato mappa v11 — #75). */
	URTHexMapAsset* MakeObjectiveBotMap(int32 Radius, const FRTCellId& Objective)
	{
		URTHexMapAsset* M = MakeBotMap(Radius);
		FRTHexCellData Data = M->FindCell(Objective) ? *M->FindCell(Objective) : FRTHexCellData(Objective);
		Data.Id = Objective;
		Data.bIsObjective = true;
		M->AddOrUpdateCell(Data);
		M->SortCells();
		return M;
	}

	/** Contesto minimo: un nemico con gittata e HP dati. */
	FRTHexBotContext MakeCtx(const FRTCellId& Origin, const FRTCellId& Enemy, int32 EnemyRange, int32 EnemyHealth)
	{
		FRTHexBotContext Ctx;
		Ctx.Origin = Origin;
		Ctx.Enemies.Add(Enemy);
		Ctx.EnemyRanges.Add(EnemyRange);
		Ctx.EnemyHealth.Add(EnemyHealth);
		Ctx.AttackRange = 3;
		Ctx.AttackDamage = 30;
		return Ctx;
	}

	/**
	 * Contesto con attacco AD AREA di raggio 1 e fuoco amico attivo — il caso di `Gadget.Overload` (#213).
	 * Gittata nemica 0: nessuna minaccia sulla cella, cosi' il punteggio isola il collaterale.
	 */
	FRTHexBotContext MakeAreaCtx(const FRTCellId& Origin, const FRTCellId& Enemy, int32 Damage)
	{
		FRTHexBotContext Ctx = MakeCtx(Origin, Enemy, /*EnemyRange*/ 0, /*EnemyHealth*/ 100);
		Ctx.AttackDamage = Damage;
		Ctx.AttackShape = ERTAbilityShape::Area;
		Ctx.AttackAreaRadius = 1;
		Ctx.bAttackFriendlyFire = true;
		return Ctx;
	}

	/** Piano con attacco AD AREA raggio 1 e fuoco amico: la forma viaggia col piano, non col contesto. */
	FRTHexBotPlan MakeAreaPlan(const FRTCellId& Dest, int32 Damage, int32 TargetHealth)
	{
		FRTHexBotPlan P;
		P.DestCell = Dest;
		P.bHasAttack = true;
		P.TargetIndex = 0;
		P.AttackDamage = Damage;
		P.TargetHealth = TargetHealth;
		P.Shape = ERTAbilityShape::Area;
		P.AreaRadius = 1;
		P.bFriendlyFire = true;
		return P;
	}

	/**
	 * ⚠️ `FromCell` resta al default `(0,0,0)`: va bene finche' l'origine del contesto e' quella, e NON va
	 * bene altrove — `ScorePlan` legge `Plan.FromCell` (`ArrivalFacingOf`), quindi un piano che parte da una
	 * cella su cui l'unita' non e' mai stata produce un facing inventato. Per le origini diverse c'e'
	 * `MakePlanFrom`.
	 */
	FRTHexBotPlan MakePlan(const FRTCellId& Dest, bool bAttack = false, int32 Damage = 0, int32 TargetHealth = 0)
	{
		FRTHexBotPlan P;
		P.DestCell = Dest;
		P.bHasAttack = bAttack;
		P.TargetIndex = bAttack ? 0 : INDEX_NONE;
		P.AttackDamage = Damage;
		P.TargetHealth = TargetHealth;
		return P;
	}

	/** Come `MakePlan`, ma dichiara da DOVE si parte: obbligatorio quando `Ctx.Origin` non e' `(0,0,0)`. */
	FRTHexBotPlan MakePlanFrom(const FRTCellId& From, const FRTCellId& Dest)
	{
		FRTHexBotPlan P = MakePlan(Dest);
		P.FromCell = From;
		return P;
	}
}

// ---------------------------------------------------------------------------------------------------------
// Scoring
// ---------------------------------------------------------------------------------------------------------

/**
 * **Due mappe vive non si scambiano il campo di distanze.**
 *
 * `StepsToGoalField` cachea una BFS con chiave `(asset, Revision, goal)`. Dopo [D-196] la `Revision` non
 * distingue piu' due arene piatte — nascono tutte con `1`, mentre prima ne portavano una per cella e si
 * discriminavano **per caso** — quindi a tenerle separate resta la sola identita' dell'asset.
 *
 * ⚠️ **Cosa questo test NON prova.** `#1436` riguarda il riuso di un INDIRIZZO dopo il GC: due oggetti
 * diversi che finiscono allo stesso posto, dove un puntatore grezzo non li distingue e `FObjectKey` — che
 * porta anche il serial number — si'. Qui le due arene sono vive **insieme**, quindi hanno indirizzi
 * diversi e il test resta verde anche col puntatore grezzo: verificato per mutazione, e la prima stesura di
 * questo commento sosteneva il contrario.
 *
 * Riprodurre il riuso vorrebbe dire pilotare collettore e allocatore di UObject, e un test che ci prova
 * senza garanzie sarebbe intermittente — peggio di non averlo. Questo pinna la proprieta' piu' debole che
 * si puo' pinnare in modo deterministico: **l'asset e' nella chiave**, che e' il modo in cui il difetto
 * tornerebbe per una svista.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotFieldCacheKeepsMapsApartTest,
	"RefactorTactics.HexBot.PathFieldCacheKeepsLiveMapsApart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotFieldCacheKeepsMapsApartTest::RunTest(const FString&)
{
	// ⚠️ `TStrongObjectPtr`: sono UObject non rooted, e un GC fra qui e la fine del test li porterebbe via
	// lasciando due puntatori penzolanti. In un test che parla di identita' attraverso il GC, la premessa
	// dev'essere sicura invece che fortunata.
	TStrongObjectPtr<URTHexMapAsset> Libera(MakeBotMap(4));
	TStrongObjectPtr<URTHexMapAsset> ConMuro(MakeBotMap(4));
	if (!TestNotNull(TEXT("arena libera"), Libera.Get()))
	{
		return false;
	}
	if (!TestNotNull(TEXT("arena col muro"), ConMuro.Get()))
	{
		return false;
	}

	// Il muro si aggiunge con UNA `ReplaceContent`, non ricostruendo l'arena a mano: cosi' il builder
	// condiviso resta nel percorso.
	//
	// ⚠️ La stessa operazione si applica a ENTRAMBE, anche a quella che non cambia: `ReplaceContent` muove
	// la `Revision`, quindi rimpiazzare le celle di una sola le renderebbe distinguibili proprio per il
	// campo che questo test vuole neutralizzare. La prima stesura lo faceva, e la premessa cadeva.
	auto Rimpiazza = [](URTHexMapAsset* Mappa, bool bConMuro)
	{
		TArray<FRTHexCellData> Celle;
		for (const FRTHexCellData& Cella : Mappa->Cells)
		{
			FRTHexCellData Copia = Cella;
			// Colonna q=0 murata con UN varco in cima (r = -4): obbliga a girarci intorno, quindi i PASSI
			// cambiano mentre la distanza in linea d'aria no.
			//
			// ⚠️ Il varco serve: murata tutta, l'arena si spezza in due e `ApproachSteps` ricade su
			// `HexDistance` — le due mappe tornerebbero a dare lo stesso numero, verde per il motivo
			// sbagliato. E l'origine va murata: lasciarla libera lascia aperta la retta fra le due celle.
			if (bConMuro && Copia.Id.X == 0 && Copia.Id.Y > -4)
			{
				Copia.bBlocksMovement = true;
			}
			Celle.Add(Copia);
		}
		Mappa->ReplaceContent(Celle, {});
		Mappa->SortCells();
	};
	Rimpiazza(ConMuro.Get(), /*bConMuro=*/ true);
	Rimpiazza(Libera.Get(), /*bConMuro=*/ false);

	// La premessa che rende il test significativo: la `Revision` NON le distingue.
	TestEqual(TEXT("premessa: le due arene portano la stessa Revision"),
		Libera->Revision, ConMuro->Revision);

	FRTHexBotContext Ctx;
	Ctx.Enemies.Add(FRTCellId(3, 0, 0));
	// ⚠️ `EnemyRanges` va popolato: il termine di avvicinamento gira su
	// `Min(Enemies.Num(), EnemyRanges.Num())`, quindi senza questa riga il ciclo NON parte, `ApproachSteps`
	// non viene mai chiamato e i due punteggi coincidono — un test verde per il motivo sbagliato, che e'
	// come questa fixture ha fallito la prima volta.
	Ctx.EnemyRanges.Add(1);

	// Nessun attacco: si misura il solo termine di avvicinamento. `TargetIndex` resta `INDEX_NONE` come
	// vuole la convenzione di questo file per i piani senza attacco.
	FRTHexBotPlan Plan;
	Plan.FromCell = FRTCellId(-3, 0, 0);
	Plan.DestCell = FRTCellId(-3, 0, 0);
	Plan.TargetIndex = INDEX_NONE;
	Plan.bHasAttack = false;

	// L'ordine conta: la prima chiamata popola la cache, la seconda la troverebbe se la chiave non
	// distinguesse le due mappe.
	const int32 ScoreConMuro = URTHexBotLibrary::ScorePlan(ConMuro.Get(), Plan, Ctx);
	const int32 ScoreLibera = URTHexBotLibrary::ScorePlan(Libera.Get(), Plan, Ctx);

	AddInfo(FString::Printf(TEXT("punteggi: col muro %d, libera %d"), ScoreConMuro, ScoreLibera));

	// Girare intorno al muro costa passi, e l'avvicinamento entra nel punteggio: due topologie diverse non
	// possono dare lo stesso numero, a meno che la seconda non abbia riletto il campo della prima.
	TestNotEqual(TEXT("due mappe diverse non condividono il campo di distanze"),
		ScoreConMuro, ScoreLibera);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotFocusFireTest,
	"RefactorTactics.HexBot.ScoreFocusFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotFocusFireTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Enemy(3, 0);
	const FRTHexBotContext Ctx = MakeCtx(Origin, Enemy, /*range*/ 0, /*hp*/ 100);

	const int32 NoAttack = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin), Ctx);
	const int32 Weak = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin, true, 20, 100), Ctx);
	const int32 Strong = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin, true, 40, 100), Ctx);
	const int32 Lethal = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin, true, 40, 35), Ctx);

	TestTrue(TEXT("attaccare batte non attaccare"), Weak > NoAttack);
	TestTrue(TEXT("piu' danno vale di piu'"), Strong > Weak);
	TestTrue(TEXT("il colpo letale domina"), Lethal > Strong);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotCoverTest,
	"RefactorTactics.HexBot.ScoreThreatRespectsCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotCoverTest::RunTest(const FString&)
{
	const FRTCellId Exposed(0, 0);
	const FRTCellId Enemy(3, 0);
	FRTHexBotContext Ctx = MakeCtx(Exposed, Enemy, /*range*/ 5, /*hp*/ 100);
	Ctx.KiteStandoff = 0;
	Ctx.WApproach = 0; // isola il contributo della minaccia
	// 🔴 **E isola anche il termine di ingaggio** (#1300, D-185), per la stessa ragione dell'`WApproach`
	// qui sopra: su campo aperto ogni cella vede il nemico, quindi il bonus varrebbe su TUTTE le celle
	// tranne quella coperta e i due zeri assoluti che questo test pinna diventerebbero `+WEngage`.
	// ⚠️ Non e' un indebolimento: quei due zeri dicono cosa fa la MINACCIA, e la minaccia non e' cambiata.
	// Col termine acceso i numeri sarebbero `−85` sotto tiro e `+15` fuori gittata — l'ordinamento regge,
	// gli assoluti no. Il termine ha il proprio oracolo in `EngageBonusFadesWithIdleTurns`.
	Ctx.WEngage = 0;

	URTHexMapAsset* Open = MakeBotMap(4);
	const int32 UnderFire = URTHexBotLibrary::ScorePlan(Open, MakePlan(Exposed), Ctx);

	URTHexMapAsset* Covered = MakeBotMap(4);
	BlockBotSight(Covered, FRTCellId(2, 0)); // muro fra nemico e cella
	const int32 BehindCover = URTHexBotLibrary::ScorePlan(Covered, MakePlan(Exposed), Ctx);

	TestTrue(TEXT("la cella sotto tiro e' penalizzata"), UnderFire < 0);
	TestEqual(TEXT("dietro copertura nessuna penalita' di minaccia"), BehindCover, 0);
	TestTrue(TEXT("la copertura migliora il punteggio"), BehindCover > UnderFire);

	// Fuori dalla gittata nemica non c'e' minaccia, anche senza copertura.
	FRTHexBotContext ShortRange = Ctx;
	ShortRange.EnemyRanges[0] = 1;
	TestEqual(TEXT("fuori gittata nessuna minaccia"),
		URTHexBotLibrary::ScorePlan(Open, MakePlan(Exposed), ShortRange), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotKiterVsMeleeTest,
	"RefactorTactics.HexBot.ScoreKiterVsMelee",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotKiterVsMeleeTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);
	const FRTCellId Enemy(4, 0);
	FRTHexBotContext Ctx = MakeCtx(FRTCellId(0, 0), Enemy, /*range*/ 0, /*hp*/ 100);
	// 🔴 **Il termine di ingaggio si spegne qui** (#1300, D-185): la mappa e' aperta, quindi da ogni cella
	// si vede il nemico e il bonus sarebbe una costante `+WEngage` su tutte e cinque le candidate. Non
	// sposterebbe nessuno degli ordinamenti in prova — misurato: `0 → +15`, `−10 → +5`, `−20 → −5`, e la
	// riga «il kiter torna verso la distanza di sicurezza» resta vera — ma renderebbe i tre valori
	// ASSOLUTI qui sotto la somma di due termini, cioe' un numero che si muove quando si tara l'altro.
	Ctx.WEngage = 0;

	// Mischia: piu' vicino = meglio.
	Ctx.KiteStandoff = 0;
	const int32 MeleeNear = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(3, 0)), Ctx);
	const int32 MeleeFar = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(1, 0)), Ctx);
	TestTrue(TEXT("la mischia preferisce chiudere la distanza"), MeleeNear > MeleeFar);

	// Kiter con standoff 3: sotto la soglia viene penalizzato in proporzione.
	Ctx.KiteStandoff = 3;
	const int32 KiterAtStandoff = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(1, 0)), Ctx); // dist 3
	const int32 KiterTooClose = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(3, 0)), Ctx);   // dist 1
	TestTrue(TEXT("il kiter preferisce restare alla distanza di sicurezza"), KiterAtStandoff > KiterTooClose);
	TestEqual(TEXT("alla distanza di sicurezza nessuna penalita'"), KiterAtStandoff, 0);

	// 🔴 **Oltre lo standoff si paga per riavvicinarsi, ed e' il ramo che chiude #1088.** Non esisteva:
	// sopra la distanza di sicurezza nessun termine di distanza si applicava, quindi per un kiter
	// l'elevazione restava l'unico termine posizionale e restare in quota vinceva con qualunque
	// `WElevation > 0`. Senza queste due righe, cancellare il ramo lascia il test verde.
	//
	// ⚠️ Le celle si scelgono per distanza dal NEMICO, non dall'origine: `Enemy` sta a (4,0), quindi (0,0)
	// dista 4 e (-1,0) dista 5. Sbagliare riferimento porta la candidata sotto lo standoff, dove a rispondere
	// e' `WKiteViolation` e non il ramo in prova.
	const int32 KiterOneBeyond = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(0, 0)), Ctx);   // dist 4
	TestEqual(TEXT("una cella oltre lo standoff costa WApproach"), KiterOneBeyond, -Ctx.WApproach);

	const int32 KiterTwoBeyond = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(-1, 0)), Ctx);  // dist 5
	TestEqual(TEXT("e due celle ne costano il doppio"), KiterTwoBeyond, -2 * Ctx.WApproach);

	TestTrue(TEXT("quindi il kiter torna verso la distanza di sicurezza invece di allontanarsi"),
		KiterAtStandoff > KiterOneBeyond && KiterOneBeyond > KiterTwoBeyond);

	// ⚠️ **E il costo dichiarato**: allontanarsi oltre lo standoff paga, quindi un kiter con portata
	// maggiore dello standoff rinuncia a parte della propria gittata. E' la scelta di #1088 — un bot che
	// non conclude e' un difetto, due celle di gittata sono bilanciamento (#149).
	return true;
}

/**
 * **Il bonus di ingaggio compra la deviazione quando il bot e' fresco, e smette di pagare quando e' fermo.**
 *
 * E' l'oracolo di `WEngage`/`WEngageDecay` (#1300, `D-185`), e misura l'**ESITO** di `ChooseBestPlan` — non
 * il punteggio di un piano isolato, che cambierebbe senza che l'ordinamento si muova. Stessa disciplina di
 * `ElevationNeverOutweighsClosingOneCell`, e per la stessa ragione: la difesa contro lo stato assorbente e'
 * un rapporto fra numeri, non una forma, quindi va misurata dove il rapporto decide qualcosa.
 *
 * 🔴 **Senza questo test `WEngageDecay = 0` non lo noterebbe nessuno**, e il termine tornerebbe alla forma
 * puramente posizionale — quella per cui **non esiste alcun peso** che passi entrambi gli oracoli di
 * parcheggio (misurato intero per intero il 2026-08-24: l'arena generata cade da `W = 7`, la mappa d'autore
 * si sblocca da `W = 11`).
 *
 * L'allestimento evita la geometria a memoria: le due candidate si passano a mano, e la **precondizione si
 * asserisce** invece di sperarla — senza il termine deve vincere la cella cieca, e il divario dev'essere
 * minore del bonus fresco, altrimenti il test passerebbe anche con un bonus che non serve a niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotEngageFadesTest,
	"RefactorTactics.HexBot.EngageBonusFadesWithIdleTurns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotEngageFadesTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Enemy(4, 0);
	BlockBotSight(M, FRTCellId(2, 0)); // schermo sull'asse: chi sta dietro non vede (e passa lo stesso)

	const FRTCellId Cieca(1, 0);    // piu' vicina al nemico, ma lo schermo le toglie la linea di tiro
	const FRTCellId CheVede(1, -1); // vede il nemico, e costa un passo in piu'

	// Gittata nemica 0: nessuna minaccia, cosi' restano in campo solo avvicinamento e ingaggio.
	FRTHexBotContext Ctx = MakeCtx(FRTCellId(0, 0), Enemy, /*EnemyRange*/ 0, /*EnemyHealth*/ 100);
	Ctx.KiteStandoff = 0;
	Ctx.WElevation = 0;

	// --- la premessa, asserita ---------------------------------------------------------------------
	TestFalse(TEXT("la cella vicina e' cieca"), URTHexVisionLibrary::HasLineOfSight(M, Cieca, Enemy));
	TestTrue(TEXT("l'altra vede il nemico"), URTHexVisionLibrary::HasLineOfSight(M, CheVede, Enemy));

	FRTHexBotContext Senza = Ctx;
	Senza.WEngage = 0;
	const int32 PuraCieca = URTHexBotLibrary::ScorePlan(M, MakePlan(Cieca), Senza);
	const int32 PuraCheVede = URTHexBotLibrary::ScorePlan(M, MakePlan(CheVede), Senza);
	TestTrue(TEXT("senza il termine vincerebbe la cella cieca"), PuraCieca > PuraCheVede);
	TestTrue(TEXT("e il divario e' colmabile dal bonus fresco"), PuraCieca - PuraCheVede < Ctx.WEngage);

	// --- l'esito -----------------------------------------------------------------------------------
	TArray<FRTHexBotPlan> Candidate;
	Candidate.Add(MakePlan(Cieca));
	Candidate.Add(MakePlan(CheVede));

	Ctx.IdleTurns = 0;
	TestTrue(TEXT("da fresco il bot paga il passo in piu' per vedere"),
		URTHexBotLibrary::ChooseBestPlan(M, Candidate, Ctx).DestCell == CheVede);

	// Abbastanza turni perche' il bonus sia sceso a zero: `ceil(WEngage / WEngageDecay)`.
	Ctx.IdleTurns = FMath::DivideAndRoundUp(Ctx.WEngage, FMath::Max(1, Ctx.WEngageDecay));
	TestTrue(TEXT("da fermo da abbastanza turni smette di pagarlo, e si avvicina"),
		URTHexBotLibrary::ChooseBestPlan(M, Candidate, Ctx).DestCell == Cieca);

	// ⚠️ E il decadimento non va sotto zero: un'unita' inerte da venti turni non deve PAGARE per vedere.
	Ctx.IdleTurns = 20;
	TestEqual(TEXT("il bonus non diventa mai una penalita'"),
		URTHexBotLibrary::ScorePlan(M, MakePlan(CheVede), Ctx), PuraCheVede);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotElevationTest,
	"RefactorTactics.HexBot.ScoreElevationBonus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotElevationTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeLayeredBotMap(3, 3);   // le celle in quota devono esistere nell'asset
	FRTHexBotContext Ctx;
	Ctx.Origin = FRTCellId(0, 0, 0);
	Ctx.WElevation = 20;

	// Il bonus e' ASSOLUTO sulla quota della destinazione, e cresce col layer.
	const int32 Ground = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(1, 0, 0)), Ctx);
	const int32 High = URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(1, 0, 2)), Ctx);
	TestTrue(TEXT("la quota alta vale di piu'"), High > Ground);
	TestEqual(TEXT("bonus proporzionale al layer"), High - Ground, 40);

	// 🔴 **E la quota si paga anche PARTENDO da li', che e' il cuore di #1088.** Con `Origin` a layer 0 le
	// due forme — assoluta e relativa all'origine — danno lo stesso numero, quindi questo test da solo non
	// distingue niente: e' il motivo per cui la forma relativa e' passata per un fix. Partendo da L2, la
	// differenza si vede.
	FRTHexBotContext FromHigh = Ctx;
	FromHigh.Origin = FRTCellId(0, 0, 2);
	const int32 StayHigh = URTHexBotLibrary::ScorePlan(
		M, MakePlanFrom(FRTCellId(0, 0, 2), FRTCellId(0, 0, 2)), FromHigh);
	TestEqual(TEXT("restare in quota incassa il bonus: e' il termine che forma lo stato assorbente"),
		StayHigh, 40);

	// ⚠️ **Questo test misura punteggi, non comportamento.** Fra due candidate conta la DIFFERENZA, e una
	// costante aggiunta a entrambe non muove l'esito: e' l'errore che il 2026-08-22 ha prodotto un fix
	// inerte (#1088). Cio' che decide e' pinnato da `ElevationNeverOutweighsClosingOneCell`, che confronta
	// l'esito di `ChooseBestPlan`.
	return true;
}

/**
 * L'INVARIANTE, e si misura sull'ESITO di `ChooseBestPlan` (#1088).
 *
 * Il bonus di quota compete con l'avvicinamento: se scendere di `MaxLayer` per guadagnare UNA cella non
 * paga, il bot resta in alto e si parcheggia. Non c'e' una forma che lo renda impossibile — un bonus di
 * posizione abbastanza grande batte sempre l'avvicinamento — quindi la difesa e' questo vincolo numerico,
 * e va misurato dove il comportamento si decide.
 *
 * ⚠️ **Su `ChooseBestPlan` e non su `ScorePlan`**: due punteggi confrontati a mano ignorano il tie-break
 * («a parita', mossa minima da `Origin`»), che e' proprio il meccanismo che fa vincere il restare quando
 * i punteggi pareggiano. Il caso `-40` contro `-40` misurato il 2026-08-22 sarebbe passato inosservato.
 *
 * 🔴 **`MaxLayer` e' un limite DICHIARATO, non garantito**: `FRTCellId::Layer` e' un `int32` senza cap, e
 * nessun validator di mappa ne impone uno. Una mappa d'autore a quattro livelli renderebbe `4 x 3 = 12`
 * maggiore di `WApproach`, e questo test resterebbe verde perche' misura il caso che dichiara. Chi aggiunge
 * layer oltre `MaxLayerSupported` deve rivedere i pesi o aggiungere il cap al formato mappa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotElevationInvariantTest,
	"RefactorTactics.HexBot.ElevationNeverOutweighsClosingOneCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotElevationInvariantTest::RunTest(const FString&)
{
	// Le arene generate usano due layer (`MaxLayer` 1); tre e' il caso peggiore che ponti e multilivello
	// di E9 rendono plausibile senza che nessuno lo dichiari.
	const int32 MaxLayerSupported = 2;
	URTHexMapAsset* M = MakeLayeredBotMap(4, MaxLayerSupported + 1);

	FRTHexBotContext Ctx = MakeCtx(FRTCellId(0, 0, MaxLayerSupported), FRTCellId(4, 0, 0), /*range*/ 0, /*hp*/ 1000);
	Ctx.WElevation = GetDefault<ARTTurnManager>()->WElevation;
	Ctx.WApproach = GetDefault<ARTTurnManager>()->WApproach;

	// Due sole candidate, e la domanda e' quale vince: restare in quota, o scendere avvicinandosi di una
	// cella. Passano da `ChooseBestPlan`, quindi il tie-break e' incluso nel verdetto.
	const FRTCellId StayCell(0, 0, MaxLayerSupported);
	const FRTCellId CloserCell(1, 0, 0);
	TArray<FRTHexBotPlan> Candidates;
	Candidates.Add(MakePlanFrom(StayCell, StayCell));
	Candidates.Add(MakePlanFrom(StayCell, CloserCell));
	const FRTHexBotPlan Best = URTHexBotLibrary::ChooseBestPlan(M, Candidates, Ctx);

	TestTrue(FString::Printf(
		TEXT("scendere di %d layer per una cella batte restare (WElevation %d, WApproach %d): scelto (%d,%d,L%d)"),
		MaxLayerSupported, Ctx.WElevation, Ctx.WApproach, Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer),
		Best.DestCell == CloserCell);

	// 🔴 **E vale per il KITER, che e' il ramo in cui l'invariante era vuoto.** Sopra la distanza di
	// sicurezza non si applicava alcun termine di distanza, quindi l'elevazione era l'unico termine
	// posizionale e restare in quota vinceva con qualunque peso — `WApproach` non era in gioco, quindi
	// `WElevation * MaxLayer < WApproach` non diceva niente. `MakeCtx` lascia `KiteStandoff` a 0, percio'
	// il caso va costruito: senza questa meta' il test pinna solo la mischia mentre header e spec
	// dichiarano l'invariante senza condizioni.
	FRTHexBotContext KiterCtx = Ctx;
	KiterCtx.KiteStandoff = 3;   // Phase: `PressureJet` portata 5 -> `DeriveKiteStandoff` 3
	TArray<FRTHexBotPlan> KiterCandidates;
	KiterCandidates.Add(MakePlanFrom(StayCell, StayCell));
	KiterCandidates.Add(MakePlanFrom(StayCell, CloserCell));
	const FRTHexBotPlan KiterBest = URTHexBotLibrary::ChooseBestPlan(M, KiterCandidates, KiterCtx);
	TestTrue(FString::Printf(
		TEXT("anche il kiter scende invece di parcheggiarsi in quota: scelto (%d,%d,L%d)"),
		KiterBest.DestCell.X, KiterBest.DestCell.Y, KiterBest.DestCell.Layer),
		KiterBest.DestCell == CloserCell);

	// E le due sorgenti dei pesi devono coincidere: `PlanBots` copia le UPROPERTY di `ARTTurnManager` sopra
	// i default della struct, quindi tarare solo i secondi non muove nulla di cio' che il giocatore vede.
	//
	// ⚠️ **Il CDO non e' l'ultima parola.** `ARTGameMode` riusa un `ARTTurnManager` gia' presente nel livello
	// invece di spawnarlo, quindi un'istanza piazzata in un `.umap` con `WElevation` modificato in editor
	// sopravvive al cambio di default C++ e questo test non la vedrebbe. ⏳ Presidiato da **#1276**, che
	// apre la voce PIE: verificarlo richiede l'editor, perche' i `.umap` sono pacchetti compressi e un grep
	// non prova nulla in nessuna delle due direzioni.
	// ✅ **La deriva fra le due sorgenti non e' piu' rilevabile: e' impossibile.** `ARTTurnManager` derivava
	// i sei pesi da altrettanti letterali scritti a mano, e questa coppia di asserzioni li confrontava DOPO
	// il fatto — su due dei sei. Ora ogni default e' `FRTHexBotContext{}.W*`, quindi c'e' una sorgente sola
	// e le righe qui sotto verificano il legame, non una coincidenza fortunata.
	const FRTHexBotContext Defaults;
	const ARTTurnManager* CDO = GetDefault<ARTTurnManager>();
	TestEqual(TEXT("WKill deriva dalla struct"), CDO->WKill, Defaults.WKill);
	TestEqual(TEXT("WDamage deriva dalla struct"), CDO->WDamage, Defaults.WDamage);
	TestEqual(TEXT("WThreat deriva dalla struct"), CDO->WThreat, Defaults.WThreat);
	TestEqual(TEXT("WKiteViolation deriva dalla struct"), CDO->WKiteViolation, Defaults.WKiteViolation);
	TestEqual(TEXT("WApproach deriva dalla struct"), CDO->WApproach, Defaults.WApproach);
	TestEqual(TEXT("WElevation deriva dalla struct"), CDO->WElevation, Defaults.WElevation);
	// I due dell'obiettivo (`#2269`) entrano nella stessa verifica: la sorgente e' una sola anche per loro,
	// e tararli sulla struct senza toccare l'attore non muoverebbe nulla di cio' che il giocatore vede.
	TestEqual(TEXT("WObjective deriva dalla struct"), CDO->WObjective, Defaults.WObjective);
	TestEqual(TEXT("WObjectiveFalloff deriva dalla struct"), CDO->WObjectiveFalloff, Defaults.WObjectiveFalloff);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// OBIETTIVO (#2269) — la condizione di vittoria del formato spedito entra nel punteggio.
//
// Fino a qui `ScorePlan` aveva termini per danno, uccisione, minaccia, ingaggio, kite, avvicinamento, quota e
// fuoco amico, e **nessuno** per l'obiettivo: misurato il 2026-09-04, una partita 2v2 bot contro bot su
// `L_HexArena` si e' decisa `obiettivo 0-3` senza che nessuno dei due bot lo stesse giocando.
// ---------------------------------------------------------------------------------------------------------

/**
 * **La forma del termine**: bonus pieno sulla cella, decadimento per PASSO, mai una penalita'.
 *
 * ⚠️ **I passi sono quelli sul grafo, non la distanza in linea d'aria**, ed e' la meta' che un muro rivela.
 * E' la stessa correzione di `#1296`: se qualcuno sostituisse `ApproachSteps` con `HexDistance` il punteggio
 * resterebbe plausibile ovunque tranne dietro un ostacolo, cioe' proprio dove il bot deve decidere se vale la
 * pena aggirarlo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotObjectiveTermTest,
	"RefactorTactics.HexBot.ScoreObjectiveTerm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotObjectiveTermTest::RunTest(const FString&)
{
	const FRTCellId Objective(-6, 0);
	URTHexMapAsset* M = MakeObjectiveBotMap(6, Objective);

	FRTHexBotContext Ctx;
	Ctx.Origin = FRTCellId(0, 0);
	Ctx.ObjectiveCells.Add(Objective);

	TestEqual(TEXT("controllare la cella vale WObjective per intero"),
		URTHexBotLibrary::ScoreObjectiveTerm(M, Objective, Ctx), Ctx.WObjective);
	TestEqual(TEXT("un passo di distanza costa esattamente WObjectiveFalloff"),
		URTHexBotLibrary::ScoreObjectiveTerm(M, FRTCellId(-5, 0), Ctx),
		Ctx.WObjective - Ctx.WObjectiveFalloff);

	// Il RAGGIO d'attrazione e' dichiarato, non infinito: oltre `WObjective / WObjectiveFalloff` passi il
	// termine e' spento, e la cella si giudica come su una mappa senza obiettivi.
	const int32 Raggio = FMath::DivideAndRoundUp(Ctx.WObjective, Ctx.WObjectiveFalloff);
	TestEqual(TEXT("al raggio dichiarato il termine e' spento"),
		URTHexBotLibrary::ScoreObjectiveTerm(M, FRTCellId(-6 + Raggio, 0), Ctx), 0);

	// 🔴 **E oltre il raggio NON diventa una penalita'.** Senza il floor, un'unita' lontana pagherebbe per
	// non stare sull'obiettivo, e quella penalita' entrerebbe in OGNI confronto — comprese le scelte di
	// combattimento dall'altra parte della mappa, dove l'obiettivo non c'entra niente.
	TestEqual(TEXT("il termine non diventa mai negativo"),
		URTHexBotLibrary::ScoreObjectiveTerm(M, FRTCellId(6, 0), Ctx), 0);

	// Le due spegnature, e sono cose diverse: una mappa senza obiettivi, e un peso azzerato.
	FRTHexBotContext SenzaCelle = Ctx;
	SenzaCelle.ObjectiveCells.Reset();
	TestEqual(TEXT("su una mappa senza obiettivi il termine e' zero"),
		URTHexBotLibrary::ScoreObjectiveTerm(M, Objective, SenzaCelle), 0);

	FRTHexBotContext SenzaPeso = Ctx;
	SenzaPeso.WObjective = 0;
	TestEqual(TEXT("a peso zero il termine e' zero"),
		URTHexBotLibrary::ScoreObjectiveTerm(M, Objective, SenzaPeso), 0);

	// --- I PASSI, non la linea d'aria ------------------------------------------------------------------
	// Muro fra `(0,0)` e l'obiettivo a `(2,0)`: in linea d'aria sono due celle, sul grafo sono tre.
	const FRTCellId Vicino(2, 0);
	URTHexMapAsset* Murata = MakeObjectiveBotMap(3, Vicino);
	BlockBotMove(Murata, FRTCellId(1, 0));

	FRTHexBotContext CtxMuro;
	CtxMuro.Origin = FRTCellId(0, 0);
	CtxMuro.ObjectiveCells.Add(Vicino);

	const int32 InLineaDAria = CtxMuro.WObjective - CtxMuro.WObjectiveFalloff * 2;
	const int32 Misurato = URTHexBotLibrary::ScoreObjectiveTerm(Murata, FRTCellId(0, 0), CtxMuro);
	TestEqual(TEXT("il muro allunga il cammino a tre passi"),
		Misurato, CtxMuro.WObjective - CtxMuro.WObjectiveFalloff * 3);
	TestTrue(TEXT("e vale MENO di quanto direbbe la distanza in linea d'aria"), Misurato < InLineaDAria);
	return true;
}

/**
 * **Il DoD: obiettivo raggiungibile, nessuno che possa colpire — il bot ci va.**
 *
 * Si misura sull'ESITO di `ChooseBestPlan`, non sul punteggio di una candidata isolata: fra due candidate
 * conta la DIFFERENZA, e il tie-break «a parita' vince la mossa minima da `Origin`» e' parte del verdetto.
 *
 * 🔴 **La premessa e' ASSERITA, e senza di essa il test passerebbe per la ragione sbagliata.** L'obiettivo
 * sta dalla parte OPPOSTA al nemico proprio perche' un obiettivo sulla strada del nemico verrebbe raggiunto
 * anche da un bot che non sa che esista — e il verde direbbe soltanto che il bot si avvicina, cosa che
 * faceva gia'. Le due righe qui sotto misurano che senza il termine vince la cella d'avvicinamento, e che il
 * divario e' colmabile dal termine invece di essere travolto da un peso qualunque.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotSeeksObjectiveTest,
	"RefactorTactics.HexBot.SeeksUncontestedObjective",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotSeeksObjectiveTest::RunTest(const FString&)
{
	const FRTCellId Objective(-1, 0);  // un passo, nella direzione OPPOSTA al nemico
	const FRTCellId Start(0, 0);
	const FRTCellId Closer(1, 0);      // un passo VERSO il nemico
	const FRTCellId Enemy(4, 0);
	URTHexMapAsset* M = MakeObjectiveBotMap(4, Objective);

	// Gittata nemica 0: «nessun nemico in grado di colpire», che e' la condizione del DoD. Cosi' restano in
	// campo avvicinamento, ingaggio e obiettivo, e la minaccia non decide al posto loro.
	FRTHexBotContext Ctx = MakeCtx(Start, Enemy, /*EnemyRange*/ 0, /*EnemyHealth*/ 100);
	Ctx.ObjectiveCells.Add(Objective);
	Ctx.WElevation = 0;   // arena piatta: il termine vale gia' zero, azzerarlo lo DICHIARA

	TArray<FRTHexBotPlan> Candidate;
	Candidate.Add(MakePlanFrom(Start, Start));
	Candidate.Add(MakePlanFrom(Start, Closer));
	Candidate.Add(MakePlanFrom(Start, Objective));

	// --- la premessa, asserita -------------------------------------------------------------------------
	FRTHexBotContext Senza = Ctx;
	Senza.WObjective = 0;
	const int32 PuraObiettivo = URTHexBotLibrary::ScorePlan(M, MakePlanFrom(Start, Objective), Senza);
	const int32 PuraVicino = URTHexBotLibrary::ScorePlan(M, MakePlanFrom(Start, Closer), Senza);
	TestTrue(TEXT("senza il termine vincerebbe la cella che chiude la distanza"), PuraVicino > PuraObiettivo);
	TestTrue(TEXT("e il divario e' colmabile dal termine"), PuraVicino - PuraObiettivo < Ctx.WObjective);
	TestTrue(TEXT("senza il termine il bot NON va sull'obiettivo"),
		URTHexBotLibrary::ChooseBestPlan(M, Candidate, Senza).DestCell == Closer);

	// --- l'esito ---------------------------------------------------------------------------------------
	const FRTHexBotPlan Best = URTHexBotLibrary::ChooseBestPlan(M, Candidate, Ctx);
	TestTrue(FString::Printf(TEXT("il bot va sull'obiettivo: scelto (%d,%d,L%d)"),
		Best.DestCell.X, Best.DestCell.Y, Best.DestCell.Layer),
		Best.DestCell == Objective);
	return true;
}

/**
 * **Il caso simmetrico: con un colpo letale disponibile, l'obiettivo NON vince.**
 *
 * 🔴 **Senza questo, `SeeksUncontestedObjective` passerebbe anche con un peso infinito** — ed e' lo stesso
 * difetto che `HexBot.ElevationNeverOutweighsClosingOneCell` esiste per impedire sull'elevazione: un bonus
 * di posizione abbastanza grande batte qualunque cosa, e un test che misura solo «ci va» non se ne accorge.
 *
 * ⚠️ **Il margine si misura, non si spera.** L'ultima riga non chiede che il colpo vinca — quello lo dice
 * gia' `ChooseBestPlan` — ma che vinca di **piu' di `WObjective`**: cioe' che nemmeno regalando all'obiettivo
 * il suo bonus massimo l'ordine si rovescerebbe. Un margine di un punto passerebbe il primo controllo e
 * cadrebbe alla prima taratura.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotObjectiveVsKillTest,
	"RefactorTactics.HexBot.ObjectiveNeverOutweighsAKill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotObjectiveVsKillTest::RunTest(const FString&)
{
	const FRTCellId Objective(-1, 0);
	const FRTCellId Start(0, 0);
	const FRTCellId Enemy(2, 0);
	URTHexMapAsset* M = MakeObjectiveBotMap(4, Objective);

	// Il nemico ha 30 HP e il bot infligge 30: il colpo e' LETALE, che e' la meta' che deve dominare.
	FRTHexBotContext Ctx = MakeCtx(Start, Enemy, /*EnemyRange*/ 0, /*EnemyHealth*/ 30);
	Ctx.ObjectiveCells.Add(Objective);
	Ctx.WElevation = 0;

	const FRTHexBotPlan Colpo = MakePlan(Start, /*bAttack*/ true, /*Damage*/ 30, /*TargetHealth*/ 30);
	const FRTHexBotPlan Corsa = MakePlanFrom(Start, Objective);

	TArray<FRTHexBotPlan> Candidate;
	Candidate.Add(Colpo);
	Candidate.Add(Corsa);

	const FRTHexBotPlan Best = URTHexBotLibrary::ChooseBestPlan(M, Candidate, Ctx);
	TestTrue(TEXT("con un colpo letale disponibile il bot spara invece di correre sull'obiettivo"),
		Best.bHasAttack);

	const int32 PunteggioColpo = URTHexBotLibrary::ScorePlan(M, Colpo, Ctx);
	const int32 PunteggioCorsa = URTHexBotLibrary::ScorePlan(M, Corsa, Ctx);
	TestTrue(FString::Printf(TEXT("e il margine (%d) non e' colmabile dal termine al suo massimo (%d)"),
		PunteggioColpo - PunteggioCorsa, Ctx.WObjective),
		PunteggioColpo - PunteggioCorsa > Ctx.WObjective);

	// L'invariante DICHIARATA, sulla sorgente che vince in partita. ⚠️ Non e' un gate: nessun valore sensato
	// la viola, e un'asserzione che non puo' fallire non prova niente da sola. Sta qui accanto alla misura
	// dell'esito perche' e' quella misura a darle un significato.
	const ARTTurnManager* CDO = GetDefault<ARTTurnManager>();
	TestTrue(TEXT("WObjective < WKill"), CDO->WObjective < CDO->WKill);
	return true;
}

/**
 * L'INVARIANTE che PUO' fallire: **`WObjectiveFalloff > WApproach`**.
 *
 * 🔴 **Sotto quella soglia il termine e' decorativo proprio nel caso per cui esiste.** Il termine tira verso
 * l'obiettivo mentre `WApproach` tira verso il nemico: se i due gradienti si pareggiano, un passo che
 * avvicina l'obiettivo e allontana il nemico vale esattamente **zero**, i punteggi si appiattiscono, e il
 * tie-break «a parita' vince la mossa minima da `Origin`» fa restare fermo il bot.
 *
 * ⚠️ **E' l'analogo di `WElevation * MaxLayer < WApproach`**, con il segno rovesciato: li' un bonus di
 * posizione troppo GRANDE batte l'avvicinamento e produce il parcheggio; qui un gradiente troppo PICCOLO si
 * fa battere e produce l'indifferenza. In entrambi i casi la difesa e' un numero, e si misura dove il
 * comportamento si decide.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotObjectivePullInvariantTest,
	"RefactorTactics.HexBot.ObjectivePullBeatsClosingOneCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotObjectivePullInvariantTest::RunTest(const FString&)
{
	const FRTCellId Objective(-1, 0);
	const FRTCellId Start(0, 0);
	const FRTCellId Closer(1, 0);
	const FRTCellId Enemy(4, 0);
	URTHexMapAsset* M = MakeObjectiveBotMap(4, Objective);

	FRTHexBotContext Ctx = MakeCtx(Start, Enemy, /*EnemyRange*/ 0, /*EnemyHealth*/ 100);
	Ctx.ObjectiveCells.Add(Objective);
	Ctx.WElevation = 0;
	// I pesi che vincono in partita, non quelli della struct: `PlanBots` copia queste UPROPERTY nel contesto.
	const ARTTurnManager* CDO = GetDefault<ARTTurnManager>();
	Ctx.WApproach = CDO->WApproach;
	Ctx.WObjective = CDO->WObjective;
	Ctx.WObjectiveFalloff = CDO->WObjectiveFalloff;

	TArray<FRTHexBotPlan> Candidate;
	Candidate.Add(MakePlanFrom(Start, Start));
	Candidate.Add(MakePlanFrom(Start, Closer));
	Candidate.Add(MakePlanFrom(Start, Objective));

	TestTrue(TEXT("con i pesi spediti il bot raggiunge l'obiettivo"),
		URTHexBotLibrary::ChooseBestPlan(M, Candidate, Ctx).DestCell == Objective);

	// 🔴 **E il vincolo MORDE**: a gradienti pari le tre candidate pareggiano e il tie-break chiude. Senza
	// questa meta' l'invariante sarebbe una frase nell'header, e nessuno saprebbe che abbassare il falloff
	// in editor spegne il termine senza spegnere il peso.
	FRTHexBotContext Pari = Ctx;
	Pari.WObjectiveFalloff = Pari.WApproach;
	const FRTHexBotPlan Fermo = URTHexBotLibrary::ChooseBestPlan(M, Candidate, Pari);
	TestFalse(FString::Printf(
		TEXT("a WObjectiveFalloff == WApproach il bot NON ci va: scelto (%d,%d,L%d)"),
		Fermo.DestCell.X, Fermo.DestCell.Y, Fermo.DestCell.Layer),
		Fermo.DestCell == Objective);

	TestTrue(TEXT("e la sorgente spedita rispetta il vincolo"), CDO->WObjectiveFalloff > CDO->WApproach);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Forma dell'attacco e collaterale (#213)
//
// ScorePlan valutava ogni attacco come se colpisse una cella sola: non contava i nemici in piu' presi da
// un'area, ne' gli alleati che ci finiscono dentro. Con il fuoco amico attivo di default (2026-08-08) la
// seconda meta' dell'omissione e' diventata dannosa, non solo conservativa.
//
// Il modello segue il resolver, non un'idea del resolver: `CollectHexAttacks` esclude SEMPRE l'attaccante
// (`u == Intent.AttackerId`) e colpisce un alleato solo se l'azione dichiara `bFriendlyFire`.
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotAreaExtraEnemiesTest,
	"RefactorTactics.HexBot.ScoreAreaCountsExtraEnemies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotAreaExtraEnemiesTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Target(2, 0);

	// Un solo nemico nell'area.
	const FRTHexBotContext One = MakeAreaCtx(Origin, Target, /*damage*/ 30);

	// Un secondo nemico ADIACENTE al bersaglio: entra nell'esagono di raggio 1, ma non cambia MinDist
	// (2 contro 3) ne' la minaccia (gittata 0), quindi l'unica differenza di punteggio e' il collaterale.
	FRTHexBotContext Two = One;
	Two.Enemies.Add(FRTCellId(3, 0));
	Two.EnemyRanges.Add(0);
	Two.EnemyHealth.Add(100);

	const int32 ScoreOne = URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, 30, 100), One);
	const int32 ScoreTwo = URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, 30, 100), Two);

	TestTrue(TEXT("l'area che prende due nemici vale piu' di quella che ne prende uno"), ScoreTwo > ScoreOne);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotAreaPenalizesAllyTest,
	"RefactorTactics.HexBot.ScoreAreaPenalizesAlly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotAreaPenalizesAllyTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Target(2, 0);

	const FRTHexBotContext Clean = MakeAreaCtx(Origin, Target, /*damage*/ 30);

	FRTHexBotContext WithAlly = Clean;
	WithAlly.Allies.Add(FRTCellId(3, 0)); // adiacente al bersaglio: dentro l'area
	WithAlly.AllyHealth.Add(100);

	const int32 ScoreClean = URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, 30, 100), Clean);
	const int32 ScoreAlly = URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, 30, 100), WithAlly);

	TestTrue(TEXT("prendere il compagno nell'area abbassa il punteggio"), ScoreAlly < ScoreClean);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotAllyPenaltyScalesTest,
	"RefactorTactics.HexBot.ScoreAllyPenaltyScalesWithDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotAllyPenaltyScalesTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Target(2, 0);
	const FRTCellId Ally(3, 0);

	auto PenaltyForDamage = [&](int32 Damage)
	{
		const FRTHexBotContext Clean = MakeAreaCtx(Origin, Target, Damage);
		FRTHexBotContext WithAlly = Clean;
		WithAlly.Allies.Add(Ally);
		WithAlly.AllyHealth.Add(100);

		return URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, Damage, 100), Clean)
			- URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, Damage, 100), WithAlly);
	};

	// La decisione di design e' PENALITA' PROPORZIONALE, non veto: ferire il compagno per prendere due
	// nemici resta una scelta legittima, ma costa in proporzione a quanto lo si ferisce.
	TestTrue(TEXT("la penalita' e' proporzionale al danno, non una costante"),
		PenaltyForDamage(40) > PenaltyForDamage(20));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotNoFriendlyFireTest,
	"RefactorTactics.HexBot.ScoreIgnoresAllyWithoutFriendlyFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotNoFriendlyFireTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Target(2, 0);

	const FRTHexBotContext Clean = MakeAreaCtx(Origin, Target, /*damage*/ 30);

	FRTHexBotContext NoFire = Clean;
	NoFire.bAttackFriendlyFire = false;
	NoFire.Allies.Add(FRTCellId(3, 0));
	NoFire.AllyHealth.Add(100);

	FRTHexBotPlan NoFirePlan = MakeAreaPlan(Origin, 30, 100);
	NoFirePlan.bFriendlyFire = false; // l'azione non colpisce gli alleati: il piano lo porta con se'

	const int32 ScoreClean = URTHexBotLibrary::ScorePlan(M, MakeAreaPlan(Origin, 30, 100), Clean);
	const int32 ScoreNoFire = URTHexBotLibrary::ScorePlan(M, NoFirePlan, NoFire);

	// Il resolver non tocca l'alleato: penalizzarlo qui renderebbe il bot timido su un pericolo che non esiste.
	TestEqual(TEXT("senza fuoco amico l'alleato nell'area non entra nel punteggio"), ScoreNoFire, ScoreClean);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotSingleShapeTest,
	"RefactorTactics.HexBot.ScoreSingleShapeIgnoresNeighbours",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotSingleShapeTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Target(2, 0);

	// Forma Single: il vicino del bersaglio non viene colpito, ne' se nemico ne' se alleato.
	FRTHexBotContext One = MakeCtx(Origin, Target, /*range*/ 0, /*hp*/ 100);
	One.AttackDamage = 30;

	FRTHexBotContext Two = One;
	Two.Enemies.Add(FRTCellId(3, 0));
	Two.EnemyRanges.Add(0);
	Two.EnemyHealth.Add(100);

	const int32 ScoreOne = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin, true, 30, 100), One);
	const int32 ScoreTwo = URTHexBotLibrary::ScorePlan(M, MakePlan(Origin, true, 30, 100), Two);

	TestEqual(TEXT("Single non guadagna nulla dai vicini del bersaglio"), ScoreTwo, ScoreOne);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotCandidateShapeTest,
	"RefactorTactics.HexBot.CandidatesCarryShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotCandidateShapeTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 1));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	FRTHexBotContext Ctx = MakeAreaCtx(FRTCellId(0, 0), FRTCellId(1, 0), /*damage*/ 18);
	Ctx.AttackRange = 3;

	const TArray<FRTHexBotPlan> Candidates = URTHexBotLibrary::BuildCandidates(Snap, 1, Ctx);

	// Il primo anello del cablaggio: la forma passa dal contesto AL PIANO. Se restasse nel contesto,
	// ChooseBestPlan — che confronta in una lista sola le candidate di abilita' diverse — la applicherebbe
	// a tutte.
	int32 Attacks = 0;
	for (const FRTHexBotPlan& P : Candidates)
	{
		if (!P.bHasAttack)
		{
			continue;
		}
		++Attacks;
		TestEqual(TEXT("la candidata porta la forma dell'azione"), P.Shape, ERTAbilityShape::Area);
		TestEqual(TEXT("la candidata porta il raggio dell'area"), P.AreaRadius, 1);
		TestEqual(TEXT("la candidata porta la gittata"), P.RangeCells, 3);
		TestTrue(TEXT("la candidata porta il fuoco amico"), P.bFriendlyFire);
	}
	TestTrue(TEXT("almeno una candidata con attacco, o il ciclo non verifica nulla"), Attacks > 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Scelta del piano
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotOrderIndependenceTest,
	"RefactorTactics.HexBot.ChooseBestPlanOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotOrderIndependenceTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	FRTHexBotContext Ctx = MakeCtx(Origin, FRTCellId(3, 0), /*range*/ 2, /*hp*/ 100);

	TArray<FRTHexBotPlan> A;
	A.Add(MakePlan(FRTCellId(1, 0)));
	A.Add(MakePlan(FRTCellId(0, 1), true, 30, 25)); // letale
	A.Add(MakePlan(FRTCellId(1, -1)));

	TArray<FRTHexBotPlan> B;
	B.Add(A[2]); B.Add(A[0]); B.Add(A[1]);

	const FRTHexBotPlan BestA = URTHexBotLibrary::ChooseBestPlan(M, A, Ctx);
	const FRTHexBotPlan BestB = URTHexBotLibrary::ChooseBestPlan(M, B, Ctx);

	TestTrue(TEXT("stessa scelta indipendentemente dall'ordine"), BestA.DestCell == BestB.DestCell);
	TestTrue(TEXT("sceglie il colpo letale"), BestA.bHasAttack && BestA.DestCell == FRTCellId(0, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotTieBreakTest,
	"RefactorTactics.HexBot.ChooseBestPlanTieBreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotTieBreakTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	FRTHexBotContext Ctx;
	Ctx.Origin = Origin; // nessun nemico: tutte le candidate valgono 0

	TArray<FRTHexBotPlan> Candidates;
	Candidates.Add(MakePlan(FRTCellId(2, 0)));
	Candidates.Add(MakePlan(Origin));
	Candidates.Add(MakePlan(FRTCellId(1, 0)));

	const FRTHexBotPlan Best = URTHexBotLibrary::ChooseBestPlan(M, Candidates, Ctx);
	TestTrue(TEXT("a parita' di punteggio resta fermo"), Best.DestCell == Origin);

	// Senza la candidata "resta", vince la mossa minima; a parita' di distanza, l'ordine stabile.
	TArray<FRTHexBotPlan> Moves;
	Moves.Add(MakePlan(FRTCellId(0, 1)));
	Moves.Add(MakePlan(FRTCellId(1, 0)));
	Moves.Add(MakePlan(FRTCellId(2, 0)));
	const FRTHexBotPlan Nearest = URTHexBotLibrary::ChooseBestPlan(M, Moves, Ctx);
	TestEqual(TEXT("mossa minima"), URTHexLibrary::HexDistance(Nearest.DestCell, Origin), 1);
	TestTrue(TEXT("tie-break stabile fra celle equidistanti"), Nearest.DestCell == FRTCellId(0, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotEmptyPlanTest,
	"RefactorTactics.HexBot.ChooseBestPlanEmptyStays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotEmptyPlanTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(2);
	FRTHexBotContext Ctx;
	Ctx.Origin = FRTCellId(1, -1);

	const FRTHexBotPlan Best = URTHexBotLibrary::ChooseBestPlan(M, TArray<FRTHexBotPlan>(), Ctx);
	TestTrue(TEXT("nessuna candidata -> resta all'origine"), Best.DestCell == Ctx.Origin);
	TestFalse(TEXT("nessun attacco"), Best.bHasAttack);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Pianificazione completa
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotKillingShotTest,
	"RefactorTactics.HexBot.PlanUnitTakesKillingShot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotKillingShotTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);

	// Bot a (0,0) con budget 2 e gittata 2; nemico a (4,0) con 10 HP: da (2,0) e' colpibile e muore.
	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	FRTHexBotContext Ctx = MakeCtx(FRTCellId(0, 0), FRTCellId(4, 0), /*range*/ 1, /*hp*/ 10);
	Ctx.AttackRange = 2;
	Ctx.AttackDamage = 30;

	const TArray<FRTHexBotPlan> Candidates = URTHexBotLibrary::BuildCandidates(Snap, 1, Ctx);
	TestTrue(TEXT("candidate generate"), Candidates.Num() > 0);
	TestTrue(TEXT("esiste una candidata con attacco"),
		Candidates.ContainsByPredicate([](const FRTHexBotPlan& P) { return P.bHasAttack; }));

	const FRTHexBotPlan Best = URTHexBotLibrary::PlanUnit(Snap, 1, Ctx);
	TestTrue(TEXT("pianifica l'attacco"), Best.bHasAttack);
	TestEqual(TEXT("bersaglio corretto"), Best.TargetIndex, 0);
	TestTrue(TEXT("si porta in gittata"), URTHexLibrary::HexDistance(Best.DestCell, FRTCellId(4, 0)) <= 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotBudgetTest,
	"RefactorTactics.HexBot.PlanUnitRespectsBudgetAndOccupancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotBudgetTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 1));
	Units.Add(FRTHexSimUnit(2, FRTCellId(1, 0), /*budget*/ 0)); // alleato fermo davanti
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	FRTHexBotContext Ctx = MakeCtx(FRTCellId(0, 0), FRTCellId(4, 0), /*range*/ 1, /*hp*/ 100);
	Ctx.KiteStandoff = 0; // mischia: vuole avvicinarsi il piu' possibile

	const TArray<FRTHexBotPlan> Candidates = URTHexBotLibrary::BuildCandidates(Snap, 1, Ctx);
	// Senza questa asserzione il ciclo sotto non verificherebbe nulla su una lista vuota.
	TestTrue(TEXT("candidate generate (origine + celle a distanza 1 libere)"), Candidates.Num() >= 6);
	for (const FRTHexBotPlan& P : Candidates)
	{
		TestTrue(TEXT("nessuna candidata oltre il budget"), URTHexLibrary::HexDistance(P.DestCell, FRTCellId(0, 0)) <= 1);
		TestTrue(TEXT("nessuna candidata sulla cella occupata"), !(P.DestCell == FRTCellId(1, 0)));
	}

	const FRTHexBotPlan Best = URTHexBotLibrary::PlanUnit(Snap, 1, Ctx);
	TestTrue(TEXT("destinazione legale"), !(Best.DestCell == FRTCellId(1, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotSeeksCoverTest,
	"RefactorTactics.HexBot.PlanUnitSeeksCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotSeeksCoverTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);
	BlockBotSight(M, FRTCellId(1, 1)); // muro che copre la cella (0,2) dal nemico a (3,-1)

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	FRTHexBotContext Ctx = MakeCtx(FRTCellId(0, 0), FRTCellId(3, -1), /*range*/ 6, /*hp*/ 100);
	Ctx.AttackRange = 0;      // non puo' rispondere: conta solo il posizionamento
	Ctx.AttackDamage = 0;
	Ctx.KiteStandoff = 0;
	Ctx.WApproach = 0;        // isola il contributo della copertura
	Ctx.WElevation = 0;

	// L'origine e' esposta: esiste quindi una scelta migliore da fare (senza questa verifica il test
	// passerebbe anche con un pianificatore che non fa nulla).
	TestTrue(TEXT("l'origine e' sotto tiro"), URTHexBotLibrary::ScorePlan(M, MakePlan(FRTCellId(0, 0)), Ctx) < 0);

	const FRTHexBotPlan Best = URTHexBotLibrary::PlanUnit(Snap, 1, Ctx);
	TestFalse(TEXT("non pianifica attacchi senza gittata"), Best.bHasAttack);
	TestFalse(TEXT("si sposta dall'origine esposta"), Best.DestCell == FRTCellId(0, 0));
	TestEqual(TEXT("la cella scelta e' al riparo"),
		URTHexBotLibrary::ScorePlan(M, MakePlan(Best.DestCell), Ctx), 0);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Fuga (kiting in panico): la guardia che il quadrato risolve con BestKiteCell
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotKiteCellTest,
	"RefactorTactics.HexBot.KiteCellMaximizesDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotKiteCellTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(5);

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 2));
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	const FRTCellId Threat(2, 0);
	const FRTCellId Flee = URTHexBotLibrary::BestKiteCell(Snap, /*UnitId*/ 1, Threat);

	TestTrue(TEXT("si allontana dalla minaccia"),
		URTHexLibrary::HexDistance(Flee, Threat) > URTHexLibrary::HexDistance(FRTCellId(0, 0), Threat));
	TestTrue(TEXT("resta entro il budget"), URTHexLibrary::HexDistance(Flee, FRTCellId(0, 0)) <= 2);
	TestTrue(TEXT("massimizza la distanza raggiungibile"), URTHexLibrary::HexDistance(Flee, Threat) == 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotKiteCellLegalTest,
	"RefactorTactics.HexBot.KiteCellStaysLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotKiteCellLegalTest::RunTest(const FString&)
{
	// Mappa STRETTA: la fuga non deve proporre celle inesistenti, bloccate od occupate.
	URTHexMapAsset* M = MakeBotMap(1); // solo l'origine e i suoi 6 vicini

	TArray<FRTHexSimUnit> Units;
	Units.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 3));
	Units.Add(FRTHexSimUnit(2, FRTCellId(-1, 0), /*budget*/ 0)); // alleato fermo sulla via di fuga
	const FRTHexSnapshot Snap = URTHexSimLibrary::MakeSnapshot(M, Units);

	const FRTCellId Flee = URTHexBotLibrary::BestKiteCell(Snap, /*UnitId*/ 1, FRTCellId(1, 0));

	TestTrue(TEXT("la cella di fuga esiste nella mappa"), M->ContainsCell(Flee));
	TestFalse(TEXT("non fugge dentro un'altra unita'"), Flee == FRTCellId(-1, 0));

	// Unita' immobile: non c'e' fuga possibile, resta dov'e' (nessuna mossa illegale, nessun crash).
	TArray<FRTHexSimUnit> Stuck;
	Stuck.Add(FRTHexSimUnit(1, FRTCellId(0, 0), /*budget*/ 0));
	const FRTHexSnapshot StuckSnap = URTHexSimLibrary::MakeSnapshot(M, Stuck);
	TestTrue(TEXT("senza budget resta dov'e'"),
		URTHexBotLibrary::BestKiteCell(StuckSnap, 1, FRTCellId(1, 0)) == FRTCellId(0, 0));
	return true;
}


// ---------------------------------------------------------------------------------------------------------
// CP 13.5 / ADR-0005 — l'ORIENTAMENTO nel punteggio delle candidate.
//
// I tre test qui sotto isolano il facing tenendo TUTTO il resto identico: due contesti che differiscono per
// il solo orientamento del bersaglio, e lo stesso piano. E' l'unico allestimento in cui una differenza di
// punteggio non possa venire da altro — distanza, minaccia e danno sono gli stessi per costruzione.
// ---------------------------------------------------------------------------------------------------------

namespace
{
	/** Copertura bassa sul bordo indicato: la stessa forma che usano i test di `RTHexCoverTests`. */
	void SetBotLowCover(URTHexMapAsset* Map, const FRTCellId& Id, ERTHexDirection Edge)
	{
		const FRTHexCellData* Existing = Map->FindCell(Id);
		FRTHexCellData Data = Existing ? *Existing : FRTHexCellData(Id);
		Data.Covers.Add(FRTHexCover(Edge, ERTHexCoverType::Low, FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)));
		Map->AddOrUpdateCell(Data);
		Map->SortCells();
	}

	/** Bot in `Origin`, un solo nemico in `Enemy` con l'orientamento dato. Nessuna minaccia: gittata 0. */
	FRTHexBotContext MakeFacingCtx(const FRTCellId& Origin, const FRTCellId& Enemy, ERTHexDirection EnemyFacing)
	{
		FRTHexBotContext Ctx;
		Ctx.Origin = Origin;
		Ctx.SelfFacing = ERTHexDirection::E;
		Ctx.Enemies.Add(Enemy);
		Ctx.EnemyRanges.Add(0);
		Ctx.EnemyHealth.Add(1000); // alto: nessun colpo letale, cosi' `WKill` non entra nel confronto
		Ctx.EnemyFacings.Add(EnemyFacing);
		Ctx.AttackRange = 4;
		Ctx.AttackDamage = 20;
		return Ctx;
	}

	/** Attacco singolo sul nemico 0, sferrato restando fermi in `Origin`. */
	FRTHexBotPlan MakeFacingPlan(const FRTCellId& Origin)
	{
		FRTHexBotPlan Plan;
		Plan.DestCell = Origin;
		Plan.FromCell = Origin; // fermo: il facing non deriva dal movimento
		Plan.bHasAttack = true;
		Plan.TargetIndex = 0;
		Plan.AttackDamage = 20;
		Plan.TargetHealth = 1000;
		Plan.Shape = ERTAbilityShape::Single;
		Plan.RangeCells = 4;
		return Plan;
	}
}

/**
 * Il bot preferisce colpire il lato SCOPERTO: fra due bersagli identici, quello che gli offre il fianco vale
 * di piu', perche' la sua copertura non lo protegge da li' (CP 16.2).
 *
 * Il nemico sta a EST del bot e ha una copertura sul bordo OVEST, cioe' quella rivolta al bot. Se guarda a
 * ovest il colpo e' frontale e la copertura tiene; se guarda a est ha voltato le spalle e la copertura non
 * vale piu' niente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotExposedRearArcTest,
	"RefactorTactics.HexBot.ConsidersExposedRearArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotExposedRearArcTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Enemy(2, 0);
	SetBotLowCover(M, Enemy, ERTHexDirection::W); // il lato da cui il bot spara

	const int32 Facing = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin),
		MakeFacingCtx(Origin, Enemy, ERTHexDirection::W)); // guarda il bot: coperto
	const int32 Exposed = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin),
		MakeFacingCtx(Origin, Enemy, ERTHexDirection::E)); // gli volta le spalle: scoperto

	TestTrue(TEXT("colpire il bersaglio che offre il fianco vale piu' che colpirlo di fronte"), Exposed > Facing);
	return true;
}

/**
 * Il termine direzionale vale ESATTAMENTE il danno che la direzione aggiunge: `WDamage x riduzione scavalcata`.
 *
 * E' il test che impedisce al termine di diventare un peso libero. Un peso nuovo, in questo modello, si tara
 * contro una scala che `#149` ha gia' misurato come rotta — e la lezione di quella issue e' che non esiste un
 * valore che funzioni. Qui non c'e' niente da tarare: il numero viene dal catalogo di combattimento, e se
 * qualcuno lo sostituisse con una costante questo test cadrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotRearBonusValueTest,
	"RefactorTactics.HexBot.RearBonusMatchesBypassedReduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotRearBonusValueTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4);
	const FRTCellId Origin(0, 0);
	const FRTCellId Enemy(2, 0);
	SetBotLowCover(M, Enemy, ERTHexDirection::W);

	const FRTHexBotContext Covered = MakeFacingCtx(Origin, Enemy, ERTHexDirection::W);
	const int32 Facing = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin), Covered);
	const int32 Exposed = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin),
		MakeFacingCtx(Origin, Enemy, ERTHexDirection::E));

	TestEqual(TEXT("il bonus e' WDamage x la riduzione annullata, non un numero suo"),
		Exposed - Facing, Covered.WDamage * URTCombatLibrary::LowCoverDamageReduction);
	return true;
}

/**
 * Senza una protezione da scavalcare, l'orientamento non muove il punteggio di un punto.
 *
 * ⚠️ **Asserisce uno ZERO che oggi e' la norma**, e per questo va letto insieme al suo motivo:
 * `RearHitBypassedCover` ANNULLA una riduzione, e dove non c'e' riduzione non c'e' niente da annullare. Il
 * giorno in cui il bot guadagnasse una protezione propria da mettere in gioco, questo test diventerebbe rosso
 * e chiederebbe di essere promosso — che e' esattamente il suo mestiere. Un residuo asserito non sparisce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHexBotExposureZeroTest,
	"RefactorTactics.HexBot.ExposureIsZeroWithoutProtection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHexBotExposureZeroTest::RunTest(const FString&)
{
	URTHexMapAsset* M = MakeBotMap(4); // arena liscia: nessuna copertura da nessuna parte
	const FRTCellId Origin(0, 0);
	const FRTCellId Enemy(2, 0);

	const int32 Facing = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin),
		MakeFacingCtx(Origin, Enemy, ERTHexDirection::W));
	const int32 Exposed = URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin),
		MakeFacingCtx(Origin, Enemy, ERTHexDirection::E));

	TestEqual(TEXT("senza copertura l'orientamento non cambia il punteggio"), Exposed, Facing);

	// E il verso difensivo, dallo stesso allestimento: un nemico che minaccia davvero la cella, ma nessuna
	// copertura da perdere. La penalita' resta quella di sempre, `WThreat`, senza aggiunte direzionali.
	FRTHexBotContext Threat = MakeFacingCtx(Origin, Enemy, ERTHexDirection::W);
	Threat.EnemyRanges[0] = 4; // ora ci arriva
	FRTHexBotContext ThreatExposed = Threat;
	ThreatExposed.EnemyFacings[0] = ERTHexDirection::E;

	TestEqual(TEXT("nemmeno sul lato difensivo, finche' non c'e' copertura da perdere"),
		URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin), ThreatExposed),
		URTHexBotLibrary::ScorePlan(M, MakeFacingPlan(Origin), Threat));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
