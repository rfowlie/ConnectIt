// Fill out your copyright notice in the Description page of Project Settings.


#include "GridMechanics_GridLibrary.h"

#include "GridMechanics_Structs.h"

FGridPosition UGridMechanics_GridLibrary::CalculateGridPositionFromSize(const AActor* Actor, int32 Size)
{
	Size = FMath::Max(10, Size);
	FGridPosition GridPosition;
	if (IsValid(Actor))
	{
		GridPosition.X = FMath::TruncToInt(Actor->GetActorLocation().X / Size);
		GridPosition.Y = FMath::TruncToInt(Actor->GetActorLocation().Y / Size);
	}

	// UE_LOG(LogTemp, Display, TEXT("Grid Position: %d %d"), GridPosition.X, GridPosition.Y);
	
	return GridPosition;
}
