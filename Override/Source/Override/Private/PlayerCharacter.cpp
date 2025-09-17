// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"
#include "Engine/Engine.h"
#include "GameFramework/GameModeBase.h"

APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPlayerMovementComponent>(ACharacter::CharacterMovementComponentName))
{
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
	{
		RPC_SetSprint(true);
	}
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

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	if (!PlayerMovementComponent->IsMovingOnGround()) {
		FVector TraceStart = GetActorLocation();
		FVector TraceEnd = GetActorLocation() + GetActorRightVector() * 200;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		GetWorld()->LineTraceSingleByChannel(WallRunHitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams);

		if (WallRunHitResult.bBlockingHit) {
			PlayerMovementComponent->CloseToWall = true;
		}
		else
			PlayerMovementComponent->CloseToWall = false;
	}

	else
		PlayerMovementComponent->CloseToWall = false;
	
	Super::Tick(DeltaTime);
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	PlayerMovementComponent->bHasResetWallRide = true;
}

bool APlayerCharacter::CanJumpInternal_Implementation() const
{
	return JumpIsAllowedInternal();
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

