// Copyright RefactorTactics. All Rights Reserved.

// Il budget di presentazione non accelera la locomozione — `#1878`.
//
// 🔑 **Perche' esiste un file d'integrazione invece di due asserzioni sulla library.** La library sa gia'
// dire che `SlackScaleForBudget` comprime lo slack e non tocca `Shown`: lo prova
// `RefactorTactics.Playback.SlackScaleForBudget`, senza mondo. Cio' che NESSUN test sapeva dire e' cosa si
// vede a schermo — quanto velocemente un cilindro attraversa le sue celle mentre il playback scorre. E'
// la grandezza che il product owner ha osservato il 2026-08-30 («sembrano andare in fast-forward»), ed era
// l'unica non misurata: la si ottiene solo tickando una risoluzione vera e guardando dove i modelli sono.
//
// ⚠️ **Cio' che questo file prova, e cio' che NON prova.** Prova che il rate dichiarato comanda la
// velocita' osservata, e che una fase che mostra qualcosa non viene toccata dal budget nemmeno passando
// per `DurationForPlaybackPhase` e per l'`Alpha` del tick. **Non** prova che la compressione dello slack
// funzioni: qui lo slack e' zero per costruzione — la risoluzione ha la sola fase `Move` — e la
// compressione e' esercitata a tavolino da `RefactorTactics.Playback.SlackScaleForBudget`. Detto qui
// perche' una prima stesura affermava il contrario in un commento, e la misura lo ha smentito.
//
// 🔴 **Il piano e' SCRITTO, non chiesto al bot, e la prima stesura di questo file sbagliava proprio qui.**
// Allestiva un autobattle su un'arena di raggio 4 e si aspettava del movimento: gli spawn distano 8 celle,
// il `VisionRange` piu' corto e' 5, quindi il bot non vedeva nessuno e le quattro unita' **restavano ferme**
// (`[RT] Gadget: resta (q=-4,r=0,L=0)`). E' il difetto che `#1738` descrive, incontrato di lato. La guardia
// «qualcosa si e' mosso» ha fatto cadere il test invece di lasciarlo passare confrontando due zeri — ed e'
// la ragione per cui quella guardia c'e' e resta.
//
// 📐 Sul difetto che questo file avrebbe colto: prima del 2026-09-02 `TickPlayback` faceva
// `Dt = DeltaSeconds * EffectivePlaybackSpeed(Viewer, CapSpeed)` e `Alpha = PlaybackPhaseElapsed /
// PhaseDur` con `PhaseDur` non compressa — il numeratore correva e il denominatore no, quindi il tetto
// accelerava i cilindri. Con budget stretto il primo caso avrebbe misurato una velocita' PIU' ALTA del
// secondo, e la riga sarebbe caduta.

