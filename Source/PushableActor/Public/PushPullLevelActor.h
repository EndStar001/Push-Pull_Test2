// Copyright 2015 Greg Mladucky

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PushPullAsset.h"
#include "TimerManager.h"
#include "PushPullLevelActor.generated.h"

class UStaticMesh;
class ACameraActor;
class APlayerController;
class UChildActorComponent;
class UBoxComponent;
class UPrimitiveComponent;
class APawn;
class USceneComponent;
class UStaticMeshComponent;
class UUserWidget;

UCLASS()
class PUSHABLEACTOR_API APushPullLevelActor : public AActor
{
	GENERATED_BODY()

public:
	APushPullLevelActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPullAsset")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PushPullAsset")
	TObjectPtr<UBoxComponent> OverviewCameraTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPullAsset")
	TObjectPtr<UPushPullAsset> PushPullAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPullAsset")
	TObjectPtr<UStaticMesh> PreviewCubeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPullAsset|Camera", meta = (ClampMin = "0.5", ClampMax = "1.5"))
	float OverviewOrthoScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PushPullAsset|UI")
	TSubclassOf<UUserWidget> ResetTipsWidgetClass;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "PushPullAsset")
	void RebuildInstances();

	UFUNCTION(BlueprintCallable, Category = "PushPullAsset")
	void ResetInstance(AActor* ActorToReset);

private:
	static const FName GeneratedComponentTag;
	static const FName GeneratedPushBoxComponentTag;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> OverviewCamera;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PreviousViewTarget;

	UPROPERTY(Transient)
	FRotator PreviousControlRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ResetTipsWidget;

	bool bUsingOverviewCamera = false;
	FTimerHandle OverviewCameraTransitionTimerHandle;
	float OverviewCameraBlendTime = 0.75f;

	void ClearGeneratedComponents();
	void BuildActorInstance(const FPushPullPaletteEntry& Entry, int32 X, int32 Y);
	void ConfigurePushBoxCollision(AActor* PushBoxActor) const;
	void AlignChildActorBottom(UChildActorComponent* ChildActorComponent) const;
	void PrepareActorForReset(AActor* ActorToReset, bool bDetachActor = true) const;
	void ResetGeneratedActors();
	void RestoreActorInputAfterReset(AActor* ResetActor) const;
	void RefreshResetOverlaps(AActor* ResetActor) const;
	void HandleResetKey(APlayerController* PlayerController, APawn* PlayerPawn);
	void FinishResetRefresh(AActor* ResetActor);
	void EnsureOverviewCamera();
	void UpdateOverviewCamera();
	void UpdateOverviewCameraTrigger();
	void MaintainOverviewCamera(APlayerController* PlayerController);
	FRotator GetOverviewControlRotation() const;
	FVector GetOverviewForwardVector() const;
	FVector GetOverviewRightVector() const;
	void ApplyOverviewControlRotation(APlayerController* PlayerController) const;
	void ApplyCameraRelativeMovementSettings(AActor* Actor, APlayerController* PlayerController = nullptr) const;
	void ApplyOverviewMoveDirection(AActor* Actor, APlayerController* PlayerController) const;
	void ApplyOverviewPushActorSettings(AActor* Actor, APlayerController* PlayerController) const;
	void ApplyOverviewCharacterFacing(APlayerController* PlayerController) const;
	void StartOverviewCameraTransition(APlayerController* PlayerController);
	void FinishOverviewCameraTransition(APlayerController* PlayerController);
	void ShowResetTips(APlayerController* PlayerController);
	void HideResetTips();
	float GetViewportAspectRatio() const;
	bool TryGetVisibleMeshBottomZ(const AActor* Actor, float& OutBottomZ) const;
	bool TryGetGeneratedPlayerStartTransform(FTransform& OutTransform) const;
	bool TryGetPlayerStartCellTransform(FTransform& OutTransform) const;
	TSubclassOf<AActor> ResolveActorClass(const FPushPullPaletteEntry& Entry) const;
	FTransform MakeCellTransform(int32 X, int32 Y) const;
	const FPushPullPaletteEntry* FindPaletteEntry(const FPushPullGridCell& Cell) const;
	static void SetBoolPropertyIfPresent(UObject* Object, FName PropertyName, bool bValue);
	static void ClearArrayPropertyIfPresent(UObject* Object, FName PropertyName);
	static bool GetBoolPropertyIfPresent(const UObject* Object, FName PropertyName, bool& OutValue);
	static UObject* GetObjectPropertyIfPresent(const UObject* Object, FName PropertyName);
	static void SetObjectPropertyIfPresent(UObject* Object, FName PropertyName, UObject* Value);
	static void SetVectorPropertyIfPresent(UObject* Object, FName PropertyName, const FVector& Value);
	static float GetMappedAxisValue(const APlayerController* PlayerController, FName AxisName);

	UFUNCTION()
	void HandleOverviewTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleOverviewTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
