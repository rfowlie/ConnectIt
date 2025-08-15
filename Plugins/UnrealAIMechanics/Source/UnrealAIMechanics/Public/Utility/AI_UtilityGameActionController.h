// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AI_UtilityGameActionController.generated.h"

class UAI_UtilityGameActionHandler;
struct FGameActionEvaluated;

/**
 * 
 */
UCLASS()
class UNREALAIMECHANICS_API UAI_UtilityGameActionController : public UObject
{
	GENERATED_BODY()

public:
	// get evaluated actions from all stored handlers
	UFUNCTION(BlueprintCallable)
	void GetAllEvaluatedActions(TArray<FGameActionEvaluated>& OutEvaluatedActions);

	// get evaluated action from specified handler
	UFUNCTION(BlueprintCallable)
	void GetEvaluatedActions(TArray<FGameActionEvaluated>& OutEvaluatedActions, TSubclassOf<UAI_UtilityGameActionHandler> HandlerClass);

	UFUNCTION(BlueprintCallable)
	FGameActionEvaluated SelectEvaluatedAction(const TArray<FGameActionEvaluated>& InEvaluatedActions);

	// should run all logic to conclusion
	UFUNCTION(BlueprintCallable)
	void Execute();

	
protected:	
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite)
	TArray<UAI_UtilityGameActionHandler*> GameActionHandlers;
};
