// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "AI_UtilityStructs.generated.h"

/*
 * this will act as a payload to activate GAS events (gameplay tag + context object)
 */
USTRUCT(BlueprintType)
struct FGameActionEvaluated
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, Category = "AI | Utility")
	FGameplayTag GameplayActionTag;

	UPROPERTY(BlueprintReadWrite, Category = "AI | Utility")
	UObject* GameplayActionContext;
	
	UPROPERTY(BlueprintReadWrite, Category = "AI | Utility")
	float Score = 0;

	// add all values in scoring map and set score
	void UpdateScore()
	{
		Score = 0;
		for (const auto OutScore : Scoring)
		{
			Score += OutScore.Value;
		}
	}
	
	// to help with debugging
	UPROPERTY(BlueprintReadWrite, Category = "AI | Utility")
	TMap<FGameplayTag, float> Scoring;	
	
};