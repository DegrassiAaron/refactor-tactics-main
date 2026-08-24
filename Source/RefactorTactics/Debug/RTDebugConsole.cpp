// CP 11.4 (#80) — gli otto comandi `rt.Debug.*`.
//
// Ogni comando qui e' un WRAPPER: prende il mondo, trova cio' che serve, delega a
// `URTDebugReportLibrary` e stampa. Nessuna regola vive in questo file — e' la condizione perche' il
// contenuto sia verificabile headless mentre un `FAutoConsoleCommand` non lo e'.
//
// ⚠️ **`rt.Debug.DrawCells` e `rt.Debug.Pacing` NON stanno qui**, e non e' una dimenticanza: vivono in
// `Map/RTHexOverlayConsole.cpp` e `Turn/RTPacingConsole.cpp`, dov'e' il dominio che ispezionano — il
// precedente del repository, cinque file su cinque. Il gate «gli otto comandi esistono» non si verifica
// guardando questa cartella ma interrogando il namespace a runtime:
// `RefactorTactics.Debug.NamespaceDeclaresAllCommands`.
//
// **Sola lettura**: nessuno di questi comandi modifica lo stato di gioco. Uno strumento d'ispezione che
// muove cio' che ispeziona produce sessioni di debug che non si possono confrontare fra loro.

#include "CoreMinimal.h"
#include "Debug/RTDebugReportLibrary.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Misc/FileHelper.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnManager.h"
#include "UI/RTHudViewModel.h"
#include "Unit/RTUnit.h"

namespace
{
	/** Il TurnManager del livello, o `nullptr` con un messaggio gia' stampato. */
	ARTTurnManager* FindTurnManager(UWorld* World, FOutputDevice& Ar)
	{
		if (!World)
		{
			Ar.Log(TEXT("[RT] Nessun mondo attivo."));
			return nullptr;
		}
		ARTTurnManager* TM = Cast<ARTTurnManager>(
			UGameplayStatics::GetActorOfClass(World, ARTTurnManager::StaticClass()));
		if (!TM)
		{
			Ar.Log(TEXT("[RT] Nessun TurnManager nel livello."));
		}
		return TM;
	}

	void LogAll(FOutputDevice& Ar, const TArray<FString>& Lines)
	{
		for (const FString& Line : Lines) { Ar.Log(*Line); }
	}

	/**
	 * Il team dell'osservatore. In v0.1 e' sempre `0`: la partita e' offline e il giocatore controlla il
	 * team blu — la stessa costante che `ARTHUD` dichiara. Un argomento permette di ispezionare l'altro
	 * lato, ed e' legittimo perche' questo e' uno strumento di sviluppo che gira sulla macchina di chi
	 * possiede gia' tutto lo stato. ⚠️ Non lo sara' piu' in rete (M10), dove un comando del genere
	 * dev'essere lato server o non esistere.
	 */
	int32 ObserverTeamFromArgs(const TArray<FString>& Args)
	{
		return (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 0;
	}
}

// ---------------------------------------------------------------------------------------------------
// I quattro comandi TESTUALI
// ---------------------------------------------------------------------------------------------------

static void RTDebugDumpSnapshotCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	ARTTurnManager* TM = FindTurnManager(World, Ar);
	if (!TM) { return; }

	TArray<ARTUnit*> Units;
	const FRTHexSnapshot Snapshot = TM->MakeCurrentSnapshot(Units);
	LogAll(Ar, URTDebugReportLibrary::DescribeSnapshot(Snapshot));
}

static void RTDebugDumpTurnLogCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	ARTTurnManager* TM = FindTurnManager(World, Ar);
	if (!TM) { return; }

	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	if (Log.Num() == 0)
	{
		Ar.Log(TEXT("[RT] TurnLog vuoto: nessun turno risolto in questa partita."));
		return;
	}
	LogAll(Ar, URTDebugReportLibrary::DescribeTurnLogEntries(Log));
}

static void RTDebugVerifyReplayCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	ARTTurnManager* TM = FindTurnManager(World, Ar);
	if (!TM) { return; }

	if (Args.Num() == 0)
	{
		Ar.Log(TEXT("[RT] Uso: rt.Debug.VerifyReplay <percorso-traccia-golden>"));
		return;
	}

	TArray<uint8> GoldenBytes;
	if (!FFileHelper::LoadFileToArray(GoldenBytes, *Args[0]))
	{
		Ar.Logf(TEXT("[RT] Traccia di riferimento non leggibile: %s"), *Args[0]);
		return;
	}

	// 🔴 **Il contesto si LEGGE dal file, non si assume.** Una stesura precedente passava
	// `ERTLogTopology::Hex` e `NAME_None` fissi: `CompareSerializedTraces` guarda il formato **prima** del
	// contenuto (`RTTurnLogLibrary.cpp:1037`) e ogni traccia registrata porta il `FormatId` vero del
	// formato di partita — quindi il comando rispondeva `FormatMismatch` su qualunque replay reale e non
	// poteva **mai** rilevare una divergenza. Il test non se ne accorgeva perche' chiama la funzione pura,
	// dove il formato lo passa il chiamante.
	//
	// `DeserializeTurnLog` e' il loader del progetto, e' pubblico, e restituisce insieme le voci, la
	// topologia e il formato: le tre cose che servono qui. Le voci servono anche a LOCALIZZARE la
	// divergenza, che senza di esse resterebbe solo annunciata.
	TArray<FRTTurnLogEntry> GoldenEntries;
	ERTLogTopology Topology = ERTLogTopology::Hex;
	FName FormatId = NAME_None;
	if (!URTTurnLogLibrary::DeserializeTurnLog(GoldenBytes, GoldenEntries, &Topology, &FormatId))
	{
		Ar.Logf(TEXT("[RT] La traccia di riferimento non e' leggibile: %s"), *Args[0]);
		Ar.Log(TEXT("[RT]   (magic, versione, topologia non riconosciuta o buffer troncato)"));
		return;
	}

	const FRTDebugReplayVerdict Verdict = URTDebugReportLibrary::VerifyReplay(
		GoldenBytes, GoldenEntries, TM->GetTurnLog(), Topology, FormatId);
	LogAll(Ar, Verdict.Lines);
}

