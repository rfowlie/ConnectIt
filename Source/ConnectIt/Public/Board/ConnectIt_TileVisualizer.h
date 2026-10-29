// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Events/ConnectIt_BoardEvents.h"
#include "UObject/Object.h"
#include "ConnectIt_TileVisualizer.generated.h"

class AConnectIt_TileBase;
class UConnectIt_TileReaction;
class UConnectIt_TileRegistry;
class UGameEventTask_Async;
struct FInstancedStruct;


// Plays board events on the tile actors, on every machine: for each kind of
// board event about tiles it registers a gated task on the event queue, and
// when one plays it calls the matching hook (AConnectIt_TileBase) on each
// tile actor it concerns, handing each a fresh reaction
// (UConnectIt_TileReaction). The event waits until every reaction has
// finished; one with no tile actor to call finishes at once. Runs alongside
// the other visuals (e.g. the piece visuals) -- the queue waits for all.
//
// Owned by UConnectIt_BoardRegistrySubsystem, which creates it after the tile
// registry.
UCLASS()
class CONNECTIT_API UConnectIt_TileVisualizer : public UObject
{
    GENERATED_BODY()

public:

    // Registers its tasks on the world's event queue; tile actors are found
    // through InTileRegistry
    void Initialise(UConnectIt_TileRegistry* InTileRegistry);

    // The board event tags it reacts to
    static TArray<FGameplayTag> GetHandledEventTags();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category= "ConnectIt | Visualizer")
    void HandlePlacePiece(const FConnectItBoardEvent_PiecePlaced& Payload);

private:

    // A task of ours is playing: the board event is the queue's active payload
    UFUNCTION()
    void HandleTaskExecute();

    // Calls the hooks the event concerns
    void Dispatch(const FInstancedStruct& Payload);

    // A fresh reaction for Tile, counted
    UConnectIt_TileReaction* MakeReaction(AConnectIt_TileBase* Tile);

    AConnectIt_TileBase* TileAt(const struct FGridPosition& Position) const;

    void HandleReactionFinished();
    void CompleteActiveTask();

    UPROPERTY()
    TObjectPtr<UConnectIt_TileRegistry> TileRegistry;

    // One persistent task per handled event tag
    UPROPERTY()
    TMap<FGameplayTag, TObjectPtr<UGameEventTask_Async>> Tasks;

    // The task playing now, and its reactions not yet finished
    UPROPERTY()
    TObjectPtr<UGameEventTask_Async> ActiveTask;

    int32 PendingReactions = 0;

    // True while hooks are being called: a completion then waits a tick (the
    // queue is still walking its task list)
    bool bDispatching = false;
};
