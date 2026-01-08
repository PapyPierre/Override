#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Hacks/GA_BaseAbility.h"
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

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Hack")
	TArray<TSubclassOf<UGA_BaseAbility>> CharacterAbilities;

	bool GetCdRemainingForTag(FGameplayTagContainer CdTags, float & TimeRemaining, float & CdDuration) const;
	
#pragma region Attribute
	
	UPROPERTY()
	TObjectPtr<class UHealthAttributeSet> HealthSet;
	
#pragma endregion

protected:
	virtual void BeginPlay() override;
	
	virtual void GiveCharacterAbilities();
};
