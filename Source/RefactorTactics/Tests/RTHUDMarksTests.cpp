// Marcatori sopra la testa: chi verrebbe colpito dai piani delle PROPRIE unita'.
//
// Il difetto che questi test chiudono e' stato trovato in PIE: l'avviso di fuoco amico spariva appena si
// selezionava un'altra unita' — per esempio per muoverla, cioe' proprio mentre si finisce il turno. Leggeva
// l'anteprima, che appartiene all'unita' SELEZIONATA, invece dei piani.
//
// `ComputePlannedHitMarks` non ha accesso alla selezione: l'indipendenza e' una proprieta' della firma, non
// una disciplina da ricordare. Questi test verificano il resto — che legga i piani giusti e ignori quelli
// che non deve leggere.

#include "Misc/AutomationTest.h"
#include "UI/RTHudViewModel.h" // FRTPlayerEventLineView: le righe da cui i marcatori si derivano (#2697)
#include "UI/RTHUD.h"
#include "Turn/RTTurnManager.h"     // il ciclo del turno: e' cio' che D-359 usa come confine
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nomi distinti da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakeMarksWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyMarksWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}

	ARTUnit* SpawnMarksUnit(UWorld* World, FName HeroId, int32 TeamId, const FRTCellId& Cell)
	{
		const URTHeroData* Hero = nullptr;
		for (const URTHeroData* H : URTHeroCatalogLibrary::GetHeroRoster())
		{
			if (H && H->HeroId == HeroId) { Hero = H; break; }
		}
		if (!World || !Hero) { return nullptr; }

		ARTUnit* Unit = World->SpawnActor<ARTUnit>();
		if (!Unit) { return nullptr; }
		Unit->ConfigureFromHeroData(Hero);
		Unit->TeamId = TeamId;
		Unit->Cell = Cell;
		return Unit;
	}

	/**
	 * Un turno intero: commit e risoluzione, come `RTCombatLogFixture::RunTurn` fa nel proprio file.
	 * Il tick e' quello della risoluzione, non un'attesa: `IsResolving()` e' la condizione, il 400 e' il
	 * tetto che impedisce a un difetto di diventare un test appeso.
	 */
	void PlayOneMarksTurn(ARTTurnManager* TM)
	{
		if (!TM) { return; }
		TM->LockInAndResolve();
		for (int32 I = 0; I < 400 && TM->IsResolving(); ++I)
		{
			TM->Tick(0.05f);
		}
	}

	/** Indice dell'abilita' con quell'ActionId nel kit dell'unita', o INDEX_NONE. */
	int32 MarksAbilityIndex(const ARTUnit* Unit, const TCHAR* ActionId)
	{
		for (int32 I = 0; Unit && I < Unit->NumAbilities(); ++I)
		{
			const URTActionData* A = Unit->GetAbility(I);
			if (A && A->Def.ActionId == FName(ActionId)) { return I; }
		}
		return INDEX_NONE;
	}
}

