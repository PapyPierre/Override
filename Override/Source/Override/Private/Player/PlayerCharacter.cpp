#include "Player/PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/TargetingComponent.h"
#include "Engine/Engine.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Hacks/GameplayHackTargetData.h"
#include "Player/CustomPlayerState.h"
#include "Net/UnrealNetwork.h"

APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPlayerMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	if (!PlayerMovementComponent) PlayerMovementComponent = Cast<UPlayerMovementComponent>(GetCharacterMovement());

	PlayerMovementComponent->CharacterRef = this;
	bReplicates = true;
	GetCharacterMovement()->SetIsReplicated(true);
	
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("Targeting Component"));
}

UAbilitySystemComponent* APlayerCharacter::GetAbilitySystemComponent() const
{
	return GetCustomPlayerState()->GetAbilitySystemComponent();
}

void APlayerCharacter::Target()
{
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultCoyoteTime = PlayerMovementComponent->CoyoteTime;
}

void APlayerCharacter::Sprint()
{
	if (IsLocallyControlled())
	{
		bool bCan = PlayerMovementComponent->CanSprint();
		PlayerMovementComponent->bWantsToSprint = bCan;
		PlayerMovementComponent->MaxWalkSpeed = PlayerMovementComponent->DefaultSprintSpeed;
		RPC_SetSprint(bCan);
	}
}

void APlayerCharacter::RPC_SetSprint_Implementation(bool value)
{
	PlayerMovementComponent->bWantsToSprint = value && PlayerMovementComponent->CanSprint();
	if (PlayerMovementComponent->bWantsToSprint)
		PlayerMovementComponent->MaxWalkSpeed = PlayerMovementComponent->DefaultSprintSpeed;
	else
		PlayerMovementComponent->MaxWalkSpeed = PlayerMovementComponent->DefaultMaxWalkSpeed;
}

void APlayerCharacter::StopSprint()
{
	if (IsLocallyControlled())
	{
		PlayerMovementComponent->bWantsToSprint = false;
		PlayerMovementComponent->MaxWalkSpeed = PlayerMovementComponent->DefaultMaxWalkSpeed;
		RPC_SetSprint(false);
	}
}

void APlayerCharacter::AimWeapon()
{
	if (IsLocallyControlled())
	{
		SetAimingState(true);
		ServerSetAim(true);
	}
}

void APlayerCharacter::StopAimWeapon()
{
	if (IsLocallyControlled())
	{
		SetAimingState(false);
		ServerSetAim(false);
	}
}

void APlayerCharacter::SetAimingState(bool bNewAiming)
{
	if (bIsAimingWeapon == bNewAiming)
		return;

	bIsAimingWeapon = bNewAiming;
	UpdateAimingSettings();
}

void APlayerCharacter::UpdateAimingSettings()
{
	if (bIsAimingWeapon)
	{
		MouseSensitivity = MouseAimSensitivity;
		PlayerMovementComponent->MaxWalkSpeedCrouched = AimCrouchedSpeed;
		PlayerMovementComponent->MaxWalkSpeed = AimSpeed;
	}
	else
	{
		MouseSensitivity = 1.0f;
		PlayerMovementComponent->MaxWalkSpeedCrouched = PlayerMovementComponent->DefaultMaxWalkSpeedCrouched;
		PlayerMovementComponent->MaxWalkSpeed = PlayerMovementComponent->DefaultMaxWalkSpeed;
	}

	OnRep_IsAimingWeapon_BP();
}

void APlayerCharacter::OnRep_IsAimingWeapon()
{
	UpdateAimingSettings();
}

bool APlayerCharacter::ServerSetAim_Validate(bool bNewAiming)
{
	return true;
}

void APlayerCharacter::ServerSetAim_Implementation(bool bNewAiming)
{
	SetAimingState(bNewAiming);
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	//GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Yellow, TEXT("PossessedBy"));

	// Server-side
	SetControllerRef();

	if (PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (InputMappingContext) Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	// Server-side
	InitAbilitySystem();
}

void APlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	//GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Yellow, TEXT("OnRep_PlayerState"));

	// Client-side
	SetControllerRef();
	InitAbilitySystem();
}

void APlayerCharacter::Tick(float DeltaTime)
{
	if (!PlayerController) return;

	if (IsLocallyControlled())
	{
		CameraShake();

		if (PlayerMovementComponent->IsRunning())
		{
			float Speed = GetVelocity().Size();
			float TargetFOV = FMath::GetMappedRangeValueClamped(
				FVector2D(PlayerMovementComponent->DefaultMaxWalkSpeed, PlayerMovementComponent->DefaultSprintSpeed),
				FVector2D(DefaultFOV, SprintFOV),
				Speed
			);

			float NewFOV = FMath::FInterpTo(
				FirstPersonCameraComponent->GetFOVAngle(),
				TargetFOV,
				DeltaTime,
				FOVInterpSpeed
			);

			FirstPersonCameraComponent->SetFOV(NewFOV);
		}

		if (bIsAimingWeapon)
		{
			float NewFOV = FMath::FInterpTo(
				FirstPersonCameraComponent->GetFOVAngle(),
				AimFOV,
				DeltaTime,
				FOVInterpSpeed
			);

			FirstPersonCameraComponent->SetFOV(NewFOV);
		}
		else if (!FMath::IsNearlyEqual(FirstPersonCameraComponent->GetFOVAngle(), DefaultFOV) && !bIsAimingWeapon && PlayerMovementComponent->IsMovingOnGround() && !PlayerMovementComponent->IsSliding())
		{
			float NewFOV = FMath::FInterpTo(
				FirstPersonCameraComponent->GetFOVAngle(),
				DefaultFOV,
				DeltaTime,
				FOVInterpSpeed
			);

			FirstPersonCameraComponent->SetFOV(NewFOV);
		}
	}

	Super::Tick(DeltaTime);
}

