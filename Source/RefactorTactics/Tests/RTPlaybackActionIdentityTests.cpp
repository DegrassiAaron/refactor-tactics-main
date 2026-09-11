// L'IDENTITA' DELL'AZIONE SULLA TIMELINE DI PLAYBACK (`#2857`).
//
// 🔴 **Perche' serve un test CON MONDO, quando la regola del confine e' gia' coperta dai test puri.**
// `RTPlaybackLibraryTests.cpp` verifica che `NextActionBoundary` legga bene una timeline; qui si verifica
// che il dato ci **arrivi**. Sono due difetti diversi, e il secondo e' quello muto: `NAME_None` e' un
// valore legittimo — «nessuna azione dietro» — quindi un produttore che dimentica di copiare l'`ActionId`
// non fa fallire niente e fa saltare un atto a `Next Action` senza dirlo.
//
// 🔑 **L'oracolo e' il TurnLog, sullo STESSO turno.** Non un valore atteso scritto a mano nel test — che
// direbbe solo «il produttore fa quello che credevo» — ma l'altro canale, quello canonico, che l'azione la
// porta da `#79` e `#307`. Cio' che va escluso e' che le due fonti **divergano**: e' l'unica proprieta'
// che conta, perche' `#2857` copia il dato invece di ricalcolarlo proprio per non farle divergere mai.
//
// ⚠️ **`ResolvedTimeline` non entra in `StateHash` ne' nel formato di replay** — lo dichiarano i due siti
// di `AppendLogEntry` in `RTTurnManager.cpp` — quindi nessuna asserzione di questo file tocca golden,
// determinismo o archivi.

#include "Misc/AutomationTest.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTResolvedEvent.h"
#include "Turn/RTPlaybackLibrary.h" // NextActionBoundary: la regola letta su una timeline vera
#include "Unit/RTUnit.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"
#include "Ability/RTActionData.h"
#include "Kismet/GameplayStatics.h"
#include "RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// ⚠️ Nomi distinti per file: in unity build i test condividono la translation unit. Il mondo lo creano
	// e lo distruggono le fixture condivise (`RTWorldFixtures`); qui restano solo mappa e unita', che
	// quell'header non copre.

	URTHexMapAsset* SpawnActionIdentityMap(UWorld* World, int32 Radius = 12)
	{
		if (!World) { return nullptr; }
		URTHexMapAsset* M = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), Radius);
		ARTHexMapActor* Actor = World->SpawnActor<ARTHexMapActor>();
		Actor->MapAsset = M;
		return M;
	}

	ARTUnit* SpawnActionIdentityUnit(UWorld* World, int32 TeamId, const URTHeroData* Hero, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // i piani li scriviamo noi: niente decisioni del bot in mezzo
		U->ConfigureFromHeroData(Hero);
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		// Stessa geometria che il MapActor dichiara (HexSize 100, LayerHeight 250).
		U->PlaceOnCell(Cell, FVector::ZeroVector, 100.f, /*LayerHeight=*/ 250.f);
		return U;
	}

	/** Gli `ActionId` DISTINTI che le voci di TurnLog di una categoria dichiarano in questo turno. */
	TSet<FName> LogActionIdsOfCategory(const ARTTurnManager* TM, ERTLogCategory Category)
	{
		TSet<FName> Out;
		for (const FRTTurnLogEntry& Entry : TM->GetTurnLog())
		{
			if (Entry.Category == Category && !Entry.ActionId.IsNone())
			{
				Out.Add(Entry.ActionId);
			}
		}
		return Out;
	}

	/** Gli eventi della timeline di un tipo. */
	TArray<FRTResolvedEvent> TimelineEventsOfType(const ARTTurnManager* TM, ERTResolvedEventType Type)
	{
		TArray<FRTResolvedEvent> Out;
		for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
		{
			if (Ev.Type == Type)
			{
				Out.Add(Ev);
			}
		}
		return Out;
	}
}

