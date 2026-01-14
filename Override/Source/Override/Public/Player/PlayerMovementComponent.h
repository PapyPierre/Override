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
	CMOVE_Melee = 0,
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
	float DefaultMaxWalkSpeed;
	
	bool IsCustomMovementModeOn(uint8 customMovementMode) const;

	virtual float GetMaxSpeed() const override;

	virtual bool IsMovingOnGround() const override;

#pragma region Melee
	UPROPERTY()
	FTimeline DashMeleeTimeline;
	
	float EaseOutTimeMelee = 0.15f;
	float MeleeImpulse = 2000.f;
	
	FVector DirectionMelee;
	FTimerHandle SimpleDelayHandle;

	void OnDelayFinished();

	UPROPERTY(BlueprintReadOnly, Category = "CMC|CaC", Replicated)
	bool bIsMelee = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CMC|CaC")
	bool bWantsToDash;

	UFUNCTION()
	void MeleeVelocityUpdate(float Value);

	UFUNCTION(Server, Reliable)
	void Server_GetForwardCamera(FVector Direction);

	UFUNCTION()
	void StopMeleeVelocityEaseTimeline();
#pragma endregion

#pragma region Slide

	UPROPERTY(Replicated)
	float TimeSliding = 0.f;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsSliding = false;
	bool bResetSlideCrouch = false;
	bool bResetSlideLanded = true;

	UPROPERTY(Replicated)
	bool bPendingCancelSlide = false;
	
	bool bCoolDownFinished = true;
	
	UPROPERTY(Replicated)
	FVector VelocityAtCrouch;

	float SlidingCoolDown;
	float BoostSlidingTime;
	float EaseOutTime;
	float SlideImpulse;
	float SlopeToleranceValue;
	float MinDiffVelocityToAllowSlide;
	float MaxVelocityForSlide;
	
	float TimeToWaitBetweenSlide = 0;
	
	bool SlideLineTrace();

	EMovementMode _PreviousMovementMode;
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
	
#pragma endregion

#pragma region Jump
	float FirstJumpZVelocity;
	float CoyoteTime;
	
	FVector InitialHorizontalVelocity;
	int JumpCount = 0;
	float JumpResetTime;
	
	float DefaultAirControl = 0;
	float DefaultBrakingDecelerationFalling = 0;

	UCurveFloat* JumpCurve;
	FTimeline JumpTimeline;

	UFUNCTION()
	void OnJumpTimelineFinished();
#pragma endregion

#pragma region Parkour

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "CMC|EdgeGrab")
	bool bGrabbedLedge = false;
	
	float MaxVaultThickness;
	float MaxVaultHeight;
	float RaycastStartHeight;
	float RaycastEndHeight;
	
	UAnimMontage* EdgeClimbMontage;
	UAnimMontage* VaultMontage;
	float ParkourDistanceDetection = 70.f;

	float IncomingWallThickness;

	UFUNCTION()
	void OnMoveNoOp() {}
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayWallClimbMontage(UAnimMontage* Montage, FName EndCallbackFunctionName, AActor* Wall, APlayerCharacter* Player);
	
	UFUNCTION(Client, Reliable)
	void RPC_WallClimbMoveTo(AActor* Wall);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_CapsuleMoveTo(UCapsuleComponent* Capsule, FVector Location);

	UFUNCTION(Server, Reliable)
	void Server_CallVaultAnimation(AActor* Actor , FVector EndLocation);
	
	UFUNCTION()
	void OnMontageVaultEnded(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void OnMontageWallClimbEnded(UAnimMontage* Montage, bool bInterrupted);

	bool CanVaultOrClimb();
	AActor* ParkourWallDetection(float &Thickness, float &Height);
	FHitResult SweepResult;
	AActor* HitSecondWallActor;
	AActor* MultiPlayerHitWall;
	bool bMontagePending = false;
	bool bDebugLedge = false;
	
#pragma endregion

private:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;


	virtual void PhysMelee(float DeltaTime, int32 Iterations);
	
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	
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
	void DebugPrintClientIds();

	FVector CharaLocation;
	FVector CharaForward;
	FVector CharaUp;

	FCollisionQueryParams TraceParams;

	UAnimInstance* AnimInstance;
};
