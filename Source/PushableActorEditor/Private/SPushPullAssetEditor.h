// Copyright 2015 Greg Mladucky

#pragma once

#include "CoreMinimal.h"
#include "PushPullAsset.h"
#include "Widgets/SCompoundWidget.h"

class SUniformGridPanel;

class SPushPullAssetEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPushPullAssetEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UPushPullAsset* InAsset);

private:
	friend class SPushPullGridCell;

	using FPaletteEntryPtr = TSharedPtr<int32>;

	TWeakObjectPtr<UPushPullAsset> EditingAsset;
	TArray<FPaletteEntryPtr> PaletteItems;
	TSharedPtr<SUniformGridPanel> GridPanel;
	int32 SelectedPaletteIndex = INDEX_NONE;
	bool bIsBatchPainting = false;
	int32 LastPaintedCellIndex = INDEX_NONE;

	void RebuildPaletteItems();
	void RebuildGrid();
	TSharedRef<SWidget> BuildPalette();
	TSharedRef<SWidget> BuildWorkspace();
	TSharedRef<ITableRow> MakePaletteRow(FPaletteEntryPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleCellPressed(int32 X, int32 Y);
	void HandleCellReleased();
	void HandleCellHovered(int32 X, int32 Y);
	FReply HandleResizeClicked(int32 WidthDelta, int32 HeightDelta);
	void PaintCell(int32 X, int32 Y);
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	FText GetCellText(int32 X, int32 Y) const;
	FSlateColor GetCellColor(int32 X, int32 Y) const;
	const FPushPullPaletteEntry* GetPaletteEntryById(FName PaletteId) const;
	const FPushPullPaletteEntry* GetSelectedPaletteEntry() const;
};
