#pragma once

#include "CoreMinimal.h"
#include "Network/Lobby.h"
#include "Network/MasterServerHttpClient.h"
#include "UObject/Object.h"
#include "CustomPlayerController.generated.h"

UCLASS()
class OVERRIDE_API ACustomPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void FetchLobbyList();

	UFUNCTION(BlueprintCallable)
	void CreateNewLobby();

	UFUNCTION(BlueprintCallable)
	void JoinLobby(FString LobbyId);
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnLobbyListReceived(const TArray<FLobby>& Lobbies);

	UFUNCTION(BlueprintImplementableEvent)
	void OnLobbyCreated(const FString& LobbyId);

	UFUNCTION(BlueprintImplementableEvent)
	void OnLobbyJoined();
	
	
private:
	UPROPERTY()
	TObjectPtr<UMasterServerHttpClient> HttpClient;
};
