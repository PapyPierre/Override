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
	Asc->SetReplicationMode(EGameplayEffectReplicationMode::Full);
	check(Asc);
}

UAbilitySystemComponent* ACustomPlayerState::GetAbilitySystemComponent() const
{
	return Asc;
}

bool ACustomPlayerState::GetCdRemainingForTag(FGameplayTagContainer CdTags, float& TimeRemaining, float& CdDuration) const
{
	if (Asc && CdTags.Num() > 0)
	{
		TimeRemaining = 0.f;
		CdDuration = 0.f;

		FGameplayEffectQuery const Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CdTags);
		TArray<TPair<float, float>> DurationAndTimeRemaining = Asc->GetActiveEffectsTimeRemainingAndDuration(Query);
		if (DurationAndTimeRemaining.Num() > 0)
		{
			int32 BestIdx = 0;
			float LongestTime = DurationAndTimeRemaining[0].Key;
			for (int32 Idx = 1; Idx < DurationAndTimeRemaining.Num(); ++Idx)
			{
				if (DurationAndTimeRemaining[Idx].Key > LongestTime)
				{
					LongestTime = DurationAndTimeRemaining[Idx].Key;
					BestIdx = Idx;
				}
			}

			TimeRemaining = DurationAndTimeRemaining[BestIdx].Key;
			CdDuration = DurationAndTimeRemaining[BestIdx].Value;

			return true;
		}
	}

	return false;
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

		UE_LOG(LogTemp, Warning, TEXT("Server hack gave: %s to %s"), *Hack->GetName(), *this->GetName());
	}
}
