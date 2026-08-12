// Fill out your copyright notice in the Description page of Project Settings.

#include "GameEvent/GameEventTaskSubsystem.h"

#include "GameEvent/GameEventTaskManager.h"
#include "GameEvent/GameEventTask_Async.h"


void UGameEventTaskSubsystem::RegisterAsyncTask(
    FGameplayTag Tag, UGameEventTask_Async* Task, int32 Phase)
{
    GetOrCreateManager(Tag)->RegisterAsyncTask(Task, Phase);
}

void UGameEventTaskSubsystem::TriggerTag(FGameplayTag Tag)
{
    GetOrCreateManager(Tag)->InitiateAllTasks();
}

void UGameEventTaskSubsystem::BindOnTagBegin(
    FGameplayTag Tag, UObject* Listener, FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerBegin.Add(Delegate);
}

void UGameEventTaskSubsystem::UnbindOnTagBegin(
    FGameplayTag Tag, UObject* Listener, FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerBegin.Remove(Delegate);
}

void UGameEventTaskSubsystem::BindOnTagComplete(
    FGameplayTag Tag, UObject* Listener, FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerComplete.Add(Delegate);
}

void UGameEventTaskSubsystem::UnbindOnTagComplete(
    FGameplayTag Tag, UObject* Listener, FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerComplete.Remove(Delegate);
}

UGameEventTaskManager* UGameEventTaskSubsystem::GetGameEventTaskManagerByTag_Implementation(
    FGameplayTag InGameplayTag)
{
    return GetOrCreateManager(InGameplayTag);
}

UGameEventTaskManager* UGameEventTaskSubsystem::GetOrCreateManager(FGameplayTag Tag)
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
    ManagersByTag.Add(Tag, NewManager);
    return NewManager;
}

// --- Queued Tag Sequences ---

void UGameEventTaskSubsystem::QueueTagSequence(
    const TArray<FGameplayTagContainer>& Steps,
    FOnTagSequenceStepComplete OnStepComplete,
    FOnTagSequenceComplete OnComplete)
{
    if (Steps.IsEmpty())
    {
        OnComplete.ExecuteIfBound();
        return;
    }

    FGameEventQueuedTagSequence NewSequence;
    NewSequence.Steps          = Steps;
    NewSequence.OnStepComplete = OnStepComplete;
    NewSequence.OnComplete     = OnComplete;

    SequenceQueue.Add(NewSequence);
    TryStartNextQueuedSequence();
}

void UGameEventTaskSubsystem::TryStartNextQueuedSequence()
{
    if (bSequenceInFlight) return;
    if (SequenceQueue.IsEmpty()) return;

    bSequenceInFlight = true;
    AdvanceSequenceToStep(0);
}

void UGameEventTaskSubsystem::AdvanceSequenceToStep(int32 StepIndex)
{
    if (SequenceQueue.IsEmpty()) return; // defensive -- should not happen

    FGameEventQueuedTagSequence& Active = SequenceQueue[0];
    Active.CurrentStepIndex = StepIndex;

    if (!Active.Steps.IsValidIndex(StepIndex))
    {
        // Every step in this sequence has completed
        Active.OnComplete.ExecuteIfBound();
        SequenceQueue.RemoveAt(0);
        bSequenceInFlight = false;
        TryStartNextQueuedSequence();
        return;
    }

    const FGameplayTagContainer& StepTags = Active.Steps[StepIndex];
    Active.PendingInStep = StepTags.Num();

    if (Active.PendingInStep == 0)
    {
        // Empty step -- nothing to wait on, advance immediately
        AdvanceSequenceToStep(StepIndex + 1);
        return;
    }

    for (const FGameplayTag& Tag : StepTags)
    {
        BindOnTagComplete(Tag, this,
            GET_FUNCTION_NAME_CHECKED(UGameEventTaskSubsystem, HandleSequenceStepTagComplete));
        TriggerTag(Tag);
    }
}

void UGameEventTaskSubsystem::HandleSequenceStepTagComplete()
{
    if (SequenceQueue.IsEmpty()) return; // defensive -- should not happen

    FGameEventQueuedTagSequence& Active = SequenceQueue[0];
    Active.PendingInStep--;

    if (Active.PendingInStep > 0) return;

    // Every tag in this step has completed -- unbind, notify, advance
    if (Active.Steps.IsValidIndex(Active.CurrentStepIndex))
    {
        const FGameplayTagContainer& StepTags = Active.Steps[Active.CurrentStepIndex];

        for (const FGameplayTag& Tag : StepTags)
        {
            UnbindOnTagComplete(Tag, this,
                GET_FUNCTION_NAME_CHECKED(UGameEventTaskSubsystem, HandleSequenceStepTagComplete));
        }

        Active.OnStepComplete.ExecuteIfBound(StepTags);
    }

    AdvanceSequenceToStep(Active.CurrentStepIndex + 1);
}
