#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameMode/MatchPlayerData.h"
#include "MatchActor.generated.h"

UCLASS()
class OVERRIDEEDITOR_API  AMatchActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FMatchPlayerData> Players;

	bool SeeThrough;

	float TimeValue;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	void Clear();
};
