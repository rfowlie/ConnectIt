// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "ConnectIt_GameStateHandlerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UConnectIt_GameStateHandlerInterface : public UInterface
{
	GENERATED_BODY()
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnConnectItStateDelegate, FGameplayTag, Tag);

/**
 * 
 */
class CONNECTIT_API IConnectIt_GameStateHandlerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Delegate Registration")
	void RegisterGameStateCallback(const FOnConnectItStateDelegate& Callback);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Delegate Registration")
	void UnregisterGameStateCallback(const FOnConnectItStateDelegate& Callback);
	
};
