// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GridMechanics_ShapeLibrary.generated.h"

struct FGridPosition;


UENUM(BlueprintType)
enum EGridDirection
{
	Row,
	Column,
	Diagonal_TopDown,
	Diagonal_BottomUp
};

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
	UFUNCTION(BlueprintCallable)
	static void GetLongestLine(
		TArray<FGridPosition>& OutGridPositions, const TArray<FGridPosition>& InGridPositions,
		const FGridPosition& StartingPosition, const EGridDirection GridDirection);
};
