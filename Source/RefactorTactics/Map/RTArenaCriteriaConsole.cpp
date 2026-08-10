#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Map/RTArenaCriteriaLibrary.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"

/**
 * `rt.Arena.Check [MoveBudget] [MaxCostRatio] [MinExposureGap]` — misura i **tre criteri dell'arena** che il
 * `done_when` di U1 dichiara, sulla mappa esagonale presente nel livello.
 *
 * Perche' esiste: senza, `URTArenaCriteriaLibrary` sarebbe verificabile solo dai test, cioe' solo su mappe
 * costruite da codice — e i criteri servono proprio a giudicare una mappa **dipinta a mano**, mentre la si
 * dipinge. Una regola che si puo' applicare solo dove il problema non c'e' non e' una regola.
 *
 * Sola lettura: non tocca l'asset ne' lo stato di gioco. Stampa **i numeri**, non solo il verdetto, perche'
 * «non passa» senza il margine trasforma la correzione in tentativo ed errore.
 *
 * I tre argomenti sono opzionali e servono a provare una soglia diversa senza ricompilare: le soglie di
 * default (`1,5` sul costo, `15%` sull'esposizione) sono **proposte, non misurate**, e la seduta che
 * costruisce l'arena e' il posto dove confermarle o cambiarle.
 */
static void RTArenaCheckCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (!World)
	{
		Ar.Log(TEXT("[RT] Nessun mondo attivo."));
		return;
	}

	ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(World);
	if (!HexMap)
	{
		Ar.Log(TEXT("[RT] Nessuna mappa esagonale nel livello."));
		return;
	}

	const URTHexMapAsset* Map = HexMap->MapAsset;
	if (!Map || Map->NumCells() == 0)
	{
		Ar.Log(TEXT("[RT] La mappa non ha celle: niente da misurare."));
		return;
	}

	const int32 MoveBudget = (Args.Num() > 0)
		? FCString::Atoi(*Args[0]) : URTArenaCriteriaLibrary::DefaultMoveBudget;
	const float MaxCostRatio = (Args.Num() > 1)
		? FCString::Atof(*Args[1]) : URTArenaCriteriaLibrary::DefaultMaxCostRatio;
	const float MinExposureGap = (Args.Num() > 2)
		? FCString::Atof(*Args[2]) : URTArenaCriteriaLibrary::DefaultMinExposureGap;

	const FRTArenaCriteriaReport Report = URTArenaCriteriaLibrary::EvaluateWithDerivedSpawns(
		Map, /*NumPerTeam=*/ 2, /*Layer=*/ 0, MoveBudget, MaxCostRatio, MinExposureGap);

	Ar.Logf(TEXT("[RT] Criteri dell'arena su %d celle (budget %d, rapporto max %.2f, scarto minimo %.0f%%):"),
		Map->NumCells(), MoveBudget, MaxCostRatio, MinExposureGap * 100.f);

	// Riga per riga, cosi' l'output resta leggibile nella console dell'editor (che non va a capo da sola).
	TArray<FString> Lines;
	URTArenaCriteriaLibrary::FormatReport(Report).ParseIntoArrayLines(Lines);
	for (const FString& Line : Lines) { Ar.Logf(TEXT("[RT] %s"), *Line); }

	Ar.Logf(TEXT("[RT] %s"), Report.AllPassed()
		? TEXT("Tutti e tre i criteri sono soddisfatti: l'arena rispetta il DoD di U1.")
		: TEXT("Almeno un criterio non passa: il dettaglio qui sopra dice di quanto."));
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTArenaCheck(
	TEXT("rt.Arena.Check"),
	TEXT("Misura i tre criteri dell'arena (copertura, costo, due rotte) sulla mappa esagonale del livello. "
		 "Argomenti opzionali: MoveBudget, MaxCostRatio, MinExposureGap. Sola lettura."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTArenaCheckCommand));
