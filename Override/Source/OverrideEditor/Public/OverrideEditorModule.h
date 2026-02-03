#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class OVERRIDEEDITOR_API FOverrideEditorModule : public IModuleInterface
{
public:
	static const FName TabName;
	
	virtual void StartupModule() override;
	
	void AddMenuEntry(FMenuBuilder& Builder);
	void OpenVisualizerTab();

	TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);

	void VisualizeMatch(FString VersionID, FString MatchID, FString PlayerID, FString TeamID);

	virtual void ShutdownModule() override;
};
