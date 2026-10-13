// Copyright 2015 Greg Mladucky

#include "PushPullLevelActor.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/InputSettings.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PushPullGeneratedActors.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

const FName APushPullLevelActor::GeneratedComponentTag(TEXT("PushPullGenerated"));
const FName APushPullLevelActor::GeneratedPushBoxComponentTag(TEXT("PushPullGeneratedPushBox"));

APushPullLevelActor::APushPullLevelActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	RootComponent = SceneRoot;

	OverviewCameraTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("OverviewCameraTrigger"));
	OverviewCameraTrigger->SetupAttachment(SceneRoot);
	OverviewCameraTrigger->SetMobility(EComponentMobility::Movable);
	OverviewCameraTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverviewCameraTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	OverviewCameraTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverviewCameraTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	OverviewCameraTrigger->SetGenerateOverlapEvents(true);
	OverviewCameraTrigger->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		PreviewCubeMesh = CubeMeshFinder.Object;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> ResetTipsWidgetFinder(TEXT("/Game/UI_ResetTips"));
	if (ResetTipsWidgetFinder.Succeeded())
	{
		ResetTipsWidgetClass = ResetTipsWidgetFinder.Class;
	}
}

void APushPullLevelActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildInstances();
	UpdateOverviewCameraTrigger();
	UpdateOverviewCamera();
}

void APushPullLevelActor::BeginPlay()
{
	Super::BeginPlay();

	if (OverviewCameraTrigger)
	{
		OverviewCameraTrigger->OnComponentBeginOverlap.AddDynamic(this, &APushPullLevelActor::HandleOverviewTriggerBeginOverlap);
		OverviewCameraTrigger->OnComponentEndOverlap.AddDynamic(this, &APushPullLevelActor::HandleOverviewTriggerEndOverlap);
	}

	UpdateOverviewCameraTrigger();
	EnsureOverviewCamera();
	UpdateOverviewCamera();

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* PlayerPawn = PlayerController->GetPawn())
		{
			if (OverviewCameraTrigger && OverviewCameraTrigger->IsOverlappingActor(PlayerPawn))
			{
				HandleOverviewTriggerBeginOverlap(OverviewCameraTrigger, PlayerPawn, Cast<UPrimitiveComponent>(PlayerPawn->GetRootComponent()), 0, false, FHitResult());
			}
		}
	}
}

void APushPullLevelActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		HideResetTips();
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(OverviewCameraTransitionTimerHandle);
		}
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->SetManualCameraFade(0.0f, FLinearColor::Black, false);
		}

		if (bUsingOverviewCamera && PlayerController->GetViewTarget() == OverviewCamera)
		{
			AActor* RestoreTarget = PreviousViewTarget.IsValid() ? PreviousViewTarget.Get() : PlayerController->GetPawn();
			if (RestoreTarget)
			{
				PlayerController->SetViewTarget(RestoreTarget);
			}
		}
	}

	if (OverviewCamera)
	{
		OverviewCamera->Destroy();
		OverviewCamera = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void APushPullLevelActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!OverviewCameraTrigger || !OverviewCamera)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn || !OverviewCameraTrigger->IsOverlappingActor(PlayerPawn))
	{
		return;
	}

	HandleResetKey(PlayerController, PlayerPawn);

	if (!bUsingOverviewCamera)
	{
		HandleOverviewTriggerBeginOverlap(OverviewCameraTrigger, PlayerPawn, Cast<UPrimitiveComponent>(PlayerPawn->GetRootComponent()), 0, false, FHitResult());
		return;
	}

	MaintainOverviewCamera(PlayerController);
}

void APushPullLevelActor::RebuildInstances()
{
	ClearGeneratedComponents();

	if (!PushPullAsset)
	{
		return;
	}

	PushPullAsset->EnsureGridSize();
	PushPullAsset->SanitizeDeprecatedTiles();
	PushPullAsset->EnsureDefaultPaletteEntries();

	for (int32 Y = 0; Y < PushPullAsset->GridHeight; ++Y)
	{
		for (int32 X = 0; X < PushPullAsset->GridWidth; ++X)
		{
			const FPushPullGridCell Cell = PushPullAsset->GetCell(X, Y);
			const FPushPullPaletteEntry* Entry = FindPaletteEntry(Cell);
			if (!Entry || Entry->TileKind == EPushPullTileKind::Empty || Entry->TileKind == EPushPullTileKind::Floor)
			{
				continue;
			}

			BuildActorInstance(*Entry, X, Y);
		}
	}

	UpdateOverviewCameraTrigger();
	UpdateOverviewCamera();
}

