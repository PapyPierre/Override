#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CustomHUD.generated.h"

UCLASS()
class OVERRIDE_API ACustomHUD : public AHUD
{
	GENERATED_BODY()
	virtual void DrawHUD() override;
};
