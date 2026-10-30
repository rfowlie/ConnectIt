// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GridMechanicsBaseStructs.h"
#include "StructUtils/InstancedStruct.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "ConnectIt_BoardVisualizer.generated.h"

class AConnectIt_PieceBase;
class AConnectIt_TileBase;
class AConnectIt_EventVisual;
class AGridPieceBase;
class AGridTileBase;
class UConnectIt_PieceRegistry;
class UConnectIt_TileRegistry;
class UConnectIt_VisualHandle;
class UGameEventTask_Async;


// Plays every board event on the board's actors, on every machine -- the one
// entry point for artists and designers to add visuals. One per map: the board
// registry subsystem spawns the level config's Visualizer Class (default: this
// class) after the registries. Subclass it in Blueprint per look.
//
// For each kind of board event (every FConnectItBoardEvent struct, found by
// reflection) it registers a gated task on the event queue. When an event
// plays it:
//   1. does the event's **registry bookkeeping** in C++ -- spawning, re-keying,
//      unmapping pieces and tiles -- so the registries are right whatever a
//      Blueprint override does (or doesn't) call;
//   2. calls the event's function (On Piece Placed, On Board Shifted...) with
//      the event's struct and a **handle** (UConnectIt_VisualHandle). Each
//      default calls the actors' own hooks with that handle; override to add
//      visuals (call the parent) or replace them (don't);
//   3. waits until every hold on the handle is released (or the timeout),
//      then finishes the bookkeeping -- releasing pieces that left, destroying
//      tiles that left, snapping moved actors onto their tiles -- reconciles
//      the actors the event named, and lets the queue move on.
// Once a board change has played out it reconciles every actor. Reconcile:
// the registries are the truth -- an actor off its cell, a piece whose
// position disagrees with the registry, or a board the registry doesn't
// match is an error, and (ConnectIt.Visualizer.Reconcile 2, the default) is
// put right.
// An event with no function of its own goes to On Board Event. An event kind
// with an event visual (the level config's Event Visuals) plays that instead
// of its function -- AConnectIt_EventVisual.
//
// Being an actor, a Blueprint subclass can spawn effects (Niagara, sounds,
// meshes), use timelines and delays; it goes away with the map.
UCLASS(Blueprintable)
class CONNECTIT_API AConnectIt_BoardVisualizer : public AActor
{
    GENERATED_BODY()

public:

    AConnectIt_BoardVisualizer();

    // How long a playing event waits for its holds before it is completed
    // anyway (with a log naming what never released). 0 = no limit.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visualizer", meta = (ClampMin = 0))
    float HoldTimeoutSeconds = 10.f;

    // Where a piece rests, from its tile actor's location
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visualizer")
    FVector PieceOffset = FVector::ZeroVector;

    // How far (world units) an actor may be from where it belongs before
    // reconcile reports it
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visualizer", meta = (ClampMin = 0))
    float ReconcileTolerance = 1.f;

    // The map's visualizer (null if the map has no board)
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visualizer", meta = (WorldContext = "WorldContextObject"))
    static AConnectIt_BoardVisualizer* GetBoardVisualizer(const UObject* WorldContextObject);

    // Registers a task for every board event tag on the world's event queue.
    // Called once by the board registry subsystem after spawning it.
    void Initialise();

    // Every concrete board event struct (reflection, cached): what is bound
    static const TArray<const UScriptStruct*>& GetAllEventStructs();

    // Lets go of every hold Holder has on the playing event (an actor leaving
    // play)
    void ReleaseHoldsBy(const UObject* Holder);

    // --- Reconcile (the registries are the truth) ---

    // Checks the tiles then the pieces at Positions against the registries:
    // a tile on its cell; a piece's position equal to the registry's, with a
    // tile under it, resting on it. Each mismatch is an error (actor, where
    // it is, where it belongs, and the last event's handle if it timed out)
    // and, with ConnectIt.Visualizer.Reconcile 2 (the default), is put right;
    // 1 only logs, 0 skips. Returns the mismatches found. Runs by itself after
    // each event; call it after moving actors outside an event.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Visualizer")
    int32 ReconcileActors(const TArray<FGridPosition>& Positions);

    // --- Event visuals ---

