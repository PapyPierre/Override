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
	
	float radius = TargetingComp->MaxDistFromCursor / 2;
	
	const float Step = 2 * PI / 32;
	FVector2D PrevPoint(ViewportX/2 + radius, ViewportY/2);

	for (int32 i = 1; i <= 32; i++)
	{
		const float Angle = i * Step;
		FVector2D NextPoint(
			ViewportX/2 + radius * FMath::Cos(Angle),
			ViewportY/2 + radius * FMath::Sin(Angle)
		);

		DrawLine(PrevPoint.X, PrevPoint.Y, NextPoint.X, NextPoint.Y, FLinearColor::White, 1.5f);
		PrevPoint = NextPoint;
	}
}