void APlayerCharacter::CameraShake()
{
	if (PlayerMovementComponent->IsMovingOnGround())
	{
		if (GetVelocity() == FVector::ZeroVector)
			FirstPersonCameraComponent->StartCameraShake(ShakeIdle, 1.0f, ECameraShakePlaySpace::CameraLocal,
			                                             FRotator::ZeroRotator);
		else
		{
			if (PlayerMovementComponent->IsRunning() && GetVelocity().Size() > PlayerMovementComponent->
				DefaultSprintSpeed)
				FirstPersonCameraComponent->StartCameraShake(ShakeRunning, 1.0f, ECameraShakePlaySpace::CameraLocal,
				                                             FRotator::ZeroRotator);
			else if (!PlayerMovementComponent->IsSliding() && !PlayerMovementComponent->IsCrouching())
				FirstPersonCameraComponent->StartCameraShake(ShakeWalk, 1.0f, ECameraShakePlaySpace::CameraLocal,
				                                             FRotator::ZeroRotator);
		}
	}
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (IsLocallyControlled())
	{
		FirstPersonCameraComponent->StartCameraShake(ShakeJump, 1.0f, ECameraShakePlaySpace::CameraLocal,
		                                             FRotator::ZeroRotator);
	}
	PlayerMovementComponent->ResetJumpValues();
}

void APlayerCharacter::Falling()
{
	JumpCurrentCount--;

	GetWorldTimerManager().SetTimer(
		JumpDelayHandle,
		this,
		&APlayerCharacter::OnJumpDelayFinished,
		DefaultCoyoteTime,
		false
	);
}

void APlayerCharacter::Jump()
{
	if (!PlayerMovementComponent->CanVaultOrClimb())
	{
		Super::Jump();
	}
}

bool APlayerCharacter::CanJumpInternal_Implementation() const
{
	return JumpIsAllowedInternal();
}

void APlayerCharacter::OnJumpDelayFinished()
{
	JumpCurrentCount++;
}

void APlayerCharacter::Server_SetCrouchVelocity_Implementation(const FVector& InVelocity)
{
	PlayerMovementComponent->VelocityAtCrouch = InVelocity;
}

void APlayerCharacter::Crouch(bool bClientSimulation)
{
	PlayerMovementComponent->VelocityAtCrouch = GetVelocity();

	if (!HasAuthority())
	{
		Server_SetCrouchVelocity(PlayerMovementComponent->VelocityAtCrouch);
	}

	Super::Crouch(bClientSimulation);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (Hack1Action)
			EnhancedInput->BindAction(Hack1Action, ETriggerEvent::Started, this,
			                          &APlayerCharacter::ActivateHack1);

		if (Hack2Action)
			EnhancedInput->BindAction(Hack2Action, ETriggerEvent::Started,
			                          this, &APlayerCharacter::ActivateHack2);

		if (Hack3Action)

			EnhancedInput->BindAction(Hack3Action, ETriggerEvent::Started,
			                          this, &APlayerCharacter::ActivateHack3);
	}
}

void APlayerCharacter::SetControllerRef()
{
	PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController) FirstPersonCameraComponent = PlayerController->PlayerCameraManager;
}

void APlayerCharacter::InitAbilitySystem()
{
	if (ACustomPlayerState* PS = GetCustomPlayerState())
	{
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
		{
			ASC->InitAbilityActorInfo(PS, this);
		}
	}

	OnPostAbilitySystemInit();
}

ACustomPlayerState* APlayerCharacter::GetCustomPlayerState() const
{
	return GetPlayerState<ACustomPlayerState>();
}

void APlayerCharacter::ActivateHack1()
{
	SendHackEventWithData(Hack1Tag);
}

void APlayerCharacter::ActivateHack2()
{
	SendHackEventWithData(Hack2Tag);
}

void APlayerCharacter::ActivateHack3()
{
	SendHackEventWithData(Hack3Tag);
}

void APlayerCharacter::SendHackEventWithData(FGameplayTag EventTag)
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("SendHackEventWithData"));
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	if (HasAuthority())
	{
		// Should not be read
		UE_LOG(LogTemp, Warning, TEXT("SERVER : Send event %s"), *EventTag.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CLIENT : Send event %s"), *EventTag.ToString());
	}

	FGameplayEventData EventData;
	EventData.Instigator = this;
	EventData.Target = this;
	EventData.EventTag = EventTag;

	FGameplayHackTargetData* HackTargetData = new FGameplayHackTargetData();

	if (TargetingComponent && TargetingComponent->CurrentTargets.Num() > 0)
	{
		for (AActor* Target : TargetingComponent->CurrentTargets)
		{
			if (Target)
			{
				HackTargetData->Targets.Add(Target);
			}
		}
	}

	FGameplayAbilityTargetDataHandle Handle;
	Handle.Add(HackTargetData);
	EventData.TargetData = Handle;

	ASC->HandleGameplayEvent(EventTag, &EventData);
}

void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerCharacter, bIsAimingWeapon);
}
