#include "Abilities/GA_BaseAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/CustomAbilityTargetData.h"
#include "Modulations/Modulation.h"

class AModulation;

UGA_BaseAbility::UGA_BaseAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_BaseAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!CooldownEffectClass) return;
	
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownEffectClass, GetAbilityLevel());

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetDuration(CooldownDuration, true);
		SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);

		auto GEHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

bool UGA_BaseAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle,
									   const FGameplayAbilityActorInfo* ActorInfo,
								   FGameplayTagContainer* OptionalRelevantTags) const
{
	const bool bParentCooldown = Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
	
	if (CooldownTag.IsValid() && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const bool bCustomCooldown = !ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(CooldownTag);
		return bParentCooldown && bCustomCooldown;
	}
	
	return bParentCooldown;
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
	if (!HasAuthority(&ActivationInfo)) return;
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}

	FVector Location = FVector::ZeroVector;
	TArray<AActor*> Targets;

	FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);

	if (!Spec)
	{
		UE_LOG(LogTemp, Error, TEXT("ABILITY : Could not find Spec from Handle!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (Spec && Spec->GameplayEventData.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("ABILITY : GameplayEventData is valid"));
		
		if (Spec->GameplayEventData->ContextHandle.IsValid())
		{
			Location = Spec->GameplayEventData->ContextHandle.GetOrigin();
			//UE_LOG(LogTemp, Log, TEXT("ABILITY : Extracted Location: %s"), *Location.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ABILITY : ContextHandle is INVALID!"));
		}
		
		const FGameplayAbilityTargetDataHandle& TargetDataHandle = Spec->GameplayEventData->TargetData;

		if (TargetDataHandle.IsValid(0))
		{
			//UE_LOG(LogTemp, Log, TEXT("ABILITY : TargetDataHandle is valid"));
			
			const FCustomAbilityTargetData* CustomData = 
				static_cast<const FCustomAbilityTargetData*>(TargetDataHandle.Get(0));

			if (CustomData)
			{
				for (TWeakObjectPtr<AActor> TargetPtr : CustomData->Targets)
				{
					if (AActor* Target = TargetPtr.Get())
					{
						Targets.Add(Target);
						//UE_LOG(LogTemp, Log, TEXT("ABILITY : Found target: %s"), *Target->GetName());
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ABILITY : Failed to cast to FCustomAbilityTargetData!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ABILITY : TargetDataHandle is INVALID at index 0!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ABILITY : GameplayEventData is INVALID!"));
	}

	/*
	UE_LOG(LogTemp, Log, TEXT("ABILITY : Calling OnAbilityActivated with Location=%s, Targets=%d"),
			   *Location.ToString(), Targets.Num());
	*/
	OnAbilityActivated(Location, Targets);

	//UE_LOG(LogTemp, Log, TEXT("SERVER:  %s Activate %s"), *ActorInfo->OwnerActor->GetName(), *this->GetName());
	
	ApplyCooldown(Handle, ActorInfo, ActivationInfo);
}

void UGA_BaseAbility::OnAbilityActivated_Implementation(FVector Location, const TArray<AActor*>& Targets)
{
	
}
