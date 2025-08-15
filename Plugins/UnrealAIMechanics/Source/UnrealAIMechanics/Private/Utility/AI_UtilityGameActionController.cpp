// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/AI_UtilityGameActionController.h"
#include "Utility/AI_UtilityGameActionHandler.h"


void UAI_UtilityGameActionController::GetAllEvaluatedActions(TArray<FGameActionEvaluated>& OutEvaluatedActions)
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

void UAI_UtilityGameActionController::GetEvaluatedActions(
	TArray<FGameActionEvaluated>& OutEvaluatedActions,
	TSubclassOf<UAI_UtilityGameActionHandler> HandlerClass)
{
	for (auto* Handler : GameActionHandlers)
	{
		if (Handler->IsA(HandlerClass))
		{
			Handler->GetEvaluatedActions(OutEvaluatedActions);
		}
	}
}

void UAI_UtilityGameActionController::Execute()
{
	TArray<FGameActionEvaluated> EvaluatedActions;
	GetAllEvaluatedActions(EvaluatedActions);
	const FGameActionEvaluated Action = SelectEvaluatedAction(EvaluatedActions);
	Action.Execute();
}