static void RTDebugDrawIntentCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (!World)
	{
		Ar.Log(TEXT("[RT] Nessun mondo attivo."));
		return;
	}
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(World, ARTUnit::StaticClass(), Actors);

	// 🔴 La stessa costruzione che usa la HUD, e poi lo stesso filtro. Un comando che leggesse i piani
	// direttamente dalle unita' sarebbe l'unico punto del gioco da cui si legge un avversario.
	const TArray<FRTPlannedIntent> Authoritative = URTHudViewModel::BuildAuthoritativeIntents(Actors);
	const TArray<FString> Lines = URTDebugReportLibrary::DescribeIntents(ObserverTeamFromArgs(Args), Authoritative);

	if (Lines.Num() == 0)
	{
		Ar.Log(TEXT("[RT] Nessun intento visibile a questo osservatore."));
		return;
	}
	LogAll(Ar, Lines);
}

// ---------------------------------------------------------------------------------------------------
// I comandi di ISPEZIONE della scena
//
// 🔴 **Stampano, non disegnano — e va detto qui invece che scoperto in PIE.** Una stesura precedente di
// questa intestazione affermava che «accendono il disegno»: falso, nessuno dei tre tocca uno stato di
// overlay. L'unico comando che disegna davvero e' `rt.Debug.DrawCells`, che chiama
// `SetCellOverlayEnabled` (`Map/RTHexOverlayConsole.cpp`).
//
// ⏳ **Conseguenza dichiarata**: il DoD di #80 chiede comandi che disegnino, e `PIE-V01-DEBUG` lo
// verifichera' a schermo. Per `DrawPaths`, `DrawCover` e `DrawResolution` quel lavoro **non e' fatto**:
// l'overlay grafico richiede un consumatore in `ARTHexMapActor` sul modello di `DrawCellOverlay`, e
// finche' non esiste questi tre rispondono in console. Il nome resta `Draw*` perche' e' quello che il DoD
// nomina; il comportamento e' descritto qui e nella help string di ciascuno.
// ---------------------------------------------------------------------------------------------------

static void RTDebugDrawCoverCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (!World) { Ar.Log(TEXT("[RT] Nessun mondo attivo.")); return; }

	ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(World);
	const URTHexMapAsset* Map = HexMap ? HexMap->MapAsset : nullptr;
	if (!Map) { Ar.Log(TEXT("[RT] Nessuna mappa esagonale nel livello.")); return; }

	ARTTurnManager* TM = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(World, ARTTurnManager::StaticClass()));
	TArray<ARTUnit*> Units;
	const FRTHexSnapshot Snapshot = TM ? TM->MakeCurrentSnapshot(Units) : FRTHexSnapshot();

	int32 Shown = 0;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		if (Cell.Covers.Num() == 0) { continue; }
		const int32* Occupant = Snapshot.Occupancy.Find(Cell.Id);
		Ar.Logf(TEXT("[RT]   %s"),
			*URTDebugReportLibrary::DescribeCell(Cell, Occupant ? *Occupant : INDEX_NONE, Map->Revision));
		++Shown;
	}
	Ar.Logf(TEXT("[RT] Coperture: %d celle su %d ne dichiarano almeno una."), Shown, Map->NumCells());
}

