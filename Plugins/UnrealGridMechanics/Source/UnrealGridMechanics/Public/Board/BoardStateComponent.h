// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridMechanicsBaseStructs.h"
#include "UnrealGridMechanics/Public/Board/Shift/GridShiftTypes.h"
#include "BoardStateComponent.generated.h"


// Extensible tile data — add game specific fields in subclass or directly here
USTRUCT(BlueprintType)
struct UNREALGRIDMECHANICS_API FTileData
{
    GENERATED_BODY()

    // Which faction owns a piece on this tile — 0 means empty
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
    int32 FactionPiece = 0;

    // Tile multiplier — used for scoring
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
    float Multiplier = 1.0f;

    // Whether this tile is currently active on the board
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
    bool bIsActive = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoardStateUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTileDataChanged, FGridPosition, Position, FTileData, NewData);

UCLASS(ClassGroup=(Grid), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UBoardStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UBoardStateComponent();

    // --- State Queries ---

    UFUNCTION(BlueprintPure, Category = "Grid|State")
    bool HasTileData(FGridPosition Position) const;

    UFUNCTION(BlueprintPure, Category = "Grid|State")
    FTileData GetTileData(FGridPosition Position) const;

    UFUNCTION(BlueprintPure, Category = "Grid|State")
    TArray<FGridPosition> GetAllPositions() const;

    // --- State Mutation ---

    UFUNCTION(BlueprintCallable, Category = "Grid|State")
    void SetTileData(FGridPosition Position, FTileData Data);

    UFUNCTION(BlueprintCallable, Category = "Grid|State")
    void RemoveTileData(FGridPosition Position);

    UFUNCTION(BlueprintCallable, Category = "Grid|State")
    void ClearAllTileData();

    // Applies a shift result to the state map atomically
    // Called by UBoardShiftComponent after animation completes
    void ApplyShiftResult(const FShiftResult& Result);

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Grid|State")
    FOnBoardStateUpdated OnBoardStateUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Grid|State")
    FOnTileDataChanged OnTileDataChanged;

private:

    // Authoritative board state — updated after animation completes
    UPROPERTY()
    TMap<FGridPosition, FTileData> TileDataMap;
};
