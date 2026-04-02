#include "Player/CustomPlayerController.h"
#include "Engine/Engine.h"
#include "GameMode/OverrideGameInstance.h"
#include "Kismet/GameplayStatics.h"

void ACustomPlayerController::BeginPlay()
{
	HttpClient = NewObject<UMasterServerHttpClient>(this);

	Super::BeginPlay();

	const FString LevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);

	if (LevelName == TEXT("Lvl_Lobby"))
	{
		OnLobbyJoined(GetGameInstance<UOverrideGameInstance>()->CurrentLobbyId);
	}
}

void ACustomPlayerController::FetchLobbyList()
{
	HttpClient->ListLobbies(this);
}

void ACustomPlayerController::CreateNewLobby()
{
#if WITH_EDITOR
	OnLobbyJoined("Editor Lobby");
#else
	HttpClient->CreateLobby(this);
#endif
}

void ACustomPlayerController::JoinLobby(FString LobbyId)
{
	HttpClient->JoinLobby(LobbyId, this);
}

void ACustomPlayerController::LeaveLobby(FString LobbyId)
{
	HttpClient->LeaveLobby(LobbyId, this);
}

void ACustomPlayerController::ConnectToLobbyServer(FString LobbyId, FString LobbyIp, int LobbyPort)
{
	GetGameInstance<UOverrideGameInstance>()->CurrentLobbyId = LobbyId;
	
	FString Address = FString::Printf(TEXT("%s:%d"), *LobbyIp, LobbyPort);

	UE_LOG(LogTemp, Log, TEXT("Connecting client to %s..."), *Address);

	this->ClientTravel(Address, TRAVEL_Absolute);
}
