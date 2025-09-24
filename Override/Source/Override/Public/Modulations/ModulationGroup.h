// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ModulationGroup.generated.h"

class UTargetingComponent;
class AModulation;

UCLASS()
class OVERRIDE_API AModulationGroup : public AActor
{
	GENERATED_BODY()

public:
	AModulationGroup();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Default")
	TArray<AModulation*> ModulationsInGroup;

	void TargetGroup(UTargetingComponent* TargetingComponent);

	void UntargetGroup(UTargetingComponent* TargetingComponent);

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
};
