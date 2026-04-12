#pragma once

#include "CoreMinimal.h"
#include "Network/HttpRequester.h"

struct FMatchData;
class FOverrideEditorModule;

class OVERRIDEEDITOR_API STchoupiVisualizerWidget : public SCompoundWidget, public IHttpRequester
{
public:
	SLATE_BEGIN_ARGS(STchoupiVisualizerWidget) {}
		SLATE_ARGUMENT(class FOverrideEditorModule*, EditorModule)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void UpdateLists(const TArray<TSharedPtr<FString>>& Versions, const TArray<TSharedPtr<FString>>& Matches);

private:
	virtual void OnMatchesDataReceived(TArray<FMatchData> MatchesData) override;

	TArray<FMatchData> CachedMatchesData;
	
	TArray<TSharedPtr<FString>> VersionIds;
	TSharedPtr<FString> SelectedVersionId;
	
	TArray<TSharedPtr<FString>> MatchIds;
	TSharedPtr<FString> SelectedMatchId;

	TArray<TSharedPtr<FString>> PlayerIds;
	TSharedPtr<FString> SelectedPlayerId;

	TArray<TSharedPtr<FString>> TeamIds;
	TSharedPtr<FString> SelectedTeamId;

	bool SeeThrough = false;

	float SliderValue = 1;

	TSharedRef<SWidget> GenerateVersionItem(TSharedPtr<FString> Item);
	void OnVersionSelected(TSharedPtr<FString> Item, ESelectInfo::Type);

	TSharedRef<SWidget> GenerateMatchItem(TSharedPtr<FString> Item);
	void OnMatchSelected(TSharedPtr<FString> Item, ESelectInfo::Type);

	TSharedRef<SWidget> GeneratePlayerItem(TSharedPtr<FString> Item);
	void OnPlayerSelected(TSharedPtr<FString> Item, ESelectInfo::Type);

	TSharedRef<SWidget> GenerateTeamItem(TSharedPtr<FString> Item);
	void OnTeamSelected(TSharedPtr<FString> Item, ESelectInfo::Type);
	
	FReply OnVisualizeClicked();

	FReply OnClearClicked();

	FReply OnUpdateClicked();

	void OnSeeThroughChecked(ECheckBoxState CheckBoxState);
	
	void OnSliderValueChanged(float NewValue);

	FOverrideEditorModule* EditorModule = nullptr;
};