static void RTDebugDrawPathsCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	ARTTurnManager* TM = FindTurnManager(World, Ar);
	if (!TM) { return; }

	const TArray<TArray<FRTCellId>>& Routes = TM->GetLastMoveRoutes();
	if (Routes.Num() == 0)
	{
		Ar.Log(TEXT("[RT] Nessun percorso: la partita non ha ancora risolto un movimento."));
		return;
	}
	// ⚠️ **L'indice NON e' un `UnitId`, e non e' nemmeno l'indice dell'unita'.** `LastMoveRoutes` e'
	// COMPATTATO: `RTTurnManager.cpp` vi aggiunge una voce solo quando `Entered.Num() > 0`, quindi con le
	// unita' 0 e 2 in movimento e la 1 ferma i due percorsi sono `#0` e `#1`. Una stesura precedente
	// stampava «unita %d» su questo indice e nominava un'unita' che non si era mossa. La prima cella del
	// percorso dice **da dove** parte, ed e' l'unico aggancio corretto disponibile qui.
	for (int32 i = 0; i < Routes.Num(); ++i)
	{
		FString Path;
		for (const FRTCellId& Cell : Routes[i])
		{
			if (!Path.IsEmpty()) { Path += TEXT(" -> "); }
			Path += Cell.ToString();
		}
		Ar.Logf(TEXT("[RT]   percorso #%d (da %s): %s"), i,
			Routes[i].Num() > 0 ? *Routes[i][0].ToString() : TEXT("?"), *Path);
	}
	Ar.Logf(TEXT("[RT] Percorsi dell'ultima risoluzione: %d — solo le unita' che si sono MOSSE."),
		Routes.Num());
}

static void RTDebugDrawResolutionCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	ARTTurnManager* TM = FindTurnManager(World, Ar);
	if (!TM) { return; }

	Ar.Logf(TEXT("[RT] Risoluzione: round %d, fase %s."),
		TM->GetTurnNumber(), *StaticEnum<ERTMatchPhase>()->GetNameStringByValue(
			static_cast<int64>(TM->GetPhase())));

	// Le voci dell'ultimo round RISOLTO: un dump dell'intera partita e' `rt.Debug.DumpTurnLog`, e
	// ripeterlo qui renderebbe i due comandi indistinguibili nell'uso.
	//
	// 🔴 **Il round da mostrare non e' `GetTurnNumber()`**, ed e' il difetto che questa riga aveva:
	// `++TurnNumber` avviene alla FINE della risoluzione, subito prima di `StartPlanningTimer()`. Chi apre
	// la console per capire cosa e' appena successo si trova gia' nel round successivo, e il filtro
	// `== GetTurnNumber()` restituiva sempre l'insieme vuoto — tranne durante la risoluzione, cioe'
	// esattamente quando nessuno puo' digitare. Il round giusto e' il massimo presente nel log.
	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	if (Log.Num() == 0)
	{
		Ar.Log(TEXT("[RT]   nessun round risolto in questa partita."));
		return;
	}
	int32 LastResolved = Log[0].TurnNumber;
	for (const FRTTurnLogEntry& E : Log)
	{
		LastResolved = FMath::Max(LastResolved, E.TurnNumber);
	}

	TArray<FRTTurnLogEntry> ThisRound;
	for (const FRTTurnLogEntry& E : Log)
	{
		if (E.TurnNumber == LastResolved) { ThisRound.Add(E); }
	}
	Ar.Logf(TEXT("[RT]   ultimo round risolto: %d."), LastResolved);
	LogAll(Ar, URTDebugReportLibrary::DescribeTurnLogEntries(ThisRound));
}

// ---------------------------------------------------------------------------------------------------
// Registrazione
// ---------------------------------------------------------------------------------------------------

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugDumpSnapshot(
	TEXT("rt.Debug.DumpSnapshot"),
	TEXT("Lo snapshot corrente: unita', occupazione, revisione e problemi di validazione. Sola lettura."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDumpSnapshotCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugDumpTurnLog(
	TEXT("rt.Debug.DumpTurnLog"),
	TEXT("Il TurnLog della partita, una riga per voce con ActionId, unita', priorita', esito e hash."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDumpTurnLogCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugVerifyReplay(
	TEXT("rt.Debug.VerifyReplay"),
	TEXT("Confronta il TurnLog corrente con una traccia di riferimento: rt.Debug.VerifyReplay <file>."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugVerifyReplayCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugDrawIntent(
	TEXT("rt.Debug.DrawIntent"),
	TEXT("Gli intenti visibili a un osservatore: rt.Debug.DrawIntent [team], default 0. Compone dalla vista "
		 "filtrata da FilterForTeam, mai dai piani grezzi. ATTENZIONE: il team e' un ARGOMENTO, quindi "
		 "'DrawIntent 1' mostra i piani del team 1 come farebbe il team 1 — e' uno strumento di sviluppo "
		 "locale, dove chi lo esegue possiede gia' tutto lo stato. In rete (M10) dovra' essere lato server "
		 "o non esistere."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDrawIntentCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugDrawCover(
	TEXT("rt.Debug.DrawCover"),
	TEXT("ELENCA in console le celle che dichiarano una copertura, col bordo, il tipo e l'integrita'. "
		 "Non disegna: l'overlay grafico e' solo di rt.Debug.DrawCells."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDrawCoverCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugDrawPaths(
	TEXT("rt.Debug.DrawPaths"),
	TEXT("ELENCA in console i percorsi dell'ultima risoluzione, cella per cella — solo le unita' che si "
		 "sono mosse. Non disegna."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDrawPathsCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugDrawResolution(
	TEXT("rt.Debug.DrawResolution"),
	TEXT("ELENCA in console le voci di TurnLog dell'ultimo round RISOLTO, piu' fase e round correnti. "
		 "Non disegna."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDrawResolutionCommand));
