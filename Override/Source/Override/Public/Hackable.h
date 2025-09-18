#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Hackable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UHackable : public UInterface
{
	GENERATED_BODY()
};

class OVERRIDE_API IHackable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void TryHack();
};
