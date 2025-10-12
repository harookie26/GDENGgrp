#pragma once

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"

#include "Text3DComponent.h"
#include "Math/UnrealMathUtility.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tile_AC.generated.h"

class UText3DComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GDENGGRP_API UTile_AC : public UActorComponent
{
	GENERATED_BODY()

public:
	UTile_AC();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	//Grid configuration: X and Y size (defaults to 4x4)
	UPROPERTY(EditAnywhere, Category = "Grid")
	int32 GridSizeX = 4;

	UPROPERTY(EditAnywhere, Category = "Grid")
	int32 GridSizeY = 4;

	//Number of mines placed on the map
	UPROPERTY(EditAnywhere, Category = "Grid")
	int32 NumMines = 4;

	//Distance between spawned actor centers (minimum)
	UPROPERTY(EditAnywhere, Category = "Grid")
	float Spacing = 200.f;

	//Extra padding added on top of the mesh size to ensure no overlap
	UPROPERTY(EditAnywhere, Category = "Grid")
	float ExtraPadding = 2.f;

	//Only run the spawner on the owner if the owner has this tag.
	//Prevents each spawned actor (which also has the component) from spawning its own grid.
	UPROPERTY(EditAnywhere, Category = "Grid")
	FName SpawnOriginTag = FName("GridOrigin");

	//If true, BeginPlay will only spawn when owner has SpawnOriginTag
	UPROPERTY(EditAnywhere, Category = "Grid")
	bool bOnlySpawnIfOwnerHasTag = true;

	//Uniform scale applied to the created Text3D components
	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "0.001", UIMin = "0.001"))
	float TextUniformScale = 0.08f;


	//Per-tile runtime state (set by the origin spawner)
	UPROPERTY(VisibleAnywhere, Category = "Tile State")
	bool bIsMine = false;

	UPROPERTY(VisibleAnywhere, Category = "Tile State")
	int32 AdjacentCount = 0;

	UPROPERTY(VisibleAnywhere, Category = "Tile State")
	bool bRevealed = false;

	//Runtime pointer to the text component created to show tile content
	UPROPERTY()
	UText3DComponent* RuntimeTextComp = nullptr;

	//Initialize this tile with computed values. Called by the origin spawner.
	UFUNCTION()
	void InitializeTile(int32 InAdjacentCount, bool bInIsMine, float InTextUniformScale);

	//Reveal this tile. Returns true if the tile was a mine.
	UFUNCTION()
	bool Reveal();

	//Gather all tile components in this world (used by mechanics to test win condition).
	static void GetAllTiles(UWorld* World, TArray<UTile_AC*>& OutTiles);
};
