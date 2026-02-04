#pragma once

#include "CoreMinimal.h"

class FOverrideEditorModule;

class OVERRIDEEDITOR_API STchoupiVisualizerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STchoupiVisualizerWidget) {}
		SLATE_ARGUMENT(class FOverrideEditorModule*, EditorModule)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
private:
	TArray<TSharedPtr<FString>> VersionIds;
	TSharedPtr<FString> SelectedVersionId;
	
	TArray<TSharedPtr<FString>> MatchIds;
	TSharedPtr<FString> SelectedMatchId;

	TArray<TSharedPtr<FString>> PlayerIds;
	TSharedPtr<FString> SelectedPlayerId;

	TArray<TSharedPtr<FString>> TeamIds;
	TSharedPtr<FString> SelectedTeamId;

	bool SeeThrough = false;

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

	FOverrideEditorModule* EditorModule = nullptr;
};
