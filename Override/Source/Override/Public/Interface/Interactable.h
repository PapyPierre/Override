#pragma once

#include "CoreMinimal.h"
#include "Targetable.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UTargetable
{
	GENERATED_BODY()
};

class OVERRIDE_API IInteractable : public ITargetable
{
	GENERATED_BODY()

public:
	virtual void Interact() = 0;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Interactable")
	void OnInteract();
};
