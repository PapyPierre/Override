#include "Components/RollbackComponent.h"

#include "AbilitySystemComponent.h"
#include "Attribute/UHealthAttributeSet.h"
#include "GameFramework/Character.h"
#include "Player/PlayerCharacter.h"

URollbackComponent::URollbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URollbackComponent::CaptureSnapshot()
{
	APlayerCharacter* Owner = Cast<APlayerCharacter>(GetOwner());
	if (!Owner) return;

	FPlayerSnapshot Snap;
	Snap.Location = Owner->GetActorLocation();
	Snap.Rotation = Owner->GetActorRotation();
	Snap.Timestamp = GetWorld()->GetTimeSeconds();
	Snap.Health = Owner->GetAbilitySystemComponent()->GetNumericAttribute(UHealthAttributeSet::GetHealthAttribute());

	History.Add(Snap);
}

void URollbackComponent::StartRollback()
{
	if (History.IsEmpty()) return;

	bIsRollingBack = true;
	RollbackTargetTime = GetWorld()->GetTimeSeconds();
	GetOwner()->SetActorEnableCollision(false);
}

void URollbackComponent::StopRollback()
{
	bIsRollingBack = false;
	GetOwner()->SetActorEnableCollision(true);
}

void URollbackComponent::ApplySnapshot(float TargetTime)
{
	for (int32 i = History.Num() - 1; i > 0; --i)
	{
		if (History[i - 1].Timestamp <= TargetTime && History[i].Timestamp >= TargetTime)
		{
			const float Range = History[i].Timestamp - History[i - 1].Timestamp;
			const float Alpha = Range > 0.f ? (TargetTime - History[i - 1].Timestamp) / Range : 0.f;

			APlayerCharacter* Owner = Cast<APlayerCharacter>(GetOwner());
			if (!Owner) return;

			const FVector NewLoc = FMath::Lerp(History[i - 1].Location, History[i].Location, Alpha);
			const FRotator NewRot = FMath::Lerp(History[i - 1].Rotation, History[i].Rotation, Alpha);
			const float NewHP = FMath::Lerp(History[i - 1].Health, History[i].Health, Alpha);

			Owner->SetActorLocationAndRotation(NewLoc, NewRot, false, nullptr,
			                                   ETeleportType::TeleportPhysics);
			
			if (UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent())
				ASC->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), NewHP);

			return;
		}
	}
}

void URollbackComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsRollingBack)
	{
		RollbackTargetTime -= DeltaTime * RollbackSpeed;

		const float OldestTime = History[0].Timestamp;
		if (RollbackTargetTime <= OldestTime)
		{
			RollbackTargetTime = OldestTime;
			StopRollback();
		}

		ApplySnapshot(RollbackTargetTime);
	}
	else
	{
		CaptureSnapshot();
		PurgeOldSnapshots();
	}
}

void URollbackComponent::PurgeOldSnapshots()
{
	const float Now = GetWorld()->GetTimeSeconds();
	History.RemoveAll([&](const FPlayerSnapshot& S)
	{
		return (Now - S.Timestamp) > MaxHistoryDuration;
	});
}
