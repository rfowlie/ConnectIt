// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Subsystem/ConnectIt_GameEventSubsystem.h"

#include "GameEvent/GameEventTaskManager.h"
#include "GameEvent/GameEventTask_Async.h"


void UConnectIt_GameEventSubsystem::RegisterAsyncTask(
    FGameplayTag Tag, UGameEventTask_Async* Task, int32 Phase)
{
    GetOrCreateManager(Tag)->RegisterAsyncTask(Task, Phase);
}

void UConnectIt_GameEventSubsystem::TriggerTag(FGameplayTag Tag)
{
    GetOrCreateManager(Tag)->InitiateAllTasks();
}

void UConnectIt_GameEventSubsystem::BindOnTagBegin(
    FGameplayTag Tag, UObject* Listener, FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerBegin.Add(Delegate);
}

void UConnectIt_GameEventSubsystem::UnbindOnTagBegin(
    FGameplayTag Tag, UObject* Listener, FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerBegin.Remove(Delegate);
}

void UConnectIt_GameEventSubsystem::BindOnTagComplete(
    FGameplayTag Tag, UObject* Listener, FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerComplete.Add(Delegate);
}

void UConnectIt_GameEventSubsystem::UnbindOnTagComplete(
    FGameplayTag Tag, UObject* Listener, FName FunctionName)
{
    if (!IsValid(Listener)) return;

    FScriptDelegate Delegate;
    Delegate.BindUFunction(Listener, FunctionName);
    GetOrCreateManager(Tag)->OnManagerComplete.Remove(Delegate);
}

UGameEventTaskManager* UConnectIt_GameEventSubsystem::GetGameEventTaskManagerByTag_Implementation(
    FGameplayTag InGameplayTag)
{
    return GetOrCreateManager(InGameplayTag);
}

UGameEventTaskManager* UConnectIt_GameEventSubsystem::GetOrCreateManager(FGameplayTag Tag)
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
