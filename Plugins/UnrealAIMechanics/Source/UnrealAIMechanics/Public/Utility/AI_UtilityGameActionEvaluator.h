// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AI_UtilityGameActionEvaluator.generated.h"


struct FGameActionEvaluated;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class UNREALAIMECHANICS_API UAI_UtilityGameActionEvaluator : public UObject
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AI | Utility | Evaluator")
	void GetEvaluatedActions(TArray<FGameActionEvaluated>& OutEvaluatedActions);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AI | Utility | Evaluator")
	void SetEvaluationWeights(const TMap<FGameplayTag, float>& InEvaluationWeights);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Utility | Evaluator")
	TMap<FGameplayTag, float> EvaluationWeights;	
};