void APushPullLevelActor::ResetInstance(AActor* ActorToReset)
{
	FTransform PlayerStartTransform;
	const bool bHasPlayerStartTransform = TryGetPlayerStartCellTransform(PlayerStartTransform) || TryGetGeneratedPlayerStartTransform(PlayerStartTransform);
	AActor* ResetActor = ActorToReset ? ActorToReset : UGameplayStatics::GetPlayerPawn(this, 0);

	if (bHasPlayerStartTransform && ResetActor)
	{
		FVector ResetLocation = PlayerStartTransform.GetLocation();
		const FRotator ResetRotation = PlayerStartTransform.Rotator();

		PrepareActorForReset(ResetActor);

		if (ACharacter* Character = Cast<ACharacter>(ResetActor))
		{
			if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
			{
				MovementComponent->StopMovementImmediately();
			}

			if (const UCapsuleComponent* CapsuleComponent = Character->GetCapsuleComponent())
			{
				ResetLocation.Z = GetActorLocation().Z + CapsuleComponent->GetScaledCapsuleHalfHeight();
			}
		}

		if (!ResetActor->TeleportTo(ResetLocation, ResetRotation, false, true))
		{
			ResetActor->SetActorLocationAndRotation(ResetLocation, ResetRotation, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	ResetGeneratedActors();
	RefreshResetOverlaps(ResetActor);
	if (GetWorld())
	{
		FTimerDelegate ResetRefreshDelegate;
		ResetRefreshDelegate.BindUObject(this, &APushPullLevelActor::FinishResetRefresh, ResetActor);
		GetWorld()->GetTimerManager().SetTimerForNextTick(ResetRefreshDelegate);
	}
}

void APushPullLevelActor::HandleResetKey(APlayerController* PlayerController, APawn* PlayerPawn)
{
	if (!PlayerController || !PlayerPawn || !PlayerController->WasInputKeyJustPressed(EKeys::Z))
	{
		return;
	}

	UWidgetBlueprintLibrary::SetInputMode_GameOnly(PlayerController);
	PlayerController->SetShowMouseCursor(false);
	ResetInstance(PlayerPawn);
}

void APushPullLevelActor::FinishResetRefresh(AActor* ResetActor)
{
	RestoreActorInputAfterReset(ResetActor);
	RefreshResetOverlaps(ResetActor);
}

void APushPullLevelActor::RestoreActorInputAfterReset(AActor* ResetActor) const
{
	if (!ResetActor)
	{
		return;
	}

	TArray<UActorComponent*> ActorComponents;
	ResetActor->GetComponents(ActorComponents);
	for (UActorComponent* Component : ActorComponents)
	{
		SetBoolPropertyIfPresent(Component, TEXT("bCanHaveInputs"), true);
		SetBoolPropertyIfPresent(Component, TEXT("bPushing"), false);
		SetBoolPropertyIfPresent(Component, TEXT("bNewMoveInput"), false);
		SetBoolPropertyIfPresent(Component, TEXT("bHighlighted"), false);
	}
}

void APushPullLevelActor::EnsureOverviewCamera()
{
	if (OverviewCamera || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	OverviewCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParameters);
	if (OverviewCamera)
	{
		OverviewCamera->SetActorHiddenInGame(true);
		if (UCameraComponent* CameraComponent = OverviewCamera->GetCameraComponent())
		{
			CameraComponent->SetProjectionMode(ECameraProjectionMode::Orthographic);
		}
	}
}

void APushPullLevelActor::UpdateOverviewCamera()
{
	if (!OverviewCamera || !PushPullAsset)
	{
		return;
	}

	const float WorldWidth = static_cast<float>(PushPullAsset->GridWidth) * PushPullAsset->TileSize;
	const float WorldHeight = static_cast<float>(PushPullAsset->GridHeight) * PushPullAsset->TileSize;
	const float Padding = 0.0f;
	const float AspectRatio = GetViewportAspectRatio();
	const float DesiredOrthoWidth = FMath::Max(WorldWidth + Padding, (WorldHeight + Padding) * AspectRatio) * OverviewOrthoScale;
	const float CameraHeight = FMath::Max(WorldWidth, WorldHeight) + 1000.0f;

	const FVector CameraLocation = GetActorLocation() + FVector(0.0f, 0.0f, CameraHeight);
	const FRotator CameraRotation(-90.0f, GetActorRotation().Yaw - 90.0f, 0.0f);

	OverviewCamera->SetActorLocationAndRotation(CameraLocation, CameraRotation);
	if (UCameraComponent* CameraComponent = OverviewCamera->GetCameraComponent())
	{
		CameraComponent->SetProjectionMode(ECameraProjectionMode::Orthographic);
		CameraComponent->SetOrthoWidth(DesiredOrthoWidth);
	}
}

void APushPullLevelActor::UpdateOverviewCameraTrigger()
{
	if (!OverviewCameraTrigger || !PushPullAsset)
	{
		return;
	}

	if (OverviewCameraTrigger->Mobility != EComponentMobility::Movable)
	{
		OverviewCameraTrigger->SetMobility(EComponentMobility::Movable);
	}

	const float ExtentX = static_cast<float>(PushPullAsset->GridWidth) * PushPullAsset->TileSize * 0.5f;
	const float ExtentY = static_cast<float>(PushPullAsset->GridHeight) * PushPullAsset->TileSize * 0.5f;
	const float ExtentZ = 600.0f;

	OverviewCameraTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, ExtentZ * 0.5f));
	OverviewCameraTrigger->SetBoxExtent(FVector(ExtentX, ExtentY, ExtentZ), true);
}

float APushPullLevelActor::GetViewportAspectRatio() const
{
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		const FIntPoint ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();
		if (ViewportSize.X > 0 && ViewportSize.Y > 0)
		{
			return static_cast<float>(ViewportSize.X) / static_cast<float>(ViewportSize.Y);
		}
	}

	return 16.0f / 9.0f;
}

