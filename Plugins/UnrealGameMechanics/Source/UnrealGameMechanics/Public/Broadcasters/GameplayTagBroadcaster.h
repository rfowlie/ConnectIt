// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "GameplayTagBroadcaster.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UGameplayTagBroadcaster : public UInterface
{
	GENERATED_BODY()
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGameplayTagDelegate, FGameplayTag, Tag);

/**
 * 
 */
class UNREALGAMEMECHANICS_API IGameplayTagBroadcaster
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Delegate Registration")
	void Register(const FOnGameplayTagDelegate& Callback);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Delegate Registration")
	void Unregister(const FOnGameplayTagDelegate& Callback);
};
