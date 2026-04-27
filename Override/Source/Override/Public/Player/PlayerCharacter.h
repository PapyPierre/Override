#pragma once

#include "CoreMinimal.h"
#include "CustomPlayerState.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "PlayerMovementComponent.h"
#include "CameraManager.h"
#include "Components/TargetingComponent.h"
#include "Interface/Targetable.h"
#include "Abilities/AbilitySlotComponent.h"
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

	UPROPERTY(BlueprintReadOnly)
	UCameraManager* CameraManager;
	
	UPROPERTY(VisibleAnywhere, Category = Camera)
	APlayerCameraManager* FirstPersonCameraComponent;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* InputMappingContext;

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Dash")
	void Dash();

#pragma region GAS
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Ability1Action;
    
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* Ability2Action;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FGameplayTag Ability1Tag;
    
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FGameplayTag Ability2Tag;

	UPROPERTY(BlueprintReadOnly)
	UAbilitySlotComponent* AbilitySlotComponent;

	UFUNCTION(Server, Reliable)
	void ActivateAbilityInSlotRPC(int32 SlotIndex, FVector CurrentPointInSight, const TArray<AActor*>& Targets);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void GiveCharacterAbilities(TArray<TSubclassOf<UGA_BaseAbility>> Abilities);

	UFUNCTION(BlueprintImplementableEvent)
	void OnRollingBack(bool IsRollingBack);

#pragma endregion

#pragma region Components
    	UPROPERTY(BlueprintReadOnly)
    	UPlayerMovementComponent* PlayerMovementComponent;
    
    	UPROPERTY(BlueprintReadWrite)
    	UTargetingComponent* TargetingComponent;
    #pragma endregion

#pragma region FOV
	float DefaultFOV;
	float WalkFOV;
	float SlideFOV;
	float DashFOV;

	float FOVInterpSlideSpeed;
	float FOVInterpSprintSpeed;
	float FOVInterpAimSpeed;
	float FOVInterpNormalSpeed;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	UCurveFloat* CurveWalkStart;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	UCurveFloat* CurveIdleStart;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	UCurveFloat* CurveSlideStart;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	UCurveFloat* CurveSlideEnd;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	UCurveFloat* CurveDashStart;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	UCurveFloat* CurveAimStart;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	UCurveFloat* CurveAimEnd;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeIdle;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeJump;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeSlide;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeWalk;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeWalkCrouch;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeLanding;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeDash;
#pragma endregion
		
#pragma region WallRun
	FHitResult WallRunHitResult;
#pragma endregion

#pragma region Jump
	FTimerHandle JumpDelayHandle;
	
	float DefaultCoyoteTime = 0.5f;

	UPROPERTY(BlueprintReadOnly)
	FVector LastGroundedPosition;
	
	UFUNCTION()
	void OnJumpDelayFinished();
#pragma endregion

#pragma region Aim
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void AimWeapon();

	UFUNCTION(BlueprintCallable, Category = "Aim")
	void StopAimWeapon();
	
	UFUNCTION(Server, Reliable)
	void ServerSetAim(bool bNewAiming);

	bool ServerSetAim_Validate(bool bNewAiming);
	void ServerSetAim_Implementation(bool bNewAiming);

	void SetAimingState(bool bNewAiming);
	void UpdateAimingSettings();

	float AimFOV;
	float AimCrouchedSpeed;
	float AimSpeed;
	UPROPERTY(BlueprintReadOnly)
	float DefaultMouseSensitivity;
	UPROPERTY(BlueprintReadOnly)
	float CurrentMouseSensitivity;
	UPROPERTY(BlueprintReadOnly)
	float MouseAimSensitivity;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_IsAimingWeapon, Category = "Aim")
	bool bIsAimingWeapon = false;

	UFUNCTION(BlueprintCallable)
	void OnRep_IsAimingWeapon();
	
	UFUNCTION(BlueprintImplementableEvent, Category="Replication")
	void OnRep_IsAimingWeapon_BP();
#pragma endregion

	void Launch(const FVector& Force);

	UFUNCTION(BlueprintCallable)
	ACustomPlayerState* GetCustomPlayerState() const;
	void UseAbility(int index);

protected:
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

	UFUNCTION(BlueprintImplementableEvent)
	void OnAbilityActivated(int SlotIndex, bool IsSelfCast);

private:
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> TPV;
	
	void SetControllerRef();
	
	void InitAbilitySystem();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
