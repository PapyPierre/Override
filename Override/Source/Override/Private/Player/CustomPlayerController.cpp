#include "Player/CustomPlayerController.h"

void ACustomPlayerController::FetchLobbyList()
{
	HttpClient->ListLobbies(this);
}

void ACustomPlayerController::InitHttpClient()
{
	HttpClient = NewObject<UMasterServerHttpClient>();
}
