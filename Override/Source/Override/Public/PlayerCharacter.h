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

	APlayerController* PlayerController;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	float DefaultFOV = 90.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	float SprintFOV = 100.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FOV")
	float FOVInterpSpeed = 10.f;

#pragma region WallRun
	FHitResult WallRunHitResult;
#pragma endregion

#pragma region Sprint
	UFUNCTION(BlueprintCallable, Category = "CMC|Sprint")
	void Sprint();

	UFUNCTION(Server, Reliable)
	void RPC_SetSprint(bool value);

	UFUNCTION(BlueprintCallable, Category = "CMC|Sprint")
	void StopSprint();
#pragma endregion
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Landed(const FHitResult& Hit) override;

	virtual bool CanJumpInternal_Implementation() const override;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	APlayerCameraManager* FirstPersonCameraComponent;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	
};
