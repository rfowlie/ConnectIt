// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_Structs.h"
#include "TurnBasedMechanicsStructs.h"
#include "UObject/Object.h"
#include "ConnectIt_BoardRequestMediator.generated.h"

class UConnectIt_PlacePieceGameEvent;
class UTurnBasedGameEvent;
class UConnectIt_BoardRules;
class UConnectIt_BoardStateComponent;

// Accepts and dispatches board-change requests -- the server-only successor
// to AConnectIt_BoardManager::ProcessRequest, now a plain UObject
// constructed and owned by AConnectIt_GameMode instead of a world Actor.
// Structurally unreachable from any client: GetWorld()->GetAuthGameMode()
// (and therefore this object, which only ever exists as a GameMode member)
// is null on every client by engine design -- the old ProcessRequest's
// `if (!HasAuthority()) return false;` guard is gone, not replaced, since
// there's no longer a path for it to be reachable in the first place.
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
    void Initialise(UConnectIt_BoardRules* InBoardRules);

    // Entry point for all board change requests -- see
    // AConnectIt_GameMode::ProcessBoardRequest, the only intended caller.
    // Dispatches by RequestType, unwrapping Request.Payload into whichever
    // concrete struct that type expects (see FTurnActionRequest) and
    // routing to the matching private HandleXRequest below. Returns
    // whether the request succeeded -- the caller (AConnectIt_PlayerController)
    // reports this back to the requesting client via ClientNotifyBoardChangeOutcome
    // so UTurnBasedActionsComponent can resolve its awaiting-confirmation state.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    bool ProcessRequest(const FTurnActionRequest& Request);

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

    // Board state lives on AConnectIt_GameState -- resolved through here
    // rather than repeating GetWorld()->GetGameState<>() at each call site.
    UConnectIt_BoardStateComponent* GetBoardState() const;

    UPROPERTY()
    TObjectPtr<UConnectIt_BoardRules> BoardRules = nullptr;

    // --- Request Handlers ---
    // FactionID is passed separately rather than living on each payload
    // struct -- it's the one piece of data every request type needs, so it
    // stays on FTurnActionRequest's envelope instead of being duplicated
    // into each request payload struct.
    bool HandlePlacePieceRequest(const FConnectItRequestPlacePiece& Request, int32 FactionID) const;

    // Same as HandlePlacePieceRequest but skips IsTileValidForPlacement --
    // only requires the position to exist in the registry, so it can place
    // on an inactive or already-occupied tile (overwriting it). Produces the
    // same ChangeEvent shape (bPiecePlaced) since visually it's the same
    // kind of event as a normal placement.
    bool HandleForcePlacePieceRequest(const FConnectItRequestForcePlacePiece& Request, int32 FactionID) const;

    bool HandleDestroyTileMultiplierRequest(const FConnectItRequestDestroyTileMultiplier& Request) const;

    // DelayTurns > 0 is rejected (logged) rather than silently treated as
    // immediate -- delayed/scheduled removal needs a per-turn ticking
    // mechanism this mediator doesn't have yet. See
    // FConnectItRequestRemovePiece's comment.
    bool HandleRemovePieceRequest(const FConnectItRequestRemovePiece& Request) const;

    // Requires both positions occupied. Deliberately does not re-run
    // scoring/win-condition checks -- IConnectIt_ScoringRule::ApplyScoring
    // is defined around a single just-completed position; a swap changes
    // two positions at once and there's no obvious single-position call
    // that means the right thing here. Left as a known scope gap rather
    // than a guessed-at implementation -- see class/action comments.
    bool HandleSwapPiecesRequest(const FConnectItRequestSwapPieces& Request) const;

    bool HandleToggleTileActiveRequest(const FConnectItRequestToggleTileActive& Request) const;

    // Unlike HandleSwapPiecesRequest, scoring IS re-run here -- exactly one
    // position changes ownership, the same well-defined case
    // HandlePlacePieceRequest already handles.
    bool HandleCapturePieceRequest(const FConnectItRequestCapturePiece& Request, int32 FactionID) const;
};
