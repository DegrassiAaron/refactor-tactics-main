// Copyright RefactorTactics. All Rights Reserved.

// Il budget di presentazione non accelera la locomozione — `#1878`.
//
// 🔑 **Perche' esiste un file d'integrazione invece di due asserzioni sulla library.** La library sa gia'
// dire che `SlackScaleForBudget` comprime lo slack e non tocca `Locomotion`: lo prova
// `RefactorTactics.Playback.SlackScaleForBudget`, senza mondo. Cio' che NESSUN test sapeva dire e' cosa si
// vede a schermo — quanto velocemente un cilindro attraversa le sue celle mentre il playback scorre. E'
// la grandezza che il product owner ha osservato il 2026-08-30 («sembrano andare in fast-forward»), ed era
// l'unica non misurata: la si ottiene solo tickando una risoluzione vera e guardando dove i modelli sono.
//
// ⚠️ **L'invariante e' un CONFRONTO, non un valore assoluto**, ed e' deliberato. Un test scritto come
// «la velocita' osservata vale 6,5 celle/s» morirebbe al primo cambio di default — e il default sta per
// cambiare, e' meta' dello scope di #1878. Il test qui confronta la STESSA risoluzione sotto due budget:
// se il budget influenzasse la velocita' della locomozione, i due numeri divergerebbero. E' vero prima e
// dopo la taratura, ed e' falso esattamente nel caso che la issue esiste per togliere.
//
// 📐 Sul difetto che questo file avrebbe colto: prima del 2026-09-02 `TickPlayback` faceva
// `Dt = DeltaSeconds * EffectivePlaybackSpeed(Viewer, CapSpeed)` e `Alpha = PlaybackPhaseElapsed /
// PhaseDur` con `PhaseDur` non compressa — il numeratore correva e il denominatore no, quindi il tetto
// accelerava i cilindri. Con budget stretto il primo caso avrebbe misurato una velocita' PIU' ALTA del
// secondo, e la riga sarebbe caduta.

