// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "GridFactionInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UGridFactionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class UNREALGRIDMECHANICS_API IGridFactionInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Faction")
	FGameplayTag GetFactionTag();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Faction")
	int32 GetFactionId();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Faction")
	void SetFactionId(int32 Id);
};
