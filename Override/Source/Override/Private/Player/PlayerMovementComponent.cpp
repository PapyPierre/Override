#include "Player/PlayerMovementComponent.h"
#include "Player/PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/MovementStats.h"

void UPlayerMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	//INITIALIZE DATA ASSET
	if (MovementData)
	{
		//Speed
		BackwardSpeed = MovementData->SpeedBackwardReduction;
		SideSpeed = MovementData->SpeedSideReduction;
		SlowedSpeed = MovementData->SlowedSpeedReduction;
		ShootingSpeed = MovementData->SpeedShootingReduction;

		//Slide
		SlideImpulse = MovementData->SlideImpulse;
		SlopeToleranceValue = MovementData->SlopeToleranceValue;
		SlideCooldownDuration = MovementData->SlideCoolDownDuration;
		MaxVelocityForSlide = MovementData->MaxVelocityForSlide;
		SpeedDecreaseInSlope = MovementData->SpeedDecreaseInSlope;
		SpeedIncreaseInSlope = MovementData->SpeedIncreaseInSlope;
		MaxAccelerationForSlide = MovementData->MaxAccelerationForSlide;
		BrakingDecelerationInSlide = MovementData->BrakingDeceleration;
		FrictionInSlide = MovementData->Friction;

		//Jump
		FirstJumpZVelocity = MovementData->FirstJumpZVelocity;
		CoyoteTime = MovementData->CoyoteTime;
		JumpCurve = MovementData->JumpCurve;
		JumpResetTime = MovementData->JumpResetTime;
		CrouchJumpZVelocity = MovementData->CrouchJumpHeight;

		//Melee
		DashDuration = MovementData->DashDuration;
		DashImpulse = MovementData->DashImpulse;
		DashCoolDown = MovementData->DashCoolDown;
	}

	//SET DEFAULT VALUE TO KEEP ORIGINAL
	DefaultGroundFriction = GroundFriction;
	DefaultBrakingDecelerationWalking = BrakingDecelerationWalking;
	DefaultMaxWalkSpeedCrouched = MaxWalkSpeedCrouched;
	JumpZVelocity = FirstJumpZVelocity;
	DefaultAirControl = AirControl;
	DefaultMaxWalkSpeed = MaxWalkSpeed;
	DefaultMaxAcceleration = MaxAcceleration;

	//TRACE FOR PARKOUR
	TraceParams.bTraceComplex = true;
	TraceParams.AddIgnoredActor(CharacterRef);

	AnimInstance = CharacterRef->GetMesh()->GetAnimInstance();

	if (JumpCurve)
	{
		FOnTimelineEvent TimelineJumpCallbackFinished;
		TimelineJumpCallbackFinished.BindUFunction(this, FName("OnJumpTimelineFinished"));
		JumpTimeline.SetTimelineFinishedFunc(TimelineJumpCallbackFinished);
		JumpTimeline.SetTimelineLength(JumpResetTime);
		JumpTimeline.SetPlayRate(1);
		JumpTimeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
		JumpTimeline.SetLooping(false);
	}
}

void UPlayerMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	JumpTimeline.TickTimeline(DeltaTime);

	if (IsSlowed)
	{
		float Z = Velocity.Z;
		Velocity *= SlowedSpeed;
		Velocity.Z = Z;
	}
	
	if (CharacterRef->HasAuthority())
	{
		bool bValidGrounded =
			IsMovingOnGround() &&
			CurrentFloor.IsWalkableFloor() &&
			CurrentFloor.bBlockingHit &&
			!IsFalling() &&
			FMath::Abs(Velocity.Z) < 1.f &&
			CurrentFloor.FloorDist <= MaxStepHeight &&
			!bJustTeleported &&
			bHasFlagCMC;

		if (bValidGrounded)
		{
			CharacterRef->LastGroundedPosition = CharacterRef->GetActorLocation();
		}
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPlayerMovementComponent::PhysDash(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}
	
	if (!bWantsToDash)
	{
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(DeltaTime,Iterations);
		return;
	}

	if (!bIsDashing)
	{
		bIsDashing = true;
		DashStartTime = GetWorld()->GetTimeSeconds();
		CharacterRef->Dash();
		if (CharacterRef->IsLocallyControlled() && CharacterRef->FirstPersonCameraComponent)
			CharacterRef->FirstPersonCameraComponent->StartCameraShake(CharacterRef->ShakeDash, 1.0f, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
	}

	
	FVector Dash = MoveDirectionMelee * Da	shImpulse;
	Dash.Z = 0;
	Velocity = Dash;
	FVector Delta = Velocity * DeltaTime;
	FHitResult Hit;

	SafeMoveUpdatedComponent(
		Delta,
		UpdatedComponent->GetComponentQuat(),
		true,
		Hit
	);

	GroundFriction = 0.0;
	BrakingDecelerationWalking = 0;
	MaxAcceleration = 0;
}

void UPlayerMovementComponent::Server_GetInputLastDirection_Implementation(const FVector& Direction)
{
	if (!bIsDashing)
	MoveDirectionMelee = Direction;
}

bool UPlayerMovementComponent::Server_GetInputLastDirection_Validate(const FVector& Direction)
{
	return true;
}

void UPlayerMovementComponent::EndOfDash(float DeltaSeconds, int32 Iterations)
{
	bIsDashing = false;
	bWantsToDash = false;
	MaxAcceleration = DefaultMaxAcceleration;
	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
	
	if (IsMovingOnGround())
		SetMovementMode(MOVE_Walking);
	else
	{
		SetMovementMode(MOVE_Falling);
		CharacterRef->JumpCurrentCount++;
	}
	
	StartNewPhysics(DeltaSeconds, Iterations);
}

void UPlayerMovementComponent::DebugSlideNetwork(const FString& Context)
{
	// ==========================
	// ROLE INFOS
	// ==========================

	FString NetModeString = CharacterRef->HasAuthority() ? "SERVER" : "CLIENT";

	FString LocalRoleString = UEnum::GetValueAsString(CharacterRef->GetLocalRole());
	FString RemoteRoleString = UEnum::GetValueAsString(CharacterRef->GetRemoteRole());

	// ==========================
	// MOVEMENT MODE
	// ==========================

	FString MovementModeString = UEnum::GetValueAsString(MovementMode);

	FString CustomModeString = "None";
	if (MovementMode == MOVE_Custom)
	{
		CustomModeString = FString::Printf(TEXT("Custom: %d"), CustomMovementMode);
	}

	// ==========================
	// FLOOR INFO
	// ==========================

	float FloorAngle = 0.f;
	FVector FloorNormal = FVector::ZeroVector;

	if (CurrentFloor.bBlockingHit)
	{
		FloorNormal = CurrentFloor.HitResult.ImpactNormal;
		FloorAngle = FMath::RadiansToDegrees(
			FMath::Acos(FVector::DotProduct(FloorNormal, FVector::UpVector))
		);
	}

	// ==========================
	// SPEED / VELOCITY
	// ==========================

	FVector speedHorizontal = Velocity;
	speedHorizontal.Z = 0.f;
	float Speed = speedHorizontal.Size();

	// ==========================
	// LOCATION
	// ==========================

	FVector Location = GetActorLocation();

	// ==========================
	// BUILD DEBUG TEXT
	// ==========================

	FString DebugText = FString::Printf(TEXT(
		"=============================\n"
		"[%s] Context: %s\n"
		"LocalRole: %s\n"
		"RemoteRole: %s\n\n"

		"---- SLIDE STATE ----\n"
		"CanSlide: %s\n"
		"IsSliding(): %s\n"
		"bIsSliding: %s\n"
		"bWantsToSlide: %s\n"
		"bResetSlideCrouch: %s\n"
		"bResetSlideLanded: %s\n"
		"bCoolDownFinished: %s\n\n"

		"TimeSliding: %.2f\n"
		"SlideImpulse: %.2f\n"
		"SlopeToleranceValue: %.2f\n\n"

		"SlideCooldownRemaining: %.2f\n"
		"SlideCooldownDuration: %.2f\n\n"

		"VelocityAtCrouch: %s\n\n"

		"---- MOVEMENT ----\n"
		"MovementMode: %s\n"
		"CustomMode: %s\n\n"

		"Speed: %.2f\n"
		"Velocity: %s\n"
		"Acceleration: %s\n\n"

		"---- FLOOR ----\n"
		"FloorAngle: %.2f\n"
		"FloorNormal: %s\n\n"

		"Location: %s\n"
		"============================="
	),
	                                    *NetModeString,
	                                    *Context,
	                                    *LocalRoleString,
	                                    *RemoteRoleString,

	                                    CanSlide() ? TEXT("TRUE") : TEXT("FALSE"),
	                                    IsSliding() ? TEXT("TRUE") : TEXT("FALSE"),
	                                    bIsSliding ? TEXT("TRUE") : TEXT("FALSE"),
	                                    bWantsToSlide ? TEXT("TRUE") : TEXT("FALSE"),
	                                    bResetSlideCrouch ? TEXT("TRUE") : TEXT("FALSE"),
	                                    bResetSlideLanded ? TEXT("TRUE") : TEXT("FALSE"),
	                                    bCoolDownFinished ? TEXT("TRUE") : TEXT("FALSE"),

	                                    TimeSliding,
	                                    SlideImpulse,
	                                    SlopeToleranceValue,

	                                    SlideCooldownRemaining,
	                                    SlideCooldownDuration,

	                                    *VelocityAtCrouch.ToCompactString(),

	                                    *MovementModeString,
	                                    *CustomModeString,

	                                    Speed,
	                                    *Velocity.ToCompactString(),
	                                    *Acceleration.ToCompactString(),

	                                    FloorAngle,
	                                    *FloorNormal.ToCompactString(),

	                                    *Location.ToCompactString()
	);

	int32 Key = CharacterRef->HasAuthority() ? 500 : 600;

	GEngine->AddOnScreenDebugMessage(
		Key,
		0.f,
		CharacterRef->HasAuthority() ? FColor::Red : FColor::Green,
		DebugText
	);
}

void UPlayerMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	switch (CustomMovementMode)
	{
	case CMOVE_Dash:
		PhysDash(DeltaTime, Iterations);
		break;
	case CMOVE_Slide:
		PhysSlide(DeltaTime, Iterations);
		break;
	default:
		break;
	}
	Super::PhysCustom(DeltaTime, Iterations);
}

