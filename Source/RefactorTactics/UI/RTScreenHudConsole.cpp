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
#include "RefactorTactics.h"
#include "TimerManager.h"
#include "UI/RTScreenHudWidgets.h"
#include "UObject/UObjectIterator.h"
#include "Engine/World.h"

static void RTComponiRapportoScreenHud(UWorld* World, TFunctionRef<void(const FString&)> Emetti)
{
	if (World == nullptr)
	{
		Emetti(TEXT("[RT] Nessun mondo: questo comando vuole una partita in corso."));
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
		Emetti(TEXT("[RT] Screen HUD 4.1: nessuna radice viva in questo mondo."));
		Emetti(TEXT("[RT] Se la partita e' stata avviata aprendo una mappa invece che da L_Frontend, il "
					"layer non e' montato: e' il percorso, non un difetto. Lo monta EnterMatch."));
		return;
	}

	for (const FString& Riga : URTTacticalHUDWidget::ComposeMountReport(Radice))
	{
		Emetti(Riga);
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
			Emetti(Riga);
		}
		break;
	}
}

/**
 * `rt.Debug.ScreenHud [secondi]`
 *
 * 🔴 **Senza l'argomento questo comando non e' eseguibile in una run non presidiata, ed e' il caso per cui
 * era stato scritto.** `#2992` lo ha aggiunto per rendere leggibile la causa di `#2964` *«senza aprire
 * l'Editor»*, ma l'unico canale che una run headless ha per digitarlo — `-ExecCmds` — spara al **frame 1**:
 * misurato il 2026-09-15 su `-game -RTAutobattle`, la risposta era
 * *«nessun TurnManager acquisito»* **perche' il manager non era ancora nato** — `ARTGameMode::BeginPlay`
 * monta il HUD prima di spawnarlo. Una lettura vera nell'istante sbagliato, che somiglia al difetto e non lo e'.
 *
 * Con l'argomento lo stesso rapporto si ripete dopo `secondi` di gioco, cioe' **a partita avviata**: e' la
 * condizione che `DescribeFeedState` gia' nomina — *«se questa riga persiste a partita avviata,
 * l'acquisizione non e' avvenuta»* — e che finora nessuno poteva verificare senza una persona davanti a PIE.
 *
 * ⚠️ **Timer del MONDO e non `FTSTicker`**: i secondi che contano sono quelli della partita, e un gioco in
 * pausa deve rimandare il rapporto, non produrlo su uno stato fermo.
 *
 * ⚠️ Un argomento non numerico vale `0` e viene **rifiutato** invece di diventare «subito»: un refuso che
 * silenziosamente esegue la variante immediata rimetterebbe il difetto che questa riga chiude.
 *
 * **Sola lettura**, come la forma immediata: il differimento non tocca nulla, ne' quando parte ne' quando spara.
 */
static void RTDebugScreenHudCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	const auto SuOutputDevice = [&Ar](const FString& Riga) { Ar.Logf(TEXT("%s"), *Riga); };

	if (Args.Num() == 0)
	{
		RTComponiRapportoScreenHud(World, SuOutputDevice);
		return;
	}

	const float Ritardo = FCString::Atof(*Args[0]);
	if (!(Ritardo > 0.f))
	{
		Ar.Logf(TEXT("[RT] rt.Debug.ScreenHud: '%s' non e' un numero di secondi positivo. "
					 "Senza argomento il rapporto e' immediato."), *Args[0]);
		return;
	}

	if (World == nullptr)
	{
		Ar.Logf(TEXT("[RT] Nessun mondo: questo comando vuole una partita in corso."));
		return;
	}

	// Il mondo puo' morire prima che il timer spari — cambio mappa, fine sessione. Il timer vive nel suo
	// `FTimerManager` e se ne va con lui, ma il puntatore debole rende la lettura sicura anche nel caso in cui
	// non se ne andasse: e' la stessa disciplina dei widget, che non tengono mai un puntatore nudo al mondo.
	const TWeakObjectPtr<UWorld> MondoDebole(World);

	// ⚠️ **Statico e uno solo**: due invocazioni ravvicinate riarmano lo stesso timer invece di accodarsi.
	// E' la forma giusta per una sonda — chi la lancia due volte vuole l'ultima richiesta, non due rapporti.
	static FTimerHandle Handle;
	World->GetTimerManager().SetTimer(
		Handle,
		FTimerDelegate::CreateLambda([MondoDebole]()
		{
			// `Display` e non `Log`: il rapporto differito finisce in un file di log che qualcuno legge dopo,
			// e deve essere visibile con la verbosita' di default di una run non presidiata.
			RTComponiRapportoScreenHud(MondoDebole.Get(),
				[](const FString& Riga) { UE_LOG(LogRT, Display, TEXT("%s"), *Riga); });
		}),
		Ritardo,
		/*bLoop=*/ false);

	Ar.Logf(TEXT("[RT] rt.Debug.ScreenHud: rapporto differito di %.1f s di gioco. "
				 "Cerca le righe 'Feed:' nel log a quel punto."), Ritardo);
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugScreenHud(
	TEXT("rt.Debug.ScreenHud"),
	TEXT("L'albero COSTRUITO dello Screen HUD 4.1 in questo istante: chi e' montato e chi manca, "
		 "scendendo dentro i widget innestati. Con un argomento in secondi il rapporto e' DIFFERITO, "
		 "per leggerlo a partita avviata da una run non presidiata (-ExecCmds spara al frame 1). "
		 "Sola lettura."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugScreenHudCommand));
