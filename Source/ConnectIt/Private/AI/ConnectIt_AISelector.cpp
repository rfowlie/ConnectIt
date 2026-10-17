// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AISelector.h"
#include "Algo/StableSort.h"
#include "Framework/Library/ConnectIt_BoardStateLibrary.h"


FConnectItAIDecision UConnectIt_AISelector::Select(
    const FConnectItAIDecisionContext& Context,
    const TArray<FConnectItAIStrategyAnswer>& Answers)
{
    if (RandomSeed != 0)
    {
        Random.Initialize(RandomSeed);
    }
    else
    {
        Random.GenerateNewSeed();
    }

    FConnectItAIDecision Decision;
    Decision.Candidates = RankCandidates(Context, Endorse(Answers));

    const int32 Keep = FMath::Max(1, MaxCandidates);
    if (Decision.Candidates.Num() > Keep)
    {
        Decision.Candidates.SetNum(Keep);
    }
    return Decision;
}

TArray<FConnectItAIEndorsedCandidate> UConnectIt_AISelector::Endorse(const TArray<FConnectItAIStrategyAnswer>& Answers)
{
    TArray<const FConnectItAIStrategyAnswer*> InPriorityOrder;
    for (const FConnectItAIStrategyAnswer& Answer : Answers)
    {
        InPriorityOrder.Add(&Answer);
    }
    Algo::StableSortBy(InPriorityOrder, [](const FConnectItAIStrategyAnswer* Answer) { return Answer->StrategyIndex; });

    TArray<FConnectItAIEndorsedCandidate> Endorsed;
    for (const FConnectItAIStrategyAnswer* Answer : InPriorityOrder)
    {
        // A strategy listing the same move twice still endorses it once
        TArray<int32> EndorsedByThisAnswer;
        for (const FConnectItAICandidate& Candidate : Answer->Decision.Candidates)
        {
            if (!Candidate.Payload.IsValid()) continue;

            const int32 Existing = Endorsed.IndexOfByPredicate(
                [&Candidate](const FConnectItAIEndorsedCandidate& Entry) { return Entry.Candidate.Payload == Candidate.Payload; });
            if (Existing == INDEX_NONE)
            {
                FConnectItAIEndorsedCandidate& Entry = Endorsed.AddDefaulted_GetRef();
                Entry.Candidate = Candidate;
                Entry.Endorsements = 1;
                Entry.FirstStrategyIndex = Answer->StrategyIndex;
                EndorsedByThisAnswer.Add(Endorsed.Num() - 1);
            }
            else if (!EndorsedByThisAnswer.Contains(Existing))
            {
                Endorsed[Existing].Endorsements++;
                EndorsedByThisAnswer.Add(Existing);
            }
        }
    }
    return Endorsed;
}

TArray<FConnectItAICandidate> UConnectIt_AISelector::RankCandidates_Implementation(
    const FConnectItAIDecisionContext& Context,
    const TArray<FConnectItAIEndorsedCandidate>& Candidates)
{
    return PickFirst(Candidates, 0);
}

TArray<FConnectItAICandidate> UConnectIt_AISelector::PickFirst(
    const TArray<FConnectItAIEndorsedCandidate>& Candidates, int32 PickIndex)
{
    TArray<FConnectItAICandidate> Ranked;
    Ranked.Reserve(Candidates.Num());
    if (Candidates.IsValidIndex(PickIndex))
    {
        Ranked.Add(Candidates[PickIndex].Candidate);
    }
    for (int32 Index = 0; Index < Candidates.Num(); Index++)
    {
        if (Index != PickIndex) Ranked.Add(Candidates[Index].Candidate);
    }
    return Ranked;
}

// --- Top N ---

