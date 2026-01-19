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
		EaseOutTimeMelee = MovementData->EaseOutTimeMelee;
		MeleeImpulse = MovementData->MeleeImpulse;
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
		TimelineMeleeCallback.BindUFunction(this, FName("MeleeVelocityUpdate"));
		TimelineMeleeCallbackFinished.BindUFunction(this, FName("StopMeleeVelocityEaseTimeline"));
		DashMeleeTimeline.SetTimelineFinishedFunc(TimelineMeleeCallbackFinished);
		DashMeleeTimeline.SetTimelineLength(EaseOutTimeMelee);
		DashMeleeTimeline.SetPlayRate(1);
		DashMeleeTimeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
		DashMeleeTimeline.AddInterpFloat(VelocityEaseCurve, TimelineMeleeCallback);
		DashMeleeTimeline.SetLooping(false);
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
	
	DashMeleeTimeline.TickTimeline(DeltaTime);
	JumpTimeline.TickTimeline(DeltaTime);

	FrameCounter++;
	
	if (FrameCounter > 1000000)
	{
		FrameCounter = 0;
	}

#pragma region Slide Verification
		if (bIsSliding)
		{
			if (FrameCounter % 3 == 0)
			{
				SlideLineTrace();
			}
		
			// HARD STOP, conditions to immediately stop the slide 
			bool bStopSliding =
				(!bWantsToCrouch && IsMovingOnGround()) ||
				(Velocity.IsNearlyZero(DefaultMaxWalkSpeedCrouched * 2) && !IsMovingOnGround()) ||
					Velocity.Size() < DefaultMaxWalkSpeedCrouched * 0.75;       
		
			// SOFT STOP, conditions to start the easing
			bool bShouldStopSliding =
				TimeSliding >= BoostSlidingTime ||
				(TimeSliding > BoostSlidingTime / 2 && Velocity.Length() < MaxWalkSpeed * 0.75f);
		
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

			else if (Impact.Z >= SlopeToleranceValue && VelocityEaseTimeline.IsPlaying())
			{
				VelocityEaseTimeline.SetPlayRate(2);
			}
			else if (VelocityEaseTimeline.IsPlaying())
			{
				VelocityEaseTimeline.SetPlayRate(1);
			}
			
			if (bShouldStopSliding && !bPendingCancelSlide)
			{
				VelocityEaseTimeline.SetPlayRate(1);
				StartVelocityEase(Velocity.GetSafeNormal() * DefaultMaxWalkSpeedCrouched);
			}
		
			if (bStopSliding)
			{
				StopVelocityEaseTimeline();
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
		/////////GROSSE ZONE DE DEBUG
	
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1132, 5.f, FColor::Green, FString::Printf(TEXT("is Sliding ?: %s"), bIsSliding ? TEXT("true") : TEXT("false")));
			GEngine->AddOnScreenDebugMessage(113642, 5.f, FColor::Green, FString::Printf(TEXT("is PendingSlide ?: %s"), bPendingCancelSlide ? TEXT("true") : TEXT("false")));
			GEngine->AddOnScreenDebugMessage(6541, 5.0, FColor::Blue, "TimeSliding: " + FString::SanitizeFloat(TimeSliding));
			GEngine->AddOnScreenDebugMessage(1, 5.0, FColor::Red, "Speed: " + FString::SanitizeFloat(Velocity.Size()));
		}
		if (CharacterRef)
		{
			if (UCharacterMovementComponent* const MoveComp = CharacterRef->GetCharacterMovement())
			{
				// Récupérer le mode de mouvement
				EMovementMode MoveMode = CharacterOwner->GetCharacterMovement()->MovementMode;
				uint8 CustomMode = CharacterOwner->GetCharacterMovement()->CustomMovementMode;

				// Créer une chaîne lisible
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
					// Modes natifs
					ModeStr = UCharacterMovementComponent::GetMovementName();
				}

				// Affichage debug
				GEngine->AddOnScreenDebugMessage(9001, 5.0f, FColor::Yellow, FString::Printf(TEXT("MovementMode: %s"), *ModeStr));
				GEngine->AddOnScreenDebugMessage(9002, 5.0f, FColor::Cyan, FString::Printf(TEXT("MaxWalkSpeed: %.1f"), MoveComp->MaxWalkSpeed));
			}
		}
	/////////FIN DE LA GRANDE ZONE DE DEBUG
