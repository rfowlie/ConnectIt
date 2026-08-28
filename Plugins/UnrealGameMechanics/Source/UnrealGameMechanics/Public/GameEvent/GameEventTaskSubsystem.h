// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "GameEvent/GameEventTaskHandler.h"
#include "GameEventTaskSubsystem.generated.h"

class UGameEventTaskManager;
class UGameEventTask_Async;

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnTagSequenceStepComplete, FGameplayTagContainer, CompletedStepTags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActiveManagerTagsChanged);


// Per-world registry of tag-keyed UGameEventTaskManager instances.
//
// Lets any system register a gated/ordered visual response against a
// gameplay tag and trigger that tag's sequence, without ever holding a raw
// UGameEventTaskManager* -- all access goes through the pass-through
// functions below. A manager is created lazily on first use per tag and
// lives for the lifetime of the world.
//
// Generic, project-agnostic infrastructure -- has no dependency on any
// specific game's types. Project code (e.g. ConnectIt's
// AConnectIt_BoardManager) is a caller of this subsystem, not an owner of
// its managers, so visual/UI systems always have a stable place to bind
// against regardless of whether any particular gameplay actor exists yet.
UCLASS()
class UNREALGAMEMECHANICS_API UGameEventTaskSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    // Enqueues Tags to fire together once it's their turn in the queue, then
    // tries to start executing immediately if nothing else is currently
    // running. Every tag in the container triggers in parallel; the queue
    // does not advance to the next queued container until every tag in this
    // one has fully completed (OnManagerComplete for all of them). This is
    // the primary way to fire a tag from outside the subsystem -- TriggerTag
    // itself is private, reachable only internally.
    UFUNCTION(BlueprintCallable, Category = "GameEvent")
    void QueueTagContainer(const FGameplayTagContainer& Tags);
    
    // Registers a gated task against a tag's sequence at the given phase.
    // See UGameEventTaskManager::RegisterAsyncTask -- refused if that tag's
    // sequence is currently running, or if the task's OnExecuteDelegate
    // isn't bound yet.
    UFUNCTION(BlueprintCallable, Category = "GameEvent")
    void RegisterAsyncTask(FGameplayTag Tag, UGameEventTask_Async* Task, int32 Phase = 0);

    // Bind/unbind to a tag's sequence starting. Listener/FunctionName is
    // the same (Object, UFUNCTION name) pair Blueprint's "Bind Event to X"
    // nodes use -- pass a zero-param UFUNCTION().
    UFUNCTION(BlueprintCallable, Category = "GameEvent")
    void BindOnTagBegin(FGameplayTag Tag, UObject* Listener, FName FunctionName);

    UFUNCTION(BlueprintCallable, Category = "GameEvent")
    void UnbindOnTagBegin(FGameplayTag Tag, UObject* Listener, FName FunctionName);

    // Bind/unbind to a tag's sequence fully completing (every phase done).
    UFUNCTION(BlueprintCallable, Category = "GameEvent")
    void BindOnTagComplete(FGameplayTag Tag, UObject* Listener, FName FunctionName);

    UFUNCTION(BlueprintCallable, Category = "GameEvent")
    void UnbindOnTagComplete(FGameplayTag Tag, UObject* Listener, FName FunctionName);

    // --- Helpers ---

    UPROPERTY(BlueprintAssignable, Category = "GameEvent")
    FOnActiveManagerTagsChanged OnActiveManagerTagsChanged;
    
    // return tag array at time of calling
    UFUNCTION(BlueprintCallable, Category = "GameEvent")
    TArray<FGameplayTag> GetTagsInQueue();

private:

    UPROPERTY()
    TMap<FGameplayTag, TObjectPtr<UGameEventTaskManager>> ManagersByTag;

    UGameEventTaskManager* GetOrCreateManager(FGameplayTag Tag);
    
    // Runs the tag's sequence -- executes phase 0, waits for every task in
    // it to signal completion, advances to the next phase, and so on until
    // OnManagerComplete fires for that tag. Private -- external callers go
    // through QueueTagContainer (or the deprecated QueueTagSequence) so
    // every firing is properly queued; this subsystem's own internals
    // (below, and QueueTagSequence's) call it directly.
    void TriggerTag(FGameplayTag Tag);

    // --- Queued Tag Containers ---
    // Deliberately separate state from the deprecated sequence queue above
    // -- QueueTagSequence's own bind/unbind of HandleSequenceStepTagComplete
    // operates on the very same per-tag managers this queue's
    // HandleOnManagerComplete binds to (via AddUniqueDynamic/RemoveDynamic
    // instead of the reflection-based BindOnTagComplete), so keeping the two
    // mechanisms' bookkeeping fully independent is what keeps them from
    // interfering with each other.

    UPROPERTY()
    TArray<FGameplayTagContainer> ContainerQueue;

    FGameplayTagContainer ActiveContainer;

    // The set of tags whose managers are still outstanding for
    // ActiveContainer -- populated in full before any of them are
    // triggered, and drained one at a time as each completes. Precise,
    // can inspect state (what's actually still running) rather than an
    // opaque pending count.
    UPROPERTY()
    TArray<FGameplayTag> ActiveManagerTags;

    void TryExecuteNextContainer();

    UFUNCTION()
    void HandleOnManagerComplete(FGameplayTag Tag);
    
};