/**
 * L'alleato dentro l'area viene marcato, e il marchio nasce dal PIANO.
 *
 * Nessun parametro di questa funzione dice chi e' selezionato: e' il punto. Prima la stessa informazione
 * passava per `IsPreviewAllyHitCell`, cioe' per lo stato dell'anteprima, e cambiare selezione la spegneva.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDAllyMarkFromPlanTest,
	"RefactorTactics.HUD.AllyInBlastIsMarkedFromThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDAllyMarkFromPlanTest::RunTest(const FString&)
{
	UWorld* World = MakeMarksWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	// Stessa geometria di `Combat.FriendlyFire`: Phase adiacente al bersaglio, dentro l'area r1.
	ARTUnit* Gadget    = SpawnMarksUnit(World, TEXT("Hero.Gadget"),    0, FRTCellId(-1, 0, 0));
	ARTUnit* Phase    = SpawnMarksUnit(World, TEXT("Hero.Phase"),    0, FRTCellId( 1, 0, 0));
	ARTUnit* Branth = SpawnMarksUnit(World, TEXT("Hero.Branth"), 1, FRTCellId( 2, 0, 0));
	if (!TestNotNull(TEXT("Gadget"), Gadget) || !TestNotNull(TEXT("Phase"), Phase) || !TestNotNull(TEXT("Branth"), Branth))
	{
		DestroyMarksWorld(World);
		return false;
	}

	const int32 Overload = MarksAbilityIndex(Gadget, TEXT("Hero.Gadget.Overload"));
	if (!TestTrue(TEXT("Gadget ha Overload"), Overload != INDEX_NONE)) { DestroyMarksWorld(World); return false; }
	Gadget->PlannedAbilityIndex = Overload;
	Gadget->PlannedAttackTarget = Branth;

	TSet<FRTCellId> Hit, Ally;
	ARTHUD::ComputePlannedHitMarks({ Gadget, Phase, Branth }, /*PlayerTeamId=*/ 0, Hit, Ally);
	DestroyMarksWorld(World);

	// L'area di raggio 1 accende il bersaglio piu' i suoi vicini: piu' di una cella distingue «area» da
	// «bersaglio singolo».
	TestTrue(FString::Printf(TEXT("l'area e' accesa (celle: %d)"), Hit.Num()), Hit.Num() > 1);
	TestTrue(TEXT("il bersaglio e' nella zona"), Hit.Contains(Branth->Cell));
	TestTrue(TEXT("anche la cella di Phase e' nella zona"), Hit.Contains(Phase->Cell));

	// Il punto del test: l'ALLEATA e' segnalata come fuoco amico.
	TestEqual(TEXT("una sola cella di fuoco amico"), Ally.Num(), 1);
	TestTrue(TEXT("ed e' quella di Phase"), Ally.Contains(Phase->Cell));
	// Chi lancia non si segnala mai da solo.
	TestFalse(TEXT("Gadget non e' marcato"), Ally.Contains(Gadget->Cell));
	return true;
}

/**
 * I piani AVVERSARI non si leggono, nemmeno per dedurne una cella.
 *
 * E' l'invariante #6 (privacy dell'intento) applicata a un caso in cui il risultato sarebbe «solo» un
 * colore: se il marcatore comparisse per un attacco nemico, il giocatore saprebbe che sta per essere
 * bersagliato senza che nessuno glielo abbia rivelato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDEnemyPlansAreNotReadTest,
	"RefactorTactics.HUD.EnemyPlansDoNotProduceMarks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDEnemyPlansAreNotReadTest::RunTest(const FString&)
{
	UWorld* World = MakeMarksWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	// Stavolta e' l'avversario a pianificare, su un bersaglio del giocatore.
	ARTUnit* NemicoGadget = SpawnMarksUnit(World, TEXT("Hero.Gadget"),    1, FRTCellId(-1, 0, 0));
	ARTUnit* MioPhase    = SpawnMarksUnit(World, TEXT("Hero.Phase"),    0, FRTCellId( 1, 0, 0));
	ARTUnit* MioBranth = SpawnMarksUnit(World, TEXT("Hero.Branth"), 0, FRTCellId( 2, 0, 0));
	if (!TestNotNull(TEXT("unita'"), NemicoGadget) || !MioPhase || !MioBranth)
	{
		DestroyMarksWorld(World);
		return false;
	}

	NemicoGadget->PlannedAbilityIndex = MarksAbilityIndex(NemicoGadget, TEXT("Hero.Gadget.Overload"));
	NemicoGadget->PlannedAttackTarget = MioBranth;

	TSet<FRTCellId> Hit, Ally;
	ARTHUD::ComputePlannedHitMarks({ NemicoGadget, MioPhase, MioBranth }, /*PlayerTeamId=*/ 0, Hit, Ally);
	DestroyMarksWorld(World);

	TestEqual(TEXT("nessuna cella dal piano avversario"), Hit.Num(), 0);
	TestEqual(TEXT("nessun marcatore di fuoco amico"), Ally.Num(), 0);
	return true;
}

