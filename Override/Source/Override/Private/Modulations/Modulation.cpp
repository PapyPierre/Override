#include "Modulations/Modulation.h"
#include "AbilitySystemComponent.h"
#include "Attribute/UHealthAttributeSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Modulations/ModulationGroup.h"

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

void AModulation::ChangeState(ModState newState)
{
	CurrentState = newState;
	OnStateChanged(newState);
}

void AModulation::StopMovement()
{
	LerpTime = 0;
	ChangeState(ModState::InCD);

	if (ApplyImpulseOnEndReach)
	{
		ApplyImpulseOnPlayer(CurrentEnd.GetLocation() - CurrentStart.GetLocation() * ImpulseForce);
	}
}

void AModulation::ApplyImpulseOnPlayer(FVector Dir)
{
	//TODO Character.AddImpulse(dir, VelocityChange: true)
}

void AModulation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleMovement(DeltaTime);
	HandleCooldown(DeltaTime);
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