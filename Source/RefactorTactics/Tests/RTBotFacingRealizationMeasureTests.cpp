// IL TASSO DI REALIZZO DEL BONUS DIREZIONALE DEL BOT (`#649`) — la seconda meta' di CP 16.2.
//
// Il bot conta un bonus quando colpisce un bersaglio fuori dal suo arco frontale, perche' li' la copertura
// non protegge. Il termine vale `WDamage x riduzione scavalcata` e **non e' stato tarato**: e' derivato, per
// non ripetere l'errore di scala che `#149` ha misurato. Il prezzo di quella scelta e' dichiarato nel codice
// del bot da allora — `RTHexBotLibrary.h`, nel commento di `EnemyFacings`:
//
//     «Chi attacca ruota verso il proprio bersaglio PRIMA che si valuti l'arco. Un nemico che attacca il bot
//      gli si gira contro, e il fianco che il bot aveva visto non c'e' piu'. […] La sovrastima che ne deriva
//      si misura sul TurnLog (`RearHitBypassedCover`), non si stima a priori.»
//
// Quella misura non era eseguibile: la traccia e' arrivata il 2026-08-12 (PR #660), ma il numero **stimato**
// moriva dentro `ScorePlan` sommato al punteggio. Questo file produce il rapporto fra i due.
//
// ⛔ **Misura, non giudica.** Nessuna asserzione confronta un tasso con una soglia, e non ne esiste una da
// difendere: `D-102` rende un risultato bot-contro-bot inammissibile come conclusione di bilanciamento
// finche' il bot non e' certificato sulle capability che lo producono (`#543`). Se domani il bot cambia e
// questi numeri cambiano, questi test **devono restare verdi**. Le sole asserzioni sono guardie di
// NON-VACUITA': che la board porti davvero coperture, che la partita sia stata giocata da soli bot, che il
// classificatore del danno risponda.
//
// ## Le due board, e perche' due
//
//   `…OnTheAuthoredMap`      la mappa d'autore, la board piu' realistica. Quante coperture porti non e'
//                            un'assunzione di questo file: si conta sull'asset caricato e si stampa.
//   `…OnACoveredArena`       un'arena piatta **satura di coperture basse**, costruita qui. Non e' una board
//                            di gioco: e' uno strumento che massimizza l'occasione. ∴ uno ZERO qui e' un
//                            risultato forte — il fenomeno non si presenta nemmeno dove abbonda — mentre un
//                            tasso **non e' generalizzabile** a una board d'autore.
//
// ## Cosa il rapporto NON dice
//
// ⚠️ **Il numeratore e il denominatore vivono in due istanti diversi, ed e' il punto.** La stima e' presa a
// pianificazione chiusa, il realizzato a risoluzione finita: fra i due c'e' la rotazione di chi attacca. Un
// tasso basso non dice «il bot sbaglia a contare», dice «il bonus contato non viene incassato» — che e'
// esattamente la domanda.
//
// ⚠️ **Due grandezze, una sola unita'.** Entrambi i lati sono PUNTI di riduzione: la stima li riporta cosi'
// (`ScorePlan` con `OutCoverBypassedByFacing`), e `Facing`/`RearHitBypassedCover` porta in `Amount` i punti
// scavalcati ([D-199]). Confrontare il termine di punteggio — `WDamage` volte tanto — darebbe un tasso dieci
// volte piu' grande, e sarebbe plausibile.
//
// ⚠️ **Il realizzato conta ogni voce `RearHitBypassedCover`, la stima solo i piani SCELTI** — e in una
// partita di soli bot le due popolazioni combaciano, per una ragione che va scritta perche' non e' ovvia.
// I colpi che POTREBBERO perdere la copertura per direzione fuori da `Plan.Hits` sono due famiglie
// — Overwatch in fuoco e boundary predittivo, le uniche due chiamanti di `BoundaryCoverReduction` — ed
// entrambe si armano da `PlannedAbilityIndex`/`bAttackTargetsCell`, che un bot non valorizza: `Action.Overwatch`
// dichiara `bSelfTarget` e i cicli di candidate saltano il self-target, e la predittiva vuole un bersaglio-CELLA
// che solo `ARTPlayerController` dichiara. Il contrattacco e' un colpo vero e non entra in nessuno dei due lati,
// ma non porta punti scavalcati: la copertura non gli viene applicata affatto.
//
// ∴ la guardia `UmaneViste == 0` non e' cerimoniale: e' la condizione che tiene in piedi questa coincidenza.
// Il giorno in cui una di quelle due famiglie emettesse la voce, a mancare non sarebbe il numeratore — non la
// conterebbe **nessuno dei due lati**, e il rapporto resterebbe sano descrivendo una popolazione piu' piccola
// di quella che il nome «ogni colpo» suggerisce.
//
// 🔴 **E' un rapporto fra due TOTALI, non un tasso di successo per evento — e la differenza cambia cosa si
// puo' dirne.** Il realizzato NON e' un sottoinsieme dello stimato: un colpo puo' scavalcare una copertura
// che il bot non aveva contato, perche' il bersaglio si e' girato verso qualcun ALTRO ed espone un fianco
// che in pianificazione non esponeva. I due insiemi si sovrappongono e nessuno dei due contiene l'altro.
// ∴ «50%» va letto come *«il bot ne ha contati 160, in partita ne sono stati scavalcati 80»*, mai come
// *«meta' dei bonus pianificati e' andata a segno»*: per quella servirebbe appaiare evento per evento, cioe'
// portare l'identita' del piano dentro la voce di TurnLog — un lavoro suo, e non questo.
//
// ## LA MISURA — 2026-09-20, clone `refactor-tactics-designer`, base `origin/main = 5958dfbc`
//
//     board              coperture basse   turni   punti STIMATI   punti REALIZZATI   voci
//     arena satura             306          12       160 (11/12)         80            8
//     mappa d'autore             0          12         0 (0/12)           0            0   <- CAMPIONE DEGENERE
//
// **Arena satura — il primo numero che questo repository abbia sulla domanda: 80 su 160.** Il bot ha contato
// 160 punti di copertura scavalcata decidendo, e in partita ne sono stati scavalcati 80. La partita e'
// arrivata al `RoundLimit` di `Format.Skirmish2v2` (12) e la stima e' stata non nulla in 11 turni su 12:
// non e' un campione di un turno fortunato.
//
// 🔴 **La mappa d'autore non ha NESSUNA copertura — zero basse e zero alte — e non e' un dettaglio della
// board: e' la ragione per cui questa misura non esisteva.** `DA_HexMap_Arena` e' l'unica board d'autore su
// cui girano partite headless, e su di lei il fenomeno non puo' presentarsi. ⚠️ La tabella dei nomi del
// pacchetto (`python tools/uasset/names.py … | grep -i cover`) elenca `Covers` e `RTHexCover`: sono i nomi
// dei TIPI, e leggerli come la presenza di istanze e' precisamente l'errore che questo conteggio a runtime
// ha corretto.
//
// ∴ chi legge il `50%` deve tenere insieme due cose: che viene da uno **strumento** e non da una board di
// gioco, e che e' un rapporto fra totali (il riquadro qui sopra). Cio' che il numero stabilisce senza
// ambiguita' e' l'esistenza del fenomeno — la sovrastima dichiarata in `RTHexBotLibrary.h` non e' teorica —
// e che il canale per misurarla ora c'e'.

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTCellId.h"                // ERTHexDirection: i sei bordi si enumerano, non si nominano
#include "Map/RTHexCellData.h"
#include "Map/RTHexCoverLibrary.h"       // AddCover: la via di PRODUZIONE, che rifiuta il bordo doppio
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "RTAuthoredArenaForTest.h"
#include "RTConsoleVariableGuardForTest.h"
#include "RTGameMode.h"
#include "RTWorldFixtures.h"
#include "Turn/RTMatchSetupLibrary.h"    // MakeFlatArena: il foglio bianco su cui posare le coperture
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"       // IsDamageInflictedByActor: «armato» si chiede al predicato
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

