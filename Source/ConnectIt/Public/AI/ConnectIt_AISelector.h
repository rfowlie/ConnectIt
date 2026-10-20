// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Math/RandomStream.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "ConnectIt_AISelector.generated.h"


// One strategy's answer as a selector sees it: the decision only, never the
// strategy itself -- a selector judges moves, not who proposed them.
USTRUCT(BlueprintType)
struct FConnectItAIStrategyAnswer
{
    GENERATED_BODY()

    // The strategy's priority: 0 = first in the director's list
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    int32 StrategyIndex = INDEX_NONE;

    // The strategy's label (its candidates carry it too)
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    FName SourceLabel;

    // True: the strategy's final answer. False: its best so far when time ran out.
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    bool bFinished = false;

    UPROPERTY(BlueprintReadOnly, Category = "AI")
    FConnectItAIDecision Decision;
};

// A move after merging every strategy that proposed it (see Endorse)
USTRUCT(BlueprintType)
struct FConnectItAIEndorsedCandidate
{
    GENERATED_BODY()

    // As the highest-priority strategy proposing it gave it
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    FConnectItAICandidate Candidate;

    // How many strategies proposed this move
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    int32 Endorsements = 0;

    // The highest-priority strategy that proposed it (0 = first)
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    int32 FirstStrategyIndex = INDEX_NONE;
};

// Picks the AI's move from its strategies' answers. Configured inline (on the
// AI director); the director can swap it during a match. Subclass in C++ or
// Blueprint by overriding RankCandidates.
//
// Select runs one fixed pipeline: Endorse merges the answers into one list
// (a move several strategies proposed appears once, with a count), in
// priority order; RankCandidates orders it; the best MaxCandidates are kept.
// The controller then submits them in that order, falling back to the next
// on a rejected move.
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_AISelector : public UObject
{
    GENERATED_BODY()

public:

    // At most this many candidates are handed on: the chosen move plus
    // fallbacks for a rejected one
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 1))
    int32 MaxCandidates = 5;

    // Endorse -> RankCandidates -> keep MaxCandidates. Game thread.
    FConnectItAIDecision Select(
        const FConnectItAIDecisionContext& Context,
        const TArray<FConnectItAIStrategyAnswer>& Answers);

    // Every candidate of every answer, in priority order (strategy order,
    // then each strategy's own order), a move proposed by several
    // strategies merged into one entry that counts them
    UFUNCTION(BlueprintPure, Category = "AI")
    static TArray<FConnectItAIEndorsedCandidate> Endorse(const TArray<FConnectItAIStrategyAnswer>& Answers);

    // Fixes the random choices of the next selections (tests); 0 = a fresh
    // random seed per selection
    int32 RandomSeed = 0;

protected:

    // Order the candidates, best first (drop any you would never play).
    // Candidates arrive in priority order. Default: that order unchanged.
    UFUNCTION(BlueprintNativeEvent, Category = "AI")
    TArray<FConnectItAICandidate> RankCandidates(
        const FConnectItAIDecisionContext& Context,
        const TArray<FConnectItAIEndorsedCandidate>& Candidates);

    // The candidates in their given order, the one at PickIndex moved first
    static TArray<FConnectItAICandidate> PickFirst(
        const TArray<FConnectItAIEndorsedCandidate>& Candidates, int32 PickIndex);

    // Seeded at the start of each Select (see RandomSeed)
    FRandomStream Random;
};

// Plays the highest-priority move: the first strategy's best, falling back
// down the list. The default.
UCLASS(meta = (DisplayName = "First Available"))
class CONNECTIT_API UConnectIt_AISelector_FirstAvailable : public UConnectIt_AISelector
{
    GENERATED_BODY()
};

// Usually the best move; with MistakeChance, one of the next best instead --
// a difficulty knob for any strategy.
UCLASS(meta = (DisplayName = "Top N (Mistake Chance)"))
class CONNECTIT_API UConnectIt_AISelector_TopN : public UConnectIt_AISelector
{
    GENERATED_BODY()

public:

    // A mistake is a random pick among ranks 2..TopMovesConsidered.
    // 1 = never deviates from the best move.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 1))
    int32 TopMovesConsidered = 3;

    // Chance per decision of making a mistake
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 0.0, ClampMax = 1.0))
    float MistakeChance = 0.2f;

protected:

    virtual TArray<FConnectItAICandidate> RankCandidates_Implementation(
        const FConnectItAIDecisionContext& Context,
        const TArray<FConnectItAIEndorsedCandidate>& Candidates) override;
};

// Plays the move that gains the AI the most points right now, judged by
// simulating each candidate on the current board. Comparable across very
// different strategies; blind to anything beyond this move. Ties keep
// priority order; a move that can't be made goes last.
UCLASS(meta = (DisplayName = "Greedy Scorer"))
class CONNECTIT_API UConnectIt_AISelector_GreedyScorer : public UConnectIt_AISelector
{
    GENERATED_BODY()

protected:

    virtual TArray<FConnectItAICandidate> RankCandidates_Implementation(
        const FConnectItAIDecisionContext& Context,
        const TArray<FConnectItAIEndorsedCandidate>& Candidates) override;
};

// Picks at random, better moves more likely: each candidate's chance grows
// with its Confidence. Temperature sets how strongly -- near 0 almost always
// the most confident move, high nearly uniform. Smooth difficulty.
UCLASS(meta = (DisplayName = "Weighted by Confidence"))
class CONNECTIT_API UConnectIt_AISelector_WeightedByConfidence : public UConnectIt_AISelector
{
    GENERATED_BODY()

public:

    // Weight = exp(Confidence / Temperature)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 0.01))
    float Temperature = 0.2f;

    // The confidence assumed for a candidate whose strategy gave none
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 0.0, ClampMax = 1.0))
    float MissingConfidence = 0.5f;

protected:

    virtual TArray<FConnectItAICandidate> RankCandidates_Implementation(
        const FConnectItAIDecisionContext& Context,
        const TArray<FConnectItAIEndorsedCandidate>& Candidates) override;
};