void APushPullLevelActor::MaintainOverviewCamera(APlayerController* PlayerController)
{
	if (!PlayerController || !OverviewCamera)
	{
		return;
	}

	if (PlayerController->GetViewTarget() != OverviewCamera)
	{
		PlayerController->SetViewTarget(OverviewCamera);
	}

	ApplyOverviewControlRotation(PlayerController);
	APawn* PlayerPawn = PlayerController->GetPawn();
	ApplyCameraRelativeMovementSettings(PlayerPawn, PlayerController);
	ApplyOverviewMoveDirection(PlayerPawn, PlayerController);
	ApplyOverviewCharacterFacing(PlayerController);

	TArray<UChildActorComponent*> ChildActorComponents;
	GetComponents(ChildActorComponents);
	for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		AActor* ChildActor = ChildActorComponent ? ChildActorComponent->GetChildActor() : nullptr;
		if (!ChildActor || !ChildActorComponent->ComponentHasTag(GeneratedComponentTag))
		{
			continue;
		}

		ApplyOverviewPushActorSettings(ChildActor, PlayerController);
		ApplyOverviewMoveDirection(ChildActor, PlayerController);
		if (APawn* PushPawn = Cast<APawn>(ChildActor))
		{
			if (AController* PushController = PushPawn->GetController())
			{
				PushController->SetControlRotation(GetOverviewControlRotation());
			}
		}
	}

	ShowResetTips(PlayerController);
}

FRotator APushPullLevelActor::GetOverviewControlRotation() const
{
	const FVector OverviewForward = GetOverviewForwardVector();
	if (!OverviewForward.IsNearlyZero())
	{
		return FRotator(0.0f, OverviewForward.Rotation().Yaw, 0.0f);
	}

	return FRotator(0.0f, GetActorRotation().Yaw, 0.0f);
}

