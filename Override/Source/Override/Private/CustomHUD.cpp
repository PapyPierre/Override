#include "CustomHUD.h"
#include "Components/TargetingComponent.h"

void ACustomHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	int32 ViewportX = 0, ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);

	APawn* Pawn = GetOwningPawn();
	if (!Pawn)
	{
		return;
	}

	UTargetingComponent* TargetingComp = Pawn->FindComponentByClass<UTargetingComponent>();
	if (!TargetingComp)
	{
		return;
	}

	/*
	
	float Padding = TargetingComp->ScreenPadding;;
	
	const float MinX = -Padding;
	const float MinY = -Padding;
	const float MaxX = ViewportX + Padding;
	const float MaxY = ViewportY + Padding;

	FLinearColor MaskColor(1.f, 0.f, 0.f, 0.2f);

	// Top
	DrawRect(MaskColor, 0, 0, ViewportX, MinY);

	// Bottom
	DrawRect(MaskColor, 0, MaxY, ViewportX, ViewportY - MaxY);

	// Left
	DrawRect(MaskColor, 0, MinY, MinX, MaxY - MinY);

	// Right
	DrawRect(MaskColor, MaxX, MinY, ViewportX - MaxX, MaxY - MinY);

	*/
}

