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

void ACustomPlayerController::LeaveLobby(FString LobbyId)
{
	HttpClient->LeaveLobby(LobbyId, this);
}

void ACustomPlayerController::ConnectToLobbyServer(FString LobbyIp, int LobbyPort)
{
	FString Address = FString::Printf(TEXT("%s:%d"), *LobbyIp, LobbyPort);
	
	ClientTravel(Address, ETravelType::TRAVEL_Absolute);
}
