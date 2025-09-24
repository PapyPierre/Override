#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HackingComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OVERRIDE_API UHackingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHackingComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
