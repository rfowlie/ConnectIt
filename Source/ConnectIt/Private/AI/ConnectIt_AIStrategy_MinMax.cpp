// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIStrategy_MinMax.h"


FGameplayTagContainer UConnectIt_AIStrategy_MinMax::GetSupportedRequestTypes_Implementation() const
{
    return ConnectItMinMaxSearch::GetModelledRequestTypes();
}

void UConnectIt_AIStrategy_MinMax::BeginDecision_Implementation(
    const FConnectItAIDecisionContext& Context)
{
    const GameIntelligence::Search::FCancelFlag Flag = GameIntelligence::Search::MakeCancelFlag();
    CancelFlag = Flag;
    Progress = MakeShared<FConnectItMinMaxSearchProgress, ESPMode::ThreadSafe>();

    const int32 DecisionId = GetActiveDecisionId();
    TWeakObjectPtr<UConnectIt_AIStrategy_MinMax> WeakThis(this);

    FString WhyNot;
    const bool bLaunched = ConnectItMinMaxSearch::Launch(Context, Search, Flag,
        [WeakThis, DecisionId](FConnectItMinMaxSearchResult&& Result, bool bCancelled)
        {
            UConnectIt_AIStrategy_MinMax* Self = WeakThis.Get();
            if (!Self || bCancelled || !Self->IsDecisionActive(DecisionId))
            {
                return;
            }

            Self->CancelFlag.Reset();

            if (Result.RankedMoves.IsEmpty())
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("ConnectIt_AIStrategy_MinMax: search found no moves "
                         "(board full?) -- no move"));
                Self->FinishDecision(FConnectItAIDecision());
                return;
            }

            // The chosen move first, then the rest in rank order as fallbacks
            // for a rejected move (FinishDecision keeps the best MaxCandidates)
            FConnectItAIDecision Decision = ConnectItMinMaxSearch::MakeDecision(
                Result.RankedMoves, Result.Describe(), Result.RankedMoves.Num());
            const int32 PickIndex = Self->PickMoveIndex(Result);
            if (PickIndex > 0)
            {
                const FConnectItAICandidate Picked = Decision.Candidates[PickIndex];
                Decision.Candidates.RemoveAt(PickIndex);
                Decision.Candidates.Insert(Picked, 0);
            }

            Self->FinishDecision(Decision);
        },
        &WhyNot,
        Progress);

    if (!bLaunched)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_AIStrategy_MinMax: can't search -- %s -- no move"), *WhyNot);
        CancelFlag.Reset();
        Progress.Reset();
        FinishDecision(FConnectItAIDecision());
    }
}

FConnectItAIDecision UConnectIt_AIStrategy_MinMax::GetBestSoFar() const
{
    if (IsDecisionFinished() || !Progress.IsValid())
    {
        return Super::GetBestSoFar();
    }

    // Still searching (or stopped early): the deepest ranking completed
    int32 DepthReached = 0;
    const TArray<FConnectItMinMaxRankedMove> RankedMoves = Progress->GetRankedMoves(&DepthReached);
    if (RankedMoves.IsEmpty())
    {
        return Super::GetBestSoFar();
    }

    return PrepareDecision(ConnectItMinMaxSearch::MakeDecision(RankedMoves,
        FString::Printf(TEXT("MinMax depth %d (search not finished)"), DepthReached), MaxCandidates));
}

void UConnectIt_AIStrategy_MinMax::CancelDecision_Implementation()
{
    if (CancelFlag.IsValid())
    {
        CancelFlag->store(true);
        CancelFlag.Reset();
    }
}

int32 UConnectIt_AIStrategy_MinMax::PickMoveIndex(const FConnectItMinMaxSearchResult& Result) const
{
    const TArray<FConnectItMinMaxRankedMove>& Moves = Result.RankedMoves;

    const int32 TopMoves = FMath::Min(TopMovesConsidered, Moves.Num());
    if (TopMoves > 1 && MistakeChance > 0.f && FMath::FRand() < MistakeChance)
    {
        return FMath::RandRange(0, TopMoves - 1);
    }

    int32 NumTiedBest = 1;
    while (NumTiedBest < Moves.Num() && Moves[NumTiedBest].Score == Moves[0].Score)
    {
        NumTiedBest++;
    }
    return FMath::RandRange(0, NumTiedBest - 1);
}
