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
	CMOVE_WallRide = 3,
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

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "CMC|Sprint")
	float SprintSpeedMultiplier = 1.5f;

	virtual bool CanSprint() const;
	
#pragma endregion
	
#pragma region WallRide
	
	UPROPERTY(BlueprintReadOnly, Category = "CMC|WallRun")
	bool isWallRunning = false;

	UPROPERTY(BlueprintReadOnly, Category = "CMC|WallRun")
	float RightAxis;

	UPROPERTY(BlueprintReadOnly, Category = "CMC|WallRun")
	float ForwardAxis;

	UPROPERTY(BlueprintReadOnly, Category = "CMC|WallRun")
	bool CloseToWall;

	UPROPERTY(EditAnywhere, Category = "CMC|WallRun")
	FVector PlayerDirection;

	UPROPERTY(BlueprintReadWrite, Category = "CMC|WallRun")
	bool bWantToWallRide = false;

	UPROPERTY(EditAnywhere, Category = "CMC|WallRun")
	UCurveFloat* WallRideCurve;

	FTimeline WallRideTimeline;
	bool bPendingWallRide = false;

	UFUNCTION()
	void OnWallRideTimelineTick(float Value);

	UFUNCTION()
	void OnWallRideTimelineFinished();

	float TimeWallRunning;

	bool CanWallRun() const;
	bool bHasResetWallRide = true;

#pragma endregion

#pragma region Slide

	float TimeSliding = 0.f;
	float MinimumSlideThreshold = -0.01f;
	bool bIsSliding = false;

	float TimeToWaitBetweenSlide = 0;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly,  Category = "CMC|Slide")
	float SlidingCoolDown = 0.2;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly,  Category = "CMC|Slide")
	float MaxSlidingTime = 0.8f;

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

	virtual bool CanSlide() const;

	UFUNCTION(BlueprintCallable)
	bool IsSliding() const;
	
#pragma endregion

#pragma region Jump
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "CMC|Jump")
	float JumpDeceleration = 20.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "CMC|Jump")
	float MinJumpHorizontalSpeed = 550.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CMC|Jump")
	float FirstJumpZVelocity = 800.f;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "CMC|Jump")
	float SecondJumpZVelocity = 1000.f;
	
	int JumpCount = 0;
	
#pragma endregion

#pragma region EdgeGrab
	
	bool bGrabbedLedge = false;

	float GrabHeight = 0;
	
#pragma endregion

#pragma region Crouch
	bool bIsCrouched = false;
#pragma endregion

private:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

	virtual void PhysSprint(float DeltaTime, int32 Iterations);

	virtual void PhysSlide(float DeltaTime, int32 Iterations);

	virtual void PhysFalling(float DeltaTime, int32 Iterations) override;

	void ResetSlideValues();

	virtual void PhysWallRide(float DeltaTime, int32 Iterations);

	virtual bool CanAttemptJump() const override;

	virtual bool DoJump(bool bReplayingMoves,  float DeltaTime) override;

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual void Crouch(bool bClientSimulation = false) override;

	virtual void UnCrouch(bool bClientSimulation = false) override;
};
