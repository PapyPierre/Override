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
	float DefaultMaxAcceleration;

	float BackwardSpeed;
	float SideSpeed;

	FVector CharaLocation;
	FVector CharaForward;
	FVector CharaUp;

	FCollisionQueryParams TraceParams;

	UAnimInstance* AnimInstance;

	virtual bool IsMovingOnGround() const override;
	bool IsCustomMovementModeOn(uint8 customMovementMode) const;

	UFUNCTION(BlueprintCallable)
	void Slow(float Duration);

#pragma region Dash
	UPROPERTY()
	FTimeline DashTimeline;
	
	float EaseOutTimeDash = 0.15f;
	float DashImpulse = 2000.f;
	float ResetDashCooldown = 0.3f;
	float DashCooldownRemaining = 0;
	bool bIsDashCoolingDown = false;
	
	FVector MoveDirectionMelee;
	FTimerHandle SimpleDelayHandle;

	void OnDelayFinishedDash();
	void DashCooldown(float DeltaSeconds);

	UPROPERTY(BlueprintReadOnly, Category = "CMC|CaC")
	bool bIsDashing = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CMC|CaC")
	bool bWantsToDash;

	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_GetInputLastDirection(const FVector& Direction);
#pragma endregion

#pragma region Slide
	UPROPERTY(BlueprintReadOnly)
	bool bIsSliding = false;
	
	UPROPERTY(Replicated)
	FVector VelocityAtCrouch;
	
	bool bResetSlideCrouch = false;
	bool bResetSlideLanded = true;
	bool bWantsToSlide = false;
	bool SlideLineTrace();
	bool bCoolDownFinished = true;
	
	float TimeSliding = 0.f;
	float SlideImpulse;
	float SlopeToleranceValue;
	
	float SlideCooldownDuration = 0.3f;
	float SlideCooldownRemaining = 0;
	float SpeedIncreaseInSlope = 20000;
	float SpeedDecreaseInSlope = 1000;
	float FrictionInSlide = 0.25f;
	float BrakingDecelerationInSlide = 750;
	float MaxVelocityForSlide;
	float MaxAccelerationForSlide;
	
	FHitResult SlideHit;
	FVector Impact;
	
	UFUNCTION(BlueprintCallable)
	bool IsSliding() const;
	void DebugSlideNetwork(const FString& Context);
	bool CanSlide();
	void ResetSlideValues();
	void ExitSlide(float DeltaTime, int32 Iterations);
	
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
	bool IsSlowed;
	float SlowTimer;
	float SlowDuration;
	
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void PhysDash(float DeltaTime, int32 Iterations);

	virtual void PhysSlide(float DeltaTime, int32 Iterations);
	
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	
	virtual void PhysWalking(float DeltaTime, int32 Iterations) override;

	virtual void PhysFalling(float DeltaTime, int32 Iterations) override;
	
	virtual bool DoJump(bool bReplayingMoves,  float DeltaTime) override;

	virtual bool CanAttemptJump() const override;
	
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual float GetMaxSpeed() const override;
	
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	virtual void Crouch(bool bClientSimulation = true) override;

	virtual void UnCrouch(bool bClientSimulation = true) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void DebugPrintClientIds();

	void HandleSlow(float DeltaTime);
};

class FSavedMove_MyMovement : public FSavedMove_Character
{
public:

    #define FLAG_STOP_SLIDE 0x10
	#define FLAG_WANT_TO_DASH 0x40
	#define FLAG_WANT_TO_SLIDE 0x80 
	
	typedef FSavedMove_Character Super;

	//Dash
	FVector SavedMoveDirection;
	bool bWantsToDash;

	//Slide
	bool bWantsToSlide;
	
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
