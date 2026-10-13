// Copyright 2015 Greg Mladucky

#include "PushPullGeneratedActors.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/ChildActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "PushPullLevelActor.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "PushPullGeneratedActors"

namespace PushPullGeneratedActorHelpers
{
	UStaticMesh* GetCubeMesh()
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
		return CubeMeshFinder.Object;
	}

	UMaterialInterface* GetBaseMaterial()
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		return MaterialFinder.Object;
	}

	UMaterialInterface* GetGoalMaterial()
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/EditorMaterials/WidgetMaterial_Y.WidgetMaterial_Y"));
		return MaterialFinder.Object;
	}

	void ConfigureCube(UStaticMeshComponent* MeshComponent, const FVector& Scale, bool bOverlapPawn)
	{
		if (!MeshComponent)
		{
			return;
		}

		MeshComponent->SetStaticMesh(GetCubeMesh());
		MeshComponent->SetMaterial(0, GetBaseMaterial());
		MeshComponent->SetRelativeScale3D(Scale);
		MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, Scale.Z * 50.0f));
		MeshComponent->SetMobility(EComponentMobility::Static);

		if (bOverlapPawn)
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
			MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
			MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			MeshComponent->SetGenerateOverlapEvents(true);
		}
		else
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
			MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
		}
	}
}

void UPushPullWidget::InitializeForGoal(ABP_Goal* InGoal, APushPullLevelActor* InLevelActor)
{
	OwningGoal = InGoal;
	OwningLevelActor = InLevelActor;
}

void UPushPullWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Reset)
	{
		Reset = Cast<UButton>(GetWidgetFromName(TEXT("Reset")));
	}

	if (!ExitGame)
	{
		ExitGame = Cast<UButton>(GetWidgetFromName(TEXT("ExitGame")));
	}

	if (Reset)
	{
		Reset->OnClicked.RemoveDynamic(this, &UPushPullWidget::HandleResetClicked);
		Reset->OnClicked.AddDynamic(this, &UPushPullWidget::HandleResetClicked);
	}

	if (ExitGame)
	{
		ExitGame->OnClicked.RemoveDynamic(this, &UPushPullWidget::HandleExitGameClicked);
		ExitGame->OnClicked.AddDynamic(this, &UPushPullWidget::HandleExitGameClicked);
	}
}

void UPushPullWidget::HandleResetClicked()
{
	ABP_Goal* GoalToReset = OwningGoal;
	APlayerController* PlayerController = GetOwningPlayer();
	APawn* PawnToReset = PlayerController ? PlayerController->GetPawn() : nullptr;
	APushPullLevelActor* LevelActorToReset = ResolveLevelActorForReset(PawnToReset);

	RestoreGameInput();
	RemoveFromParent();

	if (GoalToReset)
	{
		GoalToReset->ClearFinishWidget(this);
	}

	if (LevelActorToReset)
	{
		LevelActorToReset->ResetInstance(PawnToReset);
	}
}

APushPullLevelActor* UPushPullWidget::ResolveLevelActorForReset(AActor* ResetActor) const
{
	if (OwningLevelActor)
	{
		return OwningLevelActor;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector ReferenceLocation = OwningGoal
		? OwningGoal->GetActorLocation()
		: ResetActor
			? ResetActor->GetActorLocation()
			: FVector::ZeroVector;

	APushPullLevelActor* BestLevelActor = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (TActorIterator<APushPullLevelActor> It(World); It; ++It)
	{
		APushPullLevelActor* Candidate = *It;
		if (!Candidate || !Candidate->PushPullAsset)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(Candidate->GetActorLocation(), ReferenceLocation);
		if (!BestLevelActor || DistanceSquared < BestDistanceSquared)
		{
			BestLevelActor = Candidate;
			BestDistanceSquared = DistanceSquared;
		}
	}

	return BestLevelActor;
}

void UPushPullWidget::HandleExitGameClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UPushPullWidget::RestoreGameInput()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(PlayerController);
		PlayerController->SetShowMouseCursor(false);
	}
}

ABP_Floor::ABP_Floor()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor_Deprecated"));
	RootComponent = MeshComponent;
	PushPullGeneratedActorHelpers::ConfigureCube(MeshComponent, FVector(2.0f, 2.0f, 0.1f), false);
}

