// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIStrategy.h"
#include "Framework/Controller/ConnectIt_AIController.h"


AConnectIt_AIController* UConnectIt_AIStrategy::GetOwningController() const
{
    return Cast<AConnectIt_AIController>(GetOuter());
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
