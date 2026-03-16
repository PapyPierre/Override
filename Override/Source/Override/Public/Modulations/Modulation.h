#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "Player/PlayerCharacter.h"
#include "Modulation.generated.h"

class AModulationGroup;

UENUM(BlueprintType)
enum class ModState : uint8
{
	Stopped UMETA(DisplayName = "Stopped"),
	Moving UMETA(DisplayName = "Moving"),
	InCD UMETA(DisplayName = "In Cooldown"),
	Locked UMETA(DisplayName = "Locked"),
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

	virtual void OnTarget_Implementation(AActor* TargetingActor) override;
	
	virtual void OnInteract_Implementation(AActor* InteractingActor) override;

	UPROPERTY(BlueprintReadOnly)
	AModulationGroup* Group;

	UPROPERTY(BlueprintReadOnly)
	int CurrentEndIndex = 0;

	FTransform Start;

	UPROPERTY(EditInstanceOnly, Category="Default", meta=(MakeEditWidget))
	TArray<FTransform> Ends;

	UPROPERTY(BlueprintReadOnly, Replicated)
	FTransform CurrentStart;

	UPROPERTY(BlueprintReadOnly, Replicated)
	FTransform CurrentEnd;

	UPROPERTY(EditAnywhere, Category="Default")
	UCurveFloat* ModSpeedCurve;

	UPROPERTY(BlueprintReadOnly)
	ModState CurrentState = ModState::Stopped;
	
	UPROPERTY(BlueprintReadWrite, Category="Default")
	bool IsDemat;

	UPROPERTY(EditAnywhere, Category="Default")
	float CooldownDuration = 0;

	UPROPERTY(EditAnywhere, Category="Default")
	float LockDuration = 2;

	UPROPERTY(EditAnywhere, Category="Default")
	bool ApplyImpulseOnEndReach = false;

	UPROPERTY(EditAnywhere, Category="Default")
	float ImpulseForce = 5;

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> CurrentlyCastedGE;

	UFUNCTION(BlueprintCallable)
	void StartCastingGE(TSubclassOf<UGameplayEffect> GameplayEffect, float CastDuration);

#pragma region Attribute
	
	UPROPERTY()
	TObjectPtr<class UHealthAttributeSet> HealthSet;
	
#pragma endregion

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Default")
	bool nextMoveIsA;
	
	virtual void BeginPlay() override;
	
	void UpdateCurrentEnd();

	void HandleMovement(float DeltaTime);

	void HandleCooldown(float DeltaTime);

	void HandleLock(float DeltaTime);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Lock(bool ResetLockCD);

	UFUNCTION(NetMulticast, Reliable)
	void RPC_ChangeState(ModState NewState);

	UFUNCTION(BlueprintImplementableEvent)
	void OnStateChanged(ModState NewState);

	UFUNCTION(BlueprintImplementableEvent)
	void OnPostInteract(AActor* InteractingActor);
	
private:
	UPROPERTY(Replicated)
	float LerpTime;

	float CdTime;

	float LockTime;
	
	float HackCastingDuration = 0;
	float CastingTime;

	ModState PreviousState;

	UFUNCTION(NetMulticast, Reliable)
	void RPC_StopMovement();
	
	void ApplyImpulseOnPlayer() const;

	void ManageHackCastingCooldown(float DeltaTime);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
