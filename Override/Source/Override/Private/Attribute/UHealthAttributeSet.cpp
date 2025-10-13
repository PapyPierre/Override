#include "Attribute/UHealthAttributeSet.h"
#include "Net/UnrealNetwork.h"
 
UHealthAttributeSet::UHealthAttributeSet()
{
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
}
 
void UHealthAttributeSet::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHealthAttributeSet, MaxHealth);
	DOREPLIFETIME(UHealthAttributeSet, Health);
}

void UHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	Super::PreAttributeChange(Attribute, NewValue);
}

void UHealthAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		OnHealthChanged.Broadcast(this, OldValue, NewValue);
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		// When max health changes, broadcast OnHealthChanged so that health bars will update
		const float CurrentHealth = GetHealth();
		OnHealthChanged.Broadcast(this, CurrentHealth, CurrentHealth);
	}
}

void UHealthAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHealthAttributeSet, Health, OldValue);
	const float OldHealth = OldValue.GetCurrentValue();
	const float NewHealth = GetHealth();
	OnHealthChanged.Broadcast(this, OldHealth, NewHealth);
}

void UHealthAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHealthAttributeSet, MaxHealth, OldValue);
 
	// When max health changes, broadcast OnHealthChanged so that health bars will update
	const float CurrentHealth = GetHealth();
	OnHealthChanged.Broadcast(this, CurrentHealth, CurrentHealth);
}

void UHealthAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// Convert into -Health and then clamp
		const float OldHealthValue = GetHealth();
		const float NewHealthValue = FMath::Clamp(OldHealthValue - GetDamage(), 0.0f, GetMaxHealth());
 
		if (OldHealthValue != NewHealthValue)
		{
			// Set the new health after clamping to min-max
			SetHealth(NewHealthValue);
		}
 
		// Clear the meta attribute that temporarily held damage
		SetDamage(0.0f);
	}

	if (Data.EvaluatedData.Attribute == GetHealAttribute())
	{
		const float OldHealthValue = GetHealth();
		const float NewHealthValue = FMath::Clamp(OldHealthValue + GetHeal(), 0.0f, GetMaxHealth());
 
		if (OldHealthValue != NewHealthValue)
		{
			SetHealth(NewHealthValue);
		}
		
		SetHeal(0.0f);
	}
}
