

#pragma once
#include "CoreMinimal.h"

/**
 * @class CameraManager
 * @brief Manages camera functionalities such as field of view adjustments and camera shake effects.
 */

class APlayerCharacter;
class UPlayerMovementComponent;

class OVERRIDE_API CameraManager
{
public:
	CameraManager();
	~CameraManager();

	int LastFOV = 0;
	bool bWasAiming = false;
	
	void SetFov(APlayerCharacter* PlayerCharacter, const UPlayerMovementComponent* PlayerMovementComponent, float DeltaTime);
	void CameraShake(APlayerCharacter* PlayerCharacter, const UPlayerMovementComponent* PlayerMovementComponent);
};
