// Copyright 2015 Greg Mladucky

#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "PushPullAssetActorFactory.generated.h"

UCLASS()
class UPushPullAssetActorFactory : public UActorFactory
{
	GENERATED_BODY()

public:
	UPushPullAssetActorFactory();

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
};
