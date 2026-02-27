// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameModeStat.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class OVERRIDE_API UGameModeStat : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	TArray<int32> RoundData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	int MaxDataWin = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	int PhaseTime = 90;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	float TransitionTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	float RespawnTimeCooldown = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	int DataPourcentOnDeath = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	int DataPourcentOnTeamKill = 45;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameMode")
	int DataKillBonus = 200;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	int GameTime = 480;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	float MaxSpawnObjDistance = 4000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	float MinSpawnObjDistance = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameMode")
	float EnemiesSpawnInhibitRange = 1500;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Points")
	int FlagInitialTake = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Points")
	int FirstIgniteInARow = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Points")
	int SecondIgniteInARow = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Points")
	int ThirdIgniteInARow = 1;
};
