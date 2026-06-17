// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameMechanicsStateSimple.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameStateSimpleDelegate);

/**
 * 
 */
UCLASS()
class UNREALGAMEMECHANICS_API UGameMechanicsStateSimple : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FGameStateSimpleDelegate OnStateComplete;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Enter();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Exit();

};
