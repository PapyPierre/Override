#pragma once

#include "CoreMinimal.h"
#include "Lobby.generated.h"

USTRUCT(BlueprintType)
struct OVERRIDE_API FLobby
{
	GENERATED_BODY()
	
public:
	FString Id;

	FString Version;

	int MaxPlayers;

	int CurrentPlayersCount;

	FString ServerIp;

	int ServerPort;
};
