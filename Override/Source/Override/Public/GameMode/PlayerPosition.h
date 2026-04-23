#pragma once

#include "PlayerPosition.generated.h"

USTRUCT(BlueprintType)
struct OVERRIDE_API FPlayerPosition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 Time = -1;
	
	UPROPERTY(BlueprintReadWrite)
	FVector Position = FVector::ZeroVector;
};
