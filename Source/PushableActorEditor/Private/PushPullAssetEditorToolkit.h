// Copyright 2015 Greg Mladucky

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

class UPushPullAsset;

class FPushPullAssetEditorToolkit : public FAssetEditorToolkit, public FGCObject
{
public:
	void InitPushPullAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UPushPullAsset* InAsset);

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

private:
	static const FName WorkspaceTabId;

	TObjectPtr<UPushPullAsset> EditingAsset = nullptr;

	TSharedRef<SDockTab> SpawnWorkspaceTab(const FSpawnTabArgs& Args);
};
