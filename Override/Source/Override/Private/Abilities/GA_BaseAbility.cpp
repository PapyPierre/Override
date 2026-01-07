#include "Abilities/GA_BaseAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayHackTargetData.h"
#include "Modulations/Modulation.h"

class AModulation;

UGA_BaseAbility::UGA_BaseAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_BaseAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo) const
{
	/*
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	
	if (CooldownGE)
	{
		FGameplayEffectSpecHandle SpecHandle =
			MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());
		
		SpecHandle.Data.Get()->DynamicGrantedTags.AppendTags(CooldownTags);
		
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag(FName(CooldownTags.First().GetTagName())),
			CooldownDuration.GetValueAtLevel(GetAbilityLevel()));
		
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
	*/

	if (!CooldownEffectClass) return;
	
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownEffectClass, GetAbilityLevel());

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetDuration(CooldownDuration, true);
		SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
		
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

bool UGA_BaseAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle,
									   const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (CooldownTag.IsValid() && ActorInfo->AbilitySystemComponent.IsValid())
	{
		return !ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(CooldownTag);
	}
	
	return true;
}

float UGA_BaseAbility::GetCooldownTimeRemaining(
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (CooldownTag.IsValid() && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayEffectQuery Query;
		Query.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchTag(CooldownTag);
        
		TArray<float> Durations = ActorInfo->AbilitySystemComponent->GetActiveEffectsTimeRemaining(Query);
        
		if (Durations.Num() > 0)
		{
			return Durations[0];
		}
	}
	return 0.0f;
}

void UGA_BaseAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      const FGameplayEventData* TriggerEventData)
{
	if (TriggerEventData) CurrentEventData = *TriggerEventData;

	UE_LOG(LogTemp, Warning, TEXT("SERVER:  %s Activate %s"), *CurrentEventData.Instigator.GetName(),
	       *CurrentEventData.EventTag.ToString());

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}
