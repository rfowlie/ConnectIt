// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseEnums.h"
#include "GridMechanicsBaseStructs.h"
#include "StructUtils/InstancedStruct.h"
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

    // mark tiles that are active but not placeable by players
    // (non player piece occupying)
    UPROPERTY(BlueprintReadOnly)
    bool bIsOccupied = true;

    // Whether a board shift is allowed to move this tile's data. An
    // unshiftable tile is skipped -- it stays exactly where it is, and the
    // shiftable tiles around it rotate among themselves as if it weren't
    // part of the line at all. See
    // FConnectItBoardOperation_Shift.
    UPROPERTY(BlueprintReadOnly)
    bool bCanShift = true;

    void SetFactionPiece(const int32 InFactionPiece)
    {
        FactionPiece = InFactionPiece;
        bIsOccupied = FactionPiece != -1;
    }
};

// Full board state -- the single source of truth
// This struct is what replicates -- one property drives all visual systems
//
// Also copied wholesale into the AI's background MinMax search
// (FConnectItMinMaxRules), which is only thread-safe because this is
// plain data: keep it free of UObject references (TObjectPtr, UObject*, weak
// object pointers) -- a background thread can't safely hold or follow them.
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

    // array position equals faction slot...
    UPROPERTY(BlueprintReadOnly)
    TArray<float> ScoreBoard;

    UPROPERTY(BlueprintReadOnly)
    int32 LastModifiedTurn = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bGameOver = false;

    UPROPERTY(BlueprintReadOnly)
    int32 WinningFactionSlot = -1;

    // Score needed to win, published by the match's win condition (see
    // FConnectItRuleSet::StampWinState) so UI can render "57 / 100" without
    // knowing which condition is in play or reaching the server-only rules.
    // Rides this single replicated snapshot rather than adding a second
    // replicated property. 0 means "this win condition isn't score-based" --
    // use WinProgress for a bar that works under any condition.
    UPROPERTY(BlueprintReadOnly)
    float TargetScore = 0.f;

    // How close each faction slot is to winning, 0 to 1, under whatever the
    // match's win condition is (FConnectItWinCondition::GetProgress).
    // Stamped alongside TargetScore after every move; empty until the first.
    UPROPERTY(BlueprintReadOnly)
    TArray<float> WinProgress;

    // --- Accessors ---

    // Find tile data by position -- returns nullptr if not found
    const FConnectItTileData* GetTileData(const FGridPosition& Position) const
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
        return Data && Data->bIsOccupied;
    }

    bool IsTileActive(const FGridPosition& Position) const
    {
        const FConnectItTileData* Data = GetTileData(Position);
        return Data && Data->bIsActive;
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

// What happened in one board change: an ordered list of board events
// (FConnectItBoardEvent and its subclasses -- see ConnectIt_BoardEvents.h), in
// the order they happened. E.g. [Piece Placed, Scored, Scored, Game Won].
//
// Rides along inside FConnectItBoardStateSnapshot so it replicates atomically
// with the state it describes. Built on the server as the change is carried
// out: the operation appends what it did, the scoring rule appends what
// scored, the Mediator appends a win. UConnectIt_BoardStateComponent then
// plays the events one at a time, identically on server and client.
USTRUCT(BlueprintType)
struct FConnectItBoardChangeEvent
{
    GENERATED_BODY()

    // Each entry holds one FConnectItBoardEvent subclass
    UPROPERTY(BlueprintReadOnly)
    TArray<FInstancedStruct> Events;

    template<typename TEvent>
    void Add(const TEvent& Event)
    {
        Events.Add(FInstancedStruct::Make(Event));
    }

    // The first event of type TEvent (or a subclass of it), or null
    template<typename TEvent>
    const TEvent* FindFirst() const
    {
        for (const FInstancedStruct& Entry : Events)
        {
            if (const TEvent* Event = Entry.GetPtr<TEvent>())
            {
                return Event;
            }
        }
        return nullptr;
    }
};

// Snapshot -- the ONE replicated property on UConnectItBoardStateComponent
// Previous and current arrive atomically
// Listeners read both via GetBoardSnapshot()
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
