// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridTileSelector.generated.h"

class UGridTileSelectorComponent;

// This class does not need to be modified.
UINTERFACE(Blueprintable, BlueprintType)
class UGridTileSelector : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALGRIDMECHANICS_API IGridTileSelector
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	UGridTileSelectorComponent* GetGridTileSelectorComponent();
};
