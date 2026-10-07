// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GridMechanicsBaseEnums.h"
#include "GridMechanicsBaseStructs.h"
#include "ConnectIt_BoardEvents.generated.h"

// One thing that happened to the board, as its own small struct holding only
// what describes it. A board change (FConnectItBoardChangeEvent) is an
// ordered list of these, in the order they happened -- e.g. a piece was
// placed, then a line scored, then the game was won.
//
// The list replicates with the board. On every machine the board state
// component plays the events one at a time through the game event queue:
// each event's gameplay tag is queued with the event itself as payload, and
// a listener for that tag reads its event from the matching
// UConnectIt_BoardEventLibrary::GetActive...Event function.
//
// Two categories, told apart by base struct:
//   * FConnectItBoardOperationEvent -- what a board operation did
//     (appended by FConnectItBoardOperation::Apply);
//   * FConnectItBoardResultEvent    -- what followed from it (scoring,
//     appended by the scoring rule; winning, appended by the Mediator).
//
// Plain data: every field a UPROPERTY (they replicate, and Blueprint reads
// them), no UObject references. A new kind of event is a new struct here,
// plus (if it has data) its GetActive...Event function in
// UConnectIt_BoardEventLibrary.


USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItBoardEvent
{
    GENERATED_BODY()

    virtual ~FConnectItBoardEvent() = default;

    // The game event tag listeners bind to for this kind of event
    // (ConnectIt.Event.*)
    virtual FGameplayTag GetEventTag() const { return FGameplayTag(); }
};

// What a board operation did
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItBoardOperationEvent : public FConnectItBoardEvent
{
    GENERATED_BODY()
};

// What followed from a board operation
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItBoardResultEvent : public FConnectItBoardEvent
{
    GENERATED_BODY()
};

// The board was just initialised from the level: there may be starting pieces
// on tiles that need their visuals created. Only on the initial snapshot.
USTRUCT(BlueprintType, meta = (DisplayName = "Board Seeded"))
struct CONNECTIT_API FConnectItBoardEvent_BoardSeeded : public FConnectItBoardEvent
{
    GENERATED_BODY()

    virtual FGameplayTag GetEventTag() const override;
};

// ---------------------------------------------------------------------------
// Operation events
// ---------------------------------------------------------------------------

// A faction's piece arrived at a tile (Place Piece, Force Place Piece)
USTRUCT(BlueprintType, meta = (DisplayName = "Piece Placed"))
struct CONNECTIT_API FConnectItBoardEvent_PiecePlaced : public FConnectItBoardOperationEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    FGridPosition Position;

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    int32 Faction = -1;

    virtual FGameplayTag GetEventTag() const override;
};

// The pieces on two tiles traded places
USTRUCT(BlueprintType, meta = (DisplayName = "Pieces Swapped"))
struct CONNECTIT_API FConnectItBoardEvent_PiecesSwapped : public FConnectItBoardOperationEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    FGridPosition PositionA;

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    FGridPosition PositionB;

    virtual FGameplayTag GetEventTag() const override;
};

// A line of tiles shifted one step. StartPositions / EndPositions say where
// each shifted tile's data came from and went to -- parallel arrays,
// index-aligned (StartPositions[i] -> EndPositions[i]). Only the tiles that
// actually moved appear: a tile that can't shift stays put and is left out.
USTRUCT(BlueprintType, meta = (DisplayName = "Board Shifted"))
struct CONNECTIT_API FConnectItBoardEvent_BoardShifted : public FConnectItBoardOperationEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    EGridDirection Direction = EGridDirection::Max;

    // The tile the shift was started from
    UPROPERTY(BlueprintReadOnly, Category = "Event")
    FGridPosition AnchorPosition;

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    TArray<FGridPosition> StartPositions;

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    TArray<FGridPosition> EndPositions;

    virtual FGameplayTag GetEventTag() const override;
};

// A piece changed owner where it stood
USTRUCT(BlueprintType, meta = (DisplayName = "Piece Captured"))
struct CONNECTIT_API FConnectItBoardEvent_PieceCaptured : public FConnectItBoardOperationEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    FGridPosition Position;

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    int32 CapturingFaction = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    int32 PreviousFaction = -1;

    virtual FGameplayTag GetEventTag() const override;
};

// A piece was taken off the board
USTRUCT(BlueprintType, meta = (DisplayName = "Piece Removed"))
struct CONNECTIT_API FConnectItBoardEvent_PieceRemoved : public FConnectItBoardOperationEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    FGridPosition Position;

    // Whose piece it was
    UPROPERTY(BlueprintReadOnly, Category = "Event")
    int32 RemovedFaction = -1;

    virtual FGameplayTag GetEventTag() const override;
};

// A tile's score multiplier was reset to 1
USTRUCT(BlueprintType, meta = (DisplayName = "Tile Multiplier Destroyed"))
struct CONNECTIT_API FConnectItBoardEvent_TileMultiplierDestroyed : public FConnectItBoardOperationEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    FGridPosition Position;

    virtual FGameplayTag GetEventTag() const override;
};

// A tile was switched between active and inactive
USTRUCT(BlueprintType, meta = (DisplayName = "Tile Active Toggled"))
struct CONNECTIT_API FConnectItBoardEvent_TileActiveToggled : public FConnectItBoardOperationEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    FGridPosition Position;

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    bool bNowActive = false;

    virtual FGameplayTag GetEventTag() const override;
};

// ---------------------------------------------------------------------------
// Result events
// ---------------------------------------------------------------------------

// One thing scored: who, how much, and the tiles that took part. What counts
// as "one thing" is up to the match's scoring rule (for the Lines rule: one
// completed line). A move that scores several ways, or for several factions,
// produces several of these, and their Positions can overlap (two lines
// crossing at the piece that completed both).
USTRUCT(BlueprintType, meta = (DisplayName = "Scored"))
struct CONNECTIT_API FConnectItBoardEvent_Scored : public FConnectItBoardResultEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    int32 Faction = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    float Points = 0.f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Event")
    TArray<FGridPosition> Positions;

    // The positions whose piece this score removed from the board -- what
    // visuals should despawn. A subset of Positions: it leaves out a
    // completing piece that stays, and a position already cleared by an
    // earlier Scored event of the same change (two lines crossing at a piece
    // that doesn't stay).
    UPROPERTY(BlueprintReadOnly, Category = "Event")
    TArray<FGridPosition> ClearedPositions;

    virtual FGameplayTag GetEventTag() const override;
};

// The game was just won. Appears once, on the change that ended the game
// (FConnectItBoardState::bGameOver stays true on every snapshot after it).
USTRUCT(BlueprintType, meta = (DisplayName = "Game Won"))
struct CONNECTIT_API FConnectItBoardEvent_GameWon : public FConnectItBoardResultEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Event")
    int32 WinningFaction = -1;

    virtual FGameplayTag GetEventTag() const override;
};
