#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FirstPersonGameMode.generated.h"

UCLASS()
class OVERRIDE_API AFirstPersonGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable)
	void SendDataToDB();
	
	static bool RecvAll(FSocket* Socket, FString& OutResponse);

	static bool ParseJsonSafe(const FString& JsonString, TSharedPtr<FJsonObject>& OutJson);

private:
	static FString GetVersionFromFile(const FString& FilePath);
};