FVector APushPullLevelActor::GetOverviewForwardVector() const
{
	if (OverviewCamera)
	{
		FVector ScreenForward = OverviewCamera->GetActorUpVector();
		ScreenForward.Z = 0.0f;
		if (ScreenForward.Normalize())
		{
			return ScreenForward;
		}
	}

	return FRotationMatrix(FRotator(0.0f, GetActorRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::X);
}

FVector APushPullLevelActor::GetOverviewRightVector() const
{
	if (OverviewCamera)
	{
		FVector ScreenRight = OverviewCamera->GetActorRightVector();
		ScreenRight.Z = 0.0f;
		if (ScreenRight.Normalize())
		{
			return ScreenRight;
		}
	}

	return FRotationMatrix(FRotator(0.0f, GetActorRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::Y);
}

void APushPullLevelActor::ApplyOverviewControlRotation(APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return;
	}

	const FRotator OverviewControlRotation = GetOverviewControlRotation();
	PlayerController->SetControlRotation(OverviewControlRotation);
}

void APushPullLevelActor::ApplyCameraRelativeMovementSettings(AActor* Actor, APlayerController* PlayerController) const
{
	if (!Actor)
	{
		return;
	}

	SetBoolPropertyIfPresent(Actor, TEXT("bUseCameraRotationMove"), true);
	SetBoolPropertyIfPresent(Actor, TEXT("bUsePushCamera"), false);
	SetObjectPropertyIfPresent(Actor, TEXT("PushController"), PlayerController);

	TArray<UActorComponent*> ActorComponents;
	Actor->GetComponents(ActorComponents);
	for (UActorComponent* Component : ActorComponents)
	{
		SetBoolPropertyIfPresent(Component, TEXT("bUseCameraRotationMove"), true);
		SetBoolPropertyIfPresent(Component, TEXT("bUsePushCamera"), false);
		SetObjectPropertyIfPresent(Component, TEXT("PushController"), PlayerController);
	}
}

void APushPullLevelActor::ApplyOverviewMoveDirection(AActor* Actor, APlayerController* PlayerController) const
{
	if (!Actor || !PlayerController)
	{
		return;
	}

	const float InputForward = GetMappedAxisValue(PlayerController, TEXT("MoveForward"));
	const float InputRight = GetMappedAxisValue(PlayerController, TEXT("MoveRight"));
	const bool bForwardDirection = FMath::Abs(InputForward) >= FMath::Abs(InputRight);
	const bool bForward = bForwardDirection ? InputForward >= 0.0f : InputRight >= 0.0f;
	const FVector OverviewForward = GetOverviewForwardVector();
	const FVector OverviewRight = GetOverviewRightVector();
	FVector MoveDirection = OverviewForward * InputForward + OverviewRight * InputRight;
	MoveDirection.Z = 0.0f;
	MoveDirection = MoveDirection.GetSafeNormal();

	SetBoolPropertyIfPresent(Actor, TEXT("bForwardDirection"), bForwardDirection);
	SetBoolPropertyIfPresent(Actor, TEXT("bForward"), bForward);
	SetVectorPropertyIfPresent(Actor, TEXT("MoveDirection"), MoveDirection);
	SetVectorPropertyIfPresent(Actor, TEXT("MovementDirection"), MoveDirection);

	TArray<UActorComponent*> ActorComponents;
	Actor->GetComponents(ActorComponents);
	for (UActorComponent* Component : ActorComponents)
	{
		SetBoolPropertyIfPresent(Component, TEXT("bForwardDirection"), bForwardDirection);
		SetBoolPropertyIfPresent(Component, TEXT("bForward"), bForward);
		SetVectorPropertyIfPresent(Component, TEXT("MoveDirection"), MoveDirection);
		SetVectorPropertyIfPresent(Component, TEXT("MovementDirection"), MoveDirection);
	}
}

void APushPullLevelActor::ApplyOverviewPushActorSettings(AActor* Actor, APlayerController* PlayerController) const
{
	if (!Actor)
	{
		return;
	}

	const FVector OverviewForward = GetOverviewForwardVector();
	const FVector OverviewRight = GetOverviewRightVector();
	SetBoolPropertyIfPresent(Actor, TEXT("bUseCameraRotationMove"), false);
	SetBoolPropertyIfPresent(Actor, TEXT("bUsePushCamera"), false);
	SetObjectPropertyIfPresent(Actor, TEXT("PushController"), PlayerController);
	SetVectorPropertyIfPresent(Actor, TEXT("CameraForwardDirection"), OverviewForward);
	SetVectorPropertyIfPresent(Actor, TEXT("ForwardDirection"), OverviewForward);
	SetVectorPropertyIfPresent(Actor, TEXT("RightDirection"), OverviewRight);

	TArray<UActorComponent*> ActorComponents;
	Actor->GetComponents(ActorComponents);
	for (UActorComponent* Component : ActorComponents)
	{
		SetBoolPropertyIfPresent(Component, TEXT("bUseCameraRotationMove"), false);
		SetBoolPropertyIfPresent(Component, TEXT("bUsePushCamera"), false);
		SetObjectPropertyIfPresent(Component, TEXT("PushController"), PlayerController);
		SetVectorPropertyIfPresent(Component, TEXT("CameraForwardDirection"), OverviewForward);
		SetVectorPropertyIfPresent(Component, TEXT("ForwardDirection"), OverviewForward);
		SetVectorPropertyIfPresent(Component, TEXT("RightDirection"), OverviewRight);
	}
}

void APushPullLevelActor::ApplyOverviewCharacterFacing(APlayerController* PlayerController) const
{
	APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		return;
	}

	TArray<UActorComponent*> ActorComponents;
	PlayerPawn->GetComponents(ActorComponents);
	for (const UActorComponent* Component : ActorComponents)
	{
		bool bComponentPushing = false;
		if (GetBoolPropertyIfPresent(Component, TEXT("bPushing"), bComponentPushing) && bComponentPushing)
		{
			if (const AActor* GrabbedPushActor = Cast<AActor>(GetObjectPropertyIfPresent(Component, TEXT("GrabbedPushActor"))))
			{
				FVector FacingDirection = GrabbedPushActor->GetActorLocation() - PlayerPawn->GetActorLocation();
				FacingDirection.Z = 0.0f;
				if (FacingDirection.Normalize())
				{
					const FRotator FacingRotation(0.0f, FacingDirection.Rotation().Yaw, 0.0f);
					PlayerPawn->SetActorRotation(FacingRotation);
					PlayerController->SetControlRotation(FacingRotation);
				}
			}
			return;
		}
	}

	const float InputForward = GetMappedAxisValue(PlayerController, TEXT("MoveForward"));
	const float InputRight = GetMappedAxisValue(PlayerController, TEXT("MoveRight"));
	FVector MoveDirection = GetOverviewForwardVector() * InputForward + GetOverviewRightVector() * InputRight;
	MoveDirection.Z = 0.0f;
	if (!MoveDirection.Normalize())
	{
		return;
	}

	PlayerPawn->SetActorRotation(FRotator(0.0f, MoveDirection.Rotation().Yaw, 0.0f));
}

void APushPullLevelActor::StartOverviewCameraTransition(APlayerController* PlayerController)
{
	if (!PlayerController || !GetWorld())
	{
		return;
	}

	HideResetTips();
	GetWorld()->GetTimerManager().ClearTimer(OverviewCameraTransitionTimerHandle);

	if (PlayerController->PlayerCameraManager)
	{
		PlayerController->PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
	}

	FTimerDelegate TransitionFinishedDelegate;
	TransitionFinishedDelegate.BindUObject(this, &APushPullLevelActor::FinishOverviewCameraTransition, PlayerController);
	GetWorld()->GetTimerManager().SetTimer(OverviewCameraTransitionTimerHandle, TransitionFinishedDelegate, OverviewCameraBlendTime, false);
}

