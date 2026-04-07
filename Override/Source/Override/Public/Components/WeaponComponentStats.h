

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponComponentStats.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class OVERRIDE_API UWeaponComponentStats : public UDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	float BaseDamage = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	float ReducedDamage = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	float MaxRange = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float ShootRadius = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Reload = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float EndOfShootStance = 0.8f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	float RPM = 750;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	float MunitionRecallTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	int MaxMunition = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	int StockMunition = 200;	
};
