#pragma once

#include "CoreMinimal.h"
#include "CustomGameInstance.h"
#include "MatchPlayerData.h"
#include "OverrideGameInstance.generated.h"

UCLASS()
class OVERRIDE_API UOverrideGameInstance : public UCustomGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	TArray<FMatchPlayerData> MatchPlayers;
};