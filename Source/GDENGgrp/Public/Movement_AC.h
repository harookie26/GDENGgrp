#pragma once

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Movement_AC.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GDENGGRP_API UMovement_AC : public UActorComponent
{
    GENERATED_BODY()

public:
    UMovement_AC();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION()
    void MoveActorX(const FInputActionValue& Value);
    UFUNCTION()
    void MoveActorY(const FInputActionValue& Value);

    UPROPERTY(EditAnywhere, Category = "Pawn")
    APawn* SnowmanPawn;
    
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputMappingContext* MappingContext;
    
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* InputAxisX;
    
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* InputAxisY;

    UPROPERTY(EditAnywhere, Category = "Movement Settings")
    float SPEED_MULTIPLIER = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Movement Settings")
    float DECELERATION_PERCENTAGE = 0.2f;

    float xMoveSpeed = 0.0f;
    float yMoveSpeed = 0.0f;
};