/**
 * Un'abilita' che NON dichiara fuoco amico non produce l'avviso.
 *
 * Un allarme su un evento impossibile insegna a ignorare gli allarmi: era gia' il difetto corretto
 * nell'anteprima, e va tenuto anche qui — sono due posti che dicono la stessa cosa e devono dirla uguale.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHUDNoFriendlyFireNoMarkTest,
	"RefactorTactics.HUD.NoFriendlyFireMeansNoAllyMark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHUDNoFriendlyFireNoMarkTest::RunTest(const FString&)
{
	UWorld* World = MakeMarksWorld();
	if (!TestNotNull(TEXT("world"), World)) { return false; }

	ARTUnit* Gadget    = SpawnMarksUnit(World, TEXT("Hero.Gadget"),    0, FRTCellId(-1, 0, 0));
	ARTUnit* Phase    = SpawnMarksUnit(World, TEXT("Hero.Phase"),    0, FRTCellId( 1, 0, 0));
	ARTUnit* Branth = SpawnMarksUnit(World, TEXT("Hero.Branth"), 1, FRTCellId( 2, 0, 0));
	if (!Gadget || !Phase || !Branth) { DestroyMarksWorld(World); return false; }

	const int32 Overload = MarksAbilityIndex(Gadget, TEXT("Hero.Gadget.Overload"));
	if (!TestTrue(TEXT("Gadget ha Overload"), Overload != INDEX_NONE)) { DestroyMarksWorld(World); return false; }
	Gadget->PlannedAbilityIndex = Overload;
	Gadget->PlannedAttackTarget = Branth;

	// Si spegne il flag sulla COPIA dell'unita', non nel catalogo: il test non deve lasciare il roster sporco
	// per chi gira dopo di lui.
	URTActionData* Ability = const_cast<URTActionData*>(Gadget->GetAbility(Overload));
	if (!TestNotNull(TEXT("abilita'"), Ability)) { DestroyMarksWorld(World); return false; }
	const bool bSaved = Ability->Def.bFriendlyFire;
	Ability->Def.bFriendlyFire = false;

	TSet<FRTCellId> Hit, Ally;
	ARTHUD::ComputePlannedHitMarks({ Gadget, Phase, Branth }, /*PlayerTeamId=*/ 0, Hit, Ally);

	Ability->Def.bFriendlyFire = bSaved;
	DestroyMarksWorld(World);

	// La zona c'e' comunque — l'attacco parte lo stesso — ma l'alleata non e' segnalata.
	TestTrue(TEXT("la zona resta accesa"), Hit.Num() > 1);
	TestEqual(TEXT("nessun avviso di fuoco amico"), Ally.Num(), 0);
	return true;
}

