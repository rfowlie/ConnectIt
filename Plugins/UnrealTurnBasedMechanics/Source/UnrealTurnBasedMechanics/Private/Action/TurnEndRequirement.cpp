// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/TurnEndRequirement.h"
#include "Action/TurnBasedAction.h"


ETurnEndRequirementResult UTurnEndRequirementLeaf::Evaluate(
    FTurnEndUsesLookup GetUsesThisTurn) const
{
    const FGameplayTag ActionTag = UTurnBasedAction::GetTagForClass(ActionClass);

    if (!bIsEnabled || !ActionTag.IsValid())
    {
        // Disabled, or an unconfigured leaf (no class, or a class with no
        // tag) with nothing to require
        return ETurnEndRequirementResult::Skipped;
    }

    return GetUsesThisTurn(ActionTag) >= RequiredUsesThisTurn
        ? ETurnEndRequirementResult::Satisfied
        : ETurnEndRequirementResult::Unsatisfied;
}

ETurnEndRequirementResult UTurnEndRequirementGroup::Evaluate(
    FTurnEndUsesLookup GetUsesThisTurn) const
{
    if (!bIsEnabled)
    {
        return ETurnEndRequirementResult::Skipped;
    }

    bool bAnyEvaluated = false;
    bool bAnySatisfied = false;
    bool bAllSatisfied = true;

    for (const UTurnEndRequirementNode* Child : Children)
    {
        if (!IsValid(Child)) continue;

        const ETurnEndRequirementResult ChildResult =
            Child->Evaluate(GetUsesThisTurn);

        if (ChildResult == ETurnEndRequirementResult::Skipped) continue;

        bAnyEvaluated = true;
        if (ChildResult == ETurnEndRequirementResult::Satisfied)
        {
            bAnySatisfied = true;
        }
        else
        {
            bAllSatisfied = false;
        }
    }

    // Every child skipped -- this group contributes nothing either
    if (!bAnyEvaluated)
    {
        return ETurnEndRequirementResult::Skipped;
    }

    const bool bSatisfied = (Mode == ETurnEndCombineMode::Any)
        ? bAnySatisfied
        : bAllSatisfied;

    return bSatisfied
        ? ETurnEndRequirementResult::Satisfied
        : ETurnEndRequirementResult::Unsatisfied;
}
