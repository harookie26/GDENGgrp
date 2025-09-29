// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tile_AC.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDENGGRP_API UTile_AC : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTile_AC();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	// Grid configuration: X and Y size (defaults to 4x4)
	UPROPERTY(EditAnywhere, Category="Grid")
	int32 GridSizeX = 4;

	UPROPERTY(EditAnywhere, Category="Grid")
	int32 GridSizeY = 4;

	// Distance between spawned actor centers (minimum)
	UPROPERTY(EditAnywhere, Category="Grid")
	float Spacing = 200.f;

	// Extra padding added on top of the mesh size to ensure no overlap
	UPROPERTY(EditAnywhere, Category="Grid")
	float ExtraPadding = 2.f;

	// Only run the spawner on the owner if the owner has this tag.
	// Prevents each spawned actor (which also has the component) from spawning its own grid.
	UPROPERTY(EditAnywhere, Category="Grid")
	FName SpawnOriginTag = FName("GridOrigin");

	// If true, BeginPlay will only spawn when owner has SpawnOriginTag
	UPROPERTY(EditAnywhere, Category="Grid")
	bool bOnlySpawnIfOwnerHasTag = true;
		
};
