#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "RTLauncherWorkspace.h"

#include "RTDevSandboxLauncherSubsystem.generated.h"

class URTScenarioAuthoring;

/**
 * La categoria di log del launcher, **dichiarata una volta e definita una volta**.
 *
 * 🔴 **Era `DEFINE_LOG_CATEGORY_STATIC` in DUE `.cpp` dello stesso modulo**, e il secondo (`fix(2168)`,
 * `7d8cfd63`) lo motivava proprio con *«stessa categoria del subsystem, e non una nuova»*. L'intento era
 * giusto e la forma no: `_STATIC` crea un tipo **locale al file**, quindi due definizioni non sono la stessa
 * categoria — sono due omonime, e sotto **unity build** finiscono nella stessa unita' di traduzione dove
 * collidono (`C2027: utilizzo di tipo non definito 'FLogCategoryLogRTDevSandboxLauncher'`).
 *
 * ⚠️ **Il difetto e' latente e non deterministico**: si vede solo quando UBT raggruppa quei due file
 * insieme, cosa che dipende da quali file del modulo sono cambiati. Una build verde non lo smentisce.
 *
 * ⚠️ **Resta una categoria del modulo EDITOR e non `LogRT`**, e la ragione originale regge: `LogRT` e'
 * dichiarata in `RefactorTactics.h` senza `REFACTORTACTICS_API`, quindi il simbolo non attraversa il
 * confine di modulo — misurato, il link fallisce con `LNK2001`.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogRTDevSandboxLauncher, Log, All);

/**
 * Punto d'ingresso d'editor del workflow Tactical Designer: quando si apre il livello di bootstrap,
 * presenta il launcher. Owner documentale: `docs/technical/tooling/spec-tactical-designer.md` §4.1.
 *
 * ⚠️ **Non e' un'autorita' e non possiede una sessione.** La sessione d'authoring e' `URTScenarioAuthoring`
 * (ADR-0010), che ha gia' apertura, stato aperto, chiusura, validazione, Run e Reset. Questo subsystem e'
 * il «punto d'accesso unico in Editor» che ADR-0010 §4 prevede *sopra quella stessa facade*: possiede quale
 * facade sia la sessione corrente, il ciclo di vita legato all'apertura della mappa, e lo stato per-utente.
 * Nessuna delle tre riguarda il gioco.
 *
 * Perche' `UEditorSubsystem` e non un `UGameInstanceSubsystem`: fuori da PIE non esiste una `GameInstance`,
 * e il launcher deve vivere nell'Editor. Perche' non e' la facade a essere un subsystem: quella deve restare
 * chiamabile headless dai test automation, ed e' la ragione per cui ADR-0010 la tiene un `UObject`.
 */
