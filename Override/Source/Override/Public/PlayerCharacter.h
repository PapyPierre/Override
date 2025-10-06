// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class OVERRIDE_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(BlueprintReadOnly)
	UPlayerMovementComponent* PlayerMovementComponent;

	UPROPERTY(BlueprintReadOnly)
	APlayerController* PlayerController;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	float DefaultFOV = 90.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	float SprintFOV = 100.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	float FOVInterpSpeed = 10.f;

	float DefaultCoyoteTime = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeIdle;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeRunning;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeWalk;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	TSubclassOf<UCameraShakeBase> ShakeJump;
	
#pragma region WallRun
	FHitResult WallRunHitResult;
#pragma endregion

#pragma region Sprint
	UFUNCTION(BlueprintCallable, Category = "CMC|Sprint")
	void Sprint();

	UFUNCTION(BlueprintCallable, Category = "CMC|Sprint")
	void StopSprint();
	
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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Landed(const FHitResult& Hit) override;

	virtual void Falling() override;

	virtual void Jump() override;

	virtual void Crouch(bool bClientSimulation = false) override;

	virtual bool CanJumpInternal_Implementation() const override;

	UFUNCTION(Server, Reliable)
	void Server_SetCrouchVelocity(const FVector& InVelocity);
	
	UPROPERTY(VisibleAnywhere, Category = Camera)
	APlayerCameraManager* FirstPersonCameraComponent;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void CameraShake();

	FTimerHandle JumpDelayHandle;

	UFUNCTION()
	void OnJumpDelayFinished();
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
