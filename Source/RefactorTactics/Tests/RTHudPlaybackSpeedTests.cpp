// Il controllo di velocita' in HUD (CP 47.7, #1015): la scala, l'etichetta e cosa il controllo scrive.
//
// #955 ha consegnato la manopola NEL MODELLO — `ARTTurnManager::ViewerPlaybackSpeed` — e il suo gate
// (`Match.Autobattle.DeterminismIsIndependentOfPlayback`, sette varianti) prova che girare la manopola non
// cambia il risultato logico. ⛔ **Quel gate non va duplicato qui**: e' esplicito nel DoD di #1015, e un
// secondo test di determinismo sposterebbe le varianti future in due posti.
//
// ⚠️ **Dal 2026-09-02 la manopola e' l'UNICA cosa che accelera la riproduzione** (#1878): il tetto di
// durata non produce piu' un fattore di velocita', comprime le attese. La domanda 1 qui sotto era
// «l'etichetta dice il vero anche quando il tetto vince?» e non ha piu' un soggetto — il tetto non vince
// piu' su niente che si veda come velocita'.
//
// Cio' che manca a quel gate, e che questi test coprono, e' l'altro lato: la manopola era raggiungibile
// solo da dettagli dell'attore e da Blueprint. Tre domande, una per test:
//
//   1. l'etichetta dice il vero, e senza un secondo numero che non ha piu' causa?
//   2. la scala resta `x1 · x2 · x4` anche partendo da un valore che non le appartiene?
//   3. il controllo scrive `ViewerPlaybackSpeed` **e nient'altro**?
//
// Le prime due sono pure e non toccano il mondo. La terza allestisce un `ARTTurnManager`, perche' «scrive
// solo quel campo» e' una proprieta' dell'ATTORE e non della funzione: verificarla su un valore di ritorno
// sarebbe verificare un'altra cosa.

#include "Misc/AutomationTest.h"
#include "UI/RTHUD.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Unit/RTUnit.h" // la guardia di `D-350` interroga le unita', non una modalita'
#include "Tests/RTWorldFixtures.h" // MakePlayerOnTeam: la squadra viene dal PlayerState, non da un default
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome distinto da ogni altro file di test: la unity build condivide la translation unit.
	UWorld* MakePlaybackSpeedWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	void DestroyPlaybackSpeedWorld(UWorld* World)
	{
		if (World && GEngine)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ false);
		}
	}
}

/**
 * L'etichetta dice la velocita' a cui la riproduzione scorre, e dal 2026-09-02 quel numero e' uno solo.
 *
 * ⚠️ **Il nome di questo test era `...DeclaresTheCapWhenItWins` e mentiva** dopo `#1878`: il tetto non
 * vince piu' su niente che si veda come velocita', perche' non produce piu' un fattore — comprime le
 * attese via `URTPlaybackLibrary::SlackScaleForBudget`. Il criterio 2 di CP 47.7 (`#1015`) chiedeva che
 * l'etichetta non mentisse quando lo schermo scorreva piu' in fretta di quanto la manopola dicesse; quel
 * caso non esiste piu', e il criterio e' soddisfatto dal fatto che di numeri ce n'e' uno.
 *
 * ⛔ **Le asserzioni `TestFalse(Contains("tetto"))` sono state tolte, non conservate.** Erano vacue: la
 * stringa «tetto» e' irraggiungibile in `ComposePlaybackSpeedLabel`, quindi passavano comunque — e il
 * commento che le accompagnava affermava l'inverso di cio' che verificavano. Un test che non puo' fallire
 * non protegge nulla; quello che protegge davvero e' l'ultima asserzione, che lega il numero mostrato alla
 * funzione che lo produce.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPlaybackSpeedLabelTest,
	"RefactorTactics.HUD.PlaybackSpeedLabelShowsTheChosenSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPlaybackSpeedLabelTest::RunTest(const FString&)
{
	// I tre valori della scala, e l'etichetta e' esattamente il numero: nessun ornamento da interpretare.
	TestEqual(TEXT("x1"), ARTHUD::ComposePlaybackSpeedLabel(1.f), FString(TEXT("x1")));
	TestEqual(TEXT("x2"), ARTHUD::ComposePlaybackSpeedLabel(2.f), FString(TEXT("x2")));
	TestEqual(TEXT("x4"), ARTHUD::ComposePlaybackSpeedLabel(4.f), FString(TEXT("x4")));

	// Fuori scala: il campo e' `EditAnywhere` e puo' portare un valore scritto a mano. Un decimale non si
	// arrotonda a un intero che nessuno ha scelto.
	TestEqual(TEXT("x2,6 non diventa x3"), ARTHUD::ComposePlaybackSpeedLabel(2.6f), FString(TEXT("x2.6")));

	// --- Guardia: un campo azzerato vale x1, come lo tratta `EffectivePlaybackSpeed`. Un'etichetta «x0»
	// direbbe che la riproduzione e' ferma, che e' un'altra cosa — e non e' vera.
	{
		const FString Label = ARTHUD::ComposePlaybackSpeedLabel(/*Viewer=*/ 0.f);
		TestFalse(TEXT("velocita' scelta nulla: mai «x0»"), Label.Contains(TEXT("x0")));
		TestTrue(TEXT("velocita' scelta nulla: letta come x1"), Label.Contains(TEXT("x1")));
	}

	// --- L'etichetta non inventa: il numero che dichiara E' quello che la libreria normalizza. Se
	// qualcuno rifacesse il conto qui dentro invece di interrogarla, questa riga cade.
	{
		const float Viewer = 2.f;
		const float Effective = URTPlaybackLibrary::EffectivePlaybackSpeed(Viewer);
		const FString Label = ARTHUD::ComposePlaybackSpeedLabel(Viewer);
		TestTrue(TEXT("il numero dell'etichetta viene da EffectivePlaybackSpeed"),
			Label.Contains(FString::Printf(TEXT("x%d"), FMath::RoundToInt(Effective))));
	}

	return true;
}

