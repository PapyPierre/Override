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
	if (ACustomPlayerState* PS = GetCustomPlayerState())
	{
		return PS->GetAbilitySystemComponent();
	}

	UE_LOG(LogTemp, Warning, TEXT("ACustomPlayerState not found"));

	return nullptr;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (PlayerMovementComponent->MovementData)
	{
		DefaultFOV = PlayerMovementComponent->MovementData->DefaultFOV;
		WalkFOV = PlayerMovementComponent->MovementData->WalkFOV;
		SlideFOV = PlayerMovementComponent->MovementData->SlideFOV;
		DashFOV = PlayerMovementComponent->MovementData->DashFOV;		

		AimFOV = PlayerMovementComponent->MovementData->AimFOV;
		AimCrouchedSpeed = PlayerMovementComponent->MovementData->AimCrouchedSpeed;
		AimSpeed = PlayerMovementComponent->MovementData->AimSpeed;
		DefaultMouseSensitivity = PlayerMovementComponent->MovementData->MouseSensitivity;
		CurrentMouseSensitivity = DefaultMouseSensitivity;
		MouseAimSensitivity = PlayerMovementComponent->MovementData->MouseAimSensitivity;
	}

	CameraManager = NewObject<UCameraManager>(this);
	CameraManager->BeginPlay(this);
	
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
		CurrentMouseSensitivity = MouseAimSensitivity;
		PlayerMovementComponent->MaxWalkSpeedCrouched = AimCrouchedSpeed;
		PlayerMovementComponent->MaxWalkSpeed = AimSpeed;
	}
	else
	{
		CurrentMouseSensitivity = DefaultMouseSensitivity;
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

	UE_LOG(LogTemp, Log, TEXT("On possessed by %s"), *NewController->GetName());

	// Server-side
	SetControllerRef();

	if (PlayerController && IsLocallyControlled()m)
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
		CameraManager->SetFov(this, PlayerMovementComponent, DeltaTime);
	}

	Super::Tick(DeltaTime);
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (PlayerMovementComponent->bResetSlideCrouch)
		PlayerMovementComponent->bResetSlideLanded = true;
	
	if (!PlayerMovementComponent->JumpTimeline.IsPlaying())
	{
		PlayerMovementComponent->JumpTimeline.PlayFromStart();
	}
	else
	{
		PlayerMovementComponent->JumpTimeline.Stop();
		PlayerMovementComponent->JumpTimeline.PlayFromStart();
	}

	FVector CurrentVelocity = PlayerMovementComponent->Velocity;
	FVector HorizontalVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.f);
	float HorizontalSpeed = GetVelocity().Size();

	if (PlayerMovementComponent->bIsSliding && PlayerMovementComponent->bWantsToCrouch && HorizontalSpeed > 1500.f)
	{
		float PreservePercent = 1.5f;
		FVector NewHorizontalVelocity = HorizontalVelocity.GetSafeNormal() * (HorizontalSpeed * PreservePercent);
		PlayerMovementComponent->Velocity += NewHorizontalVelocity;
	}

	if (IsLocallyControlled() && FirstPersonCameraComponent)
	{
		FirstPersonCameraComponent->StartCameraShake(ShakeLanding, 1.0f, ECameraShakePlaySpace::CameraLocal,
		                                             FRotator::ZeroRotator);
	}
}

void APlayerCharacter::Falling()
{
	if (!PlayerMovementComponent->bIsDashing)
	{
		JumpCurrentCount--;
	}
	
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
	Super::Jump();

	if (IsLocallyControlled())
		FirstPersonCameraComponent->StartCameraShake(ShakeJump, 1.0f, ECameraShakePlaySpace::CameraLocal,
		                                             FRotator::ZeroRotator);
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
	UE_LOG(LogTemp, Log, TEXT("Init Ability System"));

	if (ACustomPlayerState* PS = GetCustomPlayerState())
	{
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
		{
			ASC->InitAbilityActorInfo(PS, this);
		}
	}

	AbilitySlotComponent->Init();

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

				FGameplayAbilitySpecHandle Handle; 
				bool bIsInstance = true;
				
				for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
				{
					if (Spec.InputID == index)
					{
						Handle = Spec.Handle;
						break;
					}
				}
				
				const UGameplayAbility* GameplayAbility =
					UAbilitySystemBlueprintLibrary::GetGameplayAbilityFromSpecHandle(ASC, Handle, bIsInstance);
				const UGA_BaseAbility* BaseAbility = Cast<UGA_BaseAbility>(GameplayAbility);
				
				if (!BaseAbility)
				{
					UE_LOG(LogTemp, Error, TEXT("BaseAbility not found!"));
					return;
				}

				if (TargetingComponent->CurrentTargets.IsEmpty() && BaseAbility->RequiresTargets) return;
				
				ActivateAbilityInSlotRPC(index, TargetingComponent->GetPointInSight(),
				                         TargetingComponent->CurrentTargets);

				OnAbilityActivated(index, BaseAbility->SelfCast);
			}
		}
	}
}

void APlayerCharacter::ActivateAbilityInSlotRPC_Implementation(int32 SlotIndex, FVector CurrentPointInSight,
                                                               const TArray<AActor*>& Targets)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	if (!ASC || !AbilitySlotComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Missing ASC or AbilitySlotComponent"));
		return;
	}

	FGameplayAbilitySpec* AbilitySpec = nullptr;

	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.InputID == SlotIndex)
		{
			AbilitySpec = &Spec;
			break;
		}
	}

	if (!AbilitySpec)
	{
		UE_LOG(LogTemp, Warning, TEXT("SERVER : No ability found in slot %d"), SlotIndex);
		return;
	}

	FCustomAbilityTargetData* TargetData = new FCustomAbilityTargetData();

	if (Targets.Num() > 0)
	{
		for (const AActor* Target : Targets)
		{
			if (Target)
			{
				TargetData->Targets.Add(const_cast<AActor*>(Target));
			}
		}
	}

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	TargetDataHandle.Add(TargetData);

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddOrigin(CurrentPointInSight);
	if (!AbilitySpec->GameplayEventData.IsValid())
	{
		AbilitySpec->GameplayEventData = MakeShared<FGameplayEventData>();
		//UE_LOG(LogTemp, Log, TEXT("SERVER : Created new GameplayEventData"));
	}

	AbilitySpec->GameplayEventData->Instigator = this;
	AbilitySpec->GameplayEventData->Target = this;
	AbilitySpec->GameplayEventData->TargetData = TargetDataHandle;
	AbilitySpec->GameplayEventData->ContextHandle = ContextHandle;

	ASC->TryActivateAbility(AbilitySpec->Handle);
}

void APlayerCharacter::GiveCharacterAbilities(TArray<TSubclassOf<UGA_BaseAbility>> Abilities)
{
	if (!GetAbilitySystemComponent() || !HasAuthority()) return;

	for (int i = 0; i < Abilities.Num(); ++i)
	{
		UE_LOG(LogTemp, Log, TEXT("Gave ability: %s"), *Abilities[i]->GetName());

		if (i >= AbilitySlotComponent->MaxSlots) return;

		AbilitySlotComponent->AssignAbilityToSlot(Abilities[i], i);
	}

	UE_LOG(LogTemp, Log, TEXT("Gave all character abilities"));
}

void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerCharacter, bIsAimingWeapon);
}
