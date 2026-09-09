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
	 * Un turno intero: commit e risoluzione. Il tick e' quello della risoluzione, non un'attesa:
	 * `IsResolving()` e' la condizione, il 400 e' il tetto che impedisce a un difetto di diventare un test
	 * appeso.
	 *
	 * ⛔ **Non si riusa `RTWorldFixtures::PlayOneTurn`, e la ragione non e' la comodita'**: quella chiama
	 * `PlanBotsForTest()` prima del commit, e i test di questo file **dichiarano il piano a mano**
	 * (`PlannedAbilityIndex`, `PlannedAttackTarget`). Farlo ripianificare misurerebbe il bot, non il ciclo
	 * del turno. Il corpo e' lo stesso di quel helper meno quella riga, ed e' la riga che conta.
	 *
	 * ⚠️ **Chi chiama verifica che il turno sia AVANZATO**, e non lo fa questo helper: `RTTurnManager.h`
	 * avverte che una seconda `LockInAndResolve()` mentre `IsResolving()` e' ancora vero e' un **no-op
	 * silenzioso**. Un test che non guardasse `GetTurnNumber()` racconterebbe di un lock-in mai avvenuto.
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

	// ⛔ **Le celle si copiano PRIMA di distruggere il mondo.** Le asserzioni qui sotto leggevano
	// `Branth->Cell` e `Phase->Cell` **dopo** `DestroyMarksWorld`, cioe' da Actor di un mondo gia'
	// distrutto: funzionava solo perche' il GC non era ancora passato, e un giro di garbage collection
	// fra le due righe — plausibile quando la suite esegue l'intero gruppo `RefactorTactics.HUD` — le
	// avrebbe fatte leggere memoria liberata. Trovato dalla code review su `#2726`.
	const FRTCellId CellaBranth = Branth->Cell;
	const FRTCellId CellaPhase  = Phase->Cell;
	const FRTCellId CellaGadget = Gadget->Cell;

	DestroyMarksWorld(World);

	// L'area di raggio 1 accende il bersaglio piu' i suoi vicini: piu' di una cella distingue «area» da
	// «bersaglio singolo».
	TestTrue(FString::Printf(TEXT("l'area e' accesa (celle: %d)"), Hit.Num()), Hit.Num() > 1);
	TestTrue(TEXT("il bersaglio e' nella zona"), Hit.Contains(CellaBranth));
	TestTrue(TEXT("anche la cella di Phase e' nella zona"), Hit.Contains(CellaPhase));

	// Il punto del test: l'ALLEATA e' segnalata come fuoco amico.
	TestEqual(TEXT("una sola cella di fuoco amico"), Ally.Num(), 1);
	TestTrue(TEXT("ed e' quella di Phase"), Ally.Contains(CellaPhase));
	// Chi lancia non si segnala mai da solo.
	TestFalse(TEXT("Gadget non e' marcato"), Ally.Contains(CellaGadget));
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
 * Le due meta' vanno tenute insieme, e ciascuna copre il buco dell'altra:
 *   (1) dopo la risoluzione la CELLA DEL MURO e' fra le marche -> il segno c'e' quando serve;
 *   (2) subito dopo il commit del turno 2, e PRIMA dei tick    -> il segno e' gia' spento.
 * Con la sola (1) passerebbe un segno che non si spegne mai; con la sola (2), un segno che non si accende.
 *
 * 🔑 **Il campionamento della (2) e' la parte non ovvia.** Leggere dopo la risoluzione del turno 2 non
 * distinguerebbe `D-359` — *«si spegne quando il giocatore committa»* — da *«si spegne a fine risoluzione»*:
 * entrambe darebbero zero. Fra `OnLockInCommitted.Broadcast()` e `TurnLog.Reset()` corrono quattro righe, e
 * leggere in quell'istante e' l'unico modo di dire quale delle due regole vale.
 *
 * ⌫ **La prima stesura non provava niente di tutto questo**, ed e' stata trovata dalla code review: contava
 * la lunghezza del feed su un montaggio senza muro e senza attacco, quindi non produceva **nessuna** riga
 * con ostacolo e non chiamava mai `ComputeBlockerMarks`. Restava verde anche cancellando la riga che copia
 * `BlockerCell` nel feed. E la sua premessa poggiava su un incidente: `SpawnMarksUnit` non inizializza
 * `PlannedCell`, che resta `(0,0,0)`, quindi le due unita' «ferme» pianificavano entrambe l'origine e si
 * contendevano la cella — le righe contate venivano da li'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBlockerMarkLivesUntilNextLockInTest,
	"RefactorTactics.HUD.BlockerMarkLivesUntilNextLockIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBlockerMarkLivesUntilNextLockInTest::RunTest(const FString&)
{
	UWorld* World = MakeMarksWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	// ⚠️ **L'arena si controlla**: senza `MapAsset` l'actor genera il proprio esagono dimostrativo
	// (`DemoRadius`), e il turno si risolverebbe su una topologia che nessuno ha dichiarato.
	URTHexMapAsset* Asset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
	if (!TestNotNull(TEXT("arena di prova"), Asset)) { DestroyMarksWorld(World); return false; }

	// Il MURO, ed e' cio' che rende questo test diverso da una misura di lunghezza del feed: la cella
	// (0,0,0) nega la linea di tiro, quindi l'attacco pianificato sotto produce `NoLineOfSight` **con la
	// cella bloccante** — l'unica voce da cui nasce un marcatore.
	FRTHexCellData Muro(FRTCellId(0, 0, 0));
	Muro.bBlocksLineOfSight = true;
	Asset->AddOrUpdateCell(Muro);
	Asset->SortCells();

	ARTHexMapActor* Map = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("mappa"), Map) || !TestNotNull(TEXT("turn manager"), TM))
	{
		DestroyMarksWorld(World);
		return false;
	}
	Map->MapAsset = Asset;

	ARTUnit* Gadget = SpawnMarksUnit(World, TEXT("Hero.Gadget"), /*TeamId=*/ 0, FRTCellId(-1, 0, 0));
	ARTUnit* Branth = SpawnMarksUnit(World, TEXT("Hero.Branth"), /*TeamId=*/ 1, FRTCellId(1, 0, 0));
	if (!TestNotNull(TEXT("Gadget"), Gadget) || !TestNotNull(TEXT("Branth"), Branth))
	{
		DestroyMarksWorld(World);
		return false;
	}

	// ⛔ **`PlaceOnCell`, non il solo `Cell`.** E' l'unico punto che inizializza anche `PlannedCell`: senza,
	// resta al default `(0,0,0)` e ogni unita' pianifica in silenzio un movimento verso l'origine — due
	// unita' che si contendono la stessa cella, esito `BlockedContested`, righe nel feed che nessuno ha
	// chiesto. E' l'incidente su cui la prima stesura di questo test poggiava la propria premessa.
	Gadget->PlaceOnCell(FRTCellId(-1, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
	Branth->PlaceOnCell(FRTCellId(1, 0, 0), FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);

	// L'attacco che il muro nega: e' il fenomeno di `#2534`, e produce la voce che porta `SightBlockerCell`.
	const int32 ArcPulse = MarksAbilityIndex(Gadget, TEXT("Hero.Gadget.ArcPulse"));
	if (!TestTrue(TEXT("premessa: Gadget ha ArcPulse nel kit"), ArcPulse != INDEX_NONE))
	{
		DestroyMarksWorld(World);
		return false;
	}
	Gadget->PlannedAbilityIndex = ArcPulse;
	Gadget->PlannedAttackTarget = Branth;

	// ── Turno 1: risolto per intero. Da qui il giocatore PIANIFICA il turno 2, ed e' la finestra in cui
	//    la spiegazione di cio' che e' appena successo deve restare leggibile.
	const int32 TurnoPrima = TM->GetTurnNumber();
	PlayOneMarksTurn(TM);
	if (!TestFalse(TEXT("premessa: il turno 1 si e' concluso, non e' rimasto in risoluzione"),
			TM->IsResolving())
		|| !TestTrue(TEXT("premessa: il turno e' avanzato"), TM->GetTurnNumber() > TurnoPrima))
	{
		DestroyMarksWorld(World);
		return false;
	}

	// (1) IL SEGNO C'E', e si misura dove vive davvero: nelle marche, non nel numero di righe.
	TSet<FRTCellId> DuranteLaPianificazione;
	ARTHUD::ComputeBlockerMarks(
		URTHudViewModel::BuildPlayerEventFeed(TM, /*ObserverTeamId=*/ 0), DuranteLaPianificazione);
	if (!TestTrue(TEXT("(1) mentre si pianifica, la cella che ha fermato il tiro e' marcata"),
		DuranteLaPianificazione.Contains(FRTCellId(0, 0, 0))))
	{
		DestroyMarksWorld(World);
		return false;
	}

	// ── (2) IL COMMIT, e NIENTE TICK. E' il campionamento che rende il test fedele al proprio nome:
	//    `D-359` dice che il segno si spegne **quando il giocatore committa**, non a fine risoluzione.
	//    Fra `OnLockInCommitted.Broadcast()` e `TurnLog.Reset()` corrono quattro righe; leggere qui — dopo
	//    il commit e prima che la risoluzione dreni — e' l'unico istante che distingue le due regole.
	//    🔴 Spostare il reset in coda alla risoluzione lascerebbe verde un test che campionasse dopo i
	//    tick, e il marcatore sopravviverebbe a tutto il turno seguente.
	TM->LockInAndResolve();
	TSet<FRTCellId> SubitoDopoIlCommit;
	ARTHUD::ComputeBlockerMarks(
		URTHudViewModel::BuildPlayerEventFeed(TM, /*ObserverTeamId=*/ 0), SubitoDopoIlCommit);
	TestEqual(TEXT("(2) al commit del turno 2 il segno e' gia' spento, non a fine risoluzione"),
		SubitoDopoIlCommit.Num(), 0);

	// Il turno 2 si drena: un TurnManager lasciato a meta' non e' uno stato che questo test debba produrre.
	for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }

	DestroyMarksWorld(World);
	return true;
}

