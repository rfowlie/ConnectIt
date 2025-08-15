// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AI_UtilityGameActionHandler.generated.h"



DECLARE_DYNAMIC_DELEGATE_OneParam(FGameActionEvaluatedSignature, const FGameActionEvaluated&, ActionData);

USTRUCT(BlueprintType)
struct FGameActionEvaluated
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "AI | Utility")
	float Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "AI | Utility")
	TMap<FGameplayTag, float> Scoring;

	/*
	 * the expectation is that handlers will set the correct execute logic here
	 */
	UPROPERTY(BlueprintReadWrite, Category = "AI | Utility")
	FGameActionEvaluatedSignature ExecuteAction;

	UFUNCTION()
	void Execute() const
	{
		if (ExecuteAction.IsBound()) { ExecuteAction.Execute(*this); }
	}
	
};

/**
 * 
 */
UCLASS()
class UNREALAIMECHANICS_API UAI_UtilityGameActionHandler : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintNativeEvent, BlueprintCallable)
	void GetEvaluatedActions(TArray<FGameActionEvaluated>& InEvaluatedActions);
};

