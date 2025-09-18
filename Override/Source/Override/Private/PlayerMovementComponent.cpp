// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerMovementComponent.h"
#include "PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UPlayerMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	DefaultGroundFriction = GroundFriction;
	DefaultBrakingDecelerationWalking = BrakingDecelerationWalking;
	DefaultMaxWalkSpeedCrouched = MaxWalkSpeedCrouched;
	JumpZVelocity = FirstJumpZVelocity;

	if (WallRideCurve)
	{
		FOnTimelineFloat TickFunc;
		TickFunc.BindUFunction(this, FName("OnWallRideTimelineTick"));
		WallRideTimeline.AddInterpFloat(WallRideCurve, TickFunc);

		FOnTimelineEvent FinishedFunc;
		FinishedFunc.BindUFunction(this, FName("OnWallRideTimelineFinished"));
		WallRideTimeline.SetTimelineFinishedFunc(FinishedFunc);
	}

	if (VelocityEaseCurve)
	{
		FOnTimelineFloat TimelineCallback;
		TimelineCallback.BindUFunction(this, FName("EaseVelocityUpdate"));
		VelocityEaseTimeline.AddInterpFloat(VelocityEaseCurve, TimelineCallback);
		VelocityEaseTimeline.SetTimelineLength(0.25f);
		VelocityEaseTimeline.SetLooping(false);
	}
}

void UPlayerMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	WallRideTimeline.TickTimeline(DeltaTime);
	VelocityEaseTimeline.TickTimeline(DeltaTime);
	FrameCounter++;

#pragma region WallClimb Verification
	if (Velocity.Z < 0.0) {
		FCollisionShape Shape = FCollisionShape::MakeBox(FVector(25, 5, 1));
		FHitResult SweepResult;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(CharacterRef);

		FVector CharaLocation = CharacterRef->GetActorLocation();
		FVector CharaForward = CharacterRef->GetActorForwardVector();

		FVector StartLocation = (CharaLocation + CharaForward * 45) + FVector(0, 0, 100);
		FVector EndLocation = (CharaLocation + CharaForward * 45) + FVector(0, 0, 50);

		///Debug///
		/*
		DrawDebugBox(
			GetWorld(),
			StartLocation,
			Shape.GetBox(),
			FQuat::Identity,
			FColor::Red,
			false,
			2.0f
		);

		DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Green, false, 2.0f, 0, 2.0f);
		*/

		bool bHit = GetWorld()->SweepSingleByChannel(SweepResult, StartLocation, EndLocation, CharacterRef->GetActorRotation().Quaternion(), ECC_WorldStatic, Shape, QueryParams);

		if (bHit && SweepResult.Distance > 0) {
			StartLocation = CharaLocation + CharaForward * 5;
			StartLocation = FVector(StartLocation.X, StartLocation.Y, SweepResult.ImpactPoint.Z);

			EndLocation = SweepResult.Location + CharaForward * 5;

			Shape = FCollisionShape::MakeBox(FVector(5, 5, 5));

			bool bHitSecond = GetWorld()->SweepSingleByChannel(SweepResult, StartLocation, EndLocation, CharacterRef->GetActorRotation().Quaternion(), ECC_WorldStatic, Shape, QueryParams);

			if (bHitSecond) {

				FVector OriginBounds;
				FVector BoxExtent;

				SweepResult.GetActor()->GetActorBounds(false, OriginBounds, BoxExtent);

				UCapsuleComponent* Capsule = CharacterRef->GetCapsuleComponent();
				float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

				GrabHeight = (OriginBounds.Z + BoxExtent.Z) - HalfHeight;
				FVector NewPosition = FVector(SweepResult.Location.X - CharaForward.X * 50, SweepResult.Location.Y - CharaForward.Z * 50, GrabHeight);
				CharacterRef->SetActorLocation(NewPosition);
				FVector NewRotation = FVector(SweepResult.Normal * -1);
				CharacterRef->SetActorRotation(UKismetMathLibrary::MakeRotFromX(NewRotation));

				bGrabbedLedge = true;
				/*//////////////////////////////////////
				StopMovementImmediately();
				SetMovementMode(MOVE_None);
					//PLAY MONTAGE/ANIMATION HERE
							//WHEN FINISHED
				bGrabbedLedge = false;
				SetMovementMode(MOVE_Walking);
				//////////////////////////////////////*/
				FVector LaunchImpulse = FVector(0, 0, 1000);
				CharacterRef->LaunchCharacter(LaunchImpulse, false, false);
			}
		}
	}
#pragma endregion

