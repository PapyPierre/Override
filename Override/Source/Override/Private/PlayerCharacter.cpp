#include "PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"

APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UPlayerMovementComponent>(ACharacter::CharacterMovementComponentName)){
	PrimaryActorTick.bCanEverTick = true;

	if (!PlayerMovementComponent) PlayerMovementComponent = Cast<UPlayerMovementComponent>(GetCharacterMovement());
	
	PlayerMovementComponent->CharacterRef = this;
	bReplicates = true;
	GetCharacterMovement()->SetIsReplicated(true);
}

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
	if (IsLocallyControlled())
	{
		bool bCan = PlayerMovementComponent->CanSprint();
		PlayerMovementComponent->bWantsToSprint = bCan;

		RPC_SetSprint(bCan);
	}
}

void APlayerCharacter::RPC_SetSprint_Implementation(bool value)
{
	PlayerMovementComponent->bWantsToSprint = value && PlayerMovementComponent->CanSprint();
}

void APlayerCharacter::StopSprint()
{
	if (IsLocallyControlled())
	{
		PlayerMovementComponent->bWantsToSprint = false;
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
	return true; // tu peux ajouter une validation plus avancée si nécessaire
}

void APlayerCharacter::ServerSetAim_Implementation(bool bNewAiming)
{
	SetAimingState(bNewAiming);
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	if (IsLocallyControlled())
	{
		CameraShake();
	
		if (PlayerMovementComponent->IsMovingOnGround() && PlayerMovementComponent->IsRunning())
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
		
		else if (!FMath::IsNearlyEqual(FirstPersonCameraComponent->GetFOVAngle(), DefaultFOV) && !bIsAimingWeapon)
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
			FirstPersonCameraComponent->StartCameraShake(ShakeIdle, 1.0f, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
		else
		{
			if (PlayerMovementComponent->IsRunning() && GetVelocity().Size() > PlayerMovementComponent->DefaultSprintSpeed)
				FirstPersonCameraComponent->StartCameraShake(ShakeRunning, 1.0f, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
			else if (!PlayerMovementComponent->IsSliding() && !PlayerMovementComponent->IsCrouching())
				FirstPersonCameraComponent->StartCameraShake(ShakeWalk, 1.0f, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
		}
	}
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (IsLocallyControlled())
	{
		FirstPersonCameraComponent->StartCameraShake(ShakeJump, 1.0f, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
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
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerCharacter, bIsAimingWeapon);
}

