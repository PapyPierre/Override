#include "Components/InteractingComponent.h"
#include "Engine/OverlapResult.h"
#include "Interface/Hackable.h"
#include "Interface/Interactable.h"

UInteractingComponent::UInteractingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UInteractingComponent::TryInteractWithActor(AActor* Target)
{
	if (!Target->Implements<UInteractable>()) return;

	RPC_TryInteractWithActor(Target);
}

void UInteractingComponent::RPC_TryInteractWithActor_Implementation(AActor* Target)
{
	if (Target) Cast<IInteractable>(Target)->Interact();
}