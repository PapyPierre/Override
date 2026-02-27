#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "UObject/Object.h"
#include "CustomAbilityTargetData.generated.h"

USTRUCT(BlueprintType)
struct OVERRIDE_API FCustomAbilityTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()
	
public:
	FCustomAbilityTargetData() {}
	
	TArray<TWeakObjectPtr<AActor>> Targets;

	virtual TArray<TWeakObjectPtr<AActor>> GetActors() const override
	{
		return Targets;
	}

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FCustomAbilityTargetData::StaticStruct();
	}

	virtual FGameplayAbilityTargetDataHandle CloneFrom(const FGameplayAbilityTargetDataHandle& SourceHandle) const
	{
		FGameplayAbilityTargetDataHandle ReturnHandle;
		for (int32 i = 0; i < SourceHandle.Num(); i++)
		{
			if (const FCustomAbilityTargetData* SourceData = static_cast<const FCustomAbilityTargetData*>(SourceHandle.Get(i)))
			{
				FCustomAbilityTargetData* NewData = new FCustomAbilityTargetData();
				NewData->Targets = SourceData->Targets;
				ReturnHandle.Add(NewData);
			}
		}
		return ReturnHandle;
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FCustomAbilityTargetData> : public TStructOpsTypeTraitsBase2<FCustomAbilityTargetData>
{
	enum
	{
		WithNetSerializer = true
	};
};
