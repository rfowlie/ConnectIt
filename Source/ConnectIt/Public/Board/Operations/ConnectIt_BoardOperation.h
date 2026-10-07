// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ConnectIt_Structs.h"
#include "ConnectIt_BoardOperation.generated.h"


// The positions where a piece arrived or changed owner, reported by
// FConnectItBoardOperation::Apply for the after-move step
// (FConnectItRuleSet::ResolveBoardChange scores exactly these). Inline
// storage: no heap allocation for ordinary moves in the AI's search.
using FConnectItTouchedPositions = TArray<FGridPosition, TInlineAllocator<8>>;

// One specific change to the board, carrying everything it needs: who is
// making it (Faction) and, in each kind of operation, what it acts on (place
// a piece on THIS tile, swap THESE two...). It knows whether it may be made
// (CanApply) and how it changes the board (Apply). It does not score, check
// for a win, or know whose turn is next: after Apply, the caller runs
// FConnectItRuleSet::ResolveBoardChange on the positions it reports.
//
// An operation IS the request: a player's action (UConnectIt_PlacePieceAction...)
// builds one and sends it as FTurnActionRequest::Payload; the server's
// Mediator validates and applies that same struct. The AI's search builds
// them too, and applies them to copies of the board.
//
// Because of that it must stay a plain struct, like the rules (see
// ConnectIt_ScoringRule.h): a pure function of its own fields and the board
// it is handed, no UObject references. And every field must be a UPROPERTY,
// or it won't travel with the request to the server.
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItBoardOperation
{
    GENERATED_BODY()

    virtual ~FConnectItBoardOperation() = default;

    // Who is making the move. When an operation arrives in a request this is
    // client-supplied, so the server overwrites it with the requester's
    // faction before using the operation.
    UPROPERTY(BlueprintReadWrite, Category = "Operation")
    int32 Faction = INDEX_NONE;

    // The request type this kind of operation is (e.g.
    // ConnectIt_Game_PlacePiece). The server's gate decides from the request
    // type whether the player's action may send this, so it checks the
    // operation it was sent really is of the type the request claims.
    virtual FGameplayTag GetRequestType() const { return FGameplayTag(); }

    // Whether this operation may be made on Board. OutWhyNot, if given, gets
    // a reason for the log.
    virtual bool CanApply(const FConnectItBoardState& Board, FString* OutWhyNot = nullptr) const
    {
        return false;
    }

    // Changes Board -- nothing else. Only call when CanApply.
    // OutTouched: appends every position a piece arrived on or changed owner
    // at (the positions that might now score).
    // OutEvents: if not null, the operation appends the event describing
    // what it did (an FConnectItBoardOperationEvent, e.g. Piece Placed +
    // where + who) for visuals to react to. The AI's search passes null.
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItTouchedPositions& OutTouched,
        FConnectItBoardChangeEvent* OutEvents) const
    {
    }

    // Short text for logs, e.g. "(3,2)"
    virtual FString Describe() const { return FString(); }
};
