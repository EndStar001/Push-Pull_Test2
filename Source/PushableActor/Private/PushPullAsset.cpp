// Copyright 2015 Greg Mladucky

#include "PushPullAsset.h"

#include "PushPullGeneratedActors.h"

namespace PushPullAssetDefaults
{
	static const TCHAR* PushBoxBlueprintPath = TEXT("/Game/PushableActorProject/Blueprints/BasicPush/BP_PushPawn.BP_PushPawn_C");

	static FName GetDeprecatedFloorId()
	{
		static const FName DeprecatedFloorId(TEXT("Floor"));
		return DeprecatedFloorId;
	}

	static TSoftClassPtr<AActor> MakeDefaultPushBoxClass()
	{
		return TSoftClassPtr<AActor>(FSoftObjectPath(PushBoxBlueprintPath));
	}

	static TArray<FPushPullPaletteEntry> MakeDefaultPalette()
	{
		const TSoftClassPtr<AActor> PushBoxClass = MakeDefaultPushBoxClass();
		TArray<FPushPullPaletteEntry> Defaults;

		Defaults.Add({TEXT("Empty"), NSLOCTEXT("PushPullAsset", "Empty", "Empty"), EPushPullTileKind::Empty, FLinearColor(0.03f, 0.03f, 0.03f, 0.2f), TSoftClassPtr<AActor>()});
		Defaults.Add({TEXT("Wall"), NSLOCTEXT("PushPullAsset", "Wall", "Wall"), EPushPullTileKind::Wall, FLinearColor(0.08f, 0.11f, 0.13f, 1.0f), ABP_Wall::StaticClass()});
		Defaults.Add({TEXT("PlayerStart"), NSLOCTEXT("PushPullAsset", "PlayerStart", "Player"), EPushPullTileKind::PlayerStart, FLinearColor(0.10f, 0.42f, 0.86f, 1.0f), TSoftClassPtr<AActor>()});

		FPushPullPaletteEntry PushBoxEntry;
		PushBoxEntry.Id = TEXT("PushBox");
		PushBoxEntry.DisplayName = NSLOCTEXT("PushPullAsset", "PushBox", "Box");
		PushBoxEntry.TileKind = EPushPullTileKind::PushBox;
		PushBoxEntry.Color = FLinearColor(0.80f, 0.42f, 0.12f, 1.0f);
		PushBoxEntry.ActorClass = PushBoxClass;
		Defaults.Add(PushBoxEntry);

		Defaults.Add({TEXT("Goal"), NSLOCTEXT("PushPullAsset", "Goal", "Goal"), EPushPullTileKind::Goal, FLinearColor::Yellow, ABP_Goal::StaticClass()});
		return Defaults;
	}
}

UPushPullAsset::UPushPullAsset()
{
	DefaultPushBoxClass = PushPullAssetDefaults::MakeDefaultPushBoxClass();

	Palette = PushPullAssetDefaults::MakeDefaultPalette();

	EnsureGridSize();
	SanitizeDeprecatedTiles();
	EnsureDefaultPaletteEntries();
}

void UPushPullAsset::PostLoad()
{
	Super::PostLoad();
	EnsureGridSize();
	SanitizeDeprecatedTiles();
	EnsureDefaultPaletteEntries();
}

#if WITH_EDITOR
void UPushPullAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	GridWidth = FMath::Clamp(GridWidth, 1, 128);
	GridHeight = FMath::Clamp(GridHeight, 1, 128);
	TileSize = FMath::Max(TileSize, 10.0f);
	EnsureGridSize();
	SanitizeDeprecatedTiles();
	EnsureDefaultPaletteEntries();
}
#endif

