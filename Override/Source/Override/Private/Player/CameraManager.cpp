#include "Player/CameraManager.h"

#include "FrameTypes.h"
#include "Player/MovementStats.h"
#include "Player/PlayerCharacter.h"
#include "Player/PlayerMovementComponent.h"

UCameraManager::UCameraManager(const FObjectInitializer& ObjectInitializer)
{
}

void UCameraManager::BeginPlay(APlayerCharacter* PlayerCharacter)
{
	FovCurve = PlayerCharacter->CurveSlideStart;
	OnTimelineCallback.BindUFunction(this, FName("UpdateFOV"));
	FovTimeline.AddInterpFloat(FovCurve, OnTimelineCallback);
	FovTimeline.SetLooping(false);
	FovTimeline.PlayFromStart();

	PlayerRef = PlayerCharacter;
}

void UCameraManager::SetFov(APlayerCharacter* PlayerCharacter, const UPlayerMovementComponent* PlayerMovementComponent, float DeltaTime)
{	
	if (!PlayerCharacter->FirstPersonCameraComponent)
		return;

	FovTimeline.TickTimeline(DeltaTime);
	CameraShake(PlayerCharacter, PlayerMovementComponent);
	
	if (PlayerMovementComponent->Velocity.IsNearlyZero())
		CurrentMovementMode = MovementMode::Idle;
	else
		CurrentMovementMode = MovementMode::Walking;
	if (PlayerMovementComponent->IsSliding())
		CurrentMovementMode = MovementMode::Sliding;
	if (PlayerMovementComponent->bIsDashing)
		CurrentMovementMode = MovementMode::Dashing;
	if (PlayerCharacter->bIsAimingWeapon)
		CurrentMovementMode = MovementMode::Aiming;

	if (CurrentMovementMode == LastMovementMode)
		return;

	if (FovTimeline.IsPlaying())
		FovTimeline.Stop();
	
	switch (CurrentMovementMode)
	{
	case MovementMode::Idle:
		TargetFov = PlayerCharacter->DefaultFOV;
		FovCurve = PlayerCharacter->CurveIdleStart;
		break;
	case MovementMode::Walking:
		if (LastMovementMode == MovementMode::Sliding)
		{
			TargetFov = PlayerCharacter->WalkFOV;
			FovCurve = PlayerCharacter->CurveSlideEnd;
		}
		else
		{
			TargetFov = PlayerCharacter->WalkFOV;
			FovCurve = PlayerCharacter->CurveWalkStart;
		}
		break;
	case MovementMode::Sliding:
		TargetFov = PlayerCharacter->SlideFOV;
		FovCurve = PlayerCharacter->CurveSlideStart;
		break;
	case MovementMode::Dashing:
		TargetFov = PlayerCharacter->DashFOV;
		FovCurve = PlayerCharacter->CurveDashStart;
		break;
	case MovementMode::Aiming:
		TargetFov = PlayerCharacter->AimFOV;
		FovCurve = PlayerCharacter->CurveAimStart;
		break;
	default:
		break;
	}

	CurrentFov = PlayerCharacter->FirstPersonCameraComponent->GetFOVAngle();
	FovTimeline.AddInterpFloat(FovCurve, OnTimelineCallback);
	FovTimeline.PlayFromStart();

	LastMovementMode = CurrentMovementMode;
}

void UCameraManager::CameraShake(APlayerCharacter* PlayerCharacter, const UPlayerMovementComponent* PlayerMovementComponent)
{
	if (!PlayerCharacter->FirstPersonCameraComponent)
		return;
	
	if (PlayerMovementComponent->IsMovingOnGround())
	{
		if (PlayerMovementComponent->Velocity.IsNearlyZero())
			PlayerCharacter->FirstPersonCameraComponent->StartCameraShake(PlayerCharacter->ShakeIdle, 1.0f, ECameraShakePlaySpace::CameraLocal,
														 FRotator::ZeroRotator);
		
		else if (PlayerMovementComponent->IsWalking() && !PlayerMovementComponent->IsSliding() && !PlayerMovementComponent->bIsDashing)
			PlayerCharacter->FirstPersonCameraComponent->StartCameraShake(PlayerCharacter->ShakeWalk, 1.0f, ECameraShakePlaySpace::CameraLocal,
														 FRotator::ZeroRotator);
	}
}


void UCameraManager::UpdateFOV(float Alpha)
{
	if (!PlayerRef)
	{
		return;
	}
	
	float InterpolatedFOV = FMath::Lerp(CurrentFov, TargetFov, Alpha);
	PlayerRef->FirstPersonCameraComponent->SetFOV(InterpolatedFOV);
}