#pragma endregion
		
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPlayerMovementComponent::PhysMelee(float DeltaTime, int32 Iterations)
{
	if (CharacterRef->IsLocallyControlled())
	{
		DirectionMelee = CharacterRef->GetLastMovementInputVector() * MeleeImpulse;
		if (DirectionMelee == FVector::ZeroVector)
			DirectionMelee = CharacterRef->GetActorForwardVector() * MeleeImpulse;
		Server_GetForwardCamera(DirectionMelee);
	}

	GroundFriction = 0.0;
	BrakingDecelerationWalking = 1400;
	AddImpulse(DirectionMelee, true);
	GravityScale = 0.0;
	DashMeleeTimeline.PlayFromStart();
	SetMovementMode(MOVE_Walking);
	bWantsToDash = false;
	bIsMelee = true;
}

void UPlayerMovementComponent::Server_GetForwardCamera_Implementation(FVector Direction)
{
	DirectionMelee = Direction;
}

void UPlayerMovementComponent::OnDelayFinished()
{
	bIsMelee = false;
}

void UPlayerMovementComponent::MeleeVelocityUpdate(float Value)
{
}

void UPlayerMovementComponent::StopMeleeVelocityEaseTimeline()
{	
	if (!IsMovingOnGround())
	{
		FVector Vel = Velocity;

		FVector HorizontalVel = Vel;
		HorizontalVel.Z = 0.f;

		float MaxAirSpeed = MaxFlySpeed;

		if (HorizontalVel.Size() > MaxAirSpeed)
		{
			HorizontalVel = HorizontalVel.GetSafeNormal() * MaxAirSpeed;
		}
		
		float DesiredFallSpeed = JumpZVelocity * 0.6f;

		Vel.Z = FMath::Min(Vel.Z, DesiredFallSpeed);
		Vel.X = HorizontalVel.X;
		Vel.Y = HorizontalVel.Y;

		Velocity = Vel;
	}

	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
	GravityScale = 2.0f	;
	
	CharacterRef->GetWorldTimerManager().SetTimer(
		SimpleDelayHandle,
		this,
		&UPlayerMovementComponent::OnDelayFinished,
		0.2f,
		false
	);
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

#pragma endregion

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
	
	/*if (CharacterRef && CharacterRef->HasAuthority())
	{
		FVector Start = CharacterRef->GetActorLocation();

		// Scale pour que le point soit lisible en world
		float DebugScale = 0.1f; // ajuste si besoin
		FVector PointLocation = Start + Velocity * DebugScale;

		// Point bleu bien visible
		DrawDebugPoint(
			CharacterRef->GetWorld(),
			PointLocation,
			20.f,          // taille du point
			FColor::Blue,
			false,
			5.0f           // durée en secondes
		);

		// Optionnel : ligne entre le joueur et le point
		DrawDebugLine(
			CharacterRef->GetWorld(),
			Start,
			PointLocation,
			FColor::Cyan,
			false,
			5.0f,
			0,
			2.0f
		);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				131313211,
				5.f,
				FColor::Green,
				FString::Printf(
					TEXT("Velocity: X=%f Y=%f Z=%f"),
					Velocity.X,
					Velocity.Y,
					Velocity.Z
				)
			);
		}
	}*/
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

void UPlayerMovementComponent::StopVelocityEaseTimeline()
{
	bIsSliding = false;
	bPendingCancelSlide = false;
	TimeSliding = 0;
	ResetSlideValues();
	if (VelocityEaseTimeline.IsPlaying())
		VelocityEaseTimeline.Stop();
}

bool UPlayerMovementComponent::CanSlide()
{
	SlideLineTrace();
	bool bResult = IsMovingOnGround() && TimeToWaitBetweenSlide <= 0;
	bResult &= VelocityAtCrouch.Size()	 > 200;
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
	return Super::GetMaxSpeed();
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
	if (bWantsToDash)
	{
		SetMovementMode(MOVE_Custom, CMOVE_Melee);
	}
	
	if (bWantsToCrouch && CanSlide())
	{
		SetMovementMode(MOVE_Custom, CMOVE_Slide);
	}
	
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
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

	if (MovementMode  == EMovementMode::MOVE_Falling)
	{
		InitialHorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.f);
	}
}

