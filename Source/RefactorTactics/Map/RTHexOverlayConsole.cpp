#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"

/**
 * `rt.Debug.DrawCells [0|1]` — accende l'overlay di LEGGIBILITA' della mappa: ogni cella col colore della sua
 * superficie, piu' marcatori distinti per «blocca il movimento» e «blocca la vista».
 *
 * Perche' esiste: in partita le celle sono tutte cilindri graybox identici, quindi fango, ostacoli e muri sono
 * invisibili. Una mappa che non comunica le proprie regole contraddice il pilastro «leggibilita' tattica», e
 * rende un playtest incapace di distinguere un difetto del gioco da una regola non mostrata.
 *
 * E' uno strumento di SVILUPPO, non la presentazione definitiva (materiali per superficie: M8/M9). Sola lettura:
 * non tocca lo stato di gioco. Il prefisso `rt.Debug.*` e' il namespace di CP 11.4 (#80), come `rt.Debug.Pacing`:
 * quando quella issue verra' lavorata, questo comando va aggiunto al suo elenco, non duplicato.
 */
static void RTDebugDrawCellsCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
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

	// Nessun argomento = interruttore. Con argomento, 0 spegne e qualunque altro valore accende.
	const bool bEnable = (Args.Num() == 0)
		? !HexMap->IsCellOverlayEnabled()
		: (Args[0] != TEXT("0"));
	HexMap->SetCellOverlayEnabled(bEnable);

	const URTHexMapAsset* Map = HexMap->MapAsset;
	if (!bEnable)
	{
		Ar.Log(TEXT("[RT] Overlay celle: spento."));
		return;
	}
	if (!Map || Map->NumCells() == 0)
	{
		Ar.Log(TEXT("[RT] Overlay celle: acceso, ma la mappa non ha celle (nulla da mostrare)."));
		return;
	}

	int32 Blocked = 0;
	int32 SightBlockers = 0;
	int32 Costly = 0;
	for (const FRTHexCellData& Cell : Map->Cells)
	{
		Blocked       += Cell.bBlocksMovement ? 1 : 0;
		SightBlockers += Cell.bBlocksLineOfSight ? 1 : 0;
		Costly        += (Cell.TotalMoveCost() > 1) ? 1 : 0;
	}

	Ar.Logf(TEXT("[RT] Overlay celle: acceso su %d celle."), Map->NumCells());
	Ar.Logf(TEXT("[RT]   contorno = superficie · rosso interno = blocca il movimento (%d) · giallo interno = blocca la vista (%d)"),
		Blocked, SightBlockers);
	Ar.Logf(TEXT("[RT]   celle a costo maggiore di 1: %d (si riconoscono dal colore della superficie)"), Costly);
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugDrawCells(
	TEXT("rt.Debug.DrawCells"),
	TEXT("Overlay di leggibilita' della mappa esagonale: superficie, blocchi del movimento e della vista. "
		 "Senza argomenti fa da interruttore; 0 spegne. Strumento di sviluppo, sola lettura."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugDrawCellsCommand));
