// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PlayerMovementComponent.generated.h"

class APlayerCharacter;

UENUM()
enum ECustomMovementMode
{
	CMOVE_Sprint = 0,
	CMOVE_Crouch = 1,
	CMOVE_Slide = 2,
};

UCLASS()
class OVERRIDE_API UPlayerMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
	
public:
	APlayerCharacter* CharacterRef;
	int32 FrameCounter = 0;
	float DefaultGroundFriction;
	float DefaultBrakingDecelerationWalking;
	float DefaultMaxWalkSpeedCrouched;

	bool IsCustomMovementModeOn(uint8 customMovementMode) const;

	virtual float GetMaxSpeed() const override;

	virtual bool IsMovingOnGround() const override;

#pragma region Sprint

	UPROPERTY(BlueprintReadOnly, Category = "CMC|Sprint")
	bool bWantsToSprint = false;

	UPROPERTY(BlueprintReadOnly, Category = "CMC|Sprint")
	float DefaultMaxWalkSpeed = 0;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "CMC|Sprint")
	float SprintSpeed = 825.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "CMC|Sprint")
	float SprintAcceleration = 200.f;
	
	float DefaultSprintSpeed = 0;
	float DefaultAcceleration = 0;
	
	virtual bool CanSprint() const;
	
#pragma endregion

#pragma region Slide

	float TimeSliding = 0.f;
	float MinimumSlideThreshold = -0.01f;
	bool bIsSliding = false;
	bool bPendingCancelSlide = false;

	float TimeToWaitBetweenSlide = 0;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly,  Category = "CMC|Slide")
	float SlidingCoolDown = 0.2;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly,  Category = "CMC|Slide", meta=(ToolTip="Le temps de Boost que va avoir le joueur, Si c'est 3 secondes, pendant 3sec il sera à la vitesse max du Slide"))
	float BoostSlidingTime = 1.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CMC|Slide", meta=(ToolTip="Le temps de l'easing de la vitesse du joueur, en gros le transfert Slide->Crouch"))
	float EaseOutTime = 0.2f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CMC|Slide")
	float SlideImpulse = 600.0f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CMC|Slide")
	float SlopeToleranceValue = 0.02;

	
	bool SlideLineTrace();

	FHitResult SlideHit;
	FTimeline VelocityEaseTimeline;
	FVector Impact;

	UPROPERTY(EditAnywhere, Category = "CMC|Slide")
	UCurveFloat* VelocityEaseCurve;

	FVector InitialEaseVelocity;
	FVector TargetEaseVelocity;

	UFUNCTION()
	void EaseVelocityUpdate(float Value);

	void StartVelocityEase(const FVector& NewTargetVelocity);

	UFUNCTION()
	void StopVelocityEaseTimeline();

	bool CanSlide();

	UFUNCTION(BlueprintCallable)
	bool IsSliding() const;

	UFUNCTION(BlueprintCallable)
	bool IsRunning() const;
	
#pragma endregion

#pragma region Jump
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CMC|Jump")
	float FirstJumpZVelocity = 800.f;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CMC|Jump")
	float SecondJumpZVelocity = 1000.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CMC|Jump")
	float SecondJumpAirControl = 0.05f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CMC|Jump")
	float AirHorizontalRetainPercent = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CMC|Jump")
	float CoyoteTime= 0.5f;
	
	FVector InitialHorizontalVelocity;
	
	UPROPERTY(Replicated)
	int32 JumpCount;
	
	float DefaultAirControl = 0;
	float DefaultBrakingDecelerationFalling = 0;
	
	void ResetJumpValues();
	
#pragma endregion

#pragma region EdgeGrab
	
	bool bGrabbedLedge = false;

	float GrabHeight = 0;

	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	FHitResult SweepResult;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="CMC|EdgeGrab")
	UAnimMontage* EdgeClimbMontage;
	
#pragma endregion

private:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	virtual void PhysSprint(float DeltaTime, int32 Iterations);

	virtual void PhysWalking(float DeltaTime, int32 Iterations) override;
	
	virtual void PhysSlide(float DeltaTime, int32 Iterations);

	virtual void PhysFalling(float DeltaTime, int32 Iterations) override;
	
	void ResetSlideValues();
	
	virtual bool DoJump(bool bReplayingMoves,  float DeltaTime) override;

	virtual bool CanAttemptJump() const override;
	
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual void Crouch(bool bClientSimulation = false) override;

	virtual void UnCrouch(bool bClientSimulation = false) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
