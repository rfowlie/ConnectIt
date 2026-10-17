// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIStrategy_Coordinator.h"
#include "Engine/World.h"
#include "TimerManager.h"


FGameplayTagContainer UConnectIt_AIStrategy_Coordinator::GetSupportedRequestTypes_Implementation() const
{
    FGameplayTagContainer Supported;
    for (const UConnectIt_AIStrategy* Child : Children)
    {
        if (IsValid(Child))
        {
            Supported.AppendTags(Child->GetSupportedRequestTypes());
        }
    }
    return Supported;
}

void UConnectIt_AIStrategy_Coordinator::BeginDecision_Implementation(const FConnectItAIDecisionContext& Context)
{
    CollectingDecisionId = GetActiveDecisionId();
    ActiveContext = Context;
    bBudgetEnded = false;
    Collected.Reset();

    // Each child is told it has a share of the budget
    FConnectItAIDecisionContext ChildContext = Context;
    if (TimeBudgetSeconds > 0.f)
    {
        const float ChildBudget = TimeBudgetSeconds * ChildBudgetShare;
        ChildContext.TimeBudgetSeconds = Context.TimeBudgetSeconds > 0.f
            ? FMath::Min(Context.TimeBudgetSeconds, ChildBudget) : ChildBudget;
    }

    for (int32 Index = 0; Index < Children.Num(); Index++)
    {
        FConnectItAIChildDecision& Entry = Collected.AddDefaulted_GetRef();
        Entry.ChildIndex = Index;
        Entry.Child = Children[Index];
    }

    if (Collected.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_AIStrategy_Coordinator: %s has no children -- no move"), *GetName());
        FinishDecision(FConnectItAIDecision());
        return;
    }

    // The budget timer, before the children start: a child may answer at once
    if (TimeBudgetSeconds > 0.f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(BudgetTimer,
                FTimerDelegate::CreateUObject(this, &UConnectIt_AIStrategy_Coordinator::EndTimeBudget),
                TimeBudgetSeconds, false);
        }
    }

    const int32 DecisionId = CollectingDecisionId;
    for (int32 Index = 0; Index < Children.Num(); Index++)
    {
        UConnectIt_AIStrategy* Child = Children[Index];
        if (!IsDecisionActive(DecisionId)) return; // concluded or cancelled meanwhile

        if (!IsValid(Child))
        {
            Collected[Index].bAnswered = true; // nothing to wait for
            continue;
        }

        Child->OnDecisionFinished.BindUObject(this, &UConnectIt_AIStrategy_Coordinator::HandleChildFinished, Index);
        Child->StartDecision(ChildContext);
    }

    // Every child may have answered synchronously
    if (IsDecisionActive(DecisionId) && Collected.ContainsByPredicate(
        [](const FConnectItAIChildDecision& Entry) { return !Entry.bAnswered; }) == false)
    {
        Conclude();
    }
}

void UConnectIt_AIStrategy_Coordinator::HandleChildFinished(const FConnectItAIDecision& Decision, int32 ChildIndex)
{
    if (!IsDecisionActive(CollectingDecisionId) || !Collected.IsValidIndex(ChildIndex)) return;

    Collected[ChildIndex].Decision = Decision;
    Collected[ChildIndex].bAnswered = true;

    const bool bAllAnswered = !Collected.ContainsByPredicate(
        [](const FConnectItAIChildDecision& Entry) { return !Entry.bAnswered; });

    // All in, or the budget already ran out and this was the first answer
    if (bAllAnswered || bBudgetEnded)
    {
        Conclude();
    }
}

void UConnectIt_AIStrategy_Coordinator::EndTimeBudget()
{
    if (!IsDecisionActive(CollectingDecisionId)) return;
    bBudgetEnded = true;

    const bool bAnyAnswered = Collected.ContainsByPredicate(
        [](const FConnectItAIChildDecision& Entry) { return Entry.bAnswered && Entry.Decision.HasMove(); });
    if (bAnyAnswered)
    {
        Conclude();
    }
    // else keep waiting: the first answer to arrive concludes
}

void UConnectIt_AIStrategy_Coordinator::Conclude()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BudgetTimer);
    }
    StopChildren();

    FinishDecision(RankCandidates(ActiveContext, Collected));
}

void UConnectIt_AIStrategy_Coordinator::CancelDecision_Implementation()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BudgetTimer);
    }
    StopChildren();
}

void UConnectIt_AIStrategy_Coordinator::StopChildren()
{
    // Callbacks stay bound (this can run from inside one of them); an answer
    // arriving after the coordinator concluded is ignored by its decision id
    for (UConnectIt_AIStrategy* Child : Children)
    {
        if (IsValid(Child))
        {
            Child->StopDecision();
        }
    }
}

FConnectItAIDecision UConnectIt_AIStrategy_Coordinator::RankCandidates_Implementation(
    const FConnectItAIDecisionContext& Context, const TArray<FConnectItAIChildDecision>& ChildDecisions)
{
    // Child priority order: the first child's candidates first, then the
    // next child's; a move already offered by a higher-priority child is
    // skipped
    FConnectItAIDecision Ranked;
    for (const FConnectItAIChildDecision& Entry : ChildDecisions)
    {
        if (!Entry.bAnswered) continue;

        for (const FConnectItAICandidate& Candidate : Entry.Decision.Candidates)
        {
            const bool bRepeat = Ranked.Candidates.ContainsByPredicate(
                [&Candidate](const FConnectItAICandidate& Existing) { return Existing.Payload == Candidate.Payload; });
            if (!bRepeat)
            {
                Ranked.Candidates.Add(Candidate);
            }
        }
    }
    return Ranked;
}
