#include "OverrideEditorModule.h"
#include "FMatchDataFetcher.h"
#include "MatchActor.h"
#include "LevelEditor.h"
#include "STchoupiVisualizerWidget.h"

IMPLEMENT_MODULE(FOverrideEditorModule, OverrideEditor)

void FOverrideEditorModule::StartupModule()
{
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

void FOverrideEditorModule::UpdateLists(TArray<TSharedPtr<FString>>& VersionIds, TArray<TSharedPtr<FString>>& MatchIds)
{
	VersionIds.Empty();
	MatchIds.Empty();
	
	VersionIds.Add(MakeShared<FString>("All"));
	MatchIds.Add(MakeShared<FString>("All"));
	
	FMatchDataFetcher MatchDataFetcher;
	MatchDataFetcher.FetchMatchList(VersionIds, MatchIds);
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

void FOverrideEditorModule::VisualizeMatch(FString VersionID, FString MatchID, FString PlayerID, FString TeamID, bool SeeThrough)
{
	UE_LOG(LogTemp, Log, TEXT("Trying to visualizing players positions of match %s"), *MatchID);

	TArray<FMatchPlayerData> MatchPlayersData;

	FMatchDataFetcher MatchDataFetcher;
	MatchDataFetcher.FetchMatch(VersionID, MatchID, PlayerID, TeamID, MatchPlayersData);

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return;

	AMatchActor* MatchActor = World->SpawnActor<AMatchActor>();
	MatchActor->Players = MatchPlayersData;
	MatchActor->SeeThrough = SeeThrough;
	MatchActor->RerunConstructionScripts();
	SpawnedActors.Add(MatchActor);
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