void UPlayerMovementComponent::Crouch(bool bClientSimulation)
{
	if (!HasValidData())
	{
		return;
	}

	if (!bClientSimulation && !CanCrouchInCurrentState())
	{
		return;
	}
	
	bResetSlideCrouch	= true;
	
	// See if collision is already at desired size.
	if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == GetCrouchedHalfHeight())
	{
		if (!bClientSimulation)
		{
			CharacterOwner->SetIsCrouched(true);
		}
		CharacterOwner->OnStartCrouch( 0.f, 0.f );
		return;
	}

	if (bClientSimulation && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
	{
		// restore collision size before crouching
		ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		bShrinkProxyCapsule = true;
	}

	// Change collision size to crouching dimensions
	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float OldUnscaledRadius = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	// Height is not allowed to be smaller than radius.
	const float ClampedCrouchedHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, GetCrouchedHalfHeight());
	float HalfHeightAdjust = (OldUnscaledHalfHeight - ClampedCrouchedHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	if( !bClientSimulation )
	{
		// Crouching to a larger height? (this is rare)
		if (ClampedCrouchedHalfHeight > OldUnscaledHalfHeight)
		{
			FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CrouchTrace), false, CharacterOwner);
			FCollisionResponseParams ResponseParam;
			InitCollisionParams(CapsuleParams, ResponseParam);
			const bool bEncroached = GetWorld()->OverlapBlockingTestByChannel(UpdatedComponent->GetComponentLocation() + ScaledHalfHeightAdjust * GetGravityDirection(), GetWorldToGravityTransform(),
				UpdatedComponent->GetCollisionObjectType(), GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleParams, ResponseParam);

			// If encroached, cancel
			if( bEncroached )
			{
				CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(OldUnscaledRadius, OldUnscaledHalfHeight);
				return;
			}
		}

		if (bCrouchMaintainsBaseLocation)
		{
			// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
			UpdatedComponent->MoveComponent(ScaledHalfHeightAdjust * GetGravityDirection(), UpdatedComponent->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
		}

		CharacterOwner->SetIsCrouched(true);
	}

	bForceNextFloorCheck = true;

	// OnStartCrouch takes the change from the Default size, not the current one (though they are usually the same).
	const float MeshAdjust = ScaledHalfHeightAdjust;
	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	HalfHeightAdjust = (DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - ClampedCrouchedHalfHeight);
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	AdjustProxyCapsuleSize();
	CharacterOwner->OnStartCrouch( HalfHeightAdjust, ScaledHalfHeightAdjust );

	// Don't smooth this change in mesh position
	if ((bClientSimulation && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy) || (IsNetMode(NM_ListenServer) && CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy))
	{
		FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
		if (ClientData)
		{
			ClientData->MeshTranslationOffset -= MeshAdjust * -GetGravityDirection();
			ClientData->OriginalMeshTranslationOffset = ClientData->MeshTranslationOffset;
		}
	}
}