TArray<FConnectItAICandidate> UConnectIt_AISelector_TopN::RankCandidates_Implementation(
    const FConnectItAIDecisionContext& Context,
    const TArray<FConnectItAIEndorsedCandidate>& Candidates)
{
    const int32 TopMoves = FMath::Min(TopMovesConsidered, Candidates.Num());
    int32 PickIndex = 0;
    if (TopMoves > 1 && MistakeChance > 0.f && Random.FRand() < MistakeChance)
    {
        PickIndex = Random.RandRange(1, TopMoves - 1);
    }
    return PickFirst(Candidates, PickIndex);
}

// --- Greedy scorer ---

TArray<FConnectItAICandidate> UConnectIt_AISelector_GreedyScorer::RankCandidates_Implementation(
    const FConnectItAIDecisionContext& Context,
    const TArray<FConnectItAIEndorsedCandidate>& Candidates)
{
    const auto OwnScore = [&Context](const FConnectItBoardState& Board)
    {
        return Board.ScoreBoard.IsValidIndex(Context.OwnSlot) ? Board.ScoreBoard[Context.OwnSlot] : 0.f;
    };
    const float ScoreBefore = OwnScore(Context.Board);

    struct FScored
    {
        int32 Index = 0;
        float Gain = 0.f;
    };

    TArray<FScored> Scored;
    Scored.Reserve(Candidates.Num());
    for (int32 Index = 0; Index < Candidates.Num(); Index++)
    {
        FConnectItBoardState ResultBoard;
        float PointsScored = 0.f;
        int32 NumScoredEvents = 0;
        FConnectItBoardChangeEvent Events;
        const bool bPossible = UConnectIt_BoardStateLibrary::SimulateOperation(
            Context.Board, Context.Rules, Candidates[Index].Candidate.Payload, Context.OwnSlot,
            ResultBoard, PointsScored, NumScoredEvents, Events);

        // The AI's own gain: a move that scores for the opponent gains nothing
        Scored.Add({ Index, bPossible ? OwnScore(ResultBoard) - ScoreBefore : TNumericLimits<float>::Lowest() });
    }

    // Stable: equal gains keep priority order
    Algo::StableSort(Scored, [](const FScored& A, const FScored& B) { return A.Gain > B.Gain; });

    TArray<FConnectItAICandidate> Ranked;
    Ranked.Reserve(Scored.Num());
    for (const FScored& Entry : Scored)
    {
        Ranked.Add(Candidates[Entry.Index].Candidate);
    }
    return Ranked;
}

// --- Weighted by confidence ---

TArray<FConnectItAICandidate> UConnectIt_AISelector_WeightedByConfidence::RankCandidates_Implementation(
    const FConnectItAIDecisionContext& Context,
    const TArray<FConnectItAIEndorsedCandidate>& Candidates)
{
    if (Candidates.IsEmpty()) return {};

    TArray<float> Confidences;
    Confidences.Reserve(Candidates.Num());
    for (const FConnectItAIEndorsedCandidate& Entry : Candidates)
    {
        Confidences.Add(UConnectIt_AIStrategy::CandidateHasConfidence(Entry.Candidate)
            ? FMath::Clamp(Entry.Candidate.Confidence, 0.f, 1.f)
            : MissingConfidence);
    }

    // exp((c - max) / T): the same odds as exp(c / T), without overflow
    const float SafeTemperature = FMath::Max(Temperature, 0.01f);
    const float MaxConfidence = FMath::Max(Confidences);
    TArray<double> Weights;
    double TotalWeight = 0.0;
    for (const float Confidence : Confidences)
    {
        const double Weight = FMath::Exp((Confidence - MaxConfidence) / SafeTemperature);
        Weights.Add(Weight);
        TotalWeight += Weight;
    }

    double Roll = Random.FRand() * TotalWeight;
    int32 PickIndex = Weights.Num() - 1;
    for (int32 Index = 0; Index < Weights.Num(); Index++)
    {
        Roll -= Weights[Index];
        if (Roll < 0.0)
        {
            PickIndex = Index;
            break;
        }
    }
    return PickFirst(Candidates, PickIndex);
}