void APushPullLevelActor::FinishOverviewCameraTransition(APlayerController* PlayerController)
{
	if (!PlayerController || PlayerController->GetViewTarget() != OverviewCamera)
	{
		return;
	}

	if (PlayerController->PlayerCameraManager)
	{
		PlayerController->PlayerCameraManager->SetManualCameraFade(0.0f, FLinearColor::Black, false);
	}

	ShowResetTips(PlayerController);
}

void APushPullLevelActor::ShowResetTips(APlayerController* PlayerController)
{
	if (!PlayerController || ResetTipsWidget || !ResetTipsWidgetClass)
	{
		return;
	}

	ResetTipsWidget = CreateWidget<UUserWidget>(PlayerController, ResetTipsWidgetClass);
	if (ResetTipsWidget)
	{
		ResetTipsWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		ResetTipsWidget->AddToViewport(10);
	}
}

void APushPullLevelActor::HideResetTips()
{
	if (ResetTipsWidget)
	{
		ResetTipsWidget->RemoveFromParent();
		ResetTipsWidget = nullptr;
	}
}

void APushPullLevelActor::HandleOverviewTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !OverviewCamera || OtherActor != PlayerController->GetPawn())
	{
		return;
	}

	UpdateOverviewCamera();

	if (PlayerController->GetViewTarget() != OverviewCamera)
	{
		PreviousViewTarget = PlayerController->GetViewTarget();
		PreviousControlRotation = PlayerController->GetControlRotation();
		ApplyOverviewControlRotation(PlayerController);
		StartOverviewCameraTransition(PlayerController);
		PlayerController->SetViewTargetWithBlend(OverviewCamera, OverviewCameraBlendTime, VTBlend_Cubic);
		bUsingOverviewCamera = true;
	}
	else
	{
		MaintainOverviewCamera(PlayerController);
	}
}

void APushPullLevelActor::HandleOverviewTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || OtherActor != PlayerController->GetPawn())
	{
		return;
	}

	AActor* RestoreTarget = PreviousViewTarget.IsValid() ? PreviousViewTarget.Get() : PlayerController->GetPawn();
	if (RestoreTarget && PlayerController->GetViewTarget() == OverviewCamera)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(OverviewCameraTransitionTimerHandle);
		}
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->SetManualCameraFade(0.0f, FLinearColor::Black, false);
		}
		HideResetTips();
		PlayerController->SetViewTargetWithBlend(RestoreTarget, OverviewCameraBlendTime, VTBlend_Cubic);
		PlayerController->SetControlRotation(PreviousControlRotation);
	}

	bUsingOverviewCamera = false;
	PreviousViewTarget.Reset();
}

void APushPullLevelActor::PrepareActorForReset(AActor* ActorToReset, bool bDetachActor) const
{
	if (!ActorToReset)
	{
		return;
	}

	if (bDetachActor)
	{
		ActorToReset->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	static const FName PushStateFunctionNames[] =
	{
		TEXT("InteractReleased"),
		TEXT("EndPush"),
		TEXT("ExitPushState"),
		TEXT("RemovePushUI")
	};

	static const FName BoolStateNames[] =
	{
		TEXT("bPushing"),
		TEXT("bNewMoveInput"),
		TEXT("bHighlighted")
	};

	static const FName BoolReadyStateNames[] =
	{
		TEXT("bCanHaveInputs")
	};

	static const FName ObjectStateNames[] =
	{
		TEXT("GrabbedPushActor"),
		TEXT("HighlightedPush"),
		TEXT("PushUI")
	};

	static const FName ArrayStateNames[] =
	{
		TEXT("PushActors")
	};

	TArray<UActorComponent*> ActorComponents;
	ActorToReset->GetComponents(ActorComponents);

	auto ResetPushStateObject = [this](UObject* Object)
	{
		if (!Object)
		{
			return;
		}

		for (const FName FunctionName : PushStateFunctionNames)
		{
			if (UFunction* Function = Object->FindFunction(FunctionName))
			{
				if (Function->NumParms == 0)
				{
					Object->ProcessEvent(Function, nullptr);
				}
			}
		}

		for (const FName BoolStateName : BoolStateNames)
		{
			if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), BoolStateName))
			{
				BoolProperty->SetPropertyValue_InContainer(Object, false);
			}
		}

		for (const FName BoolReadyStateName : BoolReadyStateNames)
		{
			if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), BoolReadyStateName))
			{
				BoolProperty->SetPropertyValue_InContainer(Object, true);
			}
		}

		for (const FName ObjectStateName : ObjectStateNames)
		{
			if (FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), ObjectStateName))
			{
				ObjectProperty->SetObjectPropertyValue_InContainer(Object, nullptr);
			}
		}

		for (const FName ArrayStateName : ArrayStateNames)
		{
			ClearArrayPropertyIfPresent(Object, ArrayStateName);
		}
	};

	ResetPushStateObject(ActorToReset);
	for (UActorComponent* Component : ActorComponents)
	{
		ResetPushStateObject(Component);
	}
}