void UPlayerMovementComponent::PhysWalking(float DeltaTime, int32 Iterations)
{
	Super::PhysWalking(DeltaTime, Iterations);
}

#pragma region Slide
void UPlayerMovementComponent::PhysSlide(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!CanSlide())
	{
		ExitSlide(DeltaTime, Iterations);
		return;
	}

	if (!bIsSliding)
	{
		if (SlideLineTrace())
		{
			
				bIsSliding = true;
				bResetSlideCrouch = false;
				TimeSliding = 0.f;
				VelocityAtCrouch = FVector::ZeroVector;

				const FVector SlideDir = Impact.GetSafeNormal2D();
				Velocity += SlideDir * SlideImpulse;
				if (CharacterRef->IsLocallyControlled())
					CharacterRef->FirstPersonCameraComponent->StartCameraShake(CharacterRef->ShakeSlide, 1.0f, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
			
		}
		else
		{
			ExitSlide(DeltaTime, Iterations);
		}
	}

	if (bIsSliding)
	{
		TimeSliding += DeltaTime;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const float timeTick = GetSimulationTimeStep(DeltaTime, Iterations);

		Acceleration = Acceleration.GetSafeNormal() * MaxAccelerationForSlide;
		CalcVelocity(
			timeTick,
			FrictionInSlide,
			false,
			BrakingDecelerationInSlide
		);

		FFindFloorResult FloorResult;
		FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false);
		CurrentFloor = FloorResult;
		
		if (CurrentFloor.IsWalkableFloor())
		{
			const FVector FloorNormal = CurrentFloor.HitResult.ImpactNormal;
			FVector DownSlope = FVector::VectorPlaneProject(FVector::DownVector, FloorNormal).GetSafeNormal();

			const float FloorDot = FVector::DotProduct(FloorNormal, FVector::UpVector);
			const float FloorAngleRad = FMath::Acos(FloorDot);
			const float GravityFactor = FMath::Sin(FloorAngleRad);

			const FVector VelocityDir = Velocity.GetSafeNormal();
			const float SlopeDirectionDot = FVector::DotProduct(VelocityDir, DownSlope);

			float SlideGravityForce = 0.f;

			if (SlopeDirectionDot > 0.f)
			{
				SlideGravityForce = SpeedIncreaseInSlope;
			}
			else
			{
				SlideGravityForce = SpeedDecreaseInSlope;
			}

			Velocity += DownSlope * GravityFactor * SlideGravityForce * timeTick;

			const float MaxSlideSpeed = MaxVelocityForSlide;
			if (Velocity.Size() > MaxSlideSpeed)
			{
				Velocity = Velocity.GetSafeNormal() * MaxSlideSpeed;
			}
		}

		MoveAlongFloor(Velocity, timeTick);

		if (IsMovingOnGround())
		{
			if (!bJustTeleported
				&& !HasAnimRootMotion()
				&& !CurrentRootMotion.HasOverrideVelocity()
				&& timeTick >= MIN_TICK_TIME)
			{
				Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
				MaintainHorizontalGroundVelocity();
			}
		}

		FFindFloorResult NewFloor;
		FVector UpdatedComponentLocation = UpdatedComponent->GetComponentLocation();
		UpdatedComponentLocation.Z -= 20.f;
		FindFloor(UpdatedComponentLocation, NewFloor, false);

		const float EdgeTolerance = 20.f;

		if (!NewFloor.IsWalkableFloor() || NewFloor.FloorDist > EdgeTolerance)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player is sliding on non-walkable floor!"));
			SetMovementMode(MOVE_Falling);
			return;
		}

		CurrentFloor = NewFloor;

		if ((Velocity.Size() < DefaultMaxWalkSpeedCrouched && TimeSliding >= 0.3f) || !bWantsToSlide)
		{
			ExitSlide(DeltaTime, Iterations);
		}
	}
}

