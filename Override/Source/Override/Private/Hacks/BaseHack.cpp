#include "Hacks/BaseHack.h"

UBaseHack::UBaseHack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UBaseHack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                const FGameplayAbilityActorInfo* ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo,
                                const FGameplayEventData* TriggerEventData)
{
	if (TriggerEventData) CurrentEventData = *TriggerEventData;
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}
