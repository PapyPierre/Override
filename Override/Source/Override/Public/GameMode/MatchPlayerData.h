#pragma once

#include "PlayerPosition.h"
#include "MatchPlayerData.generated.h"

USTRUCT(BlueprintType)
struct OVERRIDE_API FMatchPlayerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 PlayerId;
	
	UPROPERTY(BlueprintReadWrite)
	int32 TeamId;

	UPROPERTY(BlueprintReadWrite)
	TArray<FPlayerPosition> Positions;
};