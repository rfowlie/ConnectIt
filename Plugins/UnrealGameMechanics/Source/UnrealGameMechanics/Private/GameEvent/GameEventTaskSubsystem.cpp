// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEvent/GameEventTaskSubsystem.h"
#include "GameEvent/GameEventTaskManager.h"


// --- Queued Tag Containers ---

void UGameEventTaskSubsystem::QueueTagContainer(const FGameplayTagContainer& Tags)
{
    if (Tags.IsEmpty()) return;

    ContainerQueue.Add(Tags);
    TryExecuteNextContainer();
}

void UGameEventTaskSubsystem::RegisterAsyncTask(
    const FGameplayTag Tag, UGameEventTask_Async* Task, const int32 Phase)
{
    GetOrCreateManager(Tag)->RegisterAsyncTask(Task, Phase);
}

void UGameEventTaskSubsystem::BindOnTagBegin(
    const FGameplayTag Tag, UObject* Listener, const FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerBegin.Add(Delegate);
}

void UGameEventTaskSubsystem::UnbindOnTagBegin(
    const FGameplayTag Tag, UObject* Listener, const FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerBegin.Remove(Delegate);
}

void UGameEventTaskSubsystem::BindOnTagComplete(
    const FGameplayTag Tag, UObject* Listener, const FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerComplete.Add(Delegate);
}

void UGameEventTaskSubsystem::UnbindOnTagComplete(
    const FGameplayTag Tag, UObject* Listener, const FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerComplete.Remove(Delegate);
}

TArray<FGameplayTag> UGameEventTaskSubsystem::GetTagsInQueue()
{
    TArray<FGameplayTag> Out;
    for (const auto Tag : ActiveManagerTags)
    {
        Out.Add(Tag);
    }

    return Out;
}

UGameEventTaskManager* UGameEventTaskSubsystem::GetOrCreateManager(const FGameplayTag Tag)
{
    if (TObjectPtr<UGameEventTaskManager>* Found = ManagersByTag.Find(Tag))
    {
        return *Found;
    }

    // NewObject(this) rather than the plugin's static Create() helper --
    // Create() defaults to the transient package with no owner. Outering
    // to this subsystem gives a correct ownership chain; GC safety comes
    // from ManagersByTag being a UPROPERTY, reachable for the world's life.
    UGameEventTaskManager* NewManager = NewObject<UGameEventTaskManager>(this);
    NewManager->EventTag = Tag;
    ManagersByTag.Add(Tag, NewManager);
    return NewManager;
}

void UGameEventTaskSubsystem::TriggerTag(const FGameplayTag Tag)
{
    GetOrCreateManager(Tag)->InitiateAllTasks();
}

void UGameEventTaskSubsystem::TryExecuteNextContainer()
{
    if (!ActiveManagerTags.IsEmpty()) return; // something already firing
    if (ContainerQueue.IsEmpty()) return;

    ActiveContainer = ContainerQueue[0];
    ContainerQueue.RemoveAt(0);

    // Populate the full set BEFORE triggering anything -- a tag that
    // completes synchronously (zero registered tasks) mid-loop removes
    // itself from a set that still contains every other tag in this
    // container, so it can't look "empty" prematurely and re-enter while
    // still iterating the rest of the container. Membership makes this
    // guarantee for free, no arithmetic bias needed.
    for (const FGameplayTag& Tag : ActiveContainer)
    {
        ActiveManagerTags.Add(Tag);
    }

    // notify interested parties (e.g. debug widgets) that a new container
    // just became active -- without this, listeners only ever see tags
    // disappearing (HandleOnManagerComplete's own broadcast below), never
    // the moment they actually started.
    if (OnActiveManagerTagsChanged.IsBound()) { OnActiveManagerTagsChanged.Broadcast(); }

    for (const FGameplayTag& Tag : ActiveContainer)
    {
        // Bound directly (AddUniqueDynamic/RemoveDynamic, compile-time
        // signature-checked) rather than through the reflection-based
        // BindOnTagComplete/UnbindOnTagComplete pass-throughs -- those exist
        // for external (UObject*, FName) callers; the subsystem is binding
        // to its own known member function here.
        GetOrCreateManager(Tag)->OnManagerComplete.AddUniqueDynamic(
            this, &UGameEventTaskSubsystem::HandleOnManagerComplete);

        TriggerTag(Tag);
    }
}

void UGameEventTaskSubsystem::HandleOnManagerComplete(const FGameplayTag Tag)
{
    if (!Tag.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT(
            "UGameEventTaskSubsystem::HandleOnManagerComplete -- received an invalid tag"));
        return;
    }

    // TODO: this feels clunky. This should be happening only on create/destroy of EventManager
    GetOrCreateManager(Tag)->OnManagerComplete.RemoveDynamic(
        this, &UGameEventTaskSubsystem::HandleOnManagerComplete);

    if (!ActiveManagerTags.Remove(Tag))
    {
        UE_LOG(LogTemp, Error, TEXT(
            "UGameEventTaskSubsystem::HandleOnManagerComplete -- '%s' completed "
            "but wasn't tracked as active -- ignoring"), *Tag.ToString());
        return;
    }

    // notify interested parties of event list change
    if (OnActiveManagerTagsChanged.IsBound()) { OnActiveManagerTagsChanged.Broadcast(); }

    // only try next if all ActiveManagerTags are complete
    if (ActiveManagerTags.IsEmpty())
    {
        TryExecuteNextContainer();
    }
}