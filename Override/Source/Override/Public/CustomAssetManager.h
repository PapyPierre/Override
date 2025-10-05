#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "CustomAssetManager.generated.h"

UCLASS()
class OVERRIDE_API UCustomAssetManager : public UAssetManager
{
	GENERATED_BODY()

	static UCustomAssetManager& Get();

	/** Starts initial load, gets called from InitializeObjectReferences */
	virtual void StartInitialLoading() override;
};