#include "Misc/AutomationTest.h"
#include "EngineUtils.h"
#include "Turn/RTTurnManager.h"
#include "Turn/RTPlaybackLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Turn/RTMatchSetupLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Unit/RTUnit.h"
#include "RTGameMode.h"
#include "Tests/RTWorldFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Cosa una risoluzione ha mostrato. Nomi lunghi: il namespace anonimo entra nella unity build. */
	struct FRTPlaybackBudgetProbe
	{
		/** La velocita' PIU' ALTA a cui un modello si e' mosso, in unita' mondo al secondo. */
		float PeakVisualSpeed = 0.f;
		/** Secondi di playback effettivamente ticcati. */
		float PlaybackSeconds = 0.f;
		/** Quanto il budget ha compresso le attese: 1 = per niente, 0 = tutto il comprimibile. */
		float SlackScale = 1.f;
		/** La risoluzione e' finita da sola invece di esaurire il budget di tick. */
		bool bFinished = false;
	};

	/**
	 * Gira UN turno di autobattle e misura la velocita' visuale di picco.
	 *
	 * ⚠️ **Passo di tick FISSO**, e non e' un dettaglio: con un passo variabile la velocita' misurata
	 * porterebbe il rumore del frame rate e i due casi non sarebbero confrontabili. E' la stessa ragione
	 * per cui `Scenarios/AutoBattle/ArenaV01.json` documenta `-useFixedTimeStep -fps=60` per la riga di
	 * comando: una misura di presentazione vuole un orologio che non trema.
	 */
	FRTPlaybackBudgetProbe RTMeasureOneResolutionUnderBudget(
		float BaseCellsPerSecond, float MaxPlaybackSeconds, float ViewerSpeed)
	{
		FRTPlaybackBudgetProbe Probe;

		UWorld* World = RTWorldFixtures::MakeWorld();
		if (!World) { return Probe; }

		ARTHexMapActor* HexMap = World->SpawnActor<ARTHexMapActor>();
		ARTTurnManager* TM = World->SpawnActor<ARTTurnManager>(ARTTurnManager::StaticClass());
		ARTGameMode* GameMode = World->SpawnActor<ARTGameMode>();
		if (!HexMap || !TM || !GameMode)
		{
			RTWorldFixtures::DestroyWorld(World);
			return Probe;
		}

		HexMap->MapAsset = URTMatchSetupLibrary::MakeFlatArena(GetTransientPackage(), /*Radius=*/ 4);
		GameMode->bAutobattle = true;
		GameMode->SetupHexMatch(HexMap);

		// I tre parametri sotto misura. Il resto resta ai default: cambiarne altri renderebbe i due casi
		// diversi per piu' di una ragione, e la differenza non sarebbe piu' attribuibile.
		TM->PlaybackCellsPerSecond = BaseCellsPerSecond;
		TM->MaxPlaybackSeconds = MaxPlaybackSeconds;
		TM->ViewerPlaybackSpeed = ViewerSpeed;

		// Posizioni prima del primo tick: senza, il primo campione misurerebbe lo SNAP iniziale che
		// `BeginPlayback` fa con `SetVisualLocation(Anim.World[0])`, che non e' locomozione.
		TM->LockInAndResolve();

		TMap<TWeakObjectPtr<ARTUnit>, FVector> Previous;
		for (TActorIterator<ARTUnit> It(World); It; ++It)
		{
			Previous.Add(*It, It->GetActorLocation());
		}

		const float Step = 1.f / 60.f;
		for (int32 I = 0; I < 1200 && TM->IsResolving(); ++I)
		{
			TM->Tick(Step);
			Probe.PlaybackSeconds += Step;

			for (TActorIterator<ARTUnit> It(World); It; ++It)
			{
				ARTUnit* Unit = *It;
				const FVector Now = Unit->GetActorLocation();
				if (const FVector* Before = Previous.Find(Unit))
				{
					// Solo il piano: la quota cambia salendo di livello, e non e' velocita' di marcia.
					const float Moved = FVector::Dist2D(Now, *Before);
					Probe.PeakVisualSpeed = FMath::Max(Probe.PeakVisualSpeed, Moved / Step);
				}
				Previous.Add(Unit, Now);
			}
		}

		Probe.bFinished = !TM->IsResolving();
		Probe.SlackScale = TM->GetPlaybackSlackScale();

		RTWorldFixtures::DestroyWorld(World);
		return Probe;
	}
}

/**
 * 🔑 **LA RIGA DELLA ISSUE.** Stessa risoluzione, due budget: uno che non morde e uno stretto al punto da
 * comprimere tutto il comprimibile. La velocita' con cui i modelli attraversano il mondo deve essere la
 * STESSA — il budget accorcia le attese, non fa correre nessuno.
 *
 * ⚠️ Se il turno si risolve senza che nulla si muova, il confronto sarebbe fra due zeri e passerebbe
 * dicendo niente. Si asserisce che il movimento ci sia stato, invece di lasciarlo sperare.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackBudgetDoesNotSpeedUpLocomotionTest,
	"RefactorTactics.Playback.BudgetDoesNotSpeedUpLocomotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackBudgetDoesNotSpeedUpLocomotionTest::RunTest(const FString&)
{
	const float Base = 6.5f;

	// Budget larghissimo: non ha nulla da comprimere. E' la velocita' di riferimento.
	const FRTPlaybackBudgetProbe Loose = RTMeasureOneResolutionUnderBudget(Base, /*Max=*/ 999.f, 1.f);

	// Budget da mezzo secondo: piu' stretto di qualunque risoluzione reale, quindi lo slack va a zero e il
	// caso peggiore e' esercitato davvero invece di essere descritto.
	const FRTPlaybackBudgetProbe Tight = RTMeasureOneResolutionUnderBudget(Base, /*Max=*/ 0.5f, 1.f);

	if (!TestTrue(TEXT("la risoluzione col budget largo finisce entro il budget di tick"), Loose.bFinished)
		|| !TestTrue(TEXT("la risoluzione col budget stretto finisce entro il budget di tick"), Tight.bFinished))
	{
		return false;
	}

	if (!TestTrue(TEXT("qualcosa si e' mosso: senza, il confronto sarebbe fra due zeri"),
		Loose.PeakVisualSpeed > UE_KINDA_SMALL_NUMBER))
	{
		return false;
	}

	// Il budget stretto DEVE aver morso, o il caso peggiore non e' stato esercitato e il test passerebbe
	// per la ragione sbagliata.
	TestTrue(TEXT("il budget da 0,5 s ha compresso tutto il comprimibile"),
		Tight.SlackScale < 1.f - 1e-3f);
	TestTrue(TEXT("il budget largo non ha compresso nulla"),
		FMath::IsNearlyEqual(Loose.SlackScale, 1.f, 1e-3f));

	// 🔑 L'invariante. Tolleranza relativa: il picco e' campionato a passi discreti, e un tick che cade a
	// cavallo di un confine di fase puo' spostare l'ultimo campione di una frazione.
	const float Ratio = Tight.PeakVisualSpeed / Loose.PeakVisualSpeed;
	TestTrue(*FString::Printf(
		TEXT("il budget stretto non accelera la locomozione (largo %.1f u/s, stretto %.1f u/s, rapporto %.3f)"),
		Loose.PeakVisualSpeed, Tight.PeakVisualSpeed, Ratio),
		Ratio <= 1.02f);

	// E il referto, che e' meta' del valore di questo test: i numeri che l'AC di #1878 chiede di allegare.
	UE_LOG(LogTemp, Display,
		TEXT("[RT][#1878] budget largo: picco %.1f u/s, %.2f s, slack x%.2f | budget stretto: picco %.1f u/s, %.2f s, slack x%.2f"),
		Loose.PeakVisualSpeed, Loose.PlaybackSeconds, Loose.SlackScale,
		Tight.PeakVisualSpeed, Tight.PlaybackSeconds, Tight.SlackScale);

	return true;
}