void APushPullLevelActor::ResetGeneratedActors()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	TArray<UChildActorComponent*> ChildActorComponents;
	GetComponents(ChildActorComponents);
	for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		AActor* ChildActor = ChildActorComponent ? ChildActorComponent->GetChildActor() : nullptr;
		if (!ChildActor || !ChildActorComponent->ComponentHasTag(GeneratedComponentTag))
		{
			continue;
		}

		if (!ChildActorComponent->ComponentHasTag(GeneratedPushBoxComponentTag))
		{
			continue;
		}

		if (APawn* PushPawn = Cast<APawn>(ChildActor))
		{
			if (UPawnMovementComponent* MovementComponent = PushPawn->GetMovementComponent())
			{
				MovementComponent->StopMovementImmediately();
			}
		}

		TArray<UPrimitiveComponent*> PrimitiveComponents;
		ChildActor->GetComponents(PrimitiveComponents);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (!PrimitiveComponent)
			{
				continue;
			}

			PrimitiveComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			PrimitiveComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}

		ChildActor->SetActorTransform(ChildActorComponent->GetComponentTransform(), false, nullptr, ETeleportType::TeleportPhysics);

		PrepareActorForReset(ChildActor, false);
		ConfigurePushBoxCollision(ChildActor);
		ApplyOverviewPushActorSettings(ChildActor, PlayerController);
	}
}

void APushPullLevelActor::RefreshResetOverlaps(AActor* ResetActor) const
{
	if (!ResetActor)
	{
		return;
	}

	TArray<UPrimitiveComponent*> ResetPrimitiveComponents;
	ResetActor->GetComponents(ResetPrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : ResetPrimitiveComponents)
	{
		if (PrimitiveComponent)
		{
			PrimitiveComponent->UpdateOverlaps();
		}
	}

	TArray<UChildActorComponent*> ChildActorComponents;
	GetComponents(ChildActorComponents);
	for (const UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		AActor* ChildActor = ChildActorComponent ? ChildActorComponent->GetChildActor() : nullptr;
		if (!ChildActor || !ChildActorComponent->ComponentHasTag(GeneratedComponentTag))
		{
			continue;
		}

		TArray<UPrimitiveComponent*> ChildPrimitiveComponents;
		ChildActor->GetComponents(ChildPrimitiveComponents);
		for (UPrimitiveComponent* PrimitiveComponent : ChildPrimitiveComponents)
		{
			if (PrimitiveComponent)
			{
				PrimitiveComponent->UpdateOverlaps();
			}
		}
	}
}

void APushPullLevelActor::ClearGeneratedComponents()
{
	TArray<UActorComponent*> ComponentsToRemove;
	GetComponents(ComponentsToRemove);

	for (UActorComponent* Component : ComponentsToRemove)
	{
		if (Component && Component->ComponentHasTag(GeneratedComponentTag))
		{
			Component->DestroyComponent();
		}
	}
}

void APushPullLevelActor::BuildActorInstance(const FPushPullPaletteEntry& Entry, int32 X, int32 Y)
{
	const TSubclassOf<AActor> ActorClass = ResolveActorClass(Entry);
	if (!ActorClass)
	{
		return;
	}

	const FName ComponentName = MakeUniqueObjectName(this, UChildActorComponent::StaticClass(), TEXT("GeneratedActor"));
	UChildActorComponent* ChildActorComponent = NewObject<UChildActorComponent>(this, ComponentName, RF_Transactional);
	ChildActorComponent->ComponentTags.Add(GeneratedComponentTag);
	const bool bIsPushBox = Entry.TileKind == EPushPullTileKind::PushBox;
	if (bIsPushBox)
	{
		ChildActorComponent->ComponentTags.Add(GeneratedPushBoxComponentTag);
	}
	ChildActorComponent->SetMobility(bIsPushBox ? EComponentMobility::Movable : EComponentMobility::Static);
	ChildActorComponent->SetChildActorClass(ActorClass);
	ChildActorComponent->SetRelativeTransform(MakeCellTransform(X, Y));
	ChildActorComponent->SetupAttachment(SceneRoot);
	ChildActorComponent->RegisterComponent();
	if (bIsPushBox)
	{
		if (AActor* ChildActor = ChildActorComponent->GetChildActor())
		{
			TArray<USceneComponent*> SceneComponents;
			ChildActor->GetComponents(SceneComponents);
			for (USceneComponent* SceneComponent : SceneComponents)
			{
				if (SceneComponent)
				{
					SceneComponent->SetMobility(EComponentMobility::Movable);
				}
			}

			ConfigurePushBoxCollision(ChildActor);
			ApplyOverviewPushActorSettings(ChildActor, nullptr);
		}
	}
	if (ABP_Goal* GoalActor = Cast<ABP_Goal>(ChildActorComponent->GetChildActor()))
	{
		GoalActor->SetOwningLevelActor(this);
	}
	AlignChildActorBottom(ChildActorComponent);
	AddInstanceComponent(ChildActorComponent);
}