#include "Misc/AutomationTest.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Map/RTCellId.h"
#include "Unit/RTUnit.h"
#include "Ability/RTHeroCatalogLibrary.h"
#include "Tests/RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Cosa una risoluzione ha mostrato. Nomi lunghi: il namespace anonimo entra nella unity build. */
	struct FRTPlaybackBudgetProbe
	{
		/** La velocita' PIU' ALTA a cui il modello si e' mosso, in celle al secondo. */
		float PeakCellsPerSecond = 0.f;
		/** Secondi di playback effettivamente ticcati. */
		float PlaybackSeconds = 0.f;
		/** Quanto il budget ha compresso le attese: 1 = per niente, 0 = tutto il comprimibile. */
		float SlackScale = 1.f;
		/** La risoluzione e' finita da sola invece di esaurire il budget di tick. */
		bool bFinished = false;
		/** Il modello si e' mosso davvero: senza, ogni confronto sarebbe fra zeri. */
		bool bMoved = false;
		/** Ha percorso TUTTI i passi pianificati: senza, la conversione in celle/s e' sbagliata. */
		bool bReachedEnd = false;
	};

	ARTUnit* RTSpawnBudgetProbeUnit(UWorld* World, int32 TeamId, const FRTCellId& Cell)
	{
		if (!World) { return nullptr; }
		ARTUnit* U = World->SpawnActorDeferred<ARTUnit>(ARTUnit::StaticClass(), FTransform::Identity);
		if (!U) { return nullptr; }
		U->TeamId = TeamId;
		U->bIsBotControlled = false; // il piano lo scriviamo noi: una misura non si chiede a un'euristica
		U->ConfigureFromHeroData(URTHeroCatalogLibrary::MakeWraith());
		UGameplayStatics::FinishSpawningActor(U, FTransform::Identity);
		U->PlaceOnCell(Cell, FVector::ZeroVector, /*CellSize=*/ 100.f, /*LayerHeight=*/ 250.f);
		// Senza, `PlannedCell` resta il default `(0,0,0)` — una cella VERA — e ogni unita' pianificherebbe
		// un movimento verso l'origine che nessuno ha chiesto.
		U->PlannedCell = Cell;
		return U;
	}

	/**
	 * Gira UN turno con un movimento SCRITTO e misura la velocita' visuale di picco, in celle al secondo.
	 *
	 * ⚠️ **Passo di tick FISSO**, e non e' un dettaglio: con un passo variabile la velocita' misurata
	 * porterebbe il rumore del frame rate e i due casi non sarebbero confrontabili. E' la stessa ragione
	 * per cui `Scenarios/AutoBattle/ArenaV01.json` documenta `-useFixedTimeStep -fps=60` per la riga di
	 * comando: una misura di presentazione vuole un orologio che non trema.
	 *
	 * ⚠️ **La conversione in celle NON usa una costante.** La distanza in mondo fra due celle adiacenti si
	 * ricava dal percorso stesso — spostamento totale diviso i passi fatti — perche' la scala del playback
	 * viene da `GetHexContext` e cablarla qui creerebbe una seconda verita' che diverge al primo asset con
	 * un `CellSize` diverso.
	 */
	FRTPlaybackBudgetProbe RTMeasureOneResolutionUnderBudget(
		float BaseCellsPerSecond, float MaxPlaybackSeconds, float ViewerSpeed)
	{
		FRTPlaybackBudgetProbe Probe;

		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World) { return Probe; }

		ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		if (!HexMap || !TM)
		{
			RTWorldFixtures::DestroyWorld(World);
			return Probe;
		}
		HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 6);

		// Un percorso lungo e in linea: cinque passi danno alla fase Move abbastanza durata da campionare,
		// e una retta rende la distanza per cella una divisione invece di una somma di segmenti.
		const TArray<FRTCellId> Path = {
			FRTCellId(0, 0), FRTCellId(1, 0), FRTCellId(2, 0),
			FRTCellId(3, 0), FRTCellId(4, 0), FRTCellId(5, 0)
		};
		const int32 Steps = Path.Num() - 1;

		ARTUnit* Mover = RTSpawnBudgetProbeUnit(World, /*TeamId=*/ 0, Path[0]);
		// Un avversario fermo e lontano: serve un secondo team perche' il turno sia una partita, non perche'
		// debba fare qualcosa. Fuori portata di proposito — un attacco aggiungerebbe una fase Blast e
		// cambierebbe la composizione che stiamo misurando.
		ARTUnit* Idle = RTSpawnBudgetProbeUnit(World, /*TeamId=*/ 1, FRTCellId(-6, 0));
		if (!Mover || !Idle)
		{
			RTWorldFixtures::DestroyWorld(World);
			return Probe;
		}

		Mover->PlannedWaypoints = { Path[5] };
		Mover->PlannedPath = Path;
		Mover->PlannedCell = Path[5];
		Mover->PlannedAbilityIndex = INDEX_NONE;

		// I tre parametri sotto misura. Il resto resta ai default: cambiarne altri renderebbe i due casi
		// diversi per piu' di una ragione, e la differenza non sarebbe piu' attribuibile.
		TM->PlaybackCellsPerSecond = BaseCellsPerSecond;
		TM->MaxPlaybackSeconds = MaxPlaybackSeconds;
		TM->ViewerPlaybackSpeed = ViewerSpeed;

		TM->LockInAndResolve();

		// Dopo `LockInAndResolve`: `BeginPlayback` ha gia' fatto lo snap sulla cella di partenza, e
		// campionare prima misurerebbe quello snap invece della locomozione.
		FVector Previous = Mover->GetActorLocation();
		const FVector Start = Previous;
		float PeakWorldSpeed = 0.f;
		float TotalTravelled = 0.f;

		const float Step = 1.f / 60.f;
		for (int32 I = 0; I < 2000 && TM->IsResolving(); ++I)
		{
			TM->Tick(Step);
			Probe.PlaybackSeconds += Step;

			const FVector Now = Mover->GetActorLocation();
			// Solo il piano: la quota cambia salendo di livello, e non e' velocita' di marcia.
			const float Moved = FVector::Dist2D(Now, Previous);
			TotalTravelled += Moved;
			PeakWorldSpeed = FMath::Max(PeakWorldSpeed, Moved / Step);
			Previous = Now;
		}

		Probe.bFinished = !TM->IsResolving();
		Probe.SlackScale = TM->GetPlaybackSlackScale();
		Probe.bMoved = TotalTravelled > UE_KINDA_SMALL_NUMBER;

		// 🔴 **La conversione in celle vale solo se il percorso e' stato percorso TUTTO**, e il budget di
		// movimento dell'eroe e' un ingresso che questo file non controlla. Se un cambio di bilanciamento
		// tagliasse il piano a tre passi su cinque, dividere per `Steps` gonfierebbe ogni celle/s del 66% e
		// il referto porterebbe numeri inventati sotto un'asserzione che parla di orologi. Si registra
		// dove l'unita' e' arrivata, e il test lo verifica prima di leggere le velocita'.
		Probe.bReachedEnd = (Mover->Cell == Path.Last());

		// Distanza per cella dal percorso stesso, non da una costante. Si usa lo spostamento NETTO fra
		// inizio e fine — su una retta e' esattamente `Steps` celle — invece del cammino accumulato, che
		// includerebbe l'eventuale assestamento finale.
		const float NetDistance = FVector::Dist2D(Mover->GetActorLocation(), Start);
		const float PerCell = (Steps > 0) ? (NetDistance / Steps) : 0.f;
		Probe.PeakCellsPerSecond = (PerCell > UE_KINDA_SMALL_NUMBER) ? (PeakWorldSpeed / PerCell) : 0.f;

		RTWorldFixtures::DestroyWorld(World);
		return Probe;
	}
}

