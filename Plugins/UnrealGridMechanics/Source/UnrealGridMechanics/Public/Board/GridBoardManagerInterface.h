// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridBoardManagerInterface.generated.h"

class UGridTileRegistryComponent;
class UGridPieceRegistryComponent;
class UBoardStateComponentBase;
class UBoardShiftComponent;


// This class does not need to be modified.
UINTERFACE()
class UGridBoardManagerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALGRIDMECHANICS_API IGridBoardManagerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Grid|Board")
	UGridTileRegistryComponent* GetTileRegistry() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Grid|Board")
	UGridPieceRegistryComponent* GetPieceRegistry() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Grid|Board")
	UBoardStateComponentBase* GetBoardState() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Grid|Board")
	UBoardShiftComponent* GetShiftComponent() const;
	
};
