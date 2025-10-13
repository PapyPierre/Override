#include "Player/PlayerMovementComponent.h"
#include "Player/PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"

void UPlayerMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	//SET DEFAULT VALUE TO KEEP ORIGINAL
	DefaultGroundFriction = GroundFriction;
	DefaultBrakingDecelerationWalking = BrakingDecelerationWalking;
	DefaultMaxWalkSpeedCrouched = MaxWalkSpeedCrouched;
	DefaultMaxWalkSpeed = MaxWalkSpeed;
	JumpZVelocity = FirstJumpZVelocity;
	DefaultSprintSpeed = SprintSpeed;
	DefaultAirControl = AirControl;
	DefaultAcceleration = GetMaxAcceleration();

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
		VelocityEaseTimeline.SetTimelineLength(1.f);
		VelocityEaseTimeline.AddInterpFloat(VelocityEaseCurve, TimelineCallback);
		VelocityEaseTimeline.SetLooping(false);
	}
}

void UPlayerMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	VelocityEaseTimeline.TickTimeline(DeltaTime);
	FrameCounter++;

	if (FrameCounter > 1000000)
	{
		FrameCounter = 0;
	}

	if (FrameCounter % 9 == 0)
	{
#pragma region Parkour Verification
	
		CharaLocation = CharacterRef->GetActorLocation();
		CharaForward = CharacterRef->GetActorForwardVector();
		CharaUp = CharacterRef->GetActorUpVector();

#pragma region WALL CLIMB DETECTION
		if ((Velocity.Z < 0.0f || Velocity.Z > 0.0f) && !bGrabbedLedge)
		{
			FCollisionShape Shape = FCollisionShape::MakeBox(FVector(20, 5, 1));
		
			FQuat BoxRotation = CharacterRef->GetActorQuat();

			FVector StartLocation = (CharaLocation + CharaForward * 45) + CharaUp * RaycastStartHeight;
			FVector EndLocation   = (CharaLocation + CharaForward * 45) + CharaUp * RaycastEndHeight;

			bool bHit = GetWorld()->SweepSingleByChannel(
				SweepResult,
				StartLocation,
				EndLocation,
				BoxRotation,
				ECC_WorldStatic,
				Shape,
				TraceParams
			);
		
			if (bHit && SweepResult.Distance > 0) {
				StartLocation = CharaLocation + CharaForward * 5;
				StartLocation = FVector(StartLocation.X, StartLocation.Y, SweepResult.ImpactPoint.Z);

				EndLocation = SweepResult.Location + CharaForward * 5;

				Shape = FCollisionShape::MakeBox(FVector(5, 5, 5));

				bool bHitSecond = GetWorld()->SweepSingleByChannel(SweepResult, StartLocation, EndLocation, CharacterRef->GetActorRotation().Quaternion(), ECC_WorldStatic, Shape, TraceParams);

				if (bHitSecond) {

					FVector OriginBounds;
					FVector BoxExtent;

					SweepResult.GetActor()->GetActorBounds(false, OriginBounds, BoxExtent);

					UCapsuleComponent* Capsule = CharacterRef->GetCapsuleComponent();
					float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

					GrabHeight = (OriginBounds.Z + BoxExtent.Z) - HalfHeight;

					FVector GrabPosition = FVector(
						SweepResult.Location.X - CharaForward.X * 50,
						SweepResult.Location.Y - CharaForward.Y * 50,
						GrabHeight
					);

					CharacterRef->SetActorLocation(GrabPosition);

					FVector NewRotation = FVector(SweepResult.Normal * -1.f);
					CharacterRef->SetActorRotation(UKismetMathLibrary::MakeRotFromX(NewRotation));

					bGrabbedLedge = true;
					StopMovementImmediately();
					bUseControllerDesiredRotation = false;
					SetMovementMode(MOVE_None);	

					if (EdgeClimbMontage && CharacterRef && CharacterRef->GetMesh())
					{
						if (AnimInstance && CharacterRef->HasAuthority())
						{
							FVector TargetLocAndFwd = CharaLocation + CharaForward * 50;
							FVector TargetRelativeLocation = FVector(TargetLocAndFwd.X, TargetLocAndFwd.Y, CharaLocation.Z + 154);

							FLatentActionInfo JumpDelayInfo;
							JumpDelayInfo.CallbackTarget = this;
							JumpDelayInfo.ExecutionFunction = NAME_None;
							JumpDelayInfo.Linkage = 0;
							JumpDelayInfo.UUID = 1;
						
							UKismetSystemLibrary::MoveComponentTo(Capsule, TargetRelativeLocation, CharacterRef->GetActorRotation(), true, true, 1.0, false, EMoveComponentAction::Move,JumpDelayInfo);
							RPC_WallClimbMoveTo(Capsule,TargetRelativeLocation,JumpDelayInfo);
							
							AnimInstance->Montage_Play(EdgeClimbMontage);

							FOnMontageEnded EndDelegate;
							EndDelegate.BindUObject(this, &UPlayerMovementComponent::OnMontageWallClimbEnded);
							AnimInstance->Montage_SetEndDelegate(EndDelegate, EdgeClimbMontage);
						}
					}
				}
			}
		}
#pragma endregion

		else
		{			
			if (HitSecondWallActor && !bMontagePending)
			{
				AnimInstance->Montage_Play(VaultMontage);
				HitSecondWallActor->SetActorEnableCollision(false);

				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UPlayerMovementComponent::OnMontageVaultEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, VaultMontage);

				bMontagePending = true;
			}
		}
	
