#include "Components/TargetingComponent.h"

UTargetingComponent::UTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTargetingComponent::LookForTarget()
{
	if (!PlayerController) return;

	FVector CamPos;
	FRotator CameraRot;
	PlayerController->GetPlayerViewPoint(CamPos, CameraRot);

	FVector EndPoint = CamPos + CameraRot.Vector() * DetectionDistance_View;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	GetWorld()->LineTraceSingleByChannel(HitResult, CamPos, EndPoint, ECC_Visibility, QueryParams);

	//DrawDebugLine(GetWorld(), CamPos, EndPoint, FColor::Yellow, false);

	AActor* HitActor = HitResult.GetActor();

	if (HitActor && HitActor->Implements<UTargetable>())
	{
		if (!CurrentTargets.Contains(HitActor))
		{
			for (auto target : CurrentTargets)
			{
				ITargetable::Execute_OnUntarget(target);
			}

			ActorInSight = HitActor;
			CurrentTargets = ActorInSight;
		}
	}
	else
	{
		if (ActorInSight) ITargetable::Execute_OnUntarget(ActorInSight);
		ActorInSight = nullptr;

		AActor* newTarget = FindClosestTarget();

		if (!CurrentTargets.IsEmpty())
		{
			if (CurrentTargets != newTarget)
			{
				ITargetable::Execute_OnUntarget(CurrentTargets);
				CurrentTargets = newTarget;
			}
		}
		else
		{
			CurrentTargets.Add(newTarget);
		}
	}

	if (CurrentTargets.IsEmpty()) return;

	for (auto actor : CurrentTargets)
	{
		ITargetable::Execute_OnTarget(actor);
	}
}

AActor* UTargetingComponent::FindClosestTarget() const
{
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	TArray<AActor*> FoundTargetable;

	bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(OverlapResults, GetOwner()->GetActorLocation(),
	                                                        FQuat::Identity,
	                                                        ObjectQueryParams,
	                                                        FCollisionShape::MakeSphere(DetectionDistance_Auto),
	                                                        QueryParams);

	//DrawDebugSphere(GetWorld(), GetActorLocation(), HackDetectionDistance_Auto, 12, FColor::Yellow, false);

	if (bHasOverlap)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* Actor = Result.GetActor();
			if (Actor && Actor->Implements<UTargetable>())
			{
				FoundTargetable.Add(Actor);
			}
		}
	}

	float closestDist = DetectionDistance_View + 1;
	AActor* closestTargetable = nullptr;

	for (AActor* TargetableActor : FoundTargetable)
	{
		float dist = FVector::Dist(GetOwner()->GetActorLocation(), TargetableActor->GetActorLocation());
		if (dist < closestDist)
		{
			closestTargetable = TargetableActor;
			closestDist = dist;
		}
	}

	return closestTargetable;
}

void UTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	LookForTarget();


	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
