// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GridMechanicsBaseEnums.h"
#include "GridMechanicsBaseStructs.h"
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
	/*
	 * Find every line of x length given starting position and array of positions
	 * NOTE that this does not distinguish between position ownership, positions should be pre-filtered before calling
	 */
	UFUNCTION(BlueprintCallable)
	static void GetLongestLine(
		TArray<FGridPosition>& OutGridPositions, const TArray<FGridPosition>& InGridPositions,
		const FGridPosition& StartingPosition, const EGridDirection GridDirection);
	
	static void GetLongestLines(
		TArray<TArray<FGridPosition>>& OutLines,
		const TArray<FGridPosition>& InGridPositions,
		const FGridPosition& StartingPosition,
		const int32 MinimumLength);
	
	static void GetLinesOfLength(
		TArray<TArray<FGridPosition>>& OutLines,
		const TSet<FGridPosition>& InGridPositions,
		const EGridDirection GridDirection,
		const int32 RequiredLength,
		const bool bExactLength);


	// other
	UFUNCTION(BlueprintCallable, Category = "Grid|Shape")
	static bool IsSquare(
		const TArray<FGridPosition>& Positions,
		TArray<FGridPosition>& OutCorners);
	
	
};