extern TAutoConsoleVariable<int32> CVarRTAutobattle;
extern TAutoConsoleVariable<float> CVarRTPlanningSeconds;

/**
 * Namespace NOMINATO e non anonimo: sotto unity build due helper omonimi in namespace anonimi collidono, ed
 * e' il difetto che `Meta.AnonymousHelpersDoNotCollideUnderUnity` sorveglia.
 */
namespace RTRealizzoDirezionale
{
	/** Quanti punti il bot si aspettava di scavalcare, e quanti ne ha scavalcati davvero. */
	struct FConteggi
	{
		/** Somma, turno per turno, della stima dei piani SCELTI. */
		int32 PuntiStimati = 0;
		/** Somma degli `Amount` delle voci `Facing`/`RearHitBypassedCover` risolte. */
		int32 PuntiRealizzati = 0;
		/** Quante voci hanno prodotto quei punti: separa «pochi colpi grossi» da «molti piccoli». */
		int32 VociRealizzate = 0;
		/** In quanti turni il bot ha contato almeno un punto: distingue «mai» da «una volta sola». */
		int32 TurniConStima = 0;

		/** Guardie di non-vacuita', non soglie. Sono VOCI di log, non turni: il nome lo dice. */
		int32 VociDiDanno = 0;
		int32 UmaneViste = 0;
		int32 Osservazioni = 0;
	};

