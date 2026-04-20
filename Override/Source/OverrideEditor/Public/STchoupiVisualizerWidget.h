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

	void UpdateLists(const TArray<FString>& Versions, const TArray<FString>& Matches);

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

	float RangeMin = 0.f;
	float RangeMax = 1.f;

	float GetRangeMin() const { return RangeMin; }
	float GetRangeMax() const { return RangeMax; }
	
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

	FReply OnFetchDataClicked();

	void OnSeeThroughChecked(ECheckBoxState CheckBoxState);

	void OnRangeMinChanged(float NewValue);
	
	void OnRangeMaxChanged(float NewValue);

	FOverrideEditorModule* EditorModule = nullptr;
};
