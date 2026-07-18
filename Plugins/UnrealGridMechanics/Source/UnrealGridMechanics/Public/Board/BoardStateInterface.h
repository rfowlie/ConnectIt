// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BoardStateInterface.generated.h"

class FOnBoardStateChanged;


// This class does not need to be modified.
UINTERFACE(Blueprintable, BlueprintType)
class UBoardStateInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALGRIDMECHANICS_API IBoardStateInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// Bind a raw function to the board state changed notification
	// Observer passes itself and its handler function
	// Provider binds them to its internal OnBoardStateChanged delegate
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Board State")
	void BindObserver(UObject* Observer, FName FunctionName);

	// Unbind a previously bound observer
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Board State")
	void UnbindObserver(UObject* Observer, FName FunctionName);
};
