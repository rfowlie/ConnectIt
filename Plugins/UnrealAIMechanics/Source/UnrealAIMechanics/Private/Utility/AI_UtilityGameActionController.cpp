// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/AI_UtilityGameActionController.h"
#include "Utility/AI_UtilityGameActionEvaluator.h"
#include "Utility/AI_UtilityStructs.h"


FGameActionEvaluated UAI_UtilityGameActionController::PickNextAction()
{
	TArray<FGameActionEvaluated> EvaluatedActions;
	GetEvaluatedActions(EvaluatedActions);
	return SelectEvaluatedAction(EvaluatedActions);
}

void UAI_UtilityGameActionController::GetEvaluatedActions(TArray<FGameActionEvaluated>& OutEvaluatedActions)
{
	// Gather actions from each evaluator
	for (auto* Handler : GameActionHandlers)
	{
		if (Handler)
		{
			TArray<FGameActionEvaluated> Evaluated;
			Handler->GetEvaluatedActions(Evaluated);
			OutEvaluatedActions.Append(Evaluated);
		}
	}
}

// void UAI_UtilityGameActionController::GetEvaluatedActionsByClass(
// 	TArray<FGameActionEvaluated>& OutEvaluatedActions,
// 	TSubclassOf<UAI_UtilityGameActionEvaluator> HandlerClass)
// {
// 	for (auto* Handler : GameActionHandlers)
// 	{
// 		if (Handler->IsA(HandlerClass))
// 		{
// 			Handler->GetEvaluatedActions(OutEvaluatedActions);
// 		}
// 	}
// }

FGameActionEvaluated UAI_UtilityGameActionController::SelectEvaluatedAction(
	const TArray<FGameActionEvaluated>& InEvaluatedActions)
{
	return FGameActionEvaluated();
}
