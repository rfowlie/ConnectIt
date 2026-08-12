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
DECLARE_DYNAMIC_DELEGATE(FOnTagSequenceComplete);

// One queued call to QueueTagSequence -- tracked internally by
// UGameEventTaskSubsystem, index 0 in the queue is always the active one
USTRUCT()
struct FGameEventQueuedTagSequence
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FGameplayTagContainer> Steps;

    UPROPERTY()
    FOnTagSequenceStepComplete OnStepComplete;

    UPROPERTY()
    FOnTagSequenceComplete OnComplete;

    int32 CurrentStepIndex = -1;
    int32 PendingInStep = 0;
};

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
class UNREALGAMEMECHANICS_API UGameEventTaskSubsystem
    : public UWorldSubsystem, public IGameEventTaskHandler
{
    GENERATED_BODY()

public:

    // Registers a gated task against a tag's sequence at the given phase.
    // See UGameEventTaskManager::RegisterAsyncTask -- refused if that tag's
    // sequence is currently running, or if the task's OnExecuteDelegate
    // isn't bound yet.
    UFUNCTION(BlueprintCallable, Category = "GameEvent")
    void RegisterAsyncTask(FGameplayTag Tag, UGameEventTask_Async* Task, int32 Phase = 0);

    // Runs the tag's sequence -- executes phase 0, waits for every task in
    // it to signal completion, advances to the next phase, and so on until
    // OnManagerComplete fires for that tag.
    UFUNCTION(BlueprintCallable, Category = "GameEvent")
    void TriggerTag(FGameplayTag Tag);

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

    // Runs an ordered sequence of tag-groups. Each group's tags all trigger
    // in parallel; the sequence advances to the next group only once every
    // tag in the current group has fully completed (OnStepComplete fires
    // once per group, with that group's tags, as it finishes). Serialized
    // globally -- if another sequence is already running, this one queues
    // and starts once every sequence ahead of it in the queue has finished;
    // sequences never run concurrently or interleave.
    //
    // Callers with their own per-call payload data to correlate against a
    // specific run (e.g. AConnectIt_BoardManager's FConnectItBoardChangeEvent)
    // should track that themselves and only queue their next call once
    // OnComplete fires for the previous one -- this queue serializes
    // WHICH sequence runs when, not caller-side state.
    UFUNCTION(BlueprintCallable, Category = "GameEvent")
    void QueueTagSequence(
        const TArray<FGameplayTagContainer>& Steps,
        FOnTagSequenceStepComplete OnStepComplete,
        FOnTagSequenceComplete OnComplete);

    // IGameEventTaskHandler -- gives this previously-unimplemented plugin
    // interface a real implementation. Returns the same lazily-created
    // manager the pass-through functions above operate on; provided for
    // plugin-side code written against the interface rather than this
    // concrete class.
    virtual UGameEventTaskManager* GetGameEventTaskManagerByTag_Implementation(
        FGameplayTag InGameplayTag) override;

private:

    UGameEventTaskManager* GetOrCreateManager(FGameplayTag Tag);

    UPROPERTY()
    TMap<FGameplayTag, TObjectPtr<UGameEventTaskManager>> ManagersByTag;

    // --- Queued Tag Sequences ---

    UPROPERTY()
    TArray<FGameEventQueuedTagSequence> SequenceQueue;

    bool bSequenceInFlight = false;

    void TryStartNextQueuedSequence();
    void AdvanceSequenceToStep(int32 StepIndex);

    UFUNCTION()
    void HandleSequenceStepTagComplete();
};
