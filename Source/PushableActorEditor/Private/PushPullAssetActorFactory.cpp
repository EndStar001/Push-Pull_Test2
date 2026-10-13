// Copyright 2015 Greg Mladucky

#include "PushPullAssetActorFactory.h"

#include "AssetRegistry/AssetData.h"
#include "PushPullAsset.h"
#include "PushPullLevelActor.h"

#define LOCTEXT_NAMESPACE "PushPullAssetActorFactory"

UPushPullAssetActorFactory::UPushPullAssetActorFactory()
{
	DisplayName = LOCTEXT("DisplayName", "PushPullAsset");
	NewActorClass = APushPullLevelActor::StaticClass();
}

bool UPushPullAssetActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (!AssetData.IsValid() || !AssetData.GetClass()->IsChildOf(UPushPullAsset::StaticClass()))
	{
		OutErrorMsg = LOCTEXT("InvalidAsset", "A PushPullAsset is required.");
		return false;
	}

	return true;
}

void UPushPullAssetActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	APushPullLevelActor* PushPullLevelActor = Cast<APushPullLevelActor>(NewActor);
	UPushPullAsset* PushPullAsset = Cast<UPushPullAsset>(Asset);
	if (!PushPullLevelActor || !PushPullAsset)
	{
		return;
	}

	PushPullLevelActor->Modify();
	PushPullLevelActor->PushPullAsset = PushPullAsset;
	PushPullLevelActor->RebuildInstances();
}

UObject* UPushPullAssetActorFactory::GetAssetFromActorInstance(AActor* ActorInstance)
{
	if (const APushPullLevelActor* PushPullLevelActor = Cast<APushPullLevelActor>(ActorInstance))
	{
		return PushPullLevelActor->PushPullAsset;
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
