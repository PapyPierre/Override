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
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
	const APlayerCharacter* Owner = static_cast<APlayerCharacter*>(GetOwner());
	PlayerController = static_cast<APlayerController*>(Owner->GetController());

	Angle = FMath::RadiansToDegrees(FMath::Atan(MaxDistFromCursor / MaxTargetingDistance)) * 4;
}

void UTargetingComponent::LookForTarget()
{
	if (!GetOwner() || GetOwner()->HasAuthority()) return;
	if (!PlayerController || !PlayerController->GetLocalPlayer()) return;

	const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (!Cam) return;

	const FVector CamPos = Cam->GetCameraLocation();
	const FVector CamForward = Cam->GetActorForwardVector();
	const FVector CamRight = Cam->GetActorRightVector();
	const FVector CamUp = Cam->GetActorUpVector();

	const FMatrix CamMatrix = FMatrix(
		CamRight,
		CamUp,
		CamForward,
		FVector::ZeroVector
	);

	constexpr float NearPlane = 20.f;
	const float HalfFOVRad = FMath::DegreesToRadians(Cam->GetFOVAngle() * 0.5f);
	const float PlaneHeight = NearPlane * FMath::Tan(HalfFOVRad) * 2.f;
	const float PlaneWidth = PlaneHeight * Cam->GetCameraCacheView().AspectRatio;
	const float PlaneWidthHalf = PlaneWidth * 0.5f;
	const float PlaneHeightHalf = PlaneHeight * 0.5f;

	const int CountX = TargetingAccuracy;
	const int CountY = TargetingAccuracy;
	const float StepX = 1.f / (CountX - 1);
	const float StepY = 1.f / (CountY - 1);

	int32 Vx, Vy;
	PlayerController->GetViewportSize(Vx, Vy);
	const FVector2D ScreenCenter(Vx * 0.5f, Vy * 0.5f);

	const float MaxDistSq = FMath::Square(MaxDistFromCursor * 0.5f);
	const float MaxDist = MaxDistFromCursor * 0.5f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	ClosestActor = nullptr;
	float ClosestDist = TNumericLimits<float>::Max();

	const float CircleRadiusNormalized = MaxDist / FMath::Max(Vx, Vy);

	for (int x = 0; x < CountX; x++)
	{
		const float Tx = x * StepX;
		const float NormalizedX = Tx - 0.5f;

		for (int y = 0; y < CountY; y++)
		{
			const float Ty = y * StepY;
			const float NormalizedY = Ty - 0.5f;

			// 1st skip of points outside the targeting circle (approx)
			const float DistFromCenterSq = NormalizedX * NormalizedX + NormalizedY * NormalizedY;
			if (DistFromCenterSq > CircleRadiusNormalized * CircleRadiusNormalized * 2.f)
			{
				continue;
			}

			const float OffsetX = PlaneWidth * Tx;
			const float OffsetY = PlaneHeight * Ty;

			const FVector LocalPoint(
				OffsetX - PlaneWidthHalf,
				OffsetY - PlaneHeightHalf,
				NearPlane
			);

			const FVector PointWorld = CamPos + CamMatrix.TransformVector(LocalPoint);

			FVector2D ScreenPos;
			PlayerController->ProjectWorldLocationToScreen(PointWorld, ScreenPos);

			const float ScreenDistSq = FVector2D::DistSquared(ScreenPos, ScreenCenter);

			// 2nd skip of points outside the targeting circle (precise)
			if (ScreenDistSq > MaxDistSq)
			{
				DrawDebugPoint(GetWorld(), PointWorld, 3, FColor::Red, false, 0.1f);
				continue;
			}

			const FVector Dir = (PointWorld - CamPos).GetSafeNormal();
			const FVector End = CamPos + Dir * MaxTargetingDistance;

			if (FHitResult Hit; GetWorld()->LineTraceSingleByObjectType(
				Hit, CamPos, End, ECC_GameTraceChannel1, QueryParams))
			{
				AActor* HitActor = Hit.GetActor();
				if (!HitActor) continue;
				
				if (IsPointOnTargetVisible(CamPos, Hit.Location, HitActor, PlayerController))
				{
					DrawDebugPoint(GetWorld(), PointWorld, 3, FColor::Green, false, 0.01f);
					//DrawDebugLine(GetWorld(), CamPos, CamPos + Dir * TargetingRange, FColor::Green, false, 0.1f);

					const float Dist = FMath::Sqrt(ScreenDistSq);
					if (Dist <= ClosestDist)
					{
						ClosestDist = Dist;
						ClosestActor = HitActor;
					}

					continue;
				}

				DrawDebugPoint(GetWorld(), PointWorld, 3, FColor::Purple, false, 0.01f);
				//DrawDebugLine(GetWorld(), CamPos, CamPos + Dir * TargetingRange, FColor::Purple, false, 0.1f);

				continue;
			}

			DrawDebugPoint(GetWorld(), PointWorld, 3, FColor::Blue, false, 0.1f);
			//DrawDebugLine(GetWorld(), CamPos, CamPos + Dir * TargetingRange, FColor::Blue, false, 0.1f);
		}
	}

	if (ClosestActor == nullptr)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("No actor found, clear"));
		ClearCurrentTargets();
		return;
	}

	if (CurrentTargets.Num() > 0)
	{
		if (CurrentTargets.Contains(ClosestActor)) return;
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Clear old targets before targeting"));
		ClearCurrentTargets();
	}

	TargetActor(ClosestActor);
}

FVector UTargetingComponent::GetPointInSight() const
{
	const auto* CamPos = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetTransformComponent();
	const FVector Start = CamPos->GetComponentLocation();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	GetWorld()->LineTraceSingleByObjectType(Hit, Start, Start + CamPos->GetForwardVector() * MaxTargetingDistance,
	                                        ECC_WorldStatic,
	                                        QueryParams);
	return Hit.ImpactPoint;
}

bool UTargetingComponent::IsPointOnTargetVisible(const FVector& Start, const FVector& End, const AActor* Target,
                                                 const APlayerController* PC)
{
	if (!PC) return false;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(PC->GetPawn());

	bool bHit = PC->GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	return bHit && Hit.GetActor() == Target;
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
	LookForTarget();

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
