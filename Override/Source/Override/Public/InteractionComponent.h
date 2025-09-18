#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent))
class OVERRIDE_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite)
	APlayerController* PlayerController; // Set in Character BP
	
	UPROPERTY(BlueprintReadOnly)
	AActor* DefaultTargetToHack;

	UPROPERTY(BlueprintReadOnly)
	AActor* HackableActorInSight;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Hack")
	float HackDetectionDistance_View = 1000;
	
	UPROPERTY(EditDefaultsOnly, Category = "Hack")
	float HackDetectionDistance_Auto = 500;

	UFUNCTION(BlueprintCallable)
	void TryInteract();

	UFUNCTION(Server, Reliable)
	void RPC_TryInteractWithActor(AActor* Target);

private:
	void LookForHackable();

	AActor* FindClosestHackableActor() const;
};
