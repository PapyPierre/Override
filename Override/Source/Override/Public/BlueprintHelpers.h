

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintHelpers.generated.h"

UCLASS()
class OVERRIDE_API UBlueprintHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintPure, Category="Game|Version")
	static FString GetProjectVersion();
};
