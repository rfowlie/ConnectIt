// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseEnums.h"
#include "GridMechanicsBaseStructs.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Powers/ConnectIt_TilePower.h"
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

    // This tile's id on its board: given by the board when the tile joins it
    // (FConnectItBoardState::AddTile -- building the board, Add Tiles), never
    // reused, never authored. It travels with the tile's data (a shift moves
    // it along), so a power that refers to another tile stores this rather
    // than a position. INDEX_NONE until the tile is on a board.
    UPROPERTY(BlueprintReadOnly)
    int32 TileId = INDEX_NONE;

    // --- The tile's own power ---
    // Copied from the tile when it joins the board (its class's Tile Power Tag,
    // and the values the placed tile or layout cell gives it -- see
    // AConnectIt_TileBase), then never changed: it travels with the tile.

    // Which power (ConnectIt.TilePower.*); empty = none
    UPROPERTY(BlueprintReadOnly)
    FGameplayTag TilePowerTag;

    // The power with its values; empty = none (only powered tiles carry one)
    UPROPERTY(BlueprintReadOnly)
    TInstancedStruct<FConnectItTilePower> TilePower;

    // The first turn (FConnectItBoardState::TurnNumber) its power can fire
    // again after a cooldown
    UPROPERTY(BlueprintReadOnly)
    int32 PowerReadyTurn = 0;

    void SetFactionPiece(const int32 InFactionPiece)
    {
        FactionPiece = InFactionPiece;
        bIsOccupied = FactionPiece != -1;
    }

    // Gives the tile Power (an empty one = no power)
    void SetTilePower(const FConnectItTilePower& Power)
    {
        TilePowerTag = Power.Name;
        TilePower = Power.IsSet()
            ? TInstancedStruct<FConnectItTilePower>::Make(Power)
            : TInstancedStruct<FConnectItTilePower>();
    }

    const FConnectItTilePower* GetTilePower() const { return TilePower.GetPtr(); }
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

    // The turn this board is at (the GameState's turn number), stamped by
    // whoever resolves a change on it: the Mediator for a real change, the
    // caller of Simulate Operation, the AI for its search root (+1 per ply).
    // Tile power cooldowns count against it.
    UPROPERTY(BlueprintReadOnly)
    int32 TurnNumber = 0;

    // The seed of the board's random stream: tile powers that choose at
    // random roll from it and advance it, so every copy of the board (server,
    // the AI's search) rolls the same
    UPROPERTY(BlueprintReadOnly)
    int32 RandomSeed = 0;

    // The id the next tile to join this board gets (FConnectItTileData::TileId)
    UPROPERTY(BlueprintReadOnly)
    int32 NextTileId = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bGameOver = false;

    UPROPERTY(BlueprintReadOnly)
    int32 WinningFactionSlot = -1;

    // The faction that lost by meeting a lose condition (the winner is then the
    // other side), or -1 -- set by FConnectItRuleSet::StampWinState. -1 when
    // the game was won by completing goals.
    UPROPERTY(BlueprintReadOnly)
    int32 LosingFactionSlot = -1;

    // Score needed to win, published by the match's win condition (see
    // FConnectItRuleSet::StampWinState) so UI can render "57 / 100" without
    // knowing which condition is in play or reaching the server-only rules.
    // Rides this single replicated snapshot rather than adding a second
    // replicated property. 0 means "this win condition isn't score-based" --
    // use WinProgress for a bar that works under any condition.
    UPROPERTY(BlueprintReadOnly)
    float TargetScore = 0.f;

    // How close each faction slot is to winning, 0 to 1, under whatever the
    // match's win condition is (FConnectItMatchCondition::GetProgress).
    // Stamped alongside TargetScore after every move; empty until the first.
    UPROPERTY(BlueprintReadOnly)
    TArray<float> WinProgress;

    // How many of its win condition groups each faction slot has completed
    // (FConnectItRuleSet::PlayerWinGroups / AIWinGroups), in order. Groups stay
    // completed once they are. A faction has won when this reaches the number
    // of groups it has. Kept here, not in the rules, so the rules stay plain
    // shared data and the AI's search carries each side's progress with the
    // board. Empty on a board built without it (nobody has completed anything).
    UPROPERTY(BlueprintReadOnly)
    TArray<int32> GroupsCompleted;

    // How many pieces each faction slot started the match with (the designer's
    // starting pieces), set once when the board is built. A win condition about
    // piece counts measures its progress from this, so the designer never has
    // to type the number in. Empty on a board built without it.
    UPROPERTY(BlueprintReadOnly)
    TArray<int32> StartingPieces;

    // The phase a phased level is in (a ConnectIt.Phase.* tag), set by the
    // Level director's Begin Phase; empty until one begins. On the board so
    // clients and UI can read it, and so it changes in order with everything
    // else.
    UPROPERTY(BlueprintReadOnly)
    FGameplayTag CurrentPhase;

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

    // A new tile joins the board at Position: Data, given the board's next
    // tile id. Returns the id.
    int32 AddTile(const FGridPosition& Position, FConnectItTileData Data)
    {
        Data.TileId = NextTileId++;
        SetTileData(Position, Data);
        return Data.TileId;
    }

    // Take a tile off the board, with whatever is on it (the other tiles keep
    // their order). False if there was no tile there.
    bool RemoveTileData(const FGridPosition& Position)
    {
        const int32 Index = TilePositions.IndexOfByKey(Position);
        if (!TileDataArray.IsValidIndex(Index)) return false;

        TilePositions.RemoveAt(Index);
        TileDataArray.RemoveAt(Index);
        return true;
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

    // Whether Other has the same tiles (by position, in any order) holding
    // the same data (piece, multiplier, flags, id, power), and the same scores. The match-level fields (turn, win
    // state, progress) aren't compared. Checks that a board stepped along by
    // its events (FConnectItBoardEvent::ApplyTo) arrived where the change did.
    bool HasSameTilesAndScores(const FConnectItBoardState& Other) const
    {
        if (TilePositions.Num() != Other.TilePositions.Num()) return false;
        if (ScoreBoard.Num() != Other.ScoreBoard.Num()) return false;

        for (int32 Index = 0; Index < ScoreBoard.Num(); Index++)
        {
            if (!FMath::IsNearlyEqual(ScoreBoard[Index], Other.ScoreBoard[Index])) return false;
        }

        for (int32 Index = 0; Index < TilePositions.Num(); Index++)
        {
            const FConnectItTileData* Theirs = Other.GetTileData(TilePositions[Index]);
            if (!Theirs) return false;

            const FConnectItTileData& Ours = TileDataArray[Index];
            if (Ours.FactionPiece != Theirs->FactionPiece
                || !FMath::IsNearlyEqual(Ours.Multiplier, Theirs->Multiplier)
                || Ours.bIsActive != Theirs->bIsActive
                || Ours.bIsOccupied != Theirs->bIsOccupied
                || Ours.bCanShift != Theirs->bCanShift
                || Ours.TileId != Theirs->TileId
                || Ours.TilePowerTag != Theirs->TilePowerTag
                || Ours.PowerReadyTurn != Theirs->PowerReadyTurn
                || !(Ours.TilePower == Theirs->TilePower))
            {
                return false;
            }
        }
        return true;
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
