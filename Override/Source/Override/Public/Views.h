#pragma once

#include "CoreMinimal.h"
#include "Views.generated.h"

UENUM(BlueprintType)
enum class Views : uint8
{
	WeaponView UMETA(DisplayName = "Weapon View"),
	HackView   UMETA(DisplayName = "Hack View"),
};
