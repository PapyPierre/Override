#pragma once

#include "MatchPlayerData.h"
#include "MatchData.generated.h"

USTRUCT(BlueprintType)
struct OVERRIDE_API FMatchData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	FString Id;

	UPROPERTY(BlueprintReadWrite)
	FString Version;

	UPROPERTY(BlueprintReadWrite)
	TArray<FMatchPlayerData> Players;
};