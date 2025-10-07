#include "Modulations/ModulationGroup.h"
#include "Components/TargetingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Modulations/Modulation.h"
#include "Player/PlayerCharacter.h"


AModulationGroup::AModulationGroup()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AModulationGroup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UWorld* World = GetWorld();
	if (!World) return;

#if WITH_EDITOR
	for (AModulation* mod : ModulationsInGroup)
	{
		if (!IsValid(mod)) continue;
		DrawDebugLine(World, GetActorLocation(), mod->GetActorLocation(), FColor::Green, true, -1.0);
	}
#endif
}

void AModulationGroup::TargetGroup()
{
	ACharacter* Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	APlayerCharacter* LocalPlayer = static_cast<APlayerCharacter*>(Character);

	for (AModulation* mod : ModulationsInGroup)
	{
		UTargetingComponent* TargetingComponent = LocalPlayer->FindComponentByClass<UTargetingComponent>();
		TargetingComponent->TargetActor(mod);
	}
}

void AModulationGroup::BeginPlay()
{
	for (AModulation* mod : ModulationsInGroup)
	{
		mod->Group = this;
	}

	Super::BeginPlay();
}

void AModulationGroup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
