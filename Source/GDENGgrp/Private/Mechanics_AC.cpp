#include "Mechanics_AC.h"


UMechanics_AC::UMechanics_AC()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UMechanics_AC::BeginPlay()
{
	Super::BeginPlay();

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
			//Bind directly to the spacebar key so we don't rely on project input mappings.
			Owner->InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &UMechanics_AC::RevealUnderPlayer);
		}
	}

}


void UMechanics_AC::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UMechanics_AC::RevealUnderPlayer()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World) return;

	//Trace down from the actor's location to find the tile beneath feet.
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
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Actor under player is not a tile."));
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
		// Player loses immediately
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("You revealed a mine! You lose."));
		}

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

		if (Owner->InputComponent)
		{
			Owner->InputComponent->ClearActionBindings();
		}
		return;
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
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("You revealed all safe tiles! You win!"));
		}

		// Disable further input
		if (Owner->InputComponent)
		{
			Owner->InputComponent->ClearActionBindings();
		}
	}
}
