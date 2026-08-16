// Fill out your copyright notice in the Description page of Project Settings.


#include "GameEvent/GameEventTaskManager.h"
#include "GameEvent/GameEventTask_Async.h"


UGameEventTaskManager::UGameEventTaskManager()
{
}

UGameEventTaskManager* UGameEventTaskManager::Create()
{
	UGameEventTaskManager* Object = NewObject<UGameEventTaskManager>();
	return Object;
}

void UGameEventTaskManager::RegisterAsyncTask(UGameEventTask_Async* InAsyncTask, int32 Phase)
{
	// do not allow registering of tasks while executing
	if (bAsyncTasksInitiated) { return; }
	if (InAsyncTask == nullptr) { return; }
	if (!InAsyncTask->OnExecuteDelegate.IsBound()) { return; }

	// prevent negative numbers we want to start at zero
	Phase = FMath::Max(Phase, 0);
	
	if (InAsyncTask->bIsPersistentTask)
	{
		if (!AsyncTaskMapPersistent.Contains(Phase))
		{
			AsyncTaskMapPersistent.Add(Phase, FAsyncTaskArray());
		}

		AsyncTaskMapPersistent[Phase].Tasks.AddUnique(InAsyncTask);
	}
	else
	{
		if (!AsyncTaskMap.Contains(Phase))
		{
			AsyncTaskMap.Add(Phase, FAsyncTaskArray());
		}
		
		AsyncTaskMap[Phase].Tasks.AddUnique(InAsyncTask);
	}
}

void UGameEventTaskManager::RegisterTask(UObject* Object)
{
	if (Object && !TaskSet.Contains(Object))
	{
		TaskSet.Add(Object);
		UE_LOG(LogTemp, Log, TEXT("Registered: %s"), *Object->GetName());
	}
}

void UGameEventTaskManager::UnregisterTask(UObject* Object)
{
	if (Object && TaskSet.Contains(Object))
	{
		TaskSet.Remove(Object);
		UE_LOG(LogTemp, Log, TEXT("Completed: %s"), *Object->GetName());
		CheckTasksComplete();
	}
}

void UGameEventTaskManager::InitiateAllTasks()
{
	// Do not broadcast OnManagerBegin here -- InitiateAsyncTasks already
	// broadcasts it once. Broadcasting from both meant every trigger fired
	// OnManagerBegin twice.
	InitiateAsyncTasks();
}

void UGameEventTaskManager::InitiateAsyncTasks()
{
	if (bAsyncTasksInitiated) { return; }
	bAsyncTasksInitiated = true;

	// add persistent tasks to AsyncTaskMap
	TArray<int32> OutPhaseOrder;
	AsyncTaskMapPersistent.GetKeys(OutPhaseOrder);
	for (auto Phase : OutPhaseOrder)
	{
		if (!AsyncTaskMap.Contains(Phase))
		{
			AsyncTaskMap.Add(Phase, AsyncTaskMapPersistent[Phase]);
		}
		else
		{
			for (auto Task : AsyncTaskMapPersistent[Phase].Tasks)
			{
				AsyncTaskMap[Phase].Tasks.AddUnique(Task);
			}
		}
	}

	// remove any tasks that are no long persistent
	// (was an unsigned reverse loop that underflowed at Num()==0 and never
	// inspected index 0 -- RemoveAll is correct regardless of array size)
	for (const auto Phase : OutPhaseOrder)
	{
		AsyncTaskMapPersistent[Phase].Tasks.RemoveAll(
			[](const UGameEventTask_Async* Task)
			{
				return !IsValid(Task) || !Task->bIsPersistentTask;
			});
	}

	if (OnManagerBegin.IsBound()) {	OnManagerBegin.Broadcast(EventTag); }
	
	AsyncTaskMap.GetKeys(PhaseOrder);
	PhaseOrder.Sort();
	PhaseIndex = -1;
	ExecuteNextPhase();
}

void UGameEventTaskManager::CheckTasksComplete() const
{
	if (TaskSet.IsEmpty())
	{
		if (OnManagerComplete.IsBound()) { OnManagerComplete.Broadcast(EventTag); }
	}
}

void UGameEventTaskManager::ExecuteNextPhase()
{
	PhaseIndex++;
	// if at end of array completely finished
	if (!PhaseOrder.IsValidIndex(PhaseIndex))
	{
		if (OnManagerComplete.IsBound()) { OnManagerComplete.Broadcast(EventTag); }
		bAsyncTasksInitiated = false;
		return;
	}

	// bind to all tasks in current phase and initiate them
	for (const auto AsyncTask : AsyncTaskMap[PhaseOrder[PhaseIndex]].Tasks)
	{
		AsyncTask->OnComplete.AddUniqueDynamic(this, &ThisClass::UGameEventTaskManager::CheckPhaseComplete);
		AsyncTask->OnExecuteDelegate.Execute();
	}
}

void UGameEventTaskManager::CheckPhaseComplete(UGameEventTask_Async* AsyncTask)
{
	// Guard against a task's OnComplete firing after this manager has
	// already finished all phases (PhaseIndex sits at PhaseOrder.Num(), an
	// invalid index, from the moment ExecuteNextPhase runs off the end
	// until the next InitiateAsyncTasks reset) -- e.g. a late/overrun
	// animation, or a stale task firing twice. Previously this indexed
	// PhaseOrder[PhaseIndex] unconditionally and crashed.
	if (!PhaseOrder.IsValidIndex(PhaseIndex))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UGameEventTaskManager::CheckPhaseComplete - '%s' completed "
				 "after manager already finished -- ignoring"),
			*GetNameSafe(AsyncTask));
		return;
	}

	const int32 CurrentPhase = PhaseOrder[PhaseIndex];

	if (!AsyncTaskMap.Contains(CurrentPhase))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UGameEventTaskManager::CheckPhaseComplete - no task "
				 "bucket for phase %d -- ignoring"),
			CurrentPhase);
		return;
	}

	// flag if task does not belong to current phase
	if (AsyncTaskMap[CurrentPhase].Tasks.IsEmpty() || !AsyncTaskMap[CurrentPhase].Tasks.Contains(AsyncTask))
	{
		UE_LOG(LogTemp, Error, TEXT("UGameEventTaskManager::CheckPhaseComplete - Async task firing out of place!!"))
		return;
	}

	// remove task from array
	AsyncTask->OnComplete.RemoveAll(this);
	AsyncTaskMap[CurrentPhase].Tasks.Remove(AsyncTask);

	// check if array is empty, signal to move onto next phase
	if (AsyncTaskMap[CurrentPhase].Tasks.IsEmpty())
	{
		ExecuteNextPhase();
	}
}
