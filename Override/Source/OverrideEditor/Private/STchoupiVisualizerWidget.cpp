#include "STchoupiVisualizerWidget.h"
#include "OverrideEditorModule.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "GameMode/MatchData.h"
#include "Network/ServerHttpClient.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SSpinBox.h"

struct FMatchData;

void STchoupiVisualizerWidget::Construct(const FArguments& InArgs)
{
	EditorModule = InArgs._EditorModule;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Version ID"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(5)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&VersionIds)
				.OnGenerateWidget(this, &STchoupiVisualizerWidget::GenerateVersionItem)
				.OnSelectionChanged(this, &STchoupiVisualizerWidget::OnVersionSelected)
				.InitiallySelectedItem(SelectedVersionId)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return SelectedVersionId.IsValid()
							       ? FText::FromString(*SelectedVersionId)
							       : FText::FromString("Select version id");
					})
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Match ID"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(5)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&MatchIds)
				.OnGenerateWidget(this, &STchoupiVisualizerWidget::GenerateMatchItem)
				.OnSelectionChanged(this, &STchoupiVisualizerWidget::OnMatchSelected)
				.InitiallySelectedItem(SelectedMatchId)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return SelectedMatchId.IsValid()
							       ? FText::FromString(*SelectedMatchId)
							       : FText::FromString("Select match id");
					})
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Player ID"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(5)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&PlayerIds)
				.OnGenerateWidget(this, &STchoupiVisualizerWidget::GeneratePlayerItem)
				.OnSelectionChanged(this, &STchoupiVisualizerWidget::OnPlayerSelected)
				.InitiallySelectedItem(SelectedPlayerId)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return SelectedPlayerId.IsValid()
							       ? FText::FromString(*SelectedPlayerId)
							       : FText::FromString("Select player id");
					})
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Team ID"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(5)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&TeamIds)
				.OnGenerateWidget(this, &STchoupiVisualizerWidget::GenerateTeamItem)
				.OnSelectionChanged(this, &STchoupiVisualizerWidget::OnTeamSelected)
				.InitiallySelectedItem(SelectedTeamId)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return SelectedTeamId.IsValid()
							       ? FText::FromString(*SelectedTeamId)
							       : FText::FromString("Select team id");
					})
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5)
			[
				SNew(STextBlock)
				.Text(FText::FromString("See Through Gizmos"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(5)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &STchoupiVisualizerWidget::OnSeeThroughChecked)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Timeline min"))
			]


			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.f)
				.MaxValue(1.f)
				.Value(this, &STchoupiVisualizerWidget::GetRangeMin)
				.OnValueChanged(this, &STchoupiVisualizerWidget::OnRangeMinChanged)
				.MinDesiredWidth(60.f)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::FromString("max"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.f)
				.MaxValue(1.f)
				.Value(this, &STchoupiVisualizerWidget::GetRangeMax)
				.OnValueChanged(this, &STchoupiVisualizerWidget::OnRangeMaxChanged)
				.MinDesiredWidth(60.f)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5)
			[
				SNew(SButton)
				.Text(FText::FromString("Visualize"))
				.HAlign(HAlign_Center)
				.OnClicked(this, &STchoupiVisualizerWidget::OnVisualizeClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5)
			[
				SNew(SButton)
				.Text(FText::FromString("Clear"))
				.HAlign(HAlign_Center)
				.OnClicked(this, &STchoupiVisualizerWidget::OnClearClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5)
			[
				SNew(SButton)
				.Text(FText::FromString("Fetch Data"))
				.HAlign(HAlign_Center)
				.OnClicked(this, &STchoupiVisualizerWidget::OnFetchDataClicked)
			]
		]
	];
}

void STchoupiVisualizerWidget::UpdateLists(const TArray<FString>& Versions, const TArray<FString>& Matches)
{
	VersionIds.Empty();
	MatchIds.Empty();

	VersionIds.Add(MakeShared<FString>("All"));
	MatchIds.Add(MakeShared<FString>("All"));

	for (FString Str : Versions)
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *Str);
		VersionIds.Add(MakeShared<FString>(Str));
	}

	for (FString Str : Matches)
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *Str);
		MatchIds.Add(MakeShared<FString>(Str));
	}

	PlayerIds = {
		MakeShared<FString>("All"),
		MakeShared<FString>("0"),
		MakeShared<FString>("1"),
		MakeShared<FString>("2"),
		MakeShared<FString>("3"),
		MakeShared<FString>("4"),
		MakeShared<FString>("5")
	};

	TeamIds = {
		MakeShared<FString>("All"),
		MakeShared<FString>("0"),
		MakeShared<FString>("1"),
		MakeShared<FString>("2")
	};

	SelectedVersionId = VersionIds[0];
	SelectedMatchId = MatchIds[0];
	SelectedPlayerId = PlayerIds[0];
	SelectedTeamId = TeamIds[0];
}