#pragma endregion
	}

#pragma region Slide Verification
	
	if (bIsSliding)
	{
		if (FrameCounter % 3 == 0)
		{
			SlideLineTrace();
		}
		
		// Hard stop : conditions immédiates pour arrêter la timeline
		bool bStopSliding =
			!bWantsToCrouch ||
			Impact.Z >= SlopeToleranceValue ||
			Velocity.IsNearlyZero();       

		// Soft stop : conditions pour lancer le easing
		bool bShouldStopSliding =
			TimeSliding >= BoostSlidingTime ||
			(TimeSliding > BoostSlidingTime / 2 && Velocity.Length() < 800);
		
		if (FMath::IsNearlyZero(Impact.Z) && Impact.Z <= SlopeToleranceValue)
		{
			if (IsMovingOnGround())
				TimeSliding += DeltaTime;
		}
		
		if (bShouldStopSliding && !bPendingCancelSlide)
		{
			StartVelocityEase(Velocity.GetSafeNormal() * DefaultMaxWalkSpeedCrouched);
		}
		
		if (bStopSliding)
		{
			StopVelocityEaseTimeline();
		}
	}
	
	if (TimeToWaitBetweenSlide >= 0)
	{
		TimeToWaitBetweenSlide -= DeltaTime;
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
				case CMOVE_Sprint:
					ModeStr = TEXT("Sprint");
					break;
					// ajoute ici tous tes CMOVE_XXX
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


void UPlayerMovementComponent::OnMontageWallClimbEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bGrabbedLedge = false;
	SetMovementMode(MOVE_Walking);
}

void UPlayerMovementComponent::OnMontageVaultEnded(UAnimMontage* Montage, bool bInterrupted)
{
	HitSecondWallActor->SetActorEnableCollision(true);
	bMontagePending = false;
	HitSecondWallActor = nullptr;
}

void UPlayerMovementComponent::RPC_WallClimbMoveTo_Implementation(UCapsuleComponent* Capsule,
	FVector TargetRelativeLocation, FLatentActionInfo JumpDelayInfo)
{
	if (CharacterRef->IsLocallyControlled())
		UKismetSystemLibrary::MoveComponentTo(Capsule, TargetRelativeLocation, CharacterRef->GetActorRotation(), true, true, 1.0, false, EMoveComponentAction::Move, JumpDelayInfo);
}

bool UPlayerMovementComponent::CanVaultOrClimb()
{
	float Thickness = 0.f;
	float Height = 0.f;

	AActor* HitWall = ParkourWallDetection(Thickness, Height);

	if (!HitWall)
	{
		return false;
	}

	if (Thickness < MaxVaultThickness && Height < MaxVaultHeight)
	{
		HitSecondWallActor = HitWall;
		return true;
	}

	return false;
}

