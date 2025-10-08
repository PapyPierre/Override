#pragma once

#include "CoreMinimal.h"
#include "CustomPlayerState.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "PlayerMovementComponent.h"
#include "Components/TargetingComponent.h"
#include "Interface/Targetable.h"
#include "PlayerCharacter.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class OVERRIDE_API APlayerCharacter : public ACharacter, public ITargetable, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(BlueprintReadOnly)
	APlayerController* PlayerController;
	
	virtual void Tick(float DeltaTime) override;

	virtual void Target() override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* InputMappingContext;

#pragma region Hack
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Hack1Action;
    
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Hack2Action;
    
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Hack3Action;
	
	UPROPERTY(EditDefaultsOnly, Category = "Hack")
	FGameplayTag Hack1Tag;
    
	UPROPERTY(EditDefaultsOnly, Category = "Hack")
	FGameplayTag Hack2Tag;
    
	UPROPERTY(EditDefaultsOnly, Category = "Hack")
	FGameplayTag Hack3Tag;

	
#pragma endregion

#pragma region Components
    	UPROPERTY(BlueprintReadOnly)
    	UPlayerMovementComponent* PlayerMovementComponent;
    
    	UPROPERTY(BlueprintReadWrite)
    	UTargetingComponent* TargetingComponent;
    #pragma endregion

#pragma region FOV
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	float DefaultFOV = 90.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	float SprintFOV = 100.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	float FOVInterpSpeed = 10.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeIdle;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeRunning;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeWalk;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeJump;

	void CameraShake();
#pragma endregion
	
#pragma region WallRun
	FHitResult WallRunHitResult;
#pragma endregion

#pragma region Sprint
	UFUNCTION(BlueprintCallable, Category = "CMC|Sprint")
	void Sprint();

	UFUNCTION(BlueprintCallable, Category = "CMC|Sprint")
	void StopSprint();
#pragma endregion

	float DefaultCoyoteTime = 0.5f;

#pragma region Jump
	FTimerHandle JumpDelayHandle;

	UFUNCTION()
	void OnJumpDelayFinished();
#pragma endregion
	
	UFUNCTION(Server, Reliable)
	void RPC_SetSprint(bool value);	

#pragma endregion

#pragma region Aim
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void AimWeapon();

	UFUNCTION(BlueprintCallable, Category = "Aim")
	void StopAimWeapon();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetAim(bool bNewAiming);

	bool ServerSetAim_Validate(bool bNewAiming);
	void ServerSetAim_Implementation(bool bNewAiming);

	void SetAimingState(bool bNewAiming);
	void UpdateAimingSettings();

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Aim")
	float AimFOV = 70.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_IsAimingWeapon, Category = "Aim")
	bool bIsAimingWeapon = false;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Aim")
	float AimCrouchedSpeed = 100.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Aim")
	float AimSpeed = 300.f;

	UFUNCTION(BlueprintCallable)
	void OnRep_IsAimingWeapon();

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Aim")
	float MouseSensitivity = 1.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Aim")
	float MouseAimSensitivity = 0.4f;

	UFUNCTION(BlueprintImplementableEvent, Category="Replication")
	void OnRep_IsAimingWeapon_BP();

#pragma endregion

	UFUNCTION(BlueprintCallable)
	ACustomPlayerState* GetCustomPlayerState() const;
	
protected:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	APlayerCameraManager* FirstPersonCameraComponent;
	
	virtual void BeginPlay() override;

	virtual void Landed(const FHitResult& Hit) override;

	virtual void Falling() override;

	virtual void Jump() override;

	virtual void Crouch(bool bClientSimulation = false) override;

	virtual bool CanJumpInternal_Implementation() const override;

	UFUNCTION(Server, Reliable)
	void Server_SetCrouchVelocity(const FVector& InVelocity);
	
	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnPostAbilitySystemInit();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void SetControllerRef();
	
	void InitAbilitySystem();
	
	void ActivateHack1();
	void ActivateHack2();
	void ActivateHack3();
    
	void SendHackEventWithData(FGameplayTag EventTag);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
