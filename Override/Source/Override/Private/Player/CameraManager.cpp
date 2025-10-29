#include "Player/CameraManager.h"

#include "Player/PlayerCharacter.h"
#include "Player/PlayerMovementComponent.h"

CameraManager::CameraManager()
{
}

CameraManager::~CameraManager()
{
}

void CameraManager::SetFov(APlayerCharacter* PlayerCharacter, const UPlayerMovementComponent* PlayerMovementComponent, float DeltaTime)
{
	CameraShake(PlayerCharacter, PlayerMovementComponent);

	if (PlayerMovementComponent->IsRunning())
	{
		float Speed = PlayerCharacter->GetVelocity().Size();
		float TargetFOV = FMath::GetMappedRangeValueClamped(
			FVector2D(PlayerMovementComponent->DefaultMaxWalkSpeed, PlayerMovementComponent->DefaultSprintSpeed),
			FVector2D(PlayerCharacter->DefaultFOV, PlayerCharacter->SprintFOV),
			Speed
		);

		float NewFOV = FMath::FInterpTo(
			PlayerCharacter->FirstPersonCameraComponent->GetFOVAngle(),
			TargetFOV,
			DeltaTime,
			PlayerCharacter->FOVInterpSpeed
		);

		PlayerCharacter->FirstPersonCameraComponent->SetFOV(NewFOV);
	}

	if (PlayerCharacter->bIsAimingWeapon)
	{
		float NewFOV = FMath::FInterpTo(
			PlayerCharacter->FirstPersonCameraComponent->GetFOVAngle(),
			PlayerCharacter->AimFOV,
			DeltaTime,
			PlayerCharacter->FOVInterpSpeed
		);

		PlayerCharacter->FirstPersonCameraComponent->SetFOV(NewFOV);
	}
	else if (!FMath::IsNearlyEqual(PlayerCharacter->FirstPersonCameraComponent->GetFOVAngle(), PlayerCharacter->DefaultFOV) && !PlayerCharacter->bIsAimingWeapon && PlayerMovementComponent->IsMovingOnGround() && !PlayerMovementComponent->IsSliding())
	{
		float NewFOV = FMath::FInterpTo(
			PlayerCharacter->FirstPersonCameraComponent->GetFOVAngle(),
			PlayerCharacter->DefaultFOV,
			DeltaTime,
			PlayerCharacter->FOVInterpSpeed
		);

		PlayerCharacter->FirstPersonCameraComponent->SetFOV(NewFOV);
	}
}

void CameraManager::CameraShake(APlayerCharacter* PlayerCharacter, const UPlayerMovementComponent* PlayerMovementComponent)
{
	if (PlayerMovementComponent->IsMovingOnGround())
	{
		if (PlayerCharacter->GetVelocity().Size() == 0.0)
		{
			PlayerCharacter->FirstPersonCameraComponent->StartCameraShake(PlayerCharacter->ShakeIdle, 1.0f, ECameraShakePlaySpace::CameraLocal,
														 FRotator::ZeroRotator);
		}
		else
		{
			if (PlayerMovementComponent->IsRunning())
				PlayerCharacter->FirstPersonCameraComponent->StartCameraShake(PlayerCharacter->ShakeRunning, 1.0f, ECameraShakePlaySpace::CameraLocal,
															 FRotator::ZeroRotator);
			else if (PlayerMovementComponent->IsWalking())
				PlayerCharacter->FirstPersonCameraComponent->StartCameraShake(PlayerCharacter->ShakeWalk, 1.0f, ECameraShakePlaySpace::CameraLocal,
															 FRotator::ZeroRotator);
		}
	}
}
