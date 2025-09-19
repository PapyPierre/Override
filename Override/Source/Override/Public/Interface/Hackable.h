#pragma once

#include "CoreMinimal.h"
#include "Targetable.h"
#include "UObject/Interface.h"
#include "Hackable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UHackable : public UTargetable
{
	GENERATED_BODY()
};

class OVERRIDE_API IHackable : public ITargetable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void Hack();
};
