// Copyright 2015 Greg Mladucky

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "PushPullAssetFactory.generated.h"

UCLASS()
class UPushPullAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UPushPullAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override;
};
