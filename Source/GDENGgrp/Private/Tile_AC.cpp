// Fill out your copyright notice in the Description page of Project Settings..


#include "Tile_AC.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Text3DComponent.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values for this component's properties
UTile_AC::UTile_AC()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UTile_AC::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (bOnlySpawnIfOwnerHasTag && !Owner->ActorHasTag(SpawnOriginTag))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UClass* ClassToSpawn = Owner->GetClass();
	if (!ClassToSpawn)
	{
		return;
	}

	const FVector Origin = Owner->GetActorLocation();
	FRotator SpawnRotation = Owner->GetActorRotation();

	float EffectiveSpacingX = Spacing;
	float EffectiveSpacingY = Spacing;

	if (UStaticMeshComponent* MeshComp = Owner->FindComponentByClass<UStaticMeshComponent>())
	{
		const FVector WorldHalfExtents = MeshComp->Bounds.BoxExtent;
		const float FullX = WorldHalfExtents.X * 2.0f;
		const float FullY = WorldHalfExtents.Y * 2.0f;

		EffectiveSpacingX = FMath::Max(Spacing, FullX + ExtraPadding);
		EffectiveSpacingY = FMath::Max(Spacing, FullY + ExtraPadding);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Prepare container for all grid actors (owner at 0,0)
	const int32 TotalCells = GridSizeX * GridSizeY;
	TArray<AActor*> GridActors;
	GridActors.SetNum(TotalCells);

	auto IndexForXY = [&](int32 X, int32 Y) -> int32 { return X * GridSizeY + Y; };

	if (TotalCells > 0)
	{
		GridActors[IndexForXY(0, 0)] = Owner;
	}

	for (int32 X = 0; X < GridSizeX; ++X)
	{
		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			if (X == 0 && Y == 0)
			{
				continue;
			}

			const FVector SpawnLocation = Origin + FVector(X * EffectiveSpacingX, Y * EffectiveSpacingY, 0.f);
			AActor* Spawned = World->SpawnActor<AActor>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
			if (Spawned)
			{
				GridActors[IndexForXY(X, Y)] = Spawned;
			}
			else
			{
				// If spawn failed, leave nullptr.
				GridActors[IndexForXY(X, Y)] = nullptr;
			}
		}
	}

	if (TotalCells == 0)
	{
		return;
	}

	const int32 StartIndex = IndexForXY(0, 0);

	int32 DesiredMines = FMath::Clamp(4, 0, TotalCells > 1 ? TotalCells - 1 : 0);
	NumMines = DesiredMines;

	TSet<int32> MineIndices;
	while (MineIndices.Num() < DesiredMines)
	{
		int32 Pick = FMath::RandRange(0, TotalCells - 1);
		if (Pick == StartIndex)
		{
			continue;
		}
		MineIndices.Add(Pick);
	}

	TArray<int32> AdjacentCounts;
	AdjacentCounts.Init(0, TotalCells);

	for (int32 X = 0; X < GridSizeX; ++X)
	{
		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			int32 Idx = IndexForXY(X, Y);
			if (MineIndices.Contains(Idx))
			{
				AdjacentCounts[Idx] = -1;
				continue;
			}

			int32 Count = 0;
			for (int32 dX = -1; dX <= 1; ++dX)
			{
				for (int32 dY = -1; dY <= 1; ++dY)
				{
					if (dX == 0 && dY == 0) continue;
					int32 nX = X + dX;
					int32 nY = Y + dY;
					if (nX >= 0 && nX < GridSizeX && nY >= 0 && nY < GridSizeY)
					{
						int32 nIdx = IndexForXY(nX, nY);
						if (MineIndices.Contains(nIdx))
						{
							++Count;
						}
					}
				}
			}
			AdjacentCounts[Idx] = Count;
		}
	}

	MineIndices.Remove(StartIndex);

	for (int32 X = 0; X < GridSizeX; ++X)
	{
		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			int32 Idx = IndexForXY(X, Y);
			AActor* CellActor = GridActors[Idx];

			UTile_AC* TileComp = CellActor->FindComponentByClass<UTile_AC>();
			bool bMine = MineIndices.Contains(Idx);
			int32 Count = AdjacentCounts[Idx];
			bool bStart = (Idx == StartIndex);

			if (TileComp)
			{
				TileComp->InitializeTile(Count, bMine, TextUniformScale, bStart);
			}
			else
			{
				UText3DComponent* TextComp = NewObject<UText3DComponent>(CellActor, UText3DComponent::StaticClass(), NAME_None, RF_Transient);
				if (TextComp)
				{
					CellActor->AddInstanceComponent(TextComp);
					USceneComponent* Root = CellActor->GetRootComponent();
					if (!Root)
					{
						CellActor->SetRootComponent(TextComp);
					}
					else
					{
						TextComp->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
					}

					TextComp->SetMobility(EComponentMobility::Movable);
					TextComp->SetHorizontalAlignment(EText3DHorizontalTextAlignment::Center);
					TextComp->SetVerticalAlignment(EText3DVerticalTextAlignment::Center);
					TextComp->SetVisibility(false);

					FString Symbol;
					if (Count == -1)
					{
						Symbol = TEXT("X");
					}
					else
					{
						Symbol = FString::FromInt(Count);
					}

					TextComp->SetText(FText::FromString(Symbol));
					TextComp->SetExtrude(4.0f);
					TextComp->SetBevel(0.0f);
					TextComp->SetCastShadow(false);

					FVector DesiredWorldTop = FVector::ZeroVector;
					if (UStaticMeshComponent* MeshComp = CellActor->FindComponentByClass<UStaticMeshComponent>())
					{
						const FVector MeshWorldCenter = MeshComp->Bounds.Origin;
						const float MeshTopZ = MeshWorldCenter.Z + MeshComp->Bounds.BoxExtent.Z + 10.f;
						DesiredWorldTop = FVector(MeshWorldCenter.X, MeshWorldCenter.Y, MeshTopZ);
					}
					else
					{
						DesiredWorldTop = CellActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
					}

					FVector LocalTop = FVector::ZeroVector;
					USceneComponent* TargetRoot = CellActor->GetRootComponent();
					if (TargetRoot)
					{
						LocalTop = TargetRoot->GetComponentTransform().InverseTransformPosition(DesiredWorldTop);
					}
					else
					{
						LocalTop = CellActor->GetActorTransform().InverseTransformPosition(DesiredWorldTop);
					}

					TextComp->SetRelativeLocation(LocalTop);
					TextComp->SetRelativeScale3D(FVector(TextUniformScale));

					TextComp->RegisterComponent();
					TextComp->InitializeComponent();
				}
			}
		}
	}
}