bool UPlayerMovementComponent::SlideLineTrace()
{
	FVector UpWorld = FVector(0.f, 0.f, 1.f);
	FVector End = (UpWorld * -200) + CharacterRef->GetActorLocation();
	FVector Start = (UpWorld * 200) + CharacterRef->GetActorLocation();
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CharacterRef);
	bool bSlideHitResult = GetWorld()->LineTraceSingleByChannel(SlideHit, Start, End, ECC_WorldStatic, QueryParams);
	Impact = UKismetMathLibrary::Cross_VectorVector(SlideHit.ImpactNormal, CharacterRef->GetActorRightVector());
	Impact *= -1.0;
	return bSlideHitResult;
}

bool UPlayerMovementComponent::CanSlide()
{
	if (bIsSliding)
		return true;
	SlideLineTrace();
	bool bResult = IsMovingOnGround();
	bResult &= VelocityAtCrouch.Size() >= DefaultMaxWalkSpeed - 10;
	bResult &= bResetSlideCrouch;
	bResult &= bResetSlideLanded;
	return bResult;
}

bool UPlayerMovementComponent::IsSliding() const
{
	return IsCustomMovementModeOn(CMOVE_Slide);
}

void UPlayerMovementComponent::OnJumpTimelineFinished()
{
	JumpCount = 0;
}

void UPlayerMovementComponent::ResetSlideValues()
{
	SlideCooldownRemaining = SlideCooldownDuration;
	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
	TimeSliding = 0.0;
	bIsSliding = false;
}

void UPlayerMovementComponent::ExitSlide(float DeltaTime, int32 Iterations)
{
	ResetSlideValues();
	SetMovementMode(MOVE_Walking);
	StartNewPhysics(DeltaTime, Iterations);
}
#pragma endregion

bool UPlayerMovementComponent::IsCustomMovementModeOn(uint8 customMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == customMovementMode;
}

float UPlayerMovementComponent::GetMaxSpeed() const
{
	float BaseSpeed = DefaultMaxWalkSpeed;

	FVector VelocityDir = Velocity.GetSafeNormal2D();
	if (VelocityDir.IsNearlyZero())
		return BaseSpeed;

	FVector Forward = CharacterRef->GetActorForwardVector();
	float ForwardDot = FVector::DotProduct(Forward, VelocityDir);

	float SuperMaxSpeed = Super::GetMaxSpeed();

	float ShootingSpeedBase = 1.0f;
	if (bIsShooting)
		ShootingSpeedBase = ShootingSpeed;

	if (ForwardDot > 0.5f)
	{
		return SuperMaxSpeed * ShootingSpeedBase;
	}
	if (ForwardDot < -0.5f)
	{
		return SuperMaxSpeed * BackwardSpeed;
	}
	
	return SuperMaxSpeed * SideSpeed;
}

bool UPlayerMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() ||
	((IsCustomMovementModeOn(CMOVE_Slide)) && UpdatedComponent && !
		IsFalling());
}

