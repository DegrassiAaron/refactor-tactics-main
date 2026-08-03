#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** Modulo editor-only del pivot esagonale: registra i comandi dell'Editor Mode hex (il mode si registra via CDO). */
class FRefactorTacticsEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
