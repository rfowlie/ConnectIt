// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIStrategy.h"
#include "Framework/Controller/ConnectIt_AIController.h"
#include "Action/ActionConfig.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedPlayerAction.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"


AConnectIt_AIController* UConnectIt_AIStrategy::GetOwningController() const
{
    return GetTypedOuter<AConnectIt_AIController>();
}

bool UConnectIt_AIStrategy::LoadoutGrantsOperation(
    const UActionLoadoutDataAsset* Loadout, FGameplayTag OperationTag)
{
    if (!IsValid(Loadout) || !OperationTag.IsValid()) return false;

    const auto Grants = [OperationTag](const TSubclassOf<UTurnBasedPlayerAction>& ActionClass)
    {
        const UTurnBasedPlayerAction* DefaultAction =
            ActionClass ? ActionClass->GetDefaultObject<UTurnBasedPlayerAction>() : nullptr;
        return IsValid(DefaultAction) && DefaultAction->ProducesRequestType(OperationTag);
    };

    for (const FPermanentActionConfig& Config : Loadout->PermanentActions)
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
    BestSoFar = FConnectItAIDecision();
    bDecisionFinished = false;
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
    BestSoFar = PrepareDecision(Decision);
    bDecisionFinished = true;
}

void UConnectIt_AIStrategy::PublishBestSoFar(const FConnectItAIDecision& Decision)
{
    if (!IsDecisionActive()) return;

    BestSoFar = PrepareDecision(Decision);
}

FConnectItAIDecision UConnectIt_AIStrategy::GetBestSoFar() const
{
    return BestSoFar;
}

FConnectItAIDecision UConnectIt_AIStrategy::PrepareDecision(const FConnectItAIDecision& Decision) const
{
    // Keep the best MaxCandidates
    FConnectItAIDecision Prepared = Decision;
    const int32 Keep = FMath::Max(1, MaxCandidates);
    if (Prepared.Candidates.Num() > Keep)
    {
        Prepared.Candidates.SetNum(Keep);
    }

    const FName SourceLabel = GetSourceLabel();
    for (FConnectItAICandidate& Candidate : Prepared.Candidates)
    {
        if (Candidate.SourceLabel.IsNone())
        {
            Candidate.SourceLabel = SourceLabel;
        }
    }
    return Prepared;
}

FName UConnectIt_AIStrategy::GetSourceLabel() const
{
    if (!Label.IsNone()) return Label;

    // A Blueprint class's generated name ends in _C
    FString ClassName = GetClass()->GetName();
    ClassName.RemoveFromEnd(TEXT("_C"));
    return FName(*ClassName);
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
    return BoardOperation && Context.AvailableOperationTags.HasTagExact(BoardOperation->GetOperationTag());
}

UWorld* UConnectIt_AIStrategy::GetWorld() const
{
    if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) return nullptr;
    const UObject* Outer = GetOuter();
    return Outer ? Outer->GetWorld() : nullptr;
}
