#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "UHealthAttributeSet.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAttributeChangedEvent, UAttributeSet*, AttributeSet, float, OldValue,
                                               float, NewValue);

UCLASS()
class OVERRIDE_API UHealthAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UHealthAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Health, meta = (HideFromModifiers))
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UHealthAttributeSet, Health);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UHealthAttributeSet, MaxHealth);

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UPROPERTY(BlueprintAssignable)
	FAttributeChangedEvent OnHealthChanged;

	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	// Damage value calculated during a GE. Meta attribute.
	UPROPERTY(VisibleAnywhere)
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UHealthAttributeSet, Damage);
};
