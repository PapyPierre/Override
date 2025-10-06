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

void ACustomPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GiveCharacterHacks();
	}
}

void ACustomPlayerState::GiveCharacterHacks()
{
	if (!GetAbilitySystemComponent()) return;
	
	for (TSubclassOf<UBaseHack>& Hack : CharacterHacks)
	{
		FGameplayAbilitySpec HackSpec(Hack, 1, INDEX_NONE, this);
		FGameplayAbilitySpecHandle Handle = GetAbilitySystemComponent()->GiveAbility(HackSpec);
		
		UE_LOG(LogTemp, Warning, TEXT("Ability donnée: %s, Handle valide: %s"),
			   *Hack->GetName(),
			   Handle.IsValid() ? TEXT("OUI") : TEXT("NON"));
	}
}
