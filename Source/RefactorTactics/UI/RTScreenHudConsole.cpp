// `rt.Debug.ScreenHud` — l'albero del §4.1 A REGIME, che il log al montaggio non puo' dare.
//
// 🔴 **Perche' il dump di `NativeConstruct` non basta, e la ragione e' misurata.** Quello fotografa
// l'albero nell'istante in cui la radice si costruisce, e gli `WBP_RT_ActionSlot` non ci sono ancora: li
// crea il grafo del dock quando arrivano le azioni. Durante la seduta `U49` del 2026-09-10 il dump al
// montaggio diceva `[MANCA] ActionSlot: 0` mentre il log della STESSA sessione portava 10 431 warning
// `Icona non risolta` firmate `'ActionSlot'` — che solo un `URTActionSlotWidget` puo' emettere.
//
// Le due righe non si contraddicono: fotografano due momenti. Una lettura sola avrebbe fatto aprire una
// issue per un widget presente.
//
// ⚠️ **Collocazione, non capriccio**: i comandi vivono nel file del dominio che ispezionano —
// `Map/RTHexOverlayConsole.cpp` per la mappa, `Turn/RTPacingConsole.cpp` per il turno. Questo e' l'UI.
//
// ⚠️ **Non va aggiunto agli otto di `Debug.NamespaceDeclaresAllCommands`**: quel DoD elenca cio' che DEVE
// esserci, non tutto cio' che c'e'. Stesso precedente di `rt.Debug.Los` e `rt.Debug.HeroProfile`.
//
// **Sola lettura**: non tocca lo stato di gioco ne' l'albero che ispeziona.

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "UI/RTScreenHudWidgets.h"
#include "UObject/UObjectIterator.h"
#include "Engine/World.h"

static void RTDebugScreenHudCommand(const TArray<FString>& /*Args*/, UWorld* World, FOutputDevice& Ar)
{
	if (World == nullptr)
	{
		Ar.Logf(TEXT("[RT] Nessun mondo: questo comando vuole una partita in corso."));
		return;
	}

	// 🔑 **L'iterazione e' filtrata sul mondo, e il filtro non e' prudenza generica**: in PIE convivono
	// l'istanza del gioco e quella dell'Editor. Senza, il dump potrebbe nominare i widget di una sessione
	// che non e' quella osservata — cioe' rispondere alla domanda giusta sul soggetto sbagliato.
	// `IsTemplate` esclude il CDO, che ha un albero vuoto e direbbe «manca tutto».
	URTTacticalHUDWidget* Radice = nullptr;
	for (TObjectIterator<URTTacticalHUDWidget> It; It; ++It)
	{
		if (*It != nullptr && !It->IsTemplate() && It->GetWorld() == World)
		{
			Radice = *It;
			break;
		}
	}

	if (Radice == nullptr)
	{
		Ar.Logf(TEXT("[RT] Screen HUD 4.1: nessuna radice viva in questo mondo."));
		Ar.Logf(TEXT("[RT] Se la partita e' stata avviata aprendo una mappa invece che da L_Frontend, il "
					 "layer non e' montato: e' il percorso, non un difetto. Lo monta EnterMatch."));
		return;
	}

	for (const FString& Riga : URTTacticalHUDWidget::ComposeMountReport(Radice))
	{
		Ar.Logf(TEXT("%s"), *Riga);
	}

	// ── Perche' il feed e' vuoto, che l'albero da solo non dice.
	//
	// 🔑 **Un `EventLog` MONTATO e VUOTO e un `EventLog` assente hanno lo stesso aspetto a schermo**, e il
	// mount report qui sopra distingue solo il secondo. Nella seduta `U49` il report diceva `[ok] EventLog: 1`
	// mentre la zona destra restava vuota per cinque turni: due letture vere, nessuna delle due sufficiente.
	for (TObjectIterator<URTPlayerEventLogWidget> It; It; ++It)
	{
		if (*It == nullptr || It->IsTemplate() || It->GetWorld() != World)
		{
			continue;
		}
		for (const FString& Riga : It->DescribeFeedState())
		{
			Ar.Logf(TEXT("%s"), *Riga);
		}
		break;
	}
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugScreenHud(
	TEXT("rt.Debug.ScreenHud"),
	TEXT("L'albero COSTRUITO dello Screen HUD 4.1 in questo istante: chi e' montato e chi manca, "
		 "scendendo dentro i widget innestati. Da eseguire in PIE, a partita avviata. Sola lettura."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugScreenHudCommand));
