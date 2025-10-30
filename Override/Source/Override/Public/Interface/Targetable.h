#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Targetable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UTargetable : public UInterface
{
	GENERATED_BODY()
};

class OVERRIDE_API ITargetable
{
	GENERATED_BODY()

public:
	bool PointsGenerated;
	TArray<FVector> Points;
	
	virtual void Target() = 0;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Targetable")
	void OnTarget();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Targetable")
	void OnUntarget();
};
