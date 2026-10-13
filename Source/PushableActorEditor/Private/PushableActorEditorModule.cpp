// Copyright 2015 Greg Mladucky

#include "Modules/ModuleManager.h"

#include "AssetToolsModule.h"
#include "AssetTypeActions_PushPullAsset.h"

class FPushableActorEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		PushPullAssetTypeActions = MakeShared<FAssetTypeActions_PushPullAsset>();
		AssetTools.RegisterAssetTypeActions(PushPullAssetTypeActions.ToSharedRef());
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded("AssetTools") && PushPullAssetTypeActions.IsValid())
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			AssetTools.UnregisterAssetTypeActions(PushPullAssetTypeActions.ToSharedRef());
		}

		PushPullAssetTypeActions.Reset();
	}

private:
	TSharedPtr<IAssetTypeActions> PushPullAssetTypeActions;
};

IMPLEMENT_MODULE(FPushableActorEditorModule, PushableActorEditor)