/**
 * ⛔ A SCHERMO, «NIENTE» E «NESSUN RIFIUTO» SONO LA STESSA COSA — `#2741`, [D-225].
 *
 * 🔴 **Il canary di `RefusalForObserver` pinna l'indistinguibilita' sul VALORE; questo la pinna su cio' che
 * il giocatore riceve.** Sono due difese diverse: la prima cade se qualcuno separa i due esiti nell'enum, la
 * seconda se qualcuno da' a `Nothing` una frase — «non puoi bersagliare qui» sembra innocuo, e comparirebbe
 * dove sta un nemico velato e **non** dove la cella e' vuota. La differenza fra un messaggio e il silenzio
 * sarebbe essa stessa l'informazione.
 *
 * ⚠️ Il test asserisce l'**uguaglianza fra i due**, non che ciascuno sia vuoto: due stringhe entrambe
 * generiche ma diverse riaprirebbero il canale, e un test su ciascuna separatamente non lo vedrebbe.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRefusalTextSaysNothingForNothingTest,
	"RefactorTactics.HUD.RefusalTextSaysNothingForNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRefusalTextSaysNothingForNothingTest::RunTest(const FString&)
{
	const FString Niente = ARTHUD::RefusalText(ERTTargetRefusal::Nothing);
	const FString Nessuno = ARTHUD::RefusalText(ERTTargetRefusal::None);

	TestEqual(TEXT("«niente da bersagliare» e «nessun rifiuto» dicono la stessa cosa a schermo"),
		Niente, Nessuno);
	TestTrue(TEXT("e quella cosa e' il silenzio"), Niente.IsEmpty());

	// 🔴 La meta' che impedisce l'implementazione degenere: se `RefusalText` restituisse sempre vuoto, le
	// due righe sopra passerebbero e il giocatore non riceverebbe MAI un rifiuto. I due esiti che devono
	// parlare, parlano — e dicono cose diverse fra loro.
	const FString Coperto = ARTHUD::RefusalText(ERTTargetRefusal::Cover);
	const FString Lontano = ARTHUD::RefusalText(ERTTargetRefusal::Range);
	TestFalse(TEXT("la copertura ha un testo"), Coperto.IsEmpty());
	TestFalse(TEXT("la portata ha un testo"), Lontano.IsEmpty());
	TestNotEqual(TEXT("e i due non dicono la stessa frase"), Coperto, Lontano);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
