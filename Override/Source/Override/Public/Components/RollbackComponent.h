#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RollbackComponent.generated.h"

USTRUCT(BlueprintType)
struct FPlayerSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;
	UPROPERTY()
	FRotator Rotation;
	UPROPERTY()
	float Health;
	UPROPERTY()
	float Timestamp;
};

UCLASS(meta=(BlueprintSpawnableComponent))
class OVERRIDE_API URollbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URollbackComponent();

	UPROPERTY(EditDefaultsOnly, Category="Rollback")
	float RollbackSpeed = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category="Rollback")
	float MaxHistoryDuration = 3.0f;

	void CaptureSnapshot();

	void StartRollback();
	void StopRollback();
	bool IsRollingBack() const { return bIsRollingBack; }

protected:
	void ApplySnapshot(float TargetTime);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	bool bIsRollingBack = false;
	float RollbackTargetTime = 0.f;

	TArray<FPlayerSnapshot> History;
	void PurgeOldSnapshots();
};
