#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GA_BaseAbility.h"
#include "CustomPlayerState.generated.h"

UCLASS()
class OVERRIDE_API ACustomPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACustomPlayerState();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Hacking)
	TObjectPtr<class UAbilitySystemComponent> Asc;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	bool GetCdRemainingForTag(FGameplayTagContainer CdTags, float & TimeRemaining, float & CdDuration) const;
	
#pragma region Attribute
	
	UPROPERTY()
	TObjectPtr<class UHealthAttributeSet> HealthSet;
	
#pragma endregion
};
