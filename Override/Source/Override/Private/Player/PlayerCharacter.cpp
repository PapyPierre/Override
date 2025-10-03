#include "Player/PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Player/CustomPlayerState.h"

APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPlayerMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	if (!PlayerMovementComponent) PlayerMovementComponent = Cast<UPlayerMovementComponent>(GetCharacterMovement());

	PlayerMovementComponent->CharacterRef = this;
	bReplicates = true;
	GetCharacterMovement()->SetIsReplicated(true);
}

void APlayerCharacter::Target() { }

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	if (!PlayerController) PlayerController = Cast<APlayerController>(GetController());

	FirstPersonCameraComponent = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;

	DefaultCoyoteTime = PlayerMovementComponent->CoyoteTime;

	Super::BeginPlay();
}

void APlayerCharacter::Sprint()
{
	if (HasAuthority())
	{
		if (PlayerMovementComponent->CanSprint())
		{
			PlayerMovementComponent->bWantsToSprint = true;
		}
	}
	else
		RPC_SetSprint(true);
}

void APlayerCharacter::RPC_SetSprint_Implementation(bool value)
{
	PlayerMovementComponent->bWantsToSprint = value && PlayerMovementComponent->CanSprint();
}

void APlayerCharacter::StopSprint()
{
	if (HasAuthority())
		PlayerMovementComponent->bWantsToSprint = false;
	else
		RPC_SetSprint(false);
}

void APlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

		GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Yellow, TEXT("PossessedBy"));
	
	InitAbilitySystem(); // Server-side init
}

void APlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

		GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Yellow, TEXT("OnRep_PlayerState"));
	
	InitAbilitySystem(); // Client-side init
}

void APlayerCharacter::Tick(float DeltaTime)
{
	if (HasAuthority())
	{
		CameraShake();

		float Speed = GetVelocity().Size();
		if (PlayerMovementComponent->IsMovingOnGround() && PlayerMovementComponent->IsRunning())
		{
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
	if (HasAuthority())
	{
		FirstPersonCameraComponent->StartCameraShake(ShakeJump, 1.0f, ECameraShakePlaySpace::CameraLocal,
		                                             FRotator::ZeroRotator);
		PlayerMovementComponent->ResetJumpValues();
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

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	/*
	APlayerState* Ps = GetPlayerState();
	if (!Ps) return;
	ACustomPlayerState* CustomPs = Cast<ACustomPlayerState>(Ps);
	if (!CustomPs)
	{
		UE_LOG(LogTemp, Warning, TEXT("No CustomPs"));
		return;
	}
	UAbilitySystemComponent* Asc = CustomPs->GetAbilitySystemComponent();
	
	if (!Asc)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Asc"));
		return;
	}
	
	const FTopLevelAssetPath EnumName("/Source/Override.EHackSlotsEnum");
	FGameplayAbilityInputBinds Binds("ConfirmTargeting", "CancelTargeting", EnumName);
	Asc->BindAbilityActivationToInputComponent(PlayerInputComponent, Binds);
	*/
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
