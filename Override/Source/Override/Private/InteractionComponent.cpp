#include "InteractionComponent.h"
#include "Hackable.h"
#include "Engine/OverlapResult.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	LookForHackable();

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UInteractionComponent::TryInteract()
{
	if (!DefaultTargetToHack->Implements<UHackable>()) return;
	
	if (GetOwner()->HasAuthority())
	{
		if (DefaultTargetToHack) IHackable::Execute_TryHack(DefaultTargetToHack);
	}
	else
	{
		RPC_TryInteractWithActor(DefaultTargetToHack);
	}
}

void UInteractionComponent::LookForHackable()
{
	if (!PlayerController) return;

	FVector CamPos;
	FRotator CameraRot;
	PlayerController->GetPlayerViewPoint(CamPos, CameraRot);

	FVector EndPoint = CamPos + CameraRot.Vector() * HackDetectionDistance_View;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, CamPos, EndPoint,
	                                                 ECC_Visibility, QueryParams);

	//DrawDebugLine(GetWorld(), CamPos, EndPoint, FColor::Yellow, false);

	AActor* HitActor = HitResult.GetActor();

	if (HitActor && HitActor->Implements<UHackable>())
	{
		AActor* NewHackableActor = HitActor;

		if (NewHackableActor && NewHackableActor != HackableActorInSight)
		{
			if (HackableActorInSight) IHackable::Execute_OnUnselect(HackableActorInSight);

			HackableActorInSight = NewHackableActor;

			if (DefaultTargetToHack && DefaultTargetToHack != HackableActorInSight)
			{
				if (DefaultTargetToHack) IHackable::Execute_OnUnselect(DefaultTargetToHack);
			}

			DefaultTargetToHack = HackableActorInSight;
		}
	}
	else
	{
		if (HackableActorInSight) IHackable::Execute_OnUnselect(HackableActorInSight);
		HackableActorInSight = nullptr;

		AActor* NewHackableActor = FindClosestHackableActor();

		if (DefaultTargetToHack)
		{
			if (DefaultTargetToHack != NewHackableActor)
			{
				IHackable::Execute_OnUnselect(DefaultTargetToHack);
				DefaultTargetToHack = NewHackableActor;
			}
		}
		else
		{
			DefaultTargetToHack = NewHackableActor;
		}
	}

	if (DefaultTargetToHack == nullptr) return;

	IHackable::Execute_OnSelect(DefaultTargetToHack);
}

AActor* UInteractionComponent::FindClosestHackableActor() const
{
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	TArray<AActor*> FoundHackable;

	bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(OverlapResults, GetOwner()->GetActorLocation(),
	                                                        FQuat::Identity,
	                                                        ObjectQueryParams,
	                                                        FCollisionShape::MakeSphere(HackDetectionDistance_Auto),
	                                                        QueryParams);

	//DrawDebugSphere(GetWorld(), GetActorLocation(), HackDetectionDistance_Auto, 12, FColor::Yellow, false);

	if (bHasOverlap)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* Actor = Result.GetActor();
			if (Actor && Actor->Implements<UHackable>())
			{
				FoundHackable.Add(Actor);
			}
		}
	}

	float closestDist = HackDetectionDistance_View + 1;
	AActor* closestHackable = nullptr;

	for (AActor* HackableActor : FoundHackable)
	{
		float dist = FVector::Dist(GetOwner()->GetActorLocation(), HackableActor->GetActorLocation());
		if (dist < closestDist)
		{
			closestHackable = HackableActor;
			closestDist = dist;
		}
	}

	return closestHackable;
}

void UInteractionComponent::RPC_TryInteractWithActor_Implementation(AActor* Target)
{
	if (Target) IHackable::Execute_TryHack(Target);
}