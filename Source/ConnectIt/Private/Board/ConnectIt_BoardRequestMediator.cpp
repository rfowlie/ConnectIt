// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BoardRequestMediator.h"
#include "ConnectIt_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "TurnBasedMechanicsStructs.h"
#include "Action/TurnBasedAction.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Framework/PlayerState/ConnectIt_PlayerState.h"
#include "GameEvent/ConnectIt_PlacePieceGameEvent.h"


void UConnectIt_BoardRequestMediator::Initialise(const FConnectItRuleSet* InRules)
{
    Rules = InRules;
}

// UConnectIt_BoardStateComponent* UConnectIt_BoardRequestMediator::GetBoardState() const
// {
//     const AConnectIt_GameState* GS = GetWorld() ? GetWorld()->GetGameState<AConnectIt_GameState>() : nullptr;
//     return IsValid(GS) ? GS->GetBoardStateComponent() : nullptr;
// }

void UConnectIt_BoardRequestMediator::CreateGameEventsFromBoardUpdate_Implementation()
{
    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    if (!IsValid(BoardState))
    {
        UE_LOG(LogTemp, Error, TEXT(
            "UConnectIt_BoardRequestMediator::CreateGameEventsFromBoardUpdate — BoardStateComponent is null"));
        return;
    }

    FConnectItBoardChangeEvent ChangeEvent = BoardState->GetChangeEvent();
    if (ChangeEvent.bPiecePlaced)
    {
        // create place piece game event
        // or initialize reusable game event and add to queue
        TurnBasedGameEventQueue.Enqueue(GameEventPlacePiece);
    }

    ExecuteGameEvents();
}

void UConnectIt_BoardRequestMediator::ExecuteGameEvents()
{
    // TODO queue wait system to run each game event waiting for it's OnComplete To Fire
    // this is an alternative to the GameEventSubsystem but this will allow us to more clearly
    // group and sequence the visual effects we want from each game event
}

// --- Request Processing ---

bool UConnectIt_BoardRequestMediator::ProcessRequest(const FTurnActionRequest& Request)
{
    // Every board-change request is spent against an action in the requester's
    // loadout, and the requester's PlayerState holds the authoritative
    // per-action state. There is no ungated path: a player with no action
    // state (e.g. the PlayerState was never seeded) can do nothing.
    AConnectIt_PlayerState* PlayerState = Request.FactionID >= 0
        ? UConnectIt_GameUtilityLibrary::GetPlayerStateForFaction(this, Request.FactionID)
        : nullptr;

    if (!IsValid(PlayerState) || !PlayerState->HasActionConfig())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardRequestMediator: request '%s' rejected -- "
                 "faction %d has no action state (PlayerState missing, or not "
                 "seeded from a loadout with PermanentActions/NumberedActions)"),
            *Request.RequestType.ToString(), Request.FactionID);
        return false;
    }

    // ActionTag is client-supplied: it must name an action in this player's
    // loadout, that action must be allowed to produce this RequestType, and
    // the player must currently be able to use it.
    const TSubclassOf<UTurnBasedAction> ActionClass =
        PlayerState->FindActionClassByTag(Request.ActionTag);
    if (!ActionClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: request '%s' rejected -- "
                 "ActionTag '%s' is not in faction %d's loadout"),
            *Request.RequestType.ToString(),
            *Request.ActionTag.ToString(), Request.FactionID);
        return false;
    }

    const UTurnBasedAction* DefaultAction = ActionClass->GetDefaultObject<UTurnBasedAction>();
    if (!IsValid(DefaultAction) || !DefaultAction->ProducesRequestType(Request.RequestType))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: request '%s' rejected -- "
                 "action '%s' does not produce that request type"),
            *Request.RequestType.ToString(), *ActionClass->GetName());
        return false;
    }

    if (!PlayerState->CanUseAction(ActionClass))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: request '%s' rejected -- "
                 "faction %d cannot use '%s' right now (no uses left, "
                 "per-turn cap reached, or on cooldown)"),
            *Request.RequestType.ToString(), Request.FactionID,
            *ActionClass->GetName());
        return false;
    }

    const bool bSucceeded = DispatchRequest(Request);

    // Spend the use only once the change has actually been committed --
    // never burn one on a request rejected above or by its handler.
    if (bSucceeded)
    {
        if (!PlayerState->ConsumeActionUse(ActionClass))
        {
            UE_LOG(LogTemp, Error,
                TEXT("ConnectIt_BoardRequestMediator: '%s' committed but its "
                     "use could not be consumed on faction %d"),
                *ActionClass->GetName(), Request.FactionID);
        }
    }

    return bSucceeded;
}

