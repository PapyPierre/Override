#pragma once

#include "CoreMinimal.h"
#include "CustomGameInstance.h"
#include "OverrideGameInstance.generated.h"

UCLASS()
class OVERRIDE_API UOverrideGameInstance : public UCustomGameInstance
{
	GENERATED_BODY()

public:
	TArray<FMatchPlayerData> MatchPlayers;
};

	
USTRUCT()
struct FMatchPlayerData
{
	GENERATED_BODY()
		
	int32 PlayerId;
	int32 TeamId;
	TArray<FPlayerPosition> Positions;
};

USTRUCT()
struct FPlayerPosition
{
	GENERATED_BODY()
	
	int32 Tick;
	FVector Position;
};