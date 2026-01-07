#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "UObject/Object.h"
#include "GameplayHackTargetData.generated.h"

USTRUCT(BlueprintType)
struct OVERRIDE_API FGameplayHackTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()
	
public:
	FGameplayHackTargetData() {}
	
	TArray<TWeakObjectPtr<AActor>> Targets;

	virtual TArray<TWeakObjectPtr<AActor>> GetActors() const override
	{
		return Targets;
	}

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FGameplayHackTargetData::StaticStruct();
	}

	virtual FGameplayAbilityTargetDataHandle CloneFrom(const FGameplayAbilityTargetDataHandle& SourceHandle) const
	{
		FGameplayAbilityTargetDataHandle ReturnHandle;
		for (int32 i = 0; i < SourceHandle.Num(); i++)
		{
			if (const FGameplayHackTargetData* SourceData = static_cast<const FGameplayHackTargetData*>(SourceHandle.Get(i)))
			{
				FGameplayHackTargetData* NewData = new FGameplayHackTargetData();
				NewData->Targets = SourceData->Targets;
				ReturnHandle.Add(NewData);
			}
		}
		return ReturnHandle;
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FGameplayHackTargetData> : public TStructOpsTypeTraitsBase2<FGameplayHackTargetData>
{
	enum
	{
		WithNetSerializer = true
	};
};
