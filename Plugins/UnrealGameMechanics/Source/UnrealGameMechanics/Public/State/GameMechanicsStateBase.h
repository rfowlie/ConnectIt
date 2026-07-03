// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameMechanicsStateBase.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameStateDelegate);

/**
 * 
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class UNREALGAMEMECHANICS_API UGameMechanicsStateBase : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FGameStateDelegate OnGameStateEnter;

	UPROPERTY(BlueprintAssignable)
	FGameStateDelegate OnGameStateExit;
	
	UFUNCTION(BlueprintCallable)
	void Enter();

	UFUNCTION(BlueprintCallable)
	void Exit();

protected:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Enter_Internal();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Exit_Internal();
};
