#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Hackable.h"
#include "Interface/Interactable.h"
#include "Modulation.generated.h"

class AModulationGroup;

UENUM(BlueprintType)
enum class ModState : uint8
{
	Stopped UMETA(DisplayName = "Stopped"),
	Moving UMETA(DisplayName = "Moving"),
	InCD UMETA(DisplayName = "In Cooldown"),
};

UCLASS()
class OVERRIDE_API AModulation : public AActor, public IInteractable, public IHackable
{
	GENERATED_BODY()

public:
	AModulation();

	virtual void Tick(float DeltaTime) override;

	virtual void Interact() override;

	virtual void Hack() override;

	UPROPERTY(BlueprintReadOnly)
	AModulationGroup* Group;

	int CurrentEndIndex = 0;

	FTransform Start;

	UPROPERTY(EditInstanceOnly, Category="Default", meta=(MakeEditWidget))
	TArray<FTransform> Ends;

	FTransform CurrentStart;
	FTransform CurrentEnd;

	UPROPERTY(EditAnywhere, Category="Default")
	UCurveFloat* ModSpeedCurve;


	UPROPERTY(BlueprintReadOnly)
	ModState CurrentState = ModState::Stopped;

	UPROPERTY(EditAnywhere, Category="Default")
	float CooldownDuration = 2;

	UPROPERTY(EditAnywhere, Category="Default")
	bool ApplyImpulseOnEndReach = false;

	UPROPERTY(EditAnywhere, Category="Default")
	float ImpulseForce = 5;



protected:
	virtual void BeginPlay() override;

	void HandleMovement(float DeltaTime);

	void HandleCooldown(float DeltaTime);

	void ChangeState(ModState newState);

	UFUNCTION(BlueprintImplementableEvent)
	void OnStateChanged(ModState newState);

private:
	float LerpTime;

	float CdTime;
	
	void StopMovement();
	
	void ApplyImpulseOnPlayer(FVector Dir);
};
