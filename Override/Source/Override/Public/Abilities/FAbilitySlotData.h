#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "FAbilitySlotData.generated.h"

USTRUCT(BlueprintType)
struct OVERRIDE_API FAbilitySlotData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SlotIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag InputTag;

	UPROPERTY(BlueprintReadOnly)
	FGameplayAbilitySpecHandle AbilityHandle;
};
