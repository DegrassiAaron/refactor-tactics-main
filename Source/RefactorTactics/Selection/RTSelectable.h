#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RTSelectable.generated.h"

UINTERFACE(MinimalAPI)
class URTSelectable : public UInterface
{
	GENERATED_BODY()
};

/** Implementata dagli attori selezionabili (es. le unita' in M2). */
class IRTSelectable
{
	GENERATED_BODY()

public:
	virtual void OnSelected() {}
	virtual void OnDeselected() {}
};
