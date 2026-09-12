// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TurnBasedActionNotifier.generated.h"

class UTurnBasedAction;


DECLARE_DYNAMIC_DELEGATE_OneParam(FTurnBasedActionNotifierDelegate, TSubclassOf<UTurnBasedAction>, ActionClass);

// This class does not need to be modified.
UINTERFACE()
class UTurnBasedActionNotifier : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALTURNBASEDMECHANICS_API ITurnBasedActionNotifier
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TurnBasedActionNotifier")
	bool RegisterActionNotifier(const FTurnBasedActionNotifierDelegate& Delegate);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TurnBasedActionNotifier")
	bool UnregisterActionNotifier(UObject* Object);
};
