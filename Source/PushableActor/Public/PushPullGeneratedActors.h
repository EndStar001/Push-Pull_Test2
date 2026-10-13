// Copyright 2015 Greg Mladucky

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "PushPullGeneratedActors.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UButton;
class APushPullLevelActor;
class ABP_Goal;

UCLASS(BlueprintType, Blueprintable, DisplayName = "PushPullWidget")
class PUSHABLEACTOR_API UPushPullWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PushPull")
	void InitializeForGoal(ABP_Goal* InGoal, APushPullLevelActor* InLevelActor);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Reset;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ExitGame;

private:
	UPROPERTY(Transient)
	TObjectPtr<ABP_Goal> OwningGoal;

	UPROPERTY(Transient)
	TObjectPtr<APushPullLevelActor> OwningLevelActor;

	UFUNCTION()
	void HandleResetClicked();

	UFUNCTION()
	void HandleExitGameClicked();

	APushPullLevelActor* ResolveLevelActorForReset(AActor* ResetActor) const;
	void RestoreGameInput();
};

UCLASS(BlueprintType, NotBlueprintable, DisplayName = "BP_Floor_Deprecated")
class PUSHABLEACTOR_API ABP_Floor : public AActor
{
	GENERATED_BODY()

public:
	ABP_Floor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BP_Floor")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};

UCLASS(BlueprintType, Blueprintable, DisplayName = "BP_Wall")
class PUSHABLEACTOR_API ABP_Wall : public AActor
{
	GENERATED_BODY()

public:
	ABP_Wall();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BP_Wall")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};

UCLASS(BlueprintType, Blueprintable, DisplayName = "BP_Box")
class PUSHABLEACTOR_API ABP_Box : public AActor
{
	GENERATED_BODY()

public:
	ABP_Box();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BP_Box")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};

UCLASS(BlueprintType, Blueprintable, DisplayName = "BP_Goal")
class PUSHABLEACTOR_API ABP_Goal : public AActor
{
	GENERATED_BODY()

public:
	ABP_Goal();

	void SetOwningLevelActor(APushPullLevelActor* InLevelActor);
	void ClearFinishWidget(UPushPullWidget* Widget);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BP_Goal")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BP_Goal")
	TSubclassOf<UPushPullWidget> FinishWidgetClass;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPushPullWidget> FinishWidget;

	UPROPERTY(Transient)
	TObjectPtr<APushPullLevelActor> OwningLevelActor;

	APushPullLevelActor* ResolveOwningLevelActor() const;
	void TryShowFinishWidget(AActor* OtherActor);

	UFUNCTION()
	void HandleGoalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
