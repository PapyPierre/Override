#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PlayerMovementComponent.generated.h"

class APlayerCharacter;
class UMovementStats;

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement|Config")
	UMovementStats* MovementData;

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

	float DefaultMaxWalkSpeed = 0;
	float DefaultSprintSpeed = 0;
	float DefaultAcceleration = 0;
	
	float SprintSpeed;
	float SprintAcceleration;
	virtual bool CanSprint() const;
#pragma endregion

#pragma region Slide

	float TimeSliding = 0.f;
	bool bIsSliding = false;
	bool bPendingCancelSlide = false;
	bool bCoolDownFinished = false;

	UPROPERTY(Replicated)
	FVector VelocityAtCrouch;

	float SlidingCoolDown;
	float BoostSlidingTime;
	float EaseOutTime;
	float SlideImpulse;
	float SlopeToleranceValue;
	
	float TimeToWaitBetweenSlide = 0;
	
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
	float FirstJumpZVelocity;
	float SecondJumpZVelocity;
	float SecondJumpAirControl;
	float AirHorizontalRetainPercent;
	float CoyoteTime;
	
	FVector InitialHorizontalVelocity;
	int32 JumpCount;
	
	float DefaultAirControl = 0;
	float DefaultBrakingDecelerationFalling = 0;
	
	void ResetJumpValues();
#pragma endregion

#pragma region EdgeGrab
	
	bool bGrabbedLedge = false;
	float GrabHeight = 0;

	float MaxVaultThickness;
	float MaxVaultHeight;
	float RaycastStartHeight;
	float RaycastEndHeight;

	UAnimMontage* EdgeClimbMontage;
	UAnimMontage* VaultMontage;
	float ParkourDistanceDetection = 70.f;
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayWallClimbMontage(UAnimMontage* Montage, FName EndCallbackFunctionName);
	
	UFUNCTION(Client, Reliable)
	void RPC_WallClimbMoveTo(UCapsuleComponent* Capsule, FVector TargetRelativeLocation,FLatentActionInfo JumpDelayInfo);

	UFUNCTION(Server, Reliable)
	void Server_CallVaultAnimation(AActor* Actor);
	
	UFUNCTION()
	void OnMontageVaultEnded(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void OnMontageWallClimbEnded(UAnimMontage* Montage, bool bInterrupted);

	bool CanVaultOrClimb();
	AActor* ParkourWallDetection(float &Thickness, float &Height);
	FHitResult SweepResult;
	AActor* HitSecondWallActor;
	bool bMontagePending = false;	
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

	virtual void Crouch(bool bClientSimulation = true) override;

	virtual void UnCrouch(bool bClientSimulation = true) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FVector CharaLocation;
	FVector CharaForward;
	FVector CharaUp;

	FCollisionQueryParams TraceParams;

	UAnimInstance* AnimInstance;
};
