// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "ConnectIt_BoardOperations.generated.h"

// Every kind of change that can be made to the board. See
// FConnectItBoardOperation for what an operation is and the rules they all
// follow.


// Put one of the faction's pieces on a placeable tile.
// Sent by UConnectIt_PlacePieceAction and the AI.
USTRUCT(BlueprintType, meta = (DisplayName = "Place Piece"))
struct CONNECTIT_API FConnectItBoardOperation_PlacePiece : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FGridPosition Position;

    // THE definition of "a piece may be placed on this tile" -- every
    // condition goes here, in one place. CanApply, the AI's move generation
    // and the client's hover highlight all use it. Today: the tile is active
    // and unoccupied.
    // Inline: the AI's search asks this for every tile of every position.
    static bool IsTilePlaceable(const FConnectItBoardState& Board, int32 TileIndex)
    {
        if (!Board.TileDataArray.IsValidIndex(TileIndex)) return false;

        const FConnectItTileData& Tile = Board.TileDataArray[TileIndex];
        return Tile.bIsActive && !Tile.bIsOccupied;
    }

    // The same check for callers that have a grid position
    static bool IsTilePlaceableAt(const FConnectItBoardState& Board, const FGridPosition& Position)
    {
        return IsTilePlaceable(Board, Board.TilePositions.IndexOfByKey(Position));
    }

    // Every Place Piece operation Faction could make on Board: calls
    // Callback(const FConnectItBoardOperation_PlacePiece&) once per placeable
    // tile. For the AI's search.
    template<typename TCallback>
    static void ForEachMove(const FConnectItBoardState& Board, int32 Faction, TCallback&& Callback)
    {
        FConnectItBoardOperation_PlacePiece Operation;
        Operation.Faction = Faction;

        const int32 NumTiles = Board.NumTiles();
        for (int32 TileIndex = 0; TileIndex < NumTiles; TileIndex++)
        {
            if (IsTilePlaceable(Board, TileIndex))
            {
                Operation.Position = Board.TilePositions[TileIndex];
                Callback(Operation);
            }
        }
    }

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItTouchedPositions& OutTouched,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// Trade places between one of the faction's own pieces and one of another
// faction's: both tiles must hold a piece, and exactly one must be the
// faction's. Sent by UConnectIt_SwapPieceAction.
USTRUCT(BlueprintType, meta = (DisplayName = "Swap Pieces"))
struct CONNECTIT_API FConnectItBoardOperation_SwapPieces : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FGridPosition PositionA;

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FGridPosition PositionB;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItTouchedPositions& OutTouched,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// Shift a line of tiles one step. Positions is the full line, in line order
// from the selected tile outward (see UGridTileRegistryBase::
// GetTilesByDirection); Direction says which way it shifts. Whole tile data
// moves (piece, multiplier, active state), wrapping the far end round to the
// near end. A tile with bCanShift false stays exactly where it is and the
// shiftable tiles rotate among themselves as if it weren't in the line.
// Unrestricted: no ownership check on the line. Sent by
// UConnectIt_BoardShiftAction.
USTRUCT(BlueprintType, meta = (DisplayName = "Shift"))
struct CONNECTIT_API FConnectItBoardOperation_Shift : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    TArray<FGridPosition> Positions;

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    EGridDirection Direction = EGridDirection::Max;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItTouchedPositions& OutTouched,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// Put one of the faction's pieces on a tile regardless of the placement
// check: the tile only has to exist, so this can overwrite an inactive or
// already-occupied tile. Reported to visuals as a placement, since that is
// what it looks like. Nothing sends this yet.
USTRUCT(BlueprintType, meta = (DisplayName = "Force Place Piece"))
struct CONNECTIT_API FConnectItBoardOperation_ForcePlacePiece : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FGridPosition Position;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItTouchedPositions& OutTouched,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// Take over another faction's piece: the tile must be occupied by a piece
// that isn't already the faction's. Nothing sends this yet.
USTRUCT(BlueprintType, meta = (DisplayName = "Capture Piece"))
struct CONNECTIT_API FConnectItBoardOperation_CapturePiece : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FGridPosition Position;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItTouchedPositions& OutTouched,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// Take a piece off an occupied tile. DelayTurns is declared for a delayed /
// scheduled removal that doesn't exist yet (there is no per-turn scheduling):
// anything above 0 is refused rather than silently treated as immediate.
// Nothing sends this yet.
USTRUCT(BlueprintType, meta = (DisplayName = "Remove Piece"))
struct CONNECTIT_API FConnectItBoardOperation_RemovePiece : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FGridPosition Position;

    UPROPERTY(BlueprintReadWrite, Category = "Operation", meta = (ClampMin = 0))
    int32 DelayTurns = 0;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItTouchedPositions& OutTouched,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// Reset a tile's scoring multiplier to 1; the tile must have one above 1.
// Nothing sends this yet.
USTRUCT(BlueprintType, meta = (DisplayName = "Destroy Tile Multiplier"))
struct CONNECTIT_API FConnectItBoardOperation_DestroyTileMultiplier : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FGridPosition Position;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItTouchedPositions& OutTouched,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// Flip a tile between active and inactive. Nothing sends this yet.
USTRUCT(BlueprintType, meta = (DisplayName = "Toggle Tile Active"))
struct CONNECTIT_API FConnectItBoardOperation_ToggleTileActive : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FGridPosition Position;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItTouchedPositions& OutTouched,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};
