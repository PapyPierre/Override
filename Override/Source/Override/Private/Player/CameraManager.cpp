#include "Player/CameraManager.h"

#include "FrameTypes.h"
#include "Player/MovementStats.h"
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

	float CurrentFOV = PlayerCharacter->FirstPersonCameraComponent->GetFOVAngle();
	float InterpSpeed = PlayerCharacter->FOVInterpNormalSpeed;

	const float Speed = PlayerMovementComponent->Velocity.Size();

	const float MinSpeed = PlayerMovementComponent->DefaultMaxWalkSpeed;
	const float MaxSpeed = PlayerMovementComponent->MovementData->MaxVelocityForSlide;

	float Alpha = (Speed - MinSpeed) / (MaxSpeed - MinSpeed);
	Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
	
	float TargetFOV = FMath::Lerp(
		PlayerCharacter->DefaultFOV,
		PlayerCharacter->MaxFOV,
		Alpha
	);

	if (!PlayerMovementComponent->IsMovingOnGround())
	{
		TargetFOV = CurrentFOV;
	}
	
	// Aim
	if (PlayerCharacter->bIsAimingWeapon)
	{
		TargetFOV = PlayerCharacter->AimFOV;
		InterpSpeed = PlayerCharacter->FOVInterpAimSpeed;
		LastFOV = 1;
	}

	if (LastFOV == 1)
		InterpSpeed = PlayerCharacter->FOVInterpAimSpeed;
	
	float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, InterpSpeed);
	PlayerCharacter->FirstPersonCameraComponent->SetFOV(NewFOV);
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
			if (PlayerMovementComponent->IsWalking())
				PlayerCharacter->FirstPersonCameraComponent->StartCameraShake(PlayerCharacter->ShakeWalk, 1.0f, ECameraShakePlaySpace::CameraLocal,
															 FRotator::ZeroRotator);
		}
	}
}
