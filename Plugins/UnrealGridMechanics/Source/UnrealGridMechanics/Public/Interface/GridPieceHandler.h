// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridPieceHandler.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UGridPieceHandler : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALGRIDMECHANICS_API IGridPieceHandler
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Grid Mechanics")
	bool HasPiece();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Grid Mechanics")
	AActor* GetPiece();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Grid Mechanics")
	void SetPiece(const AActor* InPiece);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Grid Mechanics")
	void RemovePiece();
};
