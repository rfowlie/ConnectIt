// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "TurnBasedMechanicsStructs.h"
#include "UObject/Object.h"
#include "ConnectIt_BoardRequestMediator.generated.h"

class UConnectIt_PlacePieceGameEvent;
class UTurnBasedGameEvent;
struct FConnectItRuleSet;
class UConnectIt_BoardStateComponent;

// TODO: should adjust this to listen to server player controller broadcasts
// that way the flow is the player broadcasts a request instead of reaching into here force the request...

// Accepts and carries out board-change requests -- the server-only successor
// to AConnectIt_BoardManager::ProcessRequest, now a plain UObject
// constructed and owned by AConnectIt_GameMode instead of a world Actor.
// Structurally unreachable from any client: GetWorld()->GetAuthGameMode()
// (and therefore this object, which only ever exists as a GameMode member)
// is null on every client by engine design -- the old ProcessRequest's
// `if (!HasAuthority()) return false;` guard is gone, not replaced, since
// there's no longer a path for it to be reachable in the first place.
//
// It knows nothing about any particular kind of board change. A request's
// payload IS the change -- a FConnectItBoardOperation (place this piece here,
// swap these two...) -- and every request takes the same path:
//   1. ProcessRequest: may this player's action send this kind of operation
//      now?
//   2. DispatchRequest: the operation checks itself against the board
//      (CanApply), changes a copy of it (Apply), the match's rule set
//      resolves what follows (scoring, then win state), and the new board is
//      committed with a change event describing what happened.
// The AI's search runs the same operations and the same resolve step.
//
// NOTE/OneVerified: the tag-reactive interpreter pipeline that used to turn
// board-change tags into piece spawn/despawn calls has been removed
// project-wide. TurnBasedGameEventQueue/CreateGameEventsFromBoardUpdate/
// ExecuteGameEvents below are its still-in-progress replacement, ported
// verbatim from AConnectIt_BoardManager -- ExecuteGameEvents has no body
// yet, so nothing is reactively driven from board-state changes today.
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API UConnectIt_BoardRequestMediator : public UObject
{
    GENERATED_BODY()

public:

    // Called once by AConnectIt_GameMode right after construction.
    // InRules is the GameMode's per-match rule set; it outlives this object
    // (the GameMode owns both).
    void Initialise(const FConnectItRuleSet* InRules);

    // Entry point for all board change requests -- see
    // AConnectIt_GameMode::ProcessBoardRequest, the only intended caller.
    // Request.Payload must hold a FConnectItBoardOperation. RequestingFaction
    // is who is asking, as the server knows it (the sending controller's
    // PlayerState slot): a request doesn't say, and the operation's own
    // Faction field is overwritten with it. Returns whether the request succeeded -- the
    // caller (AConnectIt_PlayerController) reports this back to the
    // requesting client via ClientNotifyBoardChangeOutcome so
    // UTurnBasedActionsComponent can resolve its awaiting-confirmation state.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    bool ProcessRequest(const FTurnActionRequest& Request, int32 RequestingFaction);

    // True while a request is being carried out. The player's use of the
    // action is spent only after that returns, so the loadout must not be
    // swapped out from under it (see AConnectIt_GameMode::SetLoadout). Board
    // event listeners can run inside it: committing the new board state queues
    // the move's events, and an event with nothing to wait for completes at once.
    bool IsDispatching() const { return bDispatching; }

    // Carries out a board operation on the level's behalf, outside any
    // player's move: the same checks and steps as a move (CanApply, the
    // operation, scoring, the win check, commit with events) but no loadout
    // gate and no action use. AsFaction: who it is made by (INDEX_NONE for an
    // operation that belongs to nobody). Server only; the GameMode calls it
    // when no move is being carried out.
    bool ProcessLevelOperation(const FInstancedStruct& Operation, int32 AsFaction);

protected:

    // --- Game Events ---
    TQueue<UTurnBasedGameEvent*> TurnBasedGameEventQueue;

    // read the board state and determine what events need to occur and in what order
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ConnectIt|Game Board")
    void CreateGameEventsFromBoardUpdate();
    void ExecuteGameEvents();

    UPROPERTY(Instanced, EditDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Game Board")
    UTurnBasedGameEvent* GameEventTest = nullptr;

    UPROPERTY(Instanced, EditDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Game Board")
    UConnectIt_PlacePieceGameEvent* GameEventPlacePiece = nullptr;

private:

    // Carries out the request's operation (step 2 above). ProcessRequest
    // wraps it with the per-action gate (can this player use this action
    // right now?) and the spend of that use once the change has been
    // committed.
    bool DispatchRequest(const FInstancedStruct& Payload, int32 RequestingFaction);

    bool bDispatching = false;

    // Board state lives on AConnectIt_GameState -- resolved through here
    // rather than repeating GetWorld()->GetGameState<>() at each call site.
    // UConnectIt_BoardStateComponent* GetBoardState() const;

    const FConnectItRuleSet* Rules = nullptr;
};
