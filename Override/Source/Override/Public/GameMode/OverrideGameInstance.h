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
	virtual void Init() override;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<FMatchPlayerData> MatchPlayers;

	UPROPERTY(BlueprintReadOnly)
	FString CurrentLobbyId = FString("");
};