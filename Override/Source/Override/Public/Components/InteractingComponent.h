#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractingComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent))
class OVERRIDE_API UInteractingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void TryInteractWithActor(AActor* Target);

	UFUNCTION(Server, Reliable)
	void RPC_TryInteractWithActor(AActor* Target);

private:

};
