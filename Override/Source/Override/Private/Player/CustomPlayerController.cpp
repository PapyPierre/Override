#include "Player/CustomPlayerController.h"
#include "Engine/Engine.h"
#include "GameMode/OverrideGameInstance.h"
#include "Kismet/GameplayStatics.h"

void ACustomPlayerController::BeginPlay()
{
	HttpClient = NewObject<UServerHttpClient>(this);

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

void ACustomPlayerController::OnLogout() // Server-Sided
{
	FString LobbyId = GetGameInstance<UOverrideGameInstance>()->CurrentLobbyId;
	
	UE_LOG(LogTemp, Log, TEXT("ACustomPlayerController::OnLogout"));

	HttpClient->NotifyLeaveLobby(LobbyId, this, true);
}

void ACustomPlayerController::SetLobbyInGame(FString LobbyId, bool Value) // Server-Sided
{
	HttpClient->SetLobbyInGame(LobbyId, Value);
}

void ACustomPlayerController::LeaveLobby(FString LobbyId)
{
#if WITH_EDITOR
	OnNotifyLobbyLeft();
#else
	UE_LOG(LogTemp, Log, TEXT("ACustomPlayerController::LeaveLobby, Disconnecting client"));

	ClientTravel("/Game/Maps/Lvl_Menu", TRAVEL_Absolute);
	//ConsoleCommand("disconnect"); // To disconnect client of game server instance
	HttpClient->NotifyLeaveLobby(LobbyId, this, false);
#endif
}

void ACustomPlayerController::ConnectToLobbyServer(FString LobbyId, FString LobbyIp, int LobbyPort)
{
	GetGameInstance<UOverrideGameInstance>()->CurrentLobbyId = LobbyId;
	
	FString Address = FString::Printf(TEXT("%s:%d"), *LobbyIp, LobbyPort);

	UE_LOG(LogTemp, Log, TEXT("Connecting client to %s..."), *Address);

	this->ClientTravel(Address, TRAVEL_Absolute);
}