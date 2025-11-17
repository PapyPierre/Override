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

		//Sprint
		SprintSpeed = MovementData->SprintSpeed;
		SprintAcceleration = MovementData->SprintAcceleration;

		//Jump
		FirstJumpZVelocity = MovementData->FirstJumpZVelocity;
		SecondJumpZVelocity = MovementData->SecondJumpZVelocity;
		SecondJumpAirControl = MovementData->SecondJumpAirControl;
		AirHorizontalRetainPercent = MovementData->AirHorizontalRetainPercent;
		CoyoteTime = MovementData->CoyoteTime;

		//Parkour
		MaxVaultThickness = MovementData->MaxVaultThickness;
		MaxVaultHeight = MovementData->MaxVaultHeight;
		RaycastStartHeight = MovementData->RaycastStartHeight;
		RaycastEndHeight = MovementData->RaycastEndHeight;

		EdgeClimbMontage = MovementData->EdgeClimbMontage;
		VaultMontage = MovementData->VaultMontage;
		ParkourDistanceDetection = MovementData->ParkourDistanceDetection;
	}

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

			FVector StartLocation = (CharaLocation + CharaForward * (ParkourDistanceDetection / 2)) + CharaUp * RaycastStartHeight;
			FVector EndLocation   = (CharaLocation + CharaForward * (ParkourDistanceDetection / 2)) + CharaUp * RaycastEndHeight;

			// Object query : détecte seulement les objets statiques
			FCollisionObjectQueryParams ObjectQuery;
			ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);

			bool bHit = GetWorld()->SweepSingleByObjectType(
				SweepResult,
				StartLocation,
				EndLocation,
				BoxRotation,
				ObjectQuery,
				Shape,
				TraceParams
			);

			// ======= DEBUG =======
			if (bDebugLedge)
			{
				DrawDebugBox(
					GetWorld(),
					(StartLocation + EndLocation) / 2,
					FVector(20, 5, 1),
					BoxRotation,
					FColor::Red,
					false,
					2.0f,
					0,
					1.0f
				);
				DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f, 0, 1.0f);
			}
			// =====================

			if (bHit && SweepResult.Distance > 0)
			{
				StartLocation = CharaLocation + CharaForward * 5;
				StartLocation = FVector(StartLocation.X, StartLocation.Y, SweepResult.ImpactPoint.Z);
				EndLocation = SweepResult.Location + CharaForward * 5;
				Shape = FCollisionShape::MakeBox(FVector(5, 5, 5));

				bool bHitSecond = GetWorld()->SweepSingleByObjectType(
					SweepResult,
					StartLocation,
					EndLocation,
					CharacterRef->GetActorRotation().Quaternion(),
					ObjectQuery,
					Shape,
					TraceParams
				);

				// ======= DEBUG =======
				if (bDebugLedge)
				{
					DrawDebugBox(
						GetWorld(),
						(StartLocation + EndLocation) / 2,
						FVector(5, 5, 5),
						CharacterRef->GetActorQuat(),
						FColor::Green,
						false,
						2.0f,
						0,
						1.0f
					);
					DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Green, false, 2.0f, 0, 1.0f);
				}
				// =====================

				if (bHitSecond)
				{
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

					// ======= DEBUG Grab Position =======
					if (bDebugLedge)
					{
						DrawDebugSphere(GetWorld(), GrabPosition, 5.0f, 12, FColor::Blue, false, 2.0f);
					}
					// ===================================

					StopMovementImmediately();
					SetMovementMode(MOVE_None);

					if (EdgeClimbMontage && CharacterRef && CharacterRef->GetMesh())
					{
						if (AnimInstance && CharacterRef->HasAuthority())
						{
							bGrabbedLedge = true;

							FVector TargetLocAndFwd = CharaLocation + CharaForward * 50;
							FVector TargetRelativeLocation = FVector(TargetLocAndFwd.X, TargetLocAndFwd.Y, CharaLocation.Z + 154);

							HitSecondWallActor = SweepResult.GetActor();
							HitSecondWallActor->SetActorEnableCollision(false);
							
							/*FLatentActionInfo JumpDelayInfo;
							JumpDelayInfo.CallbackTarget = this;
							JumpDelayInfo.ExecutionFunction = FName("OnMoveNoOp");
							JumpDelayInfo.UUID = 999;
							JumpDelayInfo.Linkage = 0;

							UKismetSystemLibrary::MoveComponentTo(
								Capsule,
								TargetRelativeLocation,
								CharacterRef->GetActorRotation(),
								true, true, 1.0, false,
								EMoveComponentAction::Move,
								JumpDelayInfo
							);
							*/

							//RPC_WallClimbMoveTo(TargetRelativeLocation, CharacterRef->GetActorRotation());

							FName EndFuncName = GET_FUNCTION_NAME_CHECKED(UPlayerMovementComponent, OnMontageWallClimbEnded);
							Multicast_PlayWallClimbMontage(EdgeClimbMontage, EndFuncName,SweepResult.GetActor(),CharacterRef);
						}
					}
				}
			}
		}


