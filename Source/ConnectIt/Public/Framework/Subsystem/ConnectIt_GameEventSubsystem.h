// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "GameEvent/GameEventTaskHandler.h"
#include "ConnectIt_GameEventSubsystem.generated.h"

class UGameEventTaskManager;
class UGameEventTask_Async;

// Per-world registry of tag-keyed UGameEventTaskManager instances.
//
// Lets any system register a gated/ordered visual response against a
// gameplay tag (e.g. ConnectIt_Event_PiecePlaced) and trigger that tag's
// sequence, without ever holding a raw UGameEventTaskManager* -- all access
// goes through the pass-through functions below. A manager is created
// lazily on first use per tag and lives for the lifetime of the world.
//
// Single-board-manager assumption: this map is per-world, not per board
// manager. Fine given the existing one-AConnectIt_BoardManager-per-level
// design; would need revisiting if that assumption ever changes.
UCLASS()
class CONNECTIT_API UConnectIt_GameEventSubsystem
    : public UWorldSubsystem, public IGameEventTaskHandler
{
    GENERATED_BODY()

public:

    // Registers a gated task against a tag's sequence at the given phase.
    // See UGameEventTaskManager::RegisterAsyncTask -- refused if that tag's
    // sequence is currently running, or if the task's OnExecuteDelegate
    // isn't bound yet.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|GameEvent")
    void RegisterAsyncTask(FGameplayTag Tag, UGameEventTask_Async* Task, int32 Phase = 0);

    // Runs the tag's sequence -- executes phase 0, waits for every task in
    // it to signal completion, advances to the next phase, and so on until
    // OnManagerComplete fires for that tag.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|GameEvent")
    void TriggerTag(FGameplayTag Tag);

    // Bind/unbind to a tag's sequence starting. Listener/FunctionName is
    // the same (Object, UFUNCTION name) pair Blueprint's "Bind Event to X"
    // nodes use -- pass a zero-param UFUNCTION().
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|GameEvent")
    void BindOnTagBegin(FGameplayTag Tag, UObject* Listener, FName FunctionName);

    UFUNCTION(BlueprintCallable, Category = "ConnectIt|GameEvent")
    void UnbindOnTagBegin(FGameplayTag Tag, UObject* Listener, FName FunctionName);

    // Bind/unbind to a tag's sequence fully completing (every phase done).
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|GameEvent")
    void BindOnTagComplete(FGameplayTag Tag, UObject* Listener, FName FunctionName);

    UFUNCTION(BlueprintCallable, Category = "ConnectIt|GameEvent")
    void UnbindOnTagComplete(FGameplayTag Tag, UObject* Listener, FName FunctionName);

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
};
