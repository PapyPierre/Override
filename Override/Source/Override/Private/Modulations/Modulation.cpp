#include "Modulations/Modulation.h"
#include "AbilitySystemComponent.h"
#include "Attribute/UHealthAttributeSet.h"
#include "Abilities/GameplayHackTargetData.h"
#include "Kismet/KismetMathLibrary.h"
#include "Modulations/ModulationGroup.h"
#include <Net/UnrealNetwork.h>

struct FGameplayHackTargetData;

AModulation::AModulation()
{
	PrimaryActorTick.bCanEverTick = true;
	HealthSet = CreateDefaultSubobject<UHealthAttributeSet>(TEXT("HealthSet"));
	check(HealthSet);
	Asc = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	check(Asc);
}

UAbilitySystemComponent* AModulation::GetAbilitySystemComponent() const
{
	return Asc;
}

void AModulation::BeginPlay()
{
	Start = GetActorTransform();
	CurrentStart = Start;

	for (FTransform& End : Ends)
	{
		FVector WorldLoc = GetActorTransform().TransformPosition(End.GetLocation());
		End.SetLocation(WorldLoc);
	}

	if (Ends.Num() > 0) CurrentEnd = Ends[0];

	Super::BeginPlay();
}

void AModulation::UpdateCurrentEnd()
{
	CurrentEndIndex++;

	if (CurrentEndIndex > Ends.Num() - 1)
	{
		CurrentEndIndex = -1;
		CurrentEnd = Start;
		if (Ends.Num() > 0) CurrentStart = Ends.Last();
	}
	else
	{
		CurrentEnd = Ends[CurrentEndIndex];
		if (CurrentEndIndex == 0) CurrentStart = Start;
		else CurrentStart = Ends[CurrentEndIndex - 1];
	}
}

void AModulation::HandleMovement(float DeltaTime)
{
	if (CurrentState != ModState::Moving || ModSpeedCurve == nullptr) return;

	const float Speed = ModSpeedCurve->FloatCurve.Eval(LerpTime);
	LerpTime = FMath::Min(LerpTime + DeltaTime * Speed, 1.f);
	
	SetActorTransform(UKismetMathLibrary::TLerp(CurrentStart, CurrentEnd, LerpTime), true);
	
	if (LerpTime < 1) return;

	StopMovement();
	UpdateCurrentEnd();
}

void AModulation::HandleCooldown(float DeltaTime)
{
	if (CurrentState != ModState::InCD) return;

	CdTime += DeltaTime;

	if (CdTime > CooldownDuration)
	{
		CdTime = 0;

		RPC_ChangeState(ModState::Stopped);
	}
}

void AModulation::HandleLock(float DeltaTime)
{
	if (CurrentState != ModState::Locked) return;

	LockTime += DeltaTime;

	if (LockTime > LockDuration)
	{
		LockTime = 0;

		if (PreviousState == ModState::Moving) RPC_ChangeState(ModState::Moving);
		else RPC_ChangeState(ModState::Stopped);
	}
}

void AModulation::RPC_ChangeState_Implementation(ModState NewState)
{
	PreviousState = CurrentState;
	CurrentState = NewState;

	if (NewState == ModState::InCD) CdTime = 0;
	if (NewState == ModState::Locked) LockTime = 0;

	OnStateChanged(NewState);
}

void AModulation::Lock_Implementation()
{
	if (CurrentState == ModState::Locked)return;
	if (CurrentState == ModState::InCD) return;

	if (Group)
	{
		for (AModulation* mod : Group->ModulationsInGroup)
		{
			mod->RPC_ChangeState(ModState::Locked);
		}

		return;
	}

	RPC_ChangeState(ModState::Locked);
}

void AModulation::StartCastingGE(TSubclassOf<UGameplayEffect> GameplayEffect, float CastDuration)
{
	if (HackCastingDuration != 0) return; // Already casting an ability

	CastingTime = 0;
	CurrentlyCastedGE = GameplayEffect;
	HackCastingDuration = CastDuration;
}

void AModulation::StopMovement()
{
	LerpTime = 0;

	if (ApplyImpulseOnEndReach)
	{
		ApplyImpulseOnPlayer();
	}

	RPC_ChangeState(ModState::InCD);
}

void AModulation::ApplyImpulseOnPlayer() const
{
	//GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Yellow, TEXT("ApplyImpulseOnPlayer"));

	const FVector Dir = (CurrentEnd.GetLocation() - CurrentStart.GetLocation()).GetSafeNormal();

	//DrawDebugLine(GetWorld(), CurrentStart.GetLocation(), CurrentEnd.GetLocation(), FColor::Red, false, 2);

	FVector Origin;
	FVector Extent;

	GetActorBounds(true, Origin, Extent, false);

	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_GameTraceChannel1); // equal to ECC_Targetable (custom obj type)
	const FCollisionShape Shape = FCollisionShape::MakeBox(Extent);

	bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(OverlapResults, GetActorLocation() + Dir * Extent.Length(),
	                                                        FQuat::Identity, ObjectQueryParams, Shape);

	//DrawDebugBox(GetWorld(), GetActorLocation() + Dir * Extent.Length(), Extent, FColor::Yellow, false, 10);

	if (bHasOverlap)
	{
		//TArray<APlayerCharacter*> LaunchedPlayers;

		for (const FOverlapResult& Result : OverlapResults)
		{
			if (const auto Player = Cast<APlayerCharacter>(Result.GetActor()))
			{
				//GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Yellow, TEXT("Player"));

				//if (LaunchedPlayers.Contains(Player)) return;

				Player->Launch(Dir * ImpulseForce * 10);
				//LaunchedPlayers.Add(Player);
			}
		}
	}
}

void AModulation::ManageHackCastingCooldown(float DeltaTime)
{
	if (HackCastingDuration == 0) return; // Is not currently casting an ability

	CastingTime += DeltaTime;

	if (CastingTime >= HackCastingDuration)
	{
		CurrentlyCastedGE = nullptr;
		CastingTime = 0;
		HackCastingDuration = 0;
	}
}

void AModulation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(AModulation, CurrentState);
	DOREPLIFETIME(AModulation, CurrentStart);
	DOREPLIFETIME(AModulation, CurrentEnd);
	DOREPLIFETIME(AModulation, LerpTime);
}

void AModulation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleMovement(DeltaTime);
	HandleCooldown(DeltaTime);
	HandleLock(DeltaTime);
	ManageHackCastingCooldown(DeltaTime);
}

void AModulation::OnTarget_Implementation(AActor* TargetingActor)
{
	if (Group) Group->TargetGroup(TargetingActor);
}

void AModulation::OnInteract_Implementation(AActor* InteractingActor) // Server-side
{
	if (CurrentState != ModState::Stopped) return;

	if (Group)
	{
		for (AModulation* Mod : Group->ModulationsInGroup)
		{
			Mod->RPC_ChangeState(ModState::Moving);
		}

		return;
	}

	RPC_ChangeState(ModState::Moving);
}
