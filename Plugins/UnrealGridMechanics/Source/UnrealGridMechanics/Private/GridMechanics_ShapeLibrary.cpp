// Fill out your copyright notice in the Description page of Project Settings.


#include "GridMechanics_ShapeLibrary.h"
#include "GridMechanics_Structs.h"


void UGridMechanics_ShapeLibrary::GetLongestLine(
	TArray<FGridPosition>& OutGridPositions, const TArray<FGridPosition>& InGridPositions,
	const FGridPosition& StartingPosition, const EGridDirection GridDirection)
{
	// if (!InGridPositions.Contains(StartingPosition)) { return; }
	TMap<FGridPosition, bool> GridMap;
	for (auto GridPosition : InGridPositions) { GridMap.Add(GridPosition, false); }

	int32 RowDirectionMultiplier = 0;
	int32 ColumnDirectionMultiplier = 0;
	
	switch (GridDirection){
	case Row:
		ColumnDirectionMultiplier = 1;
		break;
	case Column:
		RowDirectionMultiplier = 1;
		break;
	case Diagonal_TopDown:
		RowDirectionMultiplier = 1;
		ColumnDirectionMultiplier = 1;
		break;
	case Diagonal_BottomUp:
		RowDirectionMultiplier = -1;
		ColumnDirectionMultiplier = 1;
		break;
	}
	
	int32 Positive = 0;
	for (int32 Index = 1; Index <= MAX_int32; Index++)
	{
		FGridPosition TempPosition (
			Index * RowDirectionMultiplier, Index * ColumnDirectionMultiplier);
		if (!GridMap.Contains(StartingPosition + TempPosition)) { break; }
		Positive = Index;
	}
	
	int32 Negative = 0;
	for (int32 Index = 1; Index <= MAX_int32; Index++)
	{
		FGridPosition TempPosition (
			-Index * RowDirectionMultiplier, -Index * ColumnDirectionMultiplier);
		if (!GridMap.Contains(StartingPosition + TempPosition)) { break; }
		Negative = -Index;
	}

	for (int32 Index = Negative; Index <= Positive; Index++)
	{
		OutGridPositions.Add(
			StartingPosition +
			FGridPosition(Index * RowDirectionMultiplier, Index * ColumnDirectionMultiplier));
	}	
}
