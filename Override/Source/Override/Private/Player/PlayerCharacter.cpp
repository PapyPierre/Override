#include "Player/PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/TargetingComponent.h"
#include "Engine/Engine.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Abilities/CustomAbilityTargetData.h"
#include "Abilities/FAbilitySlotData.h"
#include "Player/CustomPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Player/MovementStats.h"

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
	AbilitySlotComponent = CreateDefaultSubobject<UAbilitySlotComponent>(TEXT("Ability Slot Component"));
}

UAbilitySystemComponent* APlayerCharacter::GetAbilitySystemComponent() const
{
	return GetCustomPlayerState()->GetAbilitySystemComponent();
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (PlayerMovementComponent->MovementData)
	{
		DefaultFOV = PlayerMovementComponent->MovementData->DefaultFOV;
		SprintFOV = PlayerMovementComponent->MovementData->SprintFOV;
		SlideFOV = PlayerMovementComponent->MovementData->SlideFOV;
		FOVInterpSprintSpeed = PlayerMovementComponent->MovementData->FOVInterpSprintSpeed;
		FOVInterpNormalSpeed = PlayerMovementComponent->MovementData->FOVInterpNormalSpeed;
		FOVInterpAimSpeed = PlayerMovementComponent->MovementData->FOVInterpAimSpeed;
		FOVInterpSlideSpeed = PlayerMovementComponent->MovementData->FOVInterpSlideSpeed;

		AimFOV = PlayerMovementComponent->MovementData->AimFOV;
		AimCrouchedSpeed = PlayerMovementComponent->MovementData->AimCrouchedSpeed;
		AimSpeed = PlayerMovementComponent->MovementData->AimSpeed;
		MouseSensitivity = PlayerMovementComponent->MovementData->MouseSensitivity;
		MouseAimSensitivity = PlayerMovementComponent->MovementData->MouseAimSensitivity;
	}

	DefaultCoyoteTime = PlayerMovementComponent->CoyoteTime;
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

void APlayerCharacter::ServerSetAim_Implementation(bool bNewAiming)
{
	SetAimingState(bNewAiming);
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

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

	// Client-side
	SetControllerRef();

	InitAbilitySystem();
}

void APlayerCharacter::Tick(float DeltaTime)
{
	if (!PlayerController) return;

	if (IsLocallyControlled())
	{
		CameraManager.SetFov(this, PlayerMovementComponent, DeltaTime);
	}

	Super::Tick(DeltaTime);
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (PlayerMovementComponent->bWantsToCrouch)
	{
		PlayerMovementComponent->bResetSlide = true;
	}

	if (IsLocallyControlled())
	{
		FirstPersonCameraComponent->StartCameraShake(ShakeLanding, 1.0f, ECameraShakePlaySpace::CameraLocal,
		                                             FRotator::ZeroRotator);
	}
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
		if (Ability1Action)
		{
			EnhancedInput->BindAction(Ability1Action, ETriggerEvent::Started, this,
			                          &APlayerCharacter::UseAbility, 0);
		}


		if (Ability2Action)
			EnhancedInput->BindAction(Ability2Action, ETriggerEvent::Started, this,
			                          &APlayerCharacter::UseAbility, 1);
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

	AbilitySlotComponent->Init();

	GiveCharacterAbilities();
	
	OnPostAbilitySystemInit();
}

void APlayerCharacter::Launch(const FVector& Force)
{
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	LaunchCharacter(Force, true, true);
}

ACustomPlayerState* APlayerCharacter::GetCustomPlayerState() const
{
	return GetPlayerState<ACustomPlayerState>();
}

void APlayerCharacter::UseAbility(int index)
{
	if (ACustomPlayerState* PS = GetCustomPlayerState())
	{
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
		{
			if (AbilitySlotComponent && ASC)
			{
				UE_LOG(LogTemp, Log, TEXT("Try to use ability %i"), index + 1);
				
				ActivateAbilityInSlot(index, TargetingComponent->GetPointInSight());

				OnAbilityActivated(index);
			}
		}
	}
}

void APlayerCharacter::ActivateAbilityInSlot(int32 SlotIndex, FVector CurrentPointInSight)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	
	if (!ASC || !AbilitySlotComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing ASC or AbilitySlotComponent"));
	}

	const FAbilitySlotData SlotData = AbilitySlotComponent->GetAbilityInSlot(SlotIndex);
	
	if (!SlotData.AbilityHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("No ability handle in slot %d"), SlotIndex + 1);
	}

	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(SlotData.AbilityHandle);
	if (!Spec)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find ability spec for slot %d"), SlotIndex + 1);
	}

	FCustomAbilityTargetData* TargetData = new FCustomAbilityTargetData();

	if (TargetingComponent->CurrentTargets.Num() > 0)
	{
		for (const AActor* Target : TargetingComponent->CurrentTargets)
		{
			if (Target) TargetData->Targets.Add(const_cast<AActor*>(Target));
		}
	}

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	TargetDataHandle.Add(TargetData);

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddOrigin(CurrentPointInSight);
	
	if (!Spec->GameplayEventData.IsValid())
	{
		Spec->GameplayEventData = MakeShared<FGameplayEventData>();
	}

	Spec->GameplayEventData->Instigator = this;
	Spec->GameplayEventData->Target = this;
	Spec->GameplayEventData->TargetData = TargetDataHandle;
	Spec->GameplayEventData->ContextHandle = ContextHandle;
	
	SetupAbilityDataInSpec(SlotIndex, CurrentPointInSight, Targets);
	ASC->AbilityLocalInputPressed(SlotIndex);
}


void APlayerCharacter::GiveCharacterAbilities()
{
	if (!GetAbilitySystemComponent() || !HasAuthority()) return;

	for (int i = 0; i < CharacterAbilities.Num(); ++i)
	{
		if (i >= AbilitySlotComponent->MaxSlots) return;

		AbilitySlotComponent->AssignAbilityToSlot(CharacterAbilities[i], i);
	}

	UE_LOG(LogTemp, Log, TEXT("Gave all character abilities"));
}

void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerCharacter, bIsAimingWeapon);
}
