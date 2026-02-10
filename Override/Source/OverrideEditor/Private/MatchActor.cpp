#include "OverrideEditor/Public/MatchActor.h"

#include "GameMode/MatchPlayerData.h"


void AMatchActor::OnConstruction(const FTransform& Transform)
{
	if (!GetWorld()) return;

	FlushPersistentDebugLines(GetWorld());
	
	auto team1Count = 0;
    auto team2Count = 0;
    auto team3Count = 0;

	for (const FMatchPlayerData& Player : Players)
	{
		FColor Color;

		switch (Player.TeamId)
		{
		case 0:
			Color = team1Count == 0 ? FColor::Blue : FColor::Cyan;
			team1Count++;
			break;
		case 1:
			Color = team2Count == 0 ? FColor::Magenta : FColor::Red;
			team2Count++;
			break;
		case 2:
			Color = team3Count == 0 ? FColor::Orange : FColor::Yellow;
			team3Count++;
			break;
		default:
			Color = FColor::Black;
			break;
		}

		int Depth = SeeThrough ? 100 : 0;

		for (int32 i = 1; i < Player.Positions.Num(); ++i)
		{
			FVector prevPos = Player.Positions[i - 1].Position;
			FVector pos = Player.Positions[i].Position;


			if (FVector::Dist(prevPos, pos) < 100)
			{
				DrawDebugLine(
					GetWorld(),
					prevPos,
					pos,
					Color,
					true,
					-1.f,
					Depth,
					8
				);
			}
			
			DrawDebugPoint(
				GetWorld(),
				Player.Positions[i].Position,
				12.f,
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