ABP_Wall::ABP_Wall()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall_2x2x2"));
	RootComponent = MeshComponent;
	PushPullGeneratedActorHelpers::ConfigureCube(MeshComponent, FVector(2.0f, 2.0f, 2.0f), false);
}

ABP_Box::ABP_Box()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Box_2x2x2"));
	RootComponent = MeshComponent;
	PushPullGeneratedActorHelpers::ConfigureCube(MeshComponent, FVector(2.0f, 2.0f, 2.0f), false);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
}

ABP_Goal::ABP_Goal()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Goal_1x1x1"));
	RootComponent = MeshComponent;
	FinishWidgetClass = UPushPullWidget::StaticClass();

	if (FPackageName::DoesPackageExist(TEXT("/Game/UI_PushPull")))
	{
		static ConstructorHelpers::FClassFinder<UPushPullWidget> PushPullWidgetFinder(TEXT("/Game/UI_PushPull"));
		if (PushPullWidgetFinder.Succeeded())
		{
			FinishWidgetClass = PushPullWidgetFinder.Class;
		}
	}

	PushPullGeneratedActorHelpers::ConfigureCube(MeshComponent, FVector(1.0f, 1.0f, 1.0f), true);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MeshComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	MeshComponent->SetGenerateOverlapEvents(true);
	MeshComponent->SetMaterial(0, PushPullGeneratedActorHelpers::GetGoalMaterial());
}

void ABP_Goal::SetOwningLevelActor(APushPullLevelActor* InLevelActor)
{
	OwningLevelActor = InLevelActor;
}

void ABP_Goal::ClearFinishWidget(UPushPullWidget* Widget)
{
	if (!Widget || FinishWidget == Widget)
	{
		FinishWidget = nullptr;
	}
}

void ABP_Goal::BeginPlay()
{
	Super::BeginPlay();
	OwningLevelActor = ResolveOwningLevelActor();
	if (MeshComponent)
	{
		MeshComponent->OnComponentBeginOverlap.AddDynamic(this, &ABP_Goal::HandleGoalOverlap);
	}
}

void ABP_Goal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FinishWidget)
	{
		return;
	}

	ACharacter* Character = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!Character)
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	GetActorBounds(false, Origin, Extent);

	const FVector Delta = Character->GetActorLocation() - Origin;
	const float CapsuleRadius = Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.0f;
	if (FMath::Abs(Delta.X) <= Extent.X + CapsuleRadius && FMath::Abs(Delta.Y) <= Extent.Y + CapsuleRadius)
	{
		TryShowFinishWidget(Character);
	}
}

APushPullLevelActor* ABP_Goal::ResolveOwningLevelActor() const
{
	if (OwningLevelActor)
	{
		return OwningLevelActor;
	}

	if (APushPullLevelActor* OwnerLevelActor = Cast<APushPullLevelActor>(GetOwner()))
	{
		return OwnerLevelActor;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<APushPullLevelActor> It(World); It; ++It)
	{
		APushPullLevelActor* Candidate = *It;
		if (!Candidate)
		{
			continue;
		}

		TArray<UChildActorComponent*> ChildActorComponents;
		Candidate->GetComponents(ChildActorComponents);
		for (const UChildActorComponent* ChildActorComponent : ChildActorComponents)
		{
			if (ChildActorComponent && ChildActorComponent->GetChildActor() == this)
			{
				return Candidate;
			}
		}
	}

	return nullptr;
}

void ABP_Goal::HandleGoalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryShowFinishWidget(OtherActor);
}

void ABP_Goal::TryShowFinishWidget(AActor* OtherActor)
{
	if (FinishWidget || !OtherActor || !OtherActor->IsA<ACharacter>())
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !FinishWidgetClass)
	{
		return;
	}

	FinishWidget = CreateWidget<UPushPullWidget>(PlayerController, FinishWidgetClass);
	if (FinishWidget)
	{
		FinishWidget->InitializeForGoal(this, OwningLevelActor);
		FinishWidget->AddToViewport(100);
		FinishWidget->SetIsFocusable(true);
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PlayerController, FinishWidget);
		PlayerController->SetShowMouseCursor(true);
	}
}

#undef LOCTEXT_NAMESPACE
