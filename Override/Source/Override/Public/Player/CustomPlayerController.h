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
	UFUNCTION(BlueprintCallable)
	void FetchLobbyList();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnLobbyListReceived(const TArray<FLobby>& Lobbies);
	
private:
	UPROPERTY()
	TObjectPtr<UMasterServerHttpClient> HttpClient = NewObject<UMasterServerHttpClient>();
};
