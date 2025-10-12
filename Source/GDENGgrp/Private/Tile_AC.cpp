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
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
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

	// Only the origin actor (tagged) should run the spawning + grid setup.
	if (bOnlySpawnIfOwnerHasTag && !Owner->ActorHasTag(SpawnOriginTag))
	{
		// Non-origin actors do nothing here (origin will set their visuals).
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

	// Seed randomness
	FMath::RandInit(static_cast<int32>(FDateTime::Now().GetMillisecond()));

	const FVector Origin = Owner->GetActorLocation();
	FRotator SpawnRotation = Owner->GetActorRotation();

	// Determine effective spacing so tiles do not overlap.
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

	// Map (X,Y) -> index: Index = X * GridSizeY + Y
	auto IndexForXY = [&](int32 X, int32 Y) -> int32 { return X * GridSizeY + Y; };

	// Place owner into the grid at (0,0)
	if (TotalCells > 0)
	{
		GridActors[IndexForXY(0, 0)] = Owner;
	}

	// Spawn remaining cells and fill GridActors
	for (int32 X = 0; X < GridSizeX; ++X)
	{
		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			if (X == 0 && Y == 0)
			{
				continue; // owner already placed
			}

			const FVector SpawnLocation = Origin + FVector(X * EffectiveSpacingX, Y * EffectiveSpacingY, 0.f);
			AActor* Spawned = World->SpawnActor<AActor>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
			if (Spawned)
			{
				GridActors[IndexForXY(X, Y)] = Spawned;
			}
			else
			{
				// If spawn failed, leave nullptr. Continue — origin will still attempt to render for valid actors.
				GridActors[IndexForXY(X, Y)] = nullptr;
			}
		}
	}

	// --- Mines + starting tile setup ---
	// Requirements:
	// - Exactly 4 mines (or as many as fits, leaving at least one tile for the starting tile).
	// - The starting tile is at (0,0) (the owner) and is designated with "-" (we represent that with AdjacentCount == -2).
	// - Starting tile must NOT be a mine.
	//
	// Ensure we have at least one cell; otherwise nothing to do.

	if (TotalCells == 0)
	{
		return;
	}

	// Reserve the origin (0,0) as the starting tile.
	const int32 StartIndex = IndexForXY(0, 0);

	// Determine desired number of mines: exactly 4, but ensure we leave at least one non-mine for the start tile.
	int32 DesiredMines = FMath::Clamp(4, 0, TotalCells > 1 ? TotalCells - 1 : 0);
	// Keep the NumMines member in sync (optional).
	NumMines = DesiredMines;

	// Place DesiredMines unique mines randomly among TotalCells, excluding the starting index.
	TSet<int32> MineIndices;
	while (MineIndices.Num() < DesiredMines)
	{
		int32 Pick = FMath::RandRange(0, TotalCells - 1);
		if (Pick == StartIndex)
		{
			continue; // never place a mine on the starting tile
		}
		MineIndices.Add(Pick);
	}

	// Compute adjacent mine counts for each cell
	TArray<int32> AdjacentCounts;
	AdjacentCounts.Init(0, TotalCells);

	for (int32 X = 0; X < GridSizeX; ++X)
	{
		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			int32 Idx = IndexForXY(X, Y);
			if (MineIndices.Contains(Idx))
			{
				AdjacentCounts[Idx] = -1; // -1 = mine
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

	// Designate the start tile with a special value -2 (dash "-")
	if (StartIndex >= 0 && StartIndex < TotalCells)
	{
		AdjacentCounts[StartIndex] = -2; // -2 = starting tile, displayed as "-"
		// ensure start tile is not marked as a mine (shouldn't be, since we excluded it above)
		MineIndices.Remove(StartIndex);
	}

	// Initialize each spawned actor's own UTile_AC with its tile data.
	for (int32 X = 0; X < GridSizeX; ++X)
	{
		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			int32 Idx = IndexForXY(X, Y);
			AActor* CellActor = GridActors[Idx];
			if (!CellActor)
			{
				continue;
			}

			UTile_AC* TileComp = CellActor->FindComponentByClass<UTile_AC>();
			bool bMine = MineIndices.Contains(Idx);
			int32 Count = AdjacentCounts[Idx];

			if (TileComp)
			{
				TileComp->InitializeTile(Count, bMine, TextUniformScale);
			}
			else
			{
				// Fallback: if actor doesn't have a tile component, create a text component directly and hide it initially.
				// This should be rare because spawned actors should include this component.
				UText3DComponent* TextComp = NewObject<UText3DComponent>(CellActor, UText3DComponent::StaticClass(), NAME_None, RF_Transient);
				if (TextComp)
				{
					CellActor->AddInstanceComponent(TextComp);
					USceneComponent* Root = CellActor->GetRootComponent();
					if (!Root)
					{
						CellActor->SetRootComponent(TextComp);
						TextComp->SetRelativeLocation(FVector::ZeroVector);
					}
					else
					{
						TextComp->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
					}

					TextComp->SetMobility(EComponentMobility::Movable);
					TextComp->SetVisibility(false); // hidden until revealed

					FString Symbol;
					if (Count == -1)
					{
						Symbol = TEXT("X");
					}
					else if (Count == -2)
					{
						Symbol = TEXT("-"); // starting tile
					}
					else if (Count == 0)
					{
						Symbol = TEXT("_");
					}
					else
					{
						Symbol = FString::FromInt(Count);
					}

					TextComp->SetText(FText::FromString(Symbol));
					TextComp->SetExtrude(4.0f);
					TextComp->SetBevel(0.0f);
					TextComp->SetCastShadow(false);

					float ZOffset = 50.f;
					if (UStaticMeshComponent* MeshComp = CellActor->FindComponentByClass<UStaticMeshComponent>())
					{
						ZOffset = MeshComp->Bounds.BoxExtent.Z * 2.0f + 10.f;
					}
					TextComp->SetRelativeLocation(FVector(0.f, 0.f, ZOffset));
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

	// ...
}


void UTile_AC::InitializeTile(int32 InAdjacentCount, bool bInIsMine, float InTextUniformScale)
{
	bIsMine = bInIsMine;
	AdjacentCount = InAdjacentCount;
	bRevealed = false;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// create or reuse a Text3D component attached to the owner actor, keep it hidden until reveal
	UText3DComponent* Comp = Owner->FindComponentByClass<UText3DComponent>();
	if (!Comp)
	{
		Comp = NewObject<UText3DComponent>(Owner, UText3DComponent::StaticClass(), NAME_None, RF_Transient);
		if (!Comp) return;

		Owner->AddInstanceComponent(Comp);
		USceneComponent* Root = Owner->GetRootComponent();
		if (!Root)
		{
			Owner->SetRootComponent(Comp);
			Comp->SetRelativeLocation(FVector::ZeroVector);
		}
		else
		{
			Comp->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
		}

		Comp->SetMobility(EComponentMobility::Movable);
		Comp->CreationMethod = EComponentCreationMethod::Instance;

		Comp->RegisterComponent();
		Comp->InitializeComponent();
	}

	// Prepare the text but keep it hidden by default
	FString Symbol;
	if (AdjacentCount == -1)
	{
		Symbol = TEXT("X");
	}
	else if (AdjacentCount == -2)
	{
		Symbol = TEXT("-"); // starting tile
	}
	else if (AdjacentCount == 0)
	{
		Symbol = TEXT("_");
	}
	else
	{
		Symbol = FString::FromInt(AdjacentCount);
	}

	Comp->SetText(FText::FromString(Symbol));
	Comp->SetExtrude(4.0f);
	Comp->SetBevel(0.0f);
	Comp->SetCastShadow(false);

	float ZOffset = 50.f;
	if (UStaticMeshComponent* MeshComp = Owner->FindComponentByClass<UStaticMeshComponent>())
	{
		ZOffset = MeshComp->Bounds.BoxExtent.Z * 2.0f + 10.f;
	}
	Comp->SetRelativeLocation(FVector(0.f, 0.f, ZOffset));
	Comp->SetRelativeScale3D(FVector(InTextUniformScale));
	Comp->SetVisibility(false); // hide until revealed

	RuntimeTextComp = Comp;
}


bool UTile_AC::Reveal()
{
	if (bRevealed)
	{
		return bIsMine; // already revealed, return mine-state
	}

	bRevealed = true;

	// ensure the visual is shown
	if (!RuntimeTextComp)
	{
		// try to find/create one now
		AActor* Owner = GetOwner();
		if (Owner)
		{
			RuntimeTextComp = Owner->FindComponentByClass<UText3DComponent>();
			if (!RuntimeTextComp)
			{
				// create minimal text component so player sees something
				RuntimeTextComp = NewObject<UText3DComponent>(Owner, UText3DComponent::StaticClass(), NAME_None, RF_Transient);
				if (RuntimeTextComp)
				{
					Owner->AddInstanceComponent(RuntimeTextComp);
					RuntimeTextComp->RegisterComponent();
					RuntimeTextComp->InitializeComponent();
				}
			}
		}
	}

	if (RuntimeTextComp)
	{
		RuntimeTextComp->SetVisibility(true);
		// optionally change color to highlight mine
		//if (bIsMine)
		//{
		//	RuntimeTextComp->SetTextMaterial(RuntimeTextComp->GetTextMaterial()); // no-op placeholder
		//}
	}

	// Return whether this reveal hit a mine
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