void UPlayerMovementComponent::UnCrouch(bool bClientSimulation)
{
	if (CharacterRef->HasAuthority())
	{
		VelocityAtCrouch = FVector::ZeroVector;
	}
	
	if (!HasValidData())
	{
		return;
	}

	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();

	// See if collision is already at desired size.
	if( CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() )
	{
		if (!bClientSimulation)
		{
			CharacterOwner->SetIsCrouched(false);
		}
		CharacterOwner->OnEndCrouch( 0.f, 0.f );
		return;
	}

	const float CurrentCrouchedHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - OldUnscaledHalfHeight;
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	const FVector PawnLocation = UpdatedComponent->GetComponentLocation();

	// Grow to uncrouched size.
	check(CharacterOwner->GetCapsuleComponent());

	if( !bClientSimulation )
	{
		// Try to stay in place and see if the larger capsule fits. We use a slightly taller capsule to avoid penetration.
		const UWorld* MyWorld = GetWorld();
		const float SweepInflation = UE_KINDA_SMALL_NUMBER * 10.f;
		FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(CrouchTrace), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(CapsuleParams, ResponseParam);

		// Compensate for the difference between current capsule size and standing size
		const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - ScaledHalfHeightAdjust); // Shrink by negative amount, so actually grow it.
		const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
		bool bEncroached = true;

		if (!bCrouchMaintainsBaseLocation)
		{
			// Expand in place
			bEncroached = MyWorld->OverlapBlockingTestByChannel(PawnLocation, GetWorldToGravityTransform(), CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
		
			if (bEncroached)
			{
				// Try adjusting capsule position to see if we can avoid encroachment.
				if (ScaledHalfHeightAdjust > 0.f)
				{
					// Shrink to a short capsule, sweep down to base to find where that would hit something, and then try to stand up from there.
					float PawnRadius, PawnHalfHeight;
					CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
					const float ShrinkHalfHeight = PawnHalfHeight - PawnRadius;
					const float TraceDist = PawnHalfHeight - ShrinkHalfHeight;
					const FVector Down = TraceDist * GetGravityDirection();

					FHitResult Hit(1.f);
					const FCollisionShape ShortCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, ShrinkHalfHeight);
					const bool bBlockingHit = MyWorld->SweepSingleByChannel(Hit, PawnLocation, PawnLocation + Down, GetWorldToGravityTransform(), CollisionChannel, ShortCapsuleShape, CapsuleParams);
					if (Hit.bStartPenetrating)
					{
						bEncroached = true;
					}
					else
					{
						// Compute where the base of the sweep ended up, and see if we can stand there
						const float DistanceToBase = (Hit.Time * TraceDist) + ShortCapsuleShape.Capsule.HalfHeight;
						const FVector Adjustment = (-DistanceToBase + StandingCapsuleShape.Capsule.HalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f) * -GetGravityDirection();
						const FVector NewLoc = PawnLocation + Adjustment;
						bEncroached = MyWorld->OverlapBlockingTestByChannel(NewLoc, GetWorldToGravityTransform(), CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
						if (!bEncroached)
						{
							// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
							UpdatedComponent->MoveComponent(NewLoc - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
						}
					}
				}
			}
		}
		else
		{
			// Expand while keeping base location the same.
			FVector StandingLocation = PawnLocation + (StandingCapsuleShape.GetCapsuleHalfHeight() - CurrentCrouchedHalfHeight) * -GetGravityDirection();
			bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, GetWorldToGravityTransform(), CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);

			if (bEncroached)
			{
				if (IsMovingOnGround())
				{
					// Something might be just barely overhead, try moving down closer to the floor to avoid it.
					const float MinFloorDist = UE_KINDA_SMALL_NUMBER * 10.f;
					if (CurrentFloor.bBlockingHit && CurrentFloor.FloorDist > MinFloorDist)
					{
						StandingLocation -= (CurrentFloor.FloorDist - MinFloorDist) * -GetGravityDirection();
						bEncroached = MyWorld->OverlapBlockingTestByChannel(StandingLocation, GetWorldToGravityTransform(), CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
					}
				}				
			}

			if (!bEncroached)
			{
				// Commit the change in location.
				UpdatedComponent->MoveComponent(StandingLocation - PawnLocation, UpdatedComponent->GetComponentQuat(), false, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
				bForceNextFloorCheck = true;
			}
		}

		// If still encroached then abort.
		if (bEncroached)
		{
			return;
		}

		CharacterOwner->SetIsCrouched(false);
	}	
	else
	{
		bShrinkProxyCapsule = true;
	}
	
	const float MeshAdjust = ScaledHalfHeightAdjust;
	AdjustProxyCapsuleSize();
	CharacterOwner->OnEndCrouch( HalfHeightAdjust, ScaledHalfHeightAdjust );

	// Don't smooth this change in mesh position
	if ((bClientSimulation && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy) || (IsNetMode(NM_ListenServer) && CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy))
	{
		FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
		if (ClientData)
		{
			ClientData->MeshTranslationOffset += MeshAdjust * -GetGravityDirection();
			ClientData->OriginalMeshTranslationOffset = ClientData->MeshTranslationOffset;
		}
	}
}

void UPlayerMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPlayerMovementComponent, VelocityAtCrouch);
	DOREPLIFETIME(UPlayerMovementComponent, bIsMelee);
	DOREPLIFETIME(UPlayerMovementComponent, bPendingCancelSlide);
	DOREPLIFETIME(UPlayerMovementComponent, TimeSliding);
}

//Fonction de debug bien sympa
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
