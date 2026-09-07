#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
// `FObjectPostSaveContext` viaggia PER VALORE nella firma dell'handler: serve il tipo completo, non una
// forward declaration.
#include "UObject/ObjectSaveContext.h"

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

	/**
	 * Gli agganci che fanno girare la validazione del template DA SOLA sono in piedi?
	 *
	 * ⚠️ Stessa ragione di `IsLayoutExtensionRegistered`: senza, cancellare le due iscrizioni in
	 * `StartupModule` lascerebbe **verdi** tutti i test della validazione, che misurano le regole e non il
	 * fatto che qualcuno le esegua. E' precisamente il difetto che questo aggancio esiste per chiudere —
	 * una validazione corretta che non veniva mai invocata.
	 */
	bool IsValidationHooked() const { return WorldSavedHandle.IsValid() && MapOpenedHandle.IsValid(); }

private:
	/** L'iscrizione a `OnRegisterLayoutExtensions`, da disfare allo scarico del modulo. */
	FDelegateHandle LayoutExtensionHandle;

	/** Validazione del template al salvataggio del livello. */
	FDelegateHandle WorldSavedHandle;

	/** Validazione del template all'apertura del livello. */
	FDelegateHandle MapOpenedHandle;

	/**
	 * Valida il livello appena salvato.
	 *
	 * ⚠️ Il mondo arriva per argomento e NON si legge da `GEditor`: un salvataggio puo' riguardare un
	 * mondo che non e' quello aperto nel viewport.
	 */
	void HandleWorldSavedForValidation(class UWorld* World, FObjectPostSaveContext Context);

	/** Valida il livello appena aperto. Qui il mondo NON arriva per argomento: si chiede a `GEditor`. */
	void HandleMapOpenedForValidation(const FString& Filename, bool bAsTemplate);

	/** Il corpo condiviso dei due handler, con la guardia che tiene fuori PIE e commandlet. */
	static void ValidateWorldIfEditor(class UWorld* World);
};