void STchoupiVisualizerWidget::OnMatchesDataReceived(TArray<FMatchData> MatchesData)
{
	UE_LOG(LogTemp, Log, TEXT("%i Matches data received"), MatchesData.Num());

	CachedMatchesData = MatchesData;

	TArray<FString> Versions;
	TArray<FString> MatchesIds;

	for (FMatchData Match : CachedMatchesData)
	{
		UE_LOG(LogTemp, Log, TEXT("Found match %s with version: %s"), *Match.Id, *Match.Version);

		// Can't convert already to shrd ptr because we need to be able to compare strings values
		Versions.AddUnique(Match.Version);
		MatchesIds.AddUnique(Match.Id);
	}

	UpdateLists(Versions, MatchesIds);
}

TSharedRef<SWidget> STchoupiVisualizerWidget::GenerateVersionItem(TSharedPtr<FString> Item)
{
	return SNew(STextBlock).Text(FText::FromString(*Item));
}

void STchoupiVisualizerWidget::OnVersionSelected(TSharedPtr<FString> Item, ESelectInfo::Type)
{
	SelectedVersionId = Item;
}

TSharedRef<SWidget> STchoupiVisualizerWidget::GenerateMatchItem(TSharedPtr<FString> Item)
{
	return SNew(STextBlock).Text(FText::FromString(*Item));
}

void STchoupiVisualizerWidget::OnMatchSelected(TSharedPtr<FString> Item, ESelectInfo::Type)
{
	SelectedMatchId = Item;
}

TSharedRef<SWidget> STchoupiVisualizerWidget::GeneratePlayerItem(TSharedPtr<FString> Item)
{
	return SNew(STextBlock).Text(FText::FromString(*Item));
}

void STchoupiVisualizerWidget::OnPlayerSelected(TSharedPtr<FString> Item, ESelectInfo::Type)
{
	SelectedPlayerId = Item;
}

TSharedRef<SWidget> STchoupiVisualizerWidget::GenerateTeamItem(TSharedPtr<FString> Item)
{
	return SNew(STextBlock).Text(FText::FromString(*Item));
}

void STchoupiVisualizerWidget::OnTeamSelected(TSharedPtr<FString> Item, ESelectInfo::Type)
{
	SelectedTeamId = Item;
}

FReply STchoupiVisualizerWidget::OnVisualizeClicked()
{
	if (CachedMatchesData.Num() > 0)
	{
		EditorModule->ShowVisualization(CachedMatchesData, SelectedVersionId, SelectedMatchId, SelectedTeamId,
		                                SelectedPlayerId, SeeThrough, GetRangeMin(), GetRangeMax());
	}

	return FReply::Handled();
}

FReply STchoupiVisualizerWidget::OnClearClicked()
{
	if (EditorModule)
	{
		EditorModule->ClearVisualization();
	}

	return FReply::Handled();
}

FReply STchoupiVisualizerWidget::OnFetchDataClicked()
{
	if (EditorModule)
	{
		EditorModule->RequestData(this);
	}

	return FReply::Handled();
}

void STchoupiVisualizerWidget::OnSeeThroughChecked(ECheckBoxState CheckBoxState)
{
	SeeThrough = CheckBoxState == ECheckBoxState::Checked;
}

void STchoupiVisualizerWidget::OnRangeMinChanged(float NewValue)
{
	RangeMin = FMath::Min(NewValue, RangeMax);

	if (EditorModule)
	{
		EditorModule->ClearVisualization();
		EditorModule->ShowVisualization(CachedMatchesData, SelectedVersionId, SelectedMatchId, SelectedTeamId,
										SelectedPlayerId, SeeThrough, GetRangeMin(), GetRangeMax());
	}
}

void STchoupiVisualizerWidget::OnRangeMaxChanged(float NewValue)
{
	RangeMax = FMath::Max(NewValue, RangeMin);

	if (EditorModule)
	{
		EditorModule->ClearVisualization();
		EditorModule->ShowVisualization(CachedMatchesData, SelectedVersionId, SelectedMatchId, SelectedTeamId,
										SelectedPlayerId, SeeThrough, GetRangeMin(), GetRangeMax());
	}
}
