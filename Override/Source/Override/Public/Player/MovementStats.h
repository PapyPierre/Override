#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MovementStats.generated.h"

UCLASS(BlueprintType)
class OVERRIDE_API UMovementStats : public UDataAsset
{
	GENERATED_BODY()
	
	
public:
	
#pragma region Camera
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|FOV")
	float DefaultFOV = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|FOV")
	float SprintFOV = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|FOV")
	float FOVInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|FOV")
	float AimFOV = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Aim")
	float MouseSensitivity = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Aim")
	float MouseAimSensitivity = 0.4f;
#pragma endregion

#pragma region Aiming
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Aim")
	float AimCrouchedSpeed = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Aim")
	float AimSpeed = 300.f;
#pragma endregion
	
#pragma region Slide
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float SlidingCoolDown = 0.2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float BoostSlidingTime = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float EaseOutTime = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float SlideImpulse = 600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float SlopeToleranceValue = 0.02;
#pragma endregion

#pragma region Sprint
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Sprint")
	float SprintSpeed = 825.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Sprint")
	float SprintAcceleration = 200.f;
#pragma endregion

#pragma region Jump
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Jump")
	float FirstJumpZVelocity = 800.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Jump")
	float SecondJumpZVelocity = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Jump")
	float SecondJumpAirControl = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Jump")
	float AirHorizontalRetainPercent = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Jump")
	float CoyoteTime= 0.5f;
#pragma endregion

#pragma region Parkour
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Parkour")
	float MaxVaultThickness = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Parkour")
	float MaxVaultHeight = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Parkour")
	float RaycastStartHeight = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Parkour")
	float RaycastEndHeight = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Parkour")
	UAnimMontage* EdgeClimbMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Parkour")
	UAnimMontage* VaultMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Parkour")
	float ParkourDistanceDetection = 70.f;
#pragma endregion
};
