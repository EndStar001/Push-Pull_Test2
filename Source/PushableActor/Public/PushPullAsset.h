// Copyright 2015 Greg Mladucky

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "PushPullAsset.generated.h"

UENUM(BlueprintType)
enum class EPushPullTileKind : uint8
{
	Empty UMETA(DisplayName = "Empty"),
	Floor UMETA(Hidden, DisplayName = "Floor (Deprecated)"),
	Wall UMETA(DisplayName = "Wall"),
	PlayerStart UMETA(DisplayName = "Player Start"),
	PushBox UMETA(DisplayName = "Push Box"),
	Goal UMETA(DisplayName = "Goal")
};

USTRUCT(BlueprintType)
struct PUSHABLEACTOR_API FPushPullPaletteEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Pull")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Pull")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Pull")
	EPushPullTileKind TileKind = EPushPullTileKind::Empty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Pull")
	FLinearColor Color = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Pull")
	TSoftClassPtr<AActor> ActorClass;
};

USTRUCT(BlueprintType)
struct PUSHABLEACTOR_API FPushPullGridCell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Pull")
	EPushPullTileKind TileKind = EPushPullTileKind::Empty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Pull")
	FName PaletteId = NAME_None;
};

UCLASS(BlueprintType)
class PUSHABLEACTOR_API UPushPullAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPushPullAsset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "1", ClampMax = "128"))
	int32 GridWidth = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "1", ClampMax = "128"))
	int32 GridHeight = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "10.0"))
	float TileSize = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Pull")
	TSoftClassPtr<AActor> DefaultPushBoxClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Push Pull")
	TArray<FPushPullPaletteEntry> Palette;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	TArray<FPushPullGridCell> Cells;

	UFUNCTION(BlueprintCallable, Category = "Push Pull")
	void ResizeGrid(int32 NewWidth, int32 NewHeight);

	UFUNCTION(BlueprintCallable, Category = "Push Pull")
	void SetCell(int32 X, int32 Y, EPushPullTileKind TileKind, FName PaletteId);

	UFUNCTION(BlueprintPure, Category = "Push Pull")
	FPushPullGridCell GetCell(int32 X, int32 Y) const;

	int32 GetCellIndex(int32 X, int32 Y) const;
	bool IsValidCell(int32 X, int32 Y) const;
	void EnsureGridSize();
	void SanitizeDeprecatedTiles();
	void EnsureDefaultPaletteEntries();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void PostLoad() override;
};
