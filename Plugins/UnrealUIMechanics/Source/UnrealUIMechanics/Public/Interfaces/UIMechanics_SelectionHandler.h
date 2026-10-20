// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UIMechanics_SelectionHandler.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UUIMechanics_SelectionHandler : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALUIMECHANICS_API IUIMechanics_SelectionHandler
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category= "UI Mechanics")
	void UIHandleCanSelect();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category= "UI Mechanics")
	void UIHandleCannotSelect();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category= "UI Mechanics")
	void UIHandleSelected();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category= "UI Mechanics")
	void UIHandleDeselected();
	
	
};
