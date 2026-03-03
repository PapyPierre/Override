#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_BaseAbility.generated.h"

UCLASS()
class OVERRIDE_API UGA_BaseAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BaseAbility();

	UFUNCTION(BlueprintCallable, Category = "Hack")
	FGameplayEventData GetEventData() const { return CurrentEventData; }

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Cooldown")
	float CooldownDuration;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Cooldown")
	FGameplayTag CooldownTag;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	bool RequiresTargets;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool SelfCast;

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;
	
	virtual void ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                           FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle,
	                           const FGameplayAbilityActorInfo* ActorInfo,
	                           FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const override;

protected:
	FGameplayEventData CurrentEventData;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
								 const FGameplayAbilityActorInfo* ActorInfo, 
								 const FGameplayAbilityActivationInfo ActivationInfo, 
								 const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Ability")
	void OnAbilityActivated(FVector Location, const TArray<AActor*>& Targets);
};