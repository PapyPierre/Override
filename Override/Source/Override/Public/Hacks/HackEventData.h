#pragma once

#include "CoreMinimal.h"
#include "HackEventData.generated.h"

USTRUCT(BlueprintType)
struct FHackEventData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector TargetLocation;
    
	UPROPERTY(BlueprintReadWrite)
	AActor* TargetActor;
    
	UPROPERTY(BlueprintReadWrite)
	float CustomValue;
};
