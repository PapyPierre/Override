#include "CustomHUD.h"
#include "Components/TargetingComponent.h"

void ACustomHUD::DrawHUD()
{
	Super::DrawHUD();

	int32 ViewportX, ViewportY;
	GetOwningPlayerController()->GetViewportSize(ViewportX, ViewportY);

	if (!PlayerOwner || !PlayerOwner->GetPawn()) return;

	UTargetingComponent* TargetingComp =  PlayerOwner->GetPawn()->FindComponentByClass<UTargetingComponent>();

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
}
