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
	static void SendDataToDB();

private:
	static FString GetVersionFromFile(const FString& FilePath);
};