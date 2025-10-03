#include "Player/CustomPlayerState.h"
#include "Attribute/UHealthAttributeSet.h"
#include "Player/PlayerCharacter.h"

ACustomPlayerState::ACustomPlayerState()
{
	bReplicates = true;
	HealthSet = CreateDefaultSubobject<UHealthAttributeSet>(TEXT("HealthSet"));
	check(HealthSet);
	Asc = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	check(Asc);
}

UAbilitySystemComponent* ACustomPlayerState::GetAbilitySystemComponent() const
{
	return Asc;
}