AActor* UPlayerMovementComponent::ParkourWallDetection(float& Thickness, float& Height)
{
	FVector Start = CharaLocation;
	FVector End = Start + CharaForward * ParkourDistanceDetection;

	// ===== WALL RANGE DETECTION ====
	FHitResult FrontWallHit;
	bool bWallInFront = GetWorld()->LineTraceSingleByChannel(
		FrontWallHit,
		Start,
		End,
		ECC_Visibility,
		TraceParams
	);

	if (bWallInFront)
	{
		Start = CharaLocation;
		End = Start + CharaForward * 500.f; 

		// ===== FIRST TRACE =====
		FHitResult HitWallImpact;
		bool bHitFirstWall = GetWorld()->LineTraceSingleByChannel(
			HitWallImpact,
			Start,
			End,
			ECC_Visibility,
			TraceParams
		);
			
		if (bHitFirstWall)
		{
			// ===== SECOND TRACE HORIZONTAL=====
			FHitResult HitHorizontal;
			bool bWallHorizontalHit = GetWorld()->LineTraceSingleByChannel(
				HitHorizontal,
				End,
				Start,
				ECC_Visibility,
				TraceParams
			);

			Start = HitWallImpact.ImpactPoint + CharaForward * 5;

			FHitResult HitVertical;
			bool bWallVerticalHit = GetWorld()->LineTraceSingleByChannel(
				HitVertical,
				Start + CharaUp * 500.f,
				Start,
				ECC_Visibility,
				TraceParams
				);
				
			if (bWallHorizontalHit)
			{
				if (FrontWallHit.GetComponent() != HitHorizontal.GetComponent())
				{
					UE_LOG(LogTemp, Warning, TEXT("Traces hit different objects"));
					return nullptr;
				}
					Thickness = FVector::Distance(FrontWallHit.ImpactPoint, HitHorizontal.ImpactPoint);
			}

			if (bWallVerticalHit)
			{
				if (FrontWallHit.GetComponent() != HitVertical.GetComponent())
				{
					UE_LOG(LogTemp, Warning, TEXT("Traces hit different objects"));
					return nullptr;
				}
					Height = FVector::Distance(FrontWallHit.ImpactPoint, HitVertical.ImpactPoint);
			}
			
			return HitWallImpact.GetActor();
		}
	}
	
	return nullptr;
}

void UPlayerMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	switch (CustomMovementMode)
	{
	case CMOVE_Sprint:
		PhysSprint(DeltaTime, Iterations);
		break;
	case CMOVE_Slide:
		PhysSlide(DeltaTime, Iterations);
		break;
	default:
		break;
	}
	Super::PhysCustom(DeltaTime, Iterations);
}

#pragma region Sprint
void UPlayerMovementComponent::PhysSprint(float DeltaTime, int32 Iterations)
{
	if (!IsCustomMovementModeOn(CMOVE_Sprint))
	{
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}

	if (!CanSprint() || !bWantsToSprint)
	{
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}
	PhysWalking(DeltaTime, Iterations);
}

void UPlayerMovementComponent::PhysWalking(float DeltaTime, int32 Iterations)
{
	Super::PhysWalking(DeltaTime, Iterations);
	MaxAcceleration = DefaultAcceleration;
}

bool UPlayerMovementComponent::CanSprint() const
{
	if (!IsMovingOnGround() || bWantsToCrouch || IsCrouching())
		return false;

	FVector MoveDir = Velocity.GetSafeNormal();
	FVector ForwardDir = CharacterOwner->GetActorForwardVector();

	float Dot = FVector::DotProduct(MoveDir, ForwardDir);

	return Dot > 0.7f && Velocity.Size() > 0.f;
}

bool UPlayerMovementComponent::IsRunning() const
{
	return IsCustomMovementModeOn(CMOVE_Sprint);
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

	if (!CanSlide() && !bPendingCancelSlide)
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
				MaxWalkSpeedCrouched = 0.0;
				Impact *= SlideImpulse;
				AddImpulse(Impact, true);
				bIsSliding = true;
				VelocityAtCrouch = FVector::ZeroVector;
			}
		}
	}
	
	PhysWalking(DeltaTime, Iterations);
}

bool UPlayerMovementComponent::SlideLineTrace()
{
	FVector End = (CharacterRef->GetActorUpVector() * -200) + CharacterRef->GetActorLocation();
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CharacterRef);
	bool bSlideHitResult = GetWorld()->LineTraceSingleByChannel(SlideHit, CharacterRef->GetActorLocation(), End, ECC_WorldStatic, QueryParams);
	Impact = UKismetMathLibrary::Cross_VectorVector(SlideHit.ImpactNormal, CharacterRef->GetActorRightVector());
	Impact *= -1.0;
	return bSlideHitResult;
}

