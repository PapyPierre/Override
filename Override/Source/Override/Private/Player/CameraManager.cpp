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

	float CurrentFOV = PlayerCharacter->FirstPersonCameraComponent->GetFOVAngle();
	float TargetFOV = PlayerCharacter->DefaultFOV;
	float InterpSpeed = PlayerCharacter->FOVInterpNormalSpeed;
		
	// Sprint
	if (PlayerMovementComponent->IsRunning())
	{
		TargetFOV = PlayerCharacter->SprintFOV;
		InterpSpeed = PlayerCharacter->FOVInterpSprintSpeed;
		LastFOV = 0;
	}

	// Aim
	if (PlayerCharacter->bIsAimingWeapon)
	{
		TargetFOV = PlayerCharacter->AimFOV;
		InterpSpeed = PlayerCharacter->FOVInterpAimSpeed;
		LastFOV = 1;
	}

	// Slide
	if (PlayerMovementComponent->IsSliding())
	{
		TargetFOV = PlayerCharacter->SlideFOV;
		InterpSpeed = PlayerCharacter->FOVInterpSlideSpeed;
		LastFOV = 2;
	}


	if (LastFOV == 1)
		InterpSpeed = PlayerCharacter->FOVInterpAimSpeed;
	if (LastFOV == 0)
		InterpSpeed = PlayerCharacter->FOVInterpNormalSpeed;
	
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
			if (PlayerMovementComponent->IsRunning())
				PlayerCharacter->FirstPersonCameraComponent->StartCameraShake(PlayerCharacter->ShakeRunning, 1.0f, ECameraShakePlaySpace::CameraLocal,
															 FRotator::ZeroRotator);
			else if (PlayerMovementComponent->IsWalking())
				PlayerCharacter->FirstPersonCameraComponent->StartCameraShake(PlayerCharacter->ShakeWalk, 1.0f, ECameraShakePlaySpace::CameraLocal,
															 FRotator::ZeroRotator);
		}
	}
}