/**
 * La scala e' `x1 · x2 · x4` e il ciclo non ne esce MAI, nemmeno partendo da fuori.
 *
 * ⚠️ **Non e' pedanteria sui casi limite: il valore di partenza non e' sotto il controllo dell'HUD.**
 * `ViewerPlaybackSpeed` e' `EditAnywhere` e `BlueprintReadWrite` — chi apre i dettagli dell'attore, o un
 * Blueprint scritto prima di questo checkpoint, puo' lasciarci `3` o `0`. Un ciclo implementato come
 * «raddoppia, e a 8 torna a 1» su un `3` produrrebbe `6`, cioe' una velocita' che #955 non ha deciso e che
 * il DoD dichiara fuori scope.
 *
 * La regola scelta e' una sola e copre tutto: **la piu' piccola velocita' legale strettamente maggiore di
 * quella corrente, e se non esiste si torna a x1.** Il ciclo e' un caso particolare della guardia, non una
 * riga a parte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPlaybackSpeedCycleTest,
	"RefactorTactics.HUD.PlaybackSpeedCycleStaysOnTheDeclaredScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPlaybackSpeedCycleTest::RunTest(const FString&)
{
	const float Tol = 1e-3f;

	// --- Il giro nominale, e si chiude: tre pressioni riportano al punto di partenza.
	TestTrue(TEXT("x1 -> x2"), FMath::IsNearlyEqual(ARTHUD::NextViewerPlaybackSpeed(1.f), 2.f, Tol));
	TestTrue(TEXT("x2 -> x4"), FMath::IsNearlyEqual(ARTHUD::NextViewerPlaybackSpeed(2.f), 4.f, Tol));
	TestTrue(TEXT("x4 -> x1, il giro si chiude"),
		FMath::IsNearlyEqual(ARTHUD::NextViewerPlaybackSpeed(4.f), 1.f, Tol));

	// --- Fuori scala: si rientra, non si prosegue. `3` e' il caso realistico (qualcuno ha scritto a mano
	// nei dettagli dell'attore), `6` quello che un raddoppio ingenuo produrrebbe da li'.
	TestTrue(TEXT("x3 fuori scala -> x4, la prima legale sopra"),
		FMath::IsNearlyEqual(ARTHUD::NextViewerPlaybackSpeed(3.f), 4.f, Tol));
	TestTrue(TEXT("x6 fuori scala e sopra il massimo -> x1"),
		FMath::IsNearlyEqual(ARTHUD::NextViewerPlaybackSpeed(6.f), 1.f, Tol));

	// --- Guardie, coerenti con `EffectivePlaybackSpeed`: non positivo vale «non scelto», cioe' x1.
	// Premere il tasto da li' deve portare alla velocita' successiva a x1, non riproporre x1.
	TestTrue(TEXT("zero -> x2 (letto come x1, poi il passo)"),
		FMath::IsNearlyEqual(ARTHUD::NextViewerPlaybackSpeed(0.f), 2.f, Tol));
	TestTrue(TEXT("negativo -> x2 (letto come x1, poi il passo)"),
		FMath::IsNearlyEqual(ARTHUD::NextViewerPlaybackSpeed(-3.f), 2.f, Tol));

	// --- Qualunque punto di partenza, in due pressioni si e' comunque sulla scala. E' l'invariante che
	// rende la regola totale invece che un elenco di casi.
	const float Starts[] = { 1.f, 2.f, 4.f, 0.f, -3.f, 0.5f, 3.f, 6.f, 100.f };
	for (const float Start : Starts)
	{
		const float Once = ARTHUD::NextViewerPlaybackSpeed(Start);
		const bool bLegal = FMath::IsNearlyEqual(Once, 1.f, Tol)
			|| FMath::IsNearlyEqual(Once, 2.f, Tol)
			|| FMath::IsNearlyEqual(Once, 4.f, Tol);
		TestTrue(*FString::Printf(TEXT("da %.2f una pressione porta sulla scala (ottenuto %.2f)"), Start, Once),
			bLegal);
	}

	return true;
}

/**
 * Il controllo scrive `ViewerPlaybackSpeed` **e nient'altro** (voce 1 del DoD, invariante #1).
 *
 * ⚠️ **Questo test allestisce un attore invece di chiamare una funzione pura, e la ragione e' che la
 * proprieta' da verificare non e' esprimibile su un valore di ritorno.** «Non tocca il tetto» parla di cio'
 * che il codice NON fa a un oggetto: serve l'oggetto.
 *
 * ⚠️ **Il tetto e' il campo che conta.** `MaxPlaybackSeconds` non produce piu' velocita' dal 2026-09-02
 * (`#1878`) — comprime le attese — ma resta il campo da cui un controllo potrebbe essere tentato di
 * passare: abbassarlo accorcerebbe la risoluzione, e un utente lo leggerebbe come «e' andata piu' veloce».
 * Sarebbe il secondo fuori scope dichiarato nella issue, con due produttori su una sola sensazione. Il
 * gate di determinismo non lo vedrebbe: resterebbe verde, perche' anche il tetto e' presentazione.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPlaybackSpeedWritesOnlyViewerTest,
	"RefactorTactics.HUD.PlaybackSpeedControlWritesOnlyTheViewerField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPlaybackSpeedWritesOnlyViewerTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackSpeedWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>();
	if (!TestNotNull(TEXT("turn manager di prova"), TurnManager))
	{
		DestroyPlaybackSpeedWorld(World);
		return false;
	}

	// Fotografia di TUTTE le manopole di pacing prima del comando.
	const bool  bPlaybackBefore   = TurnManager->bEnablePlayback;
	const float CellsBefore       = TurnManager->PlaybackCellsPerSecond;
	const float BeatBefore        = TurnManager->PhaseBeatSeconds;
	const float AttackShowBefore  = TurnManager->AttackShowSeconds;
	const float MaxSecondsBefore  = TurnManager->MaxPlaybackSeconds;

	TurnManager->ViewerPlaybackSpeed = 1.f;
	ARTPlayerController::ApplyNextPlaybackSpeed(TurnManager);

	// Il campo che il controllo possiede E' cambiato: senza questa riga il test sarebbe soddisfatto da un
	// comando che non fa nulla — «non tocca niente» e' vero anche di chi non fa niente.
	TestTrue(TEXT("il comando ha scritto la velocita' scelta"),
		FMath::IsNearlyEqual(TurnManager->ViewerPlaybackSpeed, 2.f, 1e-3f));

	// E nessun altro.
	TestEqual(TEXT("bEnablePlayback intatto"), TurnManager->bEnablePlayback, bPlaybackBefore);
	TestEqual(TEXT("PlaybackCellsPerSecond intatto"), TurnManager->PlaybackCellsPerSecond, CellsBefore);
	TestEqual(TEXT("PhaseBeatSeconds intatto"), TurnManager->PhaseBeatSeconds, BeatBefore);
	TestEqual(TEXT("AttackShowSeconds intatto"), TurnManager->AttackShowSeconds, AttackShowBefore);
	TestEqual(TEXT("il TETTO non e' toccato: due produttori sullo stesso numero sono il fuori scope"),
		TurnManager->MaxPlaybackSeconds, MaxSecondsBefore);

	// La scelta PERSISTE: nessuno la reimposta fra un comando e l'altro (voce 3 del DoD). Il campo e' gia'
	// dell'attore e sopravvive per costruzione — cio' che si verifica qui e' che il CONTROLLO non lo
	// riazzeri, che e' il modo in cui la persistenza si perde davvero.
	ARTPlayerController::ApplyNextPlaybackSpeed(TurnManager);
	TestTrue(TEXT("seconda pressione: x2 -> x4, non un ritorno a x1"),
		FMath::IsNearlyEqual(TurnManager->ViewerPlaybackSpeed, 4.f, 1e-3f));

	// Un comando senza partita non deve esplodere: l'HUD esiste anche prima del turn manager.
	ARTPlayerController::ApplyNextPlaybackSpeed(nullptr);

	DestroyPlaybackSpeedWorld(World);
	return true;
}

// =====================================================================================================
// La manopola e' di chi OSSERVA — [`D-350`](../../../docs/decisions/RT_PDR_00_Decision_Log.md), CP 14.6 (`#166`), voce 12
//
// ⚠️ **Questi due test misurano la POLITICA, non la meccanica.** I tre sopra provano la scala e cosa il
// controllo scrive, e restano invariati: `ApplyNextPlaybackSpeed` non ha guardie e non deve averne. Qui si
// prova chi puo' girare la manopola, e la domanda passa da `ARTPlayerController::CyclePlaybackSpeed()` —
// che e' lo **stesso corpo** che l'input esegue. Un predicato provato a parte sarebbe un criterio che non
// puo' fallire: togliere la guardia dall'handler lo lascerebbe verde.
// =====================================================================================================

namespace
{
	/** Nome distinto per la unity build, come `MakePlaybackSpeedWorld`. */
	ARTUnit* SpawnSpeedUnit(UWorld* World, int32 TeamId, bool bBot)
	{
		ARTUnit* Unit = World ? World->SpawnActor<ARTUnit>() : nullptr;
		if (Unit)
		{
			Unit->TeamId = TeamId;
			Unit->bIsBotControlled = bBot;
		}
		return Unit;
	}
}