// Called every frame
void UTile_AC::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UTile_AC::InitializeTile(int32 InAdjacentCount, bool bInIsMine, float InTextUniformScale, bool bInIsStart)
{
	bIsMine = bInIsMine;
	AdjacentCount = InAdjacentCount;
	bRevealed = false;
	bIsStart = bInIsStart;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UText3DComponent* Comp = Owner->FindComponentByClass<UText3DComponent>();
	if (!Comp)
	{
		Comp = NewObject<UText3DComponent>(Owner, UText3DComponent::StaticClass(), NAME_None, RF_Transient);
		if (!Comp)
		{
			return;
		}

		Owner->AddInstanceComponent(Comp);
		USceneComponent* Root = Owner->GetRootComponent();
		if (!Root)
		{
			Owner->SetRootComponent(Comp);
		}
		else
		{
			Comp->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
		}

		Comp->SetMobility(EComponentMobility::Movable);
		Comp->CreationMethod = EComponentCreationMethod::Instance;
		Comp->SetHorizontalAlignment(EText3DHorizontalTextAlignment::Center);
		Comp->SetVerticalAlignment(EText3DVerticalTextAlignment::Center);

		Comp->RegisterComponent();
		Comp->InitializeComponent();
	}

	FString Symbol;
	if (AdjacentCount == -1)
	{
		Symbol = TEXT("X");
	}
	else
	{
		Symbol = FString::FromInt(AdjacentCount);
	}

	Comp->SetText(FText::FromString(Symbol));
	Comp->SetExtrude(4.0f);
	Comp->SetBevel(0.0f);
	Comp->SetCastShadow(false);

	FVector DesiredWorldTop = Owner->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	if (UStaticMeshComponent* MeshComp = Owner->FindComponentByClass<UStaticMeshComponent>())
	{
		const FVector MeshWorldCenter = MeshComp->Bounds.Origin;
		const float MeshTopZ = MeshWorldCenter.Z + MeshComp->Bounds.BoxExtent.Z + 10.f;
		DesiredWorldTop = FVector(MeshWorldCenter.X, MeshWorldCenter.Y, MeshTopZ);
	}

	FVector LocalTop = FVector::ZeroVector;
	USceneComponent* Root = Owner->GetRootComponent();
	if (Root)
	{
		LocalTop = Root->GetComponentTransform().InverseTransformPosition(DesiredWorldTop);
	}
	else
	{
		LocalTop = Owner->GetActorTransform().InverseTransformPosition(DesiredWorldTop);
	}

	Comp->SetRelativeLocation(LocalTop);
	Comp->SetRelativeScale3D(FVector(InTextUniformScale));
	Comp->SetVisibility(false);

	this->RuntimeTextComp = Comp;
}


