#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "RTLauncherWorkspace.h"

#include "RTDevSandboxLauncherSubsystem.generated.h"

class URTScenarioAuthoring;

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

	/** Id del tab: registrato in `FGlobalTabmanager`, e' anche la chiave con cui il layout ne ricorda la visibilita'. */
	static const FName TabId;

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
