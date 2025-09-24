#include "Components/HackingComponent.h"

UHackingComponent::UHackingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UHackingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UHackingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