void APushPullLevelActor::ConfigurePushBoxCollision(AActor* PushBoxActor) const
{
	if (!PushBoxActor)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	PushBoxActor->GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		const ECollisionResponse OriginalPawnResponse = PrimitiveComponent->GetCollisionResponseToChannel(ECC_Pawn);
		PrimitiveComponent->SetCollisionObjectType(ECC_WorldDynamic);
		PrimitiveComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		PrimitiveComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, OriginalPawnResponse);
	}

	TArray<UMeshComponent*> MeshComponents;
	PushBoxActor->GetComponents(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		for (int32 MaterialIndex = 0; MaterialIndex < MeshComponent->GetNumMaterials(); ++MaterialIndex)
		{
			UMaterialInterface* SourceMaterial = MeshComponent->GetMaterial(MaterialIndex);
			if (!SourceMaterial)
			{
				continue;
			}

			UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, MeshComponent);
			if (!DynamicMaterial)
			{
				continue;
			}

			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Black);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::Black);
			DynamicMaterial->SetVectorParameterValue(TEXT("Tint"), FLinearColor::Black);
			DynamicMaterial->SetVectorParameterValue(TEXT("Albedo"), FLinearColor::Black);
			MeshComponent->SetMaterial(MaterialIndex, DynamicMaterial);
		}
	}
}

void APushPullLevelActor::AlignChildActorBottom(UChildActorComponent* ChildActorComponent) const
{
	if (!ChildActorComponent)
	{
		return;
	}

	AActor* ChildActor = ChildActorComponent->GetChildActor();
	if (!ChildActor)
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	float BottomZ = 0.0f;
	if (!TryGetVisibleMeshBottomZ(ChildActor, BottomZ))
	{
		ChildActor->GetActorBounds(false, Origin, Extent);
		BottomZ = Origin.Z - Extent.Z;
	}

	const float DesiredBottomZ = GetActorLocation().Z;
	const float OffsetZ = DesiredBottomZ - BottomZ;
	if (!FMath::IsNearlyZero(OffsetZ))
	{
		const EComponentMobility::Type OriginalMobility = ChildActorComponent->Mobility;
		ChildActorComponent->SetMobility(EComponentMobility::Movable);
		ChildActorComponent->AddWorldOffset(FVector(0.0f, 0.0f, OffsetZ));
		ChildActorComponent->SetMobility(OriginalMobility);
	}
}

bool APushPullLevelActor::TryGetVisibleMeshBottomZ(const AActor* Actor, float& OutBottomZ) const
{
	if (!Actor)
	{
		return false;
	}

	TArray<UStaticMeshComponent*> MeshComponents;
	Actor->GetComponents(MeshComponents);

	bool bFoundMesh = false;
	float BottomZ = TNumericLimits<float>::Max();
	for (const UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent || !MeshComponent->GetStaticMesh() || !MeshComponent->IsVisible())
		{
			continue;
		}

		const FBoxSphereBounds MeshBounds = MeshComponent->Bounds;
		BottomZ = FMath::Min(BottomZ, MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z);
		bFoundMesh = true;
	}

	if (!bFoundMesh)
	{
		return false;
	}

	OutBottomZ = BottomZ;
	return true;
}

bool APushPullLevelActor::TryGetGeneratedPlayerStartTransform(FTransform& OutTransform) const
{
	TArray<UChildActorComponent*> ChildActorComponents;
	GetComponents(ChildActorComponents);

	for (const UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		if (!ChildActorComponent || !ChildActorComponent->ComponentHasTag(GeneratedComponentTag))
		{
			continue;
		}

		if (const APlayerStart* PlayerStart = Cast<APlayerStart>(ChildActorComponent->GetChildActor()))
		{
			OutTransform = PlayerStart->GetActorTransform();
			return true;
		}
	}

	return false;
}

bool APushPullLevelActor::TryGetPlayerStartCellTransform(FTransform& OutTransform) const
{
	if (!PushPullAsset)
	{
		return false;
	}

	for (int32 Y = 0; Y < PushPullAsset->GridHeight; ++Y)
	{
		for (int32 X = 0; X < PushPullAsset->GridWidth; ++X)
		{
			const FPushPullGridCell Cell = PushPullAsset->GetCell(X, Y);
			if (Cell.TileKind == EPushPullTileKind::PlayerStart)
			{
				OutTransform = MakeCellTransform(X, Y) * GetActorTransform();
				return true;
			}
		}
	}

	return false;
}

TSubclassOf<AActor> APushPullLevelActor::ResolveActorClass(const FPushPullPaletteEntry& Entry) const
{
	if (Entry.TileKind == EPushPullTileKind::PushBox)
	{
		if (!Entry.ActorClass.IsNull())
		{
			if (TSubclassOf<AActor> EntryClass = Entry.ActorClass.LoadSynchronous())
			{
				if (EntryClass != ABP_Box::StaticClass())
				{
					return EntryClass;
				}
			}
		}

		if (PushPullAsset && !PushPullAsset->DefaultPushBoxClass.IsNull())
		{
			if (TSubclassOf<AActor> DefaultPushBoxClass = PushPullAsset->DefaultPushBoxClass.LoadSynchronous())
			{
				return DefaultPushBoxClass;
			}
		}

		return ABP_Box::StaticClass();
	}

	if (!Entry.ActorClass.IsNull())
	{
		if (TSubclassOf<AActor> EntryClass = Entry.ActorClass.LoadSynchronous())
		{
			return EntryClass;
		}
	}

	switch (Entry.TileKind)
	{
	case EPushPullTileKind::Floor:
		return nullptr;
	case EPushPullTileKind::Wall:
		return ABP_Wall::StaticClass();
	case EPushPullTileKind::Goal:
		return ABP_Goal::StaticClass();
	case EPushPullTileKind::PlayerStart:
		return APlayerStart::StaticClass();
	case EPushPullTileKind::PushBox:
		return nullptr;
	case EPushPullTileKind::Empty:
	default:
		return nullptr;
	}
}

