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

	// ⚠️ Il riferimento in VOCI non c'e': dal file arrivano byte, e deserializzarli qui duplicherebbe il
	// loader. La divergenza viene quindi rilevata ma non localizzata, e il verdetto lo dichiara da se'
	// invece di lasciar credere che non ci fosse niente da dire.
	const FRTDebugReplayVerdict Verdict = URTDebugReportLibrary::VerifyReplay(
		GoldenBytes, /*GoldenEntries*/ {}, TM->GetTurnLog(), ERTLogTopology::Hex, NAME_None);
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
// I comandi di OVERLAY
//
// Il contenuto delle etichette e' testo, e come tale si verifica headless; che compaia a schermo e'
// `PIE-V01-DEBUG` (seduta U15). Qui i comandi stampano l'inventario **e** accendono il disegno, cosi' una
// sessione senza viewport resta utile.
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
	for (int32 i = 0; i < Routes.Num(); ++i)
	{
		FString Path;
		for (const FRTCellId& Cell : Routes[i])
		{
			if (!Path.IsEmpty()) { Path += TEXT(" -> "); }
			Path += Cell.ToString();
		}
		Ar.Logf(TEXT("[RT]   unita %d: %s"), i, Path.IsEmpty() ? TEXT("(ferma)") : *Path);
	}
	Ar.Logf(TEXT("[RT] Percorsi dell'ultima risoluzione: %d."), Routes.Num());
}

static void RTDebugDrawResolutionCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	ARTTurnManager* TM = FindTurnManager(World, Ar);
	if (!TM) { return; }

	Ar.Logf(TEXT("[RT] Risoluzione: round %d, fase %s."),
		TM->GetTurnNumber(), *StaticEnum<ERTMatchPhase>()->GetNameStringByValue(
			static_cast<int64>(TM->GetPhase())));

	// Le voci dell'ultimo round soltanto: un dump dell'intera partita e' `rt.Debug.DumpTurnLog`, e
	// ripeterlo qui renderebbe i due comandi indistinguibili nell'uso.
	const TArray<FRTTurnLogEntry>& Log = TM->GetTurnLog();
	const int32 Round = TM->GetTurnNumber();
	TArray<FRTTurnLogEntry> ThisRound;
	for (const FRTTurnLogEntry& E : Log)
	{
		if (E.TurnNumber == Round) { ThisRound.Add(E); }
	}
	if (ThisRound.Num() == 0)
	{
		Ar.Log(TEXT("[RT]   nessuna voce per questo round."));
		return;
	}
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
	TEXT("Gli intenti VISIBILI a un osservatore: rt.Debug.DrawIntent [team]. Passa dal filtro di squadra, "
		 "quindi non mostra i piani avversari nemmeno a chi esegue il comando."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDrawIntentCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugDrawCover(
	TEXT("rt.Debug.DrawCover"),
	TEXT("Le celle che dichiarano una copertura, col bordo, il tipo e l'integrita'."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDrawCoverCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugDrawPaths(
	TEXT("rt.Debug.DrawPaths"),
	TEXT("I percorsi dell'ultima risoluzione, cella per cella."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDrawPathsCommand));

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugDrawResolution(
	TEXT("rt.Debug.DrawResolution"),
	TEXT("Il round corrente: fase e voci di TurnLog del solo round in corso."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDrawResolutionCommand));
