#include "RTDeveloperToolsLog.h"
#include "RTDevToolset.h"

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

DEFINE_LOG_CATEGORY(LogRTDevTools);

/**
 * Registra il toolset RT presso il `ToolsetRegistry`, che e' cio' che lo rende raggiungibile dal server MCP
 * dell'engine. Stessa forma dei toolset di Epic (`GameplayTagsToolset/Module.cpp`).
 *
 * Il modulo e' `Type: "Editor"` con `TargetAllowList: ["Editor"]` nel `.uplugin`: non esiste in un target
 * packaged, quindi questa registrazione non puo' avvenire in una build di gioco.
 */
class FRTDeveloperToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(URTDevToolset::StaticClass());
		UE_LOG(LogRTDevTools, Log, TEXT("RT developer toolset registered."));
	}

	virtual void ShutdownModule() override
	{
		UToolsetRegistry::UnregisterToolsetClass(URTDevToolset::StaticClass());
	}
};

IMPLEMENT_MODULE(FRTDeveloperToolsModule, RTDeveloperTools);
