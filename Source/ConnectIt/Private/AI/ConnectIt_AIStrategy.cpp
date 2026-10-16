// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIStrategy.h"
#include "Framework/Controller/ConnectIt_AIController.h"
#include "Action/ActionConfig.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedAction.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"


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
    // Keep the best MaxCandidates
    FConnectItAIDecision Trimmed = Decision;
    const int32 Keep = FMath::Max(1, MaxCandidates);
    if (Trimmed.Candidates.Num() > Keep)
    {
        Trimmed.Candidates.SetNum(Keep);
    }

    if (OnDecisionFinished.IsBound()) { OnDecisionFinished.Execute(Trimmed); }
}

FConnectItAIDecision UConnectIt_AIStrategy::MakeSingleMoveDecision(
    const FInstancedStruct& Operation, const FString& Summary)
{
    FConnectItAIDecision Decision;
    if (Operation.IsValid())
    {
        Decision.Candidates.Add({ Operation, Summary });
    }
    return Decision;
}

bool UConnectIt_AIStrategy::IsOperationAvailable(
    const FConnectItAIDecisionContext& Context, const FInstancedStruct& Operation)
{
    const FConnectItBoardOperation* BoardOperation = Operation.GetPtr<FConnectItBoardOperation>();
    return BoardOperation && Context.AvailableRequestTypes.HasTagExact(BoardOperation->GetRequestType());
}

UWorld* UConnectIt_AIStrategy::GetWorld() const
{
    if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) return nullptr;
    const UObject* Outer = GetOuter();
    return Outer ? Outer->GetWorld() : nullptr;
}
