#include "Components/RollbackComponent.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Attribute/UHealthAttributeSet.h"
#include "Player/PlayerCharacter.h"
#include "Net/UnrealNetwork.h"

URollbackComponent::URollbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void URollbackComponent::CaptureSnapshot()
{
	APlayerCharacter* Owner = Cast<APlayerCharacter>(GetOwner());
	if (!Owner) return;

	FPlayerSnapshot Snap;
	Snap.PlayerLoc = Owner->GetActorLocation();
	Snap.PlayerRot = Owner->GetActorRotation();

	if (const auto Controller = Owner->GetController()) Snap.ControlRot = Controller->GetControlRotation();
	Snap.Timestamp = GetWorld()->GetTimeSeconds();

	UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent();
	Snap.Health = ASC ? ASC->GetNumericAttribute(UHealthAttributeSet::GetHealthAttribute()) : 3;

	History.Add(Snap);
}

void URollbackComponent::StartRollback()
{
	if (History.IsEmpty()) return;

	bIsRollingBack = true;
	RollbackTargetTime = GetWorld()->GetTimeSeconds();
	GetOwner()->SetActorEnableCollision(false);

	UE_LOG(LogTemp, Log, TEXT("Start Rollback"));
}

void URollbackComponent::StopRollback()
{
	bIsRollingBack = false;
	GetOwner()->SetActorEnableCollision(true);
}

void URollbackComponent::ApplySnapshotOnServer(float TargetTime)
{
	for (int32 i = History.Num() - 1; i > 0; --i)
	{
		if (History[i - 1].Timestamp <= TargetTime && History[i].Timestamp >= TargetTime)
		{
			const float Range = History[i].Timestamp - History[i - 1].Timestamp;
			const float Alpha = Range > 0.f ? (TargetTime - History[i - 1].Timestamp) / Range : 0.f;

			APlayerCharacter* Owner = Cast<APlayerCharacter>(GetOwner());
			if (!Owner) return;

			const FVector NewLoc = FMath::Lerp(History[i - 1].PlayerLoc, History[i].PlayerLoc, Alpha);
			const FRotator NewRot = FMath::Lerp(History[i - 1].PlayerRot, History[i].PlayerRot, Alpha);
			const FRotator NewControlRot = FMath::Lerp(History[i - 1].ControlRot, History[i].ControlRot, Alpha);
			const float NewHP = FMath::Lerp(History[i - 1].Health, History[i].Health, Alpha);

			Owner->SetActorLocationAndRotation(NewLoc, NewRot, false, nullptr,
			                                   ETeleportType::TeleportPhysics);
			Client_ApplySnapshot(NewControlRot);

			if (UAbilitySystemComponent* ASC = Owner->GetAbilitySystemComponent())
				ASC->SetNumericAttributeBase(UHealthAttributeSet::GetHealthAttribute(), NewHP);

			return;
		}
	}
}

void URollbackComponent::Client_ApplySnapshot_Implementation(FRotator NewControlRot)
{
	APlayerCharacter* Owner = Cast<APlayerCharacter>(GetOwner());
	if (!Owner) return;
	 AController* controller = Owner->GetController();
	if (!controller) return;
	Owner->GetController()->SetControlRotation(NewControlRot);
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

		ApplySnapshotOnServer(RollbackTargetTime);
	}
	else
	{
		CaptureSnapshot();
		PurgeOldSnapshots();
	}
}

void URollbackComponent::OnRep_IsRollingBack()
{
	APlayerCharacter* Owner = Cast<APlayerCharacter>(GetOwner());
	if (!Owner) return;

	UE_LOG(LogTemp, Log, TEXT("Rollingback: %i"), bIsRollingBack);

	if (Owner->IsLocallyControlled())
	{
		Owner->GetController()->SetIgnoreLookInput(bIsRollingBack);
		Owner->GetController()->SetIgnoreMoveInput(bIsRollingBack);
	}
	/*
	const ULocalPlayer* Player = Cast<ULocalPlayer>(GetOwner());
	UEnhancedInputLocalPlayerSubsystem* InputSystem = Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	bIsRollingBack
		? InputSystem->RemoveMappingContext(IMC_MouseLook)
		: InputSystem->AddMappingContext(IMC_MouseLook, 0);
	*/
	
	Owner->OnRollingBack(bIsRollingBack);
}

void URollbackComponent::PurgeOldSnapshots()
{
	const float Now = GetWorld()->GetTimeSeconds();
	History.RemoveAll([&](const FPlayerSnapshot& S)
	{
		return (Now - S.Timestamp) > MaxHistoryDuration;
	});
}

void URollbackComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URollbackComponent, bIsRollingBack);
}