	/**
	 * Le coperture dichiarate dalla cella stessa, separate per tipo.
	 *
	 * 🔑 **Le ALTE si contano anche se non servono, ed e' il punto.** Solo la bassa riduce il danno
	 * (`HexCoverDamageReduction`), quindi solo lei puo' essere scavalcata; ma «zero coperture basse» e
	 * «zero coperture» sono due diagnosi diverse per chi legge il referto, e la seconda si corregge
	 * posando muretti mentre la prima puo' voler dire che la board ha solo barriere alte.
	 *
	 * ⚠️ Guarda **la faccia dichiarata dalla cella**, che e' l'unica che il calcolo del danno legge: una
	 * barriera dichiarata dal vicino ripara quel vicino, non questa cella.
	 */
	void ConteggiaCoperture(const URTHexMapAsset* Map, int32& OutBasse, int32& OutAlte)
	{
		OutBasse = 0;
		OutAlte = 0;
		if (!Map) { return; }
		for (const FRTHexCellData& Cella : Map->Cells)
		{
			for (const FRTHexCover& Copertura : Cella.Covers)
			{
				if (Copertura.Type == ERTHexCoverType::Low) { ++OutBasse; }
				else if (Copertura.Type == ERTHexCoverType::High) { ++OutAlte; }
			}
		}
	}

	/**
	 * Posa una copertura bassa su OGNI bordo di OGNI cella, in ordine deterministico.
	 *
	 * ⚠️ **Circa meta' dei tentativi viene rifiutata, ed e' corretto**: `AddCover` nega il bordo gia'
	 * dichiarato dall'altra faccia, quindi ogni bordo interno resta riparato una volta sola e la cella che
	 * lo dichiara e' la prima che lo incontra. L'esito e' una board in cui ogni cella e' coperta **da alcune
	 * direzioni e non da altre**: non e' un difetto della posa, e' cio' che rende la board un campione
	 * invece di un caso limite uniforme.
	 *
	 * ⛔ `Low` e non `High`: solo la bassa riduce il danno (`HexCoverDamageReduction`), e l'alta toglierebbe
	 * la linea di tiro — misurerebbe l'assenza di un produttore che funziona.
	 */
	int32 SaturaDiCopertureBasse(URTHexMapAsset* Map)
	{
		if (!Map) { return 0; }
		int32 Posate = 0;
		// Copia degli id prima di modificare: `AddCover` passa da `AddOrUpdateCell`, che riordina l'array.
		TArray<FRTCellId> Ids;
		Ids.Reserve(Map->Cells.Num());
		for (const FRTHexCellData& Cella : Map->Cells) { Ids.Add(Cella.Id); }

		for (const FRTCellId& Id : Ids)
		{
			for (int32 D = 0; D < 6; ++D)
			{
				if (URTHexCoverLibrary::AddCover(Map, Id, static_cast<ERTHexDirection>(D),
					ERTHexCoverType::Low, FRTHexCover::DefaultIntegrity(ERTHexCoverType::Low)))
				{
					++Posate;
				}
			}
		}
		return Posate;
	}

