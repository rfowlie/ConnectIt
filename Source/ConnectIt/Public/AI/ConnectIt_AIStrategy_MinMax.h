// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "MinMax/ConnectIt_MinMaxRules.h"
#include "MinMax/ConnectIt_MinMaxWeights.h"
#include <atomic>
#include "ConnectIt_AIStrategy_MinMax.generated.h"


// Classic ConnectIt opponent: a negamax search
// (GameIntelligence::Search::MinMax::TAlphaBeta over FConnectItMinMaxRules)
// on a background task. Two factions, one PlacePiece per turn.
//
// Strength comes from MaxDepth / TimeBudgetSeconds; TopMovesConsidered +
// MistakeChance let it deliberately play a weaker-but-reasonable move;
// EvaluationWeights tune what it values and OrderingWeights how it guesses
// which moves to try first (see ConnectIt_MinMaxWeights.h) -- all editor data,
// so a different-feeling MinMax opponent is different numbers, not a new class.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced,
    meta = (DisplayName = "MinMax (Classic)"))
class CONNECTIT_API UConnectIt_AIStrategy_MinMax : public UConnectIt_AIStrategy
{
    GENERATED_BODY()

public:

    // Deepest search, in plies (one ply = one placement by either side)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|MinMax",
        meta = (ClampMin = 1, ClampMax = 10))
    int32 MaxDepth = 4;

    // Search time per decision; the search stops deepening when it runs out
    // and plays the best move from the deepest completed depth. 0 = no limit
    // (always reaches MaxDepth).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|MinMax",
        meta = (ClampMin = 0.0, Units = "s"))
    float TimeBudgetSeconds = 1.5f;

    // When the AI makes a "mistake" it picks uniformly among this many of
    // its best-scoring moves. 1 = never deviates from the best move.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|MinMax",
        meta = (ClampMin = 1))
    int32 TopMovesConsidered = 1;

    // Chance per decision of picking among TopMovesConsidered instead of the
    // best move
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|MinMax",
        meta = (ClampMin = 0.0, ClampMax = 1.0))
    float MistakeChance = 0.f;

    // What the AI values in an unfinished position. Win/loss is always
    // checked first, outside these.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|MinMax|Evaluation")
    FConnectItMinMaxEvaluationWeights EvaluationWeights;

    // Which moves to try first (speed only -- never changes which move wins)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|MinMax|Move Ordering")
    FConnectItMinMaxOrderingWeights OrderingWeights;

protected:

    virtual void BeginDecision_Implementation(const FConnectItAIDecisionContext& Context) override;
    virtual void CancelDecision_Implementation() override;

    // Which root move to play: normally the best (a random one among equal
    // best scores); with MistakeChance, a random one of the top
    // TopMovesConsidered.
    int32 PickMoveIndex(const FConnectItMinMaxSearch::FResult& Result) const;

private:

    // Shared with the running search task
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag;
};
