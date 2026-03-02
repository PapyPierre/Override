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

		//Melee
		EaseOutTimeDash = MovementData->EaseOutTimeMelee;
		DashImpulse = MovementData->MeleeImpulse;
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

	FOnTimelineFloat TimelineMeleeCallback;
	FOnTimelineEvent TimelineMeleeCallbackFinished;
	TimelineMeleeCallbackFinished.BindUFunction(this, FName("StopDashVelocityEaseTimeline"));
	DashTimeline.SetTimelineFinishedFunc(TimelineMeleeCallbackFinished);
	DashTimeline.SetTimelineLength(EaseOutTimeDash);
	DashTimeline.SetPlayRate(1);
	DashTimeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
	DashTimeline.SetLooping(false);

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
	DashTimeline.TickTimeline(DeltaTime);
	JumpTimeline.TickTimeline(DeltaTime);

#pragma region DEBUG
	/*
	/////////GROSSE ZONE DE DEBUG
	if (!GEngine || !CharacterOwner)
	{
		return;
	}

	// ===== Détection du rôle réseau =====
	ENetRole LocalRole = CharacterOwner->GetLocalRole();

	FString NetPrefix;
	FColor NetColor;
	int32 BaseID = 1000;

	switch (LocalRole)
	{
	case ROLE_Authority:
		NetPrefix = TEXT("[SERVER]");
		NetColor = FColor::Red;
		BaseID = 1000;
		break;

	case ROLE_AutonomousProxy:
		NetPrefix = TEXT("[CLIENT-AUTO]");
		NetColor = FColor::Green;
		BaseID = 2000;
		break;

	case ROLE_SimulatedProxy:
		NetPrefix = TEXT("[CLIENT-SIM]");
		NetColor = FColor::Cyan;
		BaseID = 3000;
		break;

	default:
		NetPrefix = TEXT("[UNKNOWN]");
		NetColor = FColor::White;
		BaseID = 4000;
		break;
	}

	// ===== Zone DEBUG SLIDE =====
	GEngine->AddOnScreenDebugMessage(BaseID + 1, 0.f, NetColor,
		FString::Printf(TEXT("%s isSliding: %s"), *NetPrefix, bIsSliding ? TEXT("true") : TEXT("false")));

	GEngine->AddOnScreenDebugMessage(BaseID + 2, 0.f, NetColor,
		FString::Printf(TEXT("%s PendingCancelSlide: %s"), *NetPrefix, bPendingCancelSlide ? TEXT("true") : TEXT("false")));

	GEngine->AddOnScreenDebugMessage(BaseID + 3, 0.f, NetColor,
		FString::Printf(TEXT("%s TimeSliding: %.3f"), *NetPrefix, TimeSliding));

	GEngine->AddOnScreenDebugMessage(BaseID + 4, 0.f, NetColor,
		FString::Printf(TEXT("%s Speed: %.2f"), *NetPrefix, Velocity.Size()));


	// ===== Zone DEBUG MOVEMENT =====
	if (UCharacterMovementComponent* MoveComp = CharacterOwner->GetCharacterMovement())
	{
		EMovementMode MoveMode = MoveComp->MovementMode;
		uint8 CustomMode = MoveComp->CustomMovementMode;

		FString ModeStr;

		if (MoveMode == MOVE_Custom)
		{
			switch (CustomMode)
			{
			case CMOVE_Slide:
				ModeStr = TEXT("Slide");
				break;
			default:
				ModeStr = FString::Printf(TEXT("Custom_%d"), CustomMode);
				break;
			}
		}
		else
		{
			ModeStr = UCharacterMovementComponent::GetMovementName();
		}

		GEngine->AddOnScreenDebugMessage(BaseID + 10, 0.f, FColor::Yellow,
			FString::Printf(TEXT("%s MovementMode: %s"), *NetPrefix, *ModeStr));

		GEngine->AddOnScreenDebugMessage(BaseID + 11, 0.f, FColor::Orange,
			FString::Printf(TEXT("%s MaxWalkSpeed: %.1f"), *NetPrefix, MoveComp->MaxWalkSpeed));
	}		
	/////////FIN DE LA GRANDE ZONE DE DEBUG
	*/
#pragma endregion
		
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPlayerMovementComponent::PhysDash(float DeltaTime, int32 Iterations)
{
	FVector Dash = MoveDirectionMelee * DashImpulse;
	Dash.Z = 0;
	Launch(Dash);
	GroundFriction = 0.0;
	BrakingDecelerationWalking = 0;
	bIsDashing = true;
	bWantsToDash = false;
	DashTimeline.PlayFromStart();
	MaxAcceleration = 0;
	DashCooldownRemaining = ResetDashCooldown;
}

void UPlayerMovementComponent::Server_GetInputLastDirection_Implementation(const FVector& Direction)
{
	MoveDirectionMelee = Direction;
}

