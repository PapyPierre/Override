#pragma once

#include "CoreMinimal.h"
#include "EViews.generated.h"

UENUM(BlueprintType)
enum class EViews : uint8
{
	WeaponView UMETA(DisplayName = "Weapon View"),
	SimView UMETA(DisplayName = "Hack View"),
};
