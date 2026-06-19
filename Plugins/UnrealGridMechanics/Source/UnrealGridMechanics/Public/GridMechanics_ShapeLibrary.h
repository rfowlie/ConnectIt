// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanics_Structs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GridMechanics_ShapeLibrary.generated.h"


USTRUCT(BlueprintType)
struct FShapeConfiguration
{
	GENERATED_BODY()
	
	FShapeConfiguration() {}
	FShapeConfiguration(const TArray<FGridPosition>& GridPositions) : ShapeConfiguration(GridPositions) {}
	
	UPROPERTY(BlueprintReadWrite)
	TArray<FGridPosition> ShapeConfiguration;

	UPROPERTY(BlueprintReadWrite)
	FName Name;
	
	UPROPERTY(BlueprintReadWrite)
	int32 Score = 0;
};

/**
 * 
 */
UCLASS()
class UNREALGRIDMECHANICS_API UGridMechanics_ShapeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FGridDirectionVector GetDirectionVector(EGridDirection GridDirection);
	
	static void GetLongestLine2(
		TArray<FGridPosition>& OutGridPositions,
		const TSet<FGridPosition>& InGridPositions,
		const FGridPosition& StartingPosition,
		const EGridDirection GridDirection);

	// find every line of x length given starting position and set of positions
	// NOTE: that this does not distinguish between position ownership
	static void GetLongestLines(
		TArray<TArray<FGridPosition>>& OutLines,
		const TSet<FGridPosition>& InGridPositions,
		const FGridPosition& StartingPosition,
		const int32 MinimumLength);

	static void GetLinesOfLength(
		TArray<TArray<FGridPosition>>& OutLines,
		const TSet<FGridPosition>& InGridPositions,
		const EGridDirection GridDirection,
		const int32 RequiredLength,
		const bool bExactLength);


	UFUNCTION(BlueprintCallable, Category = "Gigafire|Grid|Shape")
	static bool IsSquare(
		const TArray<FGridPosition>& Positions,
		TArray<FGridPosition>& OutCorners);

	// DEPRECATED
	UFUNCTION(BlueprintCallable)
	static void GetLongestLine(
		TArray<FGridPosition>& OutGridPositions, const TArray<FGridPosition>& InGridPositions,
		const FGridPosition& StartingPosition, const EGridDirection GridDirection);
};