	TArray<ARTUnit*> UnitaViveInCampo(UWorld* World)
	{
		TArray<AActor*> Trovati;
		UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Trovati);
		TArray<ARTUnit*> Vive;
		for (AActor* A : Trovati)
		{
			ARTUnit* U = Cast<ARTUnit>(A);
			if (U && U->IsAlive()) { Vive.Add(U); }
		}
		return Vive;
	}

	/**
	 * Gioca la partita gia' allestita e riempie i conteggi. Ritorna i turni giocati, o `-1` se un turno pende.
	 *
	 * 🔴 **La stima si legge PRIMA di risolvere e il realizzato DOPO, dentro lo stesso giro.** Il
	 * `TurnLog` viene azzerato da `LockInAndResolve`, quindi un conteggio fatto a partita finita vedrebbe
	 * l'ultimo turno e quasi sempre zero — restando verde. E la stima appartiene alla pianificazione di
	 * questo turno: `ConcludeTurn` la riscrive per il turno dopo.
	 */
	int32 Gioca(FAutomationTestBase& Test, UWorld* World, ARTTurnManager* TM, FConteggi& C, int32 MaxTurni,
		bool bPianificaEsplicito)
	{
		int32 Turni = 0;
		while (TM->GetPhase() != ERTMatchPhase::MatchEnded && Turni < MaxTurni)
		{
			if (bPianificaEsplicito) { TM->PlanBotsForTest(); }

			const int32 StimaDelTurno = TM->GetBotPlannedCoverBypassedByFacing();
			C.PuntiStimati += StimaDelTurno;
			if (StimaDelTurno > 0) { ++C.TurniConStima; }

			TM->LockInAndResolve();
			for (int32 I = 0; I < 400 && TM->IsResolving(); ++I) { TM->Tick(0.05f); }
			if (!Test.TestFalse(*FString::Printf(TEXT("il turno %d ha finito di risolvere entro 400 tick"),
				Turni + 1), TM->IsResolving()))
			{
				return -1;
			}
			++Turni;

			// Una passata sola sul log del turno appena risolto: il realizzato e la guardia del danno.
			for (const FRTTurnLogEntry& Voce : TM->GetTurnLog())
			{
				if (Voce.Category == ERTLogCategory::Facing
					&& Voce.Outcome == static_cast<uint8>(ERTFacingOutcome::RearHitBypassedCover))
				{
					C.PuntiRealizzati += Voce.Amount;
					++C.VociRealizzate;
				}
				if (URTTurnLogLibrary::IsDamageInflictedByActor(Voce)) { ++C.VociDiDanno; }
			}

			for (const ARTUnit* U : UnitaViveInCampo(World))
			{
				++C.Osservazioni;
				if (!U->bIsBotControlled) { ++C.UmaneViste; }
			}
		}
		return Turni;
	}

	/** Il referto. Nessun giudizio: i numeri, e cio' che li rende leggibili. */
	void Riporta(FAutomationTestBase& Test, const TCHAR* Board, const FConteggi& C, int32 Turni,
		int32 CopertureBasse, int32 CopertureAlte)
	{
		Test.AddInfo(FString::Printf(
			TEXT("%s — %d turni giocati; coperture sulla board: %d basse, %d alte (solo la bassa riduce)"),
			Board, Turni, CopertureBasse, CopertureAlte));
		Test.AddInfo(FString::Printf(
			TEXT("%s — punti STIMATI dai piani scelti: %d (in %d turni su %d)"),
			Board, C.PuntiStimati, C.TurniConStima, Turni));
		Test.AddInfo(FString::Printf(
			TEXT("%s — punti REALIZZATI nel TurnLog: %d in %d voci `Facing/RearHitBypassedCover`"),
			Board, C.PuntiRealizzati, C.VociRealizzate));

		if (C.PuntiStimati > 0)
		{
			// Per mille, perche' l'invariante #4 vieta i float nella decisione e qui non serve un'eccezione.
			const int32 PerMille = (C.PuntiRealizzati * 1000) / C.PuntiStimati;
			Test.AddInfo(FString::Printf(TEXT("%s — TASSO DI REALIZZO: %d.%d%% (%d su %d punti)"),
				Board, PerMille / 10, PerMille % 10, C.PuntiRealizzati, C.PuntiStimati));
		}

		// 🔴 Le righe che impediscono di leggere uno zero per cio' che non e'. Sono tre zeri diversi.
		if (CopertureBasse == 0)
		{
			Test.AddInfo(FString::Printf(
				TEXT("⚠ CAMPIONE DEGENERE: la board non ha NESSUNA copertura bassa (ne porta %d alte, che ")
				TEXT("bloccano la linea di tiro e non riducono il danno). Lo zero dice che non c'era niente ")
				TEXT("da scavalcare, NON che il bot non sovrastima: la domanda non e' stata posta."),
				CopertureAlte));
		}
		else if (C.PuntiStimati == 0)
		{
			Test.AddInfo(TEXT("⚠ CAMPIONE DEGENERE per questa domanda: le coperture c'erano, ma nessun ")
				TEXT("piano SCELTO ha mai contato un punto da scavalcare — nessun bersaglio riparato e' ")
				TEXT("mai stato preso di fianco in pianificazione. Il tasso non e' definito: e' 0/0."));
		}
		else if (C.PuntiRealizzati == 0)
		{
			Test.AddInfo(TEXT("🔴 Il bot ha contato punti e non ne ha incassato nessuno: e' la sovrastima ")
				TEXT("che `EnemyFacings` dichiara — chi attacca ruota verso il bersaglio prima che si ")
				TEXT("valuti l'arco. E' un risultato, non un campione degenere."));
		}
	}
}

