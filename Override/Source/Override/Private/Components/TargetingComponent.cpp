#include "Components/TargetingComponent.h"
#include "Interface/Targetable.h"
#include "Kismet/GameplayStatics.h"
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
	DOREPLIFETIME(UTargetingComponent, PointInSight);
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
	const APlayerCharacter* Owner = static_cast<APlayerCharacter*>(GetOwner());
	PlayerController = static_cast<APlayerController*>(Owner->GetController());

	Angle = FMath::RadiansToDegrees(FMath::Atan(MaxDistFromCursor / MaxTargetingDistance)) * 4;
}

void UTargetingComponent::LookForTarget(float TargetingRange)
{
	// Do not read this function server-side
	if (!GetOwner()) return;
	if (GetOwner()->HasAuthority()) return;

	if (!PlayerController) return;
	if (!PlayerController->GetLocalPlayer()) return;

	TArray<AActor*> PotentialTargets;
	AActor* ActorOnHover = GetActorInSight(TargetingRange);

	if (ActorOnHover != nullptr)
	{
		PotentialTargets.Add(ActorOnHover);
	}
	else
	{
		for (AActor* ActorInRange : FindTargetablesInRange(TargetingRange))
		{
			if (!IsActorTargetable(PlayerController, ActorInRange, ScreenPadding, MaxDistFromCursor)) continue;
			
			PotentialTargets.Add(ActorInRange);
		}
	}

	if (PotentialTargets.Num() == 0)
	{
		ClearCurrentTargets();
		return;
	}

	AActor* Target = GetClosestActorToCursor(PlayerController, PotentialTargets);

	if (CurrentTargets.Num() > 0)
	{
		if (CurrentTargets.Contains(Target)) return;
		ClearCurrentTargets();
	}

	TargetActor(Target);
}

void UTargetingComponent::FindPointInSight(float range)
{
	const auto* CamPos = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetTransformComponent();
	const FVector Start = CamPos->GetComponentLocation();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	GetWorld()->LineTraceSingleByObjectType(Hit, Start, Start + CamPos->GetForwardVector() * range, ECC_WorldStatic,
	                                        QueryParams);
	PointInSight = Hit.ImpactPoint;
	//DrawDebugSphere(GetWorld(), PointInSight, 5.0f, 24, FColor::Blue, false);
}

AActor* UTargetingComponent::GetActorInSight(const float Range) const
{
	const auto* CamPos = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetTransformComponent();

	const FVector Start = CamPos->GetComponentLocation();
	const FVector Forward = CamPos->GetForwardVector();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult Hit;

	const FVector End = Start + (Forward * Range);

	if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ECC_GameTraceChannel1, QueryParams))
	{
		AActor* HitActor = Hit.GetActor();

		if (HitActor && HitActor->Implements<UTargetable>())
		{
			return HitActor;
		}
	}

	//DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, 0.1f);
	
	return nullptr;
}

bool UTargetingComponent::IsActorTargetable(const APlayerController* PC, AActor* Actor, const float Padding,
                                            const float maxDistFromCursor)
{
	if (!PC || !Actor) return false;

	const auto TargetActor = Cast<ITargetable>(Actor);

	RegenerateTargetActorPoints(Actor);

	int32 ViewportX, ViewportY;
	PC->GetViewportSize(ViewportX, ViewportY);

	FVector2D ScreenCenter = FVector2D(ViewportX / 2, ViewportY / 2);

	const float MinX = -Padding;
	const float MinY = -Padding;
	const float MaxX = ViewportX + Padding;
	const float MaxY = ViewportY + Padding;

	bool result = false;

	for (const FVector& WorldPos : TargetActor->Points)
	{
		if (!IsPointVisiblePhysically(WorldPos, Actor, PC))
		{
			//DrawDebugLine(Actor->GetWorld(), Actor->GetActorLocation(), PC->GetPawn()->GetActorLocation(), FColor::Purple, false, 0.1f);
			continue;
		}

		//DrawDebugLine(Actor->GetWorld(), Actor->GetActorLocation(), PC->GetPawn()->GetActorLocation(), FColor::Green, false, 0.1f);

		FVector2D ScreenPos;

		//DrawDebugSphere(Actor->GetWorld(), WorldPos, 5.0f, 24, FColor::Blue, false);

		if (PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos))
		{
			FVector OutPosition;
			FVector OutDirection;
			PC->DeprojectScreenPositionToWorld(ScreenPos.X, ScreenPos.Y, OutPosition, OutDirection);
			//DrawDebugSphere(Actor->GetWorld(), OutPosition, 0.1f, 24, FColor::Red, false);

			if (ScreenPos.X < MinX || ScreenPos.X > MaxX || ScreenPos.Y < MinY || ScreenPos.Y > MaxY)
			{
				result = false;
				continue;
			}

			float DistToScreenCenter = FVector2D::Distance(ScreenCenter, ScreenPos);

			if (DistToScreenCenter < maxDistFromCursor)
			{
				result = true;
				break;
			}
		}
	}

	return result;
}