bool UTile_AC::Reveal()
{
	if (bRevealed)
	{
		return bIsMine;
	}

	bRevealed = true;

	if (!RuntimeTextComp)
	{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			RuntimeTextComp = Owner->FindComponentByClass<UText3DComponent>();
			if (!RuntimeTextComp)
			{
				RuntimeTextComp = NewObject<UText3DComponent>(Owner, UText3DComponent::StaticClass(), NAME_None, RF_Transient);
				if (RuntimeTextComp)
				{
					Owner->AddInstanceComponent(RuntimeTextComp);

					RuntimeTextComp->SetHorizontalAlignment(EText3DHorizontalTextAlignment::Center);
					RuntimeTextComp->SetVerticalAlignment(EText3DVerticalTextAlignment::Center);

					// compute top-of-mesh in world space then transform into Owner root local
					FVector DesiredWorldTop = Owner->GetActorLocation() + FVector(0.f, 0.f, 50.f);
					if (UStaticMeshComponent* MeshComp = Owner->FindComponentByClass<UStaticMeshComponent>())
					{
						const FVector MeshWorldCenter = MeshComp->Bounds.Origin;
						const float MeshTopZ = MeshWorldCenter.Z + MeshComp->Bounds.BoxExtent.Z + 10.f;
						DesiredWorldTop = FVector(MeshWorldCenter.X, MeshWorldCenter.Y, MeshTopZ);
					}

					FVector LocalTop = FVector::ZeroVector;
					USceneComponent* Root = Owner->GetRootComponent();
					if (Root)
					{
						LocalTop = Root->GetComponentTransform().InverseTransformPosition(DesiredWorldTop);
					}
					else
					{
						LocalTop = Owner->GetActorTransform().InverseTransformPosition(DesiredWorldTop);
					}
					RuntimeTextComp->SetRelativeLocation(LocalTop);

					RuntimeTextComp->RegisterComponent();
					RuntimeTextComp->InitializeComponent();
				}
			}
		}
	}

	if (RuntimeTextComp)
	{
		RuntimeTextComp->SetVisibility(true);
	}

	return bIsMine;
}


void UTile_AC::GetAllTiles(UWorld* World, TArray<UTile_AC*>& OutTiles)
{
	OutTiles.Empty();
	if (!World) return;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* A = *It;
		if (!A) continue;
		UTile_AC* Tile = A->FindComponentByClass<UTile_AC>();
		if (Tile)
		{
			OutTiles.Add(Tile);
		}
	}
}