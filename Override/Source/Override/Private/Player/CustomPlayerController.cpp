#include "Player/CustomPlayerController.h"

void ACustomPlayerController::BeginPlay()
{
	HttpClient = NewObject<UMasterServerHttpClient>(this);
	
	Super::BeginPlay();
}

void ACustomPlayerController::FetchLobbyList()
{
	HttpClient->ListLobbies(this);
}

void ACustomPlayerController::CreateNewLobby()
{
	HttpClient->CreateLobby(this);
}

void ACustomPlayerController::JoinLobby(FString LobbyId)
{
	HttpClient->JoinLobby(LobbyId, this);
}