/**
 * LE CELLE DA MARCARE SONO SOLO QUELLE NOMINABILI — `#2697`.
 *
 * 🔴 **La sentinella e' `Layer == INDEX_NONE`, e `FRTCellId::IsValid()` NON la riconosce**: la cella
 * «vuota» `(0,0,0)` supera l'invariante cubica `q + r + z == 0`. Un filtro ingenuo marcherebbe l'origine
 * dell'arena a ogni riga senza ostacolo — un pannello che compare in mezzo al campo, dove non c'e' niente,
 * ogni volta che il velo copre il muro.
 *
 * ⚠️ **La decisione vive QUI e non in `DrawHUD`** perche' `DrawHUD` non ha copertura headless: e' la
 * stessa strada di `ComputePlannedHitMarks`, e la ragione per cui quel precedente esiste.
 *
 * ⛔ **Non c'e' nessun filtro di conoscenza in questa funzione, e non deve essercene uno.** Le righe
 * arrivano gia' autorizzate da `BuildPlayerEventFeed`; riapplicare qui una regola di privacy sarebbe il
 * secondo contratto di conoscenza che `#1936` vieta — e ometterla dove serviva sarebbe stato il leak.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudBlockerMarksTest,
	"RefactorTactics.HUD.BlockerMarksOnlyNameableCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudBlockerMarksTest::RunTest(const FString&)
{
	TArray<FRTPlayerEventLineView> Feed;

	// Una riga con ostacolo nominabile: si marca.
	FRTPlayerEventLineView ConMuro;
	ConMuro.bHasBlocker = true;
	ConMuro.BlockerCell = FRTCellId(2, -1, 0);
	Feed.Add(ConMuro);

	// Una riga il cui muro il velo copre: la cella e' la SENTINELLA, e non si marca.
	FRTPlayerEventLineView SenzaMuro;
	SenzaMuro.bHasBlocker = false;
	SenzaMuro.BlockerCell = FRTCellId(0, 0, INDEX_NONE);
	Feed.Add(SenzaMuro);

	// Una riga ordinaria — un colpo, un movimento — che non ha ostacoli per costruzione.
	Feed.Add(FRTPlayerEventLineView{});

	// E lo stesso ostacolo nominato due volte nello stesso turno: due unita' possono trovare lo stesso muro.
	Feed.Add(ConMuro);

	TSet<FRTCellId> Marks;
	ARTHUD::ComputeBlockerMarks(Feed, Marks);

	TestEqual(TEXT("si marca un ostacolo solo, non quattro righe"), Marks.Num(), 1);
	TestTrue(TEXT("ed e' la cella nominabile"), Marks.Contains(FRTCellId(2, -1, 0)));

	// 🔴 L'asserzione che vale il test: l'origine dell'arena NON e' un ostacolo. Con `IsValid()` al posto
	// della sentinella, questa riga sarebbe rossa e il pannello comparirebbe in mezzo al campo.
	TestFalse(TEXT("e l'origine dell'arena non viene marcata per una sentinella"),
		Marks.Contains(FRTCellId(0, 0, 0)));
	return true;
}

/**
 * IL SEGNO D'OSTACOLO VIVE FINO AL LOCK-IN SUCCESSIVO, E NON OLTRE — `D-359`.
 *
 * 🔑 **Il confine non e' un timer: e' il ciclo del turno.** `ARTHUD::DrawHUD` costruisce le marche da
 * `URTHudViewModel::BuildPlayerEventFeed(TurnManager, ...)`, che legge `ARTTurnManager::GetTurnLog()` —
 * gli esiti dell'**ultimo turno risolto** — e `LockInAndResolve` fa `TurnLog.Reset()` una riga dopo aver
 * annunciato `OnLockInCommitted`. Ne segue, senza che nessuno lo scriva a mano, che il segno resta acceso
 * per tutta la **pianificazione** seguente e si spegne quando il giocatore committa.
 *
 * ⚠️ **Questo test esiste perche' quella proprieta' oggi non e' presidiata da niente.** E' emersa da una
 * lettura del codice durante `#2534`, non da un oracolo: chiunque sposti `TurnLog.Reset()` — per esempio a
 * fine risoluzione, che sembra il posto naturale — spegnerebbe la spiegazione **prima** che il giocatore
 * abbia la possibilita' di leggerla, e nessun test cadrebbe.
 *
 * Le due meta' vanno tenute insieme, e la seconda e' quella che rende il test non vacuo:
 *   (1) dopo la risoluzione il feed NON e' vuoto  -> il segno c'e' quando serve;
 *   (2) dopo il lock-in successivo NON accumula   -> il segno non sopravvive alla sua ragione.
 * Con la sola (1) passerebbe anche un feed che non si spegne mai; con la sola (2) passerebbe un feed
 * sempre vuoto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlockerMarkLivesUntilNextLockInTest,
	"RefactorTactics.HUD.BlockerMarkLivesUntilNextLockIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlockerMarkLivesUntilNextLockInTest::RunTest(const FString&)
{
	UWorld* World = MakeMarksWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
	ARTHexMapActor* Map = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyMarksWorld(World);
		return false;
	}
	Map->MapAsset = Asset;

	// Due unita' avversarie: basta che il turno produca voci di TurnLog, e due unita' che restano ferme
	// ne producono. Il fenomeno sotto misura e' il CICLO, non l'esito.
	ARTUnit* A = SpawnMarksUnit(World, TEXT("Hero.Gadget"), /*TeamId=*/ 0, FRTCellId(-1, 0, 0));
	ARTUnit* B = SpawnMarksUnit(World, TEXT("Hero.Branth"), /*TeamId=*/ 1, FRTCellId(1, 0, 0));
	if (!TestNotNull(TEXT("unita' A"), A) || !TestNotNull(TEXT("unita' B"), B))
	{
		DestroyMarksWorld(World);
		return false;
	}

	// ── Turno 1: risolto. Da qui in poi il giocatore PIANIFICA il turno 2, ed e' la finestra in cui la
	//    spiegazione di cio' che e' appena successo deve restare leggibile.
	PlayOneMarksTurn(TM);
	const int32 DuranteLaPianificazione =
		URTHudViewModel::BuildPlayerEventFeed(TM, /*ObserverTeamId=*/ 0).Num();

	if (!TestTrue(TEXT("(1) il turno risolto lascia righe nel feed: il segno c'e' mentre si pianifica"),
		DuranteLaPianificazione > 0))
	{
		DestroyMarksWorld(World);
		return false;
	}

	// ── Turno 2: il giocatore ha committato. `LockInAndResolve` ha azzerato il TurnLog, e cio' che il
	//    feed porta ora appartiene al turno NUOVO.
	PlayOneMarksTurn(TM);
	const int32 VociDelTurnoDue = TM->GetTurnLog().Num();
	const int32 DopoIlLockIn =
		URTHudViewModel::BuildPlayerEventFeed(TM, /*ObserverTeamId=*/ 0).Num();

	// PREMESSA della seconda meta': il turno 2 ha prodotto esiti. Senza, «feed vuoto» sarebbe soddisfatto
	// anche da un turno che non e' mai avvenuto, e l'asserzione sotto non misurerebbe il ciclo.
	TestTrue(TEXT("premessa: anche il turno 2 ha prodotto voci di TurnLog"), VociDelTurnoDue > 0);

	// 🔴 L'asserzione che vale il test, e il valore atteso e' ZERO — non «lo stesso numero di prima».
	// ⚠️ **Misurato, non dedotto**: il turno 2 di questo montaggio produce voci di TurnLog ma **nessuna
	// riga proiettabile** — sono movimenti che `Project` classifica minori e omette (`OmitsMinorMovement`).
	// E' precisamente cio' che rende l'oracolo stretto: il feed **puo' essere vuoto solo se le righe del
	// turno 1 sono sparite**. Se il TurnLog accumulasse — cioe' se il segno sopravvivesse al turno che lo
	// ha prodotto — qui si leggerebbero ancora le 2 righe di prima, e un marcatore resterebbe acceso su
	// una cella la cui ragione e' passata: un segno che non scade mente.
	// ⌫ La prima stesura asseriva `DopoIlLockIn == DuranteLaPianificazione`, dando per scontato che due
	// turni uguali producessero lo stesso numero di righe. Il test e' caduto con `2` atteso e `0` trovato:
	// l'asserzione era sbagliata, non il codice.
	TestEqual(TEXT("(2) le righe del turno 1 non sopravvivono al lock-in del turno 2"),
		DopoIlLockIn, 0);

	DestroyMarksWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
