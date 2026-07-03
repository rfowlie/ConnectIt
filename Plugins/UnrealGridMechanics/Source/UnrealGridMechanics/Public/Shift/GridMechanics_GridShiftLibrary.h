// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "GridShiftTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GridMechanics_GridShiftLibrary.generated.h"

class AGridTileBase;

/**
 * 
 */
UCLASS()
class UNREALGRIDMECHANICS_API UGridMechanics_GridShiftLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// Computes the position remap for a shift operation
	// Pure math — no world or actor references
	// InPositions — all positions in the row or column being shifted
	// Returns FShiftResult containing the full remap and wrapping positions
	UFUNCTION(BlueprintPure, Category = "Grid|Shift")
	static FShiftResult ComputeShiftResult(
		const TArray<FGridPosition>& InPositions,
		const FShiftOperation& Operation);

	// Computes the new position for a single grid position after a shift
	// MinIndex and MaxIndex define the bounds of the row or column
	static FGridPosition ComputeShiftedPosition(
		const FGridPosition& Position,
		const FShiftOperation& Operation,
		int32 MinIndex,
		int32 MaxIndex);

	// Returns true if shifting this position causes it to wrap
	static bool IsWrapping(
		const FGridPosition& Position,
		const FShiftOperation& Operation,
		int32 MinIndex,
		int32 MaxIndex);

	// Builds shift instructions for each tile actor in the affected row or column
	// Requires world positions for start and end locations
	static TArray<FTileShiftInstruction> BuildShiftInstructions(
		const FShiftResult& Result,
		const TMap<FGridPosition, AGridTileBase*>& TileActors,
		const TMap<FGridPosition, FVector>& WorldPositions);
};
