// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AI_UtilityLibrary.generated.h"

struct FGameActionEvaluated;
/**
 * 
 */
UCLASS()
class UNREALAIMECHANICS_API UAI_UtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static TArray<FGameActionEvaluated> SortGameActionEvaluations(const TArray<FGameActionEvaluated>& InEvaluatedActions, bool SortAscending = false);
};
