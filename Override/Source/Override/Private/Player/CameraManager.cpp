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
}

void UCameraManager::SetFov(APlayerCharacter* PlayerCharacter, const UPlayerMovementComponent* PlayerMovementComponent, float DeltaTime)
{	
	if (!PlayerCharacter->FirstPersonCameraComponent)
		return;

	FovTimeline.TickTimeline(DeltaTime);

	CurrentFov = PlayerCharacter->FirstPersonCameraComponent->GetFOVAngle();
	
	if (PlayerMovementComponent->IsMovingOnGround())
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
		break;
	case MovementMode::Walking:
		break;
	case MovementMode::Sliding:
		TargetFov = PlayerCharacter->MaxFOV;
		FovCurve = PlayerCharacter->CurveSlideStart;
		break;
	case MovementMode::Dashing:
		FovCurve = PlayerCharacter->CurveDashStart;
		break;
	case MovementMode::Aiming:
		FovCurve = PlayerCharacter->CurveAimStart;
		break;
	default:
		break;
	}
	
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
		if (PlayerCharacter->GetVelocity().Size() == 0.0)
		{
			PlayerCharacter->FirstPersonCameraComponent->StartCameraShake(PlayerCharacter->ShakeIdle, 1.0f, ECameraShakePlaySpace::CameraLocal,
														 FRotator::ZeroRotator);
		}
		else
		{
			if (PlayerMovementComponent->IsWalking())
				PlayerCharacter->FirstPersonCameraComponent->StartCameraShake(PlayerCharacter->ShakeWalk, 1.0f, ECameraShakePlaySpace::CameraLocal,
															 FRotator::ZeroRotator);
		}
	}
}

void UCameraManager::UpdateFOV(float Alpha)
{
	float TargetFOV = FMath::Lerp(
		CurrentFov,
		TargetFov,
		Alpha
	);
}
