// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/AI_UtilityLibrary.h"
#include "Utility/AI_UtilityStructs.h"


TArray<FGameActionEvaluated> UAI_UtilityLibrary::SortGameActionEvaluations(const TArray<FGameActionEvaluated>& InEvaluatedActions, bool SortAscending)
{
	TArray<FGameActionEvaluated> OutEvaluatedActions = InEvaluatedActions;
	OutEvaluatedActions.Sort([SortAscending](const FGameActionEvaluated& A, const FGameActionEvaluated& B)
	{
		if (SortAscending) { return A.Score < B.Score; }
		else { return A.Score > B.Score; }
	});

	return OutEvaluatedActions;
}
