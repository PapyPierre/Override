#pragma once

#include "FAbilitySlotData.h"
#include "AbilitySlotComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UAbilitySlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilitySlotComponent();
	
	void Init();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Slots")
	int32 MaxSlots = 2;

	UPROPERTY(BlueprintReadOnly, Category = "Ability Slots")
	TArray<FAbilitySlotData> AbilitySlots;

	UFUNCTION(BlueprintCallable, Category = "Ability Slots")
	void AssignAbilityToSlot(TSubclassOf<UGameplayAbility> AbilityClass, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Ability Slots")
	void RemoveAbilityFromSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Ability Slots")
	void SwapAbilities(int32 SlotA, int32 SlotB);

	UFUNCTION(BlueprintCallable, Category = "Ability Slots")
	FAbilitySlotData GetAbilityInSlot(int32 SlotIndex) const;

private:
	UPROPERTY()
	UAbilitySystemComponent* ASC;

	void InitializeSlots();
};
