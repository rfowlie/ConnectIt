// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AI_UtilityGameActionController.generated.h"

class UAI_UtilityGameActionEvaluator;
struct FGameActionEvaluated;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class UNREALAIMECHANICS_API UAI_UtilityGameActionController : public UObject
{
	GENERATED_BODY()

public:
	// template function for now
	UFUNCTION(BlueprintCallable, Category = "AI | Utility | Evaluator")
	FGameActionEvaluated PickNextAction();
	
	// get evaluated actions from all stored evaluators
	UFUNCTION(BlueprintCallable)
	void GetEvaluatedActions(TArray<FGameActionEvaluated>& OutEvaluatedActions);

	// get evaluated action from specified evaluator
	// UFUNCTION(BlueprintCallable)
	// void GetEvaluatedActionsByClass(TArray<FGameActionEvaluated>& OutEvaluatedActions, TSubclassOf<UAI_UtilityGameActionEvaluator> HandlerClass);

	// TODO: this should maybe be another class, how do we make it virtual for C++ and blueprint again?
	UFUNCTION(BlueprintCallable)
	FGameActionEvaluated SelectEvaluatedAction(const TArray<FGameActionEvaluated>& InEvaluatedActions);

	
protected:	
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite)
	TArray<UAI_UtilityGameActionEvaluator*> GameActionHandlers;
};
