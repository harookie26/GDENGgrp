#include "Movement_AC.h"

UMovement_AC::UMovement_AC()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UMovement_AC::BeginPlay()
{
    Super::BeginPlay();

    // Prefer explicit assigned pawn, otherwise fall back to the component owner
    APawn* Pawn = SnowmanPawn ? SnowmanPawn : Cast<APawn>(GetOwner());
    if (!Pawn)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: Movement_AC has no Pawn assigned and owner is not a Pawn. Disabling component tick."), *GetName());
        SetComponentTickEnabled(false);
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Pawn is not possessed by a PlayerController; input bindings will be skipped."), *GetName());
        return;
    }

    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    if (LocalPlayer && MappingContext)
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
        {
            // Clear/Add mapping context only if subsystem exists
            Subsystem->ClearAllMappings();
            Subsystem->AddMappingContext(MappingContext, 0);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: EnhancedInputLocalPlayerSubsystem not found on LocalPlayer."), *GetName());
        }
    }

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
    {
        if (InputAxisX)
        {
            EnhancedInput->BindAction(InputAxisX, ETriggerEvent::Triggered, this, &UMovement_AC::MoveActorX);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: InputAxisX is null; X movement will not be bound."), *GetName());
        }

        if (InputAxisY)
        {
            EnhancedInput->BindAction(InputAxisY, ETriggerEvent::Triggered, this, &UMovement_AC::MoveActorY);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s: InputAxisY is null; Y movement will not be bound."), *GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: PlayerController InputComponent is not an UEnhancedInputComponent (or is null)."), *GetName());
    }
}

void UMovement_AC::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    APawn* Pawn = SnowmanPawn ? SnowmanPawn : Cast<APawn>(GetOwner());
    if (!Pawn)
    {
        return;
    }

    if (!FMath::IsNearlyZero(this->xMoveSpeed) || !FMath::IsNearlyZero(this->yMoveSpeed))
    {
        FVector location = Pawn->GetActorLocation();
        location.X += this->xMoveSpeed * SPEED_MULTIPLIER * DeltaTime;
        location.Y += this->yMoveSpeed * SPEED_MULTIPLIER * DeltaTime;
        Pawn->SetActorLocation(location);
    }

    if (!FMath::IsNearlyZero(this->xMoveSpeed))
    {
        this->xMoveSpeed *= (1.0f - DECELERATION_PERCENTAGE);
        if (FMath::Abs(this->xMoveSpeed) < 0.1f) this->xMoveSpeed = 0.0f;
    }
    if (!FMath::IsNearlyZero(this->yMoveSpeed))
    {
        this->yMoveSpeed *= (1.0f - DECELERATION_PERCENTAGE);
        if (FMath::Abs(this->yMoveSpeed) < 0.1f) this->yMoveSpeed = 0.0f;
    }
}

void UMovement_AC::MoveActorX(const FInputActionValue& Value)
{
    const float Val = Value.Get<float>();
    if (!FMath::IsNearlyZero(Val))
    {
        this->xMoveSpeed = FMath::Clamp(Val, -1.0f, 1.0f);
        //if (GEngine)
        //{
        //    GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Move 1: X Axis Triggered"));
        //}
    }
}

void UMovement_AC::MoveActorY(const FInputActionValue& Value)
{
    const float Val = Value.Get<float>();
    if (!FMath::IsNearlyZero(Val))
    {
        this->yMoveSpeed = FMath::Clamp(Val, -1.0f, 1.0f);
        //if (GEngine)
        //{
        //    GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Move 1: Y Axis Triggered"));
        //}
    }
}