#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interface/Targetable.h"
#include "TargetingComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OVERRIDE_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
						   FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite)
	APlayerController* PlayerController; // Set in Character BP
	
	UPROPERTY(BlueprintReadOnly)
	TArray<AActor*> CurrentTargets;

	AActor* ActorInSight;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Hack")
	float DetectionDistance_View = 1000;
	
	UPROPERTY(EditDefaultsOnly, Category = "Hack")
	float DetectionDistance_Auto = 500;

private:
	void LookForTarget();

	AActor* FindClosestTarget() const;
};
