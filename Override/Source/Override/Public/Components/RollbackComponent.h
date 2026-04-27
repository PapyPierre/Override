#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Modulations/Modulation.h"
#include "RollbackComponent.generated.h"

USTRUCT(BlueprintType)
struct FPlayerSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	FVector PlayerLoc;
	UPROPERTY()
	FRotator PlayerRot;
	UPROPERTY()
	FRotator ControlRot;
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

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Rollback")
	float RollbackSpeed = 1.6f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Rollback")
	float MaxHistoryDuration = 3.0f;

	void CaptureSnapshot();

	UFUNCTION(BlueprintCallable)
	void StartRollback();

	UFUNCTION(BlueprintCallable)
	void StopRollback();
	bool IsRollingBack() const { return bIsRollingBack; }

protected:
	void ApplySnapshotOnServer(float TargetTime);

	UFUNCTION(Client, Unreliable)
	void Client_ApplySnapshot(FRotator NewControlRot);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_IsRollingBack)
	bool bIsRollingBack = false;

	UFUNCTION()
	void OnRep_IsRollingBack();

	UPROPERTY(EditDefaultsOnly)
	UInputMappingContext* IMC_MouseLook;

	float RollbackTargetTime = 0.f;

	TArray<FPlayerSnapshot> History;

	void PurgeOldSnapshots();
};
