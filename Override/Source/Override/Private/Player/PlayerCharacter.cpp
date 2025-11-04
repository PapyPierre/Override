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

	if (PlayerMovementComponent->MovementData)
	{
		DefaultFOV = PlayerMovementComponent->MovementData->DefaultFOV;
		SprintFOV = PlayerMovementComponent->MovementData->SprintFOV;
		FOVInterpSpeed = PlayerMovementComponent->MovementData->FOVInterpSpeed;

		AimFOV = PlayerMovementComponent->MovementData->AimFOV;
		AimCrouchedSpeed = PlayerMovementComponent->MovementData->AimCrouchedSpeed;
		AimSpeed = PlayerMovementComponent->MovementData->AimSpeed;
		MouseSensitivity = PlayerMovementComponent->MovementData->MouseSensitivity;
		MouseAimSensitivity = PlayerMovementComponent->MovementData->MouseAimSensitivity;
	}

	DefaultCoyoteTime = PlayerMovementComponent->CoyoteTime;
}

void APlayerCharacter::Sprint()
{
	if (IsLocallyControlled())
	{
		bool bCan = PlayerMovementComponent->CanSprint();
		PlayerMovementComponent->bWantsToSprint = bCan;
		if (bCan)
		{
			PlayerMovementComponent->MaxWalkSpeed = PlayerMovementComponent->DefaultSprintSpeed;
			RPC_SetSprint(bCan);
		}
		else
		{
			PlayerMovementComponent->MaxWalkSpeed = PlayerMovementComponent->DefaultMaxWalkSpeed;
			RPC_SetSprint(bCan);
		}
	}
}

void APlayerCharacter::RPC_SetSprint_Implementation(bool value)
{
	PlayerMovementComponent->bWantsToSprint = value;
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
	if (IsLocallyControlled())
	{
		FirstPersonCameraComponent->StartCameraShake(ShakeLanding, 1.0f, ECameraShakePlaySpace::CameraLocal,
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
		if (SelectWeaponAction) EnhancedInput->BindAction(SelectWeaponAction, ETriggerEvent::Started, this,
		                                                  &APlayerCharacter::UnselectHack);

		if (SelectHack1Action) EnhancedInput->BindAction(SelectHack1Action, ETriggerEvent::Started, this,
		                                                 &APlayerCharacter::SelectHack1);

		if (SelectHack2Action) EnhancedInput->BindAction(SelectHack2Action, ETriggerEvent::Started, this,
		                                                 &APlayerCharacter::SelectHack2);

		if (SelectHack3Action) EnhancedInput->BindAction(SelectHack3Action, ETriggerEvent::Started, this,
		                                                 &APlayerCharacter::SelectHack3);

		if (HackAction) EnhancedInput->BindAction(HackAction, ETriggerEvent::Started, this,
		                                          &APlayerCharacter::LaunchSelectedHack);
	}
}

bool APlayerCharacter::IsHackSelected() const
{
	return SelectedHackIndex != 0;
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

void APlayerCharacter::LaunchSelectedHack()
{
	switch (SelectedHackIndex)
	{
	case 1:
		ActivateHack1();
		break;
	case 2:
		ActivateHack2();
		break;
	case 3:
		ActivateHack3();
		break;
	default:
		return;
	}
}

void APlayerCharacter::SelectHack1()
{
	if (SelectedHackIndex == 1) return;

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Select hack 1"));

	SelectedHackIndex = 1;
}

void APlayerCharacter::SelectHack2()
{
	if (SelectedHackIndex == 2) return;

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Select hack 2"));

	SelectedHackIndex = 2;
}

void APlayerCharacter::SelectHack3()
{
	if (SelectedHackIndex == 3) return;

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Select hack 3"));

	SelectedHackIndex = 3;
}

void APlayerCharacter::UnselectHack()
{
	if (SelectedHackIndex == 0) return;

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Unselect"));

	SelectedHackIndex = 0;
}

ACustomPlayerState* APlayerCharacter::GetCustomPlayerState() const
{
	return GetPlayerState<ACustomPlayerState>();
}

void APlayerCharacter::ActivateHack1()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Activate hack 1"));

	SendHackEventWithData(Hack1Tag);
}

void APlayerCharacter::ActivateHack2()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Activate hack 2"));

	SendHackEventWithData(Hack2Tag);
}

void APlayerCharacter::ActivateHack3()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Activate hack 3"));

	SendHackEventWithData(Hack3Tag);
}

void APlayerCharacter::SendHackEventWithData(FGameplayTag EventTag)
{
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("SendHackEventWithData"));

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

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddOrigin(TargetingComponent->PointInSight);
	EventData.ContextHandle = ContextHandle;
	
	FGameplayHackTargetData* HackTargetData = new FGameplayHackTargetData();

	if (TargetingComponent->CurrentTargets.Num() > 0)
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
