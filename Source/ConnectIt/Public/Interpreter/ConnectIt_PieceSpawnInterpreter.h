// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GridMechanicsBaseStructs.h"
#include "Components/ActorComponent.h"
#include "ConnectIt_PieceSpawnInterpreter.generated.h"

class AGridPieceBase;
class AGridTileBase;
class UGridPieceRegistryComponent;
class UGridPieceSpawnInterpreter;
class UGameEventTask_Async;
class UTurnBasedGameEvent;
class UConnectIt_PlacePieceGameEvent;
class UConnectIt_LineScoreGameEvent;

USTRUCT(BlueprintType)
struct FPendingSpawn
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGridPosition Position;

    UPROPERTY(BlueprintReadWrite)
    int32 FactionSlot = -1;
};

// ConnectIt-specific piece spawn/despawn orchestrator -- watches exactly
// the board-event tags that add/remove a piece (PiecePlaced, PieceRemoved,
// PiecesSwapped, PieceCaptured, LineScored) and reads
// UConnectIt_BoardStateComponent::GetChangeEvent() to figure out what each
// firing means.
//
// PiecePlaced and LineScored are dispatched to UTurnBasedGameEvent
// subclasses (UConnectIt_PlacePieceGameEvent/UConnectIt_LineScoreGameEvent)
// -- each owns its own full sequence (retrieve/initialize/position/wait)
// as its own object, reporting completion back via OnComplete. The other
// three tags (PieceRemoved, PiecesSwapped, PieceCaptured) still run
// procedurally through HandleSpawn/HandleDespawn below, calling directly
// into UGridPieceRegistryComponent/UGridPieceSpawnInterpreter -- not yet
// converted to the game-event pattern (deliberately incremental scope,
// see "UTurnBasedGameEvent: encapsulate board-reaction sequences as
// objects" in the plan history). Notably, the still-procedural despawn
// path has a known, not-yet-fixed gap: it finalizes a piece's removal
// (unregisters it) immediately after triggering its despawn visual,
// rather than waiting for that visual to actually finish -- fixed for
// LineScored via UConnectIt_LineScoreGameEvent, not yet for the other two.
//
// Deliberately does NOT watch ConnectIt_Event_Shift -- a shift moves
// existing piece actors (already UConnectIt_BoardShiftComponent's job), it
// doesn't spawn/despawn them. Deliberately does NOT watch
// ConnectIt_Event_TileActiveToggled either -- HandleToggleTileActiveRequest
// never touches FactionPiece, so toggling a tile's active state has no
// piece-occupancy consequence.
//
// Gates each watched tag's own persistent task on every piece it
// spawned/despawned this firing actually finishing its visual -- so
// anything else waiting on the same tag (e.g. turn-end resolution, via
// UGameEventTaskSubsystem's own queue ordering) doesn't proceed until
// visuals are genuinely done, not merely started.
UCLASS(Blueprintable, ClassGroup=(ConnectIt), meta=(BlueprintSpawnableComponent))
class CONNECTIT_API UConnectIt_PieceSpawnInterpreter : public UActorComponent
{
    GENERATED_BODY()

public:

    UConnectIt_PieceSpawnInterpreter();

    // FactionPieceClasses[FactionSlot] is the concrete piece class to spawn
    // for that faction -- same "array indexed by faction slot" convention
    // FConnectItBoardState::ScoreBoard already uses.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectIt|Piece")
    TArray<TSubclassOf<AGridPieceBase>> FactionPieceClasses;

protected:

    virtual void BeginPlay() override;

private:

    UPROPERTY()
    TObjectPtr<UGridPieceRegistryComponent> PieceRegistry = nullptr;

    UPROPERTY()
    TObjectPtr<UGridPieceSpawnInterpreter> SpawnInterpreterRef = nullptr;

    // One persistent task per watched tag -- each bound to its own handler
    // below, not a shared one, so there's no need to disambiguate which
    // tag fired from inside a single callback.
    UPROPERTY() TObjectPtr<UGameEventTask_Async> PiecePlacedTask = nullptr;
    UPROPERTY() TObjectPtr<UGameEventTask_Async> PieceRemovedTask = nullptr;
    UPROPERTY() TObjectPtr<UGameEventTask_Async> PiecesSwappedTask = nullptr;
    UPROPERTY() TObjectPtr<UGameEventTask_Async> PieceCapturedTask = nullptr;
    UPROPERTY() TObjectPtr<UGameEventTask_Async> LineScoredTask = nullptr;

    UFUNCTION() void HandlePiecePlacedExecute();
    UFUNCTION() void HandlePieceRemovedExecute();
    UFUNCTION() void HandlePiecesSwappedExecute();
    UFUNCTION() void HandlePieceCapturedExecute();
    UFUNCTION() void HandleLineScoredExecute();

    // In-flight game events for the two converted tags -- kept referenced
    // here (GC-safety) while running, parallel to how ActiveTask tracks an
    // in-flight tag task. Two separate slots, not one shared one: distinct
    // tags fired together in the same QueueTagContainer run in parallel,
    // so PiecePlaced and LineScored can legitimately both be in flight at
    // the same time.
    UPROPERTY() TObjectPtr<UConnectIt_PlacePieceGameEvent> ActivePlacePieceEvent = nullptr;
    UPROPERTY() TObjectPtr<UConnectIt_LineScoreGameEvent> ActiveLineScoreEvent = nullptr;

    UFUNCTION() void HandlePlacePieceEventComplete(UTurnBasedGameEvent* Event);
    UFUNCTION() void HandleLineScoreEventComplete(UTurnBasedGameEvent* Event);

    void HandleSpawn(TArray<FPendingSpawn> Spawns);
    void HandleDespawn(TArray<FGridPosition> Despawns);

    // Called at the end of every HandleXExecute -- if that firing didn't
    // end up with anything for PendingPieces to wait on (BoardStateComponent
    // was missing, or HandleSpawn/HandleDespawn skipped every item), completes
    // ActiveTask immediately instead of leaving it to stall the tag's
    // UGameEventTaskManager -- and therefore the whole gated queue behind
    // it -- forever.
    void CompleteTaskIfNothingPending();

    AGridTileBase* ResolveTile(FGridPosition Position) const;
    TSubclassOf<AGridPieceBase> GetPieceClassForFaction(int32 FactionSlot) const;

    UFUNCTION() void HandlePieceSpawned(AGridPieceBase* Piece);
    UFUNCTION() void HandlePieceDespawned(AGridPieceBase* Piece);
    void HandlePieceVisualComplete(AGridPieceBase* Piece);

    // The tag firing currently being resolved, and which of its resulting
    // pieces are still mid-visual. At most one firing at a time in
    // practice -- UGameEventTaskSubsystem's own queue serializes tag
    // firings -- but tracked precisely rather than assumed.
    UPROPERTY()
    TObjectPtr<UGameEventTask_Async> ActiveTask = nullptr;

    TArray<TWeakObjectPtr<AGridPieceBase>> PendingPieces;
};