/**
 * 🔑 **LA RIGA DELLA ISSUE.** Stessa risoluzione, due budget: uno che non morde e uno stretto al punto da
 * comprimere tutto il comprimibile. La velocita' con cui il modello attraversa le celle deve essere la
 * STESSA — il budget accorcia le attese, non fa correre nessuno.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackBudgetDoesNotSpeedUpLocomotionTest,
	"RefactorTactics.Playback.BudgetDoesNotSpeedUpLocomotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackBudgetDoesNotSpeedUpLocomotionTest::RunTest(const FString&)
{
	const float Base = 6.5f;

	// Budget larghissimo: non ha nulla da comprimere. E' la velocita' di riferimento.
	const FRTPlaybackBudgetProbe Loose = RTMeasureOneResolutionUnderBudget(Base, /*Max=*/ 999.f, 1.f);

	// Budget da un decimo di secondo: piu' stretto di qualunque risoluzione reale.
	const FRTPlaybackBudgetProbe Tight = RTMeasureOneResolutionUnderBudget(Base, /*Max=*/ 0.1f, 1.f);

	if (!TestTrue(TEXT("la risoluzione col budget largo finisce entro il budget di tick"), Loose.bFinished)
		|| !TestTrue(TEXT("la risoluzione col budget stretto finisce entro il budget di tick"), Tight.bFinished))
	{
		return false;
	}

	// ⚠️ Non e' pedanteria: la prima stesura di questo file falliva ESATTAMENTE qui, perche' allestiva un
	// autobattle in cui nessuno si muoveva. Senza questa riga sarebbe passata confrontando due zeri.
	if (!TestTrue(TEXT("qualcosa si e' mosso: senza, il confronto sarebbe fra due zeri"), Loose.bMoved)
		|| !TestTrue(TEXT("e si e' mosso anche col budget stretto"), Tight.bMoved))
	{
		return false;
	}

	// La conversione in celle/s divide per il numero di passi PIANIFICATI: se il piano non fosse stato
	// eseguito per intero, ogni numero sotto sarebbe gonfiato e l'errore parlerebbe di orologi.
	if (!TestTrue(TEXT("il piano e' stato percorso tutto (budget largo)"), Loose.bReachedEnd)
		|| !TestTrue(TEXT("il piano e' stato percorso tutto (budget stretto)"), Tight.bReachedEnd))
	{
		return false;
	}

	// 🔑 **LA PROVA DEL BUDGET SOFT.** Con 0,1 s di budget contro una locomozione che ne chiede 0,78, il
	// playback **sfora di quasi otto volte** e la velocita' non si muove di un decimale. Un budget hard
	// avrebbe accelerato i cilindri di 7,8x.
	TestTrue(*FString::Printf(TEXT("col budget stretto la durata SFORA invece di accelerare (%.2f s su 0,1 s)"),
		Tight.PlaybackSeconds),
		Tight.PlaybackSeconds > 0.2f);

	// ⚠️ **Detto per intero, perche' e' il limite di questo test.** Qui `SlackScale` vale `1` in ENTRAMBI i
	// casi, e non perche' il budget sia stato clemente: la risoluzione ha la sola fase `Move`, che e' tutta
	// `Shown`, quindi non esiste slack da comprimere e `SlackScaleForBudget` esce al primo guard.
	//
	// ∴ Il confronto qui sotto NON prova che la compressione funzioni — quella e' di
	// `RefactorTactics.Playback.SlackScaleForBudget`, che la esercita a tavolino. Prova una cosa diversa e
	// che nessun test puro puo' dire: che una fase che mostra qualcosa **non viene toccata dal budget**
	// nemmeno passando per `DurationForPlaybackPhase` e per l'`Alpha` del tick. Cadrebbe se qualcuno
	// rimettesse dello slack nel `Move`, o applicasse la scala a `Shown`.
	TestTrue(TEXT("il budget non ha slack da togliere: la fase Move e' tutta mostrata"),
		FMath::IsNearlyEqual(Tight.SlackScale, 1.f, 1e-3f)
		&& FMath::IsNearlyEqual(Loose.SlackScale, 1.f, 1e-3f));

	// 🔑 L'invariante. Tolleranza relativa: il picco e' campionato a passi discreti, e un tick che cade a
	// cavallo di un confine di fase puo' spostare l'ultimo campione di una frazione.
	const float Ratio = Tight.PeakCellsPerSecond / Loose.PeakCellsPerSecond;
	TestTrue(*FString::Printf(
		TEXT("il budget stretto non accelera la locomozione (largo %.2f celle/s, stretto %.2f celle/s, rapporto %.3f)"),
		Loose.PeakCellsPerSecond, Tight.PeakCellsPerSecond, Ratio),
		Ratio <= 1.02f);

	// E la locomozione va alla velocita' DICHIARATA, non a una qualunque: senza questa riga il test
	// resterebbe verde anche se entrambi i casi andassero al doppio del rate base, purche' insieme.
	TestTrue(*FString::Printf(TEXT("la velocita' osservata e' quella dichiarata (attesa %.2f, misurata %.2f celle/s)"),
		Base, Loose.PeakCellsPerSecond),
		FMath::IsNearlyEqual(Loose.PeakCellsPerSecond, Base, /*Tolerance=*/ 0.35f));

	UE_LOG(LogTemp, Display,
		TEXT("[RT][#1878] budget largo: %.2f celle/s, %.2f s, slack x%.2f | budget stretto: %.2f celle/s, %.2f s, slack x%.2f"),
		Loose.PeakCellsPerSecond, Loose.PlaybackSeconds, Loose.SlackScale,
		Tight.PeakCellsPerSecond, Tight.PlaybackSeconds, Tight.SlackScale);

	return true;
}

