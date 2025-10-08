#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BaseHack.generated.h"

UCLASS()
class OVERRIDE_API UBaseHack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBaseHack();

	UFUNCTION(BlueprintCallable, Category = "Hack")
	FGameplayEventData GetEventData() const { return CurrentEventData; }

protected:
	FGameplayEventData CurrentEventData;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
								 const FGameplayAbilityActorInfo* ActorInfo, 
								 const FGameplayAbilityActivationInfo ActivationInfo, 
								 const FGameplayEventData* TriggerEventData) override;
};
