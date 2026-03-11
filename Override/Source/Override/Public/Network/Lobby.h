#pragma once

#include "CoreMinimal.h"
#include "Lobby.generated.h"

USTRUCT(BlueprintType)
struct OVERRIDE_API FLobby
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly)
	FString Id;

	UPROPERTY(BlueprintReadOnly)
	FString Version;

	UPROPERTY(BlueprintReadOnly)
	int CurrentPlayersCount;
	
	UPROPERTY(BlueprintReadOnly)
	int MaxPlayers;

	FString ServerIp;

	int ServerPort;
};