#pragma region Pending WallRide Validation
	if (bPendingWallRide)
	{
		FVector VelocityZ = CharacterOwner->GetVelocity();

		if (VelocityZ.Z <= 0.0f)
		{
			bPendingWallRide = false;
			GravityScale = 0.0f;
			WallRideTimeline.PlayFromStart();
		}
	}
#pragma endregion

#pragma region Slide Verification
	if (bIsSliding)
	{
		bool bShouldStopSliding =
			!bWantsToCrouch ||
			Velocity.IsNearlyZero() ||
			(Impact.Z >= MinimumSlideThreshold &&
			(
				TimeSliding >= MaxSlidingTime ||
				Impact.Z >= SlopeToleranceValue ||
				(TimeSliding > MaxSlidingTime / 2 && Velocity.Length() < 800)
			));
		
		if (FrameCounter % 3 == 0)
		{
			SlideLineTrace(); // Je fais ca pour reevaluer la pente 
			FrameCounter = 0;
		}

		if (bShouldStopSliding)
		{
			ResetSlideValues();
			StartVelocityEase(Velocity.GetSafeNormal() * GetMaxSpeed());
		}
	}

	else
	{
		if (VelocityEaseTimeline.IsPlaying() && !bWantsToCrouch)
		{
			VelocityEaseTimeline.Stop();
		}
		if (TimeToWaitBetweenSlide > 0) {
			TimeToWaitBetweenSlide -= DeltaTime;
		}
		else
			bIsSliding = false;;
	}

#pragma endregion

	/////////GROSSE ZONE DE DEBUG
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1132, 5.f, FColor::Green, FString::Printf(TEXT("is Sliding ?: %s"), bIsSliding ? TEXT("true") : TEXT("false")));
		GEngine->AddOnScreenDebugMessage(6541, 5.0, FColor::Blue, "TimeSliding: " + FString::SanitizeFloat(TimeSliding));
		GEngine->AddOnScreenDebugMessage(1, 5.0, FColor::Red, "Speed: " + FString::SanitizeFloat(Velocity.Length()));
	}
	if (CharacterRef)
	{
		if (UCharacterMovementComponent* const MoveComp = CharacterRef->GetCharacterMovement())
		{
			const FString ModeStr = StaticEnum<EMovementMode>()->GetNameStringByValue(MoveComp->MovementMode);
			GEngine->AddOnScreenDebugMessage(9001, 5.0f, FColor::Yellow, FString::Printf(TEXT("MovementMode: %s"), *ModeStr));
			GEngine->AddOnScreenDebugMessage(9002, 5.0f, FColor::Cyan, FString::Printf(TEXT("MaxWalkSpeed: %.1f"), MoveComp->MaxWalkSpeed));
		}
	}
	/////////FIN DE LA GRANDE ZONE DE DEBUG

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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
	case CMOVE_WallRide:
		PhysWallRide(DeltaTime, Iterations);
		break;
	default:
		break;
		;
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

bool UPlayerMovementComponent::CanSprint() const
{
	return IsMovingOnGround() && !bWantsToCrouch;
}

#pragma endregion

#pragma region WallRide

void UPlayerMovementComponent::PhysWallRide(float DeltaTime, int32 Iterations)
{
	TimeWallRunning += DeltaTime;

	if (!IsCustomMovementModeOn(CMOVE_WallRide) || !CanWallRun() || !bWantToWallRide)
	{
		TimeWallRunning = 0;
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}

	if (!CharacterRef->WallRunHitResult.GetActor())
		return;

	float DirDotHit = FVector::DotProduct(CharacterRef->WallRunHitResult.GetActor()->GetActorForwardVector(), GetOwner()->GetActorForwardVector());
	PlayerDirection = GetOwner()->GetActorForwardVector();

	if (WallRideCurve && !WallRideTimeline.IsPlaying())
	{
		bPendingWallRide = true;
	}
	DoJump(true, DeltaTime);
	bWantToWallRide = false;
}

void UPlayerMovementComponent::OnWallRideTimelineTick(float Value)
{
	if (!CloseToWall || Velocity.Length() < 100) {
		WallRideTimeline.Stop();
		OnWallRideTimelineFinished();
		return;
	}

	AddImpulse(PlayerDirection * 1200, false);

	float Lerpedvalue = FMath::Lerp(0.f, -20.f, Value);
	FRotator NewRotation = CharacterRef->GetActorRotation();
	NewRotation.Roll = Lerpedvalue;

	CharacterRef->Controller->SetControlRotation(NewRotation);
}

