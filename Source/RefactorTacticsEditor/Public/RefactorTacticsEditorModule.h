#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FLayoutExtender;

/**
 * Modulo editor-only del pivot esagonale.
 *
 * Due responsabilita', e sono di natura diversa: i comandi dell'Editor Mode hex (il mode si registra da
 * solo via CDO) e la **posizione nel layout** del tab del Tactical Designer.
 */
class FRefactorTacticsEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Dichiara **dove** vive il tab del launcher (#2168).
	 *
	 * ⚠️ **Registrare un tab e dargli un posto sono due cose diverse, e il difetto nasceva da questo.**
	 * `URTDevSandboxLauncherSubsystem::Initialize` chiama `RegisterNomadTabSpawner`, che dice al motore
	 * *come* costruire il pannello — non *dove* metterlo. Senza una posizione nel layout, `TryInvokeTab`
	 * apre il tab in una **finestra propria**: e' cio' che la seduta `U31` ha visto il 2026-09-02.
	 * Content Browser e Output Log non hanno il problema perche' il layout di default del Level Editor li
	 * dichiara (`SLevelEditor.cpp:1755-1758`) — sono nomad esattamente come il nostro, e si dockano.
	 *
	 * 🔑 **Statica e pura per la stessa ragione di `ShouldOpenFor`**: cosi' il test puo' applicarla a un
	 * layout finto senza aprire un editor. Fosse una lambda dentro `StartupModule`, di questa slice non
	 * resterebbe niente da misurare a macchina — vedi `RTLauncherLayoutTests.cpp`.
	 */
	static void ExtendLevelEditorLayout(FLayoutExtender& Extender);

	/**
	 * L'iscrizione a `OnRegisterLayoutExtensions` e' in piedi?
	 *
	 * ⚠️ Esiste per la stessa ragione di `URTDevSandboxLauncherSubsystem::IsSubscribed()`: senza,
	 * cancellare l'`AddStatic` in `StartupModule` lascerebbe **verdi** tutti i test del layout, che
	 * misurano il contenuto dell'extender e non il fatto che qualcuno lo riceva. La feature morirebbe
	 * in silenzio esattamente nel modo che il commento su quella riga dice di temere.
	 */
	bool IsLayoutExtensionRegistered() const { return LayoutExtensionHandle.IsValid(); }

private:
	/** L'iscrizione a `OnRegisterLayoutExtensions`, da disfare allo scarico del modulo. */
	FDelegateHandle LayoutExtensionHandle;
};
