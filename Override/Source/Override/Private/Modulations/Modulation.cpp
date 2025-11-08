#include "Modulations/Modulation.h"
#include "AbilitySystemComponent.h"
#include "Attribute/UHealthAttributeSet.h"
#include "Hacks/GameplayHackTargetData.h"
#include "Kismet/KismetMathLibrary.h"
#include "Modulations/ModulationGroup.h"

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

		//FVector WorldLoc = End.GetLocation() + GetActorTransform().GetLocation();
		End.SetLocation(WorldLoc);
	}

	if (Ends.Num() > 0) CurrentEnd = Ends[CurrentEndIndex];

	Super::BeginPlay();
}

void AModulation::HandleMovement(float DeltaTime)
{
	if (CurrentState != ModState::Moving) return;

	if (ModSpeedCurve == nullptr) return;

	LerpTime += DeltaTime * ModSpeedCurve->FloatCurve.Eval(LerpTime);

	if (LerpTime >= 1.0f) LerpTime = 1;

	SetActorTransform(UKismetMathLibrary::TLerp(CurrentStart, CurrentEnd, LerpTime));

	if (LerpTime < 1) return;

	StopMovement();

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

void AModulation::HandleCooldown(float DeltaTime)
{
	if (CurrentState != ModState::InCD) return;

	CdTime += DeltaTime;

	if (CdTime > CooldownDuration)
	{
		CdTime = 0;

		ChangeState(ModState::Stopped);
	}
}

void AModulation::HandleLock(float DeltaTime)
{
	if (CurrentState != ModState::Locked) return;

	LockTime += DeltaTime;

	if (LockTime > LockDuration)
	{
		LockTime = 0;

		if (PreviousState == ModState::Moving) ChangeState(ModState::Moving);
		else ChangeState(ModState::Stopped);
	}
}

void AModulation::ChangeState(ModState newState)
{
	PreviousState = CurrentState;
	CurrentState = newState;

	if (newState == ModState::InCD) CdTime = 0;
	if (newState == ModState::Locked) LockTime = 0;

	OnStateChanged(newState);
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

	ChangeState(ModState::InCD);
}

void AModulation::ApplyImpulseOnPlayer() const
{
	GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow, TEXT("ApplyImpulseOnPlayer"));

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
		TArray<APlayerCharacter*> LaunchedPlayers;

		for (const FOverlapResult& Result : OverlapResults)
		{
			if (const auto Player = Cast<APlayerCharacter>(Result.GetActor()))
			{
				if (LaunchedPlayers.Contains(Player)) return;

				GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Yellow, TEXT("Player"));

				Player->LaunchCharacter(Dir * ImpulseForce * 100, true, true);
				LaunchedPlayers.Add(Player);
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

void AModulation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleMovement(DeltaTime);
	HandleCooldown(DeltaTime);
	HandleLock(DeltaTime);
	ManageHackCastingCooldown(DeltaTime);
}

void AModulation::Target()
{
	if (Group) Group->TargetGroup();
}

void AModulation::Interact()
{
	if (CurrentState != ModState::Stopped) return;

	if (Group)
	{
		for (AModulation* mod : Group->ModulationsInGroup)
		{
			mod->ChangeState(ModState::Moving);
			mod->Execute_OnInteract(mod);
		}

		return;
	}

	ChangeState(ModState::Moving);
	Execute_OnInteract(this);
}