/**
 * 🔴 **DOVE UNA FINESTRA PUO' APRIRSI, IL TASTO `V` E' INERTE** (`#166`, voce 12).
 *
 * Accelerare mentre un `FastReactionDuration` scorre accorcerebbe il tempo che il gioco concede per
 * rispondere: sarebbe la presentazione a decidere quanto dura una regola. `D-350` lo vieta con un criterio
 * **funzionale** — *«esiste un'unita' del giocatore locale, non bot, che puo' ricevere una finestra?»* — e
 * non nominando una modalita': `ERTMatchMode` non esiste, e inventarlo per un binding di tastiera sarebbe
 * il cambiamento piu' grande dei due.
 *
 * ⚠️ **Il PASSO 1 e' il controllo, e senza di esso il test sarebbe soddisfatto da un tasto rotto**: «non
 * cambia niente» e' vero anche di un comando che non funziona piu'.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPlaybackSpeedInertWithWindowTest,
	"RefactorTactics.HUD.PlaybackSpeedIsInertWhereAWindowCanOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPlaybackSpeedInertWithWindowTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackSpeedWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 0);
	ARTUnit* Mia = SpawnSpeedUnit(World, /*TeamId=*/ 0, /*bBot=*/ true);
	if (!TestNotNull(TEXT("turn manager"), TurnManager) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("unita'"), Mia))
	{
		DestroyPlaybackSpeedWorld(World);
		return false;
	}

	// --- PASSO 1 — CONTROLLO: solo unita' bot, la manopola gira -------------------------------------
	// E' il caso dell'autobattle: `bIsBotControlled` e' vero su tutte (`RTMatchBootstrapper`), nessuna
	// finestra puo' aprirsi, e la scala resta quella per cui e' nata.
	TurnManager->ViewerPlaybackSpeed = 1.f;
	PC->CyclePlaybackSpeed();
	if (!TestTrue(TEXT("controllo: con sole unita' bot il tasto funziona (x1 -> x2)"),
			FMath::IsNearlyEqual(TurnManager->ViewerPlaybackSpeed, 2.f, 1e-3f)))
	{
		DestroyPlaybackSpeedWorld(World);
		return false;
	}

	// --- PASSO 2 — LA MISURA: la stessa unita' torna al giocatore, il tasto si spegne ---------------
	// Cambia UN campo solo fra i due passi: e' cio' che rende la differenza attribuibile alla guardia e non
	// all'allestimento.
	Mia->bIsBotControlled = false;
	const float Prima = TurnManager->ViewerPlaybackSpeed;

	PC->CyclePlaybackSpeed();
	TestTrue(TEXT("con un'unita' del giocatore il tasto e' inerte"),
		FMath::IsNearlyEqual(TurnManager->ViewerPlaybackSpeed, Prima, 1e-3f));

	// E resta inerte: non e' un ritardo di un frame, e' una porta chiusa.
	PC->CyclePlaybackSpeed();
	PC->CyclePlaybackSpeed();
	TestTrue(TEXT("e resta inerte a ogni pressione"),
		FMath::IsNearlyEqual(TurnManager->ViewerPlaybackSpeed, Prima, 1e-3f));

	DestroyPlaybackSpeedWorld(World);
	return true;
}