#pragma endregion

		else
		{
			if (HitSecondWallActor && !bMontagePending && CharacterRef->IsLocallyControlled())
			{
				UCapsuleComponent* Capsule = CharacterRef->GetCapsuleComponent();
				UPrimitiveComponent* WallComp = HitSecondWallActor->FindComponentByClass<UPrimitiveComponent>();
				if (Capsule && WallComp)
				{
					Capsule->MoveIgnoreActors.Add(HitSecondWallActor);
				}
				Server_CallVaultAnimation(HitSecondWallActor);
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
		bCoolDownFinished = false;
	}
	else
	{
		if (!bCoolDownFinished)
		{
			if (CharacterRef->HasAuthority())
				VelocityAtCrouch = FVector::ZeroVector;
		}
		bCoolDownFinished = true;
	}
	
#pragma endregion

#pragma region DEBUG
	/*
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
	*/
#pragma endregion
		
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPlayerMovementComponent::OnMontageWallClimbEnded(UAnimMontage* Montage, bool bInterrupted)
{
	JumpCount--;
	bGrabbedLedge = false;
	SetMovementMode(MOVE_Walking);
}

void UPlayerMovementComponent::Multicast_PlayWallClimbMontage_Implementation(UAnimMontage* Montage, FName EndCallbackFunctionName, AActor* Wall, APlayerCharacter* Player)
{
	if (!CharacterRef) return;
	if (!Wall || !Player) return;

	AnimInstance = CharacterRef->GetMesh()->GetAnimInstance();
	if (!AnimInstance || !Montage) return;

	AnimInstance->Montage_Play(Montage);

	if (!EndCallbackFunctionName.IsNone())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUFunction(this, EndCallbackFunctionName);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	}
	
	UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
	UPrimitiveComponent* WallComp = Wall->FindComponentByClass<UPrimitiveComponent>();
	if (Capsule && WallComp)
	{
		Capsule->MoveIgnoreActors.Add(Wall);
	}
}


void UPlayerMovementComponent::Server_CallVaultAnimation_Implementation(AActor* Actor)
{
	HitSecondWallActor = Actor;
	FName EndFuncName = GET_FUNCTION_NAME_CHECKED(UPlayerMovementComponent, OnMontageVaultEnded);
	Multicast_PlayWallClimbMontage(VaultMontage, EndFuncName, HitSecondWallActor, this->CharacterRef);
}

void UPlayerMovementComponent::OnMontageVaultEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (HitSecondWallActor)
	{
		UCapsuleComponent* Capsule = CharacterRef->GetCapsuleComponent();
		if (Capsule)
		{
			DebugPrintClientIds();
			Capsule->MoveIgnoreActors.Remove(HitSecondWallActor);
		}
	}
	
	bMontagePending = false;
	HitSecondWallActor = nullptr;
}

