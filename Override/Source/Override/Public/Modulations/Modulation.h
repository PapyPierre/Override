#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
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
class OVERRIDE_API AModulation : public AActor, public IInteractable, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AModulation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> Asc;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void Tick(float DeltaTime) override;

	virtual void Target() override;
	
	virtual void Interact() override;

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

	UPROPERTY(EditDefaultsOnly, Category = "Hack")
	FGameplayTag CurrentlyAppliedHack;

	UPROPERTY(BlueprintReadWrite, Category = "Hack")
	float HackCastingDuration = 0;

#pragma region Attribute
	
	UPROPERTY()
	TObjectPtr<class UHealthAttributeSet> HealthSet;
	
#pragma endregion

protected:
	virtual void BeginPlay() override;

	void HandleMovement(float DeltaTime);

	void HandleCooldown(float DeltaTime);

	void ChangeState(ModState newState);

	UFUNCTION(BlueprintImplementableEvent)
	void OnStateChanged(ModState newState);

	UFUNCTION(BlueprintCallable)
	void CastHackEventWithData(FGameplayTag EventTag, AActor* Target);
	
private:
	float LerpTime;

	float CdTime;

	float CastingTime;
	
	void StopMovement();
	
	void ApplyImpulseOnPlayer(FVector Dir);

	void ManageHackCastingCooldown(float DeltaTime);
};
