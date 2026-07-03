// Fill out your copyright notice in the Description page of Project Settings.

// GridShiftTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "GridShiftTypes.generated.h"

UENUM(BlueprintType)
enum class EShiftAxis : uint8
{
    Row     UMETA(DisplayName = "Row"),
    Column  UMETA(DisplayName = "Column")
};

UENUM(BlueprintType)
enum class EShiftDirection : uint8
{
    Positive UMETA(DisplayName = "Positive"),
    Negative UMETA(DisplayName = "Negative")
};

// Describes a shift operation — pure data, no logic
USTRUCT(BlueprintType)
struct UNREALGRIDMECHANICS_API FShiftOperation
{
    GENERATED_BODY()

    // Whether we are shifting a row or a column
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shift")
    EShiftAxis Axis = EShiftAxis::Row;

    // Which row or column index to shift
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shift")
    int32 Index = 0;

    // How many positions to shift — always positive
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shift",
        meta = (ClampMin = 1))
    int32 Amount = 1;

    // Direction of the shift
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shift")
    EShiftDirection Direction = EShiftDirection::Positive;

    // Returns the signed amount — positive or negative based on direction
    int32 GetSignedAmount() const
    {
        return Direction == EShiftDirection::Positive ? Amount : -Amount;
    }
};

// Maps every old position to its new position after a shift
// Used both for applying the shift and driving tile animations
USTRUCT(BlueprintType)
struct UNREALGRIDMECHANICS_API FShiftResult
{
    GENERATED_BODY()

    // Key — old position, Value — new position
    TMap<FGridPosition, FGridPosition> PositionRemap;

    // Positions that wrapped around the edge
    // Tile actors can use this to trigger special wrap animations
    TSet<FGridPosition> WrappingPositions;

    bool IsValid() const { return !PositionRemap.IsEmpty(); }
};

// Instruction sent to each tile actor before the shift animation begins
// Contains everything the tile needs to animate itself
USTRUCT(BlueprintType)
struct UNREALGRIDMECHANICS_API FTileShiftInstruction
{
    GENERATED_BODY()

    // The tile actor receiving this instruction
    UPROPERTY(BlueprintReadOnly, Category = "Shift")
    AGridTileBase* TileActor = nullptr;

    // World position the tile starts at
    UPROPERTY(BlueprintReadOnly, Category = "Shift")
    FVector StartWorldPosition = FVector::ZeroVector;

    // World position the tile should end at
    UPROPERTY(BlueprintReadOnly, Category = "Shift")
    FVector EndWorldPosition = FVector::ZeroVector;

    // Old grid position
    UPROPERTY(BlueprintReadOnly, Category = "Shift")
    FGridPosition FromGridPosition;

    // New grid position
    UPROPERTY(BlueprintReadOnly, Category = "Shift")
    FGridPosition ToGridPosition;

    // Whether this tile is wrapping around the board edge
    // Blueprint artists use this to trigger special wrap animations
    UPROPERTY(BlueprintReadOnly, Category = "Shift")
    bool bIsWrapping = false;
};