bool UPlayerMovementComponent::Server_GetInputLastDirection_Validate(const FVector& Direction)
{
	return true;
}

void UPlayerMovementComponent::OnDelayFinishedDash()
{
	bIsDashing = false;
	MaxAcceleration = DefaultMaxAcceleration;
	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
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

        // Si tu as un enum custom :
        // CustomModeString = UEnum::GetValueAsString((ECustomMovementMode)MoveComp->CustomMovementMode);
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
    float Speed = Velocity.Size();
	
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

        "CanSlide: %s\n"
        "bIsSliding: %s\n"
        "bWantsToSlide: %s\n"

        "MovementMode: %s\n"
        "CustomMode: %s\n\n"

        "Speed: %.2f\n"
        "Velocity: %s\n"
        "Acceleration: %s\n\n"

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
        bIsSliding ? TEXT("TRUE") : TEXT("FALSE"),
        bWantsToSlide ? TEXT("TRUE") : TEXT("FALSE"),

        *MovementModeString,
        *CustomModeString,

        Speed,
        *Velocity.ToCompactString(),
        *Acceleration.ToCompactString(),

        FloorAngle,
        *FloorNormal.ToCompactString(),

        *Location.ToCompactString()
    );

    // ==========================
    // DISPLAY (Keys séparées)
    // ==========================

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
	case CMOVE_Melee:
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
			if (Impact.Z <= SlopeToleranceValue) {
				bIsSliding = true;
				bResetSlideCrouch = false;
				TimeSliding = 0.f;
				VelocityAtCrouch = FVector::ZeroVector;

				const FVector SlideDir = Impact.GetSafeNormal2D();
				Velocity += SlideDir * SlideImpulse;
			}
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
		FindFloor(UpdatedComponent->GetComponentLocation(), NewFloor, false);

		const float EdgeTolerance = 5.f;

		if (!NewFloor.IsWalkableFloor() || NewFloor.FloorDist > EdgeTolerance)
		{
			SetMovementMode(MOVE_Falling);
			return;
		}

		CurrentFloor = NewFloor;

		if ((Velocity.Size() < DefaultMaxWalkSpeedCrouched && TimeSliding >= 0.3f) || !bWantsToSlide || JumpCount > 4)
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
	bResult &= Impact.Z <= SlopeToleranceValue;
	bResult &= bResetSlideCrouch;
	bResult &= bResetSlideLanded;
	return bResult;
}

bool UPlayerMovementComponent::IsSliding() const
{
	return bIsSliding;
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
	
	if (ForwardDot > 0.5f)
	{
		return Super::GetMaxSpeed();
	}
	else if (ForwardDot < -0.5f)
	{
		return Super::GetMaxSpeed() * BackwardSpeed;
	}
	else
	{
		return Super::GetMaxSpeed() * SideSpeed;
	}
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

		if (!IsSliding() && IsCrouching())
		{
			JumpZVelocity = 300;
		}
	}
	
	return Super::DoJump(bReplayingMoves, DeltaTime);
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
	
	if (bWantsToDash)
	{
		SetMovementMode(MOVE_Custom, CMOVE_Melee);
	}
	
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}

void UPlayerMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
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

	DashCooldown(DeltaSeconds);
	
	if (bWantsToSlide && IsMovingOnGround() && bCoolDownFinished)
	{
		bCoolDownFinished = false;
		SetMovementMode(MOVE_Custom, CMOVE_Slide);
	}
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UPlayerMovementComponent::DashCooldown(float DeltaSeconds)
{
	if (DashCooldownRemaining > 0.f && bIsDashing)
	{
		DashCooldownRemaining -= DeltaSeconds;
	}
	else if (bIsDashing)
	{
		DashCooldownRemaining = ResetDashCooldown;
		OnDelayFinishedDash();
	}
}

void UPlayerMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode,
                                                     uint8 PreviousCustomMode){
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

void UPlayerMovementComponent::UpdateFromCompressedFlags(uint8 Flags)//Client only
{
	Super::UpdateFromCompressedFlags(Flags);
	
	bWantsToDash = (Flags & FLAG_WANT_TO_DASH) != 0;
	bWantsToSlide  = (Flags & FLAG_WANT_TO_SLIDE) != 0;
}

void UPlayerMovementComponent::Crouch(bool bClientSimulation)
{
	if (IsMovingOnGround() || !bIsSliding)
		bResetSlideCrouch	= true;
	Super::Crouch(bClientSimulation);
}

void UPlayerMovementComponent::UnCrouch(bool bClientSimulation)
{
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

void FSavedMove_MyMovement::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData)
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

FNetworkPredictionData_Client_MyMovement::FNetworkPredictionData_Client_MyMovement(const UCharacterMovementComponent& ClientMovement)
: Super(ClientMovement)
{

}

FSavedMovePtr FNetworkPredictionData_Client_MyMovement::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_MyMovement());
}
#pragma endregion