/**
 * ⛔ **UN'UNITA' CHE NON PUO' RISPONDERE NON SPEGNE IL TASTO** (`#166`, voce 12).
 *
 * Il criterio di `D-350` dice *«un'unita' del giocatore locale, non bot, che PUO' RICEVERE una finestra»*, e
 * i due modi in cui quel «puo'» e' falso sono qui:
 *
 * · **l'unita' e' caduta** — il ciclo delle reazioni salta chi non e' vivo (`!WatchOwner->IsAlive()`), e da
 *   quel momento il giocatore sta **osservando**: e' esattamente di chi osserva che la velocita' e';
 * · **l'unita' non e' sua** — un avversario umano ricevera' le proprie finestre sul proprio client, e
 *   spegnere il tasto qui proteggerebbe una decisione che non passa da questa persona.
 *
 * 🔴 **Senza questo test la guardia sarebbe indistinguibile da un «esiste un'unita' non-bot qualsiasi?»**,
 * che e' la forma sbagliata e piu' facile da scrivere.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTHudPlaybackSpeedIgnoresSilentUnitsTest,
	"RefactorTactics.HUD.PlaybackSpeedIgnoresUnitsThatCannotAnswer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTHudPlaybackSpeedIgnoresSilentUnitsTest::RunTest(const FString&)
{
	UWorld* World = MakePlaybackSpeedWorld();
	if (!TestNotNull(TEXT("world di prova"), World)) { return false; }

	ARTTurnManager* TurnManager = World->SpawnActor<ARTTurnManager>();
	ARTPlayerController* PC = RTWorldFixtures::MakePlayerOnTeam(World, /*TeamId=*/ 0);
	// Un'unita' NON bot della squadra avversaria: la piu' vicina al falso positivo.
	ARTUnit* Avversaria = SpawnSpeedUnit(World, /*TeamId=*/ 1, /*bBot=*/ false);
	// E una MIA, non bot, ma caduta.
	ARTUnit* Caduta = SpawnSpeedUnit(World, /*TeamId=*/ 0, /*bBot=*/ false);
	if (!TestNotNull(TEXT("turn manager"), TurnManager) || !TestNotNull(TEXT("controller"), PC)
		|| !TestNotNull(TEXT("unita' avversaria"), Avversaria) || !TestNotNull(TEXT("unita' caduta"), Caduta))
	{
		DestroyPlaybackSpeedWorld(World);
		return false;
	}
	Caduta->Health = 0;
	if (!TestFalse(TEXT("premessa: l'unita' e' davvero caduta"), Caduta->IsAlive()))
	{
		DestroyPlaybackSpeedWorld(World);
		return false;
	}

	// Nessuna delle due puo' ricevere una finestra su questo client: la manopola resta girabile.
	TurnManager->ViewerPlaybackSpeed = 1.f;
	PC->CyclePlaybackSpeed();
	TestTrue(TEXT("un'unita' avversaria e una caduta non spengono il tasto (x1 -> x2)"),
		FMath::IsNearlyEqual(TurnManager->ViewerPlaybackSpeed, 2.f, 1e-3f));

	// --- La controprova: basta che UNA torni in piedi perche' il tasto si spenga --------------------
	// Senza questa meta' il test non distinguerebbe la guardia da un `return false` costante.
	Caduta->Health = 100;
	const float Prima = TurnManager->ViewerPlaybackSpeed;
	PC->CyclePlaybackSpeed();
	TestTrue(TEXT("con la stessa unita' viva il tasto torna inerte"),
		FMath::IsNearlyEqual(TurnManager->ViewerPlaybackSpeed, Prima, 1e-3f));

	DestroyPlaybackSpeedWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
