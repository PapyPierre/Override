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
		SlidingCoolDown = MovementData->SlidingCoolDown;
		BoostSlidingTime = MovementData->BoostSlidingTime;
		EaseOutTime = MovementData->EaseOutTime;
		SlideImpulse = MovementData->SlideImpulse;
		SlopeToleranceValue = MovementData->SlopeToleranceValue;
		MinDiffVelocityToAllowSlide = MovementData->MinDiffVelocityToAllowSlide;
		MaxVelocityForSlide = MovementData->MaxVelocityForSlide;
		
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

	//TRACE FOR PARKOUR
	TraceParams.bTraceComplex = true;
	TraceParams.AddIgnoredActor(CharacterRef);

	AnimInstance = CharacterRef->GetMesh()->GetAnimInstance();

	//CURVE FOR SLIDE
	if (VelocityEaseCurve)
	{
		FOnTimelineFloat TimelineCallback;
		FOnTimelineEvent TimelineCallbackFinished;
		TimelineCallback.BindUFunction(this, FName("EaseVelocityUpdate"));
		TimelineCallbackFinished.BindUFunction(this, FName("StopVelocityEaseTimeline"));
		VelocityEaseTimeline.SetTimelineFinishedFunc(TimelineCallbackFinished);
		VelocityEaseTimeline.SetTimelineLength(EaseOutTime);
		VelocityEaseTimeline.AddInterpFloat(VelocityEaseCurve, TimelineCallback);
		VelocityEaseTimeline.SetLooping(false);

		FOnTimelineFloat TimelineMeleeCallback;
		FOnTimelineEvent TimelineMeleeCallbackFinished;
		TimelineMeleeCallbackFinished.BindUFunction(this, FName("StopDashVelocityEaseTimeline"));
		DashTimeline.SetTimelineFinishedFunc(TimelineMeleeCallbackFinished);
		DashTimeline.SetTimelineLength(EaseOutTimeDash);
		DashTimeline.SetPlayRate(1);
		DashTimeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
		DashTimeline.SetLooping(false);
	}

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
	if (IsMovingOnGround() && !(VelocityEaseTimeline.IsReversing() && VelocityEaseTimeline.GetPlaybackPosition() < 0.1))
		VelocityEaseTimeline.TickTimeline(DeltaTime);
	
	DashTimeline.TickTimeline(DeltaTime);
	JumpTimeline.TickTimeline(DeltaTime);

	
	FrameCounter++;
	
	if (FrameCounter > 1000000)
	{
		FrameCounter = 0;
	}

#pragma region Slide Verification
	if (bIsSliding)
		DebugSlideState(TEXT("Sliding"));
	
	if (bIsSliding && CharacterRef->HasAuthority())
	{		
		SlideLineTrace();
		
		if (FMath::IsNearlyZero(Impact.Z) || (Impact.Z >= SlopeToleranceValue && !VelocityEaseTimeline.IsPlaying()))
		{
			if (IsMovingOnGround())
				TimeSliding += DeltaTime;
		}

		if (Impact.Z <= SlopeToleranceValue *- 1 && VelocityEaseTimeline.IsPlaying())
		{
			bPendingCancelSlide = false;
			bShouldStopSliding = false;
			VelocityEaseTimeline.Reverse();
		}

		if (Impact.Z >= SlopeToleranceValue && VelocityEaseTimeline.IsPlaying())
		{
			VelocityEaseTimeline.SetPlayRate(2);
		}
		else if (VelocityEaseTimeline.IsPlaying())
		{
			VelocityEaseTimeline.SetPlayRate(1);
		}

		if (bStopSliding)
		{
			StopVelocityEaseTimeline();
		}
		
		else if (bShouldStopSliding && !bPendingCancelSlide)
		{
			VelocityEaseTimeline.SetPlayRate(1);
			StartVelocityEase(Impact.GetSafeNormal() * DefaultMaxWalkSpeedCrouched);
		}
	}
	
	if (TimeToWaitBetweenSlide >= 0 && !IsCrouching())
	{
		TimeToWaitBetweenSlide -= DeltaTime;
		bCoolDownFinished = false;
	}
	else if (TimeToWaitBetweenSlide > 0 && !bCoolDownFinished)
	{
		TimeToWaitBetweenSlide = SlidingCoolDown;
		bCoolDownFinished = true;
	}
	
#pragma endregion

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

