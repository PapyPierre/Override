#pragma once

#include "CoreMinimal.h"
#include "EViews.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OVERRIDE_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	APlayerController* PlayerController; // Set in Character BP

	UPROPERTY(BlueprintReadOnly)
	TArray<AActor*> CurrentTargets;

	UPROPERTY()
	AActor* ActorInSight;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ScreenPadding = 10;

	UTargetingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void TargetActor(AActor* Target);

	UFUNCTION(BlueprintCallable)
	void ClearCurrentTargets();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float MaxTargetingDistance = 1000;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float SimulationDetectionDistance = 800;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float AutoTargetMinDistance = 500;
	
	EViews CurrentViewMod;

	void LookForTarget(float TargetingRange);

	//	Check if is in the viewport rectangle expanded by Padding.
	//	Positive Padding lets you count actors slightly outside the screen as still “in view”.
	//	Negative Padding forces the actor to be deeper inside the screen to count.
	static bool IsActorInFrustumWithPadding(APlayerController* PC, AActor* Actor, float Padding);

	TArray<AActor*> FindTargetablesInRange(float Range) const;

	static AActor* GetClosestActorToCursor(APlayerController* PC, const TArray<AActor*>& Actors);
};
