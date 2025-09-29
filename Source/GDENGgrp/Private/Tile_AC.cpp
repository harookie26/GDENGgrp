// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile_AC.h"

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

	// Place NumMines unique mines randomly among TotalCells
	TSet<int32> MineIndices;
	NumMines = FMath::Clamp(NumMines, 0, TotalCells);
	while (MineIndices.Num() < NumMines)
	{
		int32 Pick = FMath::RandRange(0, TotalCells - 1);
		MineIndices.Add(Pick);
	}

	// Compute adjacent mine counts for each cell
	TArray<int32> AdjacentCount;
	AdjacentCount.Init(0, TotalCells);

	for (int32 X = 0; X < GridSizeX; ++X)
	{
		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			int32 Idx = IndexForXY(X, Y);
			if (MineIndices.Contains(Idx))
			{
				AdjacentCount[Idx] = -1; // -1 = mine
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
			AdjacentCount[Idx] = Count;
		}
	}

	// Add a Text3DComponent to each actor to display X, number, or _
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

			FString Symbol;
			FColor Color = FColor::White;

			if (AdjacentCount[Idx] == -1)
			{
				Symbol = TEXT("X");
				Color = FColor::Red;
			}
			else if (AdjacentCount[Idx] == 0)
			{
				Symbol = TEXT("_");
				Color = FColor::Black;
			}
			else
			{
				Symbol = FString::FromInt(AdjacentCount[Idx]);
				Color = FColor::White;
			}

			if (Text3DComp)
			{
				// Set symbol text
				Text3DComp->SetText(FText::FromString(Symbol));

				// Visual parameters (tweak to fit your cube)
				Text3DComp->SetExtrude(4.0f);
				Text3DComp->SetBevel(0.0f);
				Text3DComp->SetCastShadow(false);

				// Position the text above the cube
				float ZOffset = 50.f;
				if (UStaticMeshComponent* MeshComp = CellActor->FindComponentByClass<UStaticMeshComponent>())
				{
					ZOffset = MeshComp->Bounds.BoxExtent.Z * 2.0f + 10.f;
				}
				Text3DComp->SetRelativeLocation(FVector(0.f, 0.f, ZOffset));

				// Scale to fit (adjust)
				const float UniformScale = 0.08f;
				Text3DComp->SetRelativeScale3D(FVector(UniformScale));

				// If you need an immediate rebuild of geometry you can call:
				// Text3DComp->RequestUpdate(EText3DRendererFlags::All);
				// but SetText already triggers an update path in the component.
			}
			else
			{
				// Optional fallback: create one if missing (keeps previous behaviour)
				// UText3DComponent* NewComp = NewObject<UText3DComponent>(CellActor);
				// ... register/configure NewComp ...
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


