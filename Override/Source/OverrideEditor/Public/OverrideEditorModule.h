#pragma once

#include "CoreMinimal.h"
#include "GameMode/MatchData.h"
#include "GameMode/MatchPlayerData.h"
#include "Modules/ModuleManager.h"

class IHttpRequester;
class STchoupiVisualizerWidget;
class UServerHttpClient;
class AMatchActor;

class OVERRIDEEDITOR_API FOverrideEditorModule : public IModuleInterface
{
public:
	static const FName TabName;

	TArray<AMatchActor*> SpawnedActors;

	virtual void StartupModule() override;
	
	void AddMenuEntry(FMenuBuilder& Builder);
	void OpenVisualizerTab();

	TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);

	void RequestData(IHttpRequester* Requester,
		FString VersionID  = TEXT(""), FString MatchID  = TEXT(""), FString PlayerID  = TEXT(""), FString TeamID = TEXT(""));

	void ShowVisualization(const TArray<FMatchData>& MatchesData,
	const TSharedPtr<FString>& SelectedVersionId, const TSharedPtr<FString>& SelectedMatchId,
	const TSharedPtr<FString>& SelectedTeamId, const TSharedPtr<FString>& SelectedPlayerId,
	const bool SeeThrough, const float MinTimeValue, const float MaxTimeValue);

	void ClearVisualization();

	virtual void ShutdownModule() override;

private:
	UPROPERTY()
	TObjectPtr<UServerHttpClient> HttpClient;
};