/**
 * Il referto comparativo che l'AC di #1878 chiede di allegare, prodotto invece che trascritto a mano.
 *
 * ⛔ **Non giudica**: la leggibilita' e il foot sliding non sono grandezze che un'automazione possa
 * decidere, e il default finale e' una firma del product owner. Questo test produce **i numeri su cui
 * quella firma si appoggia** — velocita' visuale reale, durata, compressione — per i valori che la issue
 * nomina. Una sessione PIE serve ancora, e serve per guardare, non per misurare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackBudgetSpeedReportTest,
	"RefactorTactics.Playback.LocomotionSpeedReportForTuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackBudgetSpeedReportTest::RunTest(const FString&)
{
	// I tre valori che #1878 chiede di provare, piu' il default in vigore come riferimento.
	const float Rates[] = { 6.5f, 2.00f, 1.65f, 1.35f };

	for (const float Rate : Rates)
	{
		const FRTPlaybackBudgetProbe P = RTMeasureOneResolutionUnderBudget(Rate, /*Max=*/ 12.f, 1.f);

		UE_LOG(LogTemp, Display,
			TEXT("[RT][#1878] rate dichiarato %.2f celle/s -> osservate %.2f celle/s, durata %.2f s, slack x%.2f"),
			Rate, P.PeakCellsPerSecond, P.PlaybackSeconds, P.SlackScale);

		if (!TestTrue(*FString::Printf(TEXT("a %.2f celle/s la risoluzione finisce"), Rate), P.bFinished)
			|| !TestTrue(*FString::Printf(TEXT("a %.2f celle/s il modello si muove"), Rate), P.bMoved)
			|| !TestTrue(*FString::Printf(TEXT("a %.2f celle/s il piano e' percorso tutto"), Rate), P.bReachedEnd))
		{
			return false;
		}

		// 🔑 La riga che rende credibile il referto: il numero che si sta tarando comanda davvero cio' che
		// si vede. Se il budget riprendesse il controllo, cadrebbe sul primo rate che lo fa mordere — ed e'
		// il modo in cui questo test resta utile anche dopo che il default sara' cambiato.
		TestTrue(*FString::Printf(
			TEXT("a %.2f celle/s dichiarate se ne osservano %.2f"), Rate, P.PeakCellsPerSecond),
			FMath::IsNearlyEqual(P.PeakCellsPerSecond, Rate, /*Tolerance=*/ 0.35f));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
