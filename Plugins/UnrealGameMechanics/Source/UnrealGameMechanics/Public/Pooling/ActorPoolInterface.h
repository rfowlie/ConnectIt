// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ActorPoolInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UActorPoolInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALGAMEMECHANICS_API IActorPoolInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ActivatePoolObject();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void DeactivatePoolObject();

	// UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	// bool IsPoolObjectActive();
};
