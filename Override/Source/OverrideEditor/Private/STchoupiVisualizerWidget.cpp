#include "STchoupiVisualizerWidget.h"
#include "OverrideEditorModule.h"
#include "Chaos/AABB.h"
#include "Chaos/AABB.h"

void STchoupiVisualizerWidget::Construct(const FArguments& InArgs)
{
	EditorModule = InArgs._EditorModule;

	VersionIds = {
		MakeShared<FString>("All"),
		MakeShared<FString>("Editor")
	};
	
	MatchIds = {
		MakeShared<FString>("All"),
		MakeShared<FString>("28")
	};

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
			SNew(SButton)
			.Text(FText::FromString("Visualize"))
			.HAlign(HAlign_Center)
			.OnClicked(this, &STchoupiVisualizerWidget::OnVisualizeClicked)
		]
	];
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
	if (EditorModule && SelectedMatchId.IsValid())
	{
		EditorModule->VisualizeMatch(*SelectedVersionId, *SelectedMatchId, *SelectedPlayerId, *SelectedTeamId);
	}

	return FReply::Handled();
}

FReply STchoupiVisualizerWidget::OnClearClicked()
{
	
	return FReply::Handled();
}
