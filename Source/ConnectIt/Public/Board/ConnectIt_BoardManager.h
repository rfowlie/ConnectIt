// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConnectIt_BoardStateComponent.h"
#include "ConnectIt_Structs.h"
#include "GridMechanicsBaseStructs.h"
#include "Board/GridBoardManagerInterface.h"
#include "ConnectIt_BoardManager.generated.h"

class UActionLoadoutDataAsset;
class UConnectIt_ConfigComponent;
class UConnectIt_TileStateInterpreter;
class UConnectIt_PieceSpawnInterpreter;
class UConnectIt_ScoreInterpreter;
class UGridTileRegistryComponent;
class UGridPieceRegistryComponent;
class UBoardShiftComponent;

// NOTE: OnPiecePlaced/OnLineScored/OnPlayerWin used to live here as their
// own delegates. Removed -- listeners now bind to UGameEventTaskSubsystem's
// ConnectIt_Event_PiecePlaced/LineScored/PlayerWin tags instead (see
// Workflows/GameEventSubsystem_Workflow.txt). This board manager triggers
// which tag fires when; it doesn't also maintain a parallel notification
// API for the same events.
//
// OnShiftApplied stays as a direct delegate for now -- its tag-based
// equivalent is deferred (see HandleShiftResult).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShiftApplied, const FShiftOperation&, Operation, const FShiftResult&, Result);

UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_BoardManager : public AActor, public IGridBoardManagerInterface
{
    
    GENERATED_BODY()

public:

    AConnectIt_BoardManager();

    // --- ABoardManager Interface Overrides ---

    virtual UGridTileRegistryComponent* GetTileRegistry_Implementation() const override;
    virtual UGridPieceRegistryComponent* GetPieceRegistry_Implementation() const override;
    virtual UBoardStateComponentBase* GetBoardState_Implementation() const override;
    virtual UBoardShiftComponent* GetShiftComponent_Implementation() const override;

    // --- Board Rules ---
    // Previously on UConnectIt_BoardManagerComponent

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules")
    float WinScoreThreshold = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Rules")
    int32 ConnectLength = 4;

    // --- Board Lifecycle ---

    // Called by GameMode after all tiles have registered
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    void InitialiseBoard(int32 NumFactions);

    // Entry point for all board change requests
    // Called directly by player controller ServerRPC and AI controller
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Board")
    void ProcessRequest(const FTurnActionRequest& Request);

    // Called by shift component OnShiftResultReady
    UFUNCTION()
    void HandleShiftResult(
        const FShiftOperation& Operation,
        const FShiftResult& Result);

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Board")
    FOnShiftApplied OnShiftApplied;

    // --- Config Accessors ---

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UActionLoadoutDataAsset* GetPlayerLoadout() const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    UActionLoadoutDataAsset* GetEnemyLoadout() const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Board")
    float GetWinScoreThreshold() const;

protected:
    
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleActiveControllerChanged(AController* NewActiveController);
    void BindParticipantManager();

    // Bound to BoardStateComponent->OnBoardStateChanged in BeginPlay, on
    // both server and client -- that signal already fires symmetrically on
    // both machines (server: ApplyAndBroadcast, client: OnRep_BoardSnapshot),
    // so reading BoardSnapshot.ChangeEvent here is what drives the gated
    // visual sequence (via UGameEventTaskSubsystem tags) identically on
    // every machine, instead of the old pattern of broadcasting delegates
    // directly from server-only request handlers.
    UFUNCTION()
    void HandleBoardStateChanged();
    
    // --- Components ---

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Components")
    TObjectPtr<UGridTileRegistryComponent> TileRegistryComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Components")
    TObjectPtr<UGridPieceRegistryComponent> PieceRegistryComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Components")
    TObjectPtr<UConnectIt_BoardStateComponent> BoardStateComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Components")
    TObjectPtr<UBoardShiftComponent> BoardShiftComponent = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Components")
    TObjectPtr<UConnectIt_ConfigComponent> ConnectItConfig = nullptr;

    // --- Interpreters ---

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Interpreters")
    TObjectPtr<UConnectIt_TileStateInterpreter> TileStateInterpreter = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Interpreters")
    TObjectPtr<UConnectIt_PieceSpawnInterpreter> PieceSpawnInterpreter = nullptr;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
        Category = "ConnectIt|Interpreters")
    TObjectPtr<UConnectIt_ScoreInterpreter> ScoreInterpreter = nullptr;

private:

    void BindInterpreters();
    void BindShiftComponent();

    // --- Board Logic ---
    // Moved from UConnectIt_BoardManagerComponent

    void HandlePlacePieceRequest(const FTurnActionRequest& Request);

    float CheckAndApplyScoring(
        FConnectItBoardState& MutableState,
        FGridPosition Position,
        int32 FactionSlot);

    TArray<TArray<FGridPosition>> FindScoringLines(
        const FConnectItBoardState& State,
        FGridPosition Position,
        int32 FactionSlot) const;

    float ApplyScoringLine(
        FConnectItBoardState& MutableState,
        const TArray<FGridPosition>& Line,
        FGridPosition CompletingPosition,
        int32 FactionSlot) const;

    void CheckWinCondition(FConnectItBoardState& MutableState) const;

    static const TArray<FGridDirectionVector>& GetScoringDirections();

    // --- Gated Visual Sequencing ---
    // Drives ConnectIt_Event_PiecePlaced -> ConnectIt_Event_LineScored ->
    // ConnectIt_Event_PlayerWin in strict order via UGameEventTaskSubsystem's
    // QueueTagSequence, each step gated on the previous fully completing.
    // See HandleBoardStateChanged for the entry point.
    //
    // NOTE (known, deferred limitation): nothing here gates
    // UTurnBasedParticipantManagerComponent's resolution-phase timer on this
    // sequence completing -- that timer is fixed-duration with no external
    // hold/extend mechanism, and TurnBasedPauseAction/NotifyPaused is
    // client-side input-blocking only, with no effect on it. A slow win/
    // score sequence can still be running when the next turn starts. See
    // Workflows/GameEventSubsystem_Workflow.txt for the follow-up plan.

    // Starts the next queued change event's sequence if one isn't already
    // running -- pops PendingChangeEventQueue into ActiveChangeEvent, builds
    // the step list from its flags, and calls QueueTagSequence once. No
    // typed delegates are fired from here or anywhere in this sequencing --
    // listeners bind directly to UGameEventTaskSubsystem (BindOnTagBegin for
    // "tell me now", a registered UGameEventTask_Async for "wait for my
    // visual to finish") and read UConnectIt_BoardStateComponent::
    // GetChangeEvent() for payload data.
    void TryStartNextSequence();

    // Bound as QueueTagSequence's OnComplete -- the whole sequence for
    // ActiveChangeEvent finished, drain the next queued change event
    UFUNCTION()
    void HandleSequenceComplete();

    // Change events awaiting sequencing -- a new event arriving while a
    // sequence is already in flight is queued rather than dropped or
    // interleaved with the one currently running. This is separate from
    // (and in addition to) UGameEventTaskSubsystem's own internal queue --
    // that queue only serializes WHICH sequence runs when; this one is what
    // lets HandleSequenceComplete know a new ChangeEvent is waiting once the
    // current one finishes.
    UPROPERTY()
    TArray<FConnectItBoardChangeEvent> PendingChangeEventQueue;

    // The change event currently being sequenced -- valid only while
    // bSequenceInFlight is true
    UPROPERTY()
    FConnectItBoardChangeEvent ActiveChangeEvent;

    UPROPERTY()
    bool bSequenceInFlight = false;
};