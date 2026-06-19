// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "GameEventTaskHandler.generated.h"

class UGameEventTaskManager;

// This class does not need to be modified.
UINTERFACE()
class UGameEventTaskHandler : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALGAMEMECHANICS_API IGameEventTaskHandler
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GameEventTaskHandler")
	UGameEventTaskManager* GetGameEventTaskManagerByTag(FGameplayTag InGameplayTag);
};
