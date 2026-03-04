#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MovementStats.generated.h"

UCLASS(BlueprintType)
class OVERRIDE_API UMovementStats : public UDataAsset
{
	GENERATED_BODY()
	
	
public:

#pragma region Movement
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Speed")
	float SpeedBackwardReduction = 0.7;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Speed")
	float SpeedSideReduction = 0.8;
#pragma endregion
	
#pragma region Camera
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|FOV")
	float DefaultFOV = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|FOV")
	float MaxFOV = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|FOV")
	float FOVInterpNormalSpeed = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|FOV")
	float FOVInterpAimSpeed = 15.f;
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
	float SlideImpulse = 600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float SlopeToleranceValue = 0.02;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float MaxVelocityForSlide = 1000;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float SlideCoolDownDuration = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float SpeedIncreaseInSlope = 20000;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float SpeedDecreaseInSlope = 1000;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float Friction = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float BrakingDeceleration = 750;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Slide")
	float MaxAccelerationForSlide = 1000;
#pragma endregion

#pragma region Melee
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Melee")
	float EaseOutTimeMelee = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Melee")
	float MeleeImpulse = 2000.f;
#pragma endregion

#pragma region Jump
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Jump")
	float FirstJumpZVelocity = 800.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Jump")
	float CoyoteTime= 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Jump")
	float JumpResetTime = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Jump")
	UCurveFloat* JumpCurve;
#pragma endregion

#pragma region Ping
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ping")
	float PingTime = 5.f;
#pragma endregion

#pragma region Skills
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills|Slow")
	float SlowedSpeedReduction = 0.75;
#pragma endregion

};
