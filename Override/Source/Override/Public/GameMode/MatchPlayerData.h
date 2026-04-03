#pragma once

#include "PlayerPosition.h"
#include "MatchPlayerData.generated.h"

USTRUCT(BlueprintType)
struct OVERRIDE_API FMatchPlayerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 PlayerId = -1;
	
	UPROPERTY(BlueprintReadWrite)
	int32 TeamId = -1;

	UPROPERTY(BlueprintReadWrite)
	TArray<FPlayerPosition> Positions;
};