bool UConnectIt_BoardRequestMediator::DispatchRequest(const FTurnActionRequest& Request)
{
    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    if (!IsValid(BoardState))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardRequestMediator: ProcessRequest — "
                 "BoardStateComponent is null"));
        return false;
    }

    // Universal choke point regardless of caller (player-controller RPC or
    // AI controller's direct server-side call) -- once the game is over,
    // no further board mutation is possible, full stop.
    if (BoardState->GetCurrentState().bGameOver)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: ProcessRequest rejected — "
                 "game already over"));
        return false;
    }

    if (!Request.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: Received invalid "
                 "FTurnActionRequest"));
        return false;
    }

    // The payload is the board change itself. It is client-supplied, so:
    // it must be an operation at all...
    const FConnectItBoardOperation* Requested = Request.Payload.GetPtr<FConnectItBoardOperation>();
    if (!Requested)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: '%s' rejected -- the request's "
                 "payload is missing or is not a board operation"),
            *Request.RequestType.ToString());
        return false;
    }

    // ...and of the kind the request claims. ProcessRequest's gate approved
    // Request.RequestType for this player's action; without this check a
    // request could name one type and carry another kind of operation.
    if (Requested->GetRequestType() != Request.RequestType)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: '%s' rejected -- its payload "
                 "is a '%s' operation"),
            *Request.RequestType.ToString(), *Requested->GetRequestType().ToString());
        return false;
    }

    // Work on our own copy, made by the requester the server knows -- never
    // by whoever the payload says
    FInstancedStruct OperationStorage = Request.Payload;
    FConnectItBoardOperation& Operation = OperationStorage.GetMutable<FConnectItBoardOperation>();
    Operation.Faction = Request.FactionID;

    UConnectIt_BoardStateComponent* MutableBoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    const FConnectItBoardState& Current = MutableBoardState->GetCurrentState();

    // May this be done to the board as it stands?
    FString WhyNot;
    if (!Operation.CanApply(Current, &WhyNot))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: '%s' by faction %d rejected "
                 "-- %s"),
            *Request.RequestType.ToString(), Operation.Faction, *WhyNot);
        return false;
    }

    // What happened is recorded in a change event that replicates alongside
    // the state itself via SetBoardState, instead of broadcasting gameplay
    // delegates directly here. This only ever runs on the server, so a direct
    // broadcast would never reach a real remote client.
    // ConnectIt_BoardStateComponent reads the event back from its own
    // BoardSnapshot.ChangeEvent and enqueues the matching event tags on
    // UGameEventTaskSubsystem itself, symmetrically on both server (from
    // SetBoardState) and client (from OnRep) -- this mediator plays no role
    // in sequencing, only in deciding what happened.
    FConnectItBoardChangeEvent ChangeEvent;

    // The change itself: the operation only changes the board (and says what
    // it did)...
    FConnectItBoardState NewState = Current;
    FConnectItTouchedPositions TouchedPositions;
    Operation.Apply(NewState, TouchedPositions, &ChangeEvent);

    // ...what follows from the change is a separate step: scoring where
    // pieces arrived, then whether anyone has now won
    const float PointsScored =
        Rules->ResolveBoardChange(NewState, TouchedPositions, &ChangeEvent.ScoringConfigurations);
    Rules->StampWinState(NewState);

    // Edge-triggered -- true only on the transition into game-over, not
    // "the game is currently over" (Current.bGameOver would already be
    // true on every snapshot after the winning move)
    ChangeEvent.bGameWon           = NewState.bGameOver && !Current.bGameOver;
    ChangeEvent.WinningFactionSlot = NewState.WinningFactionSlot;

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_BoardRequestMediator: '%s' %s by faction %d (scored %.0f)"),
        *Request.RequestType.ToString(), *Operation.Describe(), Operation.Faction, PointsScored);

    MutableBoardState->SetBoardState(NewState, ChangeEvent);
    return true;
}