FTransform APushPullLevelActor::MakeCellTransform(int32 X, int32 Y) const
{
	const float TileSize = PushPullAsset ? PushPullAsset->TileSize : 200.0f;
	const float CenterX = PushPullAsset ? (static_cast<float>(PushPullAsset->GridWidth - 1) * 0.5f) : 0.0f;
	const float CenterY = PushPullAsset ? (static_cast<float>(PushPullAsset->GridHeight - 1) * 0.5f) : 0.0f;
	const FVector Location((static_cast<float>(X) - CenterX) * TileSize, (static_cast<float>(Y) - CenterY) * TileSize, 0.0f);
	return FTransform(FRotator::ZeroRotator, Location, FVector::OneVector);
}

void APushPullLevelActor::SetBoolPropertyIfPresent(UObject* Object, FName PropertyName, bool bValue)
{
	if (!Object)
	{
		return;
	}

	if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName))
	{
		BoolProperty->SetPropertyValue_InContainer(Object, bValue);
	}
}

void APushPullLevelActor::ClearArrayPropertyIfPresent(UObject* Object, FName PropertyName)
{
	if (!Object)
	{
		return;
	}

	if (const FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName))
	{
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
		ArrayHelper.EmptyValues();
	}
}

bool APushPullLevelActor::GetBoolPropertyIfPresent(const UObject* Object, FName PropertyName, bool& OutValue)
{
	if (!Object)
	{
		return false;
	}

	if (const FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName))
	{
		OutValue = BoolProperty->GetPropertyValue_InContainer(Object);
		return true;
	}

	return false;
}

UObject* APushPullLevelActor::GetObjectPropertyIfPresent(const UObject* Object, FName PropertyName)
{
	if (!Object)
	{
		return nullptr;
	}

	if (const FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName))
	{
		return ObjectProperty->GetObjectPropertyValue_InContainer(Object);
	}

	return nullptr;
}

void APushPullLevelActor::SetObjectPropertyIfPresent(UObject* Object, FName PropertyName, UObject* Value)
{
	if (!Object)
	{
		return;
	}

	if (FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName))
	{
		if (!Value || !ObjectProperty->PropertyClass || Value->IsA(ObjectProperty->PropertyClass))
		{
			ObjectProperty->SetObjectPropertyValue_InContainer(Object, Value);
		}
	}
}

void APushPullLevelActor::SetVectorPropertyIfPresent(UObject* Object, FName PropertyName, const FVector& Value)
{
	if (!Object)
	{
		return;
	}

	if (FStructProperty* StructProperty = FindFProperty<FStructProperty>(Object->GetClass(), PropertyName))
	{
		if (StructProperty->Struct == TBaseStructure<FVector>::Get())
		{
			*StructProperty->ContainerPtrToValuePtr<FVector>(Object) = Value;
		}
	}
}

float APushPullLevelActor::GetMappedAxisValue(const APlayerController* PlayerController, FName AxisName)
{
	if (!PlayerController)
	{
		return 0.0f;
	}

	const UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (!InputSettings)
	{
		return 0.0f;
	}

	TArray<FInputAxisKeyMapping> AxisMappings;
	InputSettings->GetAxisMappingByName(AxisName, AxisMappings);

	float AxisValue = 0.0f;
	for (const FInputAxisKeyMapping& AxisMapping : AxisMappings)
	{
		const FKey& Key = AxisMapping.Key;
		const float KeyValue = Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D()
			? PlayerController->GetInputAnalogKeyState(Key)
			: (PlayerController->IsInputKeyDown(Key) ? 1.0f : 0.0f);

		AxisValue += KeyValue * AxisMapping.Scale;
	}

	return FMath::Clamp(AxisValue, -1.0f, 1.0f);
}

const FPushPullPaletteEntry* APushPullLevelActor::FindPaletteEntry(const FPushPullGridCell& Cell) const
{
	if (!PushPullAsset)
	{
		return nullptr;
	}

	if (!Cell.PaletteId.IsNone())
	{
		if (const FPushPullPaletteEntry* Entry = PushPullAsset->Palette.FindByPredicate([&Cell](const FPushPullPaletteEntry& PaletteEntry)
		{
			return PaletteEntry.Id == Cell.PaletteId;
		}))
		{
			return Entry;
		}
	}

	return PushPullAsset->Palette.FindByPredicate([&Cell](const FPushPullPaletteEntry& PaletteEntry)
	{
		return PaletteEntry.TileKind == Cell.TileKind;
	});
}
