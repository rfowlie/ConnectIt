// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ConnectIt_Structs.h"
#include "Board/ConnectIt_BoardChanges.h"
#include "ConnectIt_BoardOperation.generated.h"

// Whether a faction has any legal move of one kind on a board
// (FConnectItBoardOperation::HasAnyLegalInstance)
UENUM(BlueprintType)
enum class EConnectItMoveLegality : uint8
{
    // This kind of move can't tell (yet): treat it as possibly legal
    Unknown         UMETA(DisplayName = "Unknown"),
    // At least one is possible
    Legal           UMETA(DisplayName = "Legal"),
    // None is possible
    NoLegalMove     UMETA(DisplayName = "No Legal Move"),
};

// One specific change to the board, carrying everything it needs: who is
// making it (Faction) and, in each kind of operation, what it acts on (place
// a piece on THIS tile, swap THESE two...). It knows whether it may be made
// (CanApply) and how it changes the board (Apply). It does not score, check
// for a win, or know whose turn is next: after Apply, the caller runs
// FConnectItRuleSet::ResolveBoardChange on the changes it reports.
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
    // OutChanges: what it really did -- pieces that entered, left, moved or
    // changed owner, tiles that changed (FConnectItBoardChanges). The
    // after-move step scores from it, and tile powers react to it.
    // OutEvents: if not null, the operation appends the event describing
    // what it did (an FConnectItBoardOperationEvent, e.g. Piece Placed +
    // where + who) for visuals to react to. The AI's search passes null.
    virtual void Apply(
        FConnectItBoardState& Board,
        FConnectItBoardChanges& OutChanges,
        FConnectItBoardChangeEvent* OutEvents) const
    {
    }

    // Whether a piece this operation brought onto a tile stays there when
    // that piece completes something that scores. True for most (placing the
    // fourth piece of a line clears the other three and leaves it standing);
    // an operation returns false to have its arriving pieces cleared along
    // with the rest. Each operation stamps it on the arrivals it reports
    // (FConnectItPieceArrival::bSurvivesScoring), so one scoring pass can mix
    // arrivals from different operations.
    virtual bool ArrivingPiecesSurviveScoring() const { return true; }

    // Short text for logs, e.g. "(3,2)"
    virtual FString Describe() const { return FString(); }

    // Whether ForFaction could make ANY operation of this kind on Board -- e.g.
    // Place Piece: is any tile placeable? Asked of a default instance (this
    // operation's own fields are ignored). Board only: says nothing about the
    // faction's loadout or uses left. Default: Unknown (this kind of move
    // can't tell yet).
    virtual EConnectItMoveLegality HasAnyLegalInstance(const FConnectItBoardState& Board, int32 ForFaction) const
    {
        return EConnectItMoveLegality::Unknown;
    }

    // HasAnyLegalInstance for the kind of operation whose request type is
    // RequestType. Unknown for a tag that isn't a board operation. Safe on any
    // thread once GetAllRequestTypes has run on the game thread (the GameMode
    // does it at match setup).
    static EConnectItMoveLegality GetLegality(FGameplayTag RequestType, const FConnectItBoardState& Board, int32 ForFaction);

    // The request type of every kind of board operation there is (every
    // C++ struct deriving from this one), found once by reflection. Lets
    // code ask "which kinds of move can this action produce?" of an action,
    // which can only answer yes or no per type (ProducesRequestType).
    static const TArray<FGameplayTag>& GetAllRequestTypes();
};
