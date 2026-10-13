// Copyright 2015 Greg Mladucky

#include "SPushPullAssetEditor.h"

#include "ScopedTransaction.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "PushPullAssetEditor"

namespace PushPullAssetEditorConstants
{
	constexpr float GridCellSize = 48.0f;
}

class SPushPullGridCell : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPushPullGridCell) {}
		SLATE_ARGUMENT(SPushPullAssetEditor*, Owner)
		SLATE_ARGUMENT(int32, X)
		SLATE_ARGUMENT(int32, Y)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Owner = InArgs._Owner;
		X = InArgs._X;
		Y = InArgs._Y;
		ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (Owner && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			Owner->HandleCellPressed(X, Y);
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (Owner && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			Owner->HandleCellReleased();
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);

		if (Owner && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			Owner->HandleCellHovered(X, Y);
		}
	}

private:
	SPushPullAssetEditor* Owner = nullptr;
	int32 X = 0;
	int32 Y = 0;
};

void SPushPullAssetEditor::Construct(const FArguments& InArgs, UPushPullAsset* InAsset)
{
	EditingAsset = InAsset;
	SelectedPaletteIndex = InAsset && InAsset->Palette.Num() > 0 ? 0 : INDEX_NONE;
	RebuildPaletteItems();

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot()
		.Value(0.28f)
		[
			BuildPalette()
		]
		+ SSplitter::Slot()
		.Value(0.72f)
		[
			BuildWorkspace()
		]
	];

	RebuildGrid();
}

void SPushPullAssetEditor::RebuildPaletteItems()
{
	PaletteItems.Reset();

	if (UPushPullAsset* Asset = EditingAsset.Get())
	{
		Asset->SanitizeDeprecatedTiles();
		Asset->EnsureDefaultPaletteEntries();

		for (int32 Index = 0; Index < Asset->Palette.Num(); ++Index)
		{
			if (Asset->Palette[Index].TileKind == EPushPullTileKind::Floor)
			{
				continue;
			}

			PaletteItems.Add(MakeShared<int32>(Index));
		}

		const bool bSelectedPaletteIsUsable = Asset->Palette.IsValidIndex(SelectedPaletteIndex) && Asset->Palette[SelectedPaletteIndex].TileKind != EPushPullTileKind::Floor;
		if (!bSelectedPaletteIsUsable)
		{
			SelectedPaletteIndex = PaletteItems.Num() > 0 ? *PaletteItems[0] : INDEX_NONE;
		}
	}
}

TSharedRef<SWidget> SPushPullAssetEditor::BuildPalette()
{
	return SNew(SBorder)
		.Padding(10.0f)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AssetPanelTitle", "Assets"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SListView<FPaletteEntryPtr>)
				.ListItemsSource(&PaletteItems)
				.OnGenerateRow(this, &SPushPullAssetEditor::MakePaletteRow)
				.SelectionMode(ESelectionMode::Single)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text(LOCTEXT("PushPullPrinciple", "Push boxes use BP_PushPawn / BP_PushPawn_Grid in the level, while AC_PushCharacter traces and talks to BPI_PushActor to start or end push movement."))
			]
		];
}

TSharedRef<SWidget> SPushPullAssetEditor::BuildWorkspace()
{
	return SNew(SBorder)
		.Padding(10.0f)
		.BorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 16.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						const UPushPullAsset* Asset = EditingAsset.Get();
						return Asset ? FText::Format(LOCTEXT("GridSize", "Workspace {0} x {1}"), Asset->GridWidth, Asset->GridHeight) : LOCTEXT("NoAsset", "Workspace");
					})
					.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("AddColumn", "+ Column"))
					.OnClicked(this, &SPushPullAssetEditor::HandleResizeClicked, 1, 0)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("RemoveColumn", "- Column"))
					.OnClicked(this, &SPushPullAssetEditor::HandleResizeClicked, -1, 0)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("AddRow", "+ Row"))
					.OnClicked(this, &SPushPullAssetEditor::HandleResizeClicked, 0, 1)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("RemoveRow", "- Row"))
					.OnClicked(this, &SPushPullAssetEditor::HandleResizeClicked, 0, -1)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 10.0f)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(GridPanel, SUniformGridPanel)
				]
			]
		];
}

TSharedRef<ITableRow> SPushPullAssetEditor::MakePaletteRow(FPaletteEntryPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const UPushPullAsset* Asset = EditingAsset.Get();
	const int32 PaletteIndex = Item.IsValid() ? *Item : INDEX_NONE;
	const FPushPullPaletteEntry* Entry = Asset && Asset->Palette.IsValidIndex(PaletteIndex) ? &Asset->Palette[PaletteIndex] : nullptr;

	return SNew(STableRow<FPaletteEntryPtr>, OwnerTable)
		[
			SNew(SButton)
			.ContentPadding(FMargin(6.0f, 3.0f))
			.OnClicked_Lambda([this, PaletteIndex]()
			{
				SelectedPaletteIndex = PaletteIndex;
				return FReply::Handled();
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 2.0f, 8.0f, 2.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(Entry ? Entry->Color : FLinearColor::Black)
					.Padding(FMargin(10.0f))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Entry ? Entry->DisplayName : FText::GetEmpty())
				]
			]
		];
}