/**
 * Il referto comparativo che l'AC di #1878 chiede di allegare, prodotto invece che trascritto a mano.
 *
 * ⛔ **Non giudica**: la leggibilita' e il foot sliding non sono grandezze che un'automazione possa
 * decidere, e il default finale e' una firma del product owner. Questo test produce **i numeri su cui
 * quella firma si appoggia** — velocita' visuale reale, durata, compressione — per i valori che la issue
 * nomina. Una sessione PIE serve ancora, e serve per guardare, non per misurare.
 *
 * ⚠️ La sola asserzione e' quella che rende i numeri credibili: se la velocita' visuale non scalasse col
 * rate base, il referto sarebbe una tabella di numeri scollegati da cio' che si sta tarando.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlaybackBudgetSpeedReportTest,
	"RefactorTactics.Playback.LocomotionSpeedReportForTuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTPlaybackBudgetSpeedReportTest::RunTest(const FString&)
{
	// I tre valori che #1878 chiede di provare, piu' il default in vigore come riferimento.
	const float Rates[] = { 6.5f, 2.00f, 1.65f, 1.35f };

	float PreviousPeak = -1.f;
	for (const float Rate : Rates)
	{
		const FRTPlaybackBudgetProbe P = RTMeasureOneResolutionUnderBudget(Rate, /*Max=*/ 12.f, 1.f);

		UE_LOG(LogTemp, Display,
			TEXT("[RT][#1878] rate %.2f celle/s -> picco visuale %.1f u/s, durata %.2f s, slack x%.2f"),
			Rate, P.PeakVisualSpeed, P.PlaybackSeconds, P.SlackScale);

		if (!TestTrue(*FString::Printf(TEXT("a %.2f celle/s la risoluzione finisce"), Rate), P.bFinished))
		{
			return false;
		}

		// Monotonia: un rate piu' basso non puo' produrre un picco visuale piu' alto. E' la verifica che il
		// numero che si sta tarando comandi davvero cio' che si vede — se il budget riprendesse il
		// controllo, questa riga cadrebbe sul primo rate che lo fa mordere.
		if (PreviousPeak >= 0.f)
		{
			TestTrue(*FString::Printf(
				TEXT("a %.2f celle/s il picco (%.1f u/s) non supera quello del rate piu' alto (%.1f u/s)"),
				Rate, P.PeakVisualSpeed, PreviousPeak),
				P.PeakVisualSpeed <= PreviousPeak * 1.02f);
		}
		PreviousPeak = P.PeakVisualSpeed;
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