void UPlayerMovementComponent::RPC_WallClimbMoveTo_Implementation(FVector TargetRelativeLocation, FRotator TargetRotation)
{
	if (UCapsuleComponent* Capsule = CharacterRef->GetCapsuleComponent())
	{
		FLatentActionInfo JumpDelayInfo;
		JumpDelayInfo.CallbackTarget = this;
		JumpDelayInfo.ExecutionFunction = FName("OnMoveNoOp");
		JumpDelayInfo.UUID = 999;
		JumpDelayInfo.Linkage = 0;
		
		UKismetSystemLibrary::MoveComponentTo(
			Capsule,
			TargetRelativeLocation,
			TargetRotation,
			true, true, 1.0, false,
			EMoveComponentAction::Move,
			JumpDelayInfo
		);
	}
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

    // ===== WALL RANGE DETECTION =====
    FHitResult FrontWallHit;
    bool bWallInFront = GetWorld()->LineTraceSingleByChannel(
        FrontWallHit,
        Start,
        End,
        ECC_Visibility,
        TraceParams
    );

    // ===== DEBUG FRONT WALL =====
    if (bDebugLedge)
    {
        DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f, 0, 1.0f);
        if (bWallInFront)
        {
            DrawDebugSphere(GetWorld(), FrontWallHit.ImpactPoint, 5.0f, 12, FColor::Red, false, 2.0f);
        }
    }
    // ================================

    if (bWallInFront)
    {
        Start = FrontWallHit.ImpactPoint + CharaForward * 5;

        // ===== SECOND TRACE HORIZONTAL =====
        FHitResult HitHorizontal;
        bool bWallHorizontalHit = GetWorld()->LineTraceSingleByChannel(
            HitHorizontal,
            End,
            Start,
            ECC_Visibility,
            TraceParams
        );

        // ===== DEBUG HORIZONTAL =====
        if (bDebugLedge)
        {
            DrawDebugLine(GetWorld(), End, Start, FColor::Green, false, 2.0f, 0, 1.0f);
            if (bWallHorizontalHit)
            {
                DrawDebugSphere(GetWorld(), HitHorizontal.ImpactPoint, 5.0f, 12, FColor::Green, false, 2.0f);
            }
        }
        // ==================================

        // ===== SECOND TRACE VERTICAL =====
        FHitResult HitVertical;
        bool bWallVerticalHit = GetWorld()->LineTraceSingleByChannel(
            HitVertical,
            Start + CharaUp * 500.f,
            Start,
            ECC_Visibility,
            TraceParams
        );

        // ===== DEBUG VERTICAL =====
        if (bDebugLedge)
        {
            DrawDebugLine(GetWorld(), Start + CharaUp * 500.f, Start, FColor::Blue, false, 2.0f, 0, 1.0f);
            if (bWallVerticalHit)
            {
                DrawDebugSphere(GetWorld(), HitVertical.ImpactPoint, 5.0f, 12, FColor::Blue, false, 2.0f);
            }
        }
        // ==================================

        // ===== CALCULS DE THICKNESS ET HEIGHT =====
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

        return FrontWallHit.GetActor();
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
	bResult &= FMath::IsNearlyEqual(VelocityAtCrouch.Size(), DefaultSprintSpeed, 100);
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
	if (!HasValidData())
	{
		return;
	}

	if (!bClientSimulation && !CanCrouchInCurrentState())
	{
		return;
	}

	// See if collision is already at desired size.
	if (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() == CrouchedHalfHeight)
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
	const float ClampedCrouchedHalfHeight = FMath::Max3(0.f, OldUnscaledRadius, CrouchedHalfHeight);
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
	DOREPLIFETIME(UPlayerMovementComponent, bGrabbedLedge);
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