void SPushPullAssetEditor::RebuildGrid()
{
	if (!GridPanel.IsValid())
	{
		return;
	}

	GridPanel->ClearChildren();

	const UPushPullAsset* Asset = EditingAsset.Get();
	if (!Asset)
	{
		return;
	}

	for (int32 Y = 0; Y < Asset->GridHeight; ++Y)
	{
		for (int32 X = 0; X < Asset->GridWidth; ++X)
		{
			GridPanel->AddSlot(X, Y)
			[
				SNew(SPushPullGridCell)
				.Owner(this)
				.X(X)
				.Y(Y)
				[
					SNew(SBorder)
					.Padding(0.0f)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(this, &SPushPullAssetEditor::GetCellColor, X, Y)
					[
						SNew(SBox)
						.WidthOverride(PushPullAssetEditorConstants::GridCellSize)
						.HeightOverride(PushPullAssetEditorConstants::GridCellSize)
						.MinDesiredWidth(PushPullAssetEditorConstants::GridCellSize)
						.MinDesiredHeight(PushPullAssetEditorConstants::GridCellSize)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.Text(this, &SPushPullAssetEditor::GetCellText, X, Y)
						]
					]
				]
			];
		}
	}
}

void SPushPullAssetEditor::HandleCellPressed(int32 X, int32 Y)
{
	bIsBatchPainting = true;
	LastPaintedCellIndex = INDEX_NONE;
	PaintCell(X, Y);
}

void SPushPullAssetEditor::HandleCellReleased()
{
	bIsBatchPainting = false;
	LastPaintedCellIndex = INDEX_NONE;
}

void SPushPullAssetEditor::HandleCellHovered(int32 X, int32 Y)
{
	if (bIsBatchPainting)
	{
		PaintCell(X, Y);
	}
}

FReply SPushPullAssetEditor::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		HandleCellReleased();
	}

	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

void SPushPullAssetEditor::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	HandleCellReleased();
	SCompoundWidget::OnMouseLeave(MouseEvent);
}

void SPushPullAssetEditor::PaintCell(int32 X, int32 Y)
{
	UPushPullAsset* Asset = EditingAsset.Get();
	const FPushPullPaletteEntry* Entry = GetSelectedPaletteEntry();
	if (!Asset || !Entry)
	{
		return;
	}

	const int32 CellIndex = Asset->GetCellIndex(X, Y);
	if (CellIndex == INDEX_NONE || CellIndex == LastPaintedCellIndex)
	{
		return;
	}

	const FPushPullGridCell CurrentCell = Asset->GetCell(X, Y);
	if (CurrentCell.TileKind == Entry->TileKind && CurrentCell.PaletteId == Entry->Id)
	{
		LastPaintedCellIndex = CellIndex;
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("PaintPushPullCell", "Paint Push Pull Cell"));
	Asset->Modify();
	Asset->SetCell(X, Y, Entry->TileKind, Entry->Id);
	Asset->MarkPackageDirty();
	LastPaintedCellIndex = CellIndex;
	Invalidate(EInvalidateWidgetReason::Paint);
}

FReply SPushPullAssetEditor::HandleResizeClicked(int32 WidthDelta, int32 HeightDelta)
{
	UPushPullAsset* Asset = EditingAsset.Get();
	if (!Asset)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("ResizePushPullGrid", "Resize Push Pull Grid"));
	Asset->Modify();
	Asset->ResizeGrid(Asset->GridWidth + WidthDelta, Asset->GridHeight + HeightDelta);
	Asset->MarkPackageDirty();
	RebuildGrid();

	return FReply::Handled();
}

FText SPushPullAssetEditor::GetCellText(int32 X, int32 Y) const
{
	const UPushPullAsset* Asset = EditingAsset.Get();
	if (!Asset)
	{
		return FText::GetEmpty();
	}

	const FPushPullGridCell Cell = Asset->GetCell(X, Y);
	const FPushPullPaletteEntry* Entry = GetPaletteEntryById(Cell.PaletteId);
	if (!Entry || Cell.TileKind == EPushPullTileKind::Empty)
	{
		return FText::GetEmpty();
	}

	return FText::FromString(Entry->DisplayName.ToString().Left(1));
}

FSlateColor SPushPullAssetEditor::GetCellColor(int32 X, int32 Y) const
{
	const UPushPullAsset* Asset = EditingAsset.Get();
	if (!Asset)
	{
		return FLinearColor::Black;
	}

	const FPushPullGridCell Cell = Asset->GetCell(X, Y);
	const FPushPullPaletteEntry* Entry = GetPaletteEntryById(Cell.PaletteId);
	return Entry ? Entry->Color : FLinearColor(0.02f, 0.02f, 0.02f, 0.25f);
}

const FPushPullPaletteEntry* SPushPullAssetEditor::GetPaletteEntryById(FName PaletteId) const
{
	const UPushPullAsset* Asset = EditingAsset.Get();
	if (!Asset)
	{
		return nullptr;
	}

	return Asset->Palette.FindByPredicate([PaletteId](const FPushPullPaletteEntry& Entry)
	{
		return Entry.Id == PaletteId;
	});
}

const FPushPullPaletteEntry* SPushPullAssetEditor::GetSelectedPaletteEntry() const
{
	const UPushPullAsset* Asset = EditingAsset.Get();
	return Asset && Asset->Palette.IsValidIndex(SelectedPaletteIndex) ? &Asset->Palette[SelectedPaletteIndex] : nullptr;
}

#undef LOCTEXT_NAMESPACE
