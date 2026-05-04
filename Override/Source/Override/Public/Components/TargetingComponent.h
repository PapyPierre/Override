#pragma once

#include "CoreMinimal.h"
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

	UPROPERTY(EditAnywhere, Category = "Targeting")
	float ScreenPadding = -220;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	float MaxDistFromCursor = 220; // In Screen Space

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadOnly)
	float DistToTargetActor;
	
	UFUNCTION(BlueprintCallable)
	void TargetActor(AActor* Target);

	UFUNCTION(BlueprintCallable)
	void ClearCurrentTargets();

	FVector GetPointInSight() const;
	
protected:
	UPROPERTY(EditAnywhere, Category = "Targeting")
	float MaxTargetingDistance = 15000;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	float TargetingAccuracy = 180;
	
	virtual void BeginPlay() override;

private:
	float Angle;

	UPROPERTY()
	AActor* ClosestActor = nullptr;
	
	void LookForTarget();

	static bool IsPointOnTargetVisible(const FVector& Start, const FVector& Dir, const float Range, const AActor* Target,
												 const APlayerController* PC);

	static bool IsActorTargetable(AActor* Target);
};
