#pragma once

#include "CoreMinimal.h"
#include "EViews.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent))
class OVERRIDE_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetingComponent();
	
	UPROPERTY(BlueprintReadWrite)
	APlayerController* PlayerController;

	UPROPERTY(BlueprintReadOnly, Replicated)
	TArray<AActor*> CurrentTargets;

	UPROPERTY(BlueprintReadOnly, Replicated)
	FVector PointInSight;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	float ScreenPadding = -100;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	float MaxDistFromCursor = 50; // In Screen Space
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void TargetActor(AActor* Target);

	UFUNCTION(BlueprintCallable)
	void ClearCurrentTargets();

protected:
	UPROPERTY(EditAnywhere, Category = "Targeting")
	float MaxTargetingDistance = 3000;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	float SimulationDetectionDistance = 800;
	
	virtual void BeginPlay() override;

private:
	 float Angle;
	
	EViews CurrentViewMod;

	void LookForTarget(float TargetingRange);

	void FindPointInSight(float range);

	TArray<FVector> ComputeTraceDirections(const float AngleDegrees) const;

	TArray<AActor*> FindActorsWithLineTraces(float Range);

	//	Check if is in the viewport rectangle expanded by Padding.
	//	Positive Padding lets you count actors slightly outside the screen as still “in view”.
	//	Negative Padding forces the actor to be deeper inside the screen to count.
	static bool IsActorTargetable(const APlayerController* PC, AActor* Actor, float Padding, float maxDistFromCursor);

	static bool IsPointVisiblePhysically(const FVector Point, AActor* Actor, const APlayerController* PlayerController);

	TArray<AActor*> FindTargetablesInRange(float Range) const;

	static AActor* GetClosestActorToCursor(APlayerController* PC, const TArray<AActor*> Actors);

	static void RegenerateTargetActorPoints(AActor* Actor);
};
