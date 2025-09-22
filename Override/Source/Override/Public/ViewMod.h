#pragma once

#include "CoreMinimal.h"
#include "ViewMod.generated.h"

UENUM(BlueprintType)
enum class ViewMod : uint8
{
	WeaponView UMETA(DisplayName = "Weapon View"),
	HackView   UMETA(DisplayName = "Hack View"),
};
