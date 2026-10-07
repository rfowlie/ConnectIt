// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIStrategy.h"
#include "Framework/Controller/ConnectIt_AIController.h"
#include "Action/ActionConfig.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedAction.h"


AConnectIt_AIController* UConnectIt_AIStrategy::GetOwningController() const
{
    return Cast<AConnectIt_AIController>(GetOuter());
}

bool UConnectIt_AIStrategy::LoadoutGrantsRequestType(
    const UActionLoadoutDataAsset* Loadout, FGameplayTag RequestType)
{
    if (!IsValid(Loadout) || !RequestType.IsValid()) return false;

    const auto Grants = [RequestType](const TSubclassOf<UTurnBasedAction>& ActionClass)
    {
        const UTurnBasedAction* DefaultAction =
            ActionClass ? ActionClass->GetDefaultObject<UTurnBasedAction>() : nullptr;
        return IsValid(DefaultAction) && DefaultAction->ProducesRequestType(RequestType);
    };

    for (const FPermanentActionConfig& Config : Loadout->PermanentActions)
    {
        if (Grants(Config.ActionClass)) return true;
    }
    for (const FNumberedActionConfig& Config : Loadout->NumberedActions)
    {
        if (Grants(Config.ActionClass)) return true;
    }
    return false;
}

void UConnectIt_AIStrategy::StartDecision(const FConnectItAIDecisionContext& Context)
{
    if (IsDecisionActive())
    {
        StopDecision();
    }

    ActiveDecisionId = NextDecisionId++;
    BeginDecision(Context);
}

void UConnectIt_AIStrategy::StopDecision()
{
    if (!IsDecisionActive()) return;

    ActiveDecisionId = INDEX_NONE;
    CancelDecision();
}

void UConnectIt_AIStrategy::BeginDecision_Implementation(const FConnectItAIDecisionContext& Context)
{
    UE_LOG(LogTemp, Error,
        TEXT("ConnectIt_AIStrategy: %s doesn't implement BeginDecision -- "
             "no move"),
        *GetClass()->GetName());

    FinishDecision(FConnectItAIDecision());
}

void UConnectIt_AIStrategy::CancelDecision_Implementation()
{
}

void UConnectIt_AIStrategy::FinishDecision(const FConnectItAIDecision& Decision)
{
    if (!IsDecisionActive())
    {
        // Cancelled (or already finished) -- a late answer, drop it
        return;
    }

    ActiveDecisionId = INDEX_NONE;
    OnDecisionFinished.ExecuteIfBound(Decision);
}
