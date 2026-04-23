#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FirstPersonGameMode.generated.h"

class UServerHttpClient;

UCLASS()
class OVERRIDE_API AFirstPersonGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	void BeginPlay() override;
	
protected:
	UFUNCTION(BlueprintCallable)
	void SendDataToDB();

private:
	virtual void Logout(AController* Exiting) override;
	
	static FString GetVersionFromFile(const FString& FilePath);

	UPROPERTY()
	TObjectPtr<UServerHttpClient> HttpClient;
};