/**
 * **Il tasso di realizzo sulla MAPPA D'AUTORE** — la board piu' realistica di cui il repository dispone.
 *
 * ⚠️ **Quante coperture porti e' una MISURA di questo test, non una sua premessa.** L'asset e' binario e
 * nessun test esistente asserisce le sue coperture: il numero si conta sull'asset caricato e si stampa. Se
 * e' zero, il referto lo dichiara campione degenere invece di pubblicare uno `0/0` come se fosse un tasso.
 *
 * Si specchia `NobodyParksOnTheAuthoredMap`: 12 turni, nessuna pianificazione esplicita. Pilotare una board
 * diversamente dal suo oracolo misura un'altra partita — e' la lezione di `#1551`, scritta in
 * `RTStallDefinitionMeasureTests.cpp`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotFacingRealizationAuthoredMapTest,
	"RefactorTactics.Bot.FacingBypassRealizationOnTheAuthoredMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotFacingRealizationAuthoredMapTest::RunTest(const FString&)
{
	URTHexMapAsset* Authored = RTAuthoredArena::Load();
	if (!TestNotNull(TEXT("la mappa d'autore si carica"), Authored)) { return false; }

	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	RTTestConsoleVariable::TGuardia<int32> ModalitaGuard{ *CVarRTAutobattle.AsVariable() };
	RTTestConsoleVariable::TGuardia<float> PianificazioneGuard{ *CVarRTPlanningSeconds.AsVariable() };

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	HexMap->MapAsset = Authored;
	GameMode->MapSource = ERTMapSource::LevelAsset;
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	int32 CopertureBasse = 0;
	int32 CopertureAlte = 0;
	RTRealizzoDirezionale::ConteggiaCoperture(Authored, CopertureBasse, CopertureAlte);

	RTRealizzoDirezionale::FConteggi C;
	const int32 Turni = RTRealizzoDirezionale::Gioca(*this, World, TM, C, /*MaxTurni*/ 12,
		/*bPianificaEsplicito*/ false);
	if (Turni < 0) { RTWorldFixtures::DestroyWorld(World); return false; }

	RTRealizzoDirezionale::Riporta(*this, TEXT("mappa d'autore"), C, Turni, CopertureBasse, CopertureAlte);

	TestTrue(FString::Printf(TEXT("premessa: la partita e' stata giocata (%d turni, %d osservazioni)"),
		Turni, C.Osservazioni), Turni > 0 && C.Osservazioni > 0);
	TestTrue(FString::Printf(TEXT("il classificatore del danno risponde: %d voci di danno inflitto"),
		C.VociDiDanno), C.VociDiDanno > 0);
	TestEqual(FString::Printf(TEXT("nessuna unita' umana in campo (%d viste in %d osservazioni)"),
		C.UmaneViste, C.Osservazioni), C.UmaneViste, 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * **Il tasso di realizzo su un'arena SATURA DI COPERTURE** — lo strumento, non una board di gioco.
 *
 * 🔑 **Esiste perche' sulle board generate il fenomeno non puo' presentarsi.** `MakeFlatArena` costruisce
 * celle default — `Covers` vuoto — e `MakeTestArena` non dichiara nessuna copertura di bordo: su quelle
 * board la riduzione nominale e' zero, quindi non c'e' niente da scavalcare e lo zero non e' informativo.
 * Qui l'occasione e' massimizzata per costruzione.
 *
 * ⛔ **Un tasso letto qui non e' generalizzabile.** La densita' di coperture non e' quella di nessuna board
 * di gioco. Cio' che questo banco puo' dire e' asimmetrico, e va letto solo in un verso: uno **zero** e'
 * forte — il bonus non si realizza nemmeno dove la copertura abbonda — mentre un tasso alto direbbe solo
 * che con abbastanza coperture qualcosa si incassa.
 *
 * Si specchia `EngagesOnTheGeneratedTestArena`: tetto a 40 turni e pianificazione esplicita.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTBotFacingRealizationCoveredArenaTest,
	"RefactorTactics.Bot.FacingBypassRealizationOnACoveredArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTBotFacingRealizationCoveredArenaTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	if (!TestNotNull(TEXT("mondo di prova"), World)) { return false; }

	RTTestConsoleVariable::TGuardia<int32> ModalitaGuard{ *CVarRTAutobattle.AsVariable() };
	RTTestConsoleVariable::TGuardia<float> PianificazioneGuard{ *CVarRTPlanningSeconds.AsVariable() };
	ModalitaGuard.Imposta(-1); // la modalita' la decide la PROPRIETA' del GameMode, non la CVar

	URTHexMapAsset* Arena = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius*/ 5);
	if (!TestNotNull(TEXT("arena piatta"), Arena)) { RTWorldFixtures::DestroyWorld(World); return false; }

	const int32 Posate = RTRealizzoDirezionale::SaturaDiCopertureBasse(Arena);
	int32 CopertureBasse = 0;
	int32 CopertureAlte = 0;
	RTRealizzoDirezionale::ConteggiaCoperture(Arena, CopertureBasse, CopertureAlte);

	// 🔴 La premessa del banco, asserita e non assunta: senza coperture questo test misurerebbe la stessa
	// assenza delle board generate, restando verde e sembrando una misura.
	if (!TestTrue(FString::Printf(TEXT("la board porta coperture basse (%d posate, %d lette)"),
		Posate, CopertureBasse), CopertureBasse > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
	if (!TestNotNull(TEXT("GameMode"), GameMode) || !TestNotNull(TEXT("TurnManager"), TM)
		|| !TestNotNull(TEXT("mappa"), HexMap))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	HexMap->MapAsset = Arena;
	GameMode->MapSource = ERTMapSource::LevelAsset;
	GameMode->bAutobattle = true;
	GameMode->SetupHexMatch(HexMap);

	RTRealizzoDirezionale::FConteggi C;
	const int32 Turni = RTRealizzoDirezionale::Gioca(*this, World, TM, C, /*MaxTurni*/ 40,
		/*bPianificaEsplicito*/ true);
	if (Turni < 0) { RTWorldFixtures::DestroyWorld(World); return false; }

	RTRealizzoDirezionale::Riporta(*this, TEXT("arena satura"), C, Turni, CopertureBasse, CopertureAlte);

	TestTrue(FString::Printf(TEXT("premessa: la partita e' stata giocata (%d turni, %d osservazioni)"),
		Turni, C.Osservazioni), Turni > 0 && C.Osservazioni > 0);
	TestTrue(FString::Printf(TEXT("il classificatore del danno risponde: %d voci di danno inflitto"),
		C.VociDiDanno), C.VociDiDanno > 0);
	TestEqual(FString::Printf(TEXT("nessuna unita' umana in campo (%d viste in %d osservazioni)"),
		C.UmaneViste, C.Osservazioni), C.UmaneViste, 0);

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
