#pragma once

#include "CoreMinimal.h"
#include "Lobby.generated.h"

USTRUCT(BlueprintType)
struct OVERRIDE_API FLobby
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly)
	FString Id = FString("");

	UPROPERTY(BlueprintReadOnly)
	FString Version = FString("");

	UPROPERTY(BlueprintReadOnly)
	int CurrentPlayersCount = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int MaxPlayers = 7;

	FString ServerIp = FString("");

	int ServerPort = 0;

	bool IsInGame = false;
};
