#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "UHealthAttributeSet.generated.h"


UCLASS()
class OVERRIDE_API UHealthAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UHealthAttributeSet();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	FGameplayAttributeData Health;
	 ATTRIBUTE_ACCESSORS_BASIC(UHealthAttributeSet, Health);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	FGameplayAttributeData MaxHealth;
	 ATTRIBUTE_ACCESSORS_BASIC(UHealthAttributeSet, MaxHealth);
};