void UPushPullAsset::ResizeGrid(int32 NewWidth, int32 NewHeight)
{
	const int32 OldWidth = GridWidth;
	const int32 OldHeight = GridHeight;
	const TArray<FPushPullGridCell> OldCells = Cells;

	GridWidth = FMath::Clamp(NewWidth, 1, 128);
	GridHeight = FMath::Clamp(NewHeight, 1, 128);

	Cells.Reset();
	Cells.SetNum(GridWidth * GridHeight);

	const int32 CopyWidth = FMath::Min(OldWidth, GridWidth);
	const int32 CopyHeight = FMath::Min(OldHeight, GridHeight);
	for (int32 Y = 0; Y < CopyHeight; ++Y)
	{
		for (int32 X = 0; X < CopyWidth; ++X)
		{
			const int32 OldIndex = Y * OldWidth + X;
			if (OldCells.IsValidIndex(OldIndex))
			{
				Cells[Y * GridWidth + X] = OldCells[OldIndex];
			}
		}
	}

	SanitizeDeprecatedTiles();
}

void UPushPullAsset::SetCell(int32 X, int32 Y, EPushPullTileKind TileKind, FName PaletteId)
{
	const int32 CellIndex = GetCellIndex(X, Y);
	if (CellIndex == INDEX_NONE)
	{
		return;
	}

	if (TileKind == EPushPullTileKind::Floor || PaletteId == PushPullAssetDefaults::GetDeprecatedFloorId())
	{
		Cells[CellIndex].TileKind = EPushPullTileKind::Empty;
		Cells[CellIndex].PaletteId = NAME_None;
		return;
	}

	Cells[CellIndex].TileKind = TileKind;
	Cells[CellIndex].PaletteId = PaletteId;
}

FPushPullGridCell UPushPullAsset::GetCell(int32 X, int32 Y) const
{
	const int32 CellIndex = GetCellIndex(X, Y);
	return CellIndex == INDEX_NONE ? FPushPullGridCell() : Cells[CellIndex];
}

int32 UPushPullAsset::GetCellIndex(int32 X, int32 Y) const
{
	if (!IsValidCell(X, Y))
	{
		return INDEX_NONE;
	}

	return Y * GridWidth + X;
}

bool UPushPullAsset::IsValidCell(int32 X, int32 Y) const
{
	return X >= 0 && Y >= 0 && X < GridWidth && Y < GridHeight;
}

void UPushPullAsset::EnsureGridSize()
{
	const int32 DesiredCount = FMath::Max(1, GridWidth) * FMath::Max(1, GridHeight);
	Cells.SetNum(DesiredCount);
}

void UPushPullAsset::SanitizeDeprecatedTiles()
{
	Palette.RemoveAll([](const FPushPullPaletteEntry& Entry)
	{
		return Entry.TileKind == EPushPullTileKind::Floor || Entry.Id == PushPullAssetDefaults::GetDeprecatedFloorId();
	});

	for (FPushPullGridCell& Cell : Cells)
	{
		if (Cell.TileKind == EPushPullTileKind::Floor || Cell.PaletteId == PushPullAssetDefaults::GetDeprecatedFloorId())
		{
			Cell.TileKind = EPushPullTileKind::Empty;
			Cell.PaletteId = NAME_None;
		}
	}
}

void UPushPullAsset::EnsureDefaultPaletteEntries()
{
	const TSoftClassPtr<AActor> PushBoxClass = PushPullAssetDefaults::MakeDefaultPushBoxClass();
	if (DefaultPushBoxClass.IsNull())
	{
		DefaultPushBoxClass = PushBoxClass;
	}

	for (const FPushPullPaletteEntry& DefaultEntry : PushPullAssetDefaults::MakeDefaultPalette())
	{
		if (DefaultEntry.TileKind == EPushPullTileKind::Floor)
		{
			continue;
		}

		const bool bHasEntry = Palette.ContainsByPredicate([&DefaultEntry](const FPushPullPaletteEntry& Entry)
		{
			return Entry.Id == DefaultEntry.Id || Entry.TileKind == DefaultEntry.TileKind;
		});

		if (!bHasEntry)
		{
			Palette.Add(DefaultEntry);
		}
	}

	for (FPushPullPaletteEntry& Entry : Palette)
	{
		if (Entry.TileKind == EPushPullTileKind::PushBox && (Entry.ActorClass.IsNull() || Entry.ActorClass.Get() == ABP_Box::StaticClass()))
		{
			Entry.ActorClass = PushBoxClass;
		}
	}
}
