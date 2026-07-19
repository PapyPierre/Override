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
	int32 KillsCount = -1;

	UPROPERTY(BlueprintReadWrite)
	int32 DeathsCount = -1;

	// Nb of flags scored
	UPROPERTY(BlueprintReadWrite)
	int32 FlagsCount = -1;

	UPROPERTY(BlueprintReadWrite)
	bool HasWon = false;

	UPROPERTY(BlueprintReadWrite)
	TArray<FPlayerPosition> Positions;
};