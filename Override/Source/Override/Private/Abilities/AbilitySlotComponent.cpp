#include "Abilities/AbilitySlotComponent.h"
#include "AbilitySystemComponent.h"
#include "Player/PlayerCharacter.h"

UAbilitySlotComponent::UAbilitySlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAbilitySlotComponent::Init()
{
	if (const APlayerCharacter* Owner = Cast<APlayerCharacter>(GetOwner()))
	{
		ASC = Owner->GetAbilitySystemComponent();
		InitializeSlots();
	}
}

void UAbilitySlotComponent::InitializeSlots()
{
	AbilitySlots.SetNum(MaxSlots);

	for (int32 i = 0; i < MaxSlots; i++)
	{
		AbilitySlots[i].SlotIndex = i;

		// Définir les InputTags par défaut (Ability.Slot.1, Ability.Slot.2, etc.)
		AbilitySlots[i].InputTag = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("Ability.Slot.%d"), i + 1))
		);
	}
}

void UAbilitySlotComponent::AssignAbilityToSlot(TSubclassOf<UGameplayAbility> AbilityClass, int32 SlotIndex)
{
	if (!ASC || !AbilityClass || SlotIndex < 0 || SlotIndex >= MaxSlots)
		return;

	RemoveAbilityFromSlot(SlotIndex);

	FGameplayAbilitySpec AbilitySpec(AbilityClass,1, SlotIndex);
	AbilitySpec.InputID = SlotIndex;

	AbilitySlots[SlotIndex].AbilityClass = AbilityClass;
	AbilitySlots[SlotIndex].AbilityHandle = ASC->GiveAbility(AbilitySpec);

	UE_LOG(LogTemp, Log, TEXT("Gave: %s to %s, handle: %s"),
	       *AbilityClass->GetName(), *GetOwner()->GetName(), *AbilitySlots[SlotIndex].AbilityHandle.ToString());
}

void UAbilitySlotComponent::RemoveAbilityFromSlot(int32 SlotIndex)
{
	if (!ASC || SlotIndex < 0 || SlotIndex >= MaxSlots)
		return;

	FAbilitySlotData& Slot = AbilitySlots[SlotIndex];

	if (Slot.AbilityHandle.IsValid())
	{
		ASC->ClearAbility(Slot.AbilityHandle);
		Slot.AbilityHandle = FGameplayAbilitySpecHandle();
		Slot.AbilityClass = nullptr;
	}
}

void UAbilitySlotComponent::SwapAbilities(int32 SlotA, int32 SlotB)
{
	if (SlotA < 0 || SlotA >= MaxSlots || SlotB < 0 || SlotB >= MaxSlots)
		return;

	TSubclassOf<UGameplayAbility> TempClass = AbilitySlots[SlotA].AbilityClass;
	FGameplayAbilitySpecHandle TempHandle = AbilitySlots[SlotA].AbilityHandle;

	AssignAbilityToSlot(AbilitySlots[SlotB].AbilityClass, SlotA);
	AssignAbilityToSlot(TempClass, SlotB);
}

FAbilitySlotData UAbilitySlotComponent::GetAbilityInSlot(int32 SlotIndex) const
{
	if (SlotIndex >= 0 && SlotIndex < AbilitySlots.Num())
	{
		return AbilitySlots[SlotIndex];
	}

	return FAbilitySlotData();
}