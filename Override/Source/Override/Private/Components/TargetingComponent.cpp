#include "Components/TargetingComponent.h"
#include "Interface/Targetable.h"

UTargetingComponent::UTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTargetingComponent::LookForTarget(float TargetingRange)
{
	if (!PlayerController) return;

	TArray<AActor*> ActorsInRange = FindTargetablesInRange(TargetingRange);
	TArray<AActor*> ActorsInFrustum;

	for (AActor* ActorInRange : ActorsInRange)
	{
		if (IsActorInFrustumWithPadding(PlayerController, ActorInRange, ScreenPadding))
		{
			ActorsInFrustum.Add(ActorInRange);
		}
	}

	if (ActorsInFrustum.Num() == 0)
	{
		ClearCurrentTargets();
		return;
	}

	AActor* Target = GetClosestActorToCursor(PlayerController, ActorsInFrustum);
	TargetActor(Target);
}

bool UTargetingComponent::IsActorInFrustumWithPadding(APlayerController* PC, AActor* Actor, float Padding)
{
	if (!PC || !Actor) return false;

	FVector2D ScreenPos;
	FVector WorldPos = Actor->GetActorLocation();

	bool bOnScreen = PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos);
	if (!bOnScreen) return false;

	int32 ViewportX, ViewportY;
	PC->GetViewportSize(ViewportX, ViewportY);

	const float MinX = -Padding;
	const float MinY = -Padding;
	const float MaxX = ViewportX + Padding;
	const float MaxY = ViewportY + Padding;

	return (ScreenPos.X >= MinX && ScreenPos.X <= MaxX &&
		ScreenPos.Y >= MinY && ScreenPos.Y <= MaxY);
}

TArray<AActor*> UTargetingComponent::FindTargetablesInRange(const float Range) const
{
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(OverlapResults, GetOwner()->GetActorLocation(),
	                                                        FQuat::Identity,
	                                                        ObjectQueryParams,
	                                                        FCollisionShape::MakeSphere(Range),
	                                                        QueryParams);

	DrawDebugSphere(GetWorld(), GetOwner()->GetActorLocation(), Range, 24, FColor::Yellow, false);

	TArray<AActor*> FoundTargetable;

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

	return FoundTargetable;
}

AActor* UTargetingComponent::GetClosestActorToCursor(APlayerController* PC, const TArray<AActor*>& Actors)
{
	if (!PC || Actors.Num() == 0) return nullptr;

	int32 ViewportX, ViewportY;
	PC->GetViewportSize(ViewportX, ViewportY);
	FVector2D ScreenCenter(ViewportX * 0.5f, ViewportY * 0.5f);

	AActor* ClosestActor = nullptr;
	float ClosestDist = TNumericLimits<float>::Max();

	for (AActor* Actor : Actors)
	{
		if (!Actor) continue;

		FVector2D ScreenPos;
		if (PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenPos))
		{
			float Dist = FVector2D::Distance(ScreenCenter, ScreenPos);
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				ClosestActor = Actor;
			}
		}
	}

	return ClosestActor;
}

void UTargetingComponent::TargetActor(AActor* Target)
{
	if (!Target) return;
	if (!Target->Implements<UTargetable>()) return;

	if (CurrentTargets.Num() > 0)
	{
		if (CurrentTargets.Contains(Target)) return;
		ClearCurrentTargets();
	}

	//GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Yellow, TEXT("Target:" + Target->GetName() + "!"));

	CurrentTargets.Add(Target);
	ITargetable::Execute_OnTarget(Target);
}

void UTargetingComponent::ClearCurrentTargets()
{
	for (AActor* Targetable : CurrentTargets)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Yellow, TEXT("Untarget:" + Targetable->GetName() + "!"));
		ITargetable::Execute_OnUntarget(Targetable);
	}

	CurrentTargets.Empty();
}

void UTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	LookForTarget(MaxTargetingDistance);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