void UPlayerMovementComponent::EaseVelocityUpdate(float Value)
{
	if (!IsMovingOnGround())
	{
		return;
	}
	
	Velocity = FMath::Lerp(InitialEaseVelocity, TargetEaseVelocity, Value);
}

void UPlayerMovementComponent::StartVelocityEase(const FVector& NewTargetVelocity)
{
	InitialEaseVelocity = Velocity;
	TargetEaseVelocity = NewTargetVelocity;
    
	if (VelocityEaseCurve)
	{
		VelocityEaseTimeline.SetPlayRate(1.f / EaseOutTime); 
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
	bResult &= VelocityAtCrouch.Size() >= DefaultMaxWalkSpeed;
	bResult &= Impact.Z <= SlopeToleranceValue;
	return bResult;
}

bool UPlayerMovementComponent::IsSliding() const
{
	return bIsSliding;
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
	if (IsCustomMovementModeOn(CMOVE_Sprint))
	{
		return DefaultSprintSpeed;
	}
	
	return Super::GetMaxSpeed();
}

bool UPlayerMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() ||
	((IsCustomMovementModeOn(CMOVE_Sprint) || IsCustomMovementModeOn(CMOVE_Slide)) && UpdatedComponent && !
		IsFalling());
}

void UPlayerMovementComponent::PhysFalling(float DeltaTime, int32 Iterations)
{
	Super::PhysFalling(DeltaTime, Iterations);
	
	if (Velocity.SizeSquared2D() > 0.f && JumpCount == 2)
	{
		FVector TargetHorizontalVel = InitialHorizontalVelocity * AirHorizontalRetainPercent;
		FVector CurrentHorizontalVel(Velocity.X, Velocity.Y, 0.f);

		float ReductionSpeed = 1.f;
		FVector NewHorizontalVel = FMath::VInterpTo(
			CurrentHorizontalVel,
			TargetHorizontalVel,
			DeltaTime,
			ReductionSpeed
		);

		Velocity.X = NewHorizontalVel.X;
		Velocity.Y = NewHorizontalVel.Y;
	}
}

bool UPlayerMovementComponent::CanAttemptJump() const
{
	return IsJumpAllowed() &&
		   (IsMovingOnGround() || IsFalling()) &&
		   	(IsSliding() || !bWantsToCrouch);
}

bool UPlayerMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	if (CharacterOwner && CharacterOwner->CanJump())
	{	
		if (!bConstrainToPlane || !FMath::IsNearlyEqual(FMath::Abs(GetGravitySpaceZ(PlaneConstraintNormal)), 1.f))
		{
			const bool bFirstJump = (CharacterOwner->JumpCurrentCountPreJump == 0);

			if (bFirstJump || bDontFallBelowJumpZVelocityDuringJump)
			{
				JumpCount++;
				if (JumpCount == 1)
				{
					Velocity.Z = FMath::Max<FVector::FReal>(Velocity.Z, JumpZVelocity);
				}
				else
				{
					AirControl = SecondJumpAirControl;
					Velocity.Z = FMath::Max<FVector::FReal>(Velocity.Z, SecondJumpZVelocity);
				}
			}
			
			SetMovementMode(MOVE_Falling);
			return true;
		}
	}
	return false;
}

void UPlayerMovementComponent::ResetJumpValues()
{
	JumpCount = 0;
	AirControl = DefaultAirControl;
}

void UPlayerMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
                                                 const FVector& OldVelocity)
{
	if (bWantsToCrouch && CanSlide())
	{
		SetMovementMode(MOVE_Custom, CMOVE_Slide);
	}
	
	if (bWantsToSprint && CanSprint())
	{
		MaxAcceleration = SprintAcceleration;
		SetMovementMode(MOVE_Custom, CMOVE_Sprint);
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
	
	if (IsCustomMovementModeOn(ECustomMovementMode::CMOVE_Sprint)
		|| IsCustomMovementModeOn(ECustomMovementMode::CMOVE_Slide))
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
	Super::Crouch(bClientSimulation);
}

void UPlayerMovementComponent::UnCrouch(bool bClientSimulation)
{
	Super::UnCrouch(bClientSimulation);
}

void UPlayerMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPlayerMovementComponent, VelocityAtCrouch);
}
