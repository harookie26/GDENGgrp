// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile_AC.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Math/UnrealMathUtility.h"

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

	// Prevent recursive spawning: only run if owner has the designated tag (configurable)
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

	// Determine effective spacing so tiles do not overlap.
	// If the owner has a StaticMeshComponent, use its world-space bounds to compute minimum spacing.
	float EffectiveSpacingX = Spacing;
	float EffectiveSpacingY = Spacing;

	if (UStaticMeshComponent* MeshComp = Owner->FindComponentByClass<UStaticMeshComponent>())
	{
		// MeshComp->Bounds.BoxExtent is in world-space (half-size)
		const FVector WorldHalfExtents = MeshComp->Bounds.BoxExtent;
		const float FullX = WorldHalfExtents.X * 2.0f;
		const float FullY = WorldHalfExtents.Y * 2.0f;

		EffectiveSpacingX = FMath::Max(Spacing, FullX + ExtraPadding);
		EffectiveSpacingY = FMath::Max(Spacing, FullY + ExtraPadding);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Treat owner as the (0,0) cell; spawn the remaining GridSizeX x GridSizeY actors
	for (int32 X = 0; X < GridSizeX; ++X)
	{
		for (int32 Y = 0; Y < GridSizeY; ++Y)
		{
			// Skip owner's cell (0,0)
			if (X == 0 && Y == 0)
			{
				continue;
			}

			FVector SpawnLocation = Origin + FVector(X * EffectiveSpacingX, Y * EffectiveSpacingY, 0.f);

			AActor* Spawned = World->SpawnActor<AActor>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
			if (Spawned)
			{
				// Optional: mark spawned actors so they won't act as origins (we already rely on tag check)
				// Spawned->Tags.Add(FName("GridClone"));
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

