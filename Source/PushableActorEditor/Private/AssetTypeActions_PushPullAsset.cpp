// Copyright 2015 Greg Mladucky

#include "AssetTypeActions_PushPullAsset.h"

#include "PushPullAsset.h"
#include "PushPullAssetEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_PushPullAsset"

FText FAssetTypeActions_PushPullAsset::GetName() const
{
	return LOCTEXT("PushPullAssetName", "PushPullAsset");
}

FColor FAssetTypeActions_PushPullAsset::GetTypeColor() const
{
	return FColor(31, 119, 180);
}

UClass* FAssetTypeActions_PushPullAsset::GetSupportedClass() const
{
	return UPushPullAsset::StaticClass();
}

uint32 FAssetTypeActions_PushPullAsset::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FAssetTypeActions_PushPullAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Object : InObjects)
	{
		if (UPushPullAsset* PushPullAsset = Cast<UPushPullAsset>(Object))
		{
			TSharedRef<FPushPullAssetEditorToolkit> Editor = MakeShared<FPushPullAssetEditorToolkit>();
			Editor->InitPushPullAssetEditor(EToolkitMode::Standalone, EditWithinLevelEditor, PushPullAsset);
		}
	}
}

#undef LOCTEXT_NAMESPACE
