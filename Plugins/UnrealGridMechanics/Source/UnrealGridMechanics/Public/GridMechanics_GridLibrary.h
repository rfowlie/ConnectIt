// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseEnums.h"
#include "GridMechanicsBaseStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GridMechanics_GridLibrary.generated.h"

/**
 * 
 */
UCLASS()
class UNREALGRIDMECHANICS_API UGridMechanics_GridLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	static const TMap<EGridDirection, FGridDirectionVector> GridDirectionVectors;

	UFUNCTION(BlueprintCallable)
	static FGridDirectionVector GetGridDirectionVector(EGridDirection Direction);
	
	UFUNCTION(BlueprintCallable)
	static FGridPosition CalculateGridPositionFromSize(const AActor* Actor, int32 Size);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	static int32 CountValidWindows(
		const FGridPosition& Position,
		const TSet<FGridPosition>& ValidPositions,
		const FGridDirectionVector& Direction,
		int32 ConnectLength);
	
	UFUNCTION(BlueprintCallable, Category = "Grid")
	static int32 CountPossibleGridConnections(
		const FGridPosition& StartPosition,
		const TSet<FGridPosition>& ValidPositions,
		int32 ConnectLength = 4);
	
	// // Returns the breakdown per direction if you need it for scoring
	// UFUNCTION(BlueprintCallable, Category = "Grid|Shape")
	// static TMap<EGridDirection, int32> GetPotentialConnectFourCountPerDirection(
	// 	const FGridPosition& Position,
	// 	const TSet<FGridPosition>& ValidPositions,
	// 	int32 ConnectLength = 4);

	
	static TArray<TPair<int32, FGridPosition>> CreateConnectionsFloodMap(
		const TArray<FGridPosition>& GridPositions,
		const int32 ConnectionLength);
	

	
};
