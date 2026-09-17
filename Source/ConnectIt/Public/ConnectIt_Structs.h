// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseEnums.h"
#include "GridMechanicsBaseStructs.h"
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
    // UConnectIt_BoardRequestMediator::HandleBoardShiftRequest.
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

    // Score needed to win, published by whichever IConnectIt_WinCondition is
    // active (see UConnectIt_BoardRulesComponent::GetTargetScore) so UI can
    // render "57 / 100" without knowing which strategy is in play or
    // reaching into a server-side strategy object that isn't replicated.
    // Rides this single replicated snapshot rather than adding a second
    // replicated property. 0 means "this win condition isn't score-based" --
    // a progress bar should hide itself rather than divide by zero.
    UPROPERTY(BlueprintReadOnly)
    float TargetScore = 0.f;

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

    // Every tile that was part of a completed line on this placement --
    // the union across all lines if more than one completed simultaneously
    // (e.g. a horizontal and a diagonal through the same piece), not kept
    // separate per line since USTRUCT arrays-of-arrays don't replicate
    // (same reason shift below uses parallel arrays instead of a TMap/TSet).
    // Empty when bLineScored is false.
    UPROPERTY(BlueprintReadOnly)
    TArray<FGridPosition> ScoringLinePositions;

    UPROPERTY(BlueprintReadOnly)
    bool bGameWon = false;

    UPROPERTY(BlueprintReadOnly)
    int32 WinningFactionSlot = -1;

    // --- Tile Multiplier Destroyed --- (UConnectIt_TileMultiplierDestroyerAction)

    UPROPERTY(BlueprintReadOnly)
    bool bTileMultiplierDestroyed = false;

    UPROPERTY(BlueprintReadOnly)
    FGridPosition MultiplierDestroyedPosition;

    // --- Piece Removed --- (UConnectIt_TimedPieceRemoverAction)

    UPROPERTY(BlueprintReadOnly)
    bool bPieceRemoved = false;

    UPROPERTY(BlueprintReadOnly)
    FGridPosition RemovedPosition;

    UPROPERTY(BlueprintReadOnly)
    int32 RemovedFactionSlot = -1;

    // --- Pieces Swapped --- (UConnectIt_PieceSwapperAction)

    UPROPERTY(BlueprintReadOnly)
    bool bPiecesSwapped = false;

    UPROPERTY(BlueprintReadOnly)
    FGridPosition SwapPositionA;

    UPROPERTY(BlueprintReadOnly)
    FGridPosition SwapPositionB;

    // --- Tile Active Toggled --- (UConnectIt_TileActivationToggleAction)

    UPROPERTY(BlueprintReadOnly)
    bool bTileActiveToggled = false;

    UPROPERTY(BlueprintReadOnly)
    FGridPosition ToggledPosition;

    UPROPERTY(BlueprintReadOnly)
    bool bToggledPositionNowActive = false;

    // --- Piece Captured --- (UConnectIt_PieceCaptureAction)

    UPROPERTY(BlueprintReadOnly)
    bool bPieceCaptured = false;

    UPROPERTY(BlueprintReadOnly)
    FGridPosition CapturedPosition;

    UPROPERTY(BlueprintReadOnly)
    int32 CapturingFactionSlot = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 PreviousFactionSlot = -1;

    // --- Board Shifted --- (UConnectIt_BoardShiftAction)

    UPROPERTY(BlueprintReadOnly)
    bool bBoardShifted = false;

    UPROPERTY(BlueprintReadOnly)
    EGridDirection ShiftDirection = EGridDirection::Max;

    UPROPERTY(BlueprintReadOnly)
    FGridPosition ShiftAnchorPosition;

    // Where each shifted tile's data came from and went to -- parallel arrays,
    // index-aligned (ShiftStartPositions[i] -> ShiftEndPositions[i]), not a
    // TMap: this struct rides inside BoardSnapshot, which replicates via
    // standard UPROPERTY(ReplicatedUsing=...) property replication, and TMap
    // isn't net-serializable through that path (same reason
    // FConnectItBoardState uses TilePositions/TileDataArray instead of a
    // TMap -- a TMap field here would populate on the server and silently
    // stay empty on every client). Build a local TMap from these two arrays
    // client-side if a lookup-by-start-position is actually needed; don't
    // replicate one. Only the tiles that actually moved appear here -- a
    // subset of the full selected line when any tile in it has bCanShift
    // false (those are skipped, keep their own data, and are left out).
    UPROPERTY(BlueprintReadOnly)
    TArray<FGridPosition> ShiftStartPositions;

    UPROPERTY(BlueprintReadOnly)
    TArray<FGridPosition> ShiftEndPositions;
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

// FTurnActionRequest payload -- UConnectIt_TileMultiplierDestroyerAction
USTRUCT(BlueprintType)
struct FConnectItRequestDestroyTileMultiplier
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGridPosition Position;
};

// FTurnActionRequest payload -- UConnectIt_TimedPieceRemoverAction.
// DelayTurns is forward-declared for a delayed/scheduled follow-up that
// doesn't exist yet -- see that action's class comment. The current board
// manager handler only honours DelayTurns == 0 (immediate removal).
USTRUCT(BlueprintType)
struct FConnectItRequestRemovePiece
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGridPosition Position;

    UPROPERTY(BlueprintReadWrite, meta = (ClampMin = 0))
    int32 DelayTurns = 0;
};

// FTurnActionRequest payload -- UConnectIt_PieceSwapperAction
USTRUCT(BlueprintType)
struct FConnectItRequestSwapPieces
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGridPosition PositionA;

    UPROPERTY(BlueprintReadWrite)
    FGridPosition PositionB;
};

// FTurnActionRequest payload -- UConnectIt_TileActivationToggleAction
USTRUCT(BlueprintType)
struct FConnectItRequestToggleTileActive
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGridPosition Position;
};

// FTurnActionRequest payload -- UConnectIt_PieceCaptureAction. No explicit
// capturing faction field -- FTurnActionRequest::FactionID on the envelope
// is already "who's making this request" for every request type.
USTRUCT(BlueprintType)
struct FConnectItRequestCapturePiece
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGridPosition Position;
};

// FTurnActionRequest payload -- UConnectIt_ForcePiecePlaceOnTileAction.
// Deliberately the same shape as FConnectItRequestPlacePiece (not reused
// directly) -- keeps this request type free to diverge later (e.g. an
// explicit faction override) without touching the normal placement payload.
USTRUCT(BlueprintType)
struct FConnectItRequestForcePlacePiece
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGridPosition Position;
};

// FTurnActionRequest payload -- UConnectIt_BoardShiftAction. Positions is the
// full line to be shifted (see UGridTileRegistryBase::GetTilesByDirection),
// in line order from the selected tile outward, so the Mediator doesn't have
// to re-walk the line itself; Direction says which way it shifts. Still
// re-validated server-side against the actual board state before use, same
// as every other request payload -- never trusted blindly.
USTRUCT(BlueprintType)
struct FConnectItRequestBoardShift
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FGridPosition> Positions;

    UPROPERTY(BlueprintReadWrite)
    EGridDirection Direction = EGridDirection::Max;
};