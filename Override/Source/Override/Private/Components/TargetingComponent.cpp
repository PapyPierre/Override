#include "Components/TargetingComponent.h"
#include "Interface/Targetable.h"
#include "Net/UnrealNetwork.h"
#include "Player/PlayerCharacter.h"

UTargetingComponent::UTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UTargetingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTargetingComponent, CurrentTargets);
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
	const APlayerCharacter* Owner = static_cast<APlayerCharacter*>(GetOwner());
	PlayerController = static_cast<APlayerController*>(Owner->GetController());
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

	if (CurrentTargets.Num() > 0)
	{
		if (CurrentTargets.Contains(Target)) return;
		ClearCurrentTargets();
	}

	TargetActor(Target);
}

bool UTargetingComponent::IsActorInFrustumWithPadding(APlayerController* PC, AActor* Actor, float Padding)
{
	if (!PC || !Actor) return false;

	FVector Origin;
	FVector Extent;
	Actor->GetActorBounds(true, Origin, Extent);

	TArray<FVector> Points;
	Points.Add(Origin + FVector(Extent.X, Extent.Y, Extent.Z));
	Points.Add(Origin + FVector(Extent.X, Extent.Y, -Extent.Z));
	Points.Add(Origin + FVector(Extent.X, -Extent.Y, Extent.Z));
	Points.Add(Origin + FVector(Extent.X, -Extent.Y, -Extent.Z));
	Points.Add(Origin + FVector(-Extent.X, Extent.Y, Extent.Z));
	Points.Add(Origin + FVector(-Extent.X, Extent.Y, -Extent.Z));
	Points.Add(Origin + FVector(-Extent.X, -Extent.Y, Extent.Z));
	Points.Add(Origin + FVector(-Extent.X, -Extent.Y, -Extent.Z));

	int32 ViewportX, ViewportY;
	PC->GetViewportSize(ViewportX, ViewportY);

	const float MinX = -Padding;
	const float MinY = -Padding;
	const float MaxX = ViewportX + Padding;
	const float MaxY = ViewportY + Padding;

	for (const FVector& WorldPos : Points)
	{
		FVector2D ScreenPos;
		if (PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos))
		{
			if (ScreenPos.X >= MinX && ScreenPos.X <= MaxX &&
				ScreenPos.Y >= MinY && ScreenPos.Y <= MaxY)
			{
				return true;
			}
		}
	}

	return false;
}

TArray<AActor*> UTargetingComponent::FindTargetablesInRange(const float Range) const
{
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel1); // equal to ECC_Targetable (custom obj type)
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(OverlapResults, GetOwner()->GetActorLocation(),
	                                                        FQuat::Identity,
	                                                        ObjectQueryParams,
	                                                        FCollisionShape::MakeSphere(Range),
	                                                        QueryParams);

	//DrawDebugSphere(GetWorld(), GetOwner()->GetActorLocation(), Range, 24, FColor::Yellow, false);

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

	if (CurrentTargets.Contains(Target)) return;

	CurrentTargets.Add(Target);

	Cast<ITargetable>(Target)->Target();
	ITargetable::Execute_OnTarget(Target);
}

void UTargetingComponent::ClearCurrentTargets()
{
	for (AActor* Targetable : CurrentTargets)
	{
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
