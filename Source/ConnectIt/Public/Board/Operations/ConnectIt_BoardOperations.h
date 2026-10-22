// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "Board/ConnectIt_BoardLayoutCell.h"
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
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
    virtual EConnectItMoveLegality HasAnyLegalInstance(const FConnectItBoardState& Board, int32 ForFaction) const override;
};

// Trade places between one of the faction's own pieces and one of another
// faction's: both tiles must hold a piece, and exactly one must be the
// faction's. A swap that scores keeps nothing: the swapped-in piece is
// cleared with the rest of what it completed (a balance choice -- placing a
// piece keeps it). Sent by UConnectIt_SwapPieceAction.
USTRUCT(BlueprintType, meta = (DisplayName = "Swap Pieces"))
struct CONNECTIT_API FConnectItBoardOperation_SwapPieces : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FGridPosition PositionA;

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FGridPosition PositionB;

    virtual bool ArrivingPiecesSurviveScoring() const override { return false; }

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
    virtual EConnectItMoveLegality HasAnyLegalInstance(const FConnectItBoardState& Board, int32 ForFaction) const override;
};

// Shift a line of tiles Steps places. Positions is the full line, in line order
// from the selected tile outward (see UGridTileRegistryBase::
// GetTilesByDirection); Direction says which way it shifts. Whole tile data
// moves (piece, multiplier, active state), wrapping the far end round to the
// near end. A tile with bCanShift false stays exactly where it is and the
// shiftable tiles rotate among themselves as if it weren't in the line --
// so Steps counts SHIFTABLE tiles: a tile jumps over a fixed one, it doesn't
// spend a step on it. A shift by a whole number of laps (Steps a multiple of
// the shiftable tile count) changes nothing and is refused.
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

    // How many places along Direction the line moves. Client-supplied when it
    // arrives in a request: the Mediator checks it against the sending
    // action's own ShiftSteps.
    UPROPERTY(BlueprintReadWrite, Category = "Operation", meta = (ClampMin = 1))
    int32 Steps = 1;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItBoardChanges& OutChanges,
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
        FConnectItBoardChanges& OutChanges,
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
        FConnectItBoardChanges& OutChanges,
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
        FConnectItBoardChanges& OutChanges,
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
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// The board's record of a side's goals being replaced mid-match. Level only:
// no action produces it. The GameMode replaces the goals in the match's rules
// first (AConnectIt_GameMode::SetConditions), then carries this out: for new
// win goals it restarts those factions' goal progress (GroupsCompleted) at 0
// -- score carries over -- and it says what changed (Goals Changed). The win
// check that follows every board change then measures the new goals, so a
// goal already met completes at once.
USTRUCT(BlueprintType, meta = (DisplayName = "Change Goals"))
struct CONNECTIT_API FConnectItBoardOperation_ChangeGoals : public FConnectItBoardOperation
{
    GENERATED_BODY()

    // The side's factions (the AI's, or every human one)
    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    TArray<int32> Factions;

    // Win goals (progress restarts) or lose conditions (nothing on the board
    // to restart)
    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    bool bWinGoals = true;

    // The new goals' names, in order -- for the event
    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    TArray<FName> GroupNames;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// A phased level moves to a new phase: the board records it (CurrentPhase)
// and says so (Phase Changed). Level only. Refused for an empty tag.
USTRUCT(BlueprintType, meta = (DisplayName = "Begin Phase"))
struct CONNECTIT_API FConnectItBoardOperation_BeginPhase : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation", meta = (Categories = "ConnectIt.Phase"))
    FGameplayTag Phase;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// --- The board's tiles themselves ---
// The set of tiles can change during a match (a phased level changing
// layout). Level only for now: no action produces these. Their cells name a
// tile class for the visuals -- a class reference, the one UObject reference
// an operation carries; the AI's search never builds these.

// Take tiles off the board, with whatever is on them. A tile disappearing sets
// off nothing (its piece goes with it, no Piece Exit). Refused if a position
// isn't a tile (or is given twice).
USTRUCT(BlueprintType, meta = (DisplayName = "Remove Tiles"))
struct CONNECTIT_API FConnectItBoardOperation_RemoveTiles : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    TArray<FGridPosition> Positions;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// Put tiles on the board with their starting values. A tile appearing sets off
// nothing (no Piece Enter), but its starting piece is scored: it can complete
// a line, like a placed piece (and stays, as a placed piece does). Refused if
// a position is already a tile (or is given twice).
USTRUCT(BlueprintType, meta = (DisplayName = "Add Tiles"))
struct CONNECTIT_API FConnectItBoardOperation_AddTiles : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    TArray<FConnectItBoardLayoutCell> Cells;

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};

// Change the board's tiles to a layout in ONE board change: remove, then add
// (the same steps as the two operations above), then Layout Changed. One
// change means one win check, on the final board -- no in-between board is
// ever checked against the goals or shown. Shared positions are untouched
// (pieces and multipliers stay). Usually built from a layout's diff
// (UConnectIt_BoardLayout::DiffFrom).
USTRUCT(BlueprintType, meta = (DisplayName = "Change Layout"))
struct CONNECTIT_API FConnectItBoardOperation_ChangeLayout : public FConnectItBoardOperation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    TArray<FGridPosition> RemovePositions;

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    TArray<FConnectItBoardLayoutCell> AddCells;

    // For the Layout Changed event: the layout's name, and its camera if it has one
    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FName LayoutName;

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    bool bHasCamera = false;

    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    FTransform CameraTransform;

    // Removals and additions from a diff
    void SetDiff(const FConnectItBoardLayoutDiff& Diff);

    virtual FGameplayTag GetRequestType() const override;
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const override;
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItBoardChanges& OutChanges,
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
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const override;
    virtual FString Describe() const override;
};
