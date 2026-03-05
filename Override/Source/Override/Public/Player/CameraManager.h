#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Components/TimelineComponent.h"
#include "CameraManager.generated.h"

class APlayerCharacter;
class UPlayerMovementComponent;

UCLASS()
class OVERRIDE_API UCameraManager : public UObject
{
	GENERATED_BODY()

public:

	enum class MovementMode
	{
		Idle = 0,
		Aiming,
		Walking,
		Sliding,
		Dashing
	};

	UCameraManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	bool bWasAiming = false;

	MovementMode LastMovementMode;
	MovementMode CurrentMovementMode;

	float CurrentFov;
	float TargetFov;

	UPROPERTY()
	UCurveFloat* FovCurve;

	FOnTimelineFloat OnTimelineCallback;
	FTimeline FovTimeline;

	void BeginPlay(APlayerCharacter* PlayerCharacter);

	void SetFov(APlayerCharacter* PlayerCharacter, const UPlayerMovementComponent* PlayerMovementComponent, float DeltaTime);
	void CameraShake(APlayerCharacter* PlayerCharacter, const UPlayerMovementComponent* PlayerMovementComponent);

	UFUNCTION()
	void UpdateFOV(float Alpha);
};