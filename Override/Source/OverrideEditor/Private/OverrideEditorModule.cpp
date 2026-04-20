#include "OverrideEditorModule.h"

#include <string>

#include "MatchActor.h"
#include "LevelEditor.h"
#include "STchoupiVisualizerWidget.h"
#include "GameMode/MatchData.h"
#include "Network/FMatchDataFetcher.h"
#include "Network/ServerHttpClient.h"

IMPLEMENT_MODULE(FOverrideEditorModule, OverrideEditor)

void FOverrideEditorModule::StartupModule()
{
	HttpClient = NewObject<UServerHttpClient>();
	
	// Register tab in tab manager
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		                        TabName, FOnSpawnTab::CreateRaw(this, &FOverrideEditorModule::SpawnTab))
	                        .SetDisplayName(FText::FromString(TabName.ToString()))
	                        .SetMenuType(ETabSpawnerMenuType::Hidden);

	FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	// Add a menu entry in UE's Editor layout
	TSharedPtr<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddMenuExtension(
		"WindowLayout",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FOverrideEditorModule::AddMenuEntry)
	);

	LevelEditor.GetMenuExtensibilityManager()->AddExtender(Extender);
}

// When the given menu entry is clicked, invoke the tab
void FOverrideEditorModule::AddMenuEntry(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(
		FText::FromString("Tchoupi Visualizer"),
		FText::FromString("Load and draw player trajectories of a given match"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FOverrideEditorModule::OpenVisualizerTab))
	);
}

// Notify the tab manager to open the given tab
void FOverrideEditorModule::OpenVisualizerTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(TabName);
}

// Create the given widget corresponding to the tab
TSharedRef<SDockTab> FOverrideEditorModule::SpawnTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(STchoupiVisualizerWidget)
			.EditorModule(this)
		];
}

const FName FOverrideEditorModule::TabName("Tchoupi Visualizer");

void FOverrideEditorModule::RequestData(IHttpRequester* Requester,
	FString VersionID, FString MatchID, FString PlayerID, FString TeamID)
{
	UE_LOG(LogTemp, Log, TEXT("Trying to fetch data from DB"));

	TArray<FMatchPlayerData> MatchPlayersData;

	HttpClient->FetchMatchesData(Requester, VersionID, MatchID, PlayerID, TeamID);
}

void FOverrideEditorModule::ShowVisualization(const TArray<FMatchData>& MatchesData,
	const TSharedPtr<FString>& SelectedVersionId, const TSharedPtr<FString>& SelectedMatchId,
	const TSharedPtr<FString>& SelectedTeamId, const TSharedPtr<FString>& SelectedPlayerId, const bool SeeThrough,
	const float MinTimeValue, const float MaxTimeValue)
{
	UE_LOG(LogTemp, Log, TEXT("Starting visualization"));
	
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	for (const FMatchData MatchData : MatchesData)
	{
		if (*SelectedVersionId != TEXT("All") && *SelectedVersionId != TEXT(""))
		{
			if (*SelectedVersionId != MatchData.Version) continue;
		}
		
		if (*SelectedMatchId != TEXT("All") && *SelectedMatchId != TEXT(""))
		{
			if (*SelectedMatchId != MatchData.Id) continue;
		}
		
		UE_LOG(LogTemp, Log, TEXT("Trying to visualizing data of match %s"), *MatchData.Id);
		
		AMatchActor* MatchActor = World->SpawnActor<AMatchActor>();

		TArray<FMatchPlayerData> MatchPlayers;
		
		bool bFilterPlayer = (*SelectedPlayerId != TEXT("All") && *SelectedPlayerId != TEXT(""));
		bool bFilterTeam   = (*SelectedTeamId   != TEXT("All") && *SelectedTeamId   != TEXT(""));

		if (bFilterPlayer || bFilterTeam)
		{
			for (FMatchPlayerData Player : MatchData.Players)
			{
				if (bFilterPlayer && FString::FromInt(Player.PlayerId) != *SelectedPlayerId) continue;
				if (bFilterTeam && FString::FromInt(Player.TeamId) != *SelectedTeamId) continue;
				
				MatchPlayers.Add(Player);
			}
		}
		else
		{
			MatchPlayers = MatchData.Players;
		}
		
		MatchActor->Players = MatchPlayers;
		MatchActor->SeeThrough = SeeThrough;
		MatchActor->MinTimeValue = MinTimeValue;
		MatchActor->MaxTimeValue = MaxTimeValue;
		MatchActor->RerunConstructionScripts();
		SpawnedActors.Add(MatchActor);
	}
}

void FOverrideEditorModule::ClearVisualization()
{
	for (AMatchActor* MatchActor : SpawnedActors)
	{
		MatchActor->Clear();
		MatchActor->Destroy();
	}

	SpawnedActors.Empty();
}

void FOverrideEditorModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}
