#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Pingable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPingable : public UInterface
{
	GENERATED_BODY()
};

class OVERRIDE_API IPingable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Pingable")
	void Ping(float LifeTime);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Pingable")
	void StopPing();
};
