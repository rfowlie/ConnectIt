// Fill out your copyright notice in the Description page of Project Settings.

#include "Action/TurnEndRequirement.h"
#include "Action/TurnBasedPlayerAction.h"


ETurnEndRequirementResult UTurnEndRequirementLeaf::Evaluate(
    const FTurnEndUses& Uses) const
{
    const FGameplayTag ActionTag = UTurnBasedPlayerAction::GetTagForClass(ActionClass);

    if (!bIsEnabled || !ActionTag.IsValid())
    {
        // Disabled, or an unconfigured leaf (no class, or a class with no
        // tag) with nothing to require
        return ETurnEndRequirementResult::Skipped;
    }

    return Uses.ActionUsesThisTurn(ActionTag) >= RequiredUsesThisTurn
        ? ETurnEndRequirementResult::Satisfied
        : ETurnEndRequirementResult::Unsatisfied;
}

ETurnEndRequirementResult UTurnEndRequirementItemUsed::Evaluate(
    const FTurnEndUses& Uses) const
{
    if (!bIsEnabled)
    {
        return ETurnEndRequirementResult::Skipped;
    }

    return Uses.ItemUsesThisTurn >= RequiredItemUses
        ? ETurnEndRequirementResult::Satisfied
        : ETurnEndRequirementResult::Unsatisfied;
}

ETurnEndRequirementResult UTurnEndRequirementGroup::Evaluate(
    const FTurnEndUses& Uses) const
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
            Child->Evaluate(Uses);

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
