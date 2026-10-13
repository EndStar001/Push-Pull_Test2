// Copyright 2015 Greg Mladucky

#include "PushPullAssetEditorToolkit.h"

#include "PushPullAsset.h"
#include "SPushPullAssetEditor.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "PushPullAssetEditorToolkit"

const FName FPushPullAssetEditorToolkit::WorkspaceTabId(TEXT("PushPullAsset_Workspace"));

void FPushPullAssetEditorToolkit::InitPushPullAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UPushPullAsset* InAsset)
{
	EditingAsset = InAsset;

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("PushPullAssetEditorLayout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(WorkspaceTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, FName(TEXT("PushPullAssetEditor")), StandaloneDefaultLayout, true, true, InAsset);
}

FName FPushPullAssetEditorToolkit::GetToolkitFName() const
{
	return FName(TEXT("PushPullAssetEditor"));
}

FText FPushPullAssetEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "PushPullAsset");
}

FString FPushPullAssetEditorToolkit::GetWorldCentricTabPrefix() const
{
	return TEXT("PushPullAsset");
}

FLinearColor FPushPullAssetEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.12f, 0.45f, 0.80f, 1.0f);
}

void FPushPullAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(WorkspaceTabId, FOnSpawnTab::CreateSP(this, &FPushPullAssetEditorToolkit::SpawnWorkspaceTab))
		.SetDisplayName(LOCTEXT("WorkspaceTab", "Workspace"));
}

void FPushPullAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(WorkspaceTabId);
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

void FPushPullAssetEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(EditingAsset);
}

FString FPushPullAssetEditorToolkit::GetReferencerName() const
{
	return TEXT("FPushPullAssetEditorToolkit");
}

TSharedRef<SDockTab> FPushPullAssetEditorToolkit::SpawnWorkspaceTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("WorkspaceTabLabel", "Workspace"))
		[
			SNew(SPushPullAssetEditor, EditingAsset.Get())
		];
}

#undef LOCTEXT_NAMESPACE