UCLASS()
class URTDevSandboxLauncherSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Id del tab: registrato in `FGlobalTabmanager`, e' anche la chiave con cui il layout ne ricorda la
	 * visibilita'.
	 *
	 * ⚠️ **Registrarlo e collocarlo sono due cose diverse, e vivono in due file** (#2168). Qui sta il
	 * `TabId` e il suo spawner; **dove** il tab si docka lo dichiara
	 * `FRefactorTacticsEditorModule::ExtendLevelEditorLayout`, che deve iscriversi a
	 * `OnRegisterLayoutExtensions` prima che il Level Editor si costruisca. Chi cambia questa costante
	 * cambia anche quella.
	 *
	 * ⛔ **E `FGlobalTabmanager` e' proprio il manager che NON sa collocarlo**: la posizione vive nel
	 * layout del `LevelEditorTabManager`, che e' un sub-manager. Per invocarlo si usa
	 * `InvokeTabInLayout`, non `FGlobalTabmanager::Get()->TryInvokeTab` diretto.
	 */
	static const FName TabId;

	/**
	 * Apre un tab **rispettando la posizione dichiarata nel layout del Level Editor**, e ricade sul tab
	 * manager globale solo se quella strada non porta da nessuna parte.
	 *
	 * 🔑 **Perche' l'ordine non e' arbitrario.** `FGlobalTabmanager::Get()->TryInvokeTab` cerca il tab
	 * chiuso in `DockAreas` e `CollapsedDockAreas`, che sono membri **di istanza**
	 * (`TabManager.cpp:AttemptToOpenTab`). La docking area del Level Editor appartiene al
	 * `LevelEditorTabManager` — un sub-manager creato da
	 * `FGlobalTabmanager::Get()->NewTabManager(OwnerTab)` (`LevelEditor.cpp:815`) — e il motore ha un ramo
	 * che sale dal sub-manager al globale (`TabManager.cpp:1778`), **nessuno** che scenda. Invocare dal
	 * globale un tab collocato nel layout del Level Editor non lo trova, e apre una finestra: e' il
	 * difetto che #2168 correggeva a meta' finche' questa funzione non e' esistita.
	 *
	 * E' il pattern del motore, non un'invenzione: `SOutputLog.cpp:2509` prova il tab manager specifico e
	 * `2515` usa il globale come fallback; `ContentBrowserSingleton.cpp:979-983` fa lo stesso.
	 *
	 * @return `true` se un tab e' stato aperto, da una delle due strade.
	 */
	static bool InvokeTabInLayout(FName InTabId);

	/**
	 * Predicato PURO: il percorso appena aperto e' il livello di bootstrap del Tactical Designer?
	 *
	 * ⚠️ **Prende un percorso, non un nome di mappa, e la differenza e' la ragione per cui esiste.**
	 * `FEditorDelegates::OnMapOpened` consegna `const FString& Filename`, cioe' cio' che l'editor ha
	 * effettivamente caricato: puo' essere assoluto, relativo alla radice del progetto, con o senza
	 * estensione `.umap`, e la stessa mappa arriva scritta in modi diversi a seconda di come e' stata
	 * aperta. Confrontare stringhe intere sbaglierebbe su tutte queste forme; qui si confronta il solo
	 * nome base, che e' l'identita' che il livello ha davvero.
	 *
	 * Puro per poter essere verificato senza aprire un livello: e' l'unico pezzo di questa slice che un
	 * automation test puo' esaminare, e per questo la decisione vive qui invece che dentro l'handler.
	 */
	static bool ShouldOpenFor(const FString& MapFilename);

	/**
	 * L'iscrizione a `OnMapOpened` e' in piedi? Query sullo stato proprio, non una scorciatoia di test:
	 * un launcher non iscritto non si apre mai, e senza questo nessun automation test puo' accorgersene.
	 */
	bool IsSubscribed() const { return MapOpenedHandle.IsValid(); }

	/**
	 * Apre la sessione d'authoring su uno scenario esistente (#1682).
	 *
	 * ⚠️ **Non c'e' un oggetto «sessione» nuovo**: la sessione *e'* `URTScenarioAuthoring`, e questo metodo
	 * ne apre una e la tiene. E' la risposta alla domanda `A3` del referto del 2026-08-29 §8, dove
	 * «Session» risultava un nome gia' occupato due volte senza che nessuno scegliesse.
	 *
	 * ⛔ Il rifiuto non e' silenzioso e non e' parziale: se `OpenById` non dice `Success`, la facade viene
	 * chiusa e **nessuna** sessione resta aperta. Una sessione mezza aperta su uno scenario invalido e'
	 * peggio di nessuna sessione — sembra funzionare.
	 */
	FRTLauncherStartDecision StartSession(const FString& ScenarioId);

	/**
	 * Apre la sessione su uno scenario NUOVO, creato dalla facade.
	 *
	 * ⚠️ `CreateScenarioDraft` + `NewScenario`, non una costruzione locale: e' l'AC che impedisce al
	 * launcher di diventare una seconda porta verso il modello.
	 */
	FRTLauncherStartDecision StartNewSession(const FString& ScenarioId, int32 MapRadius = 3);

	/** Chiude la sessione, se c'e'. Idempotente: chiuderne una che non esiste non e' un errore. */
	void EndSession();

	/** C'e' una sessione aperta? */
	bool HasSession() const;

	/** La sessione corrente, oppure `nullptr`. Sola lettura per il pannello: chi la possiede e' questo subsystem. */
	URTScenarioAuthoring* GetSession() const { return Session; }

	/**
	 * Attiva una superficie del workspace.
	 *
	 * ⚠️ **Rifiuta una superficie non dichiarata invece di non fare niente.** Un pulsante inerte e' il
	 * `silent fallback` che il guardrail vieta: chi lo preme conclude che lo strumento e' rotto, non che
	 * la superficie non esiste ancora.
	 *
	 * @return vero se la superficie e' stata attivata.
	 */
	bool ActivateSurface(FName SurfaceKey);

private:
	/** Handler del delegate. Non decide: chiede a `ShouldOpenFor` e, se si', invoca il tab. */
	void HandleMapOpened(const FString& Filename, bool bAsTemplate);

	/** Serve a togliere l'iscrizione in `Deinitialize`: un handle che sopravvive allo scarico e' un crash. */
	FDelegateHandle MapOpenedHandle;

	/**
	 * La sessione d'authoring corrente.
	 *
	 * ⚠️ `UPROPERTY` e non un puntatore nudo: e' un `UObject` creato su `GetTransientPackage()`, e senza
	 * radice il GC lo raccoglierebbe fra un'apertura e un `Run` — un crash che si manifesta dopo una pausa,
	 * cioe' il piu' difficile da attribuire. E' la stessa ragione per cui il pannello tiene la sua facade
	 * di sola lettura in un `TStrongObjectPtr`.
	 */
	UPROPERTY()
	TObjectPtr<URTScenarioAuthoring> Session;
};
