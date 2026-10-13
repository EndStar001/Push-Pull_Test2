// Copyright 2015 Greg Mladucky

#include "PushPullAssetFactory.h"

#include "PushPullAsset.h"

UPushPullAssetFactory::UPushPullAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UPushPullAsset::StaticClass();
}

UObject* UPushPullAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UPushPullAsset>(InParent, Class, Name, Flags | RF_Transactional);
}

bool UPushPullAssetFactory::ShouldShowInNewMenu() const
{
	return true;
}
