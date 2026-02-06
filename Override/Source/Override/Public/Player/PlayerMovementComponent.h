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

	float BackwardSpeed;
	float SideSpeed;

	FVector CharaLocation;
	FVector CharaForward;
	FVector CharaUp;

	FCollisionQueryParams TraceParams;

	UAnimInstance* AnimInstance;

	virtual bool IsMovingOnGround() const override;

#pragma region Dash
	UPROPERTY()
	FTimeline DashTimeline;
	
	float EaseOutTimeDash = 0.15f;
	float DashImpulse = 2000.f;
	
    uint8 bWantsToDodge : 1;
	
	FVector MoveDirectionMelee;
	FTimerHandle SimpleDelayHandle;

	void OnDelayFinished();

	UPROPERTY(BlueprintReadOnly, Category = "CMC|CaC", Replicated)
	bool bIsMelee = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CMC|CaC")
	bool bWantsToDash;

	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_GetInputLastDirection(const FVector& Direction);

	UFUNCTION()
	void StopDashVelocityEaseTimeline();
#pragma endregion

#pragma region Slide

	UPROPERTY(Replicated)
	float TimeSliding = 0.f;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsSliding = false;
	bool bResetSlideCrouch = false;
	bool bResetSlideLanded = true;	
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

	UFUNCTION()
	void StopVelocityEaseTimeline();
	
	void StartVelocityEase(const FVector& NewTargetVelocity);

	bool CanSlide();

	void ResetSlideValues();

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

private:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void PhysMelee(float DeltaTime, int32 Iterations);
	
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	
	virtual void PhysWalking(float DeltaTime, int32 Iterations) override;
	
	virtual void PhysSlide(float DeltaTime, int32 Iterations);

	virtual void PhysFalling(float DeltaTime, int32 Iterations) override;
	
	virtual bool DoJump(bool bReplayingMoves,  float DeltaTime) override;

	virtual bool CanAttemptJump() const override;
	
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	bool IsCustomMovementModeOn(uint8 customMovementMode) const;

	virtual float GetMaxSpeed() const override;
	
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	virtual void Crouch(bool bClientSimulation = true) override;

	virtual void UnCrouch(bool bClientSimulation = true) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void DebugPrintClientIds();
};

class FSavedMove_MyMovement : public FSavedMove_Character
{
public:

	typedef FSavedMove_Character Super;

	FVector SavedMoveDirection;
	uint8 bSavedWantsToDodge : 1;

	///@brief Resets all saved variables.
	virtual void Clear() override;

	///@brief Store input commands in the compressed flags.
	virtual uint8 GetCompressedFlags() const override;

	///@brief This is used to check whether or not two moves can be combined into one.
	///Basically you just check to make sure that the saved variables are the same.
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;

	///@brief Sets up the move before sending it to the server.
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData) override;
	///@brief Sets variables on character movement component before making a predictive correction.
	virtual void PrepMoveFor(class ACharacter* Character) override;
};

class FNetworkPredictionData_Client_MyMovement : public FNetworkPredictionData_Client_Character
{
public:
	FNetworkPredictionData_Client_MyMovement(const UCharacterMovementComponent& ClientMovement);

	typedef FNetworkPredictionData_Client_Character Super;

	///@brief Allocates a new copy of our custom saved move
	virtual FSavedMovePtr AllocateNewMove() override;
};