void UPlayerMovementComponent::OnWallRideTimelineFinished()
{
	GravityScale = 2.f;
	TimeWallRunning = 0;
	bHasResetWallRide = false;

	CharacterRef->bUseControllerRotationYaw = true;
	CharacterRef->GetCharacterMovement()->bOrientRotationToMovement = false;

	FRotator ResetRot = CharacterRef->Controller->GetControlRotation();
	ResetRot.Roll = 0.f;

	ResetRot.Yaw += 0.01f;
	CharacterRef->Controller->SetControlRotation(ResetRot);
}

bool UPlayerMovementComponent::CanWallRun() const
{
	return !IsMovingOnGround() && CloseToWall && bHasResetWallRide;
}

#pragma endregion

#pragma region Slide
void UPlayerMovementComponent::PhysSlide(float DeltaTime, int32 Iterations)
{	
	TimeSliding += DeltaTime;

	if (!IsCustomMovementModeOn(CMOVE_Slide))
	{
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}

	if (!CanSlide())
	{
		ResetSlideValues();
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}

	if (GetOwner() && !bIsSliding)
	{
		if (SlideLineTrace()) {
			if (Impact.Z <= SlopeToleranceValue) {
				GroundFriction = 0.0;
				BrakingDecelerationWalking = 1400;
				MaxWalkSpeedCrouched = 0.0;
				Impact *= SlideImpulse;
				AddImpulse(Impact, true);
				bIsSliding = true;
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
	if (!IsMovingOnGround()) {
		VelocityEaseTimeline.Stop();
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
		VelocityEaseTimeline.PlayFromStart();
	}
}

bool UPlayerMovementComponent::CanSlide() const
{
	bool bResult = IsMovingOnGround() && TimeToWaitBetweenSlide <= 0 && bWantsToCrouch;
	bResult &= Velocity.Length() >= MaxWalkSpeed;
	return bResult;
}

bool UPlayerMovementComponent::IsSliding() const
{
	return IsCustomMovementModeOn(CMOVE_Slide);
}

void UPlayerMovementComponent::ResetSlideValues()
{
	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
	MaxWalkSpeedCrouched = DefaultMaxWalkSpeedCrouched;
	TimeToWaitBetweenSlide = SlidingCoolDown;
	TimeSliding = 0;
	if (bWantsToCrouch)
		SetMovementMode(MOVE_Walking);
	else
	{
		CustomMovementMode = CMOVE_Sprint;
		UnCrouch(true);
	}
	bIsSliding = false;
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
		return MaxWalkSpeed * SprintSpeedMultiplier;
	}

	if (IsCustomMovementModeOn(MOVE_Falling))
	{
		float ComputedVelocity = Velocity.Size() - JumpDeceleration;
		return FMath::Max(ComputedVelocity, MinJumpHorizontalSpeed);
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
}

bool UPlayerMovementComponent::CanAttemptJump() const
{
	return IsJumpAllowed() && (IsMovingOnGround() || IsFalling());
}

bool UPlayerMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	UE_LOG(LogTemp, Warning, TEXT("DoJump %d "), JumpCount);
	if (CharacterOwner && CharacterOwner->CanJump())
	{	
		if (!bConstrainToPlane || !FMath::IsNearlyEqual(FMath::Abs(GetGravitySpaceZ(PlaneConstraintNormal)), 1.f))
		{
			const bool bFirstJump = (CharacterOwner->JumpCurrentCountPreJump == 0);

			if (bFirstJump || bDontFallBelowJumpZVelocityDuringJump)
			{
				if (JumpCount == 0)
				{
					Velocity.Z = FMath::Max<FVector::FReal>(Velocity.Z, JumpZVelocity);
					JumpCount++;
				}
				else
				{
					Velocity.Z = FMath::Max<FVector::FReal>(Velocity.Z, SecondJumpZVelocity);
					JumpCount = 0;
				}
			}
			
			SetMovementMode(MOVE_Falling);
			return true;
		}
	}
	
	return false;
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
		SetMovementMode(MOVE_Custom, CMOVE_Sprint);
	}

	if (bWantToWallRide && CanWallRun()) {
		SetMovementMode(MOVE_Custom, CMOVE_WallRide);
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
}

void UPlayerMovementComponent::Crouch(bool bClientSimulation)
{
	if (!CanCrouchInCurrentState())
		return;
	
	bWantsToCrouch = true;
	bIsCrouched = true;
	
	if (ACharacter* CharOwner = Cast<ACharacter>(PawnOwner))
	{
		CharOwner->OnStartCrouch(CharOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), CharOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	}
}

void UPlayerMovementComponent::UnCrouch(bool bClientSimulation)
{
	bWantsToCrouch = false;
	bIsCrouched = false;

	if (ACharacter* CharOwner = Cast<ACharacter>(PawnOwner))
	{
		CharOwner->OnEndCrouch(CharOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), CharOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	}
}