    // Event visual classes by the event tag each declares. Two for one tag:
    // an error, the first kept; an empty, abstract or tagless entry: a
    // warning, skipped.
    static TMap<FGameplayTag, TSubclassOf<AConnectIt_EventVisual>> BuildEventVisualMap(
        const TArray<TSubclassOf<AConnectIt_EventVisual>>& Classes,
        TArray<FString>* OutErrors = nullptr, TArray<FString>* OutWarnings = nullptr);

    // The event visuals to use (Initialise passes the level config's);
    // problems are logged as warnings
    void SetEventVisuals(const TArray<TSubclassOf<AConnectIt_EventVisual>>& Classes);

    // An event visual's Play Default: this visualizer's own function for
    // Payload, with Handle
    void PlayDefaultVisuals(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle);

    // Every tile and piece, then the board against the piece registry (a
    // piece where the board has none, or the reverse -- logged, not fixed
    // here). Runs by itself once a board change has played out.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Visualizer")
    int32 ReconcileAll();

    // --- For event functions ---

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visualizer")
    UConnectIt_TileRegistry* GetTileRegistry() const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visualizer")
    UConnectIt_PieceRegistry* GetPieceRegistry() const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visualizer")
    AConnectIt_TileBase* GetTileAt(FGridPosition Position) const;

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visualizer")
    AConnectIt_PieceBase* GetPieceAt(FGridPosition Position) const;

    // Where a piece at Position rests: its tile's location (or the grid's)
    // plus PieceOffset
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visualizer")
    FVector GetPieceLocation(FGridPosition Position) const;

    // The pieces the playing event brought in (already mapped and placed)
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visualizer")
    TArray<AConnectIt_PieceBase*> GetArrivingPieces() const;

    // The pieces the playing event takes away: no longer in the registry,
    // released to the pool once the event's visuals are done
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visualizer")
    TArray<AConnectIt_PieceBase*> GetLeavingPieces() const;

    // The tiles the playing event takes away (Tiles Removed): no longer in
    // the registry, destroyed once the event's visuals are done
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visualizer")
    TArray<AConnectIt_TileBase*> GetLeavingTiles() const;

    // The playing event's index in its change's list (-1 if none)
    UFUNCTION(BlueprintPure, Category = "ConnectIt|Visualizer")
    int32 GetActiveBoardEventIndex() const;

    // The event that set the playing one off (e.g. a Tile Power Triggered);
    // false if it has none (the action itself)
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Visualizer", meta = (ExpandBoolAsExecs = "ReturnValue"))
    bool GetBoardEventCause(FInstancedStruct& OutCause) const;

protected:

    // --- One function per board event (each default calls the actors' hooks) ---

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Board")
    void OnBoardSeeded(const FConnectItBoardEvent_BoardSeeded& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Pieces")
    void OnPiecePlaced(const FConnectItBoardEvent_PiecePlaced& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Pieces")
    void OnPiecesSwapped(const FConnectItBoardEvent_PiecesSwapped& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Pieces")
    void OnPieceCaptured(const FConnectItBoardEvent_PieceCaptured& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Pieces")
    void OnPieceRemoved(const FConnectItBoardEvent_PieceRemoved& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Pieces")
    void OnPieceMoved(const FConnectItBoardEvent_PieceMoved& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Board")
    void OnBoardShifted(const FConnectItBoardEvent_BoardShifted& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Scoring")
    void OnScored(const FConnectItBoardEvent_Scored& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Tiles")
    void OnMultiplierChanged(const FConnectItBoardEvent_MultiplierChanged& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Tiles")
    void OnTileMultiplierDestroyed(const FConnectItBoardEvent_TileMultiplierDestroyed& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Tiles")
    void OnTileLockChanged(const FConnectItBoardEvent_TileLockChanged& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Tiles")
    void OnTileActiveToggled(const FConnectItBoardEvent_TileActiveToggled& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Tiles")
    void OnChainPartnerChanged(const FConnectItBoardEvent_ChainPartnerChanged& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Tiles")
    void OnTilePowerTriggered(const FConnectItBoardEvent_TilePowerTriggered& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Layout")
    void OnTilesAdded(const FConnectItBoardEvent_TilesAdded& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Layout")
    void OnTilesRemoved(const FConnectItBoardEvent_TilesRemoved& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Layout")
    void OnLayoutChanged(const FConnectItBoardEvent_LayoutChanged& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Match")
    void OnPhaseChanged(const FConnectItBoardEvent_PhaseChanged& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Match")
    void OnGoalsChanged(const FConnectItBoardEvent_GoalsChanged& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Match")
    void OnWinCheckpointReached(const FConnectItBoardEvent_WinCheckpointReached& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Match")
    void OnLoseConditionMet(const FConnectItBoardEvent_LoseConditionMet& Event, UConnectIt_VisualHandle* Handle);

    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer|Match")
    void OnGameWon(const FConnectItBoardEvent_GameWon& Event, UConnectIt_VisualHandle* Handle);

    // Any board event with no function of its own (a new kind not wired yet):
    // its payload is the event struct. Default: nothing.
    UFUNCTION(BlueprintNativeEvent, Category = "ConnectIt|Visualizer")
    void OnBoardEvent(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle);

private:

    // A task of ours is playing: the board event is the queue's active payload
    UFUNCTION()
    void HandleTaskExecute();

    // The event's function (or On Board Event)
    void Dispatch(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle);

    // Step 1: the registries follow the event, before any visual
    void BeginBookkeeping(const FInstancedStruct& Payload);

    // Step 3, once the handle completes
    void FinishBookkeeping();
    void HandleVisualsComplete(UConnectIt_VisualHandle* Handle);
    void CompleteActiveTask();

    // A piece for Faction at Position: taken from the pool, mapped, placed
    // and activated (an existing one there is reused). Null without a piece
    // class.
    AConnectIt_PieceBase* SpawnPieceAt(const FGridPosition& Position, int32 Faction);

    // Unmaps the piece at Position; it is released when the event's visuals
    // are done
    void TakeLeavingPiece(const FGridPosition& Position);

    // On Placed for each arriving piece, with a Piece Placed made for Source
    void PlaceArrivingPieces(const FConnectItBoardEvent& Source, UConnectIt_VisualHandle* Handle);

    // Plays Payload's event visual if it has one (spawned on first use)
    bool PlayEventVisual(const FInstancedStruct& Payload, UConnectIt_VisualHandle* Handle);

    TMap<FGameplayTag, TSubclassOf<AConnectIt_EventVisual>> EventVisualClasses;

    UPROPERTY()
    TMap<FGameplayTag, TObjectPtr<AConnectIt_EventVisual>> EventVisuals;

    // A board change has played out: reconcile everything
    UFUNCTION()
    void HandleBoardEventsPlayed();

    // Listens for that, once the board state exists
    void BindToBoardState();
    bool bBoundToBoardState = false;

    void ReportMismatch(const AActor* Actor, const FGridPosition& Position, const FString& What, bool bFixed) const;

    // Every grid position in an event's data (fields, arrays, nested structs)
    static void CollectPositions(const UStruct* Struct, const void* Data, TArray<FGridPosition>& OutPositions);

    // The playing event (reconciled once its visuals are done)
    FInstancedStruct ActivePayload;

    // The last event and how its handle ended, for reconcile errors
    FString LastEventNote;

    // The board as the player sees it (before the playing event)
    const FConnectItBoardState* GetDisplayBoard() const;

    // One persistent task per board event tag
    UPROPERTY()
    TMap<FGameplayTag, TObjectPtr<UGameEventTask_Async>> Tasks;

    UPROPERTY()
    TObjectPtr<UGameEventTask_Async> ActiveTask;

    UPROPERTY()
    TObjectPtr<UConnectIt_VisualHandle> ActiveHandle;

    // --- The playing event's bookkeeping ---

    UPROPERTY()
    TArray<TObjectPtr<AConnectIt_PieceBase>> ArrivingPieces;

    UPROPERTY()
    TMap<FGridPosition, TObjectPtr<AGridPieceBase>> LeavingPieces;

    UPROPERTY()
    TArray<TObjectPtr<AGridTileBase>> LeavingTiles;

    // Tiles to set onto their grid cell, then pieces onto their tile, once
    // the visuals are done
    TArray<FGridPosition> TilesToSnap;
    TArray<FGridPosition> PiecesToSnap;

    // True while HandleTaskExecute runs: the queue is walking its task list,
    // so a completion then waits a tick
    bool bExecuting = false;
};