/**
 * 🔴 **Il criterio d'accettazione di `#2857`**: su uno scenario con un movimento e un colpo d'eroe, ogni
 * evento che ha un'azione dietro la dichiara, e la dichiara **uguale** a quella del TurnLog.
 *
 * ⛔ **Le asserzioni di ASSENZA sono la meta' che conta di piu'.** Un test che verificasse solo «gli
 * `Attack` hanno un `ActionId`» sarebbe verde anche su un produttore che scrive lo stesso nome ovunque —
 * per esempio copiando ciecamente `Entry.ActionId` dentro `AppendLogEntry`, dove quel campo porta il tag
 * dello stato e la causa ambientale invece di un'azione. Per questo si verifica anche che `Defeated` e
 * `StatusChanged` portino `NAME_None`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackResolvedEventActionIdentityTest,
	"RefactorTactics.Playback.ResolvedEventCarriesTheActionIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackResolvedEventActionIdentityTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	SpawnActionIdentityMap(World);

	// Due unita' adiacenti: Branth colpisce, Ivrin incassa. Branth si muove ANCHE, cosi' lo stesso turno
	// produce un `Attack` (fase Blast) e un `Move` (fase Move, che risolve dopo il Blast).
	ARTUnit* Attaccante = SpawnActionIdentityUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Bersaglio  = SpawnActionIdentityUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(3, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { RTWorldFixtures::DestroyWorld(World); return false; }

	// L'azione che il piano dichiara: la si legge dal CATALOGO dell'unita', non da un letterale. Un nome
	// scritto a mano qui misurerebbe che il test e il produttore concordano, non che il produttore e il
	// TurnLog concordano.
	const URTActionData* Colpo = Attaccante->GetAbility(0);
	TestNotNull(TEXT("l'attaccante ha un'abilita' in slot 0"), Colpo);
	if (!Colpo) { RTWorldFixtures::DestroyWorld(World); return false; }
	const FName AzioneDelColpo = Colpo->Def.ActionId;

	Attaccante->PlannedAbilityIndex = 0;
	Attaccante->PlannedAttackTarget = Bersaglio;
	Attaccante->PlannedCell = FRTCellId(2, 3); // si sposta dopo aver colpito

	TM->LockInAndResolve();

	// --- 1. Il colpo -------------------------------------------------------------------------------
	const TArray<FRTResolvedEvent> Attacchi = TimelineEventsOfType(TM, ERTResolvedEventType::Attack);

	// ⛔ ANTI-VACUITA': senza un colpo a segno ogni asserzione sui colpi e' vera per assenza. Il test
	// FALLISCE invece di avvertire — un warning fra le centinaia di righe della suite non lo legge nessuno.
	if (!TestTrue(TEXT("⛔ il turno ha prodotto almeno un Attack"), Attacchi.Num() > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	const TSet<FName> AzioniDelLogCombat = LogActionIdsOfCategory(TM, ERTLogCategory::Combat);
	for (const FRTResolvedEvent& Ev : Attacchi)
	{
		TestFalse(TEXT("un Attack porta un'azione, mai NAME_None"), Ev.ActionId.IsNone());
		// 🔑 Il confronto con l'ALTRO canale: l'azione dell'evento e' fra quelle che il TurnLog dichiara
		// per il combattimento di questo turno. Se un produttore avesse inventato un nome, o ne avesse
		// copiato uno da un campo polimorfo, qui non ci sarebbe.
		TestTrue(TEXT("l'azione dell'Attack e' quella che il TurnLog dichiara"),
			AzioniDelLogCombat.Contains(Ev.ActionId));
		TestEqual(TEXT("ed e' l'azione pianificata, letta dal catalogo"), Ev.ActionId, AzioneDelColpo);
	}

	// --- 2. L'impronta a terra ---------------------------------------------------------------------
	// Ne esce UNA per intento anche quando il colpo non trova nessuno, quindi e' l'evento su cui
	// `Next Action` si ferma piu' naturalmente: se restasse senza azione, la granularita' migliore
	// sarebbe anche l'unica non indirizzabile.
	for (const FRTResolvedEvent& Ev : TimelineEventsOfType(TM, ERTResolvedEventType::AttackFootprint))
	{
		TestEqual(TEXT("l'impronta porta l'azione dell'intento che l'ha prodotta"),
			Ev.ActionId, AzioneDelColpo);
	}

	// --- 3. Il movimento ---------------------------------------------------------------------------
	const TArray<FRTResolvedEvent> Movimenti = TimelineEventsOfType(TM, ERTResolvedEventType::Move);
	if (TestTrue(TEXT("⛔ il turno ha prodotto almeno un Move"), Movimenti.Num() > 0))
	{
		const TSet<FName> AzioniDelLogMove = LogActionIdsOfCategory(TM, ERTLogCategory::Move);
		for (const FRTResolvedEvent& Ev : Movimenti)
		{
			TestFalse(TEXT("un Move porta la propria causa, mai NAME_None"), Ev.ActionId.IsNone());
			// 🔑 E' la verifica che la costante condivisa regge: `EmitMoveEvents` e `BuildMoveLog` leggono
			// lo stesso `MoveCauseActionId`, e questa riga cade se un giorno tornassero due letterali.
			TestTrue(TEXT("la causa del Move e' quella che il TurnLog dichiara"),
				AzioniDelLogMove.Contains(Ev.ActionId));
		}
	}

	// --- 4. ⛔ Cio' che NON deve avere un'azione ---------------------------------------------------
	// E' l'asserzione che distingue «copiato dalla fonte giusta» da «copiato da quella a portata di mano».
	for (const FRTResolvedEvent& Ev : TimelineEventsOfType(TM, ERTResolvedEventType::Defeated))
	{
		TestTrue(TEXT("⛔ un Defeated non ha un'azione dietro: NAME_None"), Ev.ActionId.IsNone());
	}
	for (const FRTResolvedEvent& Ev : TM->ResolvedTimelineForTest())
	{
		if (Ev.Type == ERTResolvedEventType::StatusChanged)
		{
			// Qui `FRTTurnLogEntry::ActionId` porta il TAG dello stato: chi lo copiasse in `ActionId`
			// darebbe a `Next Action` una fermata su una scadenza invece che su un atto. Lo stato continua
			// a viaggiare in `StatusTag`, che resta popolato.
			TestTrue(TEXT("⛔ uno StatusChanged non ha un'azione dietro: NAME_None"), Ev.ActionId.IsNone());
			TestFalse(TEXT("⛔ ...e il tag dello stato NON e' andato perduto"), Ev.StatusTag.IsNone());
		}
		if (Ev.Type == ERTResolvedEventType::HazardDamage)
		{
			// Stessa ragione: li' `Entry.ActionId` porta una CAUSA ambientale (`Terrain.*`, `Fall.*`), ed
			// e' su quel prefisso che `IsEnvironmentalDamage` riconosce la voce. Un rogo non e' un atto.
			TestTrue(TEXT("⛔ un HazardDamage non ha un'azione dietro: NAME_None"), Ev.ActionId.IsNone());
		}
	}

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

/**
 * 🔑 **Il campo serve a QUALCOSA, e questa e' la riga che lo dimostra.** I due test qui sopra e quelli
 * puri verificano rispettivamente che il dato arrivi e che la regola lo legga; nessuno dei due mostra i
 * due pezzi insieme. Senza questa saldatura `#2857` consegnerebbe un campo popolato e un confine
 * calcolabile, e nessuna prova che il secondo sappia leggere il primo su una timeline VERA.
 *
 * ⛔ Non e' il gate di `#2859`: qui non si stanno confrontando due esecuzioni. Si sta solo chiedendo alla
 * timeline di un turno reale dove sia il prossimo atto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackActionBoundaryOnARealTimelineTest,
	"RefactorTactics.Playback.ActionBoundaryWalksARealTimeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackActionBoundaryOnARealTimelineTest::RunTest(const FString&)
{
	UWorld* World = RTWorldFixtures::MakeWorld();
	TestNotNull(TEXT("World creato"), World);
	if (!World) { return false; }

	SpawnActionIdentityMap(World);

	ARTUnit* Attaccante = SpawnActionIdentityUnit(World, 0, URTHeroCatalogLibrary::MakeBranth(), FRTCellId(2, 2));
	ARTUnit* Bersaglio  = SpawnActionIdentityUnit(World, 1, URTHeroCatalogLibrary::MakeIvrin(),  FRTCellId(3, 2));
	ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
	if (!TM || !Attaccante || !Bersaglio) { RTWorldFixtures::DestroyWorld(World); return false; }

	Attaccante->PlannedAbilityIndex = 0;
	Attaccante->PlannedAttackTarget = Bersaglio;
	Attaccante->PlannedCell = FRTCellId(2, 3);

	TM->LockInAndResolve();

	const TArray<FRTResolvedEvent>& Timeline = TM->ResolvedTimelineForTest();
	if (!TestTrue(TEXT("⛔ la timeline non e' vuota"), Timeline.Num() > 0))
	{
		RTWorldFixtures::DestroyWorld(World);
		return false;
	}

	// Da «prima dell'inizio» si arriva al primo atto, e da li' si cammina di atto in atto. La successione
	// e' strettamente crescente e termina: due proprieta' che, insieme, escludono sia il ciclo infinito sia
	// la fermata sul posto — i due modi in cui un `Next Action` puo' rompersi.
	int32 Cursore = -1;
	int32 Atti = 0;
	for (int32 Guardia = 0; Guardia < Timeline.Num() + 2; ++Guardia)
	{
		const int32 Prossimo = URTPlaybackLibrary::NextActionBoundary(Timeline, Cursore);
		if (Prossimo >= Timeline.Num())
		{
			break; // fine timeline: non c'e' un altro atto
		}
		TestTrue(TEXT("ogni confine e' STRETTAMENTE dopo il precedente"), Prossimo > Cursore);
		TestFalse(TEXT("e ogni confine e' un evento che un'azione ce l'ha"),
			Timeline[Prossimo].ActionId.IsNone());
		Cursore = Prossimo;
		++Atti;
	}

	// ⛔ ANTI-VACUITA': se il campo non fosse popolato da nessun produttore, il ciclo qui sopra uscirebbe
	// al primo giro e ogni sua asserzione sarebbe vera per assenza. Un turno con un colpo e un movimento
	// contiene almeno un atto.
	TestTrue(TEXT("⛔ la camminata ha trovato almeno un atto"), Atti > 0);

	// L'ultimo confine porta alla fine e non oltre: e' la stessa scelta di `NextMicroStepBoundary`.
	TestEqual(TEXT("oltre l'ultimo atto si arriva alla fine della timeline"),
		URTPlaybackLibrary::NextActionBoundary(Timeline, Cursore), Timeline.Num());

	RTWorldFixtures::DestroyWorld(World);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
