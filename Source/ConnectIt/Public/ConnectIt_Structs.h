// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseStructs.h"
#include "Board/Shift/GridShiftTypes.h"
#include "ConnectIt_Structs.generated.h"


// Data for a single tile on the ConnectIt board
USTRUCT(BlueprintType)
struct FConnectItTileData
{
    GENERATED_BODY()

    // Which faction owns a piece on this tile
    // -1 means empty
    UPROPERTY(BlueprintReadOnly)
    int32 FactionPiece = -1;

    // Scoring multiplier -- increments when tile is part of a scoring line
    UPROPERTY(BlueprintReadOnly)
    float Multiplier = 1.0f;

    // Whether this tile is active on the board
    // Inactive tiles cannot be selected (used by mutations e.g. The Rift)
    UPROPERTY(BlueprintReadOnly)
    bool bIsActive = true;

    bool IsOccupied() const { return FactionPiece != -1; }
};

// Full board state -- the single source of truth
// This struct is what replicates -- one property drives all visual systems
USTRUCT(BlueprintType)
struct FConnectItBoardState
{
    GENERATED_BODY()

  // Tile positions and data stored as parallel arrays
    // TMap cannot replicate -- TArray can
    // Use GetTileData(Position) and SetTileData(Position, Data) for access
    UPROPERTY(BlueprintReadOnly)
    TArray<FGridPosition> TilePositions;

    UPROPERTY(BlueprintReadOnly)
    TArray<FConnectItTileData> TileDataArray;

    UPROPERTY(BlueprintReadOnly)
    int32 FactionTurn = -1;
    
    UPROPERTY(BlueprintReadOnly)
    TArray<float> ScoreBoard;

    UPROPERTY(BlueprintReadOnly)
    int32 LastModifiedTurn = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bGameOver = false;

    UPROPERTY(BlueprintReadOnly)
    int32 WinningFactionSlot = -1;

    // --- Accessors ---

    // Find tile data by position -- returns nullptr if not found
    const FConnectItTileData* GetTileData(
        const FGridPosition& Position) const
    {
        const int32 Index = TilePositions.IndexOfByKey(Position);
        return TileDataArray.IsValidIndex(Index)
            ? &TileDataArray[Index]
            : nullptr;
    }

    // Mutable version for server side mutation
    FConnectItTileData* GetTileDataMutable(const FGridPosition& Position)
    {
        const int32 Index = TilePositions.IndexOfByKey(Position);
        return TileDataArray.IsValidIndex(Index)
            ? &TileDataArray[Index]
            : nullptr;
    }

    // Add or update tile data
    void SetTileData(
        const FGridPosition& Position,
        const FConnectItTileData& Data)
    {
        const int32 Index = TilePositions.IndexOfByKey(Position);
        if (TileDataArray.IsValidIndex(Index))
        {
            TileDataArray[Index] = Data;
        }
        else
        {
            TilePositions.Add(Position);
            TileDataArray.Add(Data);
        }
    }

    // --- Helper queries ---

    bool IsTileOccupied(const FGridPosition& Position) const
    {
        const FConnectItTileData* Data = GetTileData(Position);
        return Data && Data->IsOccupied();
    }

    bool IsTileActive(const FGridPosition& Position) const
    {
        const FConnectItTileData* Data = GetTileData(Position);
        return Data && Data->bIsActive;
    }

    bool IsTileValidForPlacement(const FGridPosition& Position) const
    {
        return IsTileActive(Position) && !IsTileOccupied(Position);
    }

    float GetScore(int32 FactionSlot) const
    {
        return ScoreBoard.IsValidIndex(FactionSlot)
            ? ScoreBoard[FactionSlot]
            : 0.f;
    }

    // Iterate all tiles -- replaces for (auto& [Pos, Data] : TileMap)
    int32 NumTiles() const { return TilePositions.Num(); }

    FGridPosition GetPositionAt(int32 Index) const
    {
        return TilePositions.IsValidIndex(Index)
            ? TilePositions[Index]
            : FGridPosition();
    }

    const FConnectItTileData& GetTileDataAt(int32 Index) const
    {
        return TileDataArray[Index];
    }
};

