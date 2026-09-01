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

// ConnectIt-specific piece spawn/despawn orchestrator -- watches exactly
// the board-event tags that add/remove a piece (PiecePlaced, PieceRemoved,
// PiecesSwapped, PieceCaptured, LineScored), reads
// UConnectIt_BoardStateComponent::GetChangeEvent() to translate each
// firing into spawn/despawn requests, and calls into
// UGridPieceRegistryComponent (which owns pooling/registration and, via
// its own sibling UGridPieceSpawnInterpreter, the actual visuals) to carry
// them out. Composes with those two plugin classes rather than
// subclassing either -- deciding WHEN and WHY a piece spawns is entirely
// project-specific, which is exactly what the plugin classes deliberately
// have no opinion on.
//
// Deliberately does NOT watch ConnectIt_Event_Shift -- a shift moves
// existing piece actors (already UConnectIt_BoardShiftComponent's job), it
// doesn't spawn/despawn them. Deliberately does NOT watch
// ConnectIt_Event_TileActiveToggled either -- HandleToggleTileActiveRequest
// never touches FactionPiece, so toggling a tile's active state has no
// piece-occupancy consequence.
//
// Gates each watched tag's own persistent task on every piece it
// spawned/despawned this firing actually finishing its visual (tracked via
// UGridPieceSpawnInterpreter::OnPieceSpawned/OnPieceDespawned) -- so
// anything else waiting on the same tag (e.g. turn-end resolution, via
// UGameEventTaskSubsystem's own queue ordering) doesn't proceed until
// visuals are genuinely done, not merely started.
//
// Previously, a piece whose activation/deactivation visual completed
// synchronously (see AGridPieceBase::OnActivationVisualComplete's own doc
// comment on pieces with no real effect) could have that completion missed
// entirely, because UActorPool triggered activation/deactivation as an
// unavoidable side effect of pool retrieval/release, before this
// interpreter's own listener bind ever ran. Fixed by moving that trigger to
// the caller (UGridPieceRegistryComponent::SpawnPieceAt/DespawnPieceAt now
// call UActorPoolSubsystem::ActivateObject/DeactivateObject explicitly,
// only after UGridPieceSpawnInterpreter has bound its listener) -- see
// "Move pooled-actor activation/deactivation to the caller" in the plan
// history. No longer a limitation to work around here.
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

    // Shared translation + orchestration -- reads GetChangeEvent(), builds
    // spawn/despawn requests for FiredTag, calls into PieceRegistry for
    // each, and tracks which resulting pieces this firing is waiting on
    // via PendingPieces.
    void ProcessTagFired(FGameplayTag FiredTag, UGameEventTask_Async* Task);

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
