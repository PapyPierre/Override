#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TargetDataContainer.generated.h"

UCLASS(BlueprintType)
class OVERRIDE_API UTargetDataContainer : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite)
	TArray<AActor*> Targets;
};
