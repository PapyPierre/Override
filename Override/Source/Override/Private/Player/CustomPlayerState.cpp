#include "Player/CustomPlayerState.h"
#include "Player/PlayerCharacter.h"
#include "Components/HackingComponent.h"

ACustomPlayerState::ACustomPlayerState()
{
	HackingComponent = CreateDefaultSubobject<UHackingComponent>(TEXT("AbilitySystem"));
}

void ACustomPlayerState::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* MyChar = nullptr;

	if (APlayerController* PC = Cast<APlayerController>(GetOwner())) MyChar = Cast<ACharacter>(PC->GetPawn());
	
	HackingComponent->InitAbilityActorInfo(this, MyChar);
}

UAbilitySystemComponent* ACustomPlayerState::GetAbilitySystemComponent() const
{
	return HackingComponent;
}
