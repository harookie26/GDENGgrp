#include "Movement_AC.h"

UMovement_AC::UMovement_AC()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UMovement_AC::BeginPlay()
{
    Super::BeginPlay();
    
    APlayerController* PlayerController = Cast<APlayerController>(this->SnowmanPawn->GetController());
    
    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
    
    Subsystem->ClearAllMappings();
    Subsystem->AddMappingContext(MappingContext, 0);
    
    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
    EnhancedInput->BindAction(InputAxisX, ETriggerEvent::Triggered, this, &UMovement_AC::MoveActorX);
    EnhancedInput->BindAction(InputAxisY, ETriggerEvent::Triggered, this, &UMovement_AC::MoveActorY);
    
}

void UMovement_AC::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (0 != this->xMoveSpeed || 0 != this->yMoveSpeed) {
        FVector location = this->SnowmanPawn->GetTransform().GetLocation();
        location.X += this->xMoveSpeed * SPEED_MULTIPLIER;
        location.Y += this->yMoveSpeed * SPEED_MULTIPLIER;
        this->SnowmanPawn->SetActorLocation(location);
    }
    if (0 != this->xMoveSpeed) {
        this->xMoveSpeed *= (1 - DECELERATION_PERCENTAGE);
        if (abs(this->xMoveSpeed) < 0.1f) this->xMoveSpeed = 0;
    }
    if (0 != this->yMoveSpeed) {
        this->yMoveSpeed *= (1 - DECELERATION_PERCENTAGE);
        if (abs(this->yMoveSpeed) < 0.1f) this->yMoveSpeed = 0;
    }
}

void UMovement_AC::MoveActorX(const FInputActionValue& Value)
{
    if (0 != Value.Get<float>()) {
        this->xMoveSpeed = FMath::Clamp(Value.Get<float>(), -1, 1);
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Move 1: X Axis Triggered"));
    }
}

void UMovement_AC::MoveActorY(const FInputActionValue& Value)
{
    if (0 != Value.Get<float>()) {
        this->yMoveSpeed = FMath::Clamp(Value.Get<float>(), -1, 1);
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Move 1: Y Axis Triggered"));
    }
}