bool UTargetingComponent::IsPointVisiblePhysically(const FVector Pos, AActor* Actor,
                                                   const APlayerController* PlayerController)
{
	if (!PlayerController) return false;

	FVector ViewLoc;
	FRotator ViewRot;
	PlayerController->GetPlayerViewPoint(ViewLoc, ViewRot);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(PlayerController->GetPawn());

	bool bHit = PlayerController->GetWorld()->LineTraceSingleByChannel(
		Hit,
		ViewLoc,
		Pos,
		ECC_Visibility,
		Params
	);

	if (bHit && Hit.GetActor() == Actor)
	{
		//DrawDebugLine(Actor->GetWorld(), ViewLoc, Pos, FColor::Blue, false, 0.1f);
		return true;
	}

	//DrawDebugLine(Actor->GetWorld(), ViewLoc, Pos, FColor::Red, false, 0.1f);
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
				//DrawDebugLine(Actor->GetWorld(), Actor->GetActorLocation(), GetOwner()->GetActorLocation(), FColor::Yellow, false, 0.1f);
			}
		}
	}

	return FoundTargetable;
}

AActor* UTargetingComponent::GetClosestActorToCursor(APlayerController* PC, const TArray<AActor*> Actors)
{
	if (!PC || Actors.Num() == 0) return nullptr;

	int32 ViewportX, ViewportY;
	PC->GetViewportSize(ViewportX, ViewportY);
	FVector2D ScreenCenter(ViewportX * 0.5f, ViewportY * 0.5f);

	FVector OutPosition;
	FVector OutDirection;
	PC->DeprojectScreenPositionToWorld(ScreenCenter.X, ScreenCenter.Y, OutPosition, OutDirection);
	//DrawDebugSphere(PC->GetWorld(), OutPosition, 0.2f, 24, FColor::Purple, false);

	AActor* ClosestActor = nullptr;
	float ClosestDist = TNumericLimits<float>::Max();

	if (Actors.Num() == 1) return Actors[0];

	for (AActor* Actor : Actors)
	{
		if (!Actor) continue;

		const auto TargetActor = Cast<ITargetable>(Actor);

		RegenerateTargetActorPoints(Actor);

		for (const FVector Point : TargetActor->Points)
		{
			if (!IsPointVisiblePhysically(Point, Actor, PC)) continue;

			if (FVector2D ScreenPos; PC->ProjectWorldLocationToScreen(Point, ScreenPos))
			{
				if (const float Dist = FVector2D::Distance(ScreenCenter, ScreenPos); Dist < ClosestDist)
				{
					ClosestDist = Dist;
					ClosestActor = Actor;
				}
			}
		}
	}

	return ClosestActor;
}

void UTargetingComponent::RegenerateTargetActorPoints(AActor* Actor) //TODO Optimised by cashing points
{
	FVector Origin;
	FVector Extent;

	Actor->GetActorBounds(true, Origin, Extent);

	auto TargetActor = Cast<ITargetable>(Actor);

	TargetActor->Points.Empty();

	TargetActor->Points.Add(Origin);
	TargetActor->Points.Add(Origin + FVector(Extent.X, Extent.Y, Extent.Z));
	TargetActor->Points.Add(Origin + FVector(Extent.X, Extent.Y, -Extent.Z));
	TargetActor->Points.Add(Origin + FVector(Extent.X, -Extent.Y, Extent.Z));
	TargetActor->Points.Add(Origin + FVector(Extent.X, -Extent.Y, -Extent.Z));
	TargetActor->Points.Add(Origin + FVector(-Extent.X, Extent.Y, Extent.Z));
	TargetActor->Points.Add(Origin + FVector(-Extent.X, Extent.Y, -Extent.Z));
	TargetActor->Points.Add(Origin + FVector(-Extent.X, -Extent.Y, Extent.Z));
	TargetActor->Points.Add(Origin + FVector(-Extent.X, -Extent.Y, -Extent.Z));

	TargetActor->PointsGenerated = true;
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
	FindPointInSight(MaxTargetingDistance);

	LookForTarget(MaxTargetingDistance);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
