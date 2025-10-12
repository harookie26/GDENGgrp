#pragma once

#include "Tile_AC.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/EngineTypes.h"
#include "DrawDebugHelpers.h"
#include "InputCoreTypes.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Mechanics_AC.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDENGGRP_API UMechanics_AC : public UActorComponent
{
	GENERATED_BODY()

public:	
	//Sets default values for this component's properties
	UMechanics_AC();

protected:
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Reveal the tile under the player (if any)
	void RevealUnderPlayer();
};
