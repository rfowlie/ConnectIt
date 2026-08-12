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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPiecePlaced,FGridPosition, Position);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLineScored,int32, FactionSlot, float, PointsScored);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerWin,int32, WinningFactionSlot);
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
    FOnPiecePlaced OnPiecePlaced;

    UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Board")
    FOnLineScored OnLineScored;

    UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Board")
    FOnPlayerWin OnPlayerWin;

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
    // so reading BoardSnapshot.ChangeEvent here is what drives OnPiecePlaced/
    // OnLineScored/OnPlayerWin and the gated visual sequence identically on
    // every machine, instead of the old pattern of broadcasting them
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
    // ConnectIt_Event_PlayerWin in strict order via UConnectIt_GameEventSubsystem,
    // each step gated on the previous fully completing. See
    // HandleBoardStateChanged for the entry point.

    // Binds the three sequence-complete handlers to their tags, once
    void BindGameEventSequencing();

    // Starts the next queued sequence if one isn't already running
    void TryStartNextSequence();

    UFUNCTION()
    void HandlePiecePlacedSequenceComplete();

    UFUNCTION()
    void HandleLineScoredSequenceComplete();

    UFUNCTION()
    void HandlePlayerWinSequenceComplete();

    // Change events awaiting sequencing -- a new event arriving while a
    // sequence is already in flight is queued rather than dropped or
    // interleaved with the one currently running
    UPROPERTY()
    TArray<FConnectItBoardChangeEvent> PendingChangeEventQueue;

    // The change event currently being sequenced -- valid only while
    // bSequenceInFlight is true
    UPROPERTY()
    FConnectItBoardChangeEvent ActiveChangeEvent;

    UPROPERTY()
    bool bSequenceInFlight = false;
};