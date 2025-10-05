#include "Player/CustomPlayerState.h"
#include "Attribute/UHealthAttributeSet.h"
#include "Player/PlayerCharacter.h"

ACustomPlayerState::ACustomPlayerState()
{
	bReplicates = true;
	
    SetNetUpdateFrequency(10.0f); // 1.0f by default
	SetMinNetUpdateFrequency(2.0f); // 1.0f by default

	HealthSet = CreateDefaultSubobject<UHealthAttributeSet>(TEXT("HealthSet"));
	check(HealthSet);
	
	Asc = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	Asc->SetIsReplicated(true);
	Asc->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	check(Asc);
}

UAbilitySystemComponent* ACustomPlayerState::GetAbilitySystemComponent() const
{
	return Asc;
}
