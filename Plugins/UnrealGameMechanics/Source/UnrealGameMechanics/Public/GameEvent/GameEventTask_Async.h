// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameEventTask_Async.generated.h"

DECLARE_DYNAMIC_DELEGATE(FGameEventTaskExecute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameEventTask_Async, UGameEventTask_Async*, AsyncTask);

/**
 * 
 */
UCLASS(BlueprintType)
class UNREALGAMEMECHANICS_API UGameEventTask_Async : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	bool bIsPersistentTask = false;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGameEventTask_Async OnComplete;

	UPROPERTY(BlueprintReadWrite)
	FGameEventTaskExecute OnExecuteDelegate;
};
