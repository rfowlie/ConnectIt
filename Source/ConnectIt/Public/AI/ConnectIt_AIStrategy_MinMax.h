// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "AI/ConnectIt_MinMaxSearch.h"
#include "ConnectIt_AIStrategy_MinMax.generated.h"


// Classic ConnectIt opponent: one MinMax search per decision (see
// ConnectIt_MinMaxSearch.h -- the same search a Blueprint strategy can run
// with the "Find Best Moves (MinMax)" node), then its best moves as the
// decision's candidates. Two factions; it can only plan with Place Piece.
//
// Strength comes from the search settings (depth, time, what it values).
// Its candidates are the search's ranking, best first (a random one of
// equally-best moves first, for variety); deliberate mistakes are the
// selector's job (Top N). While the search runs, its best answer so far is
// the deepest ranking completed. All editor data, so a different-feeling
// MinMax opponent is different numbers, not a new class.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced,
    meta = (DisplayName = "MinMax (Classic)"))
class CONNECTIT_API UConnectIt_AIStrategy_MinMax : public UConnectIt_AIStrategy
{
    GENERATED_BODY()

public:

    // How to search: depth, time, what it values, which moves each side has
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|MinMax")
    FConnectItMinMaxSearchSettings Search;

    virtual FGameplayTagContainer GetSupportedOperationTags_Implementation() const override;

    // While the search runs: its deepest completed ranking so far
    virtual FConnectItAIDecision GetBestSoFar() const override;

protected:

    virtual void BeginDecision_Implementation(const FConnectItAIDecisionContext& Context) override;
    virtual void CancelDecision_Implementation() override;

    // Which ranked move goes first: a random one among the equal best scores
    int32 PickBestIndex(const FConnectItMinMaxSearchResult& Result) const;

private:

    // Shared with the running search task; null when none is running
    TSharedPtr<std::atomic<bool>, ESPMode::ThreadSafe> CancelFlag;

    // The current decision's search progress (written by the search task);
    // null if the search couldn't start
    FConnectItMinMaxSearchProgressPtr Progress;
};