void UPlayerMovementComponent::DebugSlideState(const FString& Context)
{
	if (!CharacterOwner || !GetWorld())
		return;

	// =========================
	// NET ROLE
	// =========================
	const bool bIsAuthority = CharacterOwner->HasAuthority();
	const bool bIsLocal = CharacterOwner->IsLocallyControlled();

	FString NetRole;

	if (bIsAuthority)
	{
		NetRole = bIsLocal ? TEXT("LISTEN_SERVER") : TEXT("SERVER");
	}
	else
	{
		NetRole = TEXT("CLIENT");
	}

	// =========================
	// COLOR + SCREEN ID
	// =========================
	const int32 ScreenID = bIsAuthority ? 700 : 701;
	const FColor ScreenColor = bIsAuthority ? FColor::Red : FColor::Green;

	// =========================
	// TIMECODE
	// =========================
	const float WorldTime = GetWorld()->GetTimeSeconds();

	// =========================
	// MOVEMENT MODE
	// =========================
	const FString MovementModeString = UEnum::GetValueAsString(MovementMode);

	// =========================
	// VELOCITY
	// =========================
	const float CurrentSpeed = Velocity.Size();
	const float CrouchSpeed = VelocityAtCrouch.Size();

	// =========================
	// TIMELINE
	// =========================
	const bool bTimelinePlaying = VelocityEaseTimeline.IsPlaying();

	// =========================
	// DEBUG STRING
	// =========================
	FString DebugString = FString::Printf(
		TEXT("======== SLIDE DEBUG ========\n")
		TEXT("[%s | %.3f] %s\n")
		TEXT("Mode=%s\n")
		TEXT("Speed=%.1f | CrouchSpeed=%.1f\n")
		TEXT("bIsSliding=%d | bStopSliding=%d | bShouldStopSliding=%d\n")
		TEXT("bPendingCancel=%d | bResetCrouch=%d | bResetLanded=%d\n")
		TEXT("CooldownFinished=%d | TimeSliding=%.2f\n")
		TEXT("Impulse=%.1f | MaxVel=%.1f | MinDiff=%.1f\n")
		TEXT("TimelinePlaying=%d\n")
		TEXT("=============================="),
		*NetRole,
		WorldTime,
		*Context,
		*MovementModeString,
		CurrentSpeed,
		CrouchSpeed,
		bIsSliding,
		bStopSliding,
		bShouldStopSliding,
		bPendingCancelSlide,
		bResetSlideCrouch,
		bResetSlideLanded,
		bCoolDownFinished,
		TimeSliding,
		SlideImpulse,
		MaxVelocityForSlide,
		MinDiffVelocityToAllowSlide,
		bTimelinePlaying
	);

	// =========================
	// UE_LOG
	// =========================
	UE_LOG(LogTemp, Warning, TEXT("%s"), *DebugString);

	// =========================
	// ON SCREEN (LONG DISPLAY)
	// =========================
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			ScreenID,      // 700 = Server | 701 = Client
			800.f,           // reste 8 secondes
			ScreenColor,   // Rouge serveur | Vert client
			DebugString
		);
	}
}

void UPlayerMovementComponent::PhysMelee(float DeltaTime, int32 Iterations)
{
	FVector Dash = MoveDirectionMelee * DashImpulse;
	Dash.Z = 0;
	Launch(Dash);
	GroundFriction = 0.0;
	BrakingDecelerationWalking = 1400;
	bIsMelee = true;
	bWantsToDash = false;
	
	CharacterRef->GetWorldTimerManager().SetTimer(
		SimpleDelayHandle,
		this,
		&UPlayerMovementComponent::OnDelayFinished,
		0.3f,
		false
	);
}

void UPlayerMovementComponent::Server_GetInputLastDirection_Implementation(const FVector& Direction)
{
	MoveDirectionMelee = Direction;
}

bool UPlayerMovementComponent::Server_GetInputLastDirection_Validate(const FVector& Direction)
{
	return true;
}

void UPlayerMovementComponent::OnDelayFinished()
{
	bIsMelee = false;
	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
}

void UPlayerMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	switch (CustomMovementMode)
	{
	case CMOVE_Melee:
		PhysMelee(DeltaTime, Iterations);
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
	if (!IsCustomMovementModeOn(CMOVE_Slide))
	{
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}

	if (!CanSlide() && !bPendingCancelSlide && !bIsSliding)
	{
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}
	
	if (!bIsSliding)
	{
		if (SlideLineTrace()) {
			if (Impact.Z <= SlopeToleranceValue) {
				GroundFriction = 0.0;
				BrakingDecelerationWalking = 1400;
				Impact *= SlideImpulse;
				if (Velocity.Size() < MaxVelocityForSlide)
				{
					AddImpulse(Impact, true);
				}
				bIsSliding = true;
				VelocityAtCrouch = FVector::ZeroVector;
				bResetSlideCrouch = false;
			}
		}
	}
	
	PhysWalking(DeltaTime, Iterations);
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

void UPlayerMovementComponent::EaseVelocityUpdate(float Value)
{
	Velocity = FMath::Lerp(InitialEaseVelocity, TargetEaseVelocity, Value);
}

void UPlayerMovementComponent::StopVelocityEaseTimeline()
{
	Client_StopVelocityEaseTimeline();
	UE_LOG(LogTemp, Warning, TEXT("StopVelocityEaseTimeline"));
	bIsSliding = false;
	bPendingCancelSlide = false;
	TimeSliding = 0;
	ResetSlideValues();
	if (VelocityEaseTimeline.IsPlaying())
		VelocityEaseTimeline.Stop();
}

void UPlayerMovementComponent::Client_StopVelocityEaseTimeline_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("StopVelocityEaseTimeline"));
	bIsSliding = false;
	bPendingCancelSlide = false;
	TimeSliding = 0;
	ResetSlideValues();
	if (VelocityEaseTimeline.IsPlaying())
		VelocityEaseTimeline.Stop();
}

void UPlayerMovementComponent::StartVelocityEase(const FVector& NewTargetVelocity)
{
	InitialEaseVelocity = Velocity;
	TargetEaseVelocity = NewTargetVelocity;
    
	if (VelocityEaseCurve)
	{
		VelocityEaseTimeline.PlayFromStart();
		bPendingCancelSlide = true;
	}
}

bool UPlayerMovementComponent::CanSlide()
{
	SlideLineTrace();
	bool bResult = IsMovingOnGround() && TimeToWaitBetweenSlide <= 0;
	bResult &= VelocityAtCrouch.Size() > 200;
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
	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
	MaxWalkSpeedCrouched = DefaultMaxWalkSpeedCrouched;
	TimeToWaitBetweenSlide = SlidingCoolDown;
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
	if (VelocityEaseTimeline.IsPlaying())
		VelocityEaseTimeline.SetPlayRate(0);

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
	
	if (bWantsToSlide)
	{
		SetMovementMode(MOVE_Custom, CMOVE_Slide);
	}
	
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}

void UPlayerMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	// HARD STOP, conditions to immediately stop the slide 
	bStopSliding =
		(!bWantsToCrouch && IsMovingOnGround()) ||
		(Velocity.IsNearlyZero(DefaultMaxWalkSpeedCrouched * 2) && !IsMovingOnGround()) ||
			Velocity.Size() < DefaultMaxWalkSpeedCrouched * 0.75;       
		
	// SOFT STOP, conditions to start the easing
	bShouldStopSliding =
		TimeSliding >= BoostSlidingTime ||
		(TimeSliding > BoostSlidingTime / 2 && Velocity.Length() < MaxWalkSpeed * 0.75f);
	
	bWantsToSlide = CanSlide() && bWantsToCrouch;
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
	
	bShouldStopSliding = (Flags & FLAG_SHOULD_STOP_SLIDE) != 0;
	bStopSliding  = (Flags & FLAG_STOP_SLIDE) != 0;
	bWantsToDash = (Flags & FLAG_WANT_TO_DASH) != 0;
	bWantsToSlide  = (Flags & FLAG_WANT_TO_SLIDE) != 0;
}

void UPlayerMovementComponent::Crouch(bool bClientSimulation)
{
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
	DOREPLIFETIME(UPlayerMovementComponent, bResetSlideCrouch);
	DOREPLIFETIME(UPlayerMovementComponent, bResetSlideLanded);
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
	bShouldStopSliding = false;
	bStopSliding = false;
	bWantsToDash = false;
	bWantsToSlide = false;
}

uint8 FSavedMove_MyMovement::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (bStopSliding)
		Result |= FLAG_STOP_SLIDE;
	
	if (bShouldStopSliding)
		Result |= FLAG_SHOULD_STOP_SLIDE;
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
		bShouldStopSliding = CharMov->bShouldStopSliding;
		bStopSliding  = CharMov->bStopSliding;
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
		CharMov->bShouldStopSliding = bShouldStopSliding;
		CharMov->bStopSliding = bStopSliding;
		CharMov->bWantsToDash = bWantsToDash;
		CharMov->bIsSliding = bWantsToSlide;
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