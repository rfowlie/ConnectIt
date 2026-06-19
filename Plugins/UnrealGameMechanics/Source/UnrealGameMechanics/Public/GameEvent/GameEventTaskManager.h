// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameEventTaskManager.generated.h"


class UGameEventTask_Async;

DECLARE_DYNAMIC_DELEGATE(FGameEventTaskDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameEventTaskManagerDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllTasksCompleted);


USTRUCT()
struct FAsyncTaskArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<UGameEventTask_Async*> Tasks;
};

/**
 * we want this manager to allow the sequential execution of registered code,
 * or allow a variety of objets to register and unregister when they are done their work
 * objects can then sub to the on all tasks complete delegate to be notified when everything
 * is completed in this manager
 */
UCLASS(BlueprintType)
class UNREALGAMEMECHANICS_API UGameEventTaskManager : public UObject
{
	GENERATED_BODY()

	UGameEventTaskManager();
	
public:
	static UGameEventTaskManager* Create();

	// delegates
	UPROPERTY(BlueprintAssignable)
	FGameEventTaskManagerDelegate OnManagerBegin;

	UPROPERTY(BlueprintAssignable)
	FGameEventTaskManagerDelegate OnManagerComplete;

	// register
	UFUNCTION(BlueprintCallable, Category = "Turn Completion")
	void RegisterTask(UObject* Object);
	
	UFUNCTION(BlueprintCallable, Category = "GameEventTaskManager")
	void RegisterAsyncTask(UGameEventTask_Async* InAsyncTask, int32 Phase = 0);
	
	// Mark an object as completed
	UFUNCTION(BlueprintCallable, Category = "Turn Completion")
	void UnregisterTask(UObject* Object);

	UFUNCTION(BlueprintCallable)
	void InitiateAllTasks();
	
protected:	
	UFUNCTION()
	void InitiateAsyncTasks();

	UPROPERTY(BlueprintReadOnly)
	bool bIsActive = false;

private:
	UPROPERTY()
	bool bAsyncTasksInitiated = false;
	
	// The set of objects that need to complete before moving forward
	UPROPERTY()
	TSet<UObject*> TaskSet;

	// Check if all objects are done
	void CheckTasksComplete() const;

	UPROPERTY()
	TMap<int32, FAsyncTaskArray> AsyncTaskMap;

	UPROPERTY()
	TMap<int32, FAsyncTaskArray> AsyncTaskMapPersistent;

	UPROPERTY()
	TArray<int32> PhaseOrder;

	UPROPERTY()
	int32 PhaseIndex = -1;

	UFUNCTION()
	void ExecuteNextPhase();
	
	UFUNCTION()
	void CheckPhaseComplete(UGameEventTask_Async* AsyncTask);
};