// Describes what specifically happened on the most recent ApplyAndBroadcast
// call -- rides along inside FConnectItBoardStateSnapshot so it replicates
// atomically with the state it describes. Consumed by AConnectIt_BoardManager
// to drive typed delegates and gated visual sequencing identically on server
// and client (see HandleBoardStateChanged), instead of the old pattern of
// firing gameplay delegates directly from server-only request handlers.
//
// bGameWon is edge-triggered -- true only on the transition into game-over,
// not "the game is currently over" (FConnectItBoardState::bGameOver stays
// true on every snapshot after the win).
USTRUCT(BlueprintType)
struct FConnectItBoardChangeEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bPiecePlaced = false;

    UPROPERTY(BlueprintReadOnly)
    FGridPosition PlacedPosition;

    UPROPERTY(BlueprintReadOnly)
    int32 PlacingFactionSlot = -1;

    UPROPERTY(BlueprintReadOnly)
    bool bLineScored = false;

    UPROPERTY(BlueprintReadOnly)
    int32 ScoringFactionSlot = -1;

    UPROPERTY(BlueprintReadOnly)
    float PointsScored = 0.f;

    UPROPERTY(BlueprintReadOnly)
    bool bGameWon = false;

    UPROPERTY(BlueprintReadOnly)
    int32 WinningFactionSlot = -1;

    // --- Shift ---
    // Always disjoint from the placement fields above -- a single
    // ApplyAndBroadcast call represents exactly one kind of change, so
    // bShiftApplied and bPiecePlaced never both end up true on the same event.

    UPROPERTY(BlueprintReadOnly)
    bool bShiftApplied = false;

    UPROPERTY(BlueprintReadOnly)
    FShiftOperation ShiftOperation;

    // Parallel arrays, not FShiftResult's TMap/TSet directly -- TMap/TSet
    // don't replicate (same reason FConnectItBoardState uses parallel
    // TArrays instead of a TMap). Old/new position identity is kept
    // explicit so the shift animation plays the same way on clients as it
    // does on the server. ShiftFromPositions[i] moved to ShiftToPositions[i].
    UPROPERTY(BlueprintReadOnly)
    TArray<FGridPosition> ShiftFromPositions;

    UPROPERTY(BlueprintReadOnly)
    TArray<FGridPosition> ShiftToPositions;

    UPROPERTY(BlueprintReadOnly)
    TArray<FGridPosition> ShiftWrappingPositions;
};

// Snapshot -- the ONE replicated property on UConnectItBoardStateComponent
// Previous and current arrive atomically
// Interpreters read both via GetBoardSnapshot()
USTRUCT(BlueprintType)
struct FConnectItBoardStateSnapshot
{
    GENERATED_BODY()

    FConnectItBoardStateSnapshot() = default;
    FConnectItBoardStateSnapshot(
        const FConnectItBoardState& InPreviousState,
        const FConnectItBoardState& InCurrentState,
        const FConnectItBoardChangeEvent& InChangeEvent) :
    PreviousState(InPreviousState), CurrentState(InCurrentState), ChangeEvent(InChangeEvent) {}

    // State before the most recent change
    // Set by server immediately before applying new state
    UPROPERTY(BlueprintReadWrite)
    FConnectItBoardState PreviousState;

    // Current authoritative state
    UPROPERTY(BlueprintReadWrite)
    FConnectItBoardState CurrentState;

    // What specifically changed on this update -- see FConnectItBoardChangeEvent
    UPROPERTY(BlueprintReadWrite)
    FConnectItBoardChangeEvent ChangeEvent;
};

// FTurnActionRequest payload -- FactionID lives on the envelope itself,
// not duplicated here (see FTurnActionRequest in TurnBasedMechanicsStructs.h)
USTRUCT(BlueprintType)
struct FConnectItRequestPlacePiece
{
    GENERATED_BODY()

    // Grid positions relevant to this request
    UPROPERTY(BlueprintReadWrite)
    TArray<FGridPosition> Positions;
};

// FTurnActionRequest payload -- see FConnectItRequestPlacePiece above
USTRUCT(BlueprintType)
struct FConnectItRequestBoardShift
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FShiftOperation ShiftOperation;
};