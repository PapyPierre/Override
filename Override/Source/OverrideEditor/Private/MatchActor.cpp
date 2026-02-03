
#include "OverrideEditor/Public/MatchActor.h"

#include "GameMode/MatchPlayerData.h"


void AMatchActor::OnConstruction(const FTransform& Transform)
{
	if (!GetWorld()) return;

	FlushPersistentDebugLines(GetWorld());

	for (const FMatchPlayerData& Player : Players)
	{
		FColor Color;

		switch (Player.TeamId)
		{
		case 0:
			Color = FColor::Blue;
			break;
		case 1:
			Color = FColor::Magenta;
			break;
		case 2:
			Color = FColor::Orange;
			break;
		default:
			Color = FColor::Black;
			break;
		}

		for (int32 i = 1; i < Player.Positions.Num(); ++i)
		{
			DrawDebugLine(
				GetWorld(),
				Player.Positions[i - 1].Position,
				Player.Positions[i].Position,
				Color,
				true,
				-1.f,
				0,
				3.f
			);

			DrawDebugPoint(
				GetWorld(),
				Player.Positions[i].Position,
				8.f,
				Color,
				true
			);
		}
	}
}

void AMatchActor::Clear()
{
	Players.Empty();
	if (GetWorld())
	{
		FlushPersistentDebugLines(GetWorld());
	}
}
