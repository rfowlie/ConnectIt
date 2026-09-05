// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_BoardStateComponent.h"
#include "ConnectIt_Structs.h"
#include "TurnBasedMechanicsStructs.h"
#include "Board/BoardManagerBase.h"
#include "ConnectIt_BoardManager.generated.h"

class UConnectIt_PlacePieceGameEvent;
class UTurnBasedGameEvent;
class UActionLoadoutDataAsset;
class UConnectIt_ConfigComponent;
class UConnectIt_BoardRulesComponent;
class UConnectIt_BoardShiftComponent;

// NOTE: OnPiecePlaced/OnLineScored/OnPlayerWin/OnShiftApplied used to live
// here as their own delegates. Removed -- listeners now bind to
// UGameEventTaskSubsystem's ConnectIt_Event_PiecePlaced/LineScored/PlayerWin/
// Shift tags instead (see Workflows/GameEventSubsystem_Workflow.txt). This
// board manager triggers which tag fires when; it doesn't also maintain a
// parallel notification API for the same events.

UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_BoardManager : public ABoardManagerBase
{

    GENERATED_BODY()

public:

    AConnectIt_BoardManager();

    // --- Component Accessors ---
    // Used to live as IGridBoardManagerInterface overrides -- removed, it
    // was never used generically anywhere in the plugin (AConnectIt_BoardManager
    // was its sole implementer project-wide) and its interface-mandated
    // GetBoardState() return type (UBoardStateComponentBase*) forced an
    // awkward Cast<UConnectIt_BoardStateComponent>(...) at every call site.
    // Plain accessors below return the real derived type directly.
    //
    // GetTileRegistry()/GetPieceRegistry() aren't redeclared here -- they're
    // already inherited from ABoardManagerBase (BlueprintPure), returning
    // UGridTileRegistryBase*/UGridPieceRegistryBase*. See RuntimeStateAccess.md's
    // Board State table for the null-unless-assigned caveat on those.

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_BoardStateComponent* GetBoardStateComponent() const { return BoardStateComponent; }
    
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_ConfigComponent* GetConfigComponent() const { return ConnectItConfigComponent; }

    // Owns the pluggable scoring/win-condition strategies -- see
    // UConnectIt_BoardRulesComponent. Replaces the WinScoreThreshold/
    // ConnectLength fields and hardcoded scoring/win methods that used to
    // live directly on this class.
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UConnectIt_BoardRulesComponent* GetBoardRulesComponent() const { return BoardRulesComponent; }

    // --- Board Lifecycle ---

    // Called by GameMode after all tiles have registered
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    void InitialiseBoard(int32 NumFactions);

    // Entry point for all board change requests
    // Called directly by player controller ServerRPC and AI controller.
    // Dispatches by RequestType, unwrapping Request.Payload into whichever
    // concrete struct that type expects (see FTurnActionRequest) and
    // routing to the matching private HandleXRequest below. Returns
    // whether the request succeeded -- the caller (AConnectIt_PlayerController)
    // reports this back to the requesting client via ClientNotifyBoardChangeOutcome
    // so UTurnBasedActionsComponent can resolve its awaiting-confirmation state.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    bool ProcessRequest(const FTurnActionRequest& Request);

protected:
    
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleActiveControllerChanged(AController* NewActiveController);
    void BindParticipantManager();

    // --- Components ---

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Components")
    TObjectPtr<UConnectIt_BoardStateComponent> BoardStateComponent = nullptr;
    
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Components")
    TObjectPtr<UConnectIt_ConfigComponent> ConnectItConfigComponent = nullptr;
    
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt|Components")
    TObjectPtr<UConnectIt_BoardRulesComponent> BoardRulesComponent = nullptr;
    

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

    // --- Request Handlers ---
    // FactionID is passed separately rather than living on each payload
    // struct -- it's the one piece of data every request type needs, so it
    // stays on FTurnActionRequest's envelope instead of being duplicated
    // into FConnectItRequestPlacePiece/FConnectItRequestBoardShift.
    bool HandlePlacePieceRequest(const FConnectItRequestPlacePiece& Request, int32 FactionID) const;
    bool HandleShiftRequest(const FConnectItRequestBoardShift& Request, int32 FactionID) const;

    // Same as HandlePlacePieceRequest but skips IsTileValidForPlacement --
    // only requires the position to exist in the registry, so it can place
    // on an inactive or already-occupied tile (overwriting it). Produces the
    // same ChangeEvent shape (bPiecePlaced) since visually it's the same
    // kind of event as a normal placement.
    bool HandleForcePlacePieceRequest(const FConnectItRequestForcePlacePiece& Request, int32 FactionID) const;

    bool HandleDestroyTileMultiplierRequest(const FConnectItRequestDestroyTileMultiplier& Request) const;

    // DelayTurns > 0 is rejected (logged) rather than silently treated as
    // immediate -- delayed/scheduled removal needs a per-turn ticking
    // mechanism this board manager doesn't have yet. See
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

    // Kept only for OnActiveControllerChanged. Used to also register a
    // persistent turn-end task here (RegisterTurnEndTask/HandleTurnEndTaskExecute/
    // HandleBoardSequenceIdleForTurnEnd) that polled BoardSequencerComponent
    // ::IsIdle() before completing -- removed along with BoardSequencerComponent
    // itself: UGameEventTaskSubsystem's own queue now guarantees
    // ConnectIt_Event_TurnEnd (triggered by UTurnBasedParticipantManagerComponent
    // ::EndTurn) can't start firing until every board-event tag enqueued
    // ahead of it (by ConnectIt_BoardStateComponent) has finished, so a task
    // that only ever polled for that same fact and then immediately
    // completed had become redundant.
    UPROPERTY()
    TObjectPtr<class UTurnBasedParticipantManagerComponent> ParticipantManagerRef = nullptr;
};