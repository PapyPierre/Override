// 


#include "GameMode/OverrideGameInstance.h"

void UOverrideGameInstance::Init()
{
	Super::Init();

	FString LobbyId;
	FParse::Value(FCommandLine::Get(), TEXT("lobbyId="), LobbyId);

	CurrentLobbyId = LobbyId;
}
