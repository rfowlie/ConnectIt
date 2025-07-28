// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GridMechanics_GridLibrary.generated.h"

/**
 * 
 */
UCLASS()
class UNREALGRIDMECHANICS_API UGridMechanics_GridLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static FGridPosition CalculateGridPositionFromSize(const AActor* Actor, int32 Size);
};
