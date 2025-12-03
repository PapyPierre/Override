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
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Targetable")
	void OnTarget(AActor* TargetingActor);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Targetable")
	void OnUntarget();
};
