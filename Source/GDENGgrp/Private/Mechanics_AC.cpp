// Fill out your copyright notice in the Description page of Project Settings..


#include "Mechanics_AC.h"

#include "Text3DComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"


// Sets default values for this component's properties
UMechanics_AC::UMechanics_AC()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UMechanics_AC::BeginPlay()
{
	Super::BeginPlay();

	// Try to enable input and bind spacebar for reveal action.
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (PC)
	{
		Owner->EnableInput(PC);
		if (Owner->InputComponent)
		{
			// Bind directly to the spacebar key so we don't rely on project input mappings.
			Owner->InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &UMechanics_AC::RevealUnderPlayer);
		}
	}

}


// Called every frame
void UMechanics_AC::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

static void SpawnFloatingTextOnOwner(UWorld* World, AActor* Owner, const FString& Text, float Duration = 5.f, float Scale = 0.12f, float ForwardOffset = 300.f)
{
	if (!World || !Owner) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	FVector ViewLoc = FVector::ZeroVector;
	FRotator ViewRot = FRotator::ZeroRotator;
	if (PC)
	{
		PC->GetPlayerViewPoint(ViewLoc, ViewRot);
	}

	const float UpOffset = 30.f;
	const FVector DesiredWorldLocation = ViewLoc + ViewRot.Vector() * ForwardOffset + FVector(0.f, 0.f, UpOffset);
	const FRotator LookAtCameraRot = (ViewLoc - DesiredWorldLocation).Rotation();

	const FRotator FlippedRot = LookAtCameraRot + FRotator(180.f, 180.0f, 180.0f);

	UText3DComponent* TextComp = NewObject<UText3DComponent>(Owner, UText3DComponent::StaticClass(), NAME_None, RF_Transient);
	if (!TextComp) return;

	Owner->AddInstanceComponent(TextComp);
	USceneComponent* OwnerRoot = Owner->GetRootComponent();
	if (!OwnerRoot)
	{
		Owner->SetRootComponent(TextComp);
	}
	else
	{
		TextComp->AttachToComponent(OwnerRoot, FAttachmentTransformRules::KeepWorldTransform);
	}

	// Configure visual properties
	TextComp->CreationMethod = EComponentCreationMethod::Instance;
	TextComp->SetMobility(EComponentMobility::Movable);
	TextComp->SetHorizontalAlignment(EText3DHorizontalTextAlignment::Center);
	TextComp->SetVerticalAlignment(EText3DVerticalTextAlignment::Center);
	TextComp->SetText(FText::FromString(Text));
	TextComp->SetExtrude(8.0f);
	TextComp->SetBevel(0.0f);
	TextComp->SetCastShadow(false);
	TextComp->SetRelativeScale3D(FVector(Scale));
	TextComp->SetVisibility(true, true);

	// Compute a relative location so the component appears at DesiredWorldLocation while remaining a child of the owner
	if (OwnerRoot)
	{
		const FVector LocalLocation = OwnerRoot->GetComponentTransform().InverseTransformPosition(DesiredWorldLocation);
		TextComp->SetRelativeLocation(LocalLocation);
	}
	else
	{
		// No root, set world directly
		TextComp->SetWorldLocation(DesiredWorldLocation);
	}

	// Face the camera and apply flip
	TextComp->SetWorldRotation(FlippedRot);

	// Register the component so it becomes active/rendered immediately
	TextComp->RegisterComponent();
	TextComp->InitializeComponent();

	// Schedule destruction of the transient component
	FTimerDelegate Del = FTimerDelegate::CreateLambda([TextComp]()
		{
			if (TextComp)
			{
				// Unregister and destroy component
				TextComp->UnregisterComponent();
				TextComp->DestroyComponent();
			}
		});

	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, Del, Duration, false);
}

void UMechanics_AC::RevealUnderPlayer()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World) return;

	// Trace down from the actor's location to find the tile beneath feet.
	const FVector Start = Owner->GetActorLocation() + FVector(0, 0, 20.f);
	const FVector End = Start - FVector(0, 0, 400.f);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	FHitResult Hit;
	bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	// Optional debug:
	// DrawDebugLine(World, Start, End, FColor::Green, false, 2.0f);

	if (!bHit || !Hit.GetActor())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("No tile under player to reveal."));
		}
		return;
	}

	AActor* HitActor = Hit.GetActor();
	UTile_AC* Tile = HitActor->FindComponentByClass<UTile_AC>();
	if (!Tile)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::White, TEXT("Actor under player is not a tile."));
		}
		return;
	}

	if (Tile->bRevealed)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::White, TEXT("Tile already revealed."));
		}
		return;
	}

	bool bWasMine = Tile->Reveal();
	if (bWasMine)
	{
		// Player loses immediately - show 3D text attached to the same pawn
		SpawnFloatingTextOnOwner(World, Owner, TEXT("You lost!"), 5.f, 0.18f, 400.f);

		// Optionally reveal all mines after loss
		TArray<UTile_AC*> AllTiles;
		UTile_AC::GetAllTiles(World, AllTiles);
		for (UTile_AC* T : AllTiles)
		{
			if (T && T->bIsMine && !T->bRevealed)
			{
				T->Reveal();
			}
		}
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, TEXT("Tile revealed."));
	}

	// Check win condition: all non-mine tiles revealed
	TArray<UTile_AC*> AllTiles;
	UTile_AC::GetAllTiles(World, AllTiles);

	int32 TotalNonMine = 0;
	int32 RevealedNonMine = 0;
	for (UTile_AC* T : AllTiles)
	{
		if (!T) continue;
		if (!T->bIsMine)
		{
			++TotalNonMine;
			if (T->bRevealed) ++RevealedNonMine;
		}
	}

	if (TotalNonMine > 0 && RevealedNonMine >= TotalNonMine)
	{
		// Show 3D win text attached to the owner pawn
		SpawnFloatingTextOnOwner(World, Owner, TEXT("You win!"), 5.f, 0.18f, 400.f);
	}
}