void UPlayerMovementComponent::PhysFalling(float DeltaTime, int32 Iterations)
{
	Super::PhysFalling(DeltaTime, Iterations);
}

bool UPlayerMovementComponent::CanAttemptJump() const
{
	return IsJumpAllowed() &&
		(IsMovingOnGround() || IsFalling());
}

bool UPlayerMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	if (CharacterOwner->CanJump())
	{
		float JumpHeight = JumpCurve->GetFloatValue(JumpCount);
		JumpCount++;

		JumpZVelocity = JumpHeight;

		if (JumpTimeline.IsPlaying())
			JumpTimeline.Stop();

		if (!bIsSliding && IsCrouching())
		{
			JumpZVelocity = CrouchJumpZVelocity;
		}
	}

	if (Super::DoJump(bReplayingMoves, DeltaTime))
	{
		if (CharacterRef->IsLocallyControlled())
			CharacterRef->FirstPersonCameraComponent->StartCameraShake(CharacterRef->ShakeJump, 1.0f, ECameraShakePlaySpace::CameraLocal,
														 FRotator::ZeroRotator);
		return true;
	}
	return false;
}

void UPlayerMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
                                                 const FVector& OldVelocity)
{
	if (PawnOwner->IsLocallyControlled())
	{
		MoveDirectionMelee = PawnOwner->GetLastMovementInputVector().GetSafeNormal();
		if (MoveDirectionMelee == FVector::ZeroVector)
			MoveDirectionMelee = CharacterRef->GetActorForwardVector();
	}
	//Send movement vector to server
	if (PawnOwner->IsLocallyControlled())
	{
		Server_GetInputLastDirection(MoveDirectionMelee);
	}

	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}

void UPlayerMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	if (IsMovingOnGround())
		bWantsToSlide = bWantsToCrouch;

	if (SlideCooldownRemaining > 0.f && !bWantsToSlide)
	{
		SlideCooldownRemaining -= DeltaSeconds;
	}
	else if (SlideCooldownRemaining > 0.f && bWantsToSlide)
	{
		SlideCooldownRemaining = SlideCooldownDuration;
	}

	if (SlideCooldownRemaining <= 0.f)
	{
		bCoolDownFinished = true;
	}
	
	DashDurationCheck(DeltaSeconds, 0);
	
	if (bWantsToSlide && IsMovingOnGround() && bCoolDownFinished)
	{
		bCoolDownFinished = false;
		SetMovementMode(MOVE_Custom, CMOVE_Slide);
	}
	else if (!bWantsToSlide && bIsSliding)
	{
		ExitSlide(DeltaSeconds, 0);
	}

	if (bWantsToDash && CanDash())
	{
		LastDashTime = GetWorld()->GetTimeSeconds();
		SetMovementMode(MOVE_Custom, CMOVE_Dash);
	}
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

bool UPlayerMovementComponent::CanDash() const
{
	return GetWorld()->GetTimeSeconds() - LastDashTime >= DashCoolDown;
}

void UPlayerMovementComponent::DebugDashValues()
{
	if (!GEngine) return;

	FString NetPrefix = GetOwner() && GetOwner()->HasAuthority() ? "[SERVER]" : "[CLIENT]";

	FString DebugText = FString::Printf(
		TEXT("%s\n"
		"DashImpulse: %f\n"
		"DashCoolDown: %f\n"
		"DashDuration: %f\n"
		"bIsDashing: %s\n"
		"bWantsToDash: %s\n"
		"MoveDirectionMelee: X=%.2f Y=%.2f Z=%.2f"),
		
		*NetPrefix,
		DashImpulse,
		DashCoolDown,
		DashDuration,
		bIsDashing ? TEXT("TRUE") : TEXT("FALSE"),
		bWantsToDash ? TEXT("TRUE") : TEXT("FALSE"),
		MoveDirectionMelee.X,
		MoveDirectionMelee.Y,
		MoveDirectionMelee.Z
	);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *DebugText);

	GEngine->AddOnScreenDebugMessage(
		42,
		0.f,
		FColor::Yellow,
		DebugText
	);
}

void UPlayerMovementComponent::DashDurationCheck(float DeltaSeconds, int32 Iterations)
{
	if (!bIsDashing)
		return;

	if (GetWorld()->GetTimeSeconds() - DashStartTime >= DashDuration)
	{
		EndOfDash(DeltaSeconds, Iterations);
	}
}

void UPlayerMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode,
                                                     uint8 PreviousCustomMode)
{
	bool bSuppressSuperNotification = false;
	if (PreviousMovementMode == MovementMode && PreviousCustomMode == CustomMovementMode)
	{
		Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	}

	if (IsCustomMovementModeOn(ECustomMovementMode::CMOVE_Slide))
	{
		bSuppressSuperNotification = true;
	}

	if (!bSuppressSuperNotification)
	{
		Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	}
	else
	{
		CharacterRef->OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	}
}

void UPlayerMovementComponent::UpdateFromCompressedFlags(uint8 Flags) //Client only
{
	Super::UpdateFromCompressedFlags(Flags);

	bWantsToDash = (Flags & FLAG_WANT_TO_DASH) != 0;
	bWantsToSlide = (Flags & FLAG_WANT_TO_SLIDE) != 0;
}

void UPlayerMovementComponent::Crouch(bool bClientSimulation)
{
	if (IsMovingOnGround() || !bIsSliding)
		bResetSlideCrouch = true;
	
	bCrouchMaintainsBaseLocation = true;
	Super::Crouch(bClientSimulation);	
}

void UPlayerMovementComponent::UnCrouch(bool bClientSimulation)
{
	bCrouchMaintainsBaseLocation = true;	
	Super::UnCrouch(bClientSimulation);
}

void UPlayerMovementComponent::DebugPrintClientIds()
{
	APlayerController* PC = Cast<APlayerController>(CharacterRef->GetController());
	FString WhoExecutes;

	if (GetOwner()->HasAuthority())
	{
		// Serveur
		if (PC && PC->IsLocalController())
		{
			WhoExecutes = TEXT("SERVER + Local Controller");
		}
		else
		{
			WhoExecutes = TEXT("SERVER (remote pawn)");
		}
	}
	else
	{
		// Client
		if (PC && PC->IsLocalController())
		{
			int32 ClientId = PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetControllerId() : -1;
			WhoExecutes = FString::Printf(TEXT("CLIENT %d (Local)"), ClientId);
		}
		else
		{
			// Autres clients
			int32 ClientId = PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetControllerId() : -1;
			WhoExecutes = FString::Printf(TEXT("CLIENT %d (Remote)"), ClientId);
		}
	}

	// Affichage écran
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, WhoExecutes);
	}

	// Log
	UE_LOG(LogTemp, Warning, TEXT("%s executing function"), *WhoExecutes);
}

void UPlayerMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPlayerMovementComponent, VelocityAtCrouch);
}

#pragma region SavedMoves

class FNetworkPredictionData_Client* UPlayerMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UPlayerMovementComponent* MutableThis = const_cast<UPlayerMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_MyMovement(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}

void FSavedMove_MyMovement::Clear()
{
	Super::Clear();

	SavedMoveDirection = FVector::ZeroVector;
	bWantsToDash = false;
	bWantsToSlide = false;
}

uint8 FSavedMove_MyMovement::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (bWantsToDash)
		Result |= FLAG_WANT_TO_DASH;

	if (bWantsToSlide)
		Result |= FLAG_WANT_TO_SLIDE;

	return Result;
}

bool FSavedMove_MyMovement::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	if (SavedMoveDirection != ((FSavedMove_MyMovement*)&NewMove)->SavedMoveDirection)
		return false;

	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FSavedMove_MyMovement::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel,
                                       class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UPlayerMovementComponent* CharMov = Cast<UPlayerMovementComponent>(Character->GetCharacterMovement());
	if (CharMov)
	{
		SavedMoveDirection = CharMov->MoveDirectionMelee;
		bWantsToDash = CharMov->bWantsToDash;
		bWantsToSlide = CharMov->bWantsToSlide;
	}
}

void FSavedMove_MyMovement::PrepMoveFor(class ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UPlayerMovementComponent* CharMov = Cast<UPlayerMovementComponent>(Character->GetCharacterMovement());
	if (CharMov)
	{
		CharMov->MoveDirectionMelee = SavedMoveDirection;
		CharMov->bWantsToDash = bWantsToDash;
		CharMov->bWantsToSlide = bWantsToSlide;
	}
}

FNetworkPredictionData_Client_MyMovement::FNetworkPredictionData_Client_MyMovement(
	const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_MyMovement::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_MyMovement());
}
#